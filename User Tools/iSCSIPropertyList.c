/*!
 * @author		Nareg Sinenian
 * @file		iSCSIPropertyList.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions to read and write
 *              daemon configuration property list
 */

#include "iSCSIPropertyList.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPreferences.h>
#include <CoreFoundation/CFPropertyList.h>

/*! A cached version of the targets dictionary. */
CFMutableDictionaryRef targetsCache = NULL;

/*! Flag that indicates whether the targets cache was modified. */
Boolean targetNodesCacheModified = false;

/*! A cached version of the discovery dictionary. */
CFMutableDictionaryRef discoveryCache = NULL;

/*! Flag that indicates whether the discovery cache was modified. */
Boolean discoveryCacheModified = false;

/*! A cached version of the initiator dictionary. */
CFMutableDictionaryRef initiatorCache = NULL;

/*! Flag that indicates whether the initiator cache was modified. */
Boolean initiatorNodeCacheModified = false;

/*! App ID. */
CFStringRef kiSCSIPKAppId = CFSTR(CF_PREFERENCES_APP_ID);

/*! Preference key name for iSCSI initiator dictionary. */
CFStringRef kiSCSIPKInitiator = CFSTR("Initiator Node");

/*! Preference key name for iSCSI targets dictionary (holds all targets). */
CFStringRef kiSCSIPKTargets = CFSTR("Target Nodes");

/*! Preference key name for iSCSI discovery dictionary. */
CFStringRef kiSCSIPKDiscovery = CFSTR("Discovery");



/*! Preference key name for iSCSI portals dictionary (specific to each target). */
CFStringRef kiSCSIPKPortals = CFSTR("Portals");

/*! Preference key name for target configuration type. */
CFStringRef kiSCSIPKTargetConfigType = CFSTR("Configuration Type");

/*! Preference key value for static target configuration. */
CFStringRef kiSCSIPVTargetConfigTypeStatic = CFSTR("Static");

/*! Preference key value for discovery target configuration. */
CFStringRef kiSCSIPVTargetConfigTypeDiscovery = CFSTR("SendTargets");

/*! Preference key name for error recovery level. */
CFStringRef kiSCSIPKErrorRecoveryLevel = CFSTR("Error Recovery Level");

/*! Preference key name for maximum number of connections. */
CFStringRef kiSCSIPKMaxConnections = CFSTR("Maximum Connections");

/*! Preference key name for data digest. */
CFStringRef kiSCSIPKDataDigest = CFSTR("Data Digest");

/*! Preference key name for header digest. */
CFStringRef kiSCSIPKHeaderDigest = CFSTR("Header Digest");

/*! Preference key value for digest. */
CFStringRef kiSCSIPVDigestNone = CFSTR("None");

/*! Preference key value for digest. */
CFStringRef kiSCSIPVDigestCRC32C = CFSTR("CRC32C");

/*! Preference key name for iSCSI authentication. */
CFStringRef kiSCSIPKAuth = CFSTR("Authentication");

/*! Preference key value for no authentication. */
CFStringRef kiSCSIPVAuthNone = CFSTR("None");

/*! Preference key value for CHAP authentication. */
CFStringRef kiSCSIPVAuthCHAP = CFSTR("CHAP");

/*! Preference key name for iSCSI CHAP authentication name. */
CFStringRef kiSCSIPKAuthCHAPName = CFSTR("CHAP Name");

/*! Preference key name for portal host interface key. */
CFStringRef kiSCSIPKPortalHostInterface = CFSTR("Host Interface");

/*! Preference key name for portal host interface key. */
CFStringRef kiSCSIPKPortalPort = CFSTR("Port");

/*! Preference key for array of targets associated with a discovery portal. */
CFStringRef kiSCSIPKDiscoveryTargetsForPortal = CFSTR("Targets");


/*! Preference key name for iSCSI initiator name. */
CFStringRef kiSCSIPKInitiatorIQN = CFSTR("Name");

/*! Preference key name for iSCSI initiator alias. */
CFStringRef kiSCSIPKInitiatorAlias = CFSTR("Alias");

/*! Default initiator alias to use. */
CFStringRef kiSCSIPVDefaultInitiatorAlias = CFSTR("localhost");

/*! Default initiator IQN to use. */
CFStringRef kiSCSIPVDefaultInitiatorIQN = CFSTR("iqn.2015-01.com.localhost:initiator");


/*! Preference key name for iSCSI discovery portals key. */
CFStringRef kiSCSIPKDiscoveryPortals = CFSTR("Portals");


/*! Preference key name for iSCSI discovery enabled/disabled. */
CFStringRef kiSCSIPKSendTargetsEnabled = CFSTR("SendTargets");

/*! Preference key name for iSCSI discovery interval. */
CFStringRef kiSCSIPKDiscoveryInterval = CFSTR("Interval");



/*! Retrieves a mutable dictionary for the specified key.
 *  @param key the name of the key, which can be either kiSCSIPKTargets,
 *  kiSCSIPKDiscovery, or kiSCSIPKInitiator.
 *  @return mutable dictionary with list of properties for the specified key. */
