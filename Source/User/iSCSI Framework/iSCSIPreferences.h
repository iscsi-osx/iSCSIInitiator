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

#ifndef __ISCSI_PREFERENCES_H__
#define __ISCSI_PREFERENCES_H__

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPreferences.h>
#include <CoreFoundation/CFPropertyList.h>

#include "iSCSIKeychain.h"
#include "iSCSITypes.h"
#include "iSCSIUtils.h"

typedef CFMutableDictionaryRef iSCSIPreferencesRef;


/*! Copies the initiator name from preferences into a CFString object.
 *  @param preferences an iSCSI preferences object.
 *  @return the initiator name. */
CFStringRef iSCSIPreferencesCopyInitiatorIQN(iSCSIPreferencesRef preferences);

/*! Sets the initiator name in preferences.
 *  @param preferences an iSCSI preferences object.
 *  @param initiatorIQN the initiator name to set. */
void iSCSIPreferencesSetInitiatorIQN(iSCSIPreferencesRef preferences,
                                     CFStringRef initiatorIQN);

/*! Copies the initiator alias from preferences into a CFString object.
 *  @param preferences an iSCSI preferences object.
 *  @return the initiator alias. */
CFStringRef iSCSIPreferencesCopyInitiatorAlias(iSCSIPreferencesRef preferences);

/*! Sets the initiator alias in preferences.
 *  @param preferences an iSCSI preferences object.
 *  @param initiatorAlias the initiator alias to set. */
void iSCSIPreferencesSetInitiatorAlias(iSCSIPreferencesRef preferences,
                                       CFStringRef initiatorAlias);

/*! Sets authentication method to be used by initiator.
 *  @param preferences an iSCSI preferences object. */
void iSCSIPreferencesSetInitiatorAuthenticationMethod(iSCSIPreferencesRef preferences,
                                                      enum iSCSIAuthMethods authMethod);

/*! Gets the current authentication method used by the initiator.
 *  @param preferences an iSCSI preferences object. */
enum iSCSIAuthMethods iSCSIPreferencesGetInitiatorAuthenticationMethod(iSCSIPreferencesRef preferences);

/*! Sets the CHAP name associated with the initiator.
 *  @param preferences an iSCSI preferences object. */
void iSCSIPreferencesSetInitiatorCHAPName(iSCSIPreferencesRef preferences,
                                          CFStringRef name);

/*! Copies the CHAP name associated with the initiator.
 *  @param preferences an iSCSI preferences object. */
CFStringRef iSCSIPreferencesCopyInitiatorCHAPName(iSCSIPreferencesRef preferences);

/*! Copies a target object for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return target the target object to copy. */
iSCSITargetRef iSCSIPreferencesCopyTarget(iSCSIPreferencesRef preferences,
                                          CFStringRef targetIQN);

/*! Adds a target object with a specified portal.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set. */
void iSCSIPreferencesAddStaticTarget(iSCSIPreferencesRef preferences,
                                     CFStringRef targetIQN,
                                     iSCSIPortalRef portal);

/*! Adds a target object with a specified portal, and associates it with
 *  a particular SendTargets portal that manages the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set.
 *  @param sendTargetsPortal the discovery portal address with which to
 *  associate the managed target. */
void iSCSIPreferencesAddDynamicTargetForSendTargets(iSCSIPreferencesRef preferences,
                                                    CFStringRef targetIQN,
                                                    iSCSIPortalRef portal,
                                                    CFStringRef sendTargetsPortal);

/*! Removes a target object.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
void iSCSIPreferencesRemoveTarget(iSCSIPreferencesRef preferences,
                                  CFStringRef targetIQN);

/*! Copies a portal object for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param poralAddress the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPreferencesCopyPortalForTarget(iSCSIPreferencesRef preferences,
                                                   CFStringRef targetIQN,
                                                   CFStringRef portalAddress);

/*! Sets a portal object for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set. */
void iSCSIPreferencesSetPortalForTarget(iSCSIPreferencesRef preferences,
                                        CFStringRef targetIQN,
                                        iSCSIPortalRef portal);

