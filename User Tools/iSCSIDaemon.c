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
#include <sys/un.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <asl.h>
#include <assert.h>
#include <pthread.h>

// Foundation includes
#include <launch.h>
#include <CoreFoundation/CoreFoundation.h>

// Mach kernel includes
#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/mach_interface.h>

// I/O Kit includes
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

// iSCSI includes
#include "iSCSISession.h"
#include "iSCSIDiscovery.h"
#include "iSCSIDaemonInterfaceShared.h"
#include "iSCSIPropertyList.h"
#include "iSCSIDA.h"


// Used to notify daemon of power state changes
io_connect_t powerPlaneRoot;
io_object_t powerNotifier;
IONotificationPortRef powerNotifyPortRef;

// Used to fire discovery timer at specified intervals
CFRunLoopTimerRef discoveryTimer = NULL;

// Mutex lock when discovery is running
pthread_mutex_t discoveryMutex = PTHREAD_MUTEX_INITIALIZER;

/*! Server-side timeouts (in milliseconds) for send()/recv(). */
static const int kiSCSIDaemonTimeoutMilliSec = 250;


struct iSCSIDIncomingRequestInfo {
    CFSocketRef socket;
    CFRunLoopSourceRef socketSourceRead;
    int fd;
};


const iSCSIDMsgLoginRsp iSCSIDMsgLoginRspInit = {
    .funcCode = kiSCSIDLogin,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILoginInvalidStatusCode
};

const iSCSIDMsgLogoutRsp iSCSIDMsgLogoutRspInit = {
    .funcCode = kiSCSIDLogout,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILogoutInvalidStatusCode
};

const iSCSIDMsgCreateArrayOfActiveTargetsRsp iSCSIDMsgCreateArrayOfActiveTargetsRspInit = {
    .funcCode = kiSCSIDCreateArrayOfActiveTargets,
    .errorCode = 0,
    .dataLength = 0
};

const iSCSIDMsgCreateArrayOfActivePortalsForTargetRsp iSCSIDMsgCreateArrayOfActivePortalsForTargetRspInit = {
    .funcCode = kiSCSIDCreateArrayOfActivePortalsForTarget,
    .errorCode = 0,
    .dataLength = 0
};

const iSCSIDMsgIsTargetActiveRsp iSCSIDMsgIsTargetActiveRspInit = {
    .funcCode = kiSCSIDIsTargetActive,
    .active = false
};

const iSCSIDMsgIsPortalActiveRsp iSCSIDMsgIsPortalActiveRspInit = {
    .funcCode = kiSCSIDIsPortalActive,
    .active = false
};

const iSCSIDMsgQueryTargetForAuthMethodRsp iSCSIDMsgQueryTargetForAuthMethodRspInit = {
    .funcCode = kiSCSIDQueryTargetForAuthMethod,
    .errorCode = 0,
    .statusCode = 0,
    .authMethod = 0
};

const iSCSIDMsgCreateCFPropertiesForSessionRsp iSCSIDMsgCreateCFPropertiesForSessionRspInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForSession,
    .errorCode = 0,
    .dataLength = 0
};

const iSCSIDMsgCreateCFPropertiesForConnectionRsp iSCSIDMsgCreateCFPropertiesForConnectionRspInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForConnection,
    .errorCode = 0,
    .dataLength = 0
};

const iSCSIDMsgUpdateDiscoveryRsp iSCSIDMsgUpdateDiscoveryRspInit = {
    .funcCode = kiSCSIDUpdateDiscovery,
    .errorCode = 0,
};

/*! Used for the logout process. */
typedef struct iSCSIDLogoutContext {
    int fd;
    DASessionRef diskSession;
    iSCSIPortalRef portal;
    errno_t errorCode;
    
} iSCSIDLogoutContext;


iSCSISessionConfigRef iSCSIDCreateSessionConfig(CFStringRef targetIQN)
{
    iSCSIMutableSessionConfigRef config = iSCSISessionConfigCreateMutable();

    iSCSISessionConfigSetErrorRecoveryLevel(config,iSCSIPLGetErrorRecoveryLevelForTarget(targetIQN));
    iSCSISessionConfigSetMaxConnections(config,iSCSIPLGetMaxConnectionsForTarget(targetIQN));

    return config;
}

iSCSIConnectionConfigRef iSCSIDCreateConnectionConfig(CFStringRef targetIQN,
                                                      CFStringRef portalAddress)
{
    iSCSIMutableConnectionConfigRef config = iSCSIConnectionConfigCreateMutable();

    enum iSCSIDigestTypes digestType;

    digestType = iSCSIPLGetDataDigestForTarget(targetIQN);

    if(digestType == kiSCSIDigestInvalid)
        digestType = kiSCSIDigestNone;

    iSCSIConnectionConfigSetDataDigest(config,digestType);

    digestType = iSCSIPLGetHeaderDigestForTarget(targetIQN);

    if(digestType == kiSCSIDigestInvalid)
        digestType = kiSCSIDigestNone;

    iSCSIConnectionConfigSetHeaderDigest(config,iSCSIPLGetHeaderDigestForTarget(targetIQN));

    return config;
}

