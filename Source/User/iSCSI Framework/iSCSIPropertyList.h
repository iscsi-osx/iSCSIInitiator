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

#ifndef __ISCSI_PROPERTY_LIST_H__
#define __ISCSI_PROPERTY_LIST_H__

#include <CoreFoundation/CoreFoundation.h>

#include "iSCSIKeychain.h"
#include "iSCSITypes.h"
#include "iSCSIUtils.h"

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

/*! Sets the CHAP secret associated with the initiator.
 *  @return status indicating the result of the operation. */
OSStatus iSCSIPLSetInitiatorCHAPSecret(CFStringRef secret);

/*! Copies the CHAP secret associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPSecret();

/*! Gets whether a CHAP secret exists for the initiator.
 *  @return true if a CHAP secret exists for the initiator. */
Boolean iSCSIPLExistsInitiatorCHAPSecret();

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

/*! Sets whether the target should be logged in during startup.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param autoLogin true to auto login, false otherwise. */
void iSCSIPLSetAutoLoginForTarget(CFStringRef targetIQN,Boolean autoLogin);

/*! Gets whether the target should be logged in during startup.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
Boolean iSCSIPLGetAutoLoginForTarget(CFStringRef targetIQN);

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

/*! Sets the alias for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param alias the alias to associate with the specified target. */
void iSCSIPLSetTargetAlias(CFStringRef targetIQN,CFStringRef alias);

/*! Gets the alias for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the alias associated with the specified target. */
CFStringRef iSCSIPLGetTargetAlias(CFStringRef targetIQN);

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

/*! Gets the SendTargets discovery portal associated with the dynamic target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return address of the discovery portal that manages the target. */
CFStringRef iSCSIPLGetDiscoveryPortalForTarget(CFStringRef targetIQN);

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
 *  @param secret the CHAP shared secret associated with the target.
 *  @return status indicating the result of the operation. */
OSStatus iSCSIPLSetTargetCHAPSecret(CFStringRef targetIQN,CFStringRef secret);

/*! Copies the CHAP secret associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP shared secret associated with the target. */
CFStringRef iSCSIPLCopyTargetCHAPSecret(CFStringRef targetIQN);

/*! Gets whether a CHAP secret exists for the specified target.
 *  @return true if a CHAP secret exists for the target. */
Boolean iSCSIPLExistsTargetCHAPSecret(CFStringRef nodeIQN);

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

/*! Creates an array of statically configured iSCSI targets (IQNs).
 *  @return an array of statically configured iSCSI targets (IQNs). */
CFArrayRef iSCSIPLCreateArrayOfStaticTargets();

/*! Creates an array of iSCSI targets (IQNs) that were dynamically configured 
 *  using SendTargets over a specific discovery portal.
 *  @param portalAddress the address of the discovery portal that corresponds
 *  to the dynamically configured discovery targets.
 *  @return an array of iSCSI targets (IQNs) that were discovered using 
 *  SendTargets over the specified portal. */
CFArrayRef iSCISPLCreateArrayOfDynamicTargetsForSendTargets(CFStringRef portalAddress);

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
iSCSIPortalRef iSCSIPLCopySendTargetsDiscoveryPortal(CFStringRef portalAddress);

/*! Creates a list of SendTargets portals.  Each element of the array is
 *  an iSCSI discovery portal address that can be used to retrieve the
 *  corresponding portal object by calling iSCSIPLCopySendTargetsPortal(). */
CFArrayRef iSCSIPLCreateArrayOfPortalsForSendTargetsDiscovery();

/*! Sets SendTargets discovery to enabled or disabled.
 *  @param enable True to set send targets discovery enabled, false otherwise. */
void iSCSIPLSetSendTargetsDiscoveryEnable(Boolean enable);

/*! Gets whether SendTargets discovery is set ot disabled or enabled.
 *  @return True if send targets discovery is set to enabled, false otherwise. */
Boolean iSCSIPLGetSendTargetsDiscoveryEnable();

/*! Sets SendTargets discovery interval.
 *  @param interval the discovery interval, in seconds. */
void iSCSIPLSetSendTargetsDiscoveryInterval(CFIndex interval);

/*! Gets SendTargets disocvery interval.
 *  @return the discovery interval, in seconds. */
CFIndex iSCSIPLGetSendTargetsDiscoveryInterval();

/*! Resets the iSCSI property list, removing all defined targets and
 *  configuration parameters. */
void iSCSIPLReset();

/*! Synchronizes the intitiator and target settings cache with the property
 *  list on the disk. */
Boolean iSCSIPLSynchronize();


#endif
