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

#ifndef __ISCSI_TYPES_H__
#define __ISCSI_TYPES_H__

#include <CoreFoundation/CoreFoundation.h>
#include "iSCSITypesShared.h"
#include "iSCSIRFC3720Defaults.h"

/*! The host interface name to use when the default interface is to be used. */
static CFStringRef kiSCSIDefaultHostInterface = CFSTR("default");

/*! The default port to use when one has not been specified. */
static CFStringRef kiSCSIDefaultPort = CFSTR("3260");

/*! The value for the target IQN in an iSCSITarget when the name has not
 *  been specified. */
static CFStringRef kiSCSIUnspecifiedTargetIQN = CFSTR("");

/*! The value for the target alias in an iSCSITarget when the alias has not
 *  been specified. */
static CFStringRef kiSCSIUnspecifiedTargetAlias = CFSTR("");


typedef CFMutableDictionaryRef iSCSIMutablePortalRef;
typedef CFDictionaryRef iSCSIPortalRef;

typedef CFDictionaryRef iSCSITargetRef;
typedef CFMutableDictionaryRef iSCSIMutableTargetRef;

/*! A composite data structure comprising intitiator and target authentication
 *  objects. Used by the iSCSI layer to establish sessions. */
typedef CFDictionaryRef iSCSIAuthRef;

/*! Target authentication object. */
typedef CFDictionaryRef iSCSITargetAuthRef;
typedef CFMutableDictionaryRef iSCSIMutableTargetAuthRef;

/*! Initiator authentication object. */
typedef CFDictionaryRef iSCSIInitiatorAuthRef;
typedef CFMutableDictionaryRef iSCSIMutableInitiatorAuthRef;

/*! Basis for initiator and target authentication objects. */
typedef CFDictionaryRef iSCSICommonAuthRef;
typedef CFMutableDictionaryRef iSCSIMutableCommonAuthRef;


typedef CFDictionaryRef iSCSIDiscoveryRecRef;
typedef CFMutableDictionaryRef iSCSIMutableDiscoveryRecRef;

typedef CFDictionaryRef iSCSISessionConfigRef;
typedef CFMutableDictionaryRef iSCSIMutableSessionConfigRef;

typedef CFDictionaryRef iSCSIConnectionConfigRef;
typedef CFMutableDictionaryRef iSCSIMutableConnectionConfigRef;

#define kiSCSITypeArrayCallbacks kCFTypeArrayCallBacks
#define kiSCSITypeDictionaryKeyCallbacks kCFTypeDictionaryKeyCallBacks
#define kiSCSITypeDictionaryValueCallbacks kCFTypeDictionaryValueCallBacks

/*! Error recovery levels. */
enum iSCSIErrorRecoveryLevels {
    
    /*! Recovery of a session. */
    kiSCSIErrorRecoverySession = 0,
    
    /*! Recovery of a digest. */
    kiSCSIErrorRecoveryDigest = 1,
    
    /*! Recovery of a connection. */
    kiSCSIErrorRecoveryConnection = 2,
    
    /*! Invalid error recovery level. */
    kiSCSIErrorRecoveryInvalid
};

/*! Valid iSCSI authentication methods. */
enum iSCSIAuthMethods {
    
    /*! No authentication. */
    kiSCSIAuthMethodNone = 0,
    
    /*! CHAP authentication. */
    kiSCSIAuthMethodCHAP = 1, 
    
    /*! Invalid authentication method. */
    kiSCSIAuthMethodInvalid
};

/*! Digest type supported by iSCSI. */
enum iSCSIDigestTypes {

    /*! No digest. */
    kiSCSIDigestNone = 0,

    /*! CRC32C digest. */
    kiSCSIDigestCRC32C = 1,

    /*! Invalid digest. */
    kiSCSIDigestInvalid = 2
};

/*! Detailed login response from a target. */
enum iSCSILoginStatusCode {
    
    /*! Login was successful. */
    kiSCSILoginSuccess = 0x0000,
    
    /*! The target has been temporarily moved. */
    kiSCSILoginTargetMovedTemp = 0x0101,
    
