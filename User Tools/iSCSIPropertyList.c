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
static CFMutableDictionaryRef targetsCache = NULL;

/*! Flag that indicates whether the targets cache was modified. */
static bool targetsCacheModified = false;

/*! A cached version of the initiator dictionary. */
static CFMutableDictionaryRef initiatorCache = NULL;

/*! Flag that indicates whether the initiator cache was modified. */
static bool initiatorCacheModified = false;

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
    CFDictionaryRef mutablePropertyList = CFPropertyListCreateDeepCopy(
        kCFAllocatorDefault,propertyList,kCFPropertyListMutableContainersAndLeaves);
    
    // Release original retrieved property
    CFRelease(propertyList);
    return (CFMutableDictionaryRef)mutablePropertyList;
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
        if(createIfMissing)
        {
            CFMutableDictionaryRef targetInfo = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);

            CFDictionarySetValue(targetInfo,kiSCSIPKSessionCfgKey,CFSTR(""));
            CFDictionarySetValue(targetInfo,kiSCSIPKPortalsKey,CFSTR(""));
            
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
        if(createIfMissing)
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
                                            CFStringRef portalName,
                                            Boolean createIfMissing)
{
    // Get list of portals for this target
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetName,createIfMissing);

    // If list was valid (target exists), get the dictionary with portal
    // information, including portal (address) information, configuration,
    // and authentication sub-dictionaries
    if(portalsList)
    {
       if(createIfMissing)
       {
           CFMutableDictionaryRef portalInfo = CFDictionaryCreateMutable(
                kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
           
           CFDictionarySetValue(portalInfo,kiSCSIPKAuthKey,CFSTR(""));
           CFDictionarySetValue(portalInfo,kiSCSIPKConnectionCfgKey,CFSTR(""));
           CFDictionarySetValue(portalInfo,kiSCSIPKPortalKey,CFSTR(""));
           
           CFDictionarySetValue(portalsList,portalName,portalInfo);
           CFRelease(portalInfo);

       }
       return (CFMutableDictionaryRef)CFDictionaryGetValue(portalsList,portalName);
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
}

iSCSIPortalRef iSCSIPLCopyPortal(CFStringRef targetName,CFStringRef portalName)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalName,false);
    iSCSIPortalRef portal = NULL;
    
    if(portalInfo)
        return iSCSIPortalCreateWithDictionary(CFDictionaryGetValue(portalInfo,kiSCSIPKPortalKey));
    
    return portal;
}

iSCSIConnectionConfigRef iSCSIPLCopyConnectionConfig(CFStringRef targetName,CFStringRef portalName)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalName,false);
    iSCSIPortalRef portal = NULL;
    
    if(portalInfo)
        return iSCSIConnectionConfigCreateWithDictionary(CFDictionaryGetValue(portal,kiSCSIPKConnectionCfgKey));

    return portal;
}

iSCSIAuthRef iSCSIPLCopyAuth(CFStringRef targetName,CFStringRef portalName)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalName,false);
    iSCSIPortalRef portal = NULL;
    
    if(portalInfo)
        return iSCSIAuthCreateWithDictionary(CFDictionaryGetValue(portal,kiSCSIPKAuthKey));
    
    return portal;
}

void iSCSIPLSetAuth(CFStringRef targetName,CFStringRef portalName,iSCSIAuthRef auth)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalName,true);
    
    // Set the authentication object
    CFDictionaryRef authDict = iSCSIAuthCreateDictionary(auth);
    CFDictionarySetValue(portalInfo,kiSCSIPKAuthKey,authDict);
    CFRelease(authDict);
    
    targetsCacheModified = true;
}

void iSCSIPLSetConnectionConfig(CFStringRef targetName,
                                CFStringRef portalName,
                                iSCSIConnectionConfigRef connCfg)
{
    // Get the dictionary containing information about the portal
    CFMutableDictionaryRef portalInfo = iSCSIPLGetPortalInfo(targetName,portalName,true);
    
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


/*! Creates an array of target names (fully qualified IQN or EUI names)
 *  defined in the property list.
 *  @return an array of target names. */
CFArrayRef iSCSIPLCreateArrayOfTargets()
{
    CFMutableDictionaryRef targetsList = iSCSIPLGetTargetsList(false);
    
    if(!targetsList)
        return NULL;

    const void * keys, * values;
    CFDictionaryGetKeysAndValues(targetsList,&keys,&values);
    
    return CFArrayCreate(kCFAllocatorDefault,&keys,CFDictionaryGetCount(targetsList),&kCFTypeArrayCallBacks);
}

/*! Creates an array of portal names for a given target.
 *  @param targetName the name of the target (fully qualified IQN or EUI name).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPLCreateArrayOfPortals(CFStringRef targetName)
{
    CFMutableDictionaryRef portalsList = iSCSIPLGetPortalsList(targetName,false);
    
    if(!portalsList)
        return NULL;
    
    const void * keys, * values;
    CFDictionaryGetKeysAndValues(portalsList,&keys,&values);
    
    return CFArrayCreate(kCFAllocatorDefault,&keys,CFDictionaryGetCount(portalsList),&kCFTypeArrayCallBacks);
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
