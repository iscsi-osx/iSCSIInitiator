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

#include "iSCSIDaemonInterface.h"
#include "iSCSIDaemonInterfaceShared.h"

#include <sys/select.h>

/*! Timeout used when connecting to daemon. */
static const int kiSCSIDaemonConnectTimeoutMilliSec = 100;

/*! Timeout to use for normal communication with daemon. This is the time
 *  that the client will have to wait for the deamon to perform the desired
 *  operation (potentially over the network). */
static const int kiSCSIDaemonDefaultTimeoutSec = 10;


const iSCSIDMsgLoginCmd iSCSIDMsgLoginCmdInit  = {
    .funcCode = kiSCSIDLogin,
    .portalLength = 0,
    .targetLength = 0
};

const iSCSIDMsgLogoutCmd iSCSIDMsgLogoutCmdInit  = {
    .funcCode = kiSCSIDLogout,
    .targetLength = 0,
    .portalLength = 0
};

const iSCSIDMsgCreateArrayOfActiveTargetsCmd iSCSIDMsgCreateArrayOfActiveTargetsCmdInit  = {
    .funcCode = kiSCSIDCreateArrayOfActiveTargets,
};

const iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd iSCSIDMsgCreateArrayOfActivePortalsForTargetCmdInit  = {
    .funcCode = kiSCSIDCreateArrayOfActivePortalsForTarget,
};

const iSCSIDMsgIsTargetActiveCmd iSCSIDMsgIsTargetActiveCmdInit  = {
    .funcCode = kiSCSIDIsTargetActive,
    .targetLength = 0
};

const iSCSIDMsgIsPortalActiveCmd iSCSIDMsgIsPortalActiveCmdInit  = {
    .funcCode = kiSCSIDIsPortalActive,
    .portalLength = 0
};

const iSCSIDMsgQueryTargetForAuthMethodCmd iSCSIDMsgQueryTargetForAuthMethodCmdInit  = {
    .funcCode = kiSCSIDQueryTargetForAuthMethod,
};

const iSCSIDMsgCreateCFPropertiesForSessionCmd iSCSIDMsgCreateCFPropertiesForSessionCmdInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForSession,
    .targetLength = 0
};

const iSCSIDMsgCreateCFPropertiesForConnectionCmd iSCSIDMsgCreateCFPropertiesForConnectionCmdInit = {
    .funcCode = kiSCSIDCreateCFPropertiesForConnection,
    .targetLength = 0,
    .portalLength = 0
};

const iSCSIDMsgUpdateDiscoveryCmd iSCSIDMsgUpdateDiscoveryCmdInit = {
    .funcCode = kiSCSIDUpdateDiscovery
};

const iSCSIDMsgPreferencesIOLockAndSyncCmd iSCSIDMsgPreferencesIOLockAndSyncCmdInit = {
    .funcCode = kiSCSIDPreferencesIOLockAndSync
};

const iSCSIDMsgPreferencesIOUnlockAndSyncCmd iSCSIDMsgPreferencesIOUnlockAndSyncCmdInit = {
    .funcCode = kiSCSIDPreferencesIOUnlockAndSync
};

const iSCSIDMsgSetSharedSecretCmd iSCSIDMsgSetSharedSecretCmdInit = {
    .funcCode = kiSCSIDSetSharedSecret
};

const iSCSIDMsgRemoveSharedSecretCmd iSCSIDMsgRemoveSharedSecretCmdInit = {
    .funcCode = kiSCSIDRemoveSharedSecret
};

