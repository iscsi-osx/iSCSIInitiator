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

#include "iSCSIKeychain.h"
#include <netdb.h>

/*! Copies a shared secret associated with a particular
 *  iSCSI node (either initiator or target) to the system keychain.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @return the shared secret for the specified node. */
CFStringRef iSCSIKeychainCopyCHAPSecretForNode(CFStringRef nodeIQN)
{
    CFStringRef sharedSecret = NULL;
    OSStatus status;
    SecKeychainItemRef item = NULL;

    // Get the system keychain and unlock it (prompts user if required)
    SecKeychainSetPreferenceDomain(kSecPreferencesDomainSystem);
    SecKeychainUnlock(NULL,0,NULL,false);

    UInt32 sharedSecretLength = 0;
    void * sharedSecretData = NULL;

    status = SecKeychainFindGenericPassword(NULL,
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            &sharedSecretLength,
            &sharedSecretData,&item);

    if(sharedSecretData) {
        sharedSecret = CFStringCreateWithCString(kCFAllocatorDefault,
                                                 sharedSecretData,
                                                 kCFStringEncodingASCII);
        SecKeychainItemFreeContent(NULL,sharedSecretData);
    }

    return sharedSecret;
}

/*! Updates the shared secret associated with a particular
 *  iSCSI node (either initiator or target) to the system keychain. An entry
 *  for the node is created if it does not exist. If it does exist, the shared
 *  secret for is updated.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @param sharedSecret the shared secret to store.
 *  @param return error code indicating the result of the operation. */
OSStatus iSCSIKeychainSetCHAPSecretForNode(CFStringRef nodeIQN,
                                           CFStringRef sharedSecret)
{
    SecKeychainSetPreferenceDomain(kSecPreferencesDomainSystem);
    SecKeychainSetUserInteractionAllowed(true);
    SecKeychainUnlock(NULL,0,NULL,false);

    OSStatus status;
    SecKeychainItemRef itemRef = NULL;

    SecKeychainFindGenericPassword(NULL,
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            0,0,&itemRef);

    // Update the secret if it exists; else create a new entry
    if(itemRef) {
        status = SecKeychainItemModifyContent(itemRef,0,
            (UInt32)CFStringGetLength(sharedSecret),
            CFStringGetCStringPtr(sharedSecret,kCFStringEncodingASCII));

        CFRelease(itemRef);
    }
    // Create a new item
    else {
        SecAccessRef accessRef;
        CFArrayRef trustedList;
        SecTrustedApplicationRef trustedApps[2];
        SecTrustedApplicationCreateFromPath("/usr/local/bin/iscsictl",&trustedApps[0]);
        SecTrustedApplicationCreateFromPath("/usr/local/libexec/iscsid",&trustedApps[1]);
        
        trustedList = CFArrayCreate(kCFAllocatorDefault,(const void **)trustedApps,2,&kCFTypeArrayCallBacks);
        
        status = SecAccessCreate(CFSTR("Description"),trustedList,&accessRef);
        
        // Get the system keychain and unlock it (prompts user if required)
        SecKeychainRef keychain;
        status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem,&keychain);

        const int nodeIQNlength = (int)CFStringGetLength(nodeIQN);
        char nodeIQNBuffer[nodeIQNlength];
        CFStringGetCString(nodeIQN,nodeIQNBuffer,NI_MAXHOST,kCFStringEncodingASCII);
        
        const char secItemDesc[] = "iSCSI CHAP Shared Secret";
        const int secItemDescLength = sizeof(secItemDesc)/sizeof(secItemDesc[0]);
        
        SecKeychainAttribute attrs[] = {
            { kSecLabelItemAttr, nodeIQNlength, nodeIQNBuffer },
            { kSecDescriptionItemAttr, secItemDescLength, (void*)secItemDesc },
            { kSecAccountItemAttr, nodeIQNlength, nodeIQNBuffer },
            { kSecServiceItemAttr, nodeIQNlength, nodeIQNBuffer }
        };
        
        SecKeychainAttributeList attrList = { sizeof(attrs)/sizeof(attrs[0]), attrs };
        
        const int secretLength = (int)CFStringGetLength(sharedSecret);
        char secretBuffer[secretLength];
        CFStringGetCString(sharedSecret,secretBuffer,secretLength,kCFStringEncodingASCII);
        
        SecKeychainItemRef itemRef;
        status = SecKeychainItemCreateFromContent(kSecGenericPasswordItemClass,
                                                  &attrList,
                                                  secretLength,
                                                  secretBuffer,
                                                  keychain,accessRef,&itemRef);
        if(keychain)
            CFRelease(keychain);
    }

    return status;
}

/*! Removes the shared secret associated with a particular
 *  iSCSI node (either initiator or target) from the system keychain. 
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @param return error code indicating the result of the operation. */
OSStatus iSCSIKeychainDeleteCHAPSecretForNode(CFStringRef nodeIQN)
{
    SecKeychainRef sysKeychain = NULL;
    OSStatus status;
    SecKeychainItemRef item = NULL;

    // Get the system keychain and unlock it (prompts user if required)
    SecKeychainSetPreferenceDomain(kSecPreferencesDomainSystem);

    status = SecKeychainFindGenericPassword(NULL,
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            0,0,&item);

    // Remove item from keychain
    if(status == errSecSuccess) {
        SecKeychainItemDelete(item);
    }

    if(sysKeychain)
        CFRelease(sysKeychain);

    return status;
}

/*! Gets whether a CHAP secret exists for the specified node.
 *  @param nodeIQN the node to test.
 *  @return true if a CHAP secret exists for the specified node. */
Boolean iSCSIKeychainContainsCHAPSecretForNode(CFStringRef nodeIQN)
{
    // Get the system keychain and unlock it (prompts user if required)
    SecKeychainSetPreferenceDomain(kSecPreferencesDomainSystem);

    SecKeychainItemRef item = NULL;

    CFStringRef keys[] = {
        kSecClass,
        kSecAttrAccount

    };

    CFStringRef values[] = {
        kSecClassGenericPassword,
        nodeIQN,
    };

    CFDictionaryRef query = CFDictionaryCreate(kCFAllocatorDefault,
                                               (void*)keys,(void*)values,
                                               sizeof(keys)/sizeof(void*),
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);

    SecItemCopyMatching(query,(CFTypeRef *)&item);

    return (item != NULL);
}

