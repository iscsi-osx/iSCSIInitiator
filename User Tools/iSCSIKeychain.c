/*!
 * @author		Nareg Sinenian
 * @file		iSCSIKeychain.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions that wrap around the
 *              security keychain to provide iSCSI key maangement
 */

#include "iSCSIKeychain.h"

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
    SecKeychainRef sysKeychain = NULL;
    OSStatus status;
    SecKeychainItemRef item = NULL;

    // Get the system keychain and unlock it (prompts user if required)
    SecKeychainSetPreferenceDomain(kSecPreferencesDomainSystem);

    SecKeychainFindGenericPassword(NULL,
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            0,0,&item);

    // Update the secret if it exists; else create a new entry
    if(item) {
        status = SecKeychainItemModifyContent(item,0,
            (UInt32)CFStringGetLength(sharedSecret),
            CFStringGetCStringPtr(sharedSecret,kCFStringEncodingASCII));

        CFRelease(item);
    }
    else {
        SecKeychainSetPreferenceDomain(kSecPreferencesDomainSystem);

        status = SecKeychainAddGenericPassword(NULL,
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(sharedSecret)+1,  // Include NULL character
            CFStringGetCStringPtr(sharedSecret,kCFStringEncodingASCII),NULL);

        if(sysKeychain)
            CFRelease(sysKeychain);
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

