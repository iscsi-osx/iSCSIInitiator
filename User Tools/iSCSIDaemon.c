/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemon.c
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI user-space daemon
 */


// BSD includes
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Foundation includes
#include <launch.h>
#include <CoreFoundation/CFPreferences.h>

// Mach kernel includes
#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/mach_interface.h>

// I/O Kit includes
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

// iSCSI includes
#include "iSCSISession.h"
#include "iSCSIDaemonInterfaceShared.h"
#include "iSCSIPropertyList.h"

// Used to notify daemon of power state changes
io_connect_t powerPlaneRoot;
io_object_t powerNotifier;
IONotificationPortRef powerNotifyPortRef;


const struct iSCSIDRspLoginSession iSCSIDRspLoginSessionInit  = {
    .funcCode = kiSCSIDLoginSession,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILoginInvalidStatusCode,
    .sessionId = kiSCSIInvalidSessionId,
    .connectionId   = kiSCSIInvalidConnectionId
};

const struct iSCSIDRspLogoutSession iSCSIDRspLogoutSessionInit = {
    .funcCode = kiSCSIDLogoutSession,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILoginInvalidStatusCode,
};

const struct iSCSIDRspLoginConnection iSCSIDRspLoginConnectionInit = {
    .funcCode = kiSCSIDLoginConnection,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILoginInvalidStatusCode,
    .connectionId = kiSCSIInvalidConnectionId
};

const struct iSCSIDRspLogoutConnection iSCSIDRspLogoutConnectionInit = {
    .funcCode = kiSCSIDLogoutConnection,
    .errorCode = 0,
    .statusCode = (UInt8) kiSCSILogoutInvalidStatusCode
};

const struct iSCSIDRspQueryPortalForTargets iSCSIDRspQueryPortalForTargetsInit = {
    .funcCode = kiSCSIDQueryPortalForTargets,
    .errorCode = 0,
    .statusCode = (UInt8) kiSCSILogoutInvalidStatusCode,
    .discoveryLength = 0
};

const struct iSCSIDRspQueryTargetForAuthMethod iSCSIDRspQueryTargetForAuthMethodInit = {
    .funcCode = kiSCSIDQueryTargetForAuthMethod,
    .errorCode = 0,
    .statusCode = 0,
    .authMethod = 0
};

const struct iSCSIDRspGetSessionIdForTarget iSCSIDRspGetSessionIdForTargetInit = {
    .funcCode = kiSCSIDGetSessionIdForTarget,
    .errorCode = 0,
    .sessionId = kiSCSIInvalidSessionId
};

const struct iSCSIDRspGetConnectionIdForPortal iSCSIDRspGetConnectionIdForPortalInit = {
    .funcCode = kiSCSIDGetConnectionIdForPortal,
    .errorCode = 0,
    .connectionId = kiSCSIInvalidConnectionId
};

const struct iSCSIDRspGetSessionIds iSCSIDRspGetSessionIdsInit = {
    .funcCode = kiSCSIDGetSessionIds,
    .errorCode = 0,
    .dataLength = 0
};

const struct iSCSIDRspGetConnectionIds iSCSIDRspGetConnectionIdsInit = {
    .funcCode = kiSCSIDGetConnectionIds,
    .errorCode = 0,
    .dataLength = 0
};

const struct iSCSIDRspCreateTargetForSessionId iSCSIDRspCreateTargetForSessionIdInit = {
    .funcCode = kiSCSIDCreateTargetForSessionId,
    .targetLength = 0
};

const struct iSCSIDRspCreatePortalForConnectionId iSCSIDRspCreatePortalForConnectionIdInit = {
    .funcCode = kiSCSIDCreatePortalForConnectionId,
    .portalLength = 0
};

const struct iSCSIDRspCopySessionConfig iSCSIDRspCopySessionConfigInit = {
    .funcCode = kiSCSIDCopySessionConfig,
    .errorCode = 0,
    .dataLength = 0
};


const struct iSCSIDRspGetConnectionConfig iSCSIDRspGetConnectionConfigInit = {
    .funcCode = kiSCSIDCopyConnectionConfig,
    .errorCode = 0,
    .dataLength = 0
};

