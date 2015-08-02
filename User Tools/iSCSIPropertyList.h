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



/*! Copies a target object for the specified target.
 *  @param targetIQN the target name.
 *  @return target the target object to copy. */
iSCSITargetRef iSCSIPLCopyTarget(CFStringRef targetIQN);

/*! Adds a target object.
 *  @param target an iSCSI target reference. */
void iSCSIPLSetTarget(iSCSITargetRef target);

/*! Removes a target object.
 *  @param targetIQN the target name. */
void iSCSIPLRemoveTarget(CFStringRef targetIQN);



/*! Copies a portal object for the specified target.
 *  @param targetIQN the target name.
 *  @param poralName the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPLCopyPortalForTarget(CFStringRef targetIQN,
                                          CFStringRef portalAddress);

/*! Sets a portal object for the specified target.
 *  @param targetIQN the target name.
 *  @param portal the portal object to set. */
void iSCSIPLSetPortalForTarget(CFStringRef targetIQN,iSCSIPortalRef portal);

/*! Removes a portal object for a particular target.
 *  @param targetIQN the target name.
 *  @param portalAddress the portal name to remove. */
void iSCSIPLRemovePortalForTarget(CFStringRef targetIQN,
                                  CFStringRef portalAddress);




/*! Copies the session configuration from the property list into an object.
 *  @param targetIQN the name of the target.
 *  @return a session configuration object. */
iSCSISessionConfigRef iSCSIPLCopySessionConfig(CFStringRef targetIQN);

/*! Sets the session configuration for a particular target.
 *  @param targetIQN the name of the target to set.
 *  @param sessCfg the session configuration object. */
void iSCSIPLSetSessionConfig(CFStringRef targetIQN,
                             iSCSISessionConfigRef sessCfg);

/*! Copies a connection configuration object associated with a particular
 *  portal for the specified target.
 *  @param targetIQN the target name.
 *  @param poralName the portal name (IPv4, IPv6 or DNS name).
 *  @return the connection configuration object to copy. */
iSCSIConnectionConfigRef iSCSIPLCopyConnectionConfig(CFStringRef targetIQN,
                                                     CFStringRef portalAddress);

/*! Sets a connection configuration object associated with a particualr portal
 *  for the specified target.
 *  @param targetIQN the target name.
 *  @param poralName the portal name (IPv4, IPv6 or DNS name).
 *  @param connCfg the connection configuration object to set. */
void iSCSIPLSetConnectionConfig(CFStringRef targetIQN,
                                CFStringRef portalAddress,
                                iSCSIConnectionConfigRef connCfg);

/*! Copies an authentication object associated with a particular
 *  portal for the specified target.
 *  @param targetIQN the target name.
 *  @return the authentication object. */
iSCSIAuthRef iSCSIPLCopyAuthenticationForTarget(CFStringRef targetIQN);

/*! Sets an authentication object associated with a particular portal
 *  for the specified target.
 *  @param targetIQN the target name.
 *  @param auth the connection configuration object to set. */
void iSCSIPLSetAuthenticationForTarget(CFStringRef targetIQN,
                                       iSCSIAuthRef auth);

/*! Gets whether a target is defined in the property list.
 *  @param targetIQN the name of the target.
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPLContainsTarget(CFStringRef targetIQN);

/*! Gets whether a portal is defined in the property list.
 *  @param targetIQN the name of the target.
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortalForTarget(CFStringRef targetIQN,
                                       CFStringRef portalAddress);

/*! Creates an array of target names (fully qualified IQN or EUI names)
 *  defined in the property list.
 *  @return an array of target names. */
CFArrayRef iSCSIPLCreateArrayOfTargets();

/*! Creates an array of portal names (addresses) for a given target.
 *  @param targetIQN the name of the target (fully qualified IQN or EUI name).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPLCreateArrayOfPortalsForTarget(CFStringRef targetIQN);





/*! Adds a discovery record to the property list.
 *  @param discoveryRecord the record to add. */
void iSCSIPLAddDiscoveryRecord(iSCSIDiscoveryRecRef discoveryRecord);

/*! Retrieves the discovery record from the property list.
 *  @return the cached discovery record. */
iSCSIDiscoveryRecRef iSCSIPLCopyDiscoveryRecord();

/*! Clears the discovery record. */
void iSCSIPLClearDiscoveryRecord();

/*! Synchronizes the intitiator and target settings cache with the property
 *  list on the disk. */
void iSCSIPLSynchronize();


#endif
