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

/*! Preference key name for iSCSI targets dictionary (holds all targets). */
CFStringRef kiSCSIPKTargetsKey = CFSTR("Target Nodes");

/*! Preference key name for iSCSI target dictionary (specific to each). */
CFStringRef kiSCSIPKTargetKey = CFSTR("Target Data");

/*! Preference key name for iSCSI discovery dictionary. */
CFStringRef kiSCSIPKDiscoveryKey = CFSTR("SendTargets Discovery");

/*! Preference key name for iSCSI initiator dictionary. */
CFStringRef kiSCSIPKInitiatorKey = CFSTR("Initiator Node");


/*! Preference key name for iSCSI session configuration (specific to each target). */
CFStringRef kiSCSIPKSessionCfgKey = CFSTR("Session Configuration");

/*! Preference key name for iSCSI portals dictionary (specific to each target). */
CFStringRef kiSCSIPKPortalsKey = CFSTR("Portals");


/*! Preference key name for iSCSI portal dictionary (specific to each). */
CFStringRef kiSCSIPKPortalKey = CFSTR("Portal Data");

/*! Preference key name for iSCSI connection configuration information. */
CFStringRef kiSCSIPKConnectionCfgKey = CFSTR("Connection Configuration");

/*! Preference key name for iSCSI authentication. */
CFStringRef kiSCSIPKAuthKey = CFSTR("Authentication");

/*! Preference key value for no authentication. */
CFStringRef kiSCSIPVAuthNone = CFSTR("None");

/*! Preference key value for CHAP authentication. */
CFStringRef kiSCSIPVAuthCHAP = CFSTR("CHAP");

/*! Preference key name for iSCSI CHAP authentication name. */
CFStringRef kiSCSIPKAuthCHAPNameKey = CFSTR("CHAP Name");

/*! Preference key name for iSCSI initiator name. */
CFStringRef kiSCSIPKInitiatorIQN = CFSTR("Name");

/*! Preference key name for iSCSI initiator alias. */
CFStringRef kiSCSIPKInitiatorAlias = CFSTR("Alias");

/*! Default initiator alias to use. */
CFStringRef kiSCSIPVDefaultInitiatorAlias = CFSTR("localhost");

/*! Default initiator IQN to use. */
CFStringRef kiSCSIPVDefaultInitiatorIQN = CFSTR("iqn.2015-01.com.localhost:initiator");

/*! Retrieves a mutable dictionary for the specified key. 
 *  @param key the name of the key, which can be either kiSCSIPKTargetsKey,
 *  kiSCSIPKDiscoveryKey, or kiSCSIPKInitiatorKey.
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
        kCFAllocatorDefault,propertyList,kCFPropertyListMutableContainersAndLeaves);
    
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
    return CFDictionaryCreateMutable(kCFAllocatorDefault,
                                     0,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
}

/*! Creates a mutable dictionary for the initiator key.
 *  @return a mutable dictionary for the initiator key. */
CFMutableDictionaryRef iSCSIPLCreateInitiatorDict()
{
    CFStringRef keys[] = {
        kiSCSIPKAuthCHAPNameKey,
        kiSCSIPKAuthKey,
        kiSCSIPKInitiatorAlias,
        kiSCSIPKInitiatorIQN };

    CFStringRef values[] = {
        kiSCSIPVDefaultInitiatorAlias,
        kiSCSIPVAuthNone,
        kiSCSIPVDefaultInitiatorAlias,
        kiSCSIPVDefaultInitiatorIQN };
    
    CFDictionaryRef initiatorPropertylist = CFDictionaryCreate(kCFAllocatorDefault,
                                                               (const void **)keys,
                                                               (const void **)values,
                                                               sizeof(keys)/sizeof(CFStringRef),
                                                               &kCFTypeDictionaryKeyCallBacks,
                                                               &kCFTypeDictionaryValueCallBacks);
//// TODO: fix memory leak
    return CFDictionaryCreateMutableCopy(kCFAllocatorDefault,0,initiatorPropertylist);
}