iSCSIAuthRef iSCSIDCreateAuthenticationForTarget(CFStringRef targetIQN)
{
    iSCSIAuthRef auth;
    enum iSCSIAuthMethods authMethod = iSCSIPLGetTargetAuthenticationMethod(targetIQN);

    if(authMethod == kiSCSIAuthMethodCHAP)
    {
        CFStringRef name = iSCSIPLCopyTargetCHAPName(targetIQN);
        CFStringRef sharedSecret = iSCSIPLCopyTargetCHAPSecret(targetIQN);

        if(!name) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"CHAP name for target has not been set, reverting to no authentication");
            auth = iSCSIAuthCreateNone();
        }
        else if(!sharedSecret) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"CHAP secret is missing or insufficient privileges to system keychain, reverting to no authentication");
            auth = iSCSIAuthCreateNone();
        }
        else {
            auth = iSCSIAuthCreateCHAP(name,sharedSecret);
        }

        if(name)
            CFRelease(name);
        if(sharedSecret)
            CFRelease(sharedSecret);

    }
    else
        auth = iSCSIAuthCreateNone();

    return auth;
}

iSCSIAuthRef iSCSIDCreateAuthenticationForInitiator()
{
    iSCSIAuthRef auth;
    enum iSCSIAuthMethods authMethod = iSCSIPLGetInitiatorAuthenticationMethod();

    if(authMethod == kiSCSIAuthMethodCHAP)
    {
        CFStringRef name = iSCSIPLCopyInitiatorCHAPName();
        CFStringRef sharedSecret = iSCSIPLCopyInitiatorCHAPSecret();

        if(!name) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"CHAP name for target has not been set, reverting to no authentication.");
            auth = iSCSIAuthCreateNone();
        }
        else if(!sharedSecret) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"CHAP secret is missing or insufficient privileges to system keychain, reverting to no authentication.");
            auth = iSCSIAuthCreateNone();
        }
        else {
            auth = iSCSIAuthCreateCHAP(name,sharedSecret);
        }

        if(name)
            CFRelease(name);
        if(sharedSecret)
            CFRelease(sharedSecret);
    }
    else
        auth = iSCSIAuthCreateNone();

    return auth;
}

errno_t iSCSIDLoginCommon(SID sessionId,
                          iSCSITargetRef target,
                          iSCSIPortalRef portal,
                          enum iSCSILoginStatusCode * statusCode)
{
    errno_t error = 0;
    iSCSISessionConfigRef sessCfg = NULL;
    iSCSIConnectionConfigRef connCfg = NULL;
    iSCSIAuthRef initiatorAuth = NULL, targetAuth = NULL;

    CID connectionId = kiSCSIInvalidConnectionId;

    *statusCode = kiSCSILoginInvalidStatusCode;

    CFStringRef targetIQN = iSCSITargetGetIQN(target);

    // If session needs to be logged in, copy session config from property list
    if(sessionId == kiSCSIInvalidSessionId)
        if(!(sessCfg = iSCSIDCreateSessionConfig(targetIQN)))
            sessCfg = iSCSISessionConfigCreateMutable();

    // Get connection configuration from property list, create one if needed
    if(!(connCfg = iSCSIDCreateConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal))))
        connCfg = iSCSIConnectionConfigCreateMutable();

    // Get authentication configuration from property list, create one if needed
    if(!(targetAuth = iSCSIDCreateAuthenticationForTarget(targetIQN)))
        targetAuth = iSCSIAuthCreateNone();

    if(!(initiatorAuth = iSCSIDCreateAuthenticationForInitiator()))
       initiatorAuth = iSCSIAuthCreateNone();

    // Do either session or connection login
    if(sessionId == kiSCSIInvalidSessionId)
        error = iSCSILoginSession(target,portal,initiatorAuth,targetAuth,sessCfg,connCfg,&sessionId,&connectionId,statusCode);
    else
        error = iSCSILoginConnection(sessionId,portal,initiatorAuth,targetAuth,connCfg,&connectionId,statusCode);

    // Log error message
    if(error) {
        CFStringRef errorString = CFStringCreateWithFormat(
            kCFAllocatorDefault,0,
            CFSTR("login to %@,%@:%@ failed: %s\n"),
            targetIQN,
            iSCSIPortalGetAddress(portal),
            iSCSIPortalGetPort(portal),
            strerror(error));

        asl_log(NULL,NULL,ASL_LEVEL_ERR,"%s",CFStringGetCStringPtr(errorString,kCFStringEncodingASCII));
        CFRelease(errorString);
    }

    return error;
}


errno_t iSCSIDLoginAllPortals(iSCSITargetRef target,
                              enum iSCSILoginStatusCode * statusCode)
{
    CFIndex activeConnections = 0;
    CFIndex maxConnections = 0;

    // Error code to return to daemon's client
    errno_t errorCode = 0;
    *statusCode = kiSCSILoginInvalidStatusCode;

    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    SID sessionId = iSCSIGetSessionIdForTarget(targetIQN);

