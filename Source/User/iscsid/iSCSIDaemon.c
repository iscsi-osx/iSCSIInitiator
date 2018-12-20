/*
 * Copyright (c) 2016, Nareg Sinenian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include <SystemConfiguration/SystemConfiguration.h>

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
#include "iSCSIPreferences.h"
#include "iSCSIDA.h"
#include "iSCSIAuthRights.h"
#include "iSCSIDiscovery.h"


// Used to notify daemon of power state changes
io_connect_t powerPlaneRoot;
io_object_t powerNotifier;
IONotificationPortRef powerNotifyPortRef;

// Used to fire discovery timer at specified intervals
CFRunLoopTimerRef discoveryTimer = NULL;

// Used by discovery to notify the main daemon thread that data is ready
CFRunLoopSourceRef discoverySource = NULL;

// Used to point to discovery records
CFDictionaryRef discoveryRecords = NULL;

// Mutex lock when discovery is running
pthread_mutex_t discoveryMutex = PTHREAD_MUTEX_INITIALIZER;

/*! Server-side timeouts (in milliseconds) for send()/recv(). */
static const int kiSCSIDaemonTimeoutMilliSec = 250;

/*! Dictionary used to keep track of portals and targets that
 *  were active when the system goes to sleep. */
CFMutableDictionaryRef activeTargets = NULL;

// Mutex lock used when preferences are being synchronized
pthread_mutex_t preferencesMutex = PTHREAD_MUTEX_INITIALIZER;

/*! Preferences object used to syncrhonize iSCSI preferences. */
iSCSIPreferencesRef preferences = NULL;

/*! Incoming request information struct. */
struct iSCSIDIncomingRequestInfo * reqInfo = NULL;

/*! Used to manage iSCSI sessions. */
iSCSISessionManagerRef sessionManager = NULL;

struct iSCSIDIncomingRequestInfo {
    CFSocketRef socket;
    CFRunLoopSourceRef socketSourceRead;
    int fd;
};

struct iSCSIDQueueLoginForTargetPortal {
    iSCSITargetRef target;
    iSCSIPortalRef portal;
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

const iSCSIDMsgPreferencesIOLockAndSyncRsp iSCSIDMsgPreferencesIOLockAndSyncRspInit = {
    .funcCode = kiSCSIDPreferencesIOLockAndSync,
    .errorCode = 0,
};

const iSCSIDMsgPreferencesIOUnlockAndSyncRsp iSCSIDMsgPreferencesIOUnlockAndSyncRspInit = {
    .funcCode = kiSCSIDPreferencesIOUnlockAndSync,
    .errorCode = 0,
};

const iSCSIDMsgSetSharedSecretRsp iSCSIDMsgSetSharedSecretRspInit = {
    .funcCode = kiSCSIDSetSharedSecret,
    .errorCode = 0,
};

const iSCSIDMsgRemoveSharedSecretRsp iSCSIDMsgRemoveSharedSecretRspInit = {
    .funcCode = kiSCSIDRemoveSharedSecret,
    .errorCode = 0,
};

/*! Used for the logout process. */
typedef struct iSCSIDLogoutContext {
    int fd;
    DASessionRef diskSession;
    iSCSIPortalRef portal;
    errno_t errorCode;
    
} iSCSIDLogoutContext;


/*! Helper function. Updates the preferences object using application values. */
void iSCSIDUpdatePreferencesFromAppValues()
{
    if(preferences != NULL)
        iSCSIPreferencesRelease(preferences);
    
    preferences = iSCSIPreferencesCreateFromAppValues();
}

iSCSISessionConfigRef iSCSIDCreateSessionConfig(CFStringRef targetIQN)
{
    iSCSIMutableSessionConfigRef config = iSCSISessionConfigCreateMutable();

    iSCSISessionConfigSetErrorRecoveryLevel(config,iSCSIPreferencesGetErrorRecoveryLevelForTarget(preferences,targetIQN));
    iSCSISessionConfigSetMaxConnections(config,iSCSIPreferencesGetMaxConnectionsForTarget(preferences,targetIQN));

    return config;
}

iSCSIConnectionConfigRef iSCSIDCreateConnectionConfig(CFStringRef targetIQN,
                                                      CFStringRef portalAddress)
{
    iSCSIMutableConnectionConfigRef config = iSCSIConnectionConfigCreateMutable();

    enum iSCSIDigestTypes digestType;

    digestType = iSCSIPreferencesGetDataDigestForTarget(preferences,targetIQN);

    if(digestType == kiSCSIDigestInvalid)
        digestType = kiSCSIDigestNone;

    iSCSIConnectionConfigSetDataDigest(config,digestType);

    digestType = iSCSIPreferencesGetHeaderDigestForTarget(preferences,targetIQN);

    if(digestType == kiSCSIDigestInvalid)
        digestType = kiSCSIDigestNone;

    iSCSIConnectionConfigSetHeaderDigest(config,digestType);

    return config;
}

iSCSIAuthRef iSCSIDCreateAuthenticationForTarget(CFStringRef targetIQN)
{
    iSCSIAuthRef auth;
    enum iSCSIAuthMethods authMethod = iSCSIPreferencesGetTargetAuthenticationMethod(preferences,targetIQN);

    if(authMethod == kiSCSIAuthMethodCHAP)
    {
        CFStringRef name = iSCSIPreferencesCopyTargetCHAPName(preferences,targetIQN);
        CFStringRef sharedSecret = iSCSIKeychainCopyCHAPSecretForNode(targetIQN);

        if(!name) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"target CHAP name for target has not been set, reverting to no authentication");
            auth = iSCSIAuthCreateNone();
        }
        else if(!sharedSecret) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"target CHAP secret is missing or insufficient privileges to system keychain, reverting to no authentication");
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
    enum iSCSIAuthMethods authMethod = iSCSIPreferencesGetInitiatorAuthenticationMethod(preferences);

    if(authMethod == kiSCSIAuthMethodCHAP)
    {
        CFStringRef name = iSCSIPreferencesCopyInitiatorCHAPName(preferences);
        CFStringRef initiatorIQN = iSCSIPreferencesCopyInitiatorIQN(preferences);
        CFStringRef sharedSecret = iSCSIKeychainCopyCHAPSecretForNode(initiatorIQN);
        CFRelease(initiatorIQN);

        if(!name) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"initiator CHAP name for target has not been set, reverting to no authentication.");
            auth = iSCSIAuthCreateNone();
        }
        else if(!sharedSecret) {
            asl_log(NULL,NULL,ASL_LEVEL_WARNING,"initiator CHAP secret is missing or insufficient privileges to system keychain, reverting to no authentication.");
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

errno_t iSCSIDLoginCommon(SessionIdentifier sessionId,
                          iSCSIMutableTargetRef target,
                          iSCSIPortalRef portal,
                          enum iSCSILoginStatusCode * statusCode)
{
    errno_t error = 0;
    iSCSISessionConfigRef sessCfg = NULL;
    iSCSIConnectionConfigRef connCfg = NULL;
    iSCSIAuthRef initiatorAuth = NULL, targetAuth = NULL;

    ConnectionIdentifier connectionId = kiSCSIInvalidConnectionId;

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
        error = iSCSISessionLogin(sessionManager,target,portal,initiatorAuth,targetAuth,sessCfg,connCfg,&sessionId,&connectionId,statusCode);
    else
        error = iSCSISessionAddConnection(sessionManager,sessionId,portal,initiatorAuth,targetAuth,connCfg,&connectionId,statusCode);

    // Log error message
    if(error) {
        CFStringRef errorString = CFStringCreateWithFormat(
            kCFAllocatorDefault,0,
            CFSTR("login to %@,%@:%@ failed: %s\n"),
            targetIQN,
            iSCSIPortalGetAddress(portal),
            iSCSIPortalGetPort(portal),
            strerror(error));

        CFIndex errorStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(errorString),kCFStringEncodingASCII) + sizeof('\0');
        char errorStringBuffer[errorStringLength];
        CFStringGetCString(errorString,errorStringBuffer,errorStringLength,kCFStringEncodingASCII);
        
        asl_log(NULL, NULL, ASL_LEVEL_ERR, "%s", errorStringBuffer);
        
        CFRelease(errorString);
    }
    // Update target alias in preferences (if one was furnished)
    else
    {
        iSCSIPreferencesSetTargetAlias(preferences,targetIQN,iSCSITargetGetAlias(target));
        iSCSIPreferencesSynchronzeAppValues(preferences);
    }
    
    if(sessCfg)
        iSCSISessionConfigRelease(sessCfg);
    
    if(connCfg)
        iSCSIConnectionConfigRelease(connCfg);
    
    if(targetAuth)
        iSCSIAuthRelease(targetAuth);
    
    if(initiatorAuth)
        iSCSIAuthRelease(initiatorAuth);
    
    return error;
}