    /*! The target has been permanently moved. */
    kiSCSILoginTargetMovedPerm = 0x0102,
    
    /*! An initiator error has occured. */
    kiSCSILoginInitiatorError = 0x0200,
    
    /*! Authentication has failed. */
    kiSCSILoginAuthFail = 0x0201,
    
    /*! Access was denied. */
    kiSCSILoginAccessDenied = 0x0202,
    
    /*! THe target was not found. */
    kiSCSILoginNotFound = 0x0203,
    
    /*! The target has been removed. */
    kiSCSILoginTargetRemoved = 0x0204,
    
    /*! Unsupported iSCSI protocol version. */
    kiSCSILoginUnsupportedVer = 0x0205,
    
    /*! Too many connections. */
    kiSCSILoginTooManyConnections = 0x0206,
    
    /*! Missing login parameters. */
    kiSCSILoginMissingParam = 0x0207,
    
    /*! Cannot include connection in this sesssion. */
    kiSCSILoginCantIncludeInSeession = 0x0208,
    
    /*! The requested session type is unsupported. */
    kiSCSILoginSessionTypeUnsupported = 0x0209,
    
    /*! The requested session does not exist. */
    kiSCSILoginSessionDoesntExist = 0x020a,
    
    /*! Invalid request during login. */
    kiSCSILoginInvalidReqDuringLogin = 0x020b,
    
    /*! A target hardware or software error has occurred. */
    kiSCSILoginTargetHWorSWError = 0x0300,
    
    /*! Login service is unavailable. */
    kiSCSILoginServiceUnavailable = 0x0301,
    
    /*! Out of resources. */
    kiSCSILoginOutOfResources = 0x0302,
    
    /*! An invalid login status code. */
    kiSCSILoginInvalidStatusCode
};

/*! Detailed logout response from a target. */
enum iSCSILogoutStatusCode {
    
    /*! Login was successful. */
    kiSCSILogoutSuccess = 0x0000,
    
    /*! The connection identifier was not found. */
    kiSCSILogoutCIDNotFound = 0x0001,
    
    /*! Recovery is not supported for this session. */
    kiSCSILogoutRecoveryNotSupported = 0x0002,
    
    /*! Cleanup of the connection resources failed. */
    kiSCSILogoutCleanupFailed = 0x0003,
    
    /*! Invalid status code. */
    kiSCSILogoutInvalidStatusCode
};

/*! Target configuration types. */
enum iSCSITargetConfigTypes {

    /*! Static target configuration. */
    kiSCSITargetConfigStatic = 0,

    /*! SendTargets dynamic target. */
    kiSCSITargetConfigDynamicSendTargets = 1,

    /*! Invalid target configuration type. */
    kiSCSITargetConfigInvalid = 2
};


/*! Creates a new portal object from an external data representation.
 *  @param data data sued to construct a portal object.
 *  @return an iSCSI portal object or NULL if object creation failed. */
iSCSIPortalRef iSCSIPortalCreateWithData(CFDataRef data);

/*! Creates a new iSCSIPortal object with empty portal parameters.
 *  @return a new portal object. */
iSCSIMutablePortalRef iSCSIPortalCreateMutable();

/*! Gets the address associated with the iSCSI portal.
 *  @param an iSCSI portal object.
 *  @return the IP or DNS of the portal or NULL if the portal is invalid. */
CFStringRef iSCSIPortalGetAddress(iSCSIPortalRef portal);

/*! Sets the address associated with the iSCSI portal.  This function has
 *  no effect if the address is blank.
 *  @param portal an iSCSI portal object.
 *  @param address the address to associated with the specified portal. */
void iSCSIPortalSetAddress(iSCSIMutablePortalRef portal,CFStringRef address);

/*! Gets the port associated with the iSCSI portal.
 *  @param portal an iSCSI portal object.
 *  @return the port associated with the portal or NULL if the portal is invalid. */
CFStringRef iSCSIPortalGetPort(iSCSIPortalRef portal);

/*! Sets the port associated with the iSCSI portal.  This function has no 
 *  effect if the portal name is blank.
 *  @param portal an iSCSI portal object.
 *  @param port the port to associate with the specified portal. */