    // Set initial values for maxConnections and activeConnections
    if(sessionId == kiSCSIInvalidSessionId)
//TODO: change to the maximum value that will be supported by this initiator
        maxConnections = 1;
    else {

        // If session exists, get the max connections and active connections
        CFDictionaryRef properties = iSCSICreateCFPropertiesForSession(target);

        if(properties) {
            // Get max connections from property list
            CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
            CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);
            CFRelease(properties);

            CFArrayRef connections = iSCSICreateArrayOfConnectionsIds(sessionId);

            if(connections) {
                activeConnections = CFArrayGetCount(connections);
                CFRelease(connections);
            }
        }
    }

    // Add portals to the session until we've run out of portals to add or
    // reached the maximum connection limit
    CFStringRef portalAddress = NULL;
    CFArrayRef portals = iSCSIPLCreateArrayOfPortalsForTarget(targetIQN);
    CFIndex portalIdx = 0;
    CFIndex portalCount = CFArrayGetCount(portals);

    while( (activeConnections < maxConnections) && portalIdx < portalCount )
    {
        portalAddress = CFArrayGetValueAtIndex(portals,portalIdx);

        // Get portal object and login
        iSCSIPortalRef portal = iSCSIPLCopyPortalForTarget(targetIQN,portalAddress);
        errorCode = iSCSIDLoginCommon(sessionId,target,portal,statusCode);
        iSCSIPortalRelease(portal);

        // Quit if there was an error communicating with the daemon
        if(errorCode)
            break;

        activeConnections++;
        portalIdx++;

        // Determine how many connections this session supports
        sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target));

        // If this was the first connection of the session, get the number of
        // allowed maximum connections
        if(activeConnections == 1) {
            CFDictionaryRef properties = iSCSICreateCFPropertiesForSession(target);
            if(properties) {
                // Get max connections from property list
                CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
                CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);
                CFRelease(properties);
            }
        }
    };
    
    return errorCode;
}


errno_t iSCSIDLoginWithPortal(iSCSITargetRef target,
                              iSCSIPortalRef portal,
                              enum iSCSILoginStatusCode * statusCode)
{
    // Check for active sessions before attempting loginb
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    *statusCode = kiSCSILoginInvalidStatusCode;
    errno_t errorCode = 0;

    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    sessionId = iSCSIGetSessionIdForTarget(targetIQN);

    // Existing session, add a connection
    if(sessionId != kiSCSIInvalidSessionId) {

        connectionId = iSCSIGetConnectionIdForPortal(sessionId,portal);

        // If there's an active session display error otherwise login
        if(connectionId != kiSCSIInvalidConnectionId)
        {} //iSCSICtlDisplayError("The specified target has an active session over the specified portal.");
        else {
            // See if the session can support an additional connection
            CFDictionaryRef properties = iSCSICreateCFPropertiesForSession(target);
            if(properties) {
                // Get max connections from property list
                UInt32 maxConnections;
                CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
                CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);
                CFRelease(properties);

                CFArrayRef connections = iSCSICreateArrayOfConnectionsIds(sessionId);
                if(connections)
                {
                    CFIndex activeConnections = CFArrayGetCount(connections);
                    if(activeConnections == maxConnections)
                    {} //iSCSICtlDisplayError("The active session cannot support additional connections.");
                    else
                        errorCode = iSCSIDLoginCommon(sessionId,target,portal,statusCode);
                    CFRelease(connections);
                }
            }
        }

    }
    else  // Leading login
        errorCode = iSCSIDLoginCommon(sessionId,target,portal,statusCode);

    return errorCode;
}


errno_t iSCSIDLogin(int fd,iSCSIDMsgLoginCmd * cmd)
{
    CFDataRef targetData = NULL, portalData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);
    iSCSITargetRef target = NULL;

    if(targetData) {
        target = iSCSITargetCreateWithData(targetData);
        CFRelease(targetData);
    }

    iSCSIPortalRef portal = NULL;

    if(portalData) {
        portal = iSCSIPortalCreateWithData(portalData);
        CFRelease(portalData);
    }

    // If portal and target are valid, login with portal.  Otherwise login to
    // target using all defined portals.
    errno_t errorCode = 0;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;

    // Synchronize property list
    iSCSIPLSynchronize();

    if(target && portal)
        errorCode = iSCSIDLoginWithPortal(target,portal,&statusCode);
    else if(target)
        errorCode = iSCSIDLoginAllPortals(target,&statusCode);
    else
        errorCode = EINVAL;

    // Compose a response to send back to the client
    iSCSIDMsgLoginRsp rsp = iSCSIDMsgLoginRspInit;
    rsp.errorCode = errorCode;
    rsp.statusCode = statusCode;

    if(target)
        iSCSITargetRelease(target);

    if(portal)
        iSCSIPortalRelease(portal);

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

void iSCSIDLogoutComplete(iSCSITargetRef target,enum iSCSIDAOperationResult result,void * context)
{
    // At this point either the we logout the session or just the connection
    // associated with the specified portal, if one was specified
    iSCSIDLogoutContext * ctx = (iSCSIDLogoutContext*)context;

    // Store local copies and free structure
    int fd = ctx->fd;
    errno_t errorCode = ctx->errorCode;
    iSCSIPortalRef portal = ctx->portal;
    CFRelease(ctx->diskSession);
    free(ctx);

    enum iSCSILogoutStatusCode statusCode;
    
    if(!errorCode)
    {
        SID sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target));

        // For session logout, ensure that disk unmount was successful...
        if(!portal) {
            
            if(result == kiSCSIDAOperationSuccess)
                errorCode = iSCSILogoutSession(sessionId,&statusCode);
            else
                errorCode = EBUSY;
        }
        else {
            CID connectionId = iSCSIGetConnectionIdForPortal(sessionId,portal);
            errorCode = iSCSILogoutConnection(sessionId,connectionId,&statusCode);
        }
    }
    
    // Log error message
    if(errorCode) {
        CFStringRef errorString;
        
        if(!portal) {
            errorString = CFStringCreateWithFormat(
               kCFAllocatorDefault,0,
               CFSTR("logout of %@ failed: %s\n"),
               iSCSITargetGetIQN(target),
               strerror(errorCode));
        }
        else {
            errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("logout of %@,%@:%@ failed: %s\n"),
                iSCSITargetGetIQN(target),
                iSCSIPortalGetAddress(portal),
                iSCSIPortalGetPort(portal),
                strerror(errorCode));
        }
        
        asl_log(NULL,NULL,ASL_LEVEL_ERR,"%s",CFStringGetCStringPtr(errorString,kCFStringEncodingASCII));
        CFRelease(errorString);
    }

    if(portal)
        iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);
    
    // Compose a response to send back to the client
    iSCSIDMsgLogoutRsp rsp = iSCSIDMsgLogoutRspInit;
    rsp.errorCode = errorCode;
    rsp.statusCode = statusCode;
    
    send(fd,&rsp,sizeof(rsp),0);
}