errno_t iSCSIDLoginAllPortals(iSCSIMutableTargetRef target,
                              enum iSCSILoginStatusCode * statusCode)
{
    CFIndex activeConnections = 0;
    CFIndex maxConnections = 0;

    // Error code to return to daemon's client
    errno_t errorCode = 0;
    *statusCode = kiSCSILoginInvalidStatusCode;

    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    SessionIdentifier sessionId = iSCSISessionGetSessionIdForTarget(sessionManager,targetIQN);

    // Set initial values for maxConnections and activeConnections
    if(sessionId == kiSCSIInvalidSessionId)
//TODO: change to the maximum value that will be supported by this initiator
        maxConnections = 1;
    else {

        // If session exists, get the max connections and active connections
        CFDictionaryRef properties = iSCSISessionCopyCFPropertiesForTarget(sessionManager,target);

        if(properties) {
            // Get max connections from property list
            CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
            CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);
            CFRelease(properties);

            CFArrayRef connections = iSCSISessionCopyArrayOfConnectionIds(sessionManager,sessionId);

            if(connections) {
                activeConnections = CFArrayGetCount(connections);
                CFRelease(connections);
            }
        }
    }

    // Add portals to the session until we've run out of portals to add or
    // reached the maximum connection limit
    CFStringRef portalAddress = NULL;
    CFArrayRef portals = iSCSIPreferencesCreateArrayOfPortalsForTarget(preferences,targetIQN);
    CFIndex portalIdx = 0;
    CFIndex portalCount = CFArrayGetCount(portals);

    while( (activeConnections < maxConnections) && portalIdx < portalCount )
    {
        portalAddress = CFArrayGetValueAtIndex(portals,portalIdx);

        // Get portal object and login
        iSCSIPortalRef portal = iSCSIPreferencesCopyPortalForTarget(preferences,targetIQN,portalAddress);
        errorCode = iSCSIDLoginCommon(sessionId,target,portal,statusCode);
        iSCSIPortalRelease(portal);

        // Quit if there was an error communicating with the daemon
        if(errorCode)
            break;

        activeConnections++;
        portalIdx++;

        // Determine how many connections this session supports
        sessionId = iSCSISessionGetSessionIdForTarget(sessionManager,iSCSITargetGetIQN(target));

        // If this was the first connection of the session, get the number of
        // allowed maximum connections
        if(activeConnections == 1) {
            CFDictionaryRef properties = iSCSISessionCopyCFPropertiesForTarget(sessionManager,target);
            if(properties) {
                // Get max connections from property list
                CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
                CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);
                CFRelease(properties);
            }
        }
    };
    
    if(portals)
        CFRelease(portals);
    
    return errorCode;
}


errno_t iSCSIDLoginWithPortal(iSCSIMutableTargetRef target,
                              iSCSIPortalRef portal,
                              enum iSCSILoginStatusCode * statusCode)
{
    // Check for active sessions before attempting loginb
    SessionIdentifier sessionId = kiSCSIInvalidSessionId;
    ConnectionIdentifier connectionId = kiSCSIInvalidConnectionId;
    *statusCode = kiSCSILoginInvalidStatusCode;
    errno_t errorCode = 0;

    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    sessionId = iSCSISessionGetSessionIdForTarget(sessionManager,targetIQN);