iSCSIDaemonHandle iSCSIDaemonConnect()
{
    iSCSIDaemonHandle handle = socket(PF_LOCAL,SOCK_STREAM,0);
    struct sockaddr_un address;
    address.sun_family = AF_LOCAL;
    strcpy(address.sun_path,"/var/run/iscsid");

    // Do non-blocking connect
    int flags = 0;
    fcntl(handle,F_GETFL,&flags);
    fcntl(handle,F_SETFL,flags | O_NONBLOCK);

    if(connect(handle,(const struct sockaddr *)&address,(socklen_t)SUN_LEN(&address)) == -1)
        return -1;

    // Set timeout for connect()
    struct timeval tv;
    memset(&tv,0,sizeof(tv));
    tv.tv_usec = kiSCSIDaemonConnectTimeoutMilliSec*1000;

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(handle,&fdset);

    if(select(handle,NULL,&fdset,NULL,&tv) != -1) {
        errno_t error;
        socklen_t errorSize = sizeof(errno_t);
        getsockopt(handle,SOL_SOCKET,SO_ERROR,&error,&errorSize);

        if(error) {
            close(handle);
            handle = -1;
        }
        else {
            // Restore flags prior to adding blocking
            fcntl(handle,F_SETFL,flags);

            // Set send & receive timeouts
            memset(&tv,0,sizeof(tv));
            tv.tv_sec = kiSCSIDaemonDefaultTimeoutSec;
            
            setsockopt(handle,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
            setsockopt(handle,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
        }
    }
    return handle;
}

void iSCSIDaemonDisconnect(iSCSIDaemonHandle handle)
{
    if(handle >= 0)
        close(handle);
}

/*! Logs into a target using a specific portal or all portals in the database.
 *  If an argument is supplied for portal, login occurs over the specified
 *  portal.  Otherwise, the daemon will attempt to login over all portals.
 *  @param handle a handle to a daemon connection.
 *  @param authorization an authorization for the right kiSCSIAuthModifyLogin
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use (use NULL for all portals).
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogin(iSCSIDaemonHandle handle,
                         AuthorizationRef authorization,
                         iSCSITargetRef target,
                         iSCSIPortalRef portal,
                         enum iSCSILoginStatusCode * statusCode)
{
    if(handle < 0 || !target || !authorization || !statusCode)
        return EINVAL;

    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef portalData = NULL;

    iSCSIDMsgLoginCmd cmd = iSCSIDMsgLoginCmdInit;
    cmd.authLength = kAuthorizationExternalFormLength;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = 0;

    if(portal) {
        portalData = iSCSIPortalCreateData(portal);
        cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    }
    
    AuthorizationExternalForm authExtForm;
    AuthorizationMakeExternalForm(authorization,&authExtForm);
    
    CFDataRef authData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                     (UInt8*)&authExtForm.bytes,
                                                     kAuthorizationExternalFormLength,
                                                     kCFAllocatorDefault);

    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       authData,targetData,portalData,NULL);
    
    if(portal)
        CFRelease(portalData);
    CFRelease(targetData);

    if(error)
        return error;

    iSCSIDMsgLoginRsp rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;

    if(rsp.funcCode != kiSCSIDLogin)
        return EIO;

    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;

    return rsp.errorCode;
}


/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param handle a handle to a daemon connection.
 *  @param authorization an authorization for the right kiSCSIAuthModifyLogin
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use (use NULL for all portals).
 *  @param statusCode iSCSI response code indicating operation status. */
errno_t iSCSIDaemonLogout(iSCSIDaemonHandle handle,
                          AuthorizationRef authorization,
                          iSCSITargetRef target,
                          iSCSIPortalRef portal,
                          enum iSCSILogoutStatusCode * statusCode)
{
    if(handle < 0 || !target || !authorization || !statusCode)
        return EINVAL;

    CFDataRef targetData = iSCSITargetCreateData(target);
    CFDataRef portalData = NULL;

    iSCSIDMsgLogoutCmd cmd = iSCSIDMsgLogoutCmdInit;
    cmd.authLength = kAuthorizationExternalFormLength;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = 0;

    if(portal) {
        portalData = iSCSIPortalCreateData(portal);
        cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    }
    
    AuthorizationExternalForm authExtForm;
    AuthorizationMakeExternalForm(authorization,&authExtForm);
    
    CFDataRef authData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                     (UInt8*)&authExtForm.bytes,
                                                     kAuthorizationExternalFormLength,
                                                     kCFAllocatorDefault);

    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       authData,targetData,portalData,NULL);
    
    if(portal)
        CFRelease(portalData);
    CFRelease(targetData);

    if(error)
        return error;

    iSCSIDMsgLogoutRsp rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
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

    iSCSIDMsgIsTargetActiveCmd cmd = iSCSIDMsgIsTargetActiveCmdInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);

    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       targetData,NULL);
    CFRelease(targetData);

    if(error)
        return false;

    iSCSIDMsgIsTargetActiveRsp rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
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

    iSCSIDMsgIsPortalActiveCmd cmd = iSCSIDMsgIsPortalActiveCmdInit;
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);

    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       targetData,portalData,NULL);
    CFRelease(targetData);
    CFRelease(portalData);

    if(error)
        return false;

    iSCSIDMsgIsPortalActiveRsp rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return false;

    if(rsp.funcCode != kiSCSIDIsPortalActive)
        return false;
    
    return rsp.active;
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
    iSCSIDMsgQueryTargetForAuthMethodCmd cmd = iSCSIDMsgQueryTargetForAuthMethodCmdInit;
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);
    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    
    if(iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,targetData,portalData,NULL))
    {
        CFRelease(portalData);
        CFRelease(targetData);
        return EIO;
    }

    CFRelease(portalData);
    CFRelease(targetData);
    iSCSIDMsgQueryTargetForAuthMethodRsp rsp;
    
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

    CFArrayRef activeTargets = NULL;
    errno_t error = 0;

    // Send command to daemon
    iSCSIDMsgCreateArrayOfActiveTargetsCmd cmd = iSCSIDMsgCreateArrayOfActiveTargetsCmdInit;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        error = EIO;

    iSCSIDMsgCreateArrayOfActiveTargetsRsp rsp;

    if(!error)
        error = iSCSIDaemonRecvMsg(handle,(iSCSIDMsgGeneric*)&rsp,NULL);
    
    if(!error) {
        CFDataRef data = NULL;
        error = iSCSIDaemonRecvMsg(handle,0,&data,rsp.dataLength,NULL);
        
        if(!error && data) {
            CFPropertyListFormat format;
            activeTargets = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
            CFRelease(data);
            
            if(format != kCFPropertyListBinaryFormat_v1_0) {
                if(activeTargets) {
                    CFRelease(activeTargets);
                    activeTargets = NULL;
                }
            }
        }
    }

    return activeTargets;
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

    CFArrayRef activePortals = NULL;
    errno_t error = 0;

    // Send command to daemon
    iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd cmd = iSCSIDMsgCreateArrayOfActivePortalsForTargetCmdInit;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        error = EIO;
    
    iSCSIDMsgCreateArrayOfActivePortalsForTargetRsp rsp;
    
    if(!error)
        error = iSCSIDaemonRecvMsg(handle,(iSCSIDMsgGeneric*)&rsp,NULL);

    if(!error) {
        CFDataRef data = NULL;
        error = iSCSIDaemonRecvMsg(handle,0,&data,rsp.dataLength,NULL);

        if(!error && data) {
            CFPropertyListFormat format;
            activePortals = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
            CFRelease(data);

            if(format != kCFPropertyListBinaryFormat_v1_0) {
                if(activePortals) {
                    CFRelease(activePortals);
                    activePortals = NULL;
                }
            }
        }
    }
    return activePortals;
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
    iSCSIDMsgCreateCFPropertiesForSessionCmd cmd = iSCSIDMsgCreateCFPropertiesForSessionCmdInit;

    cmd.targetLength = (UInt32)CFDataGetLength(targetData);

    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       targetData,NULL);
    CFRelease(targetData);

    iSCSIDMsgCreateCFPropertiesForSessionRsp rsp;

    if(!error)
        error = iSCSIDaemonRecvMsg(handle,(iSCSIDMsgGeneric*)&rsp,NULL);
    
    if(!error) {
        CFDataRef data = NULL;
        error = iSCSIDaemonRecvMsg(handle,0,&data,rsp.dataLength,NULL);
        
        if(!error && data) {
            CFPropertyListFormat format;
            properties = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
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
    iSCSIDMsgCreateCFPropertiesForConnectionCmd cmd = iSCSIDMsgCreateCFPropertiesForConnectionCmdInit;

    cmd.targetLength = (UInt32)CFDataGetLength(targetData);
    cmd.portalLength = (UInt32)CFDataGetLength(portalData);

    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       targetData,portalData,NULL);
    CFRelease(targetData);
    CFRelease(portalData);

    iSCSIDMsgCreateCFPropertiesForConnectionRsp rsp;
    
    if(!error)
        error = iSCSIDaemonRecvMsg(handle,(iSCSIDMsgGeneric*)&rsp,NULL);
    
    if(!error) {
        CFDataRef data = NULL;
        error = iSCSIDaemonRecvMsg(handle,0,&data,rsp.dataLength,NULL);
        
        if(!error && data) {
            CFPropertyListFormat format;
            properties = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
            CFRelease(data);
        }
    }
    return properties;
}

