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
        SecTrustedApplicationCreateFromPath("/Library/PrivilegedHelperTools/iscsid",&trustedApps[1]);
        
        trustedList = CFArrayCreate(kCFAllocatorDefault,(const void **)trustedApps,2,&kCFTypeArrayCallBacks);
        
        
        status = SecAccessCreate(CFSTR("Description"),trustedList,&accessRef);
        
        // Get the system keychain and unlock it (prompts user if required)
        SecKeychainRef keychain;
        status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem,&keychain);
        
        CFStringRef secItemLabel = nodeIQN;
        CFStringRef secItemDesc = CFSTR("iSCSI CHAP Shared Secret");
        CFStringRef secItemAcct = nodeIQN;
        CFStringRef secItemService = nodeIQN;
        
        
        SecKeychainAttribute attrs[] = {
            { kSecLabelItemAttr, (UInt32)CFStringGetLength(secItemLabel),(char *)CFStringGetCStringPtr(secItemLabel,kCFStringEncodingUTF8) },
            { kSecDescriptionItemAttr, (UInt32)CFStringGetLength(secItemDesc),(char *)CFStringGetCStringPtr(secItemDesc,kCFStringEncodingUTF8) },
            { kSecAccountItemAttr, (UInt32)CFStringGetLength(secItemAcct),(char *)CFStringGetCStringPtr(secItemAcct,kCFStringEncodingUTF8)     },
            { kSecServiceItemAttr, (UInt32)CFStringGetLength(secItemService),(char *)CFStringGetCStringPtr(secItemService,kCFStringEncodingUTF8) }
        };
        
        SecKeychainAttributeList attrList = { sizeof(attrs)/sizeof(attrs[0]), attrs };
        
        SecKeychainItemRef itemRef;
        status = SecKeychainItemCreateFromContent(kSecGenericPasswordItemClass,
                                                  &attrList,
                                                  (UInt32)CFStringGetLength(sharedSecret),
                                                  CFStringGetCStringPtr(sharedSecret,kCFStringEncodingASCII),
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