errno_t iSCSIDLogout(int fd,iSCSIDMsgLogoutCmd * cmd)
{
    CFDataRef targetData = NULL, portalData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);
    errno_t errorCode = 0;

    iSCSITargetRef target = NULL;

    if(targetData) {
        target = iSCSITargetCreateWithData(targetData);
        CFRelease(targetData);
    }

    iSCSIPortalRef portal = NULL;

    if(portalData) {
        portal = iSCSIPortalCreateWithData(portalData);
        CFRelease(portalData);
    }

    // See if there exists an active session for this target
    SID sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target));

    if(sessionId == kiSCSIInvalidSessionId)
    {
        CFStringRef errorString = CFStringCreateWithFormat(
            kCFAllocatorDefault,0,
            CFSTR("logout of %@ failed: the target has no active sessions"),
            iSCSITargetGetIQN(target));
        
        asl_log(0,0,ASL_LEVEL_CRIT,"%s",CFStringGetCStringPtr(errorString,kCFStringEncodingASCII));
        CFRelease(errorString);
        errorCode = EINVAL;
    }
    
    // See if there exists an active connection for this portal
    CID connectionId = kiSCSIInvalidConnectionId;
    CFIndex connectionCount = 0;
    
    if(!errorCode && portal) {
        connectionId = iSCSIGetConnectionIdForPortal(sessionId,portal);
        
        if(connectionId == kiSCSIInvalidConnectionId) {
        
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("logout of %@,%@:%@ failed: the portal has no active connections"),
                iSCSITargetGetIQN(target),iSCSIPortalGetAddress(portal),iSCSIPortalGetPort(portal));
        
            asl_log(0,0,ASL_LEVEL_CRIT,"%s",CFStringGetCStringPtr(errorString,kCFStringEncodingASCII));
            CFRelease(errorString);
            errorCode = EINVAL;
        }
        else {
            // Determine the number of connections
            CFArrayRef connectionIds = iSCSICreateArrayOfConnectionsIds(sessionId);
            connectionCount = CFArrayGetCount(connectionIds);
            CFRelease(connectionIds);
        }
    }
    
    // Unmount volumes if portal not specified (session logout)
    // or if portal is specified and is only connection...
    iSCSIDLogoutContext * context = (iSCSIDLogoutContext*)malloc(sizeof(iSCSIDLogoutContext));
    context->fd = fd;
    context->diskSession = DASessionCreate(kCFAllocatorDefault);
    context->portal = portal;
    context->errorCode = errorCode;
    
    if(!errorCode) {
        if(!portal || connectionCount == 1) {
            DASessionScheduleWithRunLoop(context->diskSession,CFRunLoopGetMain(),kCFRunLoopDefaultMode);
            iSCSIDAUnmountForTarget(context->diskSession,kDADiskUnmountOptionWhole,target,&iSCSIDLogoutComplete,context);
        }
        else {
            iSCSIDLogoutComplete(target,kiSCSIDAOperationSuccess,context);
        }
    }

    return 0;
}

errno_t iSCSIDCreateArrayOfActiveTargets(int fd,iSCSIDMsgCreateArrayOfActiveTargetsCmd * cmd)
{
    CFArrayRef sessionIds = iSCSICreateArrayOfSessionIds();
    CFIndex sessionCount = CFArrayGetCount(sessionIds);

    // Prepare an array to hold our targets
    CFMutableArrayRef activeTargets = CFArrayCreateMutable(kCFAllocatorDefault,
                                                           sessionCount,
                                                           &kCFTypeArrayCallBacks);

    // Get target object for each active session and add to array
    for(CFIndex idx = 0; idx < sessionCount; idx++)
    {
        iSCSITargetRef target = iSCSICreateTargetForSessionId((SID)CFArrayGetValueAtIndex(sessionIds,idx));
        CFArrayAppendValue(activeTargets,target);
        iSCSITargetRelease(target);
    }

    // Serialize and send array
    CFDataRef data = CFPropertyListCreateData(kCFAllocatorDefault,
                                              (CFPropertyListRef) activeTargets,
                                              kCFPropertyListBinaryFormat_v1_0,0,NULL);
    CFRelease(activeTargets);

    // Send response header
    iSCSIDMsgCreateArrayOfActiveTargetsRsp rsp = iSCSIDMsgCreateArrayOfActiveTargetsRspInit;
    if(data)
        rsp.dataLength = (UInt32)CFDataGetLength(data);
    else
        rsp.dataLength = 0;

    iSCSIDaemonSendMsg(fd,(iSCSIDMsgGeneric*)&rsp,data,NULL);

    if(data)
        CFRelease(data);

    return 0;
}