    // Existing session, add a connection
    if(sessionId != kiSCSIInvalidSessionId) {

        connectionId = iSCSISessionGetConnectionIdForPortal(sessionManager,sessionId,portal);

        // If there's an active session display error otherwise login
        if(connectionId != kiSCSIInvalidConnectionId)
        {} //iSCSICtlDisplayError("The specified target has an active session over the specified portal.");
        else {
            // See if the session can support an additional connection
            CFDictionaryRef properties = iSCSISessionCopyCFPropertiesForTarget(sessionManager,target);
            if(properties) {
                // Get max connections from property list
                UInt32 maxConnections;
                CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
                CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);
                CFRelease(properties);

                CFArrayRef connections = iSCSISessionCopyArrayOfConnectionIds(sessionManager,sessionId);
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
    CFDataRef targetData = NULL, portalData = NULL, authorizationData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&authorizationData,cmd->authLength,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);
    iSCSIMutableTargetRef target = NULL;
    errno_t errorCode = 0;

    if(targetData) {
        iSCSITargetRef targetTemp = iSCSITargetCreateWithData(targetData);
        target = iSCSITargetCreateMutableCopy(targetTemp);
        iSCSITargetRelease(targetTemp);
        CFRelease(targetData);
    }

    iSCSIPortalRef portal = NULL;

    if(portalData) {
        portal = iSCSIPortalCreateWithData(portalData);
        CFRelease(portalData);
    }

    AuthorizationRef authorization = NULL;
    
    // If authorization data is valid, create authorization object
    if(authorizationData) {
        AuthorizationExternalForm authorizationExtForm;
        
        CFDataGetBytes(authorizationData,
                       CFRangeMake(0,kAuthorizationExternalFormLength),
                       (UInt8 *)&authorizationExtForm.bytes);
        
        AuthorizationCreateFromExternalForm(&authorizationExtForm,&authorization);
        CFRelease(authorizationData);
    }
    
    // If authorization object is valid, get the necessary rights
    if(authorization) {
        if(iSCSIAuthRightsAcquire(authorization,kiSCSIAuthLoginRight) != errAuthorizationSuccess)
            errorCode = EAUTH;
        
        AuthorizationFree(authorization,kAuthorizationFlagDefaults);
    }
    else
        errorCode = EINVAL;

    // If portal and target are valid, login with portal.  Otherwise login to
    // target using all defined portals.
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;

    // Synchronize property list
    iSCSIDUpdatePreferencesFromAppValues();

    if(!errorCode) {
        if(target && portal)
            errorCode = iSCSIDLoginWithPortal(target,portal,&statusCode);
        else if(target)
            errorCode = iSCSIDLoginAllPortals(target,&statusCode);
        else
            errorCode = EINVAL;
    }
    
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
    
    if(ctx->diskSession)
        CFRelease(ctx->diskSession);
    free(ctx);

    enum iSCSILogoutStatusCode statusCode = kiSCSILogoutInvalidStatusCode;
    
    if(!errorCode)
    {
        SessionIdentifier sessionId = iSCSISessionGetSessionIdForTarget(sessionManager,iSCSITargetGetIQN(target));

        // For session logout, ensure that disk unmount was successful...
        if(!portal) {
            
            if(result == kiSCSIDAOperationSuccess)
                errorCode = iSCSISessionLogout(sessionManager,sessionId,&statusCode);
            else
                errorCode = EBUSY;
        }
        else {
            ConnectionIdentifier connectionId = iSCSISessionGetConnectionIdForPortal(sessionManager,sessionId,portal);
            errorCode = iSCSISessionRemoveConnection(sessionManager,sessionId,connectionId,&statusCode);
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
        
        CFIndex errorStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(errorString),kCFStringEncodingASCII) + sizeof('\0');
        char errorStringBuffer[errorStringLength];
        CFStringGetCString(errorString,errorStringBuffer,errorStringLength,kCFStringEncodingASCII);

        asl_log(NULL, NULL, ASL_LEVEL_ERR, "%s", errorStringBuffer);
        
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
    CFDataRef targetData = NULL, portalData = NULL, authorizationData = NULL;
    iSCSIDaemonRecvMsg(fd,0,&authorizationData,cmd->authLength,&targetData,cmd->targetLength,&portalData,cmd->portalLength,NULL);
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
    
    AuthorizationRef authorization = NULL;
    
    // If authorization data is valid, create authorization object
    if(authorizationData) {
        AuthorizationExternalForm authorizationExtForm;
        
        CFDataGetBytes(authorizationData,
                       CFRangeMake(0,kAuthorizationExternalFormLength),
                       (UInt8 *)&authorizationExtForm.bytes);
        
        AuthorizationCreateFromExternalForm(&authorizationExtForm,&authorization);
        CFRelease(authorizationData);
    }
    
    // If authorization object is valid, get the necessary rights
    if(authorization) {
        if(iSCSIAuthRightsAcquire(authorization,kiSCSIAuthLoginRight) != errAuthorizationSuccess)
            errorCode = EAUTH;
        
        AuthorizationFree(authorization,kAuthorizationFlagDefaults);
    }
    else
        errorCode = EINVAL;

    // See if there exists an active session for this target
    SessionIdentifier sessionId = iSCSISessionGetSessionIdForTarget(sessionManager,iSCSITargetGetIQN(target));

    if(!errorCode && sessionId == kiSCSIInvalidSessionId)
    {
        CFStringRef errorString = CFStringCreateWithFormat(
            kCFAllocatorDefault,0,
            CFSTR("logout of %@ failed: the target has no active sessions"),
            iSCSITargetGetIQN(target));
        
        CFIndex errorStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(errorString),kCFStringEncodingASCII) + sizeof('\0');
        char errorStringBuffer[errorStringLength];
        CFStringGetCString(errorString,errorStringBuffer,errorStringLength,kCFStringEncodingASCII);
        
        asl_log(NULL, NULL, ASL_LEVEL_CRIT, "%s", errorStringBuffer);
        
        CFRelease(errorString);
        errorCode = EINVAL;
    }
    
    // See if there exists an active connection for this portal
    ConnectionIdentifier connectionId = kiSCSIInvalidConnectionId;
    CFIndex connectionCount = 0;
    
    if(!errorCode && portal) {
        connectionId = iSCSISessionGetConnectionIdForPortal(sessionManager,sessionId,portal);
        
        if(connectionId == kiSCSIInvalidConnectionId) {
        
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("logout of %@,%@:%@ failed: the portal has no active connections"),
                iSCSITargetGetIQN(target),iSCSIPortalGetAddress(portal),iSCSIPortalGetPort(portal));
        
            CFIndex errorStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(errorString),kCFStringEncodingASCII) + sizeof('\0');
            char errorStringBuffer[errorStringLength];
            CFStringGetCString(errorString,errorStringBuffer,errorStringLength,kCFStringEncodingASCII);

            asl_log(NULL, NULL, ASL_LEVEL_CRIT, "%s", errorStringBuffer);
            
            CFRelease(errorString);
            errorCode = EINVAL;
        }
        else {
            // Determine the number of connections
            CFArrayRef connectionIds = iSCSISessionCopyArrayOfConnectionIds(sessionManager,sessionId);
            connectionCount = CFArrayGetCount(connectionIds);
            CFRelease(connectionIds);
        }
    }
    
    // Unmount volumes if portal not specified (session logout)
    // or if portal is specified and is only connection...
    iSCSIDLogoutContext * context;
    context = (iSCSIDLogoutContext*)malloc(sizeof(iSCSIDLogoutContext));
    context->fd = fd;
    context->portal = NULL;
    context->errorCode = errorCode;
    context->diskSession = NULL;
    
    // Unmount and session logout
    if(!errorCode && (!portal || connectionCount == 1))
    {
        context->diskSession = DASessionCreate(kCFAllocatorDefault);

        DASessionScheduleWithRunLoop(context->diskSession,CFRunLoopGetMain(),kCFRunLoopDefaultMode);
        iSCSIDAUnmountForTarget(context->diskSession,kDADiskUnmountOptionWhole,target,&iSCSIDLogoutComplete,context);
    }
    // Portal logout only (or no logout and just a response to client if error)
    else {
        context->portal = portal;
        iSCSIDLogoutComplete(target,kiSCSIDAOperationSuccess,context);
    }

    return 0;
}

