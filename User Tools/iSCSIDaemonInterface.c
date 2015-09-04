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

const struct iSCSIDCmdLogin iSCSIDCmdLoginInit  = {
    .funcCode = kiSCSIDLogin,
    .portalLength = 0,
    .targetLength = 0
};

const struct iSCSIDCmdLogout iSCSIDCmdLogoutInit  = {
    .funcCode = kiSCSIDLogout,
    .targetLength = 0,
    .portalLength = 0
};

const struct iSCSIDCmdCreateArrayOfActiveTargets iSCSIDCmdCreateArrayOfActiveTargetsInit  = {
    .funcCode = kiSCSIDCreateArrayOfActiveTargets,
};

const struct iSCSIDCmdCreateArrayOfActivePortalsForTarget iSCSIDCmdCreateArrayOfActivePortalsForTargetInit  = {
    .funcCode = kiSCSIDCreateArrayOfActivePortalsForTarget,
};

const struct iSCSIDCmdIsTargetActive iSCSIDCmdIsTargetActiveInit  = {
    .funcCode = kiSCSIDIsTargetActive,
    .targetLength = 0
};

const struct iSCSIDCmdIsPortalActive iSCSIDCmdIsPortalActiveInit  = {
    .funcCode = kiSCSIDIsPortalActive,
    .portalLength = 0
};

const struct iSCSIDCmdQueryPortalForTargets iSCSIDCmdQueryPortalForTargetsInit  = {
    .funcCode = kiSCSIDQueryPortalForTargets,
};

const struct iSCSIDCmdQueryTargetForAuthMethod iSCSIDCmdQueryTargetForAuthMethodInit  = {
    .funcCode = kiSCSIDQueryTargetForAuthMethod,
};

const struct iSCSIDCmdCreateCFPropertiesForSession iSCSIDCmdCreateCFPropertiesForSessionInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForSession,
    .targetLength = 0
};

const struct iSCSIDCmdCreateCFPropertiesForConnection iSCSIDCmdCreateCFPropertiesForConnectionInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForConnection,
    .targetLength = 0,
    .portalLength = 0
};

const struct iSCSIDCmdToggleSendTargetsDiscovery iSCSIDCmdToggleSendTargetsDiscoveryInit = {
    .funcCode = kiSCSIDToggleSendTargetsDiscovery,
    .enable = 0,
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

// TODO: set timeout...

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


/*! Logs into a target using a specific portal or all portals in the database.
 *  If an argument is supplied for portal, login occurs over the specified
 *  portal.  Otherwise, the daemon will attempt to login over all portals.
 *  @param handle a handle to a daemon connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use (use NULL for all portals).
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogin(iSCSIDaemonHandle handle,
                         iSCSITargetRef target,
                         iSCSIPortalRef portal,
                         enum iSCSILoginStatusCode * statusCode)
{
    if(handle < 0 || !target || !statusCode)
        return EINVAL;

    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef portalData = NULL;

    iSCSIDCmdLogin cmd = iSCSIDCmdLoginInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = 0;

    if(portal) {
        portalData = iSCSIPortalCreateData(portal);
        cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    }

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               targetData,portalData,NULL);

    if(portal)
        CFRelease(portalData);
    CFRelease(targetData);

    if(error)
        return error;

    iSCSIDRspLogin rsp;

    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return EIO;

    if(rsp.funcCode != kiSCSIDLogin)
        return EIO;

    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;

    return rsp.errorCode;
}


/*! Logs out of the target or a specific portal, if specified.
 *  @param handle a handle to a daemon connection.
 *  @param target the target to logout.
 *  @param portal the portal to logout.  If one is not specified,
 *  @param statusCode iSCSI response code indicating operation status. */
errno_t iSCSIDaemonLogout(iSCSIDaemonHandle handle,
                          iSCSITargetRef target,
                          iSCSIPortalRef portal,
                          enum iSCSILogoutStatusCode * statusCode)
{
    if(handle < 0 || !target || !statusCode)
        return EINVAL;

    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef portalData = NULL;

    iSCSIDCmdLogout cmd = iSCSIDCmdLogoutInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = 0;

    if(portal) {
        portalData = iSCSIPortalCreateData(portal);
        cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    }

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               targetData,portalData,NULL);

    if(portal)
        CFRelease(portalData);
    CFRelease(targetData);

    if(error)
        return error;

    iSCSIDRspLogout rsp;

    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return EIO;

    if(rsp.funcCode != kiSCSIDLogout)
        return EIO;

    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    
    return rsp.errorCode;
}