/*! Removes a portal object for a particular target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portalAddress the portal name to remove. */
void iSCSIPreferencesRemovePortalForTarget(iSCSIPreferencesRef preferences,
                                           CFStringRef targetIQN,
                                           CFStringRef portalAddress);

/*! Sets whether the target should be logged in during startup.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param autoLogin true to auto login, false otherwise. */
void iSCSIPreferencesSetAutoLoginForTarget(iSCSIPreferencesRef preferences,
                                           CFStringRef targetIQN,
                                           Boolean autoLogin);

/*! Gets whether the target should be logged in during startup.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
Boolean iSCSIPreferencesGetAutoLoginForTarget(iSCSIPreferencesRef preferences,
                                              CFStringRef targetIQN);

/*! Sets whether the a connection to the target should be re-established
 *  in the event of an interruption.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param persistent true to make target persistent, false otherwise. */
void iSCSIPreferencesSetPersistenceForTarget(iSCSIPreferencesRef preferences,
                                             CFStringRef targetIQN,
                                             Boolean persistent);

/*! Gets whether the a connection to the target should be re-established
 *  in the event of an interruption.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
Boolean iSCSIPreferencesGetPersistenceForTarget(iSCSIPreferencesRef preferences,
                                                CFStringRef targetIQN);

/*! Sets the maximum number of connections for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param maxConnections the maximum number of connections. */
void iSCSIPreferencesSetMaxConnectionsForTarget(iSCSIPreferencesRef preferences,
                                                CFStringRef targetIQN,
                                                UInt32 maxConnections);

/*! Gets the maximum number of connections for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the maximum number of connections for the target. */
UInt32 iSCSIPreferencesGetMaxConnectionsForTarget(iSCSIPreferencesRef preferences,
                                         CFStringRef targetIQN);

/*! Sets the error recovery level to use for the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param level the error recovery level. */
void iSCSIPreferencesSetErrorRecoveryLevelForTarget(iSCSIPreferencesRef preferences,
                                                    CFStringRef targetIQN,
                                                    enum iSCSIErrorRecoveryLevels level);

/*! Gets the error recovery level to use for the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the error recovery level. */
enum iSCSIErrorRecoveryLevels iSCSIPreferencesGetErrorRecoveryLevelForTarget(iSCSIPreferencesRef preferences,
                                                                             CFStringRef targetIQN);

/*! Gets the data digest for the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the digest type. */
enum iSCSIDigestTypes iSCSIPreferencesGetDataDigestForTarget(iSCSIPreferencesRef preferences,
                                                             CFStringRef targetIQN);

/*! Sets the data digest for the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param digestType the digest type. */
void iSCSIPreferencesSetDataDigestForTarget(iSCSIPreferencesRef preferences,
                                            CFStringRef targetIQN,
                                            enum iSCSIDigestTypes digestType);

/*! Gets the header digest for the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the digest type. */
enum iSCSIDigestTypes iSCSIPreferencesGetHeaderDigestForTarget(iSCSIPreferencesRef preferences,
                                                               CFStringRef targetIQN);

/*! Sets the header digest for the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param digestType the digest type. */
void iSCSIPreferencesSetHeaderDigestForTarget(iSCSIPreferencesRef preferences,
                                              CFStringRef targetIQN,
                                              enum iSCSIDigestTypes digestType);

/*! Modifies the target IQN for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param existingIQN the IQN of the existing target to modify.
 *  @param newIQN the new IQN to assign to the target. */
void iSCSIPreferencesSetTargetIQN(iSCSIPreferencesRef preferences,
                                  CFStringRef existingIQN,
                                  CFStringRef newIQN);

/*! Sets the alias for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param alias the alias to associate with the specified target. */
void iSCSIPreferencesSetTargetAlias(iSCSIPreferencesRef preferences,
                                    CFStringRef targetIQN,
                                    CFStringRef alias);

/*! Gets the alias for the specified target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the alias associated with the specified target. */
CFStringRef iSCSIPreferencesGetTargetAlias(iSCSIPreferencesRef preferences,
                                           CFStringRef targetIQN);

/*! Sets authentication method to be used by target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param authMethod the authentication method to use. */
void iSCSIPreferencesSetTargetAuthenticationMethod(iSCSIPreferencesRef preferences,
                                                   CFStringRef targetIQN,
                                                   enum iSCSIAuthMethods authMethod);