CFMutableDictionaryRef iSCSIPLCopyPropertyDict(CFStringRef key)
{
    // Retrieve the desired property from the property list
    CFDictionaryRef propertyList = CFPreferencesCopyValue(key,kiSCSIPKAppId,
                                                          kCFPreferencesAnyUser,
                                                          kCFPreferencesCurrentHost);
    if(!propertyList)
        return NULL;
    
    // Create a deep copy to make the dictionary mutable
    CFMutableDictionaryRef mutablePropertyList = (CFMutableDictionaryRef)CFPropertyListCreateDeepCopy(
        kCFAllocatorDefault,
        propertyList,
        kCFPropertyListMutableContainersAndLeaves);
    
    // Release original retrieved property
    CFRelease(propertyList);
    return mutablePropertyList;
}

/*! Creates a mutable dictionary for the targets key.
 *  @return a mutable dictionary for the targets key. */
CFMutableDictionaryRef iSCSIPLCreateTargetsDict()
{
    return CFDictionaryCreateMutable(kCFAllocatorDefault,
                                     0,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
}


/*! Creates a mutable dictionary for the discovery key.
 *  @return a mutable dictionary for the discovery key. */
CFMutableDictionaryRef iSCSIPLCreateDiscoveryDict()
{
    CFMutableDictionaryRef discoveryDict = CFDictionaryCreateMutable(
        kCFAllocatorDefault,0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    // Default scan interval (0 indicates never)
    CFIndex interval = 0;
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberCFIndexType,&interval);

    CFDictionaryAddValue(discoveryDict,kiSCSIPKSendTargetsEnabled,kCFBooleanFalse);
    CFDictionaryAddValue(discoveryDict,kiSCSIPKDiscoveryInterval,value);

    CFRelease(value);

    return discoveryDict;
}

/*! Creates a mutable dictionary for the initiator key.
 *  @return a mutable dictionary for the initiator key. */
CFMutableDictionaryRef iSCSIPLCreateInitiatorDict()
{
    CFMutableDictionaryRef initiatorDict = CFDictionaryCreateMutable(
        kCFAllocatorDefault,0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    CFDictionaryAddValue(initiatorDict,kiSCSIPKAuthCHAPName,kiSCSIPVDefaultInitiatorAlias);
    CFDictionaryAddValue(initiatorDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    CFDictionaryAddValue(initiatorDict,kiSCSIPKInitiatorAlias,kiSCSIPVDefaultInitiatorAlias);
    CFDictionaryAddValue(initiatorDict,kiSCSIPKInitiatorIQN,kiSCSIPVDefaultInitiatorIQN);

    return initiatorDict;
}

CFMutableDictionaryRef iSCSIPLCreateTargetDict()
{
    CFNumberRef maxConnections = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&kRFC3720_MaxConnections);
    CFNumberRef errorRecoveryLevel = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&kRFC3720_ErrorRecoveryLevel);

    CFMutableDictionaryRef targetDict = CFDictionaryCreateMutable(
        kCFAllocatorDefault,0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    CFDictionaryAddValue(targetDict,kiSCSIPKAuthCHAPName,(void *)CFSTR(""));
    CFDictionaryAddValue(targetDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    CFDictionaryAddValue(targetDict,kiSCSIPKMaxConnections,maxConnections);
    CFDictionaryAddValue(targetDict,kiSCSIPKErrorRecoveryLevel,errorRecoveryLevel);
    CFDictionaryAddValue(targetDict,kiSCSIPKHeaderDigest,kiSCSIPVDigestNone);
    CFDictionaryAddValue(targetDict,kiSCSIPKDataDigest,kiSCSIPVDigestNone);

    CFRelease(maxConnections);
    CFRelease(errorRecoveryLevel);

    return targetDict;
}

CFMutableDictionaryRef iSCSIPLGetInitiatorDict(Boolean createIfMissing)
{
    if(createIfMissing && !initiatorCache)
        initiatorCache = iSCSIPLCreateInitiatorDict();

    return initiatorCache;
}

CFMutableDictionaryRef iSCSIPLGetDiscoveryDict(Boolean createIfMissing)
{
    if(createIfMissing && !discoveryCache)
        discoveryCache = iSCSIPLCreateDiscoveryDict();

    return discoveryCache;
}

/*! Helper function. Retrieves the iSCSI discovery portals dictionary. */
CFMutableDictionaryRef iSCSIPLGetSendTargetsDiscoveryPortals(Boolean createIfMissing)
{
    // Get discovery dictionary
    CFMutableDictionaryRef discoveryDict = iSCSIPLGetDiscoveryDict(createIfMissing);

    if(discoveryDict)
    {
        if(createIfMissing &&
           CFDictionaryGetCountOfKey(discoveryDict,kiSCSIPKDiscoveryPortals) == 0)
        {
            CFMutableDictionaryRef discoveryPortalsDict = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,
                &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks);

            CFDictionarySetValue(discoveryDict,
                                 kiSCSIPKDiscoveryPortals,
                                 discoveryPortalsDict);
            CFRelease(discoveryPortalsDict);
        }
        return (CFMutableDictionaryRef)CFDictionaryGetValue(discoveryDict,kiSCSIPKDiscoveryPortals);
    }
    return NULL;
}

CFArrayRef iSCSIPLGetDynamicTargetsForSendTargets(CFStringRef portalAddress,
                                                  Boolean createIfMissing)
{
    CFArrayRef targetsList = NULL;
    CFMutableDictionaryRef discoveryPortals = iSCSIPLGetSendTargetsDiscoveryPortals(false);

    if(discoveryPortals && portalAddress) {

        CFMutableDictionaryRef portalDict = NULL;
        if(CFDictionaryGetValueIfPresent(discoveryPortals,portalAddress,(void*)&portalDict)) {
            if(!CFDictionaryGetValueIfPresent(portalDict,kiSCSIPKDiscoveryTargetsForPortal,(void*)&targetsList)) {
                targetsList = CFArrayCreateMutable(kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
                CFDictionarySetValue(portalDict,kiSCSIPKDiscoveryTargetsForPortal,targetsList);
                CFRelease(targetsList);
            }
        }
    }
    return targetsList;
}


CFMutableDictionaryRef iSCSIPLGetTargets(Boolean createIfMissing)
{
    if(createIfMissing && !targetsCache)
        targetsCache = iSCSIPLCreateTargetsDict();

    return targetsCache;
}

CFMutableDictionaryRef iSCSIPLGetTargetDict(CFStringRef targetIQN,
                                            Boolean createIfMissing)
{
    // Get list of targets
    CFMutableDictionaryRef targetsList = iSCSIPLGetTargets(createIfMissing);
    
    if(targetsList)
    {
        if(createIfMissing && CFDictionaryGetCountOfKey(targetsList,targetIQN) == 0)
        {
            CFMutableDictionaryRef targetDict = iSCSIPLCreateTargetDict();
            CFDictionarySetValue(targetsList,targetIQN,targetDict);
            CFRelease(targetDict);
        }
        
        return (CFMutableDictionaryRef)CFDictionaryGetValue(targetsList,targetIQN);
    }
    return NULL;
}

CFMutableDictionaryRef iSCSIPLGetPortalsList(CFStringRef targetIQN,
                                             Boolean createIfMissing)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,createIfMissing);

    
    if(targetDict)
    {
        if(createIfMissing && CFDictionaryGetCountOfKey(targetDict,kiSCSIPKPortals) == 0)
        {
            CFMutableDictionaryRef portalsList = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,
                &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks);
            
            CFDictionarySetValue(targetDict,kiSCSIPKPortals,portalsList);
            CFRelease(portalsList);
        }
        return (CFMutableDictionaryRef)CFDictionaryGetValue(targetDict,kiSCSIPKPortals);
    }
    return NULL;
}