CFMutableDictionaryRef iSCSIPLGetInitiatorDict(Boolean createIfMissing)
{
    if(createIfMissing && !initiatorCache)
        initiatorCache = iSCSIPLCreateInitiatorDict();

    return initiatorCache;
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
            CFMutableDictionaryRef targetDict = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
            
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
        if(createIfMissing && CFDictionaryGetCountOfKey(targetDict,kiSCSIPKPortalsKey) == 0)
        {
            CFMutableDictionaryRef portalsList = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
            
            CFDictionarySetValue(targetDict,kiSCSIPKPortalsKey,portalsList);
            CFRelease(portalsList);
        }
        return (CFMutableDictionaryRef)CFDictionaryGetValue(targetDict,kiSCSIPKPortalsKey);
    }
    return NULL;
}

CFMutableDictionaryRef iSCSIPLGetPortalInfo(CFStringRef targetIQN,
                                            CFStringRef portalAddress,
                                            Boolean createIfMissing)
{
    // Get list of portals for this target
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,createIfMissing);

    // If list was valid (target exists), get the dictionary with portal
    // information, including portal (address) information, configuration,
    // and authentication sub-dictionaries
    if(portalsList)
    {
       if(createIfMissing && CFDictionaryGetCountOfKey(portalsList,portalAddress) == 0)
       {
           CFMutableDictionaryRef portalInfo = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                         0,
                                                                         &kCFTypeDictionaryKeyCallBacks,
                                                                         &kCFTypeDictionaryValueCallBacks);

           CFDictionarySetValue(portalInfo,kiSCSIPKConnectionCfgKey,CFSTR(""));
           CFDictionarySetValue(portalInfo,kiSCSIPKPortalKey,CFSTR(""));
           
           CFDictionarySetValue(portalsList,portalAddress,portalInfo);
           CFRelease(portalInfo);

       }
       return (CFMutableDictionaryRef)CFDictionaryGetValue(portalsList,portalAddress);
    }
    return NULL;
}

iSCSISessionConfigRef iSCSIPLCopySessionConfig(CFStringRef targetIQN)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);
    
    if(targetDict)
        return iSCSISessionConfigCreateWithDictionary(CFDictionaryGetValue(targetDict,kiSCSIPKSessionCfgKey));
    
    return NULL;
}

void iSCSIPLSetSessionConfig(CFStringRef targetIQN,
                                      iSCSISessionConfigRef sessCfg)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);
    CFDictionaryRef sessCfgDict = iSCSISessionConfigCreateDictionary(sessCfg);
    
    CFDictionarySetValue(targetDict,kiSCSIPKSessionCfgKey,sessCfgDict);
    CFRelease(sessCfgDict);
    
    targetNodesCacheModified = true;
}

iSCSIPortalRef iSCSIPLCopyPortalForTarget(CFStringRef targetIQN,
                                          CFStringRef portalAddress)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetIQN,portalAddress,false);
    
    if(portalInfo)
        return iSCSIPortalCreateWithDictionary(CFDictionaryGetValue(portalInfo,kiSCSIPKPortalKey));
    
    return NULL;
}

iSCSITargetRef iSCSIPLCopyTarget(CFStringRef targetIQN)
{
    iSCSITargetRef target = NULL;

    // Get the dictionary containing information about the target
    CFDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    if(targetDict)
        target = CFDictionaryGetValue(targetDict,kiSCSIPKTargetKey);

    return target;
}

iSCSIConnectionConfigRef iSCSIPLCopyConnectionConfig(CFStringRef targetIQN,CFStringRef portalAddress)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetIQN,portalAddress,false);
    
    if(portalInfo)
        return iSCSIConnectionConfigCreateWithDictionary(CFDictionaryGetValue(portalInfo,kiSCSIPKConnectionCfgKey));

    return NULL;
}