errno_t iSCSIDCreateArrayOfActiveTargets(int fd,iSCSIDMsgCreateArrayOfActiveTargetsCmd * cmd)
{
    CFArrayRef sessionIds = iSCSISessionCopyArrayOfSessionIds(sessionManager);
    CFIndex sessionCount = CFArrayGetCount(sessionIds);

    // Prepare an array to hold our targets
    CFMutableArrayRef activeTargets = CFArrayCreateMutable(kCFAllocatorDefault,
                                                           sessionCount,
                                                           &kCFTypeArrayCallBacks);

    // Get target object for each active session and add to array
    for(CFIndex idx = 0; idx < sessionCount; idx++)
    {
        iSCSITargetRef target = iSCSISessionCopyTargetForId(sessionManager,(SessionIdentifier)CFArrayGetValueAtIndex(sessionIds,idx));
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
// TODO: null check
    CFArrayRef sessionIds = iSCSISessionCopyArrayOfSessionIds(sessionManager);
    CFIndex sessionCount = CFArrayGetCount(sessionIds);

    // Prepare an array to hold our targets
    CFMutableArrayRef activeTargets = CFArrayCreateMutable(kCFAllocatorDefault,
                                                           sessionCount,
                                                           &kCFTypeArrayCallBacks);

    // Get target object for each active session and add to array
    for(CFIndex idx = 0; idx < sessionCount; idx++)
    {
        iSCSITargetRef target = iSCSISessionCopyTargetForId(sessionManager,(SessionIdentifier)CFArrayGetValueAtIndex(sessionIds,idx));
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
    
    if(sessionIds)
        CFRelease(sessionIds);

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
        rsp.active = (iSCSISessionGetSessionIdForTarget(sessionManager,iSCSITargetGetIQN(target)) != kiSCSIInvalidSessionId);
        
        iSCSITargetRelease(target);

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
    SessionIdentifier sessionId = (iSCSISessionGetSessionIdForTarget(sessionManager,iSCSITargetGetIQN(target)));

    if(sessionId == kiSCSIInvalidSessionId)
        rsp.active = false;
    else
        rsp.active = (iSCSISessionGetConnectionIdForPortal(sessionManager,sessionId,portal) != kiSCSIInvalidConnectionId);
    
    if(target)
        iSCSITargetRelease(target);
    
    if(portal)
        iSCSIPortalRelease(portal);

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

    errno_t error = iSCSIQueryTargetForAuthMethod(sessionManager,portal,iSCSITargetGetIQN(target),&authMethod,&statusCode);

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
        CFDictionaryRef properties = iSCSISessionCopyCFPropertiesForTarget(sessionManager,target);

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
    
    if(target)
        iSCSITargetRelease(target);
    
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

        CFDictionaryRef properties = iSCSISessionCopyCFPropertiesForPortal(sessionManager,target,portal);

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
    
    if(target)
        iSCSITargetRelease(target);
    
    if(portal)
        iSCSIPortalRelease(portal);

    return error;
}


errno_t iSCSIDAddTargetForSendTargets(iSCSIPreferencesRef preferences,
                                      CFStringRef targetIQN,
                                      iSCSIDiscoveryRecRef discoveryRec,
                                      CFStringRef discoveryPortal)
{
    CFArrayRef portalGroups = iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(discoveryRec,targetIQN);
    CFIndex portalGroupCount = CFArrayGetCount(portalGroups);
    
    // Iterate over portal groups for this target
    for(CFIndex portalGroupIdx = 0; portalGroupIdx < portalGroupCount; portalGroupIdx++)
    {
        CFStringRef portalGroupTag = CFArrayGetValueAtIndex(portalGroups,portalGroupIdx);
        CFArrayRef portals = iSCSIDiscoveryRecGetPortals(discoveryRec,targetIQN,portalGroupTag);
        CFIndex portalsCount = CFArrayGetCount(portals);
        
        iSCSIPortalRef portal = NULL;
        
        // Iterate over portals within this group
        for(CFIndex portalIdx = 0; portalIdx < portalsCount; portalIdx++)
        {
            if(!(portal = CFArrayGetValueAtIndex(portals,portalIdx)))
                continue;
            
            // Add portal to target, or add target as necessary
            if(iSCSIPreferencesContainsTarget(preferences,targetIQN))
                iSCSIPreferencesSetPortalForTarget(preferences,targetIQN,portal);
            else
                iSCSIPreferencesAddDynamicTargetForSendTargets(preferences,targetIQN,portal,discoveryPortal);
        }
    }
    
    CFRelease(portalGroups);
    
    return 0;
}

void * iSCSIDRunDiscovery(void * context)
{
    if(discoveryRecords != NULL)
        CFRelease(discoveryRecords);

    discoveryRecords = iSCSIDiscoveryCreateRecordsWithSendTargets(sessionManager,preferences);

    
    // Clear mutex created when discovery was launched
    pthread_mutex_unlock(&discoveryMutex);
    
    CFRunLoopSourceSignal(discoverySource);
    return NULL;
}

void iSCSIDProcessDiscoveryData(void * info)
{
    // Process discovery results if any
    if(discoveryRecords) {
        
        pthread_mutex_lock(&preferencesMutex);
        iSCSIDUpdatePreferencesFromAppValues();
        
        const CFIndex count = CFDictionaryGetCount(discoveryRecords);
        const void * keys[count];
        const void * values[count];
        CFDictionaryGetKeysAndValues(discoveryRecords,keys,values);
        
        for(CFIndex i = 0; i < count; i++)
            iSCSIDiscoveryUpdatePreferencesWithDiscoveredTargets(sessionManager,preferences,keys[i],values[i]);
        
        iSCSIPreferencesSynchronzeAppValues(preferences);
        pthread_mutex_unlock(&preferencesMutex);
        
        CFRelease(discoveryRecords);
    }
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
    iSCSIDUpdatePreferencesFromAppValues();

    // Check whether SendTargets discovery is enabled, and get interval (sec)
    Boolean discoveryEnabled = iSCSIPreferencesGetSendTargetsDiscoveryEnable(preferences);
    CFTimeInterval interval = iSCSIPreferencesGetSendTargetsDiscoveryInterval(preferences);
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
        CFTimeInterval delay = 2;
        discoveryTimer = CFRunLoopTimerCreate(kCFAllocatorDefault,
                                              CFAbsoluteTimeGetCurrent()+delay,
                                              interval,0,0,callout,NULL);

        CFRunLoopAddTimer(CFRunLoopGetCurrent(),discoveryTimer,kCFRunLoopDefaultMode);
    }

    // Send back response
    iSCSIDMsgUpdateDiscoveryRsp rsp = iSCSIDMsgUpdateDiscoveryRspInit;
    rsp.errorCode = 0;

    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        error = EAGAIN;

    return error;
}

errno_t iSCSIDPreferencesIOLockAndSync(int fd,iSCSIDMsgPreferencesIOLockAndSyncCmd * cmd)
{
    // Verify that the client is authorized for the operation
    CFDataRef authorizationData = NULL;
    errno_t error = iSCSIDaemonRecvMsg(fd,0,&authorizationData,cmd->authorizationLength,NULL);

    AuthorizationRef authorization = NULL;
    
    // If authorization data is valid, create authorization object
    if(authorizationData) {
        AuthorizationExternalForm authorizationExtForm;
        
        CFDataGetBytes(authorizationData,
                       CFRangeMake(0,kAuthorizationExternalFormLength),
                       (UInt8 *)&authorizationExtForm.bytes);
        
        AuthorizationCreateFromExternalForm(&authorizationExtForm,&authorization);
        CFRelease(authorizationData);
    }
    
    // If authorization object is valid, get the necessary rights
    if(authorization) {
        if(iSCSIAuthRightsAcquire(authorization,kiSCSIAuthModifyRight) != errAuthorizationSuccess)
            error = EAUTH;
        
        AuthorizationFree(authorization,kAuthorizationFlagDefaults);
    }
    else
        error = EINVAL;
    
    // If we have the necessary rights, lock
    if(!error) {
        pthread_mutex_lock(&preferencesMutex);
    }
    
    // Compose a response to send back to the client
    iSCSIDMsgPreferencesIOLockAndSyncRsp rsp = iSCSIDMsgPreferencesIOLockAndSyncRspInit;
    rsp.errorCode = error;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDPreferencesIOUnlockAndSync(int fd,iSCSIDMsgPreferencesIOUnlockAndSyncCmd * cmd)
{
    // Verify that the client is authorized for the operation
    CFDataRef preferencesData = NULL;
    errno_t error = iSCSIDaemonRecvMsg(fd,0,&preferencesData,cmd->preferencesLength,NULL);
    
    iSCSIPreferencesRef preferencesToSync = NULL;
    
    if(preferencesData) {
        preferencesToSync = iSCSIPreferencesCreateWithData(preferencesData);
        CFRelease(preferencesData);
    }
    
    // If no errors and the daemon was previously locked out for sync
    if(!error && preferencesToSync && pthread_mutex_trylock(&preferencesMutex))
    {
        iSCSIPreferencesSynchronzeAppValues(preferencesToSync);
        iSCSIPreferencesUpdateWithAppValues(preferences);
    }
    
    pthread_mutex_unlock(&preferencesMutex);
    
    if(preferencesToSync)
        iSCSIPreferencesRelease(preferencesToSync);
    
    // Compose a response to send back to the client
    iSCSIDMsgPreferencesIOUnlockAndSyncRsp rsp = iSCSIDMsgPreferencesIOUnlockAndSyncRspInit;
    rsp.errorCode = error;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    return 0;
}

errno_t iSCSIDSetSharedSecret(int fd,iSCSIDMsgSetSharedSecretCmd *cmd)
{
    // Verify that the client is authorized for the operation
    CFDataRef authorizationData = NULL, nodeIQNData = NULL, sharedSecretData = NULL;
    errno_t error = iSCSIDaemonRecvMsg(fd,0,&authorizationData,cmd->authorizationLength,
                                       &nodeIQNData,cmd->nodeIQNLength,
                                       &sharedSecretData,cmd->secretLength,NULL);
    
    AuthorizationRef authorization = NULL;
    CFStringRef nodeIQN = NULL;
    CFStringRef sharedSecret = NULL;
    
    if(authorizationData) {
        AuthorizationExternalForm authorizationExtForm;
        
        CFDataGetBytes(authorizationData,
                       CFRangeMake(0,kAuthorizationExternalFormLength),
                       (UInt8 *)&authorizationExtForm.bytes);
        
        AuthorizationCreateFromExternalForm(&authorizationExtForm,&authorization);
        CFRelease(authorizationData);
    }
    
    if(nodeIQNData) {
        nodeIQN = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault,nodeIQNData,kCFStringEncodingASCII);
        CFRelease(nodeIQNData);
    }
    
    if(sharedSecretData) {
        sharedSecret = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault,sharedSecretData,kCFStringEncodingASCII);
        CFRelease(sharedSecretData);
    }
    
    // If no errors then update secret
    if(!error && nodeIQN) {
        // If authorization object is valid, get the necessary rights
        if(authorization) {
            if(iSCSIAuthRightsAcquire(authorization,kiSCSIAuthModifyRight) == errSecSuccess) {
                if(iSCSIKeychainSetCHAPSecretForNode(nodeIQN,sharedSecret) == errSecSuccess)
                    error = 0;
                else
                    error = EAUTH;
            }
        }
        else
            error = EINVAL;
    }


    // Compose a response to send back to the client
    iSCSIDMsgSetSharedSecretRsp rsp = iSCSIDMsgSetSharedSecretRspInit;
    rsp.errorCode = error;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;

    if(nodeIQN)
        CFRelease(nodeIQN);
    
    if(sharedSecret)
        CFRelease(sharedSecret);

    return 0;
}

