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

const struct iSCSIDRspGetConnectionIdForAddress iSCSIDRspGetConnectionIdForAddressInit = {
    .funcCode = kiSCSIDGetConnectionIdForAddress,
    .errorCode = 0,
    .connectionId = kiSCSIInvalidConnectionId
};

const struct iSCSIDRspGetSessionIds iSCSIDRspGetSessionIdsInit = {
    .funcCode = kiSCSIDGetSessionIds,
    .errorCode = 0,
    .sessionCount = 0
};

const struct iSCSIDRspGetConnectionIds iSCSIDRspGetConnectionIdsInit = {
    .funcCode = kiSCSIDGetConnectionIds,
    .errorCode = 0,
    .connectionCount = 0
};

const struct iSCSIDRspGetSessionInfo iSCSIDRspGetSessionInfoInit = {
    .funcCode = kiSCSIDGetSessionInfo,
    .errorCode = 0,
    .dataLength = 0
};


const struct iSCSIDRspGetConnectionInfo iSCSIDRspGetConnectionInfoInit = {
    .funcCode = kiSCSIDGetConnectionInfo,
    .errorCode = 0,
    .dataLength = 0
};

/*! Helper function. Reads data from a socket of the specified length and 
 *  calls a constructor function on the data to return an object of the
 *  appropriate type. */
void * iSCSIDCreateObjectFromSocket(int fd,UInt32 length,void *(* objectCreator)(CFDataRef))
{
                    fprintf(stderr,"Check 1.\n");
    // Receive iSCSI object data from stream socket
    UInt8 * bytes = (UInt8 *) malloc(length);
    if(!bytes || (recv(fd,bytes,length,0) != length))
    {
        free(bytes);
        return NULL;
    }
                    fprintf(stderr,"Check 2.\n");
    // Build a CFData wrapper around the data
    CFDataRef data = NULL;
    
    if(!(data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,bytes,length,kCFAllocatorMalloc)))
    {
        free(bytes);
        return NULL;
    }
                        fprintf(stderr,"Check 3.\n");
    // Create an iSCSI object from the data
    void * object = objectCreator(data);
    
    CFRelease(data);
    return object;
}

