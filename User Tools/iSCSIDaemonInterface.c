/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemonInterface.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Defines interface used by client applications to access
 *              the iSCSIDaemon
 */


#include "iSCSIDaemonInterface.h"
#include "iSCSIDaemonInterfaceShared.h"

const struct iSCSIDCmdLoginSession iSCSIDCmdLoginSessionInit  = {
    .funcCode = kiSCSIDLoginSession,
    .portalLength = 0,
    .targetLength = 0,
    .authLength   = 0
};

const struct iSCSIDCmdLogoutSession iSCSIDCmdLogoutSessionInit  = {
    .funcCode = kiSCSIDLogoutSession
};

const struct iSCSIDCmdLoginConnection iSCSIDCmdLoginConnectionInit  = {
    .funcCode = kiSCSIDLoginConnection,
};

const struct iSCSIDCmdLogoutConnection iSCSIDCmdLogoutConnectionInit  = {
    .funcCode = kiSCSIDLogoutConnection,
};

const struct iSCSIDCmdQueryPortalForTargets iSCSIDCmdQueryPortalForTargetsInit  = {
    .funcCode = kiSCSIDQueryPortalForTargets,
};

const struct iSCSIDCmdQueryTargetForAuthMethod iSCSIDCmdQueryTargetForAuthMethodInit  = {
    .funcCode = kiSCSIDQueryTargetForAuthMethod,
};

const struct iSCSIDCmdGetSessionIdForTarget iSCSIDCmdGetSessionIdForTargetInit  = {
    .funcCode = kiSCSIDGetSessionIdForTarget,
};

const struct iSCSIDCmdGetConnectionIdForAddress iSCSIDCmdGetConnectionIdForAddressInit  = {
    .funcCode = kiSCSIDGetConnectionIdForAddress,
};

const struct iSCSIDCmdGetSessionIds iSCSIDCmdGetSessionIdsInit  = {
    .funcCode = kiSCSIDGetSessionIds,
};

const struct iSCSIDCmdGetConnectionIds iSCSIDCmdGetConnectionIdsInit  = {
    .funcCode = kiSCSIDGetConnectionIds,
};

const struct iSCSIDCmdGetSessionInfo iSCSIDCmdGetSessionInfoInit  = {
    .funcCode = kiSCSIDGetSessionInfo,
    .sessionId = kiSCSIInvalidSessionId
};

const struct iSCSIDCmdGetConnectionInfo iSCSIDCmdGetConnectionInfoInit  = {
    .funcCode = kiSCSIDGetConnectionInfo,
    .sessionId = kiSCSIInvalidSessionId,
    .connectionId = kiSCSIInvalidConnectionId
};



iSCSIDaemonHandle iSCSIDaemonConnect()
{
    // Create a new socket and connect to the daemon
    iSCSIDaemonHandle handle = socket(PF_LOCAL,SOCK_STREAM,0);
    struct sockaddr_un address;
    address.sun_family = AF_LOCAL;
    strcpy(address.sun_path,"/tmp/iscsid_local");

    // Connect to local socket and return handle
    if(connect(handle,(const struct sockaddr *)&address,(socklen_t)SUN_LEN(&address))==0)
        return handle;
    
    close(handle);
    return -1;
}

void iSCSIDaemonDisconnect(iSCSIDaemonHandle handle)
{
    if(handle >= 0)
        close(handle);
}

/*! Helper function.  Sends an iSCSI daemon command followed by optional
 *  data in the form of CFDataRef objects. */
errno_t iSCSIDaemonSendCmdWithData(iSCSIDaemonHandle handle,iSCSIDCmd * cmd,...)
{
    va_list argList;
    va_start(argList,cmd);
    
    // Send command header
    if(send(handle,cmd,sizeof(iSCSIDCmd),0) != sizeof(iSCSIDCmd))
        return EAGAIN;
    
    // Iterate over data and send each one...
    CFDataRef data = NULL;
    while((data = va_arg(argList,CFDataRef)))
    {
        CFIndex length = CFDataGetLength(data);
        if(send(handle,CFDataGetBytePtr(data),length,0) != length)
        {
            va_end(argList);
            return EAGAIN;
        }
    }
    va_end(argList);
    return 0;
}

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param handle a handle to a daemon connection.
 *  @param portal specifies the portal to use for the new session.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginSession(iSCSIDaemonHandle handle,
                           iSCSIPortalRef portal,
                           iSCSITargetRef target,
                           iSCSIAuthRef auth,
                           SID * sessionId,
                           CID * connectionId,
                           enum iSCSILoginStatusCode * statusCode)
{
    if(handle < 0 || !portal || !target || !auth || !sessionId || !connectionId || !statusCode)
        return EINVAL;
    
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef authData   = iSCSIAuthCreateData(auth);
    