errno_t iSCSIDRemoveSharedSecret(int fd,iSCSIDMsgRemoveSharedSecretCmd *cmd)
{
    // Verify that the client is authorized for the operation
    CFDataRef authorizationData = NULL, nodeIQNData = NULL;
    errno_t error = iSCSIDaemonRecvMsg(fd,0,&authorizationData,cmd->authorizationLength,
                                       &nodeIQNData,cmd->nodeIQNLength,NULL);
    
    AuthorizationRef authorization = NULL;
    CFStringRef nodeIQN = NULL;
    
    if(authorizationData) {
        AuthorizationExternalForm authorizationExtForm;
        
        CFDataGetBytes(authorizationData,
                       CFRangeMake(0,kAuthorizationExternalFormLength),
                       (UInt8 *)&authorizationExtForm.bytes);
        
        AuthorizationCreateFromExternalForm(&authorizationExtForm,&authorization);
        CFRelease(authorizationData);
    }
    
    if(nodeIQNData) {
        nodeIQN = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault,nodeIQNData,kCFStringEncodingASCII);
        CFRelease(nodeIQNData);
    }
    
    // If no errors then remove secret
    if(!error && nodeIQN) {
        // If authorization object is valid, get the necessary rights
        if(authorization) {
            if(iSCSIAuthRightsAcquire(authorization,kiSCSIAuthModifyRight) == errSecSuccess) {
                if(iSCSIKeychainDeleteCHAPSecretForNode(nodeIQN) == errSecSuccess)
                    error = 0;
                else
                    error = EAUTH;
            }
        }
        else
            error = EINVAL;
    }
    
    // Compose a response to send back to the client
    iSCSIDMsgRemoveSharedSecretRsp rsp = iSCSIDMsgRemoveSharedSecretRspInit;
    rsp.errorCode = error;
    
    if(send(fd,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(nodeIQN)
        CFRelease(nodeIQN);
    
    return 0;
}

/*! Callback function used to process a queued login once
 *  the network becomes available. */
void iSCSIDProcessQueuedLogin(SCNetworkReachabilityRef reachabilityTarget,
                              SCNetworkReachabilityFlags flags,
                              void * info)
{
    struct iSCSIDQueueLoginForTargetPortal * loginRef = info;
    
    iSCSIMutableTargetRef target = iSCSITargetCreateMutableCopy(loginRef->target);
    iSCSIPortalRef portal = loginRef->portal;
    
    enum iSCSILoginStatusCode statusCode;
    iSCSIDLoginWithPortal(target,portal,&statusCode);
    
    iSCSITargetRelease(target);
    iSCSITargetRelease(loginRef->target);
    iSCSIPortalRelease(portal);
    
    free(loginRef);
}

/*! Helper function used by auto-login, sleep-mode and persistent
 *  functions to login to the specified target using the specified
 *  portal when the network becomes available. */
void iSCSIDQueueLogin(iSCSITargetRef target,iSCSIPortalRef portal)
{
    if(!target || !portal)
        return;
    
    iSCSITargetRetain(target);
    iSCSIPortalRetain(portal);
        
    SCNetworkReachabilityRef reachabilityTarget;
    SCNetworkReachabilityContext reachabilityContext;
    
    struct iSCSIDQueueLoginForTargetPortal * loginRef = malloc(sizeof(struct iSCSIDQueueLoginForTargetPortal));
    loginRef->target = target;
    loginRef->portal = portal;
    
    reachabilityContext.info = loginRef;
    reachabilityContext.copyDescription = 0;
    reachabilityContext.retain = 0;
    reachabilityContext.release = 0;
    
    char portalAddressBuffer[NI_MAXHOST];
    
    if(!CFStringGetCString(iSCSIPortalGetAddress(portal),portalAddressBuffer,NI_MAXHOST,kCFStringEncodingASCII))
        return;
    
    // If a specific host interface was specified, create with pair...
    if(CFStringCompare(iSCSIPortalGetHostInterface(portal),kiSCSIDefaultHostInterface,0) == kCFCompareEqualTo)
        reachabilityTarget = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault,portalAddressBuffer);
    else {

        struct sockaddr_storage remoteAddress, localAddress;
        iSCSIUtilsGetAddressForPortal(portal,&remoteAddress,&localAddress);
        
        reachabilityTarget = SCNetworkReachabilityCreateWithAddressPair(kCFAllocatorDefault,
                                                                        (const struct sockaddr *)&localAddress,
                                                                        (const struct sockaddr *)&remoteAddress);
    }
    
    // If the target is reachable just login; otherwise queue the login ...
    SCNetworkReachabilityFlags reachabilityFlags;
    SCNetworkReachabilityGetFlags(reachabilityTarget,&reachabilityFlags);
    
    if(reachabilityFlags & kSCNetworkReachabilityFlagsReachable) {
        enum iSCSILoginStatusCode statusCode;
        iSCSIMutableTargetRef mutableTarget = iSCSITargetCreateMutableCopy(target);
        iSCSITargetRelease(target);
        iSCSIDLoginWithPortal(mutableTarget,portal,&statusCode);
        
        iSCSITargetRelease(mutableTarget);
        iSCSIPortalRelease(portal);
        CFRelease(reachabilityTarget);
        free(loginRef);
    
    }
    else {
        SCNetworkReachabilitySetCallback(reachabilityTarget,iSCSIDProcessQueuedLogin,&reachabilityContext);
        SCNetworkReachabilityScheduleWithRunLoop(reachabilityTarget,CFRunLoopGetMain(),kCFRunLoopDefaultMode);
    }
}