/*! Sets the maximum number of connections for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param maxConnections the maximum number of connections. */
void iSCSIPLSetMaxConnectionsForTarget(CFStringRef targetIQN,
                                       UInt32 maxConnections)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&maxConnections);
    CFDictionarySetValue(targetDict,kiSCSIPKMaxConnections,value);

    targetNodesCacheModified = true;
}

/*! Sets the error recovery level to use for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param level the error recovery level. */
void iSCSIPLSetErrorRecoveryLevelForTarget(CFStringRef targetIQN,
                                           enum iSCSIErrorRecoveryLevels level)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&level);
    CFDictionarySetValue(targetDict,kiSCSIPKErrorRecoveryLevel,value);

    targetNodesCacheModified = true;
}

/*! Gets the maximum number of connections for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the maximum number of connections for the target. */
UInt32 iSCSIPLGetMaxConnectionsForTarget(CFStringRef targetIQN)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);
    CFNumberRef value = CFDictionaryGetValue(targetDict,kiSCSIPKErrorRecoveryLevel);

    UInt32 maxConnections = kRFC3720_MaxConnections;
    CFNumberGetValue(value,kCFNumberIntType,&maxConnections);
    return maxConnections;
}

/*! Gets the error recovery level to use for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the error recovery level. */
enum iSCSIErrorRecoveryLevels iSCSIPLGetErrorRecoveryLevelForTarget(CFStringRef targetIQN)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);
    CFNumberRef value = CFDictionaryGetValue(targetDict,kiSCSIPKErrorRecoveryLevel);

    enum iSCSIErrorRecoveryLevels errorRecoveryLevel = kRFC3720_ErrorRecoveryLevel;
    CFNumberGetValue(value,kCFNumberIntType,&errorRecoveryLevel);
    return errorRecoveryLevel;
}

iSCSIPortalRef iSCSIPLCopyPortalForTarget(CFStringRef targetIQN,
                                          CFStringRef portalAddress)
{
    // Get list of portals for this target
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);

    iSCSIPortalRef portal = NULL;

    if(portalsList) {
        CFMutableDictionaryRef portalDict =
            (CFMutableDictionaryRef)CFDictionaryGetValue(portalsList,
                                                         portalAddress);
        if(portalDict)
            portal = iSCSIPortalCreateWithDictionary(portalDict);
    }
    return portal;
}

iSCSITargetRef iSCSIPLCopyTarget(CFStringRef targetIQN)
{
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetIQN(target,targetIQN);

    return target;
}

enum iSCSIDigestTypes iSCSIPLGetDataDigestForTarget(CFStringRef targetIQN)
{
    // Get the dictionary containing information about the target
    CFDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    enum iSCSIDigestTypes digestType = kiSCSIDigestInvalid;