/*! Forces daemon to update discovery parameters from property list.
 *  @param handle a handle to a daemon connection.
 *  @return an error code indicating whether the operationg was successful. */
errno_t iSCSIDaemonUpdateDiscovery(iSCSIDaemonHandle handle)
{
    // Validate inputs
    if(handle < 0)
        return EINVAL;

    // Send command to daemon
    iSCSIDMsgUpdateDiscoveryCmd cmd = iSCSIDMsgUpdateDiscoveryCmdInit;

    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EIO;

    iSCSIDMsgUpdateDiscoveryRsp rsp;

    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;

    return 0;
}

/*! Semaphore that allows a client exclusive accesss to the property list
 *  that contains iSCSI configuraiton parameters and targets. Forces the provided
 *  preferences object to synchronize with property list on the disk.
 *  @param handle a handle to a daemon connection.
 *  @param authorization an authorization for the right kiSCSIAuthModifyRights
 *  @param preferences the preferences to be synchronized
 *  @return an error code indicating whether the operating was successful. */
errno_t iSCSIDaemonPreferencesIOLockAndSync(iSCSIDaemonHandle handle,
                                            AuthorizationRef authorization,
                                            iSCSIPreferencesRef preferences)
{
    // Validate inputs
    if(handle < 0 || !authorization || !preferences)
        return EINVAL;

    //  Send in authorization and acquire lock
    AuthorizationExternalForm authExtForm;
    AuthorizationMakeExternalForm(authorization,&authExtForm);
    
    CFDataRef authData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                     (UInt8*)authExtForm.bytes,
                                                     kAuthorizationExternalFormLength,
                                                     kCFAllocatorDefault);
    
    iSCSIDMsgPreferencesIOLockAndSyncCmd cmd = iSCSIDMsgPreferencesIOLockAndSyncCmdInit;
    cmd.authorizationLength = (UInt32)CFDataGetLength(authData);
    
    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,authData,NULL);
    
    if(error)
        return error;
    
    iSCSIDMsgPreferencesIOLockAndSyncRsp rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    if(rsp.funcCode != kiSCSIDPreferencesIOLockAndSync)
        return EIO;
    
    if(rsp.errorCode == 0) {
        // Force preferences to synchronize after obtaining lock
        // (this ensures that the client has the most up-to-date preferences data)
        iSCSIPreferencesUpdateWithAppValues(preferences);
    }
    
    return rsp.errorCode;
}

