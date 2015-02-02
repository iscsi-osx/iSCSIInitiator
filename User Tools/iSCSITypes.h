/*!
 * @author		Nareg Sinenian
 * @file		iSCSITypes.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI data types used in user space.
 */

#ifndef __ISCSI_TYPES_H__
#define __ISCSI_TYPES_H__

#include <CoreFoundation/CoreFoundation.h>
#include "iSCSITypesShared.h"

typedef CFMutableDictionaryRef iSCSIMutablePortalRef;
typedef CFDictionaryRef iSCSIPortalRef;

typedef CFDictionaryRef iSCSITargetRef;
typedef CFMutableDictionaryRef iSCSIMutableTargetRef;

typedef CFDictionaryRef iSCSIAuthRef;

/*! Valid iSCSI authentication methods. */
enum iSCSIAuthMethods {
    
    /*! No authentication. */
    kiSCSIAuthMethodNone = 0,
    
    /*! CHAP authentication. */
    kiSCSIAuthMethodCHAP = 1
};


/*! Detailed login respones from a target that supplement the
 *  general responses defined by iSCSIPDULoginRspStatusClass. */
enum iSCSIStatusCode {
    
    kiSCSIPDULDSuccess = 0x0000,
    kiSCSIPDULDTargetMovedTemp = 0x0101,
    kiSCSIPDULDTargetMovedPerm = 0x0102,
    kiSCSIPDULDInitiatorError = 0x0200,
    kiSCSIPDULDAuthFail = 0x0201,
    kiSCSIPDULDAccessDenied = 0x0202,
    kiSCSIPDULDNotFound = 0x0203,
    kiSCSIPDULDTargetRemoved = 0x0204,
    kiSCSIPDULDUnsupportedVer = 0x0205,
    kiSCSIPDULDTooManyConnections = 0x0206,
    kiSCSIPDULDMissingParam = 0x0207,
    kiSCSIPDULDCantIncludeInSeession = 0x0208,
    kiSCSIPDULDSessionTypeUnsupported = 0x0209,
    kiSCSIPDULDSessionDoesntExist = 0x020a,
    kiSCSIPDULDInvalidReqDuringLogin = 0x020b,
    kiSCSIPDULDTargetHWorSWError = 0x0300,
    kiSCSIPDULDServiceUnavailable = 0x0301,
    kiSCSIPDULDOutOfResources = 0x0302
};



/*! Creates a new portal object from byte representation. */
iSCSIPortalRef iSCSIPortalCreateFromBytes(CFDataRef bytes);

/*! Creates a new iSCSIPortal object with empty portal parameters. */
iSCSIMutablePortalRef iSCSIPortalCreate();

/*! Gets the address associated with the iSCSI portal. */
CFStringRef iSCSIPortalGetAddress(iSCSIPortalRef portal);

/*! Sets the address associated with the iSCSI portal. */
void iSCSIPortalSetAddress(iSCSIMutablePortalRef portal,CFStringRef address);

/*! Gets the port associated with the iSCSI portal. */
CFStringRef iSCSIPortalGetPort(iSCSIPortalRef portal);

/*! Sets the port associated with the iSCSI portal. */
void iSCSIPortalSetPort(iSCSIMutablePortalRef portal,CFStringRef port);

/*! Gets the interface associated with the iSCSI portal. */
CFStringRef iSCSIPortalGetHostInterface(iSCSIPortalRef portal);

/*! Sets the interface associated with the iSCSI portal. */
void iSCSIPortalSetHostInterface(iSCSIMutablePortalRef portal,
                                 CFStringRef hostInterface);

/*! Releases memory associated with an iSCSI portal object. */
void iSCSIPortalRelease(iSCSITargetRef portal);

/*! Retains an iSCSI portal object. */
void iSCSIPortalRetain(iSCSITargetRef portal);

/*! Creates a new portal object from a dictionary representation. */
iSCSIPortalRef iSCSIPortalCreateFromDictionary(CFDictionaryRef dict);