    if(targetDict) {
        CFStringRef digest = CFDictionaryGetValue(targetDict,kiSCSIPKDataDigest);

        if(digest) {

            if(CFStringCompare(digest,kiSCSIPVDigestNone,0) == kCFCompareEqualTo)
                digestType = kiSCSIDigestNone;
            else if(CFStringCompare(digest,kiSCSIPVDigestCRC32C,0) == kCFCompareEqualTo)
                digestType = kiSCSIDigestCRC32C;
        }
    }
    return digestType;
}

void iSCSIPLSetDataDigestForTarget(CFStringRef targetIQN,enum iSCSIDigestTypes digestType)
{
    // Get the dictionary containing information about the target
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    if(targetDict)
    {
        CFStringRef digest = NULL;

        switch(digestType)
        {
            case kiSCSIDigestNone: digest = kiSCSIPVDigestNone; break;
            case kiSCSIDigestCRC32C: digest = kiSCSIPVDigestCRC32C; break;
            case kiSCSIDigestInvalid: break;
        };

        if(digest)
            CFDictionarySetValue(targetDict,kiSCSIPKDataDigest,digest);
    }
}

enum iSCSIDigestTypes iSCSIPLGetHeaderDigestForTarget(CFStringRef targetIQN)
{
    // Get the dictionary containing information about the target
    CFDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    enum iSCSIDigestTypes digestType = kiSCSIDigestInvalid;

    if(targetDict) {
        CFStringRef digest = CFDictionaryGetValue(targetDict,kiSCSIPKHeaderDigest);

        if(digest) {

            if(CFStringCompare(digest,kiSCSIPVDigestNone,0) == kCFCompareEqualTo)
                digestType = kiSCSIDigestNone;
            else if(CFStringCompare(digest,kiSCSIPVDigestCRC32C,0) == kCFCompareEqualTo)
                digestType = kiSCSIDigestCRC32C;
        }
    }
    return digestType;
}

void iSCSIPLSetHeaderDigestForTarget(CFStringRef targetIQN,enum iSCSIDigestTypes digestType)
{
    // Get the dictionary containing information about the target
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    if(targetDict)
    {
        CFStringRef digest = NULL;

        switch(digestType)
        {
            case kiSCSIDigestNone: digest = kiSCSIPVDigestNone; break;
            case kiSCSIDigestCRC32C: digest = kiSCSIPVDigestCRC32C; break;
            case kiSCSIDigestInvalid: break;
        };

        if(digest)
            CFDictionarySetValue(targetDict,kiSCSIPKHeaderDigest,digest);
    }
}

/*! Sets authentication method to be used by initiator. */
void iSCSIPLSetInitiatorAuthenticationMethod(enum iSCSIAuthMethods authMethod)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);

    if(authMethod == kiSCSIAuthMethodNone)
        CFDictionarySetValue(initiatorDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    else if(authMethod == kiSCSIAuthMethodCHAP)
        CFDictionarySetValue(initiatorDict,kiSCSIPKAuth,kiSCSIPVAuthCHAP);

    initiatorNodeCacheModified = true;
}

/*! Gets the current authentication method used by the initiator. */
enum iSCSIAuthMethods iSCSIPLGetInitiatorAuthenticationMethod()
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);
    CFStringRef auth = CFDictionaryGetValue(initiatorDict,kiSCSIPKAuth);
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;

    if(CFStringCompare(auth,kiSCSIPVAuthNone,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodNone;
    else if(CFStringCompare(auth,kiSCSIPVAuthCHAP,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodCHAP;

    return authMethod;
}

/*! Sets the CHAP name associated with the initiator. */
void iSCSIPLSetInitiatorCHAPName(CFStringRef name)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);

    // Change CHAP name in property list
    CFDictionarySetValue(initiatorDict,kiSCSIPKAuthCHAPName,name);

    initiatorNodeCacheModified = true;
}

/*! Copies the CHAP name associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPName()
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);
    CFStringRef name = CFDictionaryGetValue(initiatorDict,kiSCSIPKAuthCHAPName);
    return CFStringCreateCopy(kCFAllocatorDefault,name);
}

/*! Sets the CHAP secret associated with the initiator. */
void iSCSIPLSetInitiatorCHAPSecret(CFStringRef secret)
{
    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    iSCSIKeychainSetCHAPSecretForNode(initiatorIQN,secret);
    CFRelease(initiatorIQN);

    initiatorNodeCacheModified = true;
}

/*! Copies the CHAP secret associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPSecret()
{
    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    CFStringRef secret = iSCSIKeychainCopyCHAPSecretForNode(initiatorIQN);
    CFRelease(initiatorIQN);
    return secret;
}

void iSCSIPLSetPortalForTarget(CFStringRef targetIQN,
                               iSCSIPortalRef portal)
{
    // Get list of portals for this target (only if it exists, otherwise NULL)
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);

    if(portal && portalsList) {
        CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFDictionarySetValue(portalsList,portalAddress,portalDict);
        CFRelease(portalDict);

        targetNodesCacheModified = true;
    }
}

void iSCSIPLRemovePortalForTarget(CFStringRef targetIQN,
                                  CFStringRef portalAddress)
{
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);
    
    if(portalsList) {
        CFDictionaryRemoveValue(portalsList,portalAddress);
        targetNodesCacheModified = true;
    }
}

/*! Adds a target object with a specified portal.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set. */
void iSCSIPLAddStaticTarget(CFStringRef targetIQN,iSCSIPortalRef portal)
{
    // Only proceed if the target does not exist; otherwise do nothing
    if(!iSCSIPLContainsTarget(targetIQN)) {

        // Create list of target portals
        CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,true);

        // Add portal to newly create list of portals for this target
        CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFDictionarySetValue(portalsList,portalAddress,portalDict);
        CFRelease(portalDict);

        // Set target config
        iSCSIPLSetTargetConfigType(targetIQN,kiSCSITargetConfigStatic);

        targetNodesCacheModified = true;
    }
}