void iSCSIDSessionTimeoutHandler(iSCSITargetRef target,iSCSIPortalRef portal)
{
    if(!target || !portal)
        return;
    
    // Post message to system log
    asl_log(NULL,NULL,ASL_LEVEL_ERR,"TCP timeout for %s over portal %s.",
            CFStringGetCStringPtr(iSCSITargetGetIQN(target),kCFStringEncodingASCII),
            CFStringGetCStringPtr(iSCSIPortalGetAddress(portal),kCFStringEncodingASCII));
    
    // If this was a persistance target, queue another login when the network is
    // available
    if(iSCSIPreferencesGetPersistenceForTarget(preferences,iSCSITargetGetIQN(target)))
        iSCSIDQueueLogin(target,portal);
}

/*! Automatically logs in to targets that were specified for auto-login.
 *  Used during startup of the daemon to log in to either static 
 *  dynamic targets for which the auto-login option is enabled. */
void iSCSIDAutoLogin()
{
    // Iterate over all targets and auto-login as required
    iSCSIDUpdatePreferencesFromAppValues();
    
    CFArrayRef targets = NULL;

    if(!(targets = iSCSIPreferencesCreateArrayOfTargets(preferences)))
       return;
    
    CFIndex targetsCount = CFArrayGetCount(targets);
    
    for(CFIndex idx = 0; idx < targetsCount; idx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(targets,idx);
        iSCSITargetRef target = NULL;
        
        if(!(target = iSCSIPreferencesCopyTarget(preferences,targetIQN)))
            continue;
        
        // See if this target requires auto-login and process it
        if(iSCSIPreferencesGetAutoLoginForTarget(preferences,targetIQN)) {
            
            CFArrayRef portals = NULL;
            
            if(!(portals = iSCSIPreferencesCreateArrayOfPortalsForTarget(preferences,targetIQN)))
                continue;
            
            CFIndex portalsCount = CFArrayGetCount(portals);
            
            // Queue a login operation for a each portal; mind the interface is one is required
            for(CFIndex portalIdx = 0; portalIdx < portalsCount; portalIdx++)
            {
                CFStringRef portalAddress = CFArrayGetValueAtIndex(portals,portalIdx);
                iSCSIPortalRef portal = iSCSIPreferencesCopyPortalForTarget(preferences,targetIQN,portalAddress);
                
                if(portal) {
                    iSCSIDQueueLogin(target,portal);
                    iSCSIPortalRelease(portal);
                }
            }
    
            CFRelease(portals);
        }
        
        iSCSITargetRelease(target);
    }
    CFRelease(targets);
}

