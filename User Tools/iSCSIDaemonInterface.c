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

const struct iSCSIDCmdShutdown iSCSIDCmdShutdownInit = {
    .funcCode = kiSCSIDShutdownDaemon
};

const struct iSCSIDCmdLoginSession iSCSIDCmdLoginSessionInit  = {
    .funcCode = kiSCSIDLoginSession,
    .portalLength = 0,
    .targetLength = 0,
    .authLength   = 0,
    .sessCfgLength = 0,
    .connCfgLength = 0
};

const struct iSCSIDCmdLogoutSession iSCSIDCmdLogoutSessionInit  = {
    .funcCode = kiSCSIDLogoutSession
};

const struct iSCSIDCmdLoginConnection iSCSIDCmdLoginConnectionInit  = {
    .funcCode = kiSCSIDLoginConnection,
    .sessionId = kiSCSIInvalidSessionId,
    .portalLength = 0,
    .authLength = 0,
    .connCfgLength = 0
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

const struct iSCSIDCmdGetConnectionIdForPortal iSCSIDCmdGetConnectionIdForPortalInit  = {
    .funcCode = kiSCSIDGetConnectionIdForPortal,
};

const struct iSCSIDCmdGetSessionIds iSCSIDCmdGetSessionIdsInit  = {
    .funcCode = kiSCSIDGetSessionIds,
};

const struct iSCSIDCmdGetConnectionIds iSCSIDCmdGetConnectionIdsInit  = {
    .funcCode = kiSCSIDGetConnectionIds,
};

const struct iSCSIDCmdCreateTargetForSessionId iSCSIDCmdCreateTargetForSessionIdInit = {
    .funcCode = kiSCSIDCreateTargetForSessionId,
    .sessionId = kiSCSIInvalidSessionId,
};

const struct iSCSIDCmdCreatePortalForConnectionId iSCSIDCmdCreatePortalForConnectionIdInit = {
    .funcCode = kiSCSIDCreatePortalForConnectionId,
    .sessionId = kiSCSIInvalidSessionId,
    .connectionId = kiSCSIInvalidConnectionId
};

const struct iSCSIDCmdCopySessionConfig iSCSIDCmdCopySessionConfigInit  = {
    .funcCode = kiSCSIDCopySessionConfig,
    .sessionId = kiSCSIInvalidSessionId
};

const struct iSCSIDCmdCopyConnectionConfig iSCSIDCmdCopyConnectionConfigInit  = {
    .funcCode = kiSCSIDCopyConnectionConfig,
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
    
    // Set the timeout for this socket so that the client will quit if
    // the daemon isn't running (otherwise it will hang on the non-blocking
    // socket
/*    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(handle,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval));
    setsockopt(handle,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));
*/
    // Connect to local socket and return handle
    if(connect(handle,(const struct sockaddr *)&address,(socklen_t)SUN_LEN(&address))==0)
        return handle;
    
    close(handle);
    return -1;
}

void iSCSIDaemonDisconnect(iSCSIDaemonHandle handle)
{
    // Tell daemon to shut down
    iSCSIDCmdShutdown cmd = iSCSIDCmdShutdownInit;
    send(handle,&cmd,sizeof(iSCSIDCmd),0);
    
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
        return EIO;
    
    // Iterate over data and send each one...
    CFDataRef data = NULL;
    while((data = va_arg(argList,CFDataRef)))
    {
        CFIndex length = CFDataGetLength(data);
        if(send(handle,CFDataGetBytePtr(data),length,0) != length)
        {
            va_end(argList);
            return EIO;
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
 *  @param sessCfg the session configuration parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginSession(iSCSIDaemonHandle handle,
                                iSCSIPortalRef portal,
                                iSCSITargetRef target,
                                iSCSIAuthRef auth,
                                iSCSISessionConfigRef sessCfg,
                                iSCSIConnectionConfigRef connCfg,
                                SID * sessionId,
                                CID * connectionId,
                                enum iSCSILoginStatusCode * statusCode)
{
    if(handle < 0 || !portal || !target || !auth || !sessionId || !connectionId || !statusCode)
        return EINVAL;
    
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef authData   = iSCSIAuthCreateData(auth);
    CFDataRef sessCfgData = iSCSISessionConfigCreateData(sessCfg);
    CFDataRef connCfgData = iSCSIConnectionConfigCreateData(connCfg);
    
    iSCSIDCmdLoginSession cmd = iSCSIDCmdLoginSessionInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.authLength = (UInt32)CFDataGetLength(authData);
    cmd.sessCfgLength = (UInt32)CFDataGetLength(sessCfgData);
    cmd.connCfgLength = (UInt32)CFDataGetLength(connCfgData);

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               portalData,targetData,authData,sessCfgData,connCfgData,NULL);
    
    CFRelease(portalData);
    CFRelease(targetData);
    CFRelease(authData);
    CFRelease(sessCfgData);
    CFRelease(connCfgData);
    
    if(error)
        return error;

    iSCSIDRspLoginSession rsp;
    
    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    if(rsp.funcCode != kiSCSIDLoginSession)
        return EIO;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    *sessionId  = rsp.sessionId;
    
    return rsp.errorCode;
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
        return EIO;
    
    iSCSIDRspLogoutSession rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    
    return rsp.errorCode;
}


/*! Adds a new connection to an iSCSI session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the new session identifier.
 *  @param portal specifies the portal to use for the connection.
 *  @param auth specifies the authentication parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginConnection(iSCSIDaemonHandle handle,
                                   SID sessionId,
                                   iSCSIPortalRef portal,
                                   iSCSIAuthRef auth,
                                   iSCSIConnectionConfigRef connCfg,
                                   CID * connectionId,
                                   enum iSCSILoginStatusCode * statusCode)

{
    if(handle < 0 || !portal || !auth || !connectionId || !statusCode)
        return EINVAL;
    
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    CFDataRef authData   = iSCSIAuthCreateData(auth);
    CFDataRef connCfgData = iSCSIConnectionConfigCreateData(connCfg);
    
    iSCSIDCmdLoginConnection cmd = iSCSIDCmdLoginConnectionInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.authLength = (UInt32)CFDataGetLength(authData);
    cmd.connCfgLength = (UInt32)CFDataGetLength(connCfgData);
    cmd.sessionId = sessionId;

    
    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               portalData,authData,connCfgData,NULL);
    
    CFRelease(portalData);
    CFRelease(authData);
    CFRelease(connCfgData);
    
    if(error)
        return error;
    
    iSCSIDRspLoginSession rsp;
    
    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    if(rsp.funcCode != kiSCSIDLoginSession)
        return EIO;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    *connectionId  = rsp.connectionId;
    
    return rsp.errorCode;
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
 *  @param auth specifies the authentication parameters to use.
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonQueryPortalForTargets(iSCSIDaemonHandle handle,
                                         iSCSIPortalRef portal,
                                         iSCSIAuthRef auth,
                                         iSCSIMutableDiscoveryRecRef * discoveryRec,
                                         enum iSCSILoginStatusCode * statusCode)
{
    // Validate inputs
    if(handle < 0 || !portal || !auth || !discoveryRec || !statusCode)
        return EINVAL;
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    CFDataRef authData = iSCSIAuthCreateData(auth);
    
    // Create command header to transmit
    iSCSIDCmdQueryPortalForTargets cmd = iSCSIDCmdQueryPortalForTargetsInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.authLength = (UInt32)CFDataGetLength(authData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,portalData,authData,NULL))
    {
        CFRelease(portalData);
        CFRelease(authData);
        return EIO;
    }

    CFRelease(portalData);
    CFRelease(authData);
    iSCSIDRspQueryPortalForTargets rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    // Retrieve the discovery record...
    UInt8 * bytes = malloc(rsp.discoveryLength);
    
    if(bytes && (recv(handle,bytes,rsp.discoveryLength,0) != rsp.discoveryLength))
    {
        free(bytes);
        return EIO;
    }
    
    CFDataRef discData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,bytes,
                                                     rsp.discoveryLength,
                                                     kCFAllocatorMalloc);
    
    if(!(*discoveryRec = iSCSIDiscoveryRecCreateMutableWithData(discData)))
        return EIO;
    
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
                                            CFStringRef targetIQN,
                                            enum iSCSIAuthMethods * authMethod,
                                            enum iSCSILoginStatusCode * statusCode)
{
    // Validate inputs
    if(handle < 0 || !portal || !targetIQN || !authMethod || !statusCode)
        return EINVAL;
    
    // Setup a target object with the target name
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetName(target,targetIQN);
    
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
        return EIO;
    }

    CFRelease(portalData);
    CFRelease(targetData);
    iSCSIDRspQueryTargetForAuthMethod rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    *authMethod = rsp.authMethod;
    *statusCode = rsp.statusCode;
    
    return rsp.errorCode;
}


/*! Retreives the initiator session identifier associated with this target.
 *  @param handle a handle to a daemon connection.
 *  @param targetIQN the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonGetSessionIdForTarget(iSCSIDaemonHandle handle,
                                         CFStringRef targetIQN,
                                         SID * sessionId)
{
    // Validate inputs
    if(handle < 0 || !targetIQN || !sessionId)
        return EINVAL;
    
    // Setup a target object with the target name
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetName(target,targetIQN);
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef targetData = iSCSITargetCreateData(target);
    iSCSITargetRelease(target);
    
    // Create command header to transmit
    iSCSIDCmdGetSessionIdForTarget cmd = iSCSIDCmdGetSessionIdForTargetInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,targetData,NULL))
    {
        CFRelease(targetData);
        return EIO;
    }

    CFRelease(targetData);
    iSCSIDRspGetSessionIdForTarget rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;

    *sessionId = rsp.sessionId;

    return rsp.errorCode;
}

/*! Looks up the connection identifier associated with a portal.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param portal the iSCSI portal.
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionIdForPortal(iSCSIDaemonHandle handle,
                                            SID sessionId,
                                            iSCSIPortalRef portal,
                                            CID * connectionId)
{
    // Validate inputs
    if(handle < 0 || !portal || sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    
    // Create command header to transmit
    iSCSIDCmdGetConnectionIdForPortal cmd = iSCSIDCmdGetConnectionIdForPortalInit;
    cmd.sessionId = sessionId;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,portalData,NULL))
    {
        CFRelease(portalData);
        return EIO;
    }
    
    CFRelease(portalData);
    iSCSIDRspGetConnectionIdForPortal rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    *connectionId = rsp.connectionId;
    
    return rsp.errorCode;
}


/*! Gets an array of session identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @return an array of session identifiers. */
CFArrayRef iSCSIDaemonCreateArrayOfSessionIds(iSCSIDaemonHandle handle)
{
    // Validate inputs
    if(handle < 0)
        return NULL;
    
    // Send command to daemon
    iSCSIDCmdGetSessionIds cmd = iSCSIDCmdGetSessionIdsInit;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;
    
    // Receive daemon response header
    iSCSIDRspGetSessionIds rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return NULL;
    
    if(rsp.errorCode || rsp.dataLength == 0)
        return NULL;
    
    void * bytes = malloc(rsp.dataLength);
    
    if(recv(handle,bytes,rsp.dataLength,0) != rsp.dataLength)
    {
        free(bytes);
        return NULL;
    }

    CFArrayRef sessionIds = CFArrayCreate(kCFAllocatorDefault,bytes,rsp.dataLength/sizeof(void *),NULL);
    free(bytes);
    return sessionIds;
}

/*! Gets an array of connection identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId session identifier.
 *  @return an array of connection identifiers. */
CFArrayRef iSCSIDaemonCreateArrayOfConnectionsIds(iSCSIDaemonHandle handle,SID sessionId)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    // Send command to daemon
    iSCSIDCmdGetConnectionIds cmd = iSCSIDCmdGetConnectionIdsInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;
    
    // Receive daemon response header
    iSCSIDRspGetConnectionIds rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return NULL;
    
    if(rsp.errorCode || rsp.dataLength == 0)
        return NULL;
    
    void * bytes = malloc(rsp.dataLength);
    
    if(recv(handle,bytes,rsp.dataLength,0) != rsp.dataLength)
    {
        free(bytes);
        return NULL;
    }
    
    CFArrayRef connectionIds = CFArrayCreate(kCFAllocatorDefault,bytes,rsp.dataLength/sizeof(void *),NULL);
    free(bytes);
    return connectionIds;
}

/*! Creates a target object for the specified session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @return target the target object. */
iSCSITargetRef iSCSIDaemonCreateTargetForSessionId(iSCSIDaemonHandle handle,
                                                   SID sessionId)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    // Send command to daemon
    iSCSIDCmdCreateTargetForSessionId cmd = iSCSIDCmdCreateTargetForSessionIdInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;
    
    // Receive daemon response header
    iSCSIDRspCreateTargetForSessionId rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return NULL;
    
    if(rsp.funcCode != kiSCSIDCreateTargetForSessionId || rsp.targetLength == 0)
        return NULL;
    
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(handle,rsp.targetLength,
                            (void *(* )(CFDataRef))&iSCSITargetCreateWithData);
    
    return target;
}

/*! Creates a connection object for the specified connection.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @return portal information about the portal. */
iSCSIPortalRef iSCSIDaemonCreatePortalForConnectionId(iSCSIDaemonHandle handle,
                                                      SID sessionId,
                                                      CID connectionId)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    // Send command to daemon
    iSCSIDCmdCreatePortalForConnectionId cmd = iSCSIDCmdCreatePortalForConnectionIdInit;
    cmd.sessionId = sessionId;
    cmd.connectionId = connectionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;
    
    // Receive daemon response header
    iSCSIDRspCreatePortalForConnectionId rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return NULL;
    
    if(rsp.funcCode != kiSCSIDCreatePortalForConnectionId || rsp.portalLength == 0)
        return NULL;
    
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(handle,rsp.portalLength,
                            (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);
    
    return portal;
}

/*! Copies the configuration object associated with a particular session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @return  the configuration object associated with the specified session. */
iSCSISessionConfigRef iSCSIDaemonCopySessionConfig(iSCSIDaemonHandle handle,
                                                   SID sessionId)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    // Send command to daemon
    iSCSIDCmdCopySessionConfig cmd = iSCSIDCmdCopySessionConfigInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;
    
    // Receive daemon response header
    iSCSIDRspCopySessionConfig rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp) || rsp.dataLength == 0)
        return NULL;
    
    // Get the session information struct
    return iSCSIDCreateObjectFromSocket(handle,rsp.dataLength,(void *(* )(CFDataRef))&iSCSISessionConfigCreateWithData);
}

/*! Copies the configuration object associated with a particular connection.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @return  the configuration object associated with the specified connection. */
iSCSIConnectionConfigRef iSCSIDaemonCopyConnectionConfig(iSCSIDaemonHandle handle,
                                                         SID sessionId,
                                                         CID connectionId)
{
    // Validate inputs
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    // Send command to daemon
    iSCSIDCmdCopyConnectionConfig cmd = iSCSIDCmdCopyConnectionConfigInit;
    cmd.sessionId = sessionId;
    cmd.connectionId = connectionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;
    
    // Receive daemon response header
    iSCSIDRspCopySessionConfig rsp;
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp) || rsp.dataLength == 0)
        return NULL;
    
    // Get the session information struct
    return iSCSIDCreateObjectFromSocket(handle,rsp.dataLength,(void *(* )(CFDataRef))&iSCSIConnectionConfigCreateWithData);
}