/*! Adds a target object with a specified portal, and associates it with
 *  a particular SendTargets portal that manages the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set.
 *  @param sendTargetsPortal the discovery portal address with which to
 *  associate the managed target. */
void iSCSIPLAddDynamicTargetForSendTargets(CFStringRef targetIQN,
                                           iSCSIPortalRef portal,
                                           CFStringRef sendTargetsPortal)
{
    // Only proceed if the target does not exist; otherwise do nothing
    if(!iSCSIPLContainsTarget(targetIQN)) {

        // Create list of target portals
        CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,true);

        // Add portal to newly create list of portals for this target
        CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFDictionarySetValue(portalsList,portalAddress,portalDict);
        CFRelease(portalDict);

        // Set target config
        iSCSIPLSetTargetConfigType(targetIQN,kiSCSITargetConfigDynamicSendTargets);

        // Associate target with the specified iSCSI discovery portal
        CFMutableArrayRef targetList = (CFMutableArrayRef)iSCSIPLGetDynamicTargetsForSendTargets(portalAddress,true);
        CFArrayAppendValue(targetList,targetIQN);

        targetNodesCacheModified = true;
        discoveryCacheModified = true;
    }
}

void iSCSIPLRemoveTarget(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetsList = iSCSIPLGetTargets(false);
    
    if(targetsList) {
        CFDictionaryRemoveValue(targetsList,targetIQN);
        targetNodesCacheModified = true;
    }
}

/*! Copies the initiator name from the iSCSI property list to a CFString object.
 *  @return the initiator name. */
CFStringRef iSCSIPLCopyInitiatorIQN()
{
    if(!initiatorCache)
        return NULL;
    
    // Lookup and copy the initiator name from the dictionary
    CFStringRef initiatorIQN = CFStringCreateCopy(
        kCFAllocatorDefault,CFDictionaryGetValue(initiatorCache,kiSCSIPKInitiatorIQN));
    
    return initiatorIQN;
}

/*! Sets the initiator name in the iSCSI property list.
 *  @param initiatorIQN the initiator name to set. */
void iSCSIPLSetInitiatorIQN(CFStringRef initiatorIQN)
{
    if(!initiatorCache)
        initiatorCache = iSCSIPLCreateInitiatorDict();
    
    // Update initiator name
    CFDictionarySetValue(initiatorCache,kiSCSIPKInitiatorIQN,initiatorIQN);

    // Update keychain if necessary
    CFStringRef oldInitiatorIQN = iSCSIPLCopyInitiatorIQN();
    iSCSIKeychainRenameNode(oldInitiatorIQN,initiatorIQN);

    initiatorNodeCacheModified = true;
}

/*! Copies the initiator alias from the iSCSI property list to a CFString object.
 *  @return the initiator alias. */
CFStringRef iSCSIPLCopyInitiatorAlias()
{
    if(!initiatorCache)
        return NULL;
    
    // Lookup and copy the initiator alias from the dictionary
    CFStringRef initiatorAlias = CFStringCreateCopy(
        kCFAllocatorDefault,CFDictionaryGetValue(initiatorCache,kiSCSIPKInitiatorAlias));
    
    return initiatorAlias;
}

/*! Sets the initiator alias in the iSCSI property list.
 *  @param initiatorAlias the initiator alias to set. */
void iSCSIPLSetInitiatorAlias(CFStringRef initiatorAlias)
{
    if(!initiatorCache)
        initiatorCache = iSCSIPLCreateInitiatorDict();
    
    // Update initiator alias
    CFDictionarySetValue(initiatorCache,kiSCSIPKInitiatorAlias,initiatorAlias);

    initiatorNodeCacheModified = true;
}

/*! Gets whether a target is defined in the property list.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPLContainsTarget(CFStringRef targetIQN)
{
    CFDictionaryRef targetsList = iSCSIPLGetTargets(false);
    return CFDictionaryContainsKey(targetsList,targetIQN);
}

/*! Gets whether a portal is defined in the property list.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortalForTarget(CFStringRef targetIQN,
                                       CFStringRef portalAddress)
{
    CFDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);
    return (portalsList && CFDictionaryContainsKey(portalsList,portalAddress));
}

/*! Gets whether a SendTargets discovery portal is defined in the property list.
 *  @param portalAddress the discovery portal address.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortalForSendTargetsDiscovery(CFStringRef portalAddress)
{
    Boolean exists = false;
    CFDictionaryRef discoveryPortals = iSCSIPLGetSendTargetsDiscoveryPortals(false);

    if(discoveryPortals)
        exists = CFDictionaryContainsKey(discoveryPortals,portalAddress);

    return exists;
}

/*! Creates an array of target iSCSI qualified name (IQN)s
 *  defined in the property list.
 *  @return an array of target iSCSI qualified name (IQN)s. */
