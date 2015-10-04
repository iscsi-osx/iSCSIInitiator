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


// Used to notify daemon of power state changes
io_connect_t powerPlaneRoot;
io_object_t powerNotifier;
IONotificationPortRef powerNotifyPortRef;

CFRunLoopTimerRef discoveryTimer = NULL;

const struct iSCSIDRspLogin iSCSIDRspLoginInit = {
    .funcCode = kiSCSIDLogin,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILoginInvalidStatusCode
};

const struct iSCSIDRspLogout iSCSIDRspLogoutInit = {
    .funcCode = kiSCSIDLogout,
    .errorCode = 0,
    .statusCode = (UInt8)kiSCSILogoutInvalidStatusCode
};

const struct iSCSIDRspCreateArrayOfActiveTargets iSCSIDRspCreateArrayOfActiveTargetsInit = {
    .funcCode = kiSCSIDCreateArrayOfActiveTargets,
    .errorCode = 0,
    .dataLength = 0
};

const struct iSCSIDRspCreateArrayOfActivePortalsForTarget iSCSIDRspCreateArrayOfActivePortalsForTargetInit = {
    .funcCode = kiSCSIDCreateArrayOfActivePortalsForTarget,
    .errorCode = 0,
    .dataLength = 0
};

const struct iSCSIDRspIsTargetActive iSCSIDRspIsTargetActiveInit = {
    .funcCode = kiSCSIDIsTargetActive,
    .active = false
};

const struct iSCSIDRspIsPortalActive iSCSIDRspIsPortalActiveInit = {
    .funcCode = kiSCSIDIsPortalActive,
    .active = false
};

const struct iSCSIDRspQueryTargetForAuthMethod iSCSIDRspQueryTargetForAuthMethodInit = {
    .funcCode = kiSCSIDQueryTargetForAuthMethod,
    .errorCode = 0,
    .statusCode = 0,
    .authMethod = 0
};

const struct iSCSIDRspCreateCFPropertiesForSession iSCSIDRspCreateCFPropertiesForSessionInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForSession,
    .errorCode = 0,
    .dataLength = 0
};

const struct iSCSIDRspCreateCFPropertiesForConnection iSCSIDRspCreateCFPropertiesForConnectionInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForConnection,
    .errorCode = 0,
    .dataLength = 0
};

const struct iSCSIDRspUpdateDiscovery iSCSIDRspUpdateDiscoveryInit = {
    .funcCode = kiSCSIDUpdateDiscovery,
    .errorCode = 0,
};


