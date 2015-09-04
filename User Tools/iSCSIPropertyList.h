/*!
 * @author		Nareg Sinenian
 * @file		iSCSIPropertyList.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions to read and write
 *              daemon configuration property list
 */

#ifndef __ISCSI_PROPERTY_LIST_H__
#define __ISCSI_PROPERTY_LIST_H__

#include <CoreFoundation/CoreFoundation.h>

#include "iSCSIKeychain.h"
#include "iSCSITypes.h"

/*! Copies the initiator name from the property list into a CFString object.
 *  @return the initiator name. */
CFStringRef iSCSIPLCopyInitiatorIQN();

/*! Sets the initiator name in the property list.
 *  @param initiatorIQN the initiator name to set. */
void iSCSIPLSetInitiatorIQN(CFStringRef initiatorIQN);

/*! Copies the initiator alias from the property list into a CFString object.
 *  @return the initiator alias. */
CFStringRef iSCSIPLCopyInitiatorAlias();

/*! Sets the initiator alias in the property list.
 *  @param initiatorAlias the initiator alias to set. */
void iSCSIPLSetInitiatorAlias(CFStringRef initiatorAlias);

/*! Sets authentication method to be used by initiator. */
void iSCSIPLSetInitiatorAuthenticationMethod(enum iSCSIAuthMethods authMethod);

/*! Gets the current authentication method used by the initiator. */
enum iSCSIAuthMethods iSCSIPLGetInitiatorAuthenticationMethod();

/*! Sets the CHAP name associated with the initiator. */
void iSCSIPLSetInitiatorCHAPName(CFStringRef name);

/*! Copies the CHAP name associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPName();

/*! Sets the CHAP secret associated with the initiator. */
void iSCSIPLSetInitiatorCHAPSecret(CFStringRef secret);

/*! Copies the CHAP secret associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPSecret();

/*! Copies a target object for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return target the target object to copy. */
iSCSITargetRef iSCSIPLCopyTarget(CFStringRef targetIQN);

/*! Adds a target object with a specified portal.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set. */
void iSCSIPLAddStaticTarget(CFStringRef targetIQN,iSCSIPortalRef portal);

/*! Adds a target object with a specified portal, and associates it with
 *  a particular SendTargets portal that manages the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set.
 *  @param sendTargetsPortal the discovery portal address with which to
 *  associate the managed target. */
void iSCSIPLAddDynamicTargetForSendTargets(CFStringRef targetIQN,
                                           iSCSIPortalRef portal,
                                           CFStringRef sendTargetsPortal);

/*! Removes a target object.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
void iSCSIPLRemoveTarget(CFStringRef targetIQN);

/*! Copies a portal object for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param poralAddress the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPLCopyPortalForTarget(CFStringRef targetIQN,
                                          CFStringRef portalAddress);

/*! Sets a portal object for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set. */
void iSCSIPLSetPortalForTarget(CFStringRef targetIQN,iSCSIPortalRef portal);

/*! Removes a portal object for a particular target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portalAddress the portal name to remove. */
void iSCSIPLRemovePortalForTarget(CFStringRef targetIQN,
                                  CFStringRef portalAddress);

/*! Sets the maximum number of connections for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param maxConnections the maximum number of connections. */
void iSCSIPLSetMaxConnectionsForTarget(CFStringRef targetIQN,
                                       UInt32 maxConnections);

/*! Gets the maximum number of connections for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the maximum number of connections for the target. */
UInt32 iSCSIPLGetMaxConnectionsForTarget(CFStringRef targetIQN);

/*! Sets the error recovery level to use for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param level the error recovery level. */
void iSCSIPLSetErrorRecoveryLevelForTarget(CFStringRef targetIQN,
                                           enum iSCSIErrorRecoveryLevels level);

/*! Gets the error recovery level to use for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the error recovery level. */
enum iSCSIErrorRecoveryLevels iSCSIPLGetErrorRecoveryLevelForTarget(CFStringRef targetIQN);

/*! Gets the data digest for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the digest type. */
enum iSCSIDigestTypes iSCSIPLGetDataDigestForTarget(CFStringRef targetIQN);

/*! Sets the data digest for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param digestType the digest type. */
void iSCSIPLSetDataDigestForTarget(CFStringRef targetIQN,
                                   enum iSCSIDigestTypes digestType);

/*! Gets the header digest for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the digest type. */
enum iSCSIDigestTypes iSCSIPLGetHeaderDigestForTarget(CFStringRef targetIQN);