CFArrayRef iSCSIPLCreateArrayOfTargets()
{
    CFDictionaryRef targetsList = iSCSIPLGetTargets(false);
    
    if(!targetsList)
        return NULL;

    const CFIndex keyCount = CFDictionaryGetCount(targetsList);
    
    if(keyCount == 0)
        return NULL;
  
    const void * keys[keyCount];
    CFDictionaryGetKeysAndValues(targetsList,keys,NULL);
    
    return CFArrayCreate(kCFAllocatorDefault,keys,keyCount,&kCFTypeArrayCallBacks);
}

/*! Creates an array of statically configured iSCSI targets (IQNs).
 *  @return an array of statically configured iSCSI targets (IQNs). */
CFArrayRef iSCSIPLCreateArrayOfStaticTargets()
{
    return NULL;
}

/*! Creates an array of iSCSI targets (IQNs) that were dynamically configured
 *  using SendTargets over a specific discovery portal.
 *  @param portalAddress the address of the discovery portal that corresponds
 *  to the dynamically configured discovery targets.
 *  @return an array of iSCSI targets (IQNs) that were discovered using
 *  SendTargets over the specified portal. */
CFArrayRef iSCISPLCreateArrayOfDynamicTargetsForSendTargets(CFStringRef portalAddress)
{
    CFArrayRef targetsList = iSCSIPLGetDynamicTargetsForSendTargets(portalAddress,false);

    // Create a copy to return
    if(targetsList)
        targetsList = CFArrayCreateCopy(kCFAllocatorDefault,targetsList);

    return targetsList;
}

/*! Creates an array of portal names for a given target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPLCreateArrayOfPortalsForTarget(CFStringRef targetIQN)
{
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);
    
    if(!portalsList)
        return NULL;
    
    const CFIndex keyCount = CFDictionaryGetCount(portalsList);
    
    if(keyCount == 0)
        return NULL;
    
    const void * keys[keyCount];
    CFDictionaryGetKeysAndValues(portalsList,keys,NULL);
    
    return CFArrayCreate(kCFAllocatorDefault,keys,keyCount,&kCFTypeArrayCallBacks);
}


/*! Modifies the target IQN for the specified target.
 *  @param existingIQN the IQN of the existing target to modify.
 *  @param newIQN the new IQN to assign to the target. */
void iSCSIPLSetTargetIQN(CFStringRef existingIQN,CFStringRef newIQN)
{
    CFMutableDictionaryRef targetNodes = iSCSIPLGetTargets(false);
    CFMutableDictionaryRef target = iSCSIPLGetTargetDict(existingIQN,false);

    if(target && targetNodes)
    {
        CFDictionarySetValue(targetNodes,newIQN,target);
        CFDictionaryRemoveValue(targetNodes,existingIQN);

        // Rename keychain if required
        iSCSIKeychainRenameNode(existingIQN,newIQN);

        targetNodesCacheModified = true;
    }
}

/*! Sets authentication method to be used by target. */
void iSCSIPLSetTargetAuthenticationMethod(CFStringRef targetIQN,
                                          enum iSCSIAuthMethods authMethod)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);

    if(authMethod == kiSCSIAuthMethodNone)
        CFDictionarySetValue(targetDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    else if(authMethod == kiSCSIAuthMethodCHAP)
        CFDictionarySetValue(targetDict,kiSCSIPKAuth,kiSCSIPVAuthCHAP);

    targetNodesCacheModified = true;
}

/*! Gets the current authentication method used by the target. */
enum iSCSIAuthMethods iSCSIPLGetTargetAuthenticationMethod(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);
    CFStringRef auth = CFDictionaryGetValue(targetDict,kiSCSIPKAuth);
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;

    if(CFStringCompare(auth,kiSCSIPVAuthNone,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodNone;
    else if(CFStringCompare(auth,kiSCSIPVAuthCHAP,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodCHAP;

    return authMethod;
}

/*! Sets target configuration type.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param configType the target configuration type. */
void iSCSIPLSetTargetConfigType(CFStringRef targetIQN,
                                enum iSCSITargetConfigTypes configType)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);
    CFStringRef configTypeString = NULL;

    switch(configType) {

        case kiSCSITargetConfigStatic:
            configTypeString = kiSCSIPVTargetConfigTypeStatic; break;
        case kiSCSITargetConfigDynamicSendTargets:
            configTypeString = kiSCSIPVTargetConfigTypeDiscovery; break;
        case kiSCSITargetConfigInvalid: break;
    };

    if(configTypeString) {
        // Change CHAP name in property list
        CFDictionarySetValue(targetDict,kiSCSIPKTargetConfigType,configTypeString);
        targetNodesCacheModified = true;
    }
}

/*! Gets target configuration type.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the target configuration type. */
enum iSCSITargetConfigTypes iSCSIPLGetTargetConfigType(CFStringRef targetIQN)
{
    enum iSCSITargetConfigTypes configType = kiSCSITargetConfigInvalid;
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    if(targetDict) {
        CFStringRef configTypeString = CFDictionaryGetValue(targetDict,kiSCSIPKTargetConfigType);

        // If target exists but configuration string doesn't, assume it is
        // a static target and repair the property list
        if(configTypeString == NULL) {
            configType = kiSCSITargetConfigStatic;
            CFDictionarySetValue(targetDict,kiSCSIPKTargetConfigType,kiSCSIPVTargetConfigTypeStatic);
        }
        else {

            if(CFStringCompare(configTypeString,kiSCSIPVTargetConfigTypeStatic,0) == kCFCompareEqualTo)
                configType = kiSCSITargetConfigStatic;
            else if(CFStringCompare(configTypeString,kiSCSIPVTargetConfigTypeDiscovery,0) == kCFCompareEqualTo)
                configType = kiSCSITargetConfigDynamicSendTargets;
        }
    }
    return configType;
}

/*! Sets the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param name the CHAP name associated with the target. */
void iSCSIPLSetTargetCHAPName(CFStringRef targetIQN,CFStringRef name)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);

    // Change CHAP name in property list
    CFDictionarySetValue(targetDict,kiSCSIPKAuthCHAPName,name);

    targetNodesCacheModified = true;
}