/*! Gets the current authentication method used by the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the iSCSI authentication method for the target. */
enum iSCSIAuthMethods iSCSIPreferencesGetTargetAuthenticationMethod(iSCSIPreferencesRef preferences,
                                                                    CFStringRef targetIQN);

/*! Sets target configuration type.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param configType the target configuration type. */
void iSCSIPreferencesSetTargetConfigType(iSCSIPreferencesRef preferences,
                                         CFStringRef targetIQN,
                                         enum iSCSITargetConfigTypes configType);

/*! Gets target configuration type.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the target configuration type. */
enum iSCSITargetConfigTypes iSCSIPreferencesGetTargetConfigType(iSCSIPreferencesRef preferences,
                                                                CFStringRef targetIQN);

/*! Gets the SendTargets discovery portal associated with the dynamic target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return address of the discovery portal that manages the target. */
CFStringRef iSCSIPreferencesGetDiscoveryPortalForTarget(iSCSIPreferencesRef preferences,
                                                        CFStringRef targetIQN);

/*! Sets the CHAP name associated with the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param name the CHAP name associated with the target. */
void iSCSIPreferencesSetTargetCHAPName(iSCSIPreferencesRef preferences,
                                       CFStringRef targetIQN,
                                       CFStringRef name);

/*! Copies the CHAP name associated with the target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP name associated with the target. */
CFStringRef iSCSIPreferencesCopyTargetCHAPName(iSCSIPreferencesRef preferences,
                                               CFStringRef targetIQN);

/*! Gets whether a target is defined in preferences.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPreferencesContainsTarget(iSCSIPreferencesRef preferences,
                                       CFStringRef targetIQN);

/*! Gets whether a portal is defined in preferences.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPreferencesContainsPortalForTarget(iSCSIPreferencesRef preferences,
                                                CFStringRef targetIQN,
                                                CFStringRef portalAddress);

/*! Gets whether a SendTargets discovery portal is defined in preferences.
 *  @param preferences an iSCSI preferences object.
 *  @param portalAddress the discovery portal address.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPreferencesContainsPortalForSendTargetsDiscovery(iSCSIPreferencesRef preferences,
                                                              CFStringRef portalAddress);

/*! Creates an array of target iSCSI qualified name (IQN)s defined in preferences.
 *  @param preferences an iSCSI preferences object.
 *  @return an array of target iSCSI qualified name (IQN)s. */
CFArrayRef iSCSIPreferencesCreateArrayOfTargets(iSCSIPreferencesRef preferences);

/*! Creates an array of iSCSI targets (IQNs) that were dynamically configured 
 *  using SendTargets over a specific discovery portal.
 *  @param preferences an iSCSI preferences object.
 *  @param portalAddress the address of the discovery portal that corresponds
 *  to the dynamically configured discovery targets.
 *  @return an array of iSCSI targets (IQNs) that were discovered using 
 *  SendTargets over the specified portal. */
CFArrayRef iSCSIPreferencesCreateArrayOfDynamicTargetsForSendTargets(iSCSIPreferencesRef preferences,
                                                            CFStringRef portalAddress);

/*! Creates an array of portal names (addresses) for a given target.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPreferencesCreateArrayOfPortalsForTarget(iSCSIPreferencesRef preferences,
                                                         CFStringRef targetIQN);

/*! Adds an iSCSI discovery portal to the list of discovery portals.
 *  @param preferences an iSCSI preferences object.
 *  @param portal the discovery portal to add. */
void iSCSIPreferencesAddSendTargetsDiscoveryPortal(iSCSIPreferencesRef preferences,
                                                   iSCSIPortalRef portal);

/*! Removes the specified iSCSI discovery portal.
 *  @param preferences an iSCSI preferences object.
 *  @param portal the discovery portal to remove. */
void iSCSIPreferencesRemoveSendTargetsDiscoveryPortal(iSCSIPreferencesRef preferences,
                                                      iSCSIPortalRef portal);

