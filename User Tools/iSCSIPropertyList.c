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
Boolean targetsCacheModified = false;

/*! A cached version of the initiator dictionary. */
CFMutableDictionaryRef initiatorCache = NULL;

/*! Flag that indicates whether the initiator cache was modified. */
Boolean initiatorCacheModified = false;

/*! App ID. */
CFStringRef kiSCSIPKAppId = CFSTR("com.NSinenian.iSCSI");

/*! Preference key name for iSCSI target dictionary. */
CFStringRef kiSCSIPKTargetsKey = CFSTR("Targets");

/*! Preference key name for iSCSI discovery dictionary. */
CFStringRef kiSCSIPKDiscoveryKey = CFSTR("Discovery");

/*! Preference key name for iSCSI initiator dictionary. */
CFStringRef kiSCSIPKInitiatorKey = CFSTR("Initiator");


/*! Preference key name for iSCSI session configuration (specific to each target). */
CFStringRef kiSCSIPKSessionCfgKey = CFSTR("Session Configuration");

/*! Preference key name for iSCSI portals dictionary (specific to each target). */
CFStringRef kiSCSIPKPortalsKey = CFSTR("Portals");


/*! Preference key name for iSCSI portal dictionary (specific to each ). */
CFStringRef kiSCSIPKPortalKey = CFSTR("Portal");

/*! Preference key name for iSCSI connection configuration information. */
CFStringRef kiSCSIPKConnectionCfgKey = CFSTR("Connection Configuration");

/*! Preference key name for iSCSI authentication (iSCSIAuth object). */
CFStringRef kiSCSIPKAuthKey = CFSTR("Authentication");


/*! Preference key name for iSCSI initiator name. */
CFStringRef kiSCSIPKInitiatorName = CFSTR("Name");

/*! Preference key name for iSCSI initiator alias. */
CFStringRef kiSCSIPKInitiatorAlias = CFSTR("Alias");

/*! Retrieves a mutable dictionary for the specified key. 
 *  @param key the name of the key, which can be either kiSCSIPKTargetsKey,
 *  kiSCSIPKDiscoveryKey, or kiSCSIPKInitiatorKey.
 *  @return mutable dictionary with list of properties for the specified key.*/
CFMutableDictionaryRef iSCSIPLCopyPropertyDict(CFStringRef key)
{
    // Retrieve the desired property from the property list
    CFDictionaryRef propertyList = CFPreferencesCopyAppValue(key,kiSCSIPKAppId);
    
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

/*! Creates a mutable dictionary for the initiator key.
 *  @return a mutable dictionary for the initiator key. */
CFMutableDictionaryRef iSCSIPLCreateInitiatorDict()
{
    CFStringRef kInitiatorName = CFSTR("");
    CFStringRef kInitiatorAlias = CFSTR("");
    
    CFStringRef keys[] = {kiSCSIPKInitiatorAlias,kiSCSIPKInitiatorName};
    CFStringRef values[] = {kInitiatorAlias,kInitiatorName};
    
    CFDictionaryRef initiatorPropertylist = CFDictionaryCreate(kCFAllocatorDefault,
                                                               (const void **)keys,
                                                               (const void **)values,
                                                               sizeof(keys)/sizeof(CFStringRef),
                                                               &kCFTypeDictionaryKeyCallBacks,
                                                               &kCFTypeDictionaryValueCallBacks);
//// TODO: fix memory leak
    return CFDictionaryCreateMutableCopy(kCFAllocatorDefault,0,initiatorPropertylist);
}

CFMutableDictionaryRef iSCSIPLGetTargetsList(Boolean createIfMissing)
{
    if(createIfMissing && !targetsCache)
        targetsCache = iSCSIPLCreateTargetsDict();

    return targetsCache;
}

CFMutableDictionaryRef iSCSIPLGetTargetInfo(CFStringRef targetName,
                                            Boolean createIfMissing)
{
    // Get list of targets
    CFMutableDictionaryRef targetsList = iSCSIPLGetTargetsList(createIfMissing);
    
    if(targetsList)
    {
        if(createIfMissing && CFDictionaryGetCountOfKey(targetsList,targetName) == 0)
        {
            CFMutableDictionaryRef targetInfo = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                          0,
                                                                          &kCFTypeDictionaryKeyCallBacks,
                                                                          &kCFTypeDictionaryValueCallBacks);
            
            CFDictionarySetValue(targetsList,targetName,targetInfo);
            CFRelease(targetInfo);
        }
        
        return (CFMutableDictionaryRef)CFDictionaryGetValue(targetsList,targetName);
    }
    return NULL;
}