/*! Gets whether a target has an active session.
 *  @param target the target to test for an active session.
 *  @return true if the is an active session for the target; false otherwise. */
Boolean iSCSIDaemonIsTargetActive(iSCSIDaemonHandle handle,
                                  iSCSITargetRef target)
{
    if(handle < 0 || !target)
        return false;

    CFDataRef targetData = iSCSITargetCreateData(target);

    iSCSIDCmdIsTargetActive cmd = iSCSIDCmdIsTargetActiveInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               targetData,NULL);
    CFRelease(targetData);

    if(error)
        return false;

    iSCSIDRspIsTargetActive rsp;

    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return false;

    if(rsp.funcCode != kiSCSIDIsTargetActive)
        return false;

    return rsp.active;
}


/*! Gets whether a portal has an active session.
 *  @param target the target to test for an active session.
 *  @param portal the portal to test for an active connection.
 *  @return true if the is an active connection for the portal; false otherwise. */
Boolean iSCSIDaemonIsPortalActive(iSCSIDaemonHandle handle,
                                  iSCSITargetRef target,
                                  iSCSIPortalRef portal)
{
    if(handle < 0 || !target || !portal)
        return false;

    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef portalData = iSCSIPortalCreateData(portal);

    iSCSIDCmdIsPortalActive cmd = iSCSIDCmdIsPortalActiveInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               targetData,portalData,NULL);
    CFRelease(targetData);
    CFRelease(portalData);

    if(error)
        return false;

    iSCSIDRspIsPortalActive rsp;

    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return false;

    if(rsp.funcCode != kiSCSIDIsPortalActive)
        return false;
    
    return rsp.active;
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
    *discoveryRec = iSCSIDiscoveryRecCreateMutableWithData(discData);
    CFRelease(discData);
    if(!discoveryRec)
        return EIO;
    
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
    iSCSITargetSetIQN(target,targetIQN);
    
    // Generate data to transmit (no longer need target object after this)
    CFDataRef targetData = iSCSITargetCreateData(target);
    iSCSITargetRelease(target);
    
    CFDataRef portalData = iSCSIPortalCreateData(portal);
    
    // Create command header to transmit
    iSCSIDCmdQueryTargetForAuthMethod cmd = iSCSIDCmdQueryTargetForAuthMethodInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    
    if(iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,targetData,portalData,NULL))
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


/*! Creates an array of active target objects.
 *  @param handle a handle to a daemon connection.
 *  @return an array of active target objects or NULL if no targets are active. */
CFArrayRef iSCSIDaemonCreateArrayOfActiveTargets(iSCSIDaemonHandle handle)
{
    // Validate inputs
    if(handle < 0)
        return NULL;

    // Send command to daemon
    iSCSIDCmdCreateArrayOfActiveTargets cmd = iSCSIDCmdCreateArrayOfActiveTargetsInit;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;

    iSCSIDRspCreateArrayOfActiveTargets rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return NULL;

    if(rsp.dataLength == 0)
        return NULL;

    CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault,rsp.dataLength);
    CFDataSetLength(data,rsp.dataLength);
    UInt8 * bytes = CFDataGetMutableBytePtr(data);

    if(recv(handle,bytes,rsp.dataLength,0) != rsp.dataLength)
    {
        CFRelease(data);
        return NULL;
    }

    CFPropertyListFormat format;
    CFArrayRef activeTargets = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
    CFRelease(data);

    if(format == kCFPropertyListBinaryFormat_v1_0)
        return activeTargets;
    else {
        CFRelease(activeTargets);
        return NULL;
    }
}


/*! Creates an array of active portal objects.
 *  @param target the target to retrieve active portals.
 *  @param handle a handle to a daemon connection.
 *  @return an array of active target objects or NULL if no targets are active. */
CFArrayRef iSCSIDaemonCreateArrayOfActivePortalsForTarget(iSCSIDaemonHandle handle,
                                                          iSCSITargetRef target)
{
    // Validate inputs
    if(handle < 0)
        return NULL;

    // Send command to daemon
    iSCSIDCmdCreateArrayOfActivePortalsForTarget cmd = iSCSIDCmdCreateArrayOfActivePortalsForTargetInit;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return NULL;

    iSCSIDRspCreateArrayOfActivePortalsForTarget rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return NULL;

    if(rsp.dataLength == 0)
        return NULL;

    CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault,rsp.dataLength);
    CFDataSetLength(data,rsp.dataLength);
    UInt8 * bytes = CFDataGetMutableBytePtr(data);

    if(recv(handle,bytes,rsp.dataLength,0) != rsp.dataLength)
    {
        CFRelease(data);
        return NULL;
    }

    CFPropertyListFormat format;
    CFArrayRef activePortals = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
    CFRelease(data);

    if(format == kCFPropertyListBinaryFormat_v1_0)
        return activePortals;
    else {
        CFRelease(activePortals);
        return NULL;
    }
}


