/*!
 * @author		Nareg Sinenian
 * @file		iSCSIKeychain.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions that wrap around the
 *              security keychain to provide iSCSI key maangement
 */


#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPreferences.h>
#include <CoreFoundation/CFPropertyList.h>

#include <Security/Security.h>

#ifndef __ISCSI_KEYCHAIN_H__
#define __ISCSI_KEYCHAIN_H__


/*! Copies a shared secret associated with a particular
 *  iSCSI node (either initiator or target) to the system keychain.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @return the shared secret for the specified node. */
CFStringRef iSCSIKeychainCopyCHAPSecretForNode(CFStringRef nodeIQN);

/*! Updates the shared secret associated with a particular
 *  iSCSI node (either initiator or target) to the system keychain. An entry
 *  for the node is created if it does not exist. If it does exist, the shared
 *  secret for is updated.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @param sharedSecret the shared secret to store. */
void iSCSIKeychainSetCHAPSecretForNode(CFStringRef nodeIQN,
                                          CFStringRef sharedSecret);

/*! Renames the iSCSI node in they keychain.
 *  @param oldNodeIQN the old node iSCSI qualified name (IQN).
 *  @param newNodeIQN the new node iSCSI qualified name (IQN). */
void iSCSIKeychainRenameNode(CFStringRef oldNodeIQN,CFStringRef newNodeIQN);

#endif /* defined(__ISCSI_KEYCHAIN_H__) */