errno_t iSCSIDLoginSession(int fd,struct iSCSIDCmdLoginSession * cmd)
{
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                            (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                            (void *(* )(CFDataRef))&iSCSITargetCreateWithData);
    iSCSIAuthRef auth = iSCSIDCreateObjectFromSocket(fd,cmd->authLength,
                            (void *(* )(CFDataRef))&iSCSIAuthCreateWithData);
    
    iSCSISessionConfigRef sessCfg = iSCSIDCreateObjectFromSocket(
        fd,cmd->sessCfgLength,(void *(* )(CFDataRef))&iSCSISessionConfigCreateWithData);
    
    iSCSIConnectionConfigRef connCfg = iSCSIDCreateObjectFromSocket(
        fd,cmd->connCfgLength,(void *(* )(CFDataRef))&iSCSIConnectionConfigCreateWithData);
    
    SID sessionId;
    CID connectionId;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
    
    // Login the session
    errno_t error = iSCSILoginSession(target,portal,auth,sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
    
    iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);
    iSCSIAuthRelease(auth);
    iSCSISessionConfigRelease(sessCfg);
    iSCSIConnectionConfigRelease(connCfg);
    
    // Compose a response to send back to the client
    struct iSCSIDRspLoginSession rsp = iSCSIDRspLoginSessionInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    rsp.sessionId = sessionId;
    rsp.connectionId = connectionId;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDLogoutSession(int fd,struct iSCSIDCmdLogoutSession * cmd)
{
    enum iSCSILogoutStatusCode statusCode = kiSCSILogoutInvalidStatusCode;
    
    errno_t error = iSCSILogoutSession(cmd->sessionId,&statusCode);
    
    // Compose a response to send back to the client
    struct iSCSIDRspLogoutSession rsp = iSCSIDRspLogoutSessionInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDLoginConnection(int fd,struct iSCSIDCmdLoginConnection * cmd)
{
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                            (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);
    iSCSIAuthRef auth = iSCSIDCreateObjectFromSocket(fd,cmd->authLength,
                            (void *(* )(CFDataRef))&iSCSIAuthCreateWithData);
    
    iSCSIConnectionConfigRef connCfg = iSCSIDCreateObjectFromSocket(
        fd,cmd->connCfgLength,(void *(* )(CFDataRef))&iSCSIConnectionConfigCreateWithData);

    
    CID connectionId;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
    
    // Login the session
    errno_t error = iSCSILoginConnection(cmd->sessionId,portal,auth,connCfg,&connectionId,&statusCode);
    
    iSCSIPortalRelease(portal);
    iSCSIAuthRelease(auth);
    iSCSIConnectionConfigRelease(connCfg);
    
    // Compose a response to send back to the client
    struct iSCSIDRspLoginSession rsp = iSCSIDRspLoginSessionInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    rsp.connectionId = connectionId;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDLogoutConnection(int fd,struct iSCSIDCmdLogoutConnection * cmd)
{
    enum iSCSILogoutStatusCode statusCode = kiSCSILogoutInvalidStatusCode;
    
    errno_t error = iSCSILogoutConnection(cmd->sessionId,cmd->connectionId,&statusCode);
    
    // Compose a response to send back to the client
    struct iSCSIDRspLogoutConnection rsp = iSCSIDRspLogoutConnectionInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDQueryPortalForTargets(int fd,struct iSCSIDCmdQueryPortalForTargets * cmd)
{
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                            (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);
    
    // Grab objects from stream
    iSCSIAuthRef auth = iSCSIDCreateObjectFromSocket(fd,cmd->authLength,
                            (void *(* )(CFDataRef))&iSCSIAuthCreateWithData);

    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;

    iSCSIMutableDiscoveryRecRef discoveryRec;
    errno_t error = iSCSIQueryPortalForTargets(portal,auth,&discoveryRec,&statusCode);
    
    iSCSIPortalRelease(portal);
    iSCSIAuthRelease(auth);
    
    if(error)
        return error;
    
    // Compose a response to send back to the client
    struct iSCSIDRspQueryPortalForTargets rsp = iSCSIDRspQueryPortalForTargetsInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    
    CFDataRef data = iSCSIDiscoveryRecCreateData(discoveryRec);
    iSCSIDiscoveryRecRelease(discoveryRec);
    rsp.discoveryLength = (UInt32)CFDataGetLength(data);
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        CFRelease(data);
        return EAGAIN;
    }
    
    if(send(fd,CFDataGetBytePtr(data),CFDataGetLength(data),0) != CFDataGetLength(data))
    {
        CFRelease(data);
        return EAGAIN;
    }
    
    CFRelease(data);
    return 0;
}

errno_t iSCSIDQueryTargetForAuthMethod(int fd,struct iSCSIDCmdQueryTargetForAuthMethod * cmd)
{
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                            (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                            (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
    
    errno_t error = iSCSIQueryTargetForAuthMethod(portal,iSCSITargetGetIQN(target),&authMethod,&statusCode);
    
    // Compose a response to send back to the client
    struct iSCSIDRspQueryTargetForAuthMethod rsp = iSCSIDRspQueryTargetForAuthMethodInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    rsp.authMethod = authMethod;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDGetSessionIdForTarget(int fd,struct iSCSIDCmdGetSessionIdForTarget * cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                            (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    SID sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target));
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetSessionIdForTarget rsp = iSCSIDRspGetSessionIdForTargetInit;
    rsp.errorCode = 0;
    rsp.sessionId = sessionId;

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDGetConnectionIdForPortal(int fd,struct iSCSIDCmdGetConnectionIdForPortal * cmd)
{
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

    CID connectionId = iSCSIGetConnectionIdForPortal(cmd->sessionId,portal);


    // Compose a response to send back to the client
    struct iSCSIDRspGetConnectionIdForPortal rsp = iSCSIDRspGetConnectionIdForPortalInit;
    rsp.errorCode = 0;
    rsp.connectionId = connectionId;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDGetSessionIds(int fd,struct iSCSIDCmdGetSessionIds * cmd)
{
    CFArrayRef sessionIds = iSCSICreateArrayOfSessionIds();
    errno_t result = EAGAIN;
    // Compose a response to send back to the client
    struct iSCSIDRspGetSessionIds rsp = iSCSIDRspGetSessionIdsInit;

    if(sessionIds) {
        const void ** sessionIdValues = malloc(CFArrayGetCount(sessionIds)*sizeof(void*));
        if (sessionIdValues) {
            CFArrayGetValues(sessionIds,CFRangeMake(0,CFArrayGetCount(sessionIds)),sessionIdValues);
            
            rsp.errorCode = 0;
            rsp.dataLength = (UInt32)CFArrayGetCount(sessionIds)*sizeof(void*);
            if (send(fd, &rsp, sizeof(rsp), 0) == sizeof(rsp)) {
                if (send(fd,sessionIdValues,rsp.dataLength,0) == rsp.dataLength) {
                    result = 0;
                }
            }
            free(sessionIdValues);
        }
        CFRelease(sessionIds);
    } else {
        rsp.errorCode = EAGAIN;
        rsp.dataLength = 0;
        if (send(fd, &rsp, sizeof(rsp), 0) == sizeof(rsp))
            result = 0;
    }
    return result;
}

errno_t iSCSIDGetConnectionIds(int fd,struct iSCSIDCmdGetConnectionIds * cmd)
{
    CFArrayRef connectionIds = iSCSICreateArrayOfConnectionsIds(cmd->sessionId);
    errno_t result = EAGAIN;
    // Compose a response to send back to the client
    struct iSCSIDRspGetConnectionIds rsp = iSCSIDRspGetConnectionIdsInit;
    const void ** connIdValues = malloc(CFArrayGetCount(connectionIds)*sizeof(void*));

    if(connectionIds)
    {
        CFArrayGetValues(connectionIds,CFRangeMake(0,CFArrayGetCount(connectionIds)),connIdValues);

        rsp.errorCode = 0;
        rsp.dataLength = (UInt32)CFArrayGetCount(connectionIds)*sizeof(void*);
    }
    else {
        rsp.errorCode = EAGAIN;
        rsp.dataLength = 0;
    }
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        if(connectionIds)
            CFRelease(connectionIds);
        if (connIdValues) {
            free(connIdValues);
            connIdValues = NULL;
        }
        return result;
    }
    
    if(connIdValues)
    {
        if(send(fd,connIdValues,rsp.dataLength,0) != rsp.dataLength)
        {
            result = EAGAIN;
        } else {
            result = 0;
        }
        if (connIdValues) {
            free(connIdValues);
            connIdValues = NULL;
        }
    }
    
    if (connectionIds)
        CFRelease(connectionIds);
    
    return result;
}

errno_t iSCSIDCreateTargetForSessionId(int fd,struct iSCSIDCmdCreateTargetForSessionId * cmd)
{
    iSCSITargetRef target = iSCSICreateTargetForSessionId(cmd->sessionId);
    
    // Compose a response to send back to the client
    struct iSCSIDRspCreateTargetForSessionId rsp = iSCSIDRspCreateTargetForSessionIdInit;
    CFDataRef data = NULL;

    if(target)
    {
        data = iSCSITargetCreateData(target);
        CFRelease(target);
        rsp.targetLength = (UInt32)CFDataGetLength(data);
    }
    else
    {
        rsp.targetLength = 0;
    }
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        if(data)
            CFRelease(data);
        return EAGAIN;
    }

    if(data)
    {
        if(send(fd,CFDataGetBytePtr(data),rsp.targetLength,0) != rsp.targetLength)
        {
            CFRelease(data);
            return EAGAIN;
        }
        
        CFRelease(data);
    }
    
    return 0;
}

errno_t iSCSIDCreatePortalForConnectionId(int fd,struct iSCSIDCmdCreatePortalForConnectionId * cmd)
{
    iSCSIPortalRef portal = iSCSICreatePortalForConnectionId(cmd->sessionId,cmd->connectionId);
    
    // Compose a response to send back to the client
    struct iSCSIDRspCreatePortalForConnectionId rsp = iSCSIDRspCreatePortalForConnectionIdInit;
    CFDataRef data = NULL;
    
    if(portal)
    {
        data = iSCSIPortalCreateData(portal);
        CFRelease(portal);
        rsp.portalLength = (UInt32)CFDataGetLength(data);
    }
    else
    {
        rsp.portalLength = 0;
    }
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        if(data)
            CFRelease(data);
        return EAGAIN;
    }
    
    if(data)
    {
        if(send(fd,CFDataGetBytePtr(data),rsp.portalLength,0) != rsp.portalLength)
        {
            CFRelease(data);
            return EAGAIN;
        }
        CFRelease(data);
    }
    
    return 0;
}

errno_t iSCSIDCopySessionConfig(int fd,struct iSCSIDCmdCopySessionConfig * cmd)
{
    iSCSISessionConfigRef sessCfg = iSCSICopySessionConfig(cmd->sessionId);
    
    // Convert session config object to data and free it
    CFDataRef data = iSCSISessionConfigCreateData(sessCfg);
    iSCSISessionConfigRelease(sessCfg);
    
    // Compose a response to send back to the client
    struct iSCSIDRspCopySessionConfig rsp = iSCSIDRspCopySessionConfigInit;
    
    rsp.errorCode = 0;
    rsp.dataLength = (UInt32)CFDataGetLength(data);
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        CFRelease(data);
        return EAGAIN;
    }
    
    if(send(fd,CFDataGetBytePtr(data),CFDataGetLength(data),0) != CFDataGetLength(data))
    {
        CFRelease(data);
        return EAGAIN;
    }

    CFRelease(data);
    return 0;
}

errno_t iSCSIDCopyConnectionConfig(int fd,struct iSCSIDCmdCopyConnectionConfig * cmd)
{
    iSCSIConnectionConfigRef connCfg = iSCSICopyConnectionConfig(cmd->sessionId,cmd->connectionId);
    
    // Convert connection config object to data and free it
    CFDataRef data = iSCSIConnectionConfigCreateData(connCfg);
    iSCSIConnectionConfigRelease(connCfg);
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetConnectionConfig rsp = iSCSIDRspGetConnectionConfigInit;
    rsp.errorCode = 0;
    rsp.dataLength = (UInt32)CFDataGetLength(data);
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        CFRelease(data);
        return EAGAIN;
    }
    
    if(send(fd,CFDataGetBytePtr(data),CFDataGetLength(data),0) != CFDataGetLength(data))
    {
        CFRelease(data);
        return EAGAIN;
    }
    
    CFRelease(data);
    return 0;
}

/*! Handles power event messages received from the kernel.  This callback
 *  is only active when iSCSIDRegisterForPowerEvents() has been called.
 *  @param refCon always NULL (not used).
 *  @param service the service associated with the notification port.
 *  @param messageType the type of notification message.
 *  @param messageArgument argument associated with the notification message. */
void iSCSIDHandlePowerEvent(void * refCon,
                            io_service_t service,
                            natural_t messageType,
                            void * messageArgument)
{
    switch(messageType)
    {
// TODO: handle sleep
        break;
    };

}

/*! Registers the daemon with the kernel to receive power events
 *  (e.g., sleep/wake notifications).
 *  @return true if the daemon was successfully registered. */
bool iSCSIDRegisterForPowerEvents()
{
    powerPlaneRoot = IORegisterForSystemPower(NULL,
                                              &powerNotifyPortRef,
                                              iSCSIDHandlePowerEvent,
                                              &powerNotifier);
   
    if(powerPlaneRoot == 0)
        return false;
    
    CFRunLoopAddSource(CFRunLoopGetMain(),
                       IONotificationPortGetRunLoopSource(powerNotifyPortRef),
                       kCFRunLoopDefaultMode);
    return true;
}

/*! Deregisters the daemon with the kernel to no longer receive power events. */
void iSCSIDDeregisterForPowerEvents()
{
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(),
                          IONotificationPortGetRunLoopSource(powerNotifyPortRef),
                          kCFRunLoopDefaultMode);
    
    IODeregisterForSystemPower(&powerNotifier);
    IOServiceClose(powerPlaneRoot);
    IONotificationPortDestroy(powerNotifyPortRef);
}

void iSCSIDProcessIncomingRequest(CFSocketRef socket,
                                  CFSocketCallBackType callbackType,
                                  CFDataRef address,
                                  const void * data,
                                  void * info)
{
    // File descriptor associated with the socket we're using
    static int fd = 0;
    
    // If this is the first connection, initialize the user client for the
    // iSCSI initiator kernel extension
    if(fd == 0) {
        iSCSIInitialize(CFRunLoopGetCurrent());

        // Wait for an incoming connection; upon timeout quit
        struct sockaddr_storage peerAddress;
        socklen_t length = sizeof(peerAddress);
        
        // Get file descriptor associated with the connection
        fd = accept(CFSocketGetNative(socket),(struct sockaddr *)&peerAddress,&length);
    }

    struct iSCSIDCmd cmd;

    while(recv(fd,&cmd,sizeof(cmd),MSG_PEEK) == sizeof(cmd)) {
        
        recv(fd,&cmd,sizeof(cmd),MSG_WAITALL);

    errno_t error = 0;
    switch(cmd.funcCode)
    {
        
        case kiSCSIDLoginSession:
            error = iSCSIDLoginSession(fd,(iSCSIDCmdLoginSession*)&cmd); break;
        case kiSCSIDLogoutSession:
            error = iSCSIDLogoutSession(fd,(iSCSIDCmdLogoutSession*)&cmd); break;
        case kiSCSIDLoginConnection:
            error = iSCSIDLoginConnection(fd,(iSCSIDCmdLoginConnection*)&cmd); break;
        case kiSCSIDLogoutConnection:
            error = iSCSIDLogoutConnection(fd,(iSCSIDCmdLogoutConnection*)&cmd); break;
        case kiSCSIDQueryPortalForTargets:
            error = iSCSIDQueryPortalForTargets(fd,(iSCSIDCmdQueryPortalForTargets*)&cmd); break;
        case kiSCSIDQueryTargetForAuthMethod:
            error = iSCSIDQueryTargetForAuthMethod(fd,(iSCSIDCmdQueryTargetForAuthMethod*)&cmd); break;
        case kiSCSIDGetSessionIdForTarget:
            error = iSCSIDGetSessionIdForTarget(fd,(iSCSIDCmdGetSessionIdForTarget*)&cmd); break;
        case kiSCSIDGetConnectionIdForPortal:
            error = iSCSIDGetConnectionIdForPortal(fd,(iSCSIDCmdGetConnectionIdForPortal*)&cmd); break;
        case kiSCSIDGetSessionIds:
            error = iSCSIDGetSessionIds(fd,(iSCSIDCmdGetSessionIds*)&cmd); break;
        case kiSCSIDGetConnectionIds:
            error = iSCSIDGetConnectionIds(fd,(iSCSIDCmdGetConnectionIds*)&cmd); break;
        case kiSCSIDCreateTargetForSessionId:
            error = iSCSIDCreateTargetForSessionId(fd,(iSCSIDCmdCreateTargetForSessionId*)&cmd); break;
        case kiSCSIDCreatePortalForConnectionId:
            error = iSCSIDCreatePortalForConnectionId(fd,(iSCSIDCmdCreatePortalForConnectionId*)&cmd); break;
        case kiSCSIDCopySessionConfig:
            error = iSCSIDCopySessionConfig(fd,(iSCSIDCmdCopySessionConfig*)&cmd); break;
        case kiSCSIDCopyConnectionConfig:
            error = iSCSIDCopyConnectionConfig(fd,(iSCSIDCmdCopyConnectionConfig*)&cmd); break;
        default:
            // Close our connection to the iSCSI kernel extension
            iSCSICleanup();
            close(fd);
            fd = 0;
    };
    }
}

/*! iSCSI daemon entry point. */
int main(void)
{
    // Connect to the preferences .plist file associated with "iscsid" and
    // read configuration parameters for the initiator
    iSCSIPLSynchronize();

    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    if(initiatorIQN) {
        iSCSISetInitiatiorName(initiatorIQN);
        CFRelease(initiatorIQN);
    }
    
    CFStringRef initiatorAlias = iSCSIPLCopyInitiatorAlias();
    
    if(initiatorAlias) {
        iSCSISetInitiatiorName(initiatorAlias);
        CFRelease(initiatorAlias);
    }
 
    // Register with launchd so it can manage this daemon
    launch_data_t reg_request = launch_data_new_string(LAUNCH_KEY_CHECKIN);
    
    // Quit if we are unable to checkin...
    if(!reg_request) {
        fprintf(stderr,"Failed to checkin with launchd.\n");
        goto ERROR_LAUNCH_DATA;
    }
    
    launch_data_t reg_response = launch_msg(reg_request);
    
    // Ensure registration was successful
    if((launch_data_get_type(reg_response) == LAUNCH_DATA_ERRNO)) {
        fprintf(stderr,"Failed to checkin with launchd.\n");
        goto ERROR_LAUNCH_DATA;
    }
    
    // Grab label and socket dictionary from daemon's property list
    launch_data_t label = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_LABEL);
    launch_data_t sockets = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_SOCKETS);
    
    if(!label || !sockets) {
        fprintf(stderr,"Could not find socket ");
        goto ERROR_NO_SOCKETS;
    }
    
    launch_data_t listen_socket_array = launch_data_dict_lookup(sockets,"iscsid");
    
    if(!listen_socket_array || launch_data_array_get_count(listen_socket_array) == 0)
        goto ERROR_NO_SOCKETS;
    
    // Grab handle to socket we want to listen on...
    launch_data_t listen_socket = launch_data_array_get_index(listen_socket_array,0);
    
    if(!iSCSIDRegisterForPowerEvents())
        goto ERROR_PWR_MGMT_FAIL;
    
    // Create a socket that will
    CFSocketRef socket = CFSocketCreateWithNative(kCFAllocatorDefault,
                                                  launch_data_get_fd(listen_socket),
                                                  kCFSocketReadCallBack,
                                                  iSCSIDProcessIncomingRequest,0);
  
    // Runloop sources associated with socket events of connected clients
    CFRunLoopSourceRef clientSockSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault,socket,0);
    CFRunLoopAddSource(CFRunLoopGetMain(),clientSockSource,kCFRunLoopDefaultMode);
    
    CFRunLoopRun();
    
    // Deregister for power
    iSCSIDDeregisterForPowerEvents();
    
    launch_data_free(reg_response);
    return 0;

// TODO: verify that launch data is freed under all possible execution paths
    
ERROR_PWR_MGMT_FAIL:
 

    
ERROR_NO_SOCKETS:
    
ERROR_LAUNCH_DATA:

    launch_data_free(reg_response);
    return ENOTSUP;
}