/*! Sets the header digest for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param digestType the digest type. */
void iSCSIPLSetHeaderDigestForTarget(CFStringRef targetIQN,
                                     enum iSCSIDigestTypes digestType);

/*! Modifies the target IQN for the specified target.
 *  @param existingIQN the IQN of the existing target to modify.
 *  @param newIQN the new IQN to assign to the target. */
void iSCSIPLSetTargetIQN(CFStringRef existingIQN,CFStringRef newIQN);

/*! Sets authentication method to be used by target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param authMethod the authentication method to use. */
void iSCSIPLSetTargetAuthenticationMethod(CFStringRef targetIQN,
                                          enum iSCSIAuthMethods authMethod);

/*! Gets the current authentication method used by the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the iSCSI authentication method for the target. */
enum iSCSIAuthMethods iSCSIPLGetTargetAuthenticationMethod(CFStringRef targetIQN);

/*! Sets target configuration type.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param configType the target configuration type. */
void iSCSIPLSetTargetConfigType(CFStringRef targetIQN,
                                enum iSCSITargetConfigTypes configType);

/*! Gets target configuration type.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the target configuration type. */
enum iSCSITargetConfigTypes iSCSIPLGetTargetConfigType(CFStringRef targetIQN);

/*! Sets the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param name the CHAP name associated with the target. */
void iSCSIPLSetTargetCHAPName(CFStringRef targetIQN,CFStringRef name);

/*! Copies the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP name associated with the target. */
CFStringRef iSCSIPLCopyTargetCHAPName(CFStringRef targetIQN);

/*! Sets the CHAP secret associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param secret the CHAP shared secret associated with the target. */
void iSCSIPLSetTargetCHAPSecret(CFStringRef targetIQN,CFStringRef secret);

/*! Copies the CHAP secret associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP shared secret associated with the target. */
CFStringRef iSCSIPLCopyTargetCHAPSecret(CFStringRef targetIQN);

/*! Gets whether a target is defined in the property list.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPLContainsTarget(CFStringRef targetIQN);

/*! Gets whether a portal is defined in the property list.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortalForTarget(CFStringRef targetIQN,
                                       CFStringRef portalAddress);

/*! Gets whether a SendTargets discovery portal is defined in the property list.
 *  @param portalAddress the discovery portal address.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortalForSendTargetsDiscovery(CFStringRef portalAddress);

/*! Creates an array of target iSCSI qualified name (IQN)s
 *  defined in the property list.
 *  @return an array of target iSCSI qualified name (IQN)s. */
CFArrayRef iSCSIPLCreateArrayOfTargets();

/*! Creates an array of portal names (addresses) for a given target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPLCreateArrayOfPortalsForTarget(CFStringRef targetIQN);

/*! Adds an iSCSI discovery portal to the list of discovery portals.
 *  @param portal the discovery portal to add. */
void iSCSIPLAddSendTargetsDiscoveryPortal(iSCSIPortalRef portal);

/*! Removes the specified iSCSI discovery portal.
 *  @param portal the discovery portal to remove. */
void iSCSIPLRemoveSendTargetsDiscoveryPortal(iSCSIPortalRef portal);

/*! Copies a portal object for the specified discovery portal.
 *  @param portalAddress the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPLCopyiSCSIDiscoveryPortal(CFStringRef portalAddress);

/*! Creates a list of target IQNs associated with a particular
 *  @param portalAddress the portal name (IPv4, IPv6 or DNS name). 
 *  @return a list of target IQNs associated with an iSCSI discovery portal. */
CFArrayRef iSCSIPLCreateArrayOfTargetsForiSCSIDiscoveryPortal(CFStringRef portalAddress);

/*! Creates a list of discovery portals.  Each element of the array is
 *  an iSCSI discovery portal address that can be used to retrieve the
 *  corresponding portal object by calling iSCSIPLCopyDiscoveryPortal(). */
CFArrayRef iSCSIPLCreateArrayOfiSCSIDiscoveryPortals();

/*! Sets iSCSI discovery to enabled or disabled.
 *  @param enable True to set send targets discovery enabled, false otherwise. */
void iSCSIPLSetiSCSIDiscoveryEnable(Boolean enable);

/*! Gets whether iSCSI discovery is set ot disabled or enabled.
 *  @return True if send targets discovery is set to enabled, false otherwise. */
Boolean iSCSIPLGetiSCSIDiscoveryEnable();

/*! Synchronizes the intitiator and target settings cache with the property
 *  list on the disk. */
void iSCSIPLSynchronize();


#endif