void iSCSIDLogError(CFStringRef logEntry)
{
    CFStringRef stamp = CFDateFormatterCreateStringWithAbsoluteTime(
                                                kCFAllocatorDefault,
                                                0,
                                                CFAbsoluteTimeGetCurrent());
    fprintf(stderr,"%s %s\n",
            CFStringGetCStringPtr(stamp,kCFStringEncodingASCII),
            CFStringGetCStringPtr(logEntry,kCFStringEncodingASCII));
}

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
            iSCSIDLogError(CFSTR("CHAP name for target has not been set, reverting to no authentication."));
            auth = iSCSIAuthCreateNone();
        }
        else if(!sharedSecret) {
            iSCSIDLogError(CFSTR("CHAP secret is missing or insufficient privileges to system keychain, reverting to no authentication."));
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
            iSCSIDLogError(CFSTR("CHAP name for target has not been set, reverting to no authentication."));
            auth = iSCSIAuthCreateNone();
        }
        else if(!sharedSecret) {
            iSCSIDLogError(CFSTR("CHAP secret is missing or insufficient privileges to system keychain, reverting to no authentication."));
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


errno_t iSCSIDLogin(int fd,struct iSCSIDCmdLogin * cmd)
{

    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                                         (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

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
    struct iSCSIDRspLogin rsp = iSCSIDRspLoginInit;
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


errno_t iSCSIDLogout(int fd,struct iSCSIDCmdLogout * cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                                         (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    enum iSCSILogoutStatusCode statusCode = kiSCSILogoutInvalidStatusCode;

    // Error code to return to daemon's client
    errno_t errorCode = 0;

    // Synchronize property list
    iSCSIPLSynchronize();

    // See if there exists an active session for this target
    if((sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target))) == kiSCSIInvalidSessionId)
    {
        //iSCSICtlDisplayError("The specified target has no active session.");
        errorCode = EINVAL;
    }

    // See if there exists an active connection for this portal
    if(!errorCode && portal)
        connectionId = iSCSIGetConnectionIdForPortal(sessionId,portal);

    // If the portal was specified and a connection doesn't exist for it...
    if(!errorCode && portal && connectionId == kiSCSIInvalidConnectionId)
    {
        //iSCSICtlDisplayError("The specified portal has no active connections.");
        errorCode = EINVAL;
    }

    // At this point either the we logout the session or just the connection
    // associated with the specified portal, if one was specified
    if(!errorCode)
    {
        if(!portal)
            errorCode = iSCSILogoutSession(sessionId,&statusCode);
        else
            errorCode = iSCSILogoutConnection(sessionId,connectionId,&statusCode);
        /*
         if(!error)
         iSCSICtlDisplayLogoutStatus(statusCode,target,portal);
         else
         iSCSICtlDisplayError(strerror(error));*/
    }

    if(portal)
        iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);

    // Compose a response to send back to the client
    struct iSCSIDRspLogout rsp = iSCSIDRspLogoutInit;
    rsp.errorCode = errorCode;
    rsp.statusCode = statusCode;

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDCreateArrayOfActiveTargets(int fd,struct iSCSIDCmdCreateArrayOfActiveTargets * cmd)
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
    struct iSCSIDRspCreateArrayOfActiveTargets rsp = iSCSIDRspCreateArrayOfActiveTargetsInit;
    if(data)
        rsp.dataLength = (UInt32)CFDataGetLength(data);
    else
        rsp.dataLength = 0;

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        if(data)
            CFRelease(data);
        return EAGAIN;
    }

    if(data)
    {
        if(send(fd,CFDataGetBytePtr(data),rsp.dataLength,0) != rsp.dataLength) {
            CFRelease(data);
            return EAGAIN;
        }
        CFRelease(data);
    }
    return 0;
}

errno_t iSCSIDCreateArrayofActivePortalsForTarget(int fd,struct iSCSIDCmdCreateArrayOfActivePortalsForTarget * cmd)
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
    struct iSCSIDRspCreateArrayOfActiveTargets rsp = iSCSIDRspCreateArrayOfActiveTargetsInit;
    if(data)
        rsp.dataLength = (UInt32)CFDataGetLength(data);
    else
        rsp.dataLength = 0;

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
    {
        if(data)
            CFRelease(data);
        return EAGAIN;
    }

    if(data)
    {
        if(send(fd,CFDataGetBytePtr(data),rsp.dataLength,0) != rsp.dataLength)
        {
            CFRelease(data);
            return EAGAIN;
        }

        CFRelease(data);
    }
    return 0;
}

errno_t iSCSIDIsTargetActive(int fd,struct iSCSIDCmdIsTargetActive *cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    iSCSIDRspIsTargetActive rsp = iSCSIDRspIsTargetActiveInit;
    rsp.active = (iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target)) != kiSCSIInvalidSessionId);

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}

errno_t iSCSIDIsPortalActive(int fd,struct iSCSIDCmdIsPortalActive *cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                                         (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

    iSCSIDRspIsPortalActive rsp = iSCSIDRspIsPortalActiveInit;
    SID sessionId = (iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target)));

    if(sessionId == kiSCSIInvalidSessionId)
        rsp.active = false;
    else
        rsp.active = (iSCSIGetConnectionIdForPortal(sessionId,portal) != kiSCSIInvalidConnectionId);

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    return 0;
}