/*! Helper function. Creates an authentication object for a particular
 *  node (initiator or target) when supplied with the node's dictionary.
 *  The node dictionary can be obtained by calling iSCSIPLGetInitiatorDict()
 *  for the initiator or iSCSIPLGetTargetDict() for the target. */
iSCSIAuthRef iSCSIPLCreateAuthenticationForNode(CFStringRef nodeIQN,
                                                CFDictionaryRef nodeDictionary)
{
    iSCSIAuthRef auth = NULL;

    if(nodeDictionary) {
        CFStringRef authMethod = CFDictionaryGetValue(nodeDictionary,kiSCSIPKAuthKey);

        if(CFStringCompare(authMethod,kiSCSIPVAuthCHAP,0) == kCFCompareEqualTo) {

            CFStringRef name = CFDictionaryGetValue(nodeDictionary,kiSCSIPKAuthCHAPNameKey);
            CFStringRef secret = iSCSIKeychainCopyCHAPSecretForNode(nodeIQN);

            if(secret)
                auth = iSCSIAuthCreateCHAP(name,secret);
            else
                auth = iSCSIAuthCreateNone();
        }
        else
            auth = iSCSIAuthCreateNone();
    }
    return auth;
}

/*! Creates an authentication object that represents the current
 *  authentication configuration of the initiator.
 *  @return the authentication object. */
iSCSIAuthRef iSCSIPLCreateAuthenticationForInitiator()
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(false);
    CFStringRef initiatorIQN = CFDictionaryGetValue(initiatorDict,kiSCSIPKInitiatorIQN);

    return iSCSIPLCreateAuthenticationForNode(initiatorIQN,initiatorDict);
}

/*! Sets authentication method to be used by initiator. */
void iSCSIPLSetInitiatorAuthenticationMethod(enum iSCSIAuthMethods authMethod)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);

    if(authMethod == kiSCSIAuthMethodNone)
        CFDictionarySetValue(initiatorDict,kiSCSIPKAuthKey,kiSCSIPVAuthNone);
    else if(authMethod == kiSCSIAuthMethodCHAP)
        CFDictionarySetValue(initiatorDict,kiSCSIPKAuthKey,kiSCSIPVAuthCHAP);

    initiatorNodeCacheModified = true;
}

/*! Gets the current authentication method used by the initiator. */
enum iSCSIAuthMethods iSCSIPLGetInitiatorAuthenticationMethod()
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);
    CFStringRef auth = CFDictionaryGetValue(initiatorDict,kiSCSIPKAuthKey);
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
    CFDictionarySetValue(initiatorDict,kiSCSIPKAuthCHAPNameKey,name);

    initiatorNodeCacheModified = true;
}

/*! Copies the CHAP name associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPName()
{
    CFMutableDictionaryRef initiatorDict = iSCSIPLGetInitiatorDict(true);
    CFStringRef name = CFDictionaryGetValue(initiatorDict,kiSCSIPKAuthCHAPNameKey);
    return CFStringCreateCopy(kCFAllocatorDefault,name);
}

/*! Sets the CHAP secret associated with the initiator. */
void iSCSIPLSetInitiatorCHAPSecret(CFStringRef secret)
{
    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    iSCSIKeychainSetCHAPSecretForNode(initiatorIQN,secret);
    CFRelease(initiatorIQN);
}

/*! Copies the CHAP secret associated with the initiator. */
CFStringRef iSCSIPLCopyInitiatorCHAPSecret()
{
    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();

    CFStringRef secret = iSCSIKeychainCopyCHAPSecretForNode(initiatorIQN);
    CFRelease(initiatorIQN);
    return secret;
}

void iSCSIPLSetConnectionConfig(CFStringRef targetIQN,
                                CFStringRef portalAddress,
                                iSCSIConnectionConfigRef connCfg)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetIQN,portalAddress,true);
    
    // Set the authentication object
    CFDictionaryRef connCfgDict = iSCSIConnectionConfigCreateDictionary(connCfg);
    CFDictionarySetValue(portalInfo,kiSCSIPKConnectionCfgKey,connCfgDict);
    CFRelease(connCfgDict);
    
    targetNodesCacheModified = true;
}