/*! Creates a dictionary of session parameters for the session associated with
 *  the specified target, if one exists.
 *  @param handle a handle to a daemon connection.
 *  @param target the target to check for associated sessions to generate
 *  a dictionary of session parameters.
 *  @return a dictionary of session properties. */
CFDictionaryRef iSCSIDaemonCreateCFPropertiesForSession(iSCSIDaemonHandle handle,
                                                        iSCSITargetRef target)
{
    // Validate inputs
    if(handle < 0 || !target)
        return NULL;

    CFDictionaryRef properties = NULL;
    CFDataRef targetData = iSCSITargetCreateData(target);

    // Send command to daemon
    iSCSIDCmdCreateCFPropertiesForSession cmd = iSCSIDCmdCreateCFPropertiesForSessionInit;

    cmd.targetLength = (UInt32)CFDataGetLength(targetData);

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               targetData,NULL);
    CFRelease(targetData);

    if(!error) {

        iSCSIDRspCreateCFPropertiesForSession rsp;

        if(recv(handle,&rsp,sizeof(rsp),0) == sizeof(rsp) && rsp.dataLength != 0)
        {
            CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault,rsp.dataLength);
            CFDataSetLength(data,rsp.dataLength);
            UInt8 * bytes = CFDataGetMutableBytePtr(data);

            if(recv(handle,bytes,rsp.dataLength,0) == rsp.dataLength)
            {
                CFPropertyListFormat format;
                properties = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
            }
            CFRelease(data);
        }
    }
    return properties;
}


/*! Creates a dictionary of connection parameters for the connection associated
 *  with the specified target and portal, if one exists.
 *  @param handle a handle to a daemon connection.
 *  @param target the target associated with the the specified portal.
 *  @param portal the portal to check for active connections to generate
 *  a dictionary of connection parameters.
 *  @return a dictionary of connection properties. */
CFDictionaryRef iSCSIDaemonCreateCFPropertiesForConnection(iSCSIDaemonHandle handle,
                                                           iSCSITargetRef target,
                                                           iSCSIPortalRef portal)
{
    // Validate inputs
    if(handle < 0 || !target || !portal)
        return NULL;

    CFDictionaryRef properties = NULL;
    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef portalData = iSCSIPortalCreateData(portal);

    // Send command to daemon
    iSCSIDCmdCreateCFPropertiesForConnection cmd = iSCSIDCmdCreateCFPropertiesForConnectionInit;

    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);

    errno_t error = iSCSIDaemonSendCmdWithData(handle,(iSCSIDCmd *)&cmd,
                                               targetData,portalData,NULL);
    CFRelease(targetData);
    CFRelease(portalData);

    if(!error) {

        iSCSIDRspCreateCFPropertiesForConnection rsp;

        if(recv(handle,&rsp,sizeof(rsp),0) == sizeof(rsp) && rsp.dataLength != 0)
        {
            CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault,rsp.dataLength);
            CFDataSetLength(data,rsp.dataLength);
            UInt8 * bytes = CFDataGetMutableBytePtr(data);

            if(recv(handle,bytes,rsp.dataLength,0) == rsp.dataLength)
            {
                CFPropertyListFormat format;
                properties = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
            }
            CFRelease(data);
        }
    }
    return properties;
}


/*! Enables or disables SendTargets discovery.
 *  @param handle a handle to a daemon connection.
 *  @param enable true to enable SendTargets discovery, false to disable.
 *  @return an error code indicating whether the operationg was successful. */
errno_t iSCSIDaemonToggleSendTargetsDiscovery(iSCSIDaemonHandle handle,
                                              Boolean enable)
{
    // Validate inputs
    if(handle < 0)
        return EINVAL;

    // Send command to daemon
    iSCSIDCmdToggleSendTargetsDiscovery cmd = iSCSIDCmdToggleSendTargetsDiscoveryInit;
    cmd.enable = enable;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EIO;

    iSCSIDRspToggleSendTargetsDiscovery rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;

    return 0;
}