errno_t iSCSIDCreateArrayofActivePortalsForTarget(int fd,
                                                  iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd * cmd)
{
    CFArrayRef sessionIds = iSCSICreateArrayOfSessionIds();
    CFIndex sessionCount = CFArrayGetCount(sessionIds);

    // Prepare an array to hold our targets
    CFMutableArrayRef activeTargets = CFArrayCreateMutable(kCFAllocatorDefault,
                                                           sessionCount,
                                                           &kCFTypeArrayCallBacks);

    // Get target object for each active session and add to array
    for(CFIndex idx = 0; idx < sessionCount; idx++)
    {
        iSCSITargetRef target = iSCSICreateTargetForSessionId((SID)CFArrayGetValueAtIndex(sessionIds,idx));
        CFArrayAppendValue(activeTargets,target);
        iSCSITargetRelease(target);
    }

    // Serialize and send array
    CFDataRef data = CFPropertyListCreateData(kCFAllocatorDefault,
                                              (CFPropertyListRef) activeTargets,
                                              kCFPropertyListBinaryFormat_v1_0,0,NULL);
    CFRelease(activeTargets);

    // Send response header
    iSCSIDMsgCreateArrayOfActivePortalsForTargetRsp rsp = iSCSIDMsgCreateArrayOfActivePortalsForTargetRspInit;
    if(data)
        rsp.dataLength = (UInt32)CFDataGetLength(data);
    else
        rsp.dataLength = 0;

    iSCSIDaemonSendMsg(fd,(iSCSIDMsgGeneric*)&rsp,data,NULL);

    if(data)
        CFRelease(data);

    return 0;
}

errno_t iSCSIDIsTargetActive(int fd,iSCSIDMsgIsTargetActiveCmd *cmd)
{
    CFDataRef targetData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,NULL);

    iSCSITargetRef target = NULL;

    if(targetData) {
        target = iSCSITargetCreateWithData(targetData);
        CFRelease(targetData);
    }

    if(target) {
        iSCSIDMsgIsTargetActiveRsp rsp = iSCSIDMsgIsTargetActiveRspInit;
        rsp.active = (iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target)) != kiSCSIInvalidSessionId);

        if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
            return EAGAIN;
    }
    return 0;
}

errno_t iSCSIDIsPortalActive(int fd,iSCSIDMsgIsPortalActiveCmd *cmd)
{
    CFDataRef targetData = NULL, portalData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);

    iSCSITargetRef target = NULL;

    if(targetData) {
        target = iSCSITargetCreateWithData(targetData);
        CFRelease(targetData);
    }

    iSCSIPortalRef portal = NULL;

    if(portalData) {
        portal = iSCSIPortalCreateWithData(portalData);
        CFRelease(portalData);
    }

    iSCSIDMsgIsPortalActiveRsp rsp = iSCSIDMsgIsPortalActiveRspInit;
    SID sessionId = (iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target)));

    if(sessionId == kiSCSIInvalidSessionId)
        rsp.active = false;
    else
        rsp.active = (iSCSIGetConnectionIdForPortal(sessionId,portal) != kiSCSIInvalidConnectionId);

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDQueryTargetForAuthMethod(int fd,iSCSIDMsgQueryTargetForAuthMethodCmd * cmd)
{
    CFDataRef targetData = NULL, portalData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);

    iSCSITargetRef target = NULL;

    if(targetData) {
        target = iSCSITargetCreateWithData(targetData);
        CFRelease(targetData);
    }

    iSCSIPortalRef portal = NULL;

    if(portalData) {
        portal = iSCSIPortalCreateWithData(portalData);
        CFRelease(portalData);
    }

    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;

    errno_t error = iSCSIQueryTargetForAuthMethod(portal,iSCSITargetGetIQN(target),&authMethod,&statusCode);

    // Compose a response to send back to the client
    iSCSIDMsgQueryTargetForAuthMethodRsp rsp = iSCSIDMsgQueryTargetForAuthMethodRspInit;
    rsp.errorCode = error;
    rsp.statusCode = statusCode;
    rsp.authMethod = authMethod;

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDCreateCFPropertiesForSession(int fd,
                                           iSCSIDMsgCreateCFPropertiesForSessionCmd * cmd)
{
    CFMutableDataRef targetData = NULL;
    errno_t error = iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,NULL);

    iSCSITargetRef target = NULL;

    if(!error) {

        if(targetData) {
            target = iSCSITargetCreateWithData(targetData);
            CFRelease(targetData);
        }

        if(!target)
            error  = EINVAL;
    }

    if(!error) {
        CFDictionaryRef properties = iSCSICreateCFPropertiesForSession(target);

        // Send back response
        iSCSIDMsgCreateCFPropertiesForSessionRsp rsp = iSCSIDMsgCreateCFPropertiesForSessionRspInit;

        CFDataRef data = NULL;
        if(properties) {

            data = CFPropertyListCreateData(kCFAllocatorDefault,
                                            (CFPropertyListRef)properties,
                                            kCFPropertyListBinaryFormat_v1_0,0,NULL);

            rsp.dataLength = (UInt32)CFDataGetLength(data);
            CFRelease(properties);
        }
        else
            rsp.dataLength = 0;

        error = iSCSIDaemonSendMsg(fd,(iSCSIDMsgGeneric*)&rsp,data,NULL);

        if(data)
            CFRelease(data);
    }
    return error;
}