void iSCSIDRestoreFromSystemSleep()
{
    // Do nothing if targets were not active before sleeping
    if(!activeTargets)
        return;
    
    const CFIndex count = CFDictionaryGetCount(activeTargets);
    
    const CFStringRef targets[count];
    const CFArrayRef portalArrays[count];
    
    CFDictionaryGetKeysAndValues(activeTargets,(const void**)targets,(const void**)portalArrays);
    
    for(CFIndex idx = 0; idx < count; idx++)
    {
        CFStringRef targetIQN = targets[idx];
        CFArrayRef portalArray = portalArrays[idx];
        CFIndex portalCount = CFArrayGetCount(portalArray);
        
        for(CFIndex portalIdx = 0; portalIdx < portalCount; portalIdx++)
        {
            enum iSCSILoginStatusCode statusCode;
            iSCSIPortalRef portal = CFArrayGetValueAtIndex(portalArray,portalIdx);
            
            iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
            iSCSITargetSetIQN(target,targetIQN);
            
            iSCSIDLoginWithPortal(target,portal,&statusCode);
            iSCSITargetRelease(target);
        }
    }
    
    CFRelease(activeTargets);
    activeTargets = NULL;
}

/*! Called to logout of target after volumes for the target are unmounted. */
void iSCSIDPrepareForSystemSleepComplete(iSCSITargetRef target,
                                         enum iSCSIDAOperationResult result,
                                         void * context)
{
    SessionIdentifier sessionId = (SessionIdentifier)context;
    enum iSCSILogoutStatusCode statusCode;
    iSCSISessionLogout(sessionManager,sessionId,&statusCode);
}

/*! Saves a dictionary of active targets and portals that
 *  is used to restore active sessions upon wakeup. */