CFMutableDictionaryRef iSCSIPLGetPortalsList(CFStringRef targetName,
                                             Boolean createIfMissing)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetInfo = iSCSIPLGetTargetInfo(targetName,createIfMissing);

    
    if(targetInfo)
    {
        if(createIfMissing && CFDictionaryGetCountOfKey(targetInfo,kiSCSIPKPortalsKey) == 0)
        {
            CFMutableDictionaryRef portalsList = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                           0,
                                                                           &kCFTypeDictionaryKeyCallBacks,
                                                                           &kCFTypeDictionaryValueCallBacks);
            
            CFDictionarySetValue(targetInfo,kiSCSIPKPortalsKey,portalsList);
            
            CFRelease(portalsList);
        }
        return (CFMutableDictionaryRef)CFDictionaryGetValue(targetInfo,kiSCSIPKPortalsKey);
    }
    return NULL;
}

CFMutableDictionaryRef iSCSIPLGetPortalInfo(CFStringRef targetName,
                                            CFStringRef portalAddress,
                                            Boolean createIfMissing)
{
    // Get list of portals for this target
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetName,createIfMissing);

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
           
           CFDictionarySetValue(portalInfo,kiSCSIPKAuthKey,CFSTR(""));
           CFDictionarySetValue(portalInfo,kiSCSIPKConnectionCfgKey,CFSTR(""));
           CFDictionarySetValue(portalInfo,kiSCSIPKPortalKey,CFSTR(""));
           
           CFDictionarySetValue(portalsList,portalAddress,portalInfo);
           CFRelease(portalInfo);

       }
       return (CFMutableDictionaryRef)CFDictionaryGetValue(portalsList,portalAddress);
    }
    return NULL;
}

iSCSISessionConfigRef iSCSIPLCopySessionConfig(CFStringRef targetName)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetInfo = iSCSIPLGetTargetInfo(targetName,false);
    
    if(targetInfo)
        return iSCSISessionConfigCreateWithDictionary(CFDictionaryGetValue(targetInfo,kiSCSIPKSessionCfgKey));
    
    return NULL;
}

void iSCSIPLSetSessionConfig(CFStringRef targetName,iSCSISessionConfigRef sessCfg)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetInfo = iSCSIPLGetTargetInfo(targetName,true);
    CFDictionaryRef sessCfgDict = iSCSISessionConfigCreateDictionary(sessCfg);
    
    CFDictionarySetValue(targetInfo,kiSCSIPKSessionCfgKey,sessCfgDict);
    CFRelease(sessCfgDict);
    
    targetsCacheModified = true;
}

iSCSIPortalRef iSCSIPLCopyPortal(CFStringRef targetName,CFStringRef portalAddress)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalAddress,false);
    
    if(portalInfo)
        return iSCSIPortalCreateWithDictionary(CFDictionaryGetValue(portalInfo,kiSCSIPKPortalKey));
    
    return NULL;
}

iSCSITargetRef iSCSIPLCopyTarget(CFStringRef targetName)
{
    // Get the dictionary containing information about the target
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,targetName);
    
    return target;
}


iSCSIConnectionConfigRef iSCSIPLCopyConnectionConfig(CFStringRef targetName,CFStringRef portalAddress)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalAddress,false);
    
    if(portalInfo)
        return iSCSIConnectionConfigCreateWithDictionary(CFDictionaryGetValue(portalInfo,kiSCSIPKConnectionCfgKey));

    return NULL;
}

iSCSIAuthRef iSCSIPLCopyAuthentication(CFStringRef targetName,CFStringRef portalAddress)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalAddress,false);
    
    if(portalInfo)
        return iSCSIAuthCreateWithDictionary(CFDictionaryGetValue(portalInfo,kiSCSIPKAuthKey));
    
    return NULL;
}

void iSCSIPLSetAuthentication(CFStringRef targetName,CFStringRef portalAddress,iSCSIAuthRef auth)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalAddress,true);
    
    // Set the authentication object
    CFDictionaryRef authDict = iSCSIAuthCreateDictionary(auth);
    CFDictionarySetValue(portalInfo,kiSCSIPKAuthKey,authDict);
    CFRelease(authDict);
    
    targetsCacheModified = true;
}

void iSCSIPLSetConnectionConfig(CFStringRef targetName,
                                CFStringRef portalAddress,
                                iSCSIConnectionConfigRef connCfg)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalAddress,true);
    
    // Set the authentication object
    CFDictionaryRef connCfgDict = iSCSIConnectionConfigCreateDictionary(connCfg);
    CFDictionarySetValue(portalInfo,kiSCSIPKConnectionCfgKey,connCfgDict);
    CFRelease(connCfgDict);
    
    targetsCacheModified = true;
}