errno_t iSCSIDCreateCFPropertiesForConnection(int fd,
                                              iSCSIDMsgCreateCFPropertiesForConnectionCmd * cmd)
{
    CFDataRef targetData = NULL, portalData = NULL;
    errno_t error = iSCSIDaemonRecvMsg(fd,0,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);

    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;

    if(!error) {

        if(targetData) {
            target = iSCSITargetCreateWithData(targetData);
            CFRelease(targetData);
        }

        if(portalData) {
            portal = iSCSIPortalCreateWithData(portalData);
            CFRelease(portalData);
        }

        if(!target || !portal)
            error = EINVAL;
    }

    if(!error) {

        CFDictionaryRef properties = iSCSICreateCFPropertiesForConnection(target,portal);

        // Send back response
        iSCSIDMsgCreateCFPropertiesForConnectionRsp rsp = iSCSIDMsgCreateCFPropertiesForConnectionRspInit;

        CFDataRef data = NULL;
        if(properties) {
            data = CFPropertyListCreateData(kCFAllocatorDefault,
                                            (CFPropertyListRef)properties,
                                            kCFPropertyListBinaryFormat_v1_0,0,NULL);

            rsp.dataLength = (UInt32)CFDataGetLength(data);
            CFRelease(properties);
        }
        else
            rsp.dataLength = 0;
        
        error = iSCSIDaemonSendMsg(fd,(iSCSIDMsgGeneric*)&rsp,data,NULL);
        
        if(data)
            CFRelease(data);
    }

    return error;
}

void * iSCSIDRunDiscovery(void * context)
{
    iSCSIDiscoveryRunSendTargets();
    pthread_mutex_unlock(&discoveryMutex);
    
    return NULL;
}


/*! Called on a timer (timer setup by iSCSIDUpdateDiscovery()) to run
 *  discovery operations on a dedicated POSIX thread. */
void iSCSIDLaunchDiscoveryThread(CFRunLoopTimerRef timer,void * context)
{
    pthread_attr_t  attribute;
    pthread_t thread;
    errno_t error = 0;
    
    if(pthread_mutex_trylock(&discoveryMutex))
    {
        error = pthread_attr_init(&attribute);
        assert(!error);
        error = pthread_attr_setdetachstate(&attribute,PTHREAD_CREATE_DETACHED);
        assert(!error);
        
        error = pthread_create(&thread,&attribute,&iSCSIDRunDiscovery,NULL);
        pthread_attr_destroy(&attribute);
    }
    else {
        asl_log(NULL,NULL,ASL_LEVEL_CRIT,"discovery is taking longer than the specified"
                " discovery interval. Consider increasing discovery interval");
    }

    // Log error if thread could not start
    if(error)
        asl_log(NULL,NULL,ASL_LEVEL_ALERT,"failed to start target discovery");
}

/*! Synchronizes the daemon with the property list. This function may be called
 *  anytime changes are made to the property list (e.g., by an external
 *  application) that require immediate action on the daemon's part. This
 *  includes for example, the initiator name and alias and discovery settings
 *  (whether discovery is enabled or disabled, and the time interval
 *  associated with discovery).
 *  @param fd unused, pass in 0 (retained for interface consistency). 
 *  @param cmd unused, pass in 0 (retained for interface consistency). */
errno_t iSCSIDUpdateDiscovery(int fd,
                              iSCSIDMsgUpdateDiscoveryCmd * cmd)
{
    errno_t error = 0;
    iSCSIPLSynchronize();

    // Check whether SendTargets discovery is enabled, and get interval (sec)
    Boolean discoveryEnabled = iSCSIPLGetSendTargetsDiscoveryEnable();
    CFTimeInterval interval = iSCSIPLGetSendTargetsDiscoveryInterval();
    CFRunLoopTimerCallBack callout = &iSCSIDLaunchDiscoveryThread;

    // Remove existing timer if one exists
    if(discoveryTimer) {
        CFRunLoopRemoveTimer(CFRunLoopGetCurrent(),discoveryTimer,kCFRunLoopDefaultMode);
        CFRelease(discoveryTimer);
        discoveryTimer = NULL;
    }

    // Add new timer with updated interval, if discovery is enabled
    if(discoveryEnabled)
    {
        discoveryTimer = CFRunLoopTimerCreate(kCFAllocatorDefault,
                                              CFAbsoluteTimeGetCurrent() + 2.0,
                                              interval,0,0,callout,NULL);
        


        CFRunLoopAddTimer(CFRunLoopGetCurrent(),discoveryTimer,kCFRunLoopDefaultMode);
    }

    // Send back response
    iSCSIDMsgUpdateDiscoveryRsp rsp = iSCSIDMsgUpdateDiscoveryRspInit;
    rsp.errorCode = 0;
    iSCSIPLSynchronize();

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        error = EAGAIN;

    return error;
}

/*! Automatically logs in to targets that were specified for auto-login.
 *  Used during startup of the daemon to log in to either static 
 *  dynamic targets for which the auto-login option is enabled. */