/*! Copies a target object to a dictionary representation. */
CFDictionaryRef iSCSIPortalCopyToDictionary(iSCSIPortalRef portal);

/*! Copies the portal object to a byte array representation. */
CFDataRef iSCSIPortalCopyToBytes(iSCSIPortalRef portal);



/*! Creates a new target object from byte representation. */
iSCSITargetRef iSCSITargetCreateFromBytes(CFDataRef bytes);

/*! Creates a new iSCSITarget object with empty target parameters. */
iSCSIMutableTargetRef iSCSITargetCreate();

/*! Gets the name associated with the iSCSI target. */
CFStringRef iSCSITargetGetName(iSCSITargetRef target);

/*! Sets the name associated with the iSCSI target. */
void iSCSITargetSetName(iSCSIMutableTargetRef target,CFStringRef name);

/*! Gets whether a header digest is enabled in the target object. */
bool iSCSITargetGetHeaderDigest(iSCSITargetRef target);

/*! Sets whether a header digest is enabled in the target object. */
void iSCSITargetSetHeaderDigest(iSCSIMutableTargetRef target,bool enable);

/*! Gets whether a data digest is enabled in the target object. */
bool iSCSITargetGetDataDigest(iSCSITargetRef target);

/*! Sets whether a data digest is enabled in the target object. */
void iSCSITargetSetDataDigest(iSCSIMutableTargetRef target,bool enable);

/*! Releases memory associated with an iSCSI target object. */
void iSCSITargetRelease(iSCSITargetRef target);

/*! Retains an iSCSI target object. */
void iSCSITargetRetain(iSCSITargetRef target);

/*! Creates a new target object from a dictionary representation. */
iSCSITargetRef iSCSITargetCreateFromDictionary(CFDictionaryRef dict);

/*! Copies a target object to a dictionary representation. */
CFDictionaryRef iSCSITargetCopyToDictionary(iSCSITargetRef target);

/*! Copies the target object to a byte array representation. */
CFDataRef iSCSITargetCopyToBytes(iSCSITargetRef target);


/*! Creates a new authentication object from byte representation. */
iSCSIAuthRef iSCSIAuthCreateFromBytes(CFDataRef bytes);

/*! Creates a new iSCSIAuth object with empty authentication parameters
 *  (defaults to no authentication). */
iSCSIAuthRef iSCSIAuthCreateNone();

/*! Creates a new iSCSIAuth object with empty authentication parameters
 *  (defaults to no authentication). */
iSCSIAuthRef iSCSIAuthCreateCHAP(CFStringRef initiatorUser,
                                 CFStringRef initiatorSecret,
                                 CFStringRef targetUser,
                                 CFStringRef targetSecret);

/*! Returns the CHAP authentication parameter values if the authentication
 *  method is actually CHAP. */
errno_t iSCSIAuthGetCHAPValues(iSCSIAuthRef auth,
                               CFStringRef * initiatorUser,
                               CFStringRef * initiatorSecret,
                               CFStringRef * targetUser,
                               CFStringRef * targetSecret);


/*! Gets the authentication method used. */
enum iSCSIAuthMethods iSCSIAuthGetMethod(iSCSIAuthRef auth);

/*! Releases memory associated with an iSCSI auth object. */
void iSCSIAuthRelease(iSCSIAuthRef auth);

/*! Retains an iSCSI auth object. */
void iSCSIAuthRetain(iSCSIAuthRef auth);

/*! Creates a new authentication object from a dictionary representation. */
iSCSIAuthRef iSCSIAuthCreateFromDictionary(CFDictionaryRef dict);

/*! Copies an authentication object to a dictionary representation. */
CFDictionaryRef iSCSIAuthCopyToDictionary(iSCSIAuthRef auth);

/*! Copies the authentication object to a byte array representation. */
CFDataRef iSCSIAuthCopyToBytes(iSCSIAuthRef auth);





#endif