void iSCSIPortalSetPort(iSCSIMutablePortalRef portal,CFStringRef port);

/*! Gets the interface associated with the iSCSI portal.
 *  @param portal an iSCSI portal object.
 *  @return the host interface associated with the specified portal or NULL
 *  if the portal is invalid. */
CFStringRef iSCSIPortalGetHostInterface(iSCSIPortalRef portal);

/*! Sets the interface associated with the iSCSI portal.
 *  @param portal an iSCSI portal object.
 *  @param hostInterface the host interface to associate with the specified
 *  portal. */
void iSCSIPortalSetHostInterface(iSCSIMutablePortalRef portal,
                                 CFStringRef hostInterface);

/*! Releases memory associated with an iSCSI portal object.
 *  @param portal an iSCSI portal object. */
void iSCSIPortalRelease(iSCSITargetRef portal);

/*! Retains an iSCSI portal object.
 *  @param portal an iSCSI portal object. */
void iSCSIPortalRetain(iSCSITargetRef portal);

/*! Creates a new portal object from a dictionary representation.
 * @param dict a dictionary used to construct an iSCSI portal object.
 * @return an iSCSI portal object or NULL if object creation failed. */
iSCSIPortalRef iSCSIPortalCreateWithDictionary(CFDictionaryRef dict);

/*! Copies a target object to a dictionary representation.
 *  @param portal an iSCSI portal object.
 *  @return a dictionary representation of the portal or
 *  NULL if portal is invalid. */
CFDictionaryRef iSCSIPortalCreateDictionary(iSCSIPortalRef portal);

/*! Copies the portal object to a byte array representation.
 *  @param portal an iSCSI poratl object.
 *  @return data representing the portal or NULL if the portal is invalid. */
CFDataRef iSCSIPortalCreateData(iSCSIPortalRef portal);

/*! Creates a new target object from an external data representation.
 *  @param data data used to construct an iSCSI target object.
 *  @return an iSCSI target object or NULL if object creation failed */
iSCSITargetRef iSCSITargetCreateWithData(CFDataRef data);

/*! Creates a new iSCSITarget object with empty target parameters.
 *  @return an iSCSI target object. */
iSCSIMutableTargetRef iSCSITargetCreateMutable();

/*! Createa a new mutable iSCSITarget object.
 *  @param target an exsiting target object.
 *  @param a mutable target object. */
iSCSIMutableTargetRef iSCSITargetCreateMutableCopy(iSCSITargetRef target);

/*! Gets the name associated with the iSCSI target.
  * @param target the iSCSI target object.
 *  @return the target name or kiSCSITargetUnspecified if one was not set.  */
CFStringRef iSCSITargetGetIQN(iSCSITargetRef target);

/*! Sets the name associated with the iSCSI target.  This function has no
 *  effect if the specified target name is blank.
 *  @param target the target object.
 *  @param IQN the name to set. */
void iSCSITargetSetIQN(iSCSIMutableTargetRef target,CFStringRef IQN);

/*! Gets the alias associated with the iSCSI target.
 *  @param target the target object.
 *  @return the alias associated with the target. */
CFStringRef iSCSITargetGetAlias(iSCSIMutableTargetRef target);

/*! Sets the alias associated with the iSCSI target.
 *  @param target the target object.
 *  @param alias the alias associated with the target. */
void iSCSITargetSetAlias(iSCSIMutableTargetRef target,CFStringRef alias);

/*! Releases memory associated with an iSCSI target object.
 * @param target the iSCSI target object. */
void iSCSITargetRelease(iSCSITargetRef target);

/*! Retains an iSCSI target object.
  * @param target the iSCSI target object. */
void iSCSITargetRetain(iSCSITargetRef target);

/*! Creates a new target object from a dictionary representation.
 * @param dict a dictionary used to construct an iSCSI target object.
 * @return an iSCSI target object or NULL if object creation failed. */
iSCSITargetRef iSCSITargetCreateWithDictionary(CFDictionaryRef dict);