/*! Copies the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP name associated with the target. */
CFStringRef iSCSIPLCopyTargetCHAPName(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);
    CFStringRef name = CFDictionaryGetValue(targetDict,kiSCSIPKAuthCHAPName);
    return CFStringCreateCopy(kCFAllocatorDefault,name);
}

/*! Sets the CHAP secret associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param secret the CHAP shared secret associated with the target. */
void iSCSIPLSetTargetCHAPSecret(CFStringRef targetIQN,CFStringRef secret)
{
    iSCSIKeychainSetCHAPSecretForNode(targetIQN,secret);
}

/*! Copies the CHAP secret associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP shared secret associated with the target. */
CFStringRef iSCSIPLCopyTargetCHAPSecret(CFStringRef targetIQN)
{
    return iSCSIKeychainCopyCHAPSecretForNode(targetIQN);
}

/*! Adds an iSCSI discovery portal to the list of discovery portals.
 *  @param portal the discovery portal to add. */
void iSCSIPLAddSendTargetsDiscoveryPortal(iSCSIPortalRef portal)
{
    CFMutableDictionaryRef discoveryPortals = iSCSIPLGetSendTargetsDiscoveryPortals(true);

    if(portal && discoveryPortals) {

        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

        // Add portal if it doesn't exist
        if(!CFDictionaryContainsKey(discoveryPortals,portalAddress)) {

            CFStringRef port = iSCSIPortalGetPort(portal);
            CFStringRef interface = iSCSIPortalGetHostInterface(portal);

            CFMutableDictionaryRef portalDict = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,
                &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks);

            CFDictionarySetValue(portalDict,kiSCSIPKPortalPort,port);
            CFDictionarySetValue(portalDict,kiSCSIPKPortalHostInterface,interface);

            // Create an array to hold targets associated with this portal
            CFMutableArrayRef targets = CFArrayCreateMutable(
                kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);

            CFDictionarySetValue(portalDict,kiSCSIPKDiscoveryTargetsForPortal,targets);

            CFDictionarySetValue(discoveryPortals,portalAddress,portalDict);
            CFRelease(targets);
            CFRelease(portalDict);

            discoveryCacheModified = true;
        }
    }
}

/*! Removes the specified iSCSI discovery portal.
 *  @param portal the discovery portal to remove. */
void iSCSIPLRemoveSendTargetsDiscoveryPortal(iSCSIPortalRef portal)
{
    CFMutableDictionaryRef discoveryPortals = iSCSIPLGetSendTargetsDiscoveryPortals(false);

    if(discoveryPortals && portal) {

        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

        if(CFDictionaryContainsKey(discoveryPortals,portalAddress)) {

            // Remove all dynamic targets associated with this portal, if any...
            CFArrayRef targetsList = iSCSIPLGetDynamicTargetsForSendTargets(portalAddress,false);

            if(targetsList) {
                CFIndex count = CFArrayGetCount(targetsList);

                for(CFIndex idx = 0; idx < count; idx++)
                {
                    CFStringRef targetIQN = CFArrayGetValueAtIndex(targetsList,idx);
                    iSCSIPLRemoveTarget(targetIQN);
                }

                targetNodesCacheModified = true;
            }

            // Remove discovery portal from SendTargets dictionary
            CFDictionaryRemoveValue(discoveryPortals,portalAddress);
            discoveryCacheModified = true;
        }
    }
}

/*! Copies a portal object for the specified discovery portal.
 *  @param poralAddress the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPLCopySendTargetsDiscoveryPortal(CFStringRef portalAddress)
{
    iSCSIMutablePortalRef portal = NULL;

    CFMutableDictionaryRef discoveryPortals = iSCSIPLGetSendTargetsDiscoveryPortals(false);

    if(discoveryPortals) {
        // The portal dictionary consists of portal information and associated
        // targets; we want to extract portal information only
        CFMutableDictionaryRef portalDict = NULL;

        if(CFDictionaryGetValueIfPresent(discoveryPortals,portalAddress,(void*)&portalDict))
        {
            portal = iSCSIPortalCreateMutable();
            iSCSIPortalSetAddress(portal,portalAddress);

            CFStringRef port = kiSCSIDefaultPort;
            CFStringRef interface = kiSCSIDefaultHostInterface;

            CFDictionaryGetValueIfPresent(portalDict,kiSCSIPKPortalPort,(void*)&port);
            CFDictionaryGetValueIfPresent(portalDict,kiSCSIPKPortalHostInterface,(void*)&interface);

            // Set portal port and host interface
            iSCSIPortalSetPort(portal,port);
            iSCSIPortalSetHostInterface(portal,interface);
        }
    }
    return portal;
}

/*! Creates a list of SendTargets portals.  Each element of the array is
 *  an iSCSI discovery portal address that can be used to retrieve the
 *  corresponding portal object by calling iSCSIPLCopySendTargetsPortal(). */