void iSCSIDAutoLogin()
{
    // Iterate over all targets and auto-login as required
    iSCSIPLSynchronize();
    
    CFArrayRef targets = NULL;

    if(!(targets = iSCSIPLCreateArrayOfTargets()))
       return;
    
    CFIndex targetsCount = CFArrayGetCount(targets);
    
    for(CFIndex idx = 0; idx < targetsCount; idx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(targets,idx);
        
        if(iSCSIPLGetAutoLoginForTarget(targetIQN)) {
            iSCSITargetRef target = iSCSIPLCopyTarget(targetIQN);
            enum iSCSILoginStatusCode statusCode;
            iSCSIDLoginAllPortals(target,&statusCode);
            iSCSITargetRelease(target);
        }
    }
    CFRelease(targets);
}

void iSCSIDPrepareForSystemSleep()
{
    // Unmount all volumes associated with active targets
    CFArrayRef sessionIds = iSCSICreateArrayOfSessionIds();
    
    if(!sessionIds)
        return;
    
    CFIndex sessionCount = CFArrayGetCount(sessionIds);
    
    for(CFIndex idx = 0; idx < sessionCount; idx++)
    {
        SID sessionId = (SID)CFArrayGetValueAtIndex(sessionIds,idx);
        iSCSITargetRef target = iSCSICreateTargetForSessionId(sessionId);
        
        CFStringRef targetIQN = iSCSITargetGetIQN(target);
//        iSCSIDAUnmountIOMediaForTarget(targetIQN);
        iSCSITargetRelease(target);
    }
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
        // The system will go to sleep (we have no control)
        case kIOMessageSystemWillSleep:
            iSCSIDPrepareForSystemSleep();
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

void iSCSIDProcessIncomingRequest(void * info)
{
    struct iSCSIDIncomingRequestInfo * reqInfo = (struct iSCSIDIncomingRequestInfo*)info;
    iSCSIDMsgCmd cmd;
    
    int fd = reqInfo->fd;
    
    if(fd != 0 && recv(fd,&cmd,sizeof(cmd),MSG_WAITALL) == sizeof(cmd)) {
        
        errno_t error = 0;
        switch(cmd.funcCode)
        {
            case kiSCSIDLogin:
                error = iSCSIDLogin(fd,(iSCSIDMsgLoginCmd*)&cmd); break;
            case kiSCSIDLogout:
                error = iSCSIDLogout(fd,(iSCSIDMsgLogoutCmd*)&cmd); break;
            case kiSCSIDCreateArrayOfActiveTargets:
                error = iSCSIDCreateArrayOfActiveTargets(fd,(iSCSIDMsgCreateArrayOfActiveTargetsCmd*)&cmd); break;
            case kiSCSIDCreateArrayOfActivePortalsForTarget:
                error = iSCSIDCreateArrayofActivePortalsForTarget(fd,(iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd*)&cmd); break;
            case kiSCSIDIsTargetActive:
                error = iSCSIDIsTargetActive(fd,(iSCSIDMsgIsTargetActiveCmd*)&cmd); break;
            case kiSCSIDIsPortalActive:
                error = iSCSIDIsPortalActive(fd,(iSCSIDMsgIsPortalActiveCmd*)&cmd); break;
            case kiSCSIDQueryTargetForAuthMethod:
                error = iSCSIDQueryTargetForAuthMethod(fd,(iSCSIDMsgQueryTargetForAuthMethodCmd*)&cmd); break;
            case kiSCSIDCreateCFPropertiesForSession:
                error = iSCSIDCreateCFPropertiesForSession(fd,(iSCSIDMsgCreateCFPropertiesForSessionCmd*)&cmd); break;
            case kiSCSIDCreateCFPropertiesForConnection:
                error = iSCSIDCreateCFPropertiesForConnection(fd,(iSCSIDMsgCreateCFPropertiesForConnectionCmd*)&cmd); break;
            case kiSCSIDUpdateDiscovery:
                error = iSCSIDUpdateDiscovery(fd,(iSCSIDMsgUpdateDiscoveryCmd*)&cmd); break;
            default:
                CFSocketInvalidate(reqInfo->socket);
                reqInfo->fd = 0;
        };
    }
    
    // If a request came in while we were processing, queue it up...
    if(reqInfo->fd && recv(fd,&cmd,sizeof(cmd),MSG_PEEK) > 0) {
            CFRunLoopSourceSignal(reqInfo->socketSourceRead);
    }
}

/*! Handle an incoming connection from iscsictl. Once a connection is
 *  established, main runloop calls this function. This function processes
 *  all incoming commands until a shutdown request is received, at which point
 *  this function terminates and returns control to the run loop. For this
 *  reason, no other commands (e.g., timers) can be executed until the 
 *  incoming request is processed. */
void iSCSIDAcceptConnection(CFSocketRef socket,
                            CFSocketCallBackType callbackType,
                            CFDataRef address,
                            const void * data,
                            void * info)
{
    struct iSCSIDIncomingRequestInfo * reqInfo = (struct iSCSIDIncomingRequestInfo*)info;

    // Get file descriptor associated with the connection
    if(callbackType == kCFSocketAcceptCallBack) {
        if(reqInfo->fd != 0) {
            close(reqInfo->fd);
        }
        
        reqInfo->fd = *(CFSocketNativeHandle*)data;
        
        // Set timeouts for send() and recv()
        struct timeval tv;
        memset(&tv,0,sizeof(tv));
        tv.tv_usec = kiSCSIDaemonTimeoutMilliSec*1000;
        
        setsockopt(reqInfo->fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
        setsockopt(reqInfo->fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    }
    
    iSCSIDProcessIncomingRequest(info);
}


/*! iSCSI daemon entry point. */
int main(void)
{
    // Initialize logging
    asl_object_t log = asl_open(NULL,NULL,ASL_OPT_STDERR);

    // Read configuration parameters from the iSCSI property list
    iSCSIPLSynchronize();

    // Update initiator name and alias internally
    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    if(initiatorIQN) {
        iSCSISetInitiatorName(initiatorIQN);
        CFRelease(initiatorIQN);
    }
    else {
        asl_log(NULL,NULL,ASL_LEVEL_WARNING,"initiator IQN not set, reverting to internal default");
    }

    CFStringRef initiatorAlias = iSCSIPLCopyInitiatorAlias();

    if(initiatorAlias) {
        iSCSISetInitiatorAlias(initiatorAlias);
        CFRelease(initiatorAlias);
    }
    else {
        asl_log(NULL,NULL,ASL_LEVEL_WARNING,"initiator alias not set, reverting to internal default");
    }

    // Register with launchd so it can manage this daemon
    launch_data_t reg_request = launch_data_new_string(LAUNCH_KEY_CHECKIN);

    // Quit if we are unable to checkin...
    if(!reg_request) {
        asl_log(NULL,NULL,ASL_LEVEL_ALERT,"failed to checkin with launchd");
        goto ERROR_LAUNCH_DATA;
    }

    launch_data_t reg_response = launch_msg(reg_request);

    // Ensure registration was successful
    if((launch_data_get_type(reg_response) == LAUNCH_DATA_ERRNO)) {
        asl_log(NULL,NULL,ASL_LEVEL_ALERT,"failed to checkin with launchd");
        goto ERROR_NO_SOCKETS;
    }

    // Grab label and socket dictionary from daemon's property list
    launch_data_t label = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_LABEL);
    launch_data_t sockets = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_SOCKETS);

    if(!label || !sockets) {
        asl_log(NULL,NULL,ASL_LEVEL_ALERT,"could not find socket definition, plist may be damaged");
        goto ERROR_NO_SOCKETS;
    }

    launch_data_t listen_socket_array = launch_data_dict_lookup(sockets,"iscsid");

    if(!listen_socket_array || launch_data_array_get_count(listen_socket_array) == 0)
        goto ERROR_NO_SOCKETS;

    // Grab handle to socket we want to listen on...
    launch_data_t listen_socket = launch_data_array_get_index(listen_socket_array,0);

    if(!iSCSIDRegisterForPowerEvents()) {
        asl_log(NULL,NULL,ASL_LEVEL_ALERT,"could not register to receive system power events");
        goto ERROR_PWR_MGMT_FAIL;
    }
    
    // Context for processing incoming requests. Holds references to CFSocket and
    // associated structures (e.g., runloop sources)
    struct iSCSIDIncomingRequestInfo * reqInfo = malloc(sizeof(struct iSCSIDIncomingRequestInfo));
    
    // Create a socket with a callback to accept incoming connections
    CFSocketContext sockContext;
    bzero(&sockContext,sizeof(sockContext));
    sockContext.info = (void*)reqInfo;

    CFSocketRef socket = CFSocketCreateWithNative(kCFAllocatorDefault,
                                                  launch_data_get_fd(listen_socket),
                                                  kCFSocketAcceptCallBack,
                                                  iSCSIDAcceptConnection,&sockContext);
    
    // Runloop source for CFSocket callback (iSCSIDAcceptConnection)
    CFRunLoopSourceRef sockSourceAccept = CFSocketCreateRunLoopSource(kCFAllocatorDefault,socket,0);
    CFRunLoopAddSource(CFRunLoopGetMain(),sockSourceAccept,kCFRunLoopDefaultMode);
    
    // Runloop source signaled by daemon to queue processing of incoming data
    CFRunLoopSourceRef sockSourceRead;
    CFRunLoopSourceContext readContext;
    bzero(&readContext,sizeof(readContext));
    readContext.info = (void*)reqInfo;
    readContext.perform = iSCSIDProcessIncomingRequest;
    sockSourceRead = CFRunLoopSourceCreate(kCFAllocatorDefault,1,&readContext);
    CFRunLoopAddSource(CFRunLoopGetMain(),sockSourceRead,kCFRunLoopDefaultMode);
    
    // Setup context
    reqInfo->socket = socket;
    reqInfo->socketSourceRead = sockSourceRead;
    reqInfo->fd = 0;

    asl_log(NULL,NULL,ASL_LEVEL_INFO,"daemon started");

    // Ignore SIGPIPE (generated when the client closes the connection)
    signal(SIGPIPE,SIG_IGN);
    
    // Initialize iSCSI connection to kernel (ability to call iSCSI kernel
    // functions and receive notifications from the kernel).
    iSCSIInitialize(CFRunLoopGetMain());
    
    // Sync discovery parameters upon startup
    iSCSIDUpdateDiscovery(0,NULL);
    
    // Auto-login upon startup
    iSCSIDAutoLogin();
    
    CFRunLoopRun();
    iSCSICleanup();
    
    // Deregister for power
    iSCSIDDeregisterForPowerEvents();
    
    // Free all CF objects and reqInfo structure...
    
    launch_data_free(reg_response);
    asl_close(log);
    return 0;
    
    // TODO: verify that launch data is freed under all possible execution paths
    
ERROR_PWR_MGMT_FAIL:
ERROR_NO_SOCKETS:
    launch_data_free(reg_response);
    
ERROR_LAUNCH_DATA:
    asl_close(log);
    return ENOTSUP;
}