/*! Copies a target object to a dictionary representation.
 *  @param target an iSCSI target object.
 *  @return a dictionary representation of the target or 
 *  NULL if target is invalid. */
CFDictionaryRef iSCSITargetCreateDictionary(iSCSITargetRef target);

/*! Copies the target object to a byte array representation.
 *  @param target an iSCSI target object.
 *  @return data representing the target or NULL if the target is invalid. */
CFDataRef iSCSITargetCreateData(iSCSITargetRef target);



/*! Creates a new iSCSIAuth object with empty authentication parameters
 *  (defaults to no authentication).
 *  @return a new iSCSI authentication object. */
iSCSIAuthRef iSCSIAuthCreateNone();

/*! Creates a new authentication object from an external data representation.
 * @param data data used to construct an iSCSI authentication object.
 * @return an iSCSI authentication object or NULL if object creation failed */
iSCSIAuthRef iSCSIAuthCreateWithData(CFDataRef data);

/*! Creates a new iSCSIAuth object for CHAP authentication. This function will 
 *  fail to return an authentication object if both parameters are not specified.
 *  @param name the name for CHAP.
 *  @param sharedSecret the shared CHAP secret.
 *  @return an iSCSI authentication object, or NULL if the parameters were
 *  invalid. */
iSCSIAuthRef iSCSIAuthCreateCHAP(CFStringRef name,CFStringRef sharedSecret);

/*! Returns the CHAP authentication parameter values if the authentication
 *  method is actually CHAP.
 *  @param auth an iSCSI authentication object.
 *  @param name the name for CHAP.
 *  @param sharedSecret the shared CHAP secret. */
void iSCSIAuthGetCHAPValues(iSCSIAuthRef auth,
                            CFStringRef * name,
                            CFStringRef * sharedSecret);

/*! Gets the authentication method used.
 *  @param auth an iSCSI authentication object.
 *  @return the authentication method used. */
enum iSCSIAuthMethods iSCSIAuthGetMethod(iSCSIAuthRef auth);

/*! Releases memory associated with an iSCSI auth object.
 *  @param auth an iSCSI authentication object. */
void iSCSIAuthRelease(iSCSIAuthRef auth);

/*! Retains an iSCSI auth object.
 *  @param auth an iSCSI authentication object. */
void iSCSIAuthRetain(iSCSIAuthRef auth);

/*! Creates a new authentication object from a dictionary representation. */
iSCSIAuthRef iSCSIAuthCreateWithDictionary(CFDictionaryRef dict);

/*! Copies an authentication object to a dictionary representation. */
CFDictionaryRef iSCSIAuthCreateDictionary(iSCSIAuthRef auth);

/*! Copies the authentication object to a byte array representation. */
CFDataRef iSCSIAuthCreateData(iSCSIAuthRef auth);


/*! Creates a discovery record object. */
iSCSIMutableDiscoveryRecRef iSCSIDiscoveryRecCreateMutable();

/*! Creates a discovery record from an external data representation.
 * @param data data used to construct an iSCSI discovery object.
 * @return an iSCSI discovery object or NULL if object creation failed */
iSCSIMutableDiscoveryRecRef iSCSIDiscoveryRecCreateMutableWithData(CFDataRef data);

/*! Creates a new discovery record object from a dictionary representation.
 * @return an iSCSI discovery object or NULL if object creation failed. */
iSCSIDiscoveryRecRef iSCSIDiscoveryRecCreateWithDictionary(CFDictionaryRef dict);

/*! Add a portal to a specified portal group tag for a given target.  If the 
 *  target does not exist, it is added to the discovery record.
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target to add.
 *  @param portalGroupTag the target portal group tag to add.
 *  @param portal the iSCSI portal to add. */
void iSCSIDiscoveryRecAddPortal(iSCSIMutableDiscoveryRecRef discoveryRec,
                                CFStringRef targetIQN,
                                CFStringRef portalGroupTag,
                                iSCSIPortalRef portal);

/*! Add a target to the discovery record (without any portals).
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target to add. */
void iSCSIDiscoveryRecAddTarget(iSCSIMutableDiscoveryRecRef discoveryRec,
                                CFStringRef targetIQN);