void iSCSIPLSetPortal(CFStringRef targetName,
                      iSCSIPortalRef portal)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,iSCSIPortalGetAddress(portal),true);
    
    // Set the authentication object
    CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
    CFDictionarySetValue(portalInfo,kiSCSIPKPortalKey,portalDict);
    CFRelease(portalDict);
    
    targetsCacheModified = true;
}

void iSCSIPLRemovePortal(CFStringRef targetName,CFStringRef portalAddress)
{
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetName,false);
    
    if(!portalsList)
        return;
    
    CFDictionaryRemoveValue(portalsList,portalAddress);
    
    targetsCacheModified = true;
}

void iSCSIPLRemoveTarget(CFStringRef targetName)
{
    CFMutableDictionaryRef targetsList = iSCSIPLGetTargetsList(false);
    
    if(!targetsList)
        return;
    
    CFDictionaryRemoveValue(targetsList,targetName);
    
    targetsCacheModified = true;
}

/*! Copies the initiator name from the iSCSI property list to a CFString object.
 *  @return the initiator name. */
CFStringRef iSCSIPLCopyInitiatorName()
{
    if(!initiatorCache)
        return NULL;
    
    // Lookup and copy the initiator name from the dictionary
    CFStringRef initiatorName = CFStringCreateCopy(
        kCFAllocatorDefault,CFDictionaryGetValue(initiatorCache,kiSCSIPKInitiatorName));
    
    return initiatorName;
}

/*! Sets the initiator name in the iSCSI property list.
 *  @param initiatorName the initiator name to set. */
void iSCSIPLSetInitiatorName(CFStringRef initiatorName)
{
    if(!initiatorCache)
        initiatorCache = iSCSIPLCreateInitiatorDict();
    
    // Update initiator name
    CFDictionarySetValue(initiatorCache,kiSCSIPKInitiatorName,initiatorName);
    
    initiatorCacheModified = true;
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
    
    initiatorCacheModified = true;
}

/*! Gets whether a target is defined in the property list.
 *  @param targetName the name of the target.
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPLContainsTarget(CFStringRef targetName)
{
    CFDictionaryRef targetsList = iSCSIPLGetTargetsList(false);
    return CFDictionaryContainsKey(targetsList,targetName);
}

/*! Gets whether a portal is defined in the property list.
 *  @param targetName the name of the target.
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPLContainsPortal(CFStringRef targetName,
                              CFStringRef portalAddress)
{
    CFDictionaryRef portalsList = iSCSIPLGetPortalsList(targetName,false);
    return (portalsList && CFDictionaryContainsKey(portalsList,portalAddress));
}


/*! Creates an array of target names (fully qualified IQN or EUI names)
 *  defined in the property list.
 *  @return an array of target names. */
CFArrayRef iSCSIPLCreateArrayOfTargets()
{
    CFDictionaryRef targetsList = iSCSIPLGetTargetsList(false);
    
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
 *  @param targetName the name of the target (fully qualified IQN or EUI name).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPLCreateArrayOfPortals(CFStringRef targetName)
{
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetName,false);
    
    if(!portalsList)
        return NULL;
    
    const CFIndex keyCount = CFDictionaryGetCount(portalsList);
    
    if(keyCount == 0)
        return NULL;
    
    const void * keys[keyCount];
    CFDictionaryGetKeysAndValues(portalsList,keys,NULL);
    
    return CFArrayCreate(kCFAllocatorDefault,keys,keyCount,&kCFTypeArrayCallBacks);
}

/*! Synchronizes the intitiator and target settings cache with the property
 *  list on the disk. */
void iSCSIPLSynchronize()
{
    // If we have modified our targets dictionary, we write changes back and
    // otherwise we'll read in the latest.
    if(targetsCacheModified)
        CFPreferencesSetAppValue(kiSCSIPKTargetsKey,targetsCache,kiSCSIPKAppId);
    
    if(initiatorCacheModified)
        CFPreferencesSetAppValue(kiSCSIPKInitiatorKey,initiatorCache,kiSCSIPKAppId);
    
    CFPreferencesAppSynchronize(kiSCSIPKAppId);
    
    if(!targetsCacheModified)
    {
        // Free old cache if present
        if(targetsCache)
            CFRelease(targetsCache);
        
        // Refresh cache from preferences
        targetsCache = iSCSIPLCopyPropertyDict(kiSCSIPKTargetsKey);
    }
    
    if(!initiatorCacheModified)
    {
        // Free old cache if present
        if(initiatorCache)
            CFRelease(initiatorCache);
        
        // Refresh cache from preferences
        initiatorCache = iSCSIPLCopyPropertyDict(kiSCSIPKInitiatorKey);
    }
    
    initiatorCacheModified = targetsCacheModified = false;
}