errno_t iSCSIDLoginSession(int fd,struct iSCSIDCmdLoginSession * cmd)
{
            fprintf(stderr,"Login session.\n");
    
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                            (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                            (void *(* )(CFDataRef))&iSCSITargetCreateWithData);
    iSCSIAuthRef auth = iSCSIDCreateObjectFromSocket(fd,cmd->authLength,
                            (void *(* )(CFDataRef))&iSCSIAuthCreateWithData);
    
    if(!portal || !target || !auth)
        return EAGAIN;
    
                fprintf(stderr,"Passed input check.\n");
    
    SID sessionId;
    CID connectionId;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
    
    // Login the session
    errno_t error = iSCSILoginSession(portal,target,auth,&sessionId,&connectionId,&statusCode);
    
    iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);
    iSCSIAuthRelease(auth);
    
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
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                            (void *(* )(CFDataRef))&iSCSITargetCreateWithData);
    iSCSIAuthRef auth = iSCSIDCreateObjectFromSocket(fd,cmd->authLength,
                            (void *(* )(CFDataRef))&iSCSIAuthCreateWithData);
    
    if(!portal || !target || !auth)
        return EAGAIN;
    
    CID connectionId;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
    
    // Login the session
    errno_t error = iSCSILoginConnection(portal,target,auth,cmd->sessionId,&connectionId,&statusCode);
    
    iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);
    iSCSIAuthRelease(auth);
    
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
    
    SID sessionId = kiSCSIInvalidSessionId;
    errno_t error = iSCSIGetSessionIdForTarget(iSCSITargetGetName(target),&sessionId);
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetSessionIdForTarget rsp = iSCSIDRspGetSessionIdForTargetInit;
    rsp.errorCode = error;
    rsp.sessionId = sessionId;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDGetConnectionIdForAddress(int fd,struct iSCSIDCmdGetConnectionIdForAddress * cmd)
{
    // Grab objects from stream
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

    CID connectionId;
    errno_t error = iSCSIGetConnectionIdFromAddress(cmd->sessionId,iSCSIPortalGetAddress(portal),&connectionId);

    // Compose a response to send back to the client
    struct iSCSIDRspGetConnectionIdForAddress rsp = iSCSIDRspGetConnectionIdForAddressInit;
    rsp.errorCode = error;
    rsp.connectionId = connectionId;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDGetSessionIds(int fd,struct iSCSIDCmdGetSessionIds * cmd)
{
    SID sessionIds[kiSCSIMaxSessions];
    UInt16 sessionCount;
    errno_t error = iSCSIGetSessionIds(sessionIds,&sessionCount);
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetSessionIds rsp = iSCSIDRspGetSessionIdsInit;
    rsp.errorCode = error;
    rsp.sessionCount = sessionCount;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(send(fd,sessionIds,sizeof(sessionIds),0) != sizeof(sessionIds))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDGetConnectionIds(int fd,struct iSCSIDCmdGetConnectionIds * cmd)
{
    CID connectionIds[kiSCSIMaxConnectionsPerSession];
    UInt32 connectionCount;
    errno_t error = iSCSIGetConnectionIds(cmd->sessionId,connectionIds,&connectionCount);

    // Compose a response to send back to the client
    struct iSCSIDRspGetConnectionIds rsp = iSCSIDRspGetConnectionIdsInit;
    rsp.errorCode = error;
    rsp.connectionCount = connectionCount;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(send(fd,connectionIds,sizeof(connectionIds),0) != sizeof(connectionIds))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDGetSessionInfo(int fd,struct iSCSIDCmdGetSessionInfo * cmd)
{
    iSCSISessionOptions sessionOptions;
    errno_t error = iSCSIGetSessionInfo(cmd->sessionId,&sessionOptions);
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetSessionInfo rsp = iSCSIDRspGetSessionInfoInit;
    rsp.errorCode = error;
    rsp.dataLength = sizeof(sessionOptions);
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(send(fd,&sessionOptions,sizeof(sessionOptions),0) != sizeof(sessionOptions))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDGetConnectionInfo(int fd,struct iSCSIDCmdGetConnectionInfo * cmd)
{
    iSCSIConnectionOptions connectionOptions;
    errno_t error = iSCSIGetConnectionInfo(cmd->sessionId,cmd->connectionId,&connectionOptions);
    
    // Compose a response to send back to the client
    struct iSCSIDRspGetConnectionInfo rsp = iSCSIDRspGetConnectionInfoInit;
    rsp.errorCode = error;
    rsp.dataLength = sizeof(connectionOptions);
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(send(fd,&connectionOptions,sizeof(connectionOptions),0) != sizeof(connectionOptions))
        return EAGAIN;

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
    
    // Associate a new kernel event with the socket
    struct kevent kernelEvent;
    EV_SET(&kernelEvent,launch_data_get_fd(listen_socket),EVFILT_READ,EV_ADD,0,0,NULL);
    if((kevent(kernelQueue,&kernelEvent,1,NULL,0,NULL) == -1))
        goto ERROR_KEVENT_REG_FAIL;
    
    launch_data_free(reg_response);
    
    while(true)
    {
        int numEvents = 0;

        // Wait for the kernel to tell that data has become available on the
        // socket (if no events or if there's an error we quit)
        struct kevent eventList;
        if((numEvents = kevent(kernelQueue,NULL,0,&eventList,1,NULL)) == -1)
            continue;
        else if (numEvents == 0)
            continue;

        // Accept an incoming connection
        struct sockaddr_storage peerAddress;
        socklen_t sizeAddress = sizeof(peerAddress);
        int fd = accept((int)eventList.ident,(struct sockaddr *)&peerAddress,&sizeAddress);
        
        // Receive data
        struct iSCSIDCmd cmd;
        if(recv(fd,&cmd,sizeof(cmd),0) != sizeof(cmd))
            goto ERROR_COMM_FAIL;
        
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
            case kiSCSIDGetConnectionIdForAddress:
                error = iSCSIDGetConnectionIdForAddress(fd,(iSCSIDCmdGetConnectionIdForAddress*)&cmd); break;
            case kiSCSIDGetSessionIds:
                error = iSCSIDGetSessionIds(fd,(iSCSIDCmdGetSessionIds*)&cmd); break;
            case kiSCSIDGetConnectionIds:
                error = iSCSIDGetConnectionIds(fd,(iSCSIDCmdGetConnectionIds*)&cmd); break;
            case kiSCSIDGetSessionInfo:
                error = iSCSIDGetSessionInfo(fd,(iSCSIDCmdGetSessionInfo*)&cmd); break;
            case kiSCSIDGetConnectionInfo:
                error = iSCSIDGetConnectionInfo(fd,(iSCSIDCmdGetConnectionInfo*)&cmd); break;
        };
        
        if(error)
        {
            // Terminate connection to client (ill-behaved client)
            close(fd);
        }
    }

    // Close our connection to the iSCSI kernel extension
    iSCSIKernelCleanUp();
    
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
    
    
    return ENOTSUP;
}