/*! Creates a CFArray object containing CFString objects with names of
 *  all of the targets in the discovery record.
 *  @param discoveryRec the discovery record.
 *  @return an array of strings with names of the targets in the record. */
CFArrayRef iSCSIDiscoveryRecCreateArrayOfTargets(iSCSIDiscoveryRecRef discoveryRec);


/*! Creates a CFArray object containing CFString objects with portal group
 *  tags for a particular target.
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target.
 *  @return an array of strings with portal group tags for the specified target. */
CFArrayRef iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(iSCSIDiscoveryRecRef discoveryRec,
                                                         CFStringRef targetIQN);

/*! Gets all of the portals associated with a partiular target and portal
 *  group tag.  
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target.
 *  @param portalGroupTag the portal group tag associated with the target.
 *  @return an array of iSCSIPortal objects associated with the specified
 *  group tag for the specified target. */
CFArrayRef iSCSIDiscoveryRecGetPortals(iSCSIDiscoveryRecRef discoveryRec,
                                       CFStringRef targetIQN,
                                       CFStringRef portalGroupTag);

/*! Releases memory associated with an iSCSI discovery record object.
 * @param target the iSCSI discovery record object. */
void iSCSIDiscoveryRecRelease(iSCSIDiscoveryRecRef discoveryRec);

/*! Retains an iSCSI discovery record object.
 * @param target the iSCSI discovery record object. */
void iSCSIDiscoveryRecRetain(iSCSIMutableDiscoveryRecRef discoveryRec);

/*! Copies an iSCSI discovery record object to a dictionary representation.
 *  @param auth an iSCSI discovery record object.
 *  @return a dictionary representation of the discovery record object or
 *  NULL if the discovery record object is invalid. */
CFDictionaryRef iSCSIDiscoveryRecCreateDictionary(iSCSIDiscoveryRecRef discoveryRec);

/*! Copies the discovery record object to a byte array representation.
 *  @param auth an iSCSI discovery record object.
 *  @return data representing the discovery record object
 *  or NULL if the discovery record object is invalid. */
CFDataRef iSCSIDiscoveryRecCreateData(iSCSIMutableDiscoveryRecRef discoveryRec);




/*! Creates a new session config object from an external data representation.
 *  @param data data used to construct an iSCSI session config object.
 *  @return an iSCSI session configuration object
 *  or NULL if object creation failed */
iSCSISessionConfigRef iSCSISessionConfigCreateWithData(CFDataRef data);

/*! Creates a new iSCSISessionConfigRef with default values. */
iSCSIMutableSessionConfigRef iSCSISessionConfigCreateMutable();

/*! Creates a mutable session configuration object from an existing one. */
iSCSIMutableSessionConfigRef iSCSISessionConfigCreateMutableCopy(iSCSISessionConfigRef config);

/*! Gets the error recovery level associated with a  session. */
enum iSCSIErrorRecoveryLevels iSCSISessionConfigGetErrorRecoveryLevel(iSCSISessionConfigRef config);

/*! Sets the desired recovery level associated with a session. */
void iSCSISessionConfigSetErrorRecoveryLevel(iSCSIMutableSessionConfigRef config,
                                             enum iSCSIErrorRecoveryLevels errorRecoveryLevel);

/*! Gets the target portal group tag for the session. */
TPGT iSCSISessionConfigGetTargetPortalGroupTag(iSCSISessionConfigRef config);

/*! Sets the target portal group tag for the session. */
void iSCSISessionConfigSetTargetPortalGroupTag(iSCSIMutableSessionConfigRef config,
                                               TPGT targetPortalGroupTag);

/*! Gets the maximum number of connections. */
UInt32 iSCSISessionConfigGetMaxConnections(iSCSISessionConfigRef config);

/*! Sets the maximum number of connections. */
void iSCSISessionConfigSetMaxConnections(iSCSIMutableSessionConfigRef config,
                                         UInt32 maxConnections);

/*! Releases memory associated with an iSCSI session configuration object.
 *  @param config an iSCSI session configuration object. */