void iSCSIPLSetPortalForTarget(CFStringRef targetIQN,
                               iSCSIPortalRef portal)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetIQN,iSCSIPortalGetAddress(portal),true);
    
    // Set the authentication object
    CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
    CFDictionarySetValue(portalInfo,kiSCSIPKPortalKey,portalDict);
    CFRelease(portalDict);
    
    targetNodesCacheModified = true;
}

void iSCSIPLRemovePortalForTarget(CFStringRef targetIQN,
                                  CFStringRef portalAddress)
{
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);
    
    if(!portalsList)
        return;
    
    CFDictionaryRemoveValue(portalsList,portalAddress);
    
    targetNodesCacheModified = true;
}

void iSCSIPLRemoveTarget(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetsList = iSCSIPLGetTargets(false);
    
    if(!targetsList)
        return;
    
    CFDictionaryRemoveValue(targetsList,targetIQN);
    
    targetNodesCacheModified = true;
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
 *  @param targetIQN the name of the target.
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPLContainsTarget(CFStringRef targetIQN)
{
    CFDictionaryRef targetsList = iSCSIPLGetTargets(false);
    return CFDictionaryContainsKey(targetsList,targetIQN);
}

/*! Gets whether a portal is defined in the property list.
 *  @param targetIQN the name of the target.
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortalForTarget(CFStringRef targetIQN,
                              CFStringRef portalAddress)
{
    CFDictionaryRef portalsList = iSCSIPLGetPortalsList(targetIQN,false);
    return (portalsList && CFDictionaryContainsKey(portalsList,portalAddress));
}


/*! Creates an array of target names (fully qualified IQN or EUI names)
 *  defined in the property list.
 *  @return an array of target names. */
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

/*! Creates an array of portal names for a given target.
 *  @param targetIQN the name of the target (fully qualified IQN or EUI name).
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
void iSCSIPLModifyTargetIQN(CFStringRef existingIQN,CFStringRef newIQN)
{
    CFMutableDictionaryRef targetNodes = iSCSIPLGetTargets(false);
    CFMutableDictionaryRef target = iSCSIPLGetTargetDict(existingIQN,false);

    if(target && targetNodes)
    {
        CFDictionarySetValue(targetNodes,newIQN,target);
        CFDictionaryRemoveValue(targetNodes,existingIQN);

        targetNodesCacheModified = true;
    }
}

/*! Creates an authentication object that represents the current
 *  authentication configuration of the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the authentication object. */
iSCSIAuthRef iSCSIPLCreateAuthenticationForTarget(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,false);

    return iSCSIPLCreateAuthenticationForNode(targetIQN,targetDict);
}

/*! Sets authentication method to be used by target. */
void iSCSIPLSetTargetAuthenticationMethod(CFStringRef targetIQN,
                                          enum iSCSIAuthMethods authMethod)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);

    if(authMethod == kiSCSIAuthMethodNone)
        CFDictionarySetValue(targetDict,kiSCSIPKAuthKey,kiSCSIPVAuthNone);
    else if(authMethod == kiSCSIAuthMethodCHAP)
        CFDictionarySetValue(targetDict,kiSCSIPKAuthKey,kiSCSIPVAuthCHAP);

    targetNodesCacheModified = true;
}