errno_t iSCSIDQueryTargetForAuthMethod(int fd,struct iSCSIDCmdQueryTargetForAuthMethod * cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                                         (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

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

errno_t iSCSIDCreateCFPropertiesForSession(int fd,struct iSCSIDCmdCreateCFPropertiesForSession * cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);

    if(!target)
        return EINVAL;

    errno_t error = 0;
    CFDictionaryRef properties = iSCSICreateCFPropertiesForSession(target);

    // Send back response
    iSCSIDRspCreateCFPropertiesForSession rsp = iSCSIDRspCreateCFPropertiesForSessionInit;

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

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        error = EAGAIN;

    // Send data if any
    if(data && !error) {
        if(send(fd,CFDataGetBytePtr(data),CFDataGetLength(data),0) != CFDataGetLength(data))
            error =  EAGAIN;
        
        CFRelease(data);
    }
    return error;
}
errno_t iSCSIDCreateCFPropertiesForConnection(int fd,struct iSCSIDCmdCreateCFPropertiesForConnection * cmd)
{
    // Grab objects from stream
    iSCSITargetRef target = iSCSIDCreateObjectFromSocket(fd,cmd->targetLength,
                                                         (void *(* )(CFDataRef))&iSCSITargetCreateWithData);
    iSCSIPortalRef portal = iSCSIDCreateObjectFromSocket(fd,cmd->portalLength,
                                                         (void *(* )(CFDataRef))&iSCSIPortalCreateWithData);

    if(!target || !portal)
        return EINVAL;

    errno_t error = 0;
    CFDictionaryRef properties = iSCSICreateCFPropertiesForConnection(target,portal);

    // Send back response
    iSCSIDRspCreateCFPropertiesForConnection rsp = iSCSIDRspCreateCFPropertiesForConnectionInit;

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

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        error = EAGAIN;

    // Send data if any
    if(data && !error) {
        if(send(fd,CFDataGetBytePtr(data),CFDataGetLength(data),0) != CFDataGetLength(data))
                error =  EAGAIN;

        CFRelease(data);
    }

    return error;
}

/*! Synchronizes the daemon with the property list. This function may be called
 *  anytime changes are made to the property list (e.g., by an external
 *  application) that require immediate action on the daemon's part. This
 *  includes for example, the initiator name and alias and discovery settings
 *  (whether discovery is enabled or disabled, and the time interval
 *  associated with discovery). */
errno_t iSCSIDUpdateDiscovery(int fd,struct iSCSIDCmdUpdateDiscovery * cmd)
{
    errno_t error = 0;

    iSCSIPLSynchronize();

    // Check whether SendTargets discovery is enabled, and get interval (sec)
    Boolean discoveryEnabled = iSCSIPLGetSendTargetsDiscoveryEnable();
    CFTimeInterval interval = iSCSIPLGetSendTargetsDiscoveryInterval();
    CFRunLoopTimerCallBack callout = &iSCSIDiscoveryRunSendTargets;

    // Remove existing timer if one exists
    if(discoveryTimer) {
        CFRunLoopRemoveTimer(CFRunLoopGetMain(),discoveryTimer,kCFRunLoopDefaultMode);
        CFRelease(discoveryTimer);
        discoveryTimer = NULL;
    }

    // Add new timer with updated interval, if discovery is enabled
    if(discoveryEnabled)
    {
        discoveryTimer = CFRunLoopTimerCreate(kCFAllocatorDefault,
                                              CFAbsoluteTimeGetCurrent() + 2.0,
                                              interval,0,0,callout,NULL);

        CFRunLoopAddTimer(CFRunLoopGetMain(),discoveryTimer,kCFRunLoopDefaultMode);
    }

    // Send back response
    iSCSIDRspUpdateDiscovery rsp = iSCSIDRspUpdateDiscoveryInit;
    iSCSIPLSynchronize();

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        error = EAGAIN;

    return error;
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

/*! Handle an incoming connection from iscsictl. Once a connection is
 *  established, main runloop calls this function. This function processes
 *  all incoming commands until a shutdown request is received, at which point
 *  this function terminates and returns control to the run loop. For this
 *  reason, no other commands (e.g., timers) can be executed until the 
 *  incoming request is processed. */
void iSCSIDProcessIncomingRequest(CFSocketRef socket,
                                  CFSocketCallBackType callbackType,
                                  CFDataRef address,
                                  const void * data,
                                  void * info)
{
    // Wait for an incoming connection; upon timeout quit
    struct sockaddr_storage peerAddress;
    socklen_t length = sizeof(peerAddress);

    // Get file descriptor associated with the connection
    int fd = accept(CFSocketGetNative(socket),(struct sockaddr *)&peerAddress,&length);

    struct iSCSIDCmd cmd;

    // Parse all commands in sequence for this iscsictl session. Once iscsictl
    // is done, it will issue a shutdown command, that will terminate this loop
    // and return execution control to the main run loop.
    while(fd != 0 && recv(fd,&cmd,sizeof(cmd),MSG_PEEK) == sizeof(cmd)) {
        recv(fd,&cmd,sizeof(cmd),MSG_WAITALL);

        errno_t error = 0;
        switch(cmd.funcCode)
        {
            case kiSCSIDLogin:
                error = iSCSIDLogin(fd,(iSCSIDCmdLogin*)&cmd); break;
            case kiSCSIDLogout:
                error = iSCSIDLogout(fd,(iSCSIDCmdLogout*)&cmd); break;
            case kiSCSIDCreateArrayOfActiveTargets:
                error = iSCSIDCreateArrayOfActiveTargets(fd,(iSCSIDCmdCreateArrayOfActiveTargets*)&cmd); break;
            case kiSCSIDCreateArrayOfActivePortalsForTarget:
                error = iSCSIDCreateArrayofActivePortalsForTarget(fd,(iSCSIDCmdCreateArrayOfActivePortalsForTarget*)&cmd); break;
            case kiSCSIDIsTargetActive:
                error = iSCSIDIsTargetActive(fd,(iSCSIDCmdIsTargetActive*)&cmd); break;
            case kiSCSIDIsPortalActive:
                error = iSCSIDIsPortalActive(fd,(iSCSIDCmdIsPortalActive*)&cmd); break;
            case kiSCSIDQueryTargetForAuthMethod:
                error = iSCSIDQueryTargetForAuthMethod(fd,(iSCSIDCmdQueryTargetForAuthMethod*)&cmd); break;
            case kiSCSIDCreateCFPropertiesForSession:
                error = iSCSIDCreateCFPropertiesForSession(fd,(iSCSIDCmdCreateCFPropertiesForSession*)&cmd); break;
            case kiSCSIDCreateCFPropertiesForConnection:
                error = iSCSIDCreateCFPropertiesForConnection(fd,(iSCSIDCmdCreateCFPropertiesForConnection*)&cmd); break;
            case kiSCSIDUpdateDiscovery:
                error = iSCSIDUpdateDiscovery(fd,(iSCSIDCmdUpdateDiscovery*)&cmd); break;
            case kiSCSIDShutdownDaemon:
            default:
                close(fd);
                fd = 0;
        };
    }
}

/*! iSCSI daemon entry point. */
int main(void)
{
    // Read configuration parameters from the iSCSI property list
    iSCSIPLSynchronize();

    // Update initiator name and alias internally
    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    if(initiatorIQN) {
        iSCSISetInitiatorName(initiatorIQN);
        CFRelease(initiatorIQN);
    }
    else {
        iSCSIDLogError(CFSTR("initiator IQN not set, reverting to internal default."));
    }

    CFStringRef initiatorAlias = iSCSIPLCopyInitiatorAlias();

    if(initiatorAlias) {
        iSCSISetInitiatorAlias(initiatorAlias);
        CFRelease(initiatorAlias);
    }
    else {
        iSCSIDLogError(CFSTR("initiator alias not set, reverting to internal default."));
    }

    // Register with launchd so it can manage this daemon
    launch_data_t reg_request = launch_data_new_string(LAUNCH_KEY_CHECKIN);

    // Quit if we are unable to checkin...
    if(!reg_request) {
        iSCSIDLogError(CFSTR("fatal error: failed to checkin with launchd."));
        goto ERROR_LAUNCH_DATA;
    }

    launch_data_t reg_response = launch_msg(reg_request);

    // Ensure registration was successful
    if((launch_data_get_type(reg_response) == LAUNCH_DATA_ERRNO)) {
        iSCSIDLogError(CFSTR("fatal error: failed to checkin with launchd."));
        goto ERROR_NO_SOCKETS;
    }

    // Grab label and socket dictionary from daemon's property list
    launch_data_t label = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_LABEL);
    launch_data_t sockets = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_SOCKETS);

    if(!label || !sockets) {
        iSCSIDLogError(CFSTR("fatal error: could not find socket definition, plist may be damaged."));
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

    iSCSIDLogError(CFSTR("daemon started."));

    // Initialize iSCSI connection to kernel (ability to call iSCSI kernel
    // functions and receive notifications from the kernel).
    iSCSIInitialize(CFRunLoopGetMain());
    CFRunLoopRun();
    iSCSICleanup();
    
    // Deregister for power
    iSCSIDDeregisterForPowerEvents();
    
    launch_data_free(reg_response);
    return 0;
    
    // TODO: verify that launch data is freed under all possible execution paths
    
ERROR_PWR_MGMT_FAIL:
ERROR_NO_SOCKETS:
    launch_data_free(reg_response);
    
ERROR_LAUNCH_DATA:
    
    return ENOTSUP;
}

