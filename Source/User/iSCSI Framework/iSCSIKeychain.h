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
 *  @param sharedSecret the shared secret to store.
 *  @param return error code indicating the result of the operation. */
OSStatus iSCSIKeychainSetCHAPSecretForNode(CFStringRef nodeIQN,
                                           CFStringRef sharedSecret);

/*! Removes the shared secret associated with a particular
 *  iSCSI node (either initiator or target) from the system keychain.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @param return error code indicating the result of the operation. */
OSStatus iSCSIKeychainDeleteCHAPSecretForNode(CFStringRef nodeIQN);

/*! Gets whether a CHAP secret exists for the specified node.
 *  @param nodeIQN the node to test.
 *  @return true if a CHAP secret exists for the specified node. */
Boolean iSCSIKeychainContainsCHAPSecretForNode(CFStringRef nodeIQN);

#endif /* defined(__ISCSI_KEYCHAIN_H__) */
