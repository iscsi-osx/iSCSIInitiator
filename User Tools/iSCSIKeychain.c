/*!
 * @author		Nareg Sinenian
 * @file		iSCSIKeychain.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions that wrap around the
 *              security keychain to provide iSCSI key maangement
 */

#include "iSCSIKeychain.h"

/*! The iSCSI service name to use when storing CHAP information in the
 *  OS X keychain. */
CFStringRef kiSCSISecCHAPService = CFSTR("iSCSI CHAP");

/*! Copies a shared secret associated with a particular
 *  iSCSI node (either initiator or target) to the system keychain.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @return the shared secret for the specified node. */
CFStringRef iSCSIKeychainCopyCHAPSecretForNode(CFStringRef nodeIQN)
{
    SecKeychainRef sysKeychain = NULL;
    CFStringRef sharedSecret = NULL;
    OSStatus status;
    SecKeychainItemRef item = NULL;

    // Get the system keychain and unlock it (prompts user if required)
    status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem,&sysKeychain);

    if(status == errSecSuccess)
        status = SecKeychainUnlock(sysKeychain,0,NULL,false);

    UInt32 sharedSecretLength = 0;
    void * sharedSecretData = NULL;

    if(status == errSecSuccess) {
        SecKeychainFindGenericPassword(sysKeychain,
            (UInt32)CFStringGetLength(kiSCSISecCHAPService),
            CFStringGetCStringPtr(kiSCSISecCHAPService,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            &sharedSecretLength,
            &sharedSecretData,&item);

        sharedSecret = CFStringCreateWithCString(kCFAllocatorDefault,
                                                 sharedSecretData,
                                                 kCFStringEncodingASCII);
    }
    return sharedSecret;
}

/*! Updates the shared secret associated with a particular
 *  iSCSI node (either initiator or target) to the system keychain. An entry
 *  for the node is created if it does not exist. If it does exist, the shared
 *  secret for is updated.
 *  @param nodeIQN the iSCSI qualified name of the target or initiator.
 *  @param sharedSecret the shared secret to store. */
void iSCSIKeychainSetCHAPSecretForNode(CFStringRef nodeIQN,
                                          CFStringRef sharedSecret)
{
    SecKeychainRef sysKeychain = NULL;
    OSStatus status;
    SecKeychainItemRef item = NULL;

    // Get the system keychain and unlock it (prompts user if required)
    status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem,&sysKeychain);

    if(status == errSecSuccess)
        status = SecKeychainUnlock(sysKeychain,0,NULL,false);

    if(status == errSecSuccess) {
        status = SecKeychainFindGenericPassword(sysKeychain,
            (UInt32)CFStringGetLength(kiSCSISecCHAPService),
            CFStringGetCStringPtr(kiSCSISecCHAPService,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            0,0,&item);
    }

    // Update the secret if it exists; else create a new entry
    if(status == errSecSuccess) {
        status = SecKeychainItemModifyContent(item,0,
            (UInt32)CFStringGetLength(sharedSecret),
            CFStringGetCStringPtr(sharedSecret,kCFStringEncodingASCII));
    }
    else {
        SecKeychainItemRef item;
        SecKeychainAddGenericPassword(sysKeychain,
            (UInt32)CFStringGetLength(kiSCSISecCHAPService),
            CFStringGetCStringPtr(kiSCSISecCHAPService,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(nodeIQN),
            CFStringGetCStringPtr(nodeIQN,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(sharedSecret),
            CFStringGetCStringPtr(sharedSecret,kCFStringEncodingASCII),&item);
    }
}

/*! Renames the iSCSI node in they keychain.
 *  @param oldNodeIQN the old node iSCSI qualified name (IQN).
 *  @param newNodeIQN the new node iSCSI qualified name (IQN). */
void iSCSIKeychainRenameNode(CFStringRef oldNodeIQN,CFStringRef newNodeIQN)
{
    SecKeychainRef sysKeychain;
    OSStatus status;
    SecKeychainItemRef item = NULL;

    // Get the system keychain and unlock it (prompts user if required)
    status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem,&sysKeychain);

    if(status == errSecSuccess)
        status = SecKeychainUnlock(sysKeychain,0,NULL,false);

    UInt32 sharedSecretLength = 0;
    void * sharedSecretData = NULL;

    if(status == errSecSuccess) {
        status = SecKeychainFindGenericPassword(sysKeychain,
            (UInt32)CFStringGetLength(kiSCSISecCHAPService),
            CFStringGetCStringPtr(kiSCSISecCHAPService,kCFStringEncodingASCII),
            (UInt32)CFStringGetLength(oldNodeIQN),
            CFStringGetCStringPtr(oldNodeIQN,kCFStringEncodingASCII),
            &sharedSecretLength,&sharedSecretData,&item);
    }

    if(status == errSecSuccess) {

        SecKeychainAttribute attributes[] = {
            { kSecAccountItemAttr,
              (UInt32)CFStringGetLength(newNodeIQN),
              (void*)CFStringGetCStringPtr(newNodeIQN,kCFStringEncodingASCII) }
        };

        SecKeychainAttributeList attrList[] = { 1, attributes };

        status = SecKeychainItemModifyContent(item,attrList,0,NULL);
    }
}