/*! Synchronizes cached preference changes to disk and releases the locked
 *  semaphore, allowing other clients to make changes. If the prefereneces
 *  parameter is NULL, then no changes are made to disk and the semaphore is
 *  unlocked.
 *  @param handle a handle to a daemon connection.
 *  @param preferences the preferences to be synchronized
 *  @return an error code indicating whether the operating was successful. */
errno_t iSCSIDaemonPreferencesIOUnlockAndSync(iSCSIDaemonHandle handle,
                                              iSCSIPreferencesRef preferences)
{
    // Validate inputs
    if(handle < 0)
        return EINVAL;
    
    CFDataRef preferencesData = NULL;
    
    iSCSIDMsgPreferencesIOUnlockAndSyncCmd cmd = iSCSIDMsgPreferencesIOUnlockAndSyncCmdInit;
    
    if(preferences) {
        preferencesData = iSCSIPreferencesCreateData(preferences);
        cmd.preferencesLength = (UInt32)CFDataGetLength(preferencesData);
    }
    else
        cmd.preferencesLength = 0;
    
    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,preferencesData,NULL);
    
    if(preferencesData)
        CFRelease(preferencesData);
    
    if(error)
        return error;
    
    iSCSIDMsgPreferencesIOUnlockAndSyncRsp rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    if(rsp.funcCode != kiSCSIDPreferencesIOUnlockAndSync)
        return EIO;
    
    return rsp.errorCode;
}