/*! Copies a portal object for the specified discovery portal.
 *  @param preferences an iSCSI preferences object.
 *  @param portalAddress the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPreferencesCopySendTargetsDiscoveryPortal(iSCSIPreferencesRef preferences,
                                                              CFStringRef portalAddress);

/*! Creates a list of SendTargets portals.  Each element of the array is
 *  an iSCSI discovery portal address that can be used to retrieve the
 *  corresponding portal object by calling iSCSIPreferencesCopySendTargetsPortal().
 *  @param preferences an iSCSI preferences object. */
CFArrayRef iSCSIPreferencesCreateArrayOfPortalsForSendTargetsDiscovery(iSCSIPreferencesRef preferences);

/*! Sets SendTargets discovery to enabled or disabled.
 *  @param preferences an iSCSI preferences object.
 *  @param enable True to set send targets discovery enabled, false otherwise. */
void iSCSIPreferencesSetSendTargetsDiscoveryEnable(iSCSIPreferencesRef preferences,
                                                   Boolean enable);

/*! Gets whether SendTargets discovery is set ot disabled or enabled.
 *  @param preferences an iSCSI preferences object.
 *  @return True if send targets discovery is set to enabled, false otherwise. */
Boolean iSCSIPreferencesGetSendTargetsDiscoveryEnable(iSCSIPreferencesRef preferences);

/*! Sets SendTargets discovery interval.
 *  @param preferences an iSCSI preferences object.
 *  @param interval the discovery interval, in seconds. */
void iSCSIPreferencesSetSendTargetsDiscoveryInterval(iSCSIPreferencesRef preferences,
                                                     CFIndex interval);

/*! Gets SendTargets disocvery interval.
 *  @param preferences an iSCSI preferences object.
 *  @return the discovery interval, in seconds. */
CFIndex iSCSIPreferencesGetSendTargetsDiscoveryInterval(iSCSIPreferencesRef preferences);

/*! Resets iSCSI preferences, removing all defined targets and
 *  configuration parameters.
 *  @param preferences an iSCSI preferences object. */
void iSCSIPreferencesReset(iSCSIPreferencesRef preferences);

/*! Create a dictionary representation of the preferences object.
 *  @param preferences an iSCSI preferences object.
 *  @return a dictionary representation containing key-value pairs of preferences values. */
CFDictionaryRef iSCSIPreferencesCreateDictionary(iSCSIPreferencesRef preferences);

/*! Creates a binary representation of an iSCSI preferences object.
 *  @param preferences an iSCSI preferences object.
 *  @return a binary data representation of a preferences object. */
CFDataRef iSCSIPreferencesCreateData(iSCSIPreferencesRef preferences);

/*! Creates a new iSCSI preferences object.
 *  @return a new (empty) preferences object. */
iSCSIPreferencesRef iSCSIPreferencesCreate();

/*! Creates a new iSCSI preferences object from using values stored in system preferences.
 *  @param kAppId the application identification string.
 *  @return a new preferences object containing stored iSCSI preferences. */
iSCSIPreferencesRef iSCSIPreferencesCreateFromAppValues();

/*! Creates a new iSCSI preferences object from a dictionary object from a dictionary representation.
 *  @param dict a dictionary used to construct an iSCSI preferences object.
 *  @return an iSCSI preferences object or NULL if object creation failed. */
iSCSIPreferencesRef iSCSIPreferencesCreateWithDictionary(CFDictionaryRef dict);

/*! Creates a new iSCSI preferences object from binary data.
 *  @param data the data used to create the preferences object.
 *  @return a new preferences object containing stored iSCSI preferences. */
iSCSIPreferencesRef iSCSIPreferencesCreateWithData(CFDataRef data);

/*! Updates the contents of the iSCSI preferences with application values.
 *  @param preferences an iSCSI preferences object. */
void iSCSIPreferencesUpdateWithAppValues(iSCSIPreferencesRef preferences);

/*! Synchronizes application values with those in the preferences object.
 *  @param preferences an iSCSI preferences object.
 *  @return true if application values were successfully updated. */
Boolean iSCSIPreferencesSynchronzeAppValues(iSCSIPreferencesRef preferences);


/*! Releasese an iSCSI preferences object.
 *  @param preferences an iSCSI preferences object. */
void iSCSIPreferencesRelease(iSCSIPreferencesRef preferences);


#endif