void iSCSISessionConfigRelease(iSCSISessionConfigRef config);

/*! Retains memory associated with an iSCSI session configuration object.
 *  @param config an iSCSI session configuration object. */
void iSCSISessionConfigRetain(iSCSISessionConfigRef config);

/*! Creates a new configuration object object from a dictionary representation.
 *  @return an iSCSI session configuration object or
 *  NULL if object creation failed. */
iSCSISessionConfigRef iSCSISessionConfigCreateWithDictionary(CFDictionaryRef dict);

/*! Copies an configuration object to a dictionary representation.
 *  @param config an iSCSI configuration object.
 *  @return a dictionary representation of the configuration object or
 *  NULL if configuration object is invalid. */
CFDictionaryRef iSCSISessionConfigCreateDictionary(iSCSISessionConfigRef config);

/*! Copies the configuration object to a byte array representation.
 *  @param config an iSCSI configuration object.
 *  @return data representing the configuration object
 *  or NULL if the configuration object is invalid. */
CFDataRef iSCSISessionConfigCreateData(iSCSISessionConfigRef config);



/*! Creates a new connection config object from an external data representation.
 *  @param data data used to construct an iSCSI connection config object.
 *  @return an iSCSI connection configuration object
 *  or NULL if object creation failed */
iSCSIConnectionConfigRef iSCSIConnectionConfigCreateWithData(CFDataRef data);

/*! Creates a new iSCSIConnectionConfigRef with default values. */
iSCSIMutableConnectionConfigRef iSCSIConnectionConfigCreateMutable();

/*! Creates a mutable connection configuration object from an existing one. */
iSCSIMutableConnectionConfigRef iSCSIConnectionConfigCreateMutableCopy(iSCSIConnectionConfigRef config);

/*! Gets whether a header digest is enabled in the config object.
 *  @param config the iSCSI config object.
 *  @return the type of digest to use. */
enum iSCSIDigestTypes iSCSIConnectionConfigGetHeaderDigest(iSCSIConnectionConfigRef config);

/*! Sets whether a header digest is enabled in the config object.
 * @param config the iSCSI config object.
 * @param digestType the type of digest to use. */
void iSCSIConnectionConfigSetHeaderDigest(iSCSIConnectionConfigRef config,
                                          enum iSCSIDigestTypes digestType);

/*! Gets whether a data digest is enabled in the config object.
 *  @param config the iSCSI config object.
 *  @return the type of digest to use. */
enum iSCSIDigestTypes iSCSIConnectionConfigGetDataDigest(iSCSIConnectionConfigRef config);

/*! Sets whether a data digest is enabled in the config object.
 *  @param config the iSCSI config object.
 *  @param digestType the type of digest to use. */
void iSCSIConnectionConfigSetDataDigest(iSCSIConnectionConfigRef config,
                                        enum iSCSIDigestTypes digestType);

/*! Releases memory associated with an iSCSI connection configuration object.
 *  @param config an iSCSI connection configuration object. */
void iSCSIConnectionConfigRelease(iSCSIConnectionConfigRef config);

/*! Retains memory associated with an iSCSI connection configuration object.
 *  @param config an iSCSI connection configuration object. */
void iSCSIConnectionConfigRetain(iSCSIConnectionConfigRef config);

/*! Creates a new configuration object object from a dictionary representation.
 *  @return an iSCSI connection configuration object or
 *  NULL if object creation failed. */
iSCSIConnectionConfigRef iSCSIConnectionConfigCreateWithDictionary(CFDictionaryRef dict);

/*! Copies an configuration object to a dictionary representation.
 *  @param config an iSCSI configuration object.
 *  @return a dictionary representation of the configuration object or
 *  NULL if configuration object is invalid. */
CFDictionaryRef iSCSIConnectionConfigCreateDictionary(iSCSIConnectionConfigRef config);

/*! Copies the configuration object to a byte array representation.
 *  @param config an iSCSI configuration object.
 *  @return data representing the configuration object
 *  or NULL if the configuration object is invalid. */
CFDataRef iSCSIConnectionConfigCreateData(iSCSIConnectionConfigRef config);



#endif