/*! Gets the current authentication method used by the target. */
enum iSCSIAuthMethods iSCSIPLGetTargetAuthenticationMethod(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);
    CFStringRef auth = CFDictionaryGetValue(targetDict,kiSCSIPKAuthKey);
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;

    if(CFStringCompare(auth,kiSCSIPVAuthNone,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodNone;
    else if(CFStringCompare(auth,kiSCSIPVAuthCHAP,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodCHAP;

    return authMethod;
}

/*! Sets the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param name the CHAP name associated with the target. */
void iSCSIPLSetTargetCHAPName(CFStringRef targetIQN,CFStringRef name)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);

    // Change CHAP name in property list
    CFDictionarySetValue(targetDict,kiSCSIPKAuthCHAPNameKey,name);

    targetNodesCacheModified = true;
}

/*! Copies the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP name associated with the target. */
CFStringRef iSCSIPLCopyTargetCHAPName(CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPLGetTargetDict(targetIQN,true);
    CFStringRef name = CFDictionaryGetValue(targetDict,kiSCSIPKAuthCHAPNameKey);
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




/*! Adds a discovery record to the property list.
 *  @param discoveryRecord the record to add. */
void iSCSIPLAddDiscoveryRecord(iSCSIDiscoveryRecRef discoveryRecord)
{
    // Iterate over the dictionary and add keys to the existing cache
    CFDictionaryRef discoveryDict = iSCSIDiscoveryRecCreateDictionary(discoveryRecord);

    if(!discoveryDict)
        return;
    
    if(!discoveryCache)
        discoveryCache = iSCSIPLCreateDiscoveryDict();
    
    const CFIndex count = CFDictionaryGetCount(discoveryDict);
    const void * keys[count];
    const void * values[count];
    CFDictionaryGetKeysAndValues(discoveryDict,keys,values);
    
    for(CFIndex idx = 0; idx < count; idx++) {
        CFDictionarySetValue(discoveryCache,keys[idx],values[idx]);
    }
    
    CFRelease(discoveryDict);
    discoveryCacheModified = true;
}

/*! Retrieves the discovery record from the property list.
 *  @return the cached discovery record. */
iSCSIDiscoveryRecRef iSCSIPLCopyDiscoveryRecord()
{
    if(!discoveryCache)
        return NULL;
    
    return iSCSIDiscoveryRecCreateWithDictionary(discoveryCache);
}

/*! Clears the discovery record. */
void iSCSIPLClearDiscoveryRecord()
{
    CFRelease(discoveryCache);
    discoveryCache = NULL;
    discoveryCacheModified = true;
}

/*! Synchronizes the intitiator and target settings cache with the property
 *  list on the disk. */
void iSCSIPLSynchronize()
{
    // If we have modified our targets dictionary, we write changes back and
    // otherwise we'll read in the latest.
    if(targetNodesCacheModified)
        CFPreferencesSetValue(kiSCSIPKTargetsKey,targetsCache,kiSCSIPKAppId,
                              kCFPreferencesAnyUser,kCFPreferencesCurrentHost );
    
    if(initiatorNodeCacheModified)
        CFPreferencesSetValue(kiSCSIPKInitiatorKey,initiatorCache,kiSCSIPKAppId,
                              kCFPreferencesAnyUser,kCFPreferencesCurrentHost );
    
    if(discoveryCacheModified)
        CFPreferencesSetValue(kiSCSIPKDiscoveryKey,discoveryCache,kiSCSIPKAppId,
                              kCFPreferencesAnyUser,kCFPreferencesCurrentHost);

    CFPreferencesAppSynchronize(kiSCSIPKAppId);
    
    if(!targetNodesCacheModified)
    {
        // Free old cache if present
        if(targetsCache)
            CFRelease(targetsCache);
        
        // Refresh cache from preferences
        targetsCache = iSCSIPLCopyPropertyDict(kiSCSIPKTargetsKey);
    }
    
    if(!initiatorNodeCacheModified)
    {
        // Free old cache if present
        if(initiatorCache)
            CFRelease(initiatorCache);
        
        // Refresh cache from preferences
        initiatorCache = iSCSIPLCopyPropertyDict(kiSCSIPKInitiatorKey);
    }
    
    if(!discoveryCacheModified)
    {
        // Free old cache if present
        if(discoveryCache)
            CFRelease(discoveryCache);
        
        // Refresh cache from preferences
        discoveryCache = iSCSIPLCopyPropertyDict(kiSCSIPKDiscoveryKey);
    }

    initiatorNodeCacheModified = targetNodesCacheModified = discoveryCacheModified = false;
}
