/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemon.c
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI daemon
 */

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

#include <launch.h>

#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"
#include "iSCSIDaemonInterfaceShared.h"

#include <CoreFoundation/CFPreferences.h>

static const CFStringRef applicationId = CFSTR("com.NSinenian.iscsix");

/*! Read target entry from .plist and construct target and portal information. */
//errno_t BuildPortalFrom

/*! Helper function.  Configures the iSCSI initiator based on parameters
 *  that are read from the preferences .plist file. */
void ConfigureiSCSIFromPreferences()
{
    // Connect to the preferences .plist file associated with "iscsid" and
    // read configuration parameters for the initiator
/*    CFStringRef initiatorName =
        CFPreferencesCopyValue(kiSCSIPKInitiatorName,applicationId,
                               kCFPreferencesAnyUser,kCFPreferencesAnyHost);
    
    CFStringRef initiatorAlias =
        CFPreferencesCopyValue(kiSCSIPKInitiatorAlias,applicationId,
                               kCFPreferencesAnyUser,kCFPreferencesAnyHost);
    
    // If initiator name & alias are provided, set them...
    if(initiatorName)
        iSCSISetInitiatiorName(initiatorName);
    
    if(initiatorAlias)
        iSCSISetInitiatorAlias(initiatorAlias);*/
    
}

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

    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;

    iSCSIMutableDiscoveryRecRef discoveryRec;
    errno_t error = iSCSIQueryPortalForTargets(portal,&discoveryRec,&statusCode);
    
    // Compose a response to send back to the client
    struct iSCSIDRspQueryPortalForTargets rsp = iSCSIDRspQueryPortalForTargetsInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        iSCSIDiscoveryRecRelease(discoveryRec);
        return EAGAIN;
    }
    
    CFDataRef data = iSCSIDiscoveryRecCreateData(discoveryRec);
    
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
    
    errno_t error = iSCSIQueryTargetForAuthMethod(portal,iSCSITargetGetName(target),&authMethod,&statusCode);
    
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
    
    SID sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetName(target));
    
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
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetSessionIds rsp = iSCSIDRspGetSessionIdsInit;
    const void ** sessionIdValues = malloc(CFArrayGetCount(sessionIds)*sizeof(void*));

    if(sessionIds)
    {
        CFArrayGetValues(sessionIds,CFRangeMake(0,CFArrayGetCount(sessionIds)),sessionIdValues);
        
        rsp.errorCode = 0;
        rsp.dataLength = (UInt32)CFArrayGetCount(sessionIds)*sizeof(void*);
    }
    else {
        rsp.errorCode = EAGAIN;
        rsp.dataLength = 0;
    }

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        if(sessionIds)
            CFRelease(sessionIds);
        return EAGAIN;
    }

    if(sessionIds)
    {
        if(send(fd,sessionIdValues,rsp.dataLength,0) != rsp.dataLength)
        {
            CFRelease(sessionIds);
            return EAGAIN;
        }
        
        CFRelease(sessionIds);
    }
    
    return 0;
}

errno_t iSCSIDGetConnectionIds(int fd,struct iSCSIDCmdGetConnectionIds * cmd)
{
    CFArrayRef connectionIds = iSCSICreateArrayOfConnectionsIds(cmd->sessionId);
    
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
        return EAGAIN;
    }
    
    if(connIdValues)
    {
        if(send(fd,connIdValues,rsp.dataLength,0) != rsp.dataLength)
        {
            CFRelease(connectionIds);
            return EAGAIN;
        }
        
        CFRelease(connectionIds);
    }
    
    
    return 0;
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

/*! iSCSI daemon entry point. */
int main(void)
{
    // Verify that the iSCSI kernel extension is running
    if(iSCSIKernelInitialize() != kIOReturnSuccess)
        return ENOTSUP;

    // Configure the initiator by reading preferences file
    ConfigureiSCSIFromPreferences();
    
    // Register with launchd so it can manage this daemon
    launch_data_t reg_request = launch_data_new_string(LAUNCH_KEY_CHECKIN);
    
    // Quit if we are unable to checkin...
    if(!reg_request)
        return -1;
    
    launch_data_t reg_response = launch_msg(reg_request);
    
    // Ensure registration was successful
    if((launch_data_get_type(reg_response) == LAUNCH_DATA_ERRNO))
        goto ERROR_LAUNCH_DATA;
    
    // Grab label and socket dictionary from daemon's property list
    launch_data_t label = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_LABEL);
    launch_data_t sockets = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_SOCKETS);
    
    if(!label || !sockets)
        goto ERROR_NO_SOCKETS;
    
    launch_data_t listen_socket_array = launch_data_dict_lookup(sockets,"iscsid");
    
    if(!listen_socket_array || launch_data_array_get_count(listen_socket_array) == 0)
        goto ERROR_NO_SOCKETS;
    
    // Grab handle to socket we want to listen on...
    launch_data_t listen_socket = launch_data_array_get_index(listen_socket_array,0);
    
    // Create a new kernel event queues. The socket descriptors are registered
    // with the kernel, and the kernel notifies us (using events) when there
    // are incoming connections on a socket.
    int kernelQueue;
    if((kernelQueue = kqueue()) == -1)
        goto ERROR_KQUEUE_FAIL;
    
    // Wait for an incoming connection; upon timeout quit
    struct sockaddr_storage peerAddress;
    socklen_t length = sizeof(peerAddress);
    
    // Get file descriptor associated with the connection
    int fd = accept(launch_data_get_fd(listen_socket),(struct sockaddr *)&peerAddress,&length);
    
    // Associate a new kernel event with the socket
    struct kevent kernelEvent;
    EV_SET(&kernelEvent,fd,EVFILT_READ,EV_ADD,0,0,NULL);
    
    // Keep track of events as we get them and errors as we do processing
    errno_t error = 0;
    bool shutdownDaemon = false;

    do
    {
        // Receive data
        struct iSCSIDCmd cmd;
        if(recv(fd,&cmd,sizeof(cmd),0) != sizeof(cmd))
            goto ERROR_COMM_FAIL;

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
            case kiSCSIDShutdownDaemon:
                shutdownDaemon = true; break;
            default:
                error = EIO;
        };
        
    } while(!error && !shutdownDaemon);
    
    close(fd);
    
    // Close our connection to the iSCSI kernel extension
    iSCSIKernelCleanUp();
    
    launch_data_free(reg_response);
    
    return 0;
        
ERROR_KQUEUE_FAIL:
    fprintf(stderr,"Failed to create kernel event queue.\n");

ERROR_KEVENT_REG_FAIL:
    fprintf(stderr,"Failed to register socket with kernel.\n");
    
ERROR_NO_SOCKETS:
    fprintf(stderr,"Could not find socket ");

    launch_data_free(reg_response);
    
ERROR_LAUNCH_DATA:
    fprintf(stderr,"Failed to checkin with launchd.\n");
    
ERROR_COMM_FAIL:
    
    // Close our connection to the iSCSI kernel extension
    iSCSIKernelCleanUp();
    
    launch_data_free(reg_response);
    
    return ENOTSUP;
}