    iSCSIDCmdLoginSession cmd = iSCSIDCmdLoginSessionInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.authLength = (UInt32)CFDataGetLength(authData);

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               portalData,targetData,authData,NULL);
    
    CFRelease(portalData);
    CFRelease(targetData);
    CFRelease(authData);
    
    if(error)
        return error;

    iSCSIDRspLoginSession rsp;
    
    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(rsp.funcCode != kiSCSIDLoginSession)
        return EAGAIN;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    *sessionId  = rsp.sessionId;
    
    return 0;
}

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to free.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogoutSession(iSCSIDaemonHandle handle,
                                 SID sessionId,
                                 enum iSCSILogoutStatusCode * statusCode)
{
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    iSCSIDCmdLogoutSession cmd = iSCSIDCmdLogoutSessionInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EAGAIN;
    
    iSCSIDRspLogoutSession rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    
    return rsp.errorCode;
}

/*! Adds a new connection to an iSCSI session.
 *  @param handle a handle to a daemon connection.
 *  @param portal specifies the portal to use for the connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginConnection(iSCSIDaemonHandle handle,
                                   iSCSIPortalRef portal,
                                   iSCSITargetRef target,
                                   iSCSIAuthRef auth,
                                   SID sessionId,
                                   CID * connectionId,
                                   enum iSCSILoginStatusCode * statusCode)
{
    return 0;
}

/*! Removes a connection from an existing session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogoutConnection(iSCSIDaemonHandle handle,
                                    SID sessionId,
                                    CID connectionId,
                                    enum iSCSILogoutStatusCode * statusCode)
{
    return 0;
}

/*! Queries a portal for available targets.
 *  @param handle a handle to a daemon connection.
 *  @param portal the iSCSI portal to query.
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonQueryPortalForTargets(iSCSIDaemonHandle handle,
                                         iSCSIPortalRef portal,
                                         iSCSIMutableDiscoveryRecRef * discoveryRec,
                                         enum iSCSILoginStatusCode * statusCode)
{
    // Validate inputs
    if(handle < 0 || !portal || !discoveryRec || !statusCode)
        return EINVAL;
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    
    // Create command header to transmit
    iSCSIDCmdQueryPortalForTargets cmd = iSCSIDCmdQueryPortalForTargetsInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,portalData,NULL))
    {
        CFRelease(portalData);
        return EAGAIN;
    }

    CFRelease(portalData);
    iSCSIDRspQueryPortalForTargets rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    // Retrieve the discovery record...
    UInt8 * bytes = malloc(rsp.discoveryLength);
    
    if(bytes && recv(handle,bytes,rsp.discoveryLength,0) != sizeof(rsp.discoveryLength))
    {
        free(bytes);
        return EAGAIN;
    }
    
    CFDataRef discData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,bytes,
                                                     rsp.discoveryLength,
                                                     kCFAllocatorMalloc);
    
    if(!(*discoveryRec = iSCSIMutableDiscoveryRecCreateWithData(discData)))
        return EAGAIN;
    
    CFRelease(discData);
    
    *statusCode = rsp.statusCode;
    return rsp.errorCode;
}


/*! Retrieves a list of targets available from a give portal.
 *  @param handle a handle to a daemon connection.
 *  @param portal the iSCSI portal to look for targets.
 *  @param authMethod the preferred authentication method.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonQueryTargetForAuthMethod(iSCSIDaemonHandle handle,
                                            iSCSIPortalRef portal,
                                            CFStringRef targetName,
                                            enum iSCSIAuthMethods * authMethod,
                                            enum iSCSILoginStatusCode * statusCode)
{
    // Validate inputs
    if(handle < 0 || !portal || !targetName || !authMethod || !statusCode)
        return EINVAL;
    
    // Setup a target object with the target name
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,targetName);
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef targetData = iSCSITargetCreateData(target);
    iSCSITargetRelease(target);
    
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    
    // Create command header to transmit
    iSCSIDCmdQueryTargetForAuthMethod cmd = iSCSIDCmdQueryTargetForAuthMethodInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,portalData,targetData,NULL))
    {
        CFRelease(portalData);
        CFRelease(targetData);
        return EAGAIN;
    }

    CFRelease(portalData);
    CFRelease(targetData);
    iSCSIDRspQueryTargetForAuthMethod rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    *authMethod = rsp.authMethod;
    *statusCode = rsp.statusCode;
    
    return rsp.errorCode;
}


/*! Retreives the initiator session identifier associated with this target.
 *  @param handle a handle to a daemon connection.
 *  @param targetName the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonGetSessionIdForTarget(iSCSIDaemonHandle handle,
                                         CFStringRef targetName,
                                         SID * sessionId)
{
    // Validate inputs
    if(handle < 0 || !targetName || !sessionId)
        return EINVAL;
    
    // Setup a target object with the target name
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,targetName);
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef targetData = iSCSITargetCreateData(target);
    iSCSITargetRelease(target);
    
    // Create command header to transmit
    iSCSIDCmdGetSessionIdForTarget cmd = iSCSIDCmdGetSessionIdForTargetInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,targetData,NULL))
    {
        CFRelease(targetData);
        return EAGAIN;
    }

    CFRelease(targetData);
    iSCSIDRspGetSessionIdForTarget rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    *sessionId = rsp.sessionId;

    return rsp.errorCode;
}


/*! Looks up the connection identifier associated with a particular connection address.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param address the name used when adding the connection (e.g., IP or DNS).
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionIdFromAddress(iSCSIDaemonHandle handle,
                                              SID sessionId,
                                              CFStringRef address,
                                              CID * connectionId)
{
    // Validate inputs
    if(handle < 0 || !address || sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;
    
    // Setup a portal object with the target name
    iSCSIMutablePortalRef portal = iSCSIMutablePortalCreate();
    iSCSIPortalSetAddress(portal,address);
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    iSCSIPortalRelease(portal);
    
    // Create command header to transmit
    iSCSIDCmdGetConnectionIdForAddress cmd = iSCSIDCmdGetConnectionIdForAddressInit;
    cmd.sessionId = sessionId;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,portalData,NULL))
    {
        CFRelease(portalData);
        return EAGAIN;
    }
    
    CFRelease(portalData);
    iSCSIDRspGetConnectionIdForAddress rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    *connectionId = rsp.connectionId;
    
    return rsp.errorCode;
}


/*! Gets an array of session identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionIds an array of session identifiers.
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxSessions.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetSessionIds(iSCSIDaemonHandle handle,
                                 SID * sessionIds,
                                 UInt16 * sessionCount)
{
    // Validate inputs
    if(handle < 0 || !sessionIds || !sessionCount)
        return EINVAL;
    
    // Send command to daemon
    iSCSIDCmdGetSessionIds cmd = iSCSIDCmdGetSessionIdsInit;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EAGAIN;
    
    // Receive daemon response header
    iSCSIDRspGetSessionIds rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    *sessionCount = rsp.sessionCount;
    size_t dataLength = sizeof(SID)*kiSCSIMaxSessions;
    
    if(recv(handle,sessionIds,dataLength,0) != dataLength)
        return EAGAIN;
    
    return rsp.errorCode;
}


/*! Gets an array of connection identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxConnectionsPerSession.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionIds(iSCSIDaemonHandle handle,
                                    SID sessionId,
                                    UInt32 * connectionIds,
                                    UInt32 * connectionCount)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId || !connectionIds || !connectionCount)
        return EINVAL;
    
    // Send command to daemon
    iSCSIDCmdGetConnectionIds cmd = iSCSIDCmdGetConnectionIdsInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EAGAIN;
    
    // Receive daemon response header
    iSCSIDRspGetConnectionIds rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    *connectionCount = rsp.connectionCount;
    size_t dataLength = sizeof(CID)*kiSCSIMaxConnectionsPerSession;
    
    if(recv(handle,connectionIds,dataLength,0) != dataLength)
        return EAGAIN;
    
    return rsp.errorCode;
}


/*! Gets information about a particular session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param options the optionts for the specified session.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetSessionInfo(iSCSIDaemonHandle handle,
                                  SID sessionId,
                                  iSCSISessionOptions * options)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId || !options)
        return EINVAL;
    
    // Send command to daemon
    iSCSIDCmdGetSessionInfo cmd = iSCSIDCmdGetSessionInfoInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EAGAIN;
    
    // Receive daemon response header
    iSCSIDRspGetSessionInfo rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    // We expect the data payload that follows the header to be an struct
    if(rsp.dataLength != sizeof(iSCSISessionOptions))
        return rsp.errorCode;
    
    // Get the session information struct
    if(recv(handle,options,sizeof(iSCSISessionOptions),0) != sizeof(iSCSISessionOptions))
        return EAGAIN;
    
    return rsp.errorCode;
}


/*! Gets information about a particular session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @param options the optionts for the specified session.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionInfo(iSCSIDaemonHandle handle,
                                     SID sessionId,
                                     CID connectionId,
                                     iSCSIConnectionOptions * options)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId || !options)
        return EINVAL;
    
    // Send command to daemon
    iSCSIDCmdGetConnectionInfo cmd = iSCSIDCmdGetConnectionInfoInit;
    cmd.sessionId = sessionId;
    cmd.connectionId = connectionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EAGAIN;
    
    // Receive daemon response header
    iSCSIDRspGetSessionInfo rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    // We expect the data payload that follows the header to be an struct
    if(rsp.dataLength != sizeof(iSCSIConnectionOptions))
        return rsp.errorCode;
    
    // Get the session information struct
    if(recv(handle,options,sizeof(iSCSIConnectionOptions),0) != sizeof(iSCSIConnectionOptions))
        return EAGAIN;
    
    return rsp.errorCode;
}