/*! Sets or updates a shared secret.
 *  @param handle a handle to a daemon connection.
 *  @param authorization an authorization for the right kiSCSIAuthModifyRights.
 *  @param nodeIQN the node iSCSI qualified name.
 *  @param sharedSecret the secret to set.
 *  @return an error code indicating whether the operating was successful. */
errno_t iSCSIDaemonSetSharedSecret(iSCSIDaemonHandle handle,
                                   AuthorizationRef authorization,
                                   CFStringRef nodeIQN,
                                   CFStringRef sharedSecret)
{
    // Validate inputs
    if(handle < 0 || !authorization || !nodeIQN || !sharedSecret)
        return EINVAL;
    
    AuthorizationExternalForm authExtForm;
    AuthorizationMakeExternalForm(authorization,&authExtForm);
    
    CFDataRef authData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                     (UInt8*)&authExtForm.bytes,
                                                     kAuthorizationExternalFormLength,
                                                     kCFAllocatorDefault);
    
    CFDataRef nodeIQNData = CFStringCreateExternalRepresentation(kCFAllocatorDefault,nodeIQN,kCFStringEncodingASCII,0);
    CFDataRef sharedSecretData = CFStringCreateExternalRepresentation(kCFAllocatorDefault,sharedSecret,kCFStringEncodingASCII,0);
    
    iSCSIDMsgSetSharedSecretCmd cmd = iSCSIDMsgSetSharedSecretCmdInit;
    cmd.authorizationLength = (UInt32)CFDataGetLength(authData);
    cmd.nodeIQNLength = (UInt32)CFDataGetLength(nodeIQNData);
    cmd.secretLength = (UInt32)CFDataGetLength(sharedSecretData);
    
    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       authData,nodeIQNData,sharedSecretData,NULL);
    
    if(nodeIQNData)
        CFRelease(nodeIQNData);
    
    if(sharedSecretData)
        CFRelease(sharedSecretData);

    if(error)
        return error;
    
    iSCSIDMsgSetSharedSecretRsp rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    if(rsp.funcCode != kiSCSIDSetSharedSecret)
        return EIO;

    return rsp.errorCode;
}

/*! Sets or updates a shared secret.
 *  @param handle a handle to a daemon connection.
 *  @param authorization an authorization for the right kiSCSIAuthModifyRights.
 *  @param nodeIQN the node iSCSI qualified name.
 *  @return an error code indicating whether the operating was successful. */
errno_t iSCSIDaemonRemoveSharedSecret(iSCSIDaemonHandle handle,
                                      AuthorizationRef authorization,
                                      CFStringRef nodeIQN)
{
    // Validate inputs
    if(handle < 0 || !authorization || !nodeIQN)
        return EINVAL;
    
    AuthorizationExternalForm authExtForm;
    AuthorizationMakeExternalForm(authorization,&authExtForm);
    
    CFDataRef authData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                     (UInt8*)&authExtForm.bytes,
                                                     kAuthorizationExternalFormLength,
                                                     kCFAllocatorDefault);
    
    CFDataRef nodeIQNData = CFStringCreateExternalRepresentation(kCFAllocatorDefault,nodeIQN,kCFStringEncodingASCII,'0');
    
    iSCSIDMsgRemoveSharedSecretCmd cmd = iSCSIDMsgRemoveSharedSecretCmdInit;
    cmd.authorizationLength = (UInt32)CFDataGetLength(authData);
    cmd.nodeIQNLength = (UInt32)CFDataGetLength(nodeIQNData);
    
    errno_t error = iSCSIDaemonSendMsg(handle,(iSCSIDMsgGeneric *)&cmd,
                                       authData,nodeIQNData,NULL);
    
    if(nodeIQNData)
        CFRelease(nodeIQNData);
    
    if(error)
        return error;
    
    iSCSIDMsgRemoveSharedSecretRsp rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EIO;
    
    if(rsp.funcCode != kiSCSIDRemoveSharedSecret)
        return EIO;
    
    
    return rsp.errorCode;
}