void iSCSIDPrepareForSystemSleep()
{
    CFArrayRef sessionIds = iSCSISessionCopyArrayOfSessionIds(sessionManager);
    
    if(!sessionIds)
        return;
    
    CFIndex sessionCount = CFArrayGetCount(sessionIds);
    
    // Create disk arbitration session to force unmount of all volumes
    DASessionRef diskSession = DASessionCreate(kCFAllocatorDefault);
    
    // Clear stale list if one is present
    if(activeTargets) {
        CFRelease(activeTargets);
    }
    
    activeTargets = CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                              &kCFTypeDictionaryKeyCallBacks,
                                              &kCFTypeDictionaryValueCallBacks);
    
    for(CFIndex idx = 0; idx < sessionCount; idx++)
    {
        SessionIdentifier sessionId = (SessionIdentifier)CFArrayGetValueAtIndex(sessionIds,idx);
        iSCSITargetRef target = iSCSISessionCopyTargetForId(sessionManager,sessionId);
        
        if(!target)
            continue;
    
        CFStringRef targetIQN = iSCSITargetGetIQN(target);
        CFArrayRef connectionIds = iSCSISessionCopyArrayOfConnectionIds(sessionManager,sessionId);
        CFIndex connectionCount = CFArrayGetCount(connectionIds);
        
        CFMutableArrayRef portals = CFArrayCreateMutable(kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
        
        for(CFIndex connIdx = 0; connIdx < connectionCount; connIdx++)
        {
            ConnectionIdentifier connectionId = (ConnectionIdentifier)CFArrayGetValueAtIndex(connectionIds,connIdx);
            iSCSIPortalRef portal = iSCSISessionCopyPortalForConnectionId(sessionManager,sessionId,connectionId);
            CFArrayAppendValue(portals,portal);
            CFRelease(portal);
        }
        
        // Add array of active portals for target...
        CFDictionarySetValue(activeTargets,targetIQN,portals);

        DASessionScheduleWithRunLoop(diskSession,CFRunLoopGetMain(),kCFRunLoopDefaultMode);
        iSCSIDAUnmountForTarget(diskSession,kDADiskUnmountOptionWhole,target,&iSCSIDPrepareForSystemSleepComplete,(void*)sessionId);
    }
    
    CFRetain(diskSession);
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
        case kIOMessageSystemWillPowerOn:
            iSCSIDRestoreFromSystemSleep();
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
            case kiSCSIDPreferencesIOLockAndSync:
                error = iSCSIDPreferencesIOLockAndSync(fd,(iSCSIDMsgPreferencesIOLockAndSyncCmd*)&cmd); break;
            case kiSCSIDPreferencesIOUnlockAndSync:
                error = iSCSIDPreferencesIOUnlockAndSync(fd,(iSCSIDMsgPreferencesIOUnlockAndSyncCmd*)&cmd); break;
            case kiSCSIDSetSharedSecret:
                error = iSCSIDSetSharedSecret(fd,(iSCSIDMsgSetSharedSecretCmd*)&cmd); break;
            case kiSCSIDRemoveSharedSecret:
                error = iSCSIDRemoveSharedSecret(fd,(iSCSIDMsgRemoveSharedSecretCmd*)&cmd); break;
            default:
                CFSocketInvalidate(reqInfo->socket);
                reqInfo->fd = 0;
                pthread_mutex_unlock(&preferencesMutex);
        };
        
        if(error)
            asl_log(NULL,NULL,ASL_LEVEL_ERR,"error code %d while processing user request",error);
    }
    
    // If a request came in while we were processing, queue it up...
    if(reqInfo->fd && recv(fd,&cmd,sizeof(cmd),MSG_PEEK) > 0) {
            CFRunLoopSourceSignal(reqInfo->socketSourceRead);
    }
}

/*! Handle an incoming connection from a client. Once a connection is
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

void sig_pipe_handler(int signal)
{
    if(!reqInfo)
        return;

    // Cleanup since pipe was broken
    CFSocketInvalidate(reqInfo->socket);
    reqInfo->fd = 0;
    pthread_mutex_unlock(&preferencesMutex);
}

/*! iSCSI daemon entry point. */
int main(void)
{
    // Initialize logging
    aslclient log = asl_open(NULL,NULL,ASL_OPT_STDERR);
    
    // Start the iSCSI session manager (which creates a connection to the HBA)
    iSCSISessionManagerCallBacks callbacks;
    callbacks.timeoutCallback = iSCSIDSessionTimeoutHandler;
    sessionManager = iSCSISessionManagerCreate(kCFAllocatorDefault,callbacks);
    
    // Let launchd call us again once the HBA kext is loaded
    if(!sessionManager) {
        asl_log(NULL,NULL,ASL_LEVEL_ALERT,"kernel extension has not been loaded, iSCSI services unavailable");
        return EAGAIN;
    }
    iSCSISessionManagerScheduleWithRunLoop(sessionManager,CFRunLoopGetMain(),kCFRunLoopDefaultMode);

    // Read configuration parameters from the iSCSI property list
    iSCSIDUpdatePreferencesFromAppValues();
    
    // Update initiator name and alias internally
    CFStringRef initiatorIQN = iSCSIPreferencesCopyInitiatorIQN(preferences);
    
    if(initiatorIQN) {
        iSCSISessionManagerSetInitiatorName(sessionManager,initiatorIQN);
        CFRelease(initiatorIQN);
    }
    else {
        asl_log(NULL,NULL,ASL_LEVEL_WARNING,"initiator IQN not set, reverting to internal default");
    }

    CFStringRef initiatorAlias = iSCSIPreferencesCopyInitiatorAlias(preferences);

    if(initiatorAlias) {
        iSCSISessionManagerSetInitiatorName(sessionManager,initiatorAlias);
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
    reqInfo = malloc(sizeof(struct iSCSIDIncomingRequestInfo));
    
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
    
    // Runloop source signal by deamoen when discovery data is ready to be processed
    CFRunLoopSourceContext discoveryContext;
    bzero(&discoveryContext,sizeof(discoveryContext));
    discoveryContext.info = &discoveryRecords;
    discoveryContext.perform = iSCSIDProcessDiscoveryData;
    discoverySource = CFRunLoopSourceCreate(kCFAllocatorDefault,1,&discoveryContext);
    CFRunLoopAddSource(CFRunLoopGetMain(),discoverySource,kCFRunLoopDefaultMode);

    asl_log(NULL,NULL,ASL_LEVEL_INFO,"daemon started");

    // Ignore SIGPIPE (generated when the client closes the connection)
    signal(SIGPIPE,sig_pipe_handler);
    
    // Setup authorization rights if none exist
    AuthorizationRef authorization;
    AuthorizationCreate(NULL,NULL,0,&authorization);
    iSCSIAuthRightsInitialize(authorization);
    AuthorizationFree(authorization,kAuthorizationFlagDefaults);
    
    // Sync discovery parameters upon startup
    iSCSIDUpdateDiscovery(0,NULL);
    
    // Auto-login upon startup
    iSCSIDAutoLogin();
    
    CFRunLoopRun();
    
    iSCSISessionManagerUnscheduleWithRunloop(sessionManager,CFRunLoopGetMain(),kCFRunLoopDefaultMode);
    iSCSISessionManagerRelease(sessionManager);
    sessionManager = NULL;
    
    // Deregister for power
    iSCSIDDeregisterForPowerEvents();
    
    // Free all CF objects and reqInfo structure...
    free(reqInfo);
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