CFArrayRef iSCSIPLCreateArrayOfPortalsForSendTargetsDiscovery()
{
    CFMutableDictionaryRef discoveryPortals = iSCSIPLGetSendTargetsDiscoveryPortals(false);

    if(!discoveryPortals)
        return NULL;

    const CFIndex keyCount = CFDictionaryGetCount(discoveryPortals);

    if(keyCount == 0)
        return NULL;

    const void * keys[keyCount];
    CFDictionaryGetKeysAndValues(discoveryPortals,keys,NULL);

    return CFArrayCreate(kCFAllocatorDefault,keys,keyCount,&kCFTypeArrayCallBacks);
}

/*! Sets SendTargets discovery to enabled or disabled.
 *  @param enable True to set send targets discovery enabled, false otherwise. */
void iSCSIPLSetSendTargetsDiscoveryEnable(Boolean enable)
{
    CFMutableDictionaryRef discoveryDict = iSCSIPLGetDiscoveryDict(true);

    if(enable)
        CFDictionarySetValue(discoveryDict,kiSCSIPKSendTargetsEnabled,kCFBooleanTrue);
    else
        CFDictionarySetValue(discoveryDict,kiSCSIPKSendTargetsEnabled,kCFBooleanFalse);

    discoveryCacheModified = true;
}

/*! Gets whether SendTargets discovery is set ot disabled or enabled.
 *  @return True if send targets discovery is set to enabled, false otherwise. */
Boolean iSCSIPLGetSendTargetsDiscoveryEnable()
{
    CFMutableDictionaryRef discoveryDict = iSCSIPLGetDiscoveryDict(true);
    return CFBooleanGetValue(CFDictionaryGetValue(discoveryDict,kiSCSIPKSendTargetsEnabled));
}

/*! Sets SendTargets discovery interval.
 *  @param interval the discovery interval, in seconds. */
void iSCSIPLSetSendTargetsDiscoveryInterval(CFIndex interval)
{
    CFMutableDictionaryRef discoveryDict = iSCSIPLGetDiscoveryDict(true);
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberCFIndexType,&interval);
    CFDictionarySetValue(discoveryDict,kiSCSIPKDiscoveryInterval,value);

    discoveryCacheModified = true;
}

/*! Gets SendTargets disocvery interval.
 *  @return the discovery interval, in seconds. */
CFIndex iSCSIPLGetSendTargetsDiscoveryInterval()
{
    CFIndex interval = 0;
    CFMutableDictionaryRef discoveryDict = iSCSIPLGetDiscoveryDict(true);
    CFNumberRef value = CFDictionaryGetValue(discoveryDict,kiSCSIPKDiscoveryInterval);
    CFNumberGetValue(value,kCFNumberCFIndexType,&interval);
    return interval;
}


/*! Synchronizes the intitiator and target settings cache with the property
 *  list on the disk. */
void iSCSIPLSynchronize()
{
    // If we have modified our targets dictionary, we write changes back and
    // otherwise we'll read in the latest.
    if(targetNodesCacheModified)
        CFPreferencesSetValue(kiSCSIPKTargets,targetsCache,kiSCSIPKAppId,
                              kCFPreferencesAnyUser,kCFPreferencesCurrentHost);
    
    if(initiatorNodeCacheModified)
        CFPreferencesSetValue(kiSCSIPKInitiator,initiatorCache,kiSCSIPKAppId,
                              kCFPreferencesAnyUser,kCFPreferencesCurrentHost);
    
    if(discoveryCacheModified)
        CFPreferencesSetValue(kiSCSIPKDiscovery,discoveryCache,kiSCSIPKAppId,
                              kCFPreferencesAnyUser,kCFPreferencesCurrentHost);

    CFPreferencesAppSynchronize(kiSCSIPKAppId);
    
    if(!targetNodesCacheModified)
    {
        // Free old cache if present
        if(targetsCache)
            CFRelease(targetsCache);
        
        // Refresh cache from preferences
        targetsCache = iSCSIPLCopyPropertyDict(kiSCSIPKTargets);
    }
    
    if(!initiatorNodeCacheModified)
    {
        // Free old cache if present
        if(initiatorCache)
            CFRelease(initiatorCache);
        
        // Refresh cache from preferences
        initiatorCache = iSCSIPLCopyPropertyDict(kiSCSIPKInitiator);
    }
    
    if(!discoveryCacheModified)
    {
        // Free old cache if present
        if(discoveryCache)
            CFRelease(discoveryCache);
        
        // Refresh cache from preferences
        discoveryCache = iSCSIPLCopyPropertyDict(kiSCSIPKDiscovery);
    }

    initiatorNodeCacheModified = targetNodesCacheModified = discoveryCacheModified = false;
}
