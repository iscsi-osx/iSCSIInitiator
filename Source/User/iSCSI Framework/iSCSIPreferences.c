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

#include "iSCSIPreferences.h"

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

/*! Target alias. */
CFStringRef kiSCSIPKTargetAlias = CFSTR("Alias");

/*! Preference key name for target configuration type. */
CFStringRef kiSCSIPKTargetConfigType = CFSTR("Configuration Type");

/*! Preference key value for static target configuration. */
CFStringRef kiSCSIPVTargetConfigTypeStatic = CFSTR("Static");

/*! Preference key value for discovery target configuration. */
CFStringRef kiSCSIPVTargetConfigTypeDiscovery = CFSTR("SendTargets");

/*! Preference key name for auto-login of target. */
CFStringRef kiSCSIPKAutoLogin = CFSTR("Automatic Login");

/*! Preference key name for target persistence. */
CFStringRef kiSCSIPKPersistent = CFSTR("Persistent");

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

/*! Preference key naem for iSCSI discovery portal that manages target. */
CFStringRef kiSCSIPKSendTargetsPortal = CFSTR("Managing Portal");


/*! Retrieves a mutable dictionary for the specified key.
 *  @param key the name of the key, which can be either kiSCSIPKTargets,
 *  kiSCSIPKDiscovery, or kiSCSIPKInitiator.
 *  @return mutable dictionary with list of properties for the specified key. */
CFMutableDictionaryRef iSCSIPreferencesCopyPropertyDict(CFStringRef kAppId,CFStringRef key)
{
    // Retrieve the desired property from preferences
    CFDictionaryRef preferences = CFPreferencesCopyValue(key,kAppId,
                                                          kCFPreferencesAnyUser,
                                                          kCFPreferencesCurrentHost);
    if(!preferences)
        return NULL;
    
    // Create a deep copy to make the dictionary mutable
    CFMutableDictionaryRef mutablePropertyList = (CFMutableDictionaryRef)CFPropertyListCreateDeepCopy(
        kCFAllocatorDefault,
        preferences,
        kCFPropertyListMutableContainersAndLeaves);
    
    // Release original retrieved property
    CFRelease(preferences);
    return mutablePropertyList;
}

/*! Creates a mutable dictionary for the targets key.
 *  @return a mutable dictionary for the targets key. */
CFMutableDictionaryRef iSCSIPreferencesCreateTargetsDict()
{
    return CFDictionaryCreateMutable(kCFAllocatorDefault,
                                     0,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
}


/*! Creates a mutable dictionary for the discovery key.
 *  @return a mutable dictionary for the discovery key. */
CFMutableDictionaryRef iSCSIPreferencesCreateDiscoveryDict()
{
    CFMutableDictionaryRef discoveryDict = CFDictionaryCreateMutable(
        kCFAllocatorDefault,0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    // Default scan interval (0 indicates never)
    CFIndex interval = kiSCSIInitiator_DiscoveryInterval;
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberCFIndexType,&interval);

    CFDictionaryAddValue(discoveryDict,kiSCSIPKSendTargetsEnabled,kCFBooleanFalse);
    CFDictionaryAddValue(discoveryDict,kiSCSIPKDiscoveryInterval,value);

    CFRelease(value);

    return discoveryDict;
}

/*! Creates a mutable dictionary for the initiator key.
 *  @return a mutable dictionary for the initiator key. */
CFMutableDictionaryRef iSCSIPreferencesCreateInitiatorDict()
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

CFMutableDictionaryRef iSCSIPreferencesCreateTargetDict()
{
    CFNumberRef maxConnections = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&kRFC3720_MaxConnections);
    CFNumberRef errorRecoveryLevel = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&kRFC3720_ErrorRecoveryLevel);

    CFMutableDictionaryRef targetDict = CFDictionaryCreateMutable(
        kCFAllocatorDefault,0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    CFDictionaryAddValue(targetDict,kiSCSIPKAuthCHAPName,(void *)CFSTR(""));
    CFDictionaryAddValue(targetDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    CFDictionaryAddValue(targetDict,kiSCSIPKAutoLogin,kCFBooleanFalse);
    CFDictionaryAddValue(targetDict,kiSCSIPKPersistent,kCFBooleanTrue);
    CFDictionaryAddValue(targetDict,kiSCSIPKMaxConnections,maxConnections);
    CFDictionaryAddValue(targetDict,kiSCSIPKErrorRecoveryLevel,errorRecoveryLevel);
    CFDictionaryAddValue(targetDict,kiSCSIPKHeaderDigest,kiSCSIPVDigestNone);
    CFDictionaryAddValue(targetDict,kiSCSIPKDataDigest,kiSCSIPVDigestNone);

    CFRelease(maxConnections);
    CFRelease(errorRecoveryLevel);

    return targetDict;
}

CFMutableDictionaryRef iSCSIPreferencesGetInitiatorDict(iSCSIPreferencesRef preferences,Boolean createIfMissing)
{
    CFMutableDictionaryRef initiatorDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKInitiator);
    
    if(createIfMissing && !initiatorDict) {
        initiatorDict = iSCSIPreferencesCreateInitiatorDict();
        CFDictionarySetValue(preferences,kiSCSIPKInitiator,initiatorDict);
    }
    
    return initiatorDict;
}

CFMutableDictionaryRef iSCSIPreferencesGetDiscoveryDict(iSCSIPreferencesRef preferences,Boolean createIfMissing)
{
    CFMutableDictionaryRef discoveryDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKDiscovery);
    
    if(createIfMissing && !discoveryDict) {
        discoveryDict = iSCSIPreferencesCreateDiscoveryDict();
        CFDictionarySetValue(preferences,kiSCSIPKDiscovery,discoveryDict);
    }
    
    return discoveryDict;
}

/*! Helper function. Retrieves the iSCSI discovery portals dictionary. */
CFMutableDictionaryRef iSCSIPreferencesGetSendTargetsDiscoveryPortals(iSCSIPreferencesRef preferences,Boolean createIfMissing)
{
    // Get discovery dictionary
    CFMutableDictionaryRef discoveryDict = iSCSIPreferencesGetDiscoveryDict(preferences,createIfMissing);

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

CFArrayRef iSCSIPreferencesGetDynamicTargetsForSendTargets(iSCSIPreferencesRef preferences,
                                                  CFStringRef portalAddress,
                                                  Boolean createIfMissing)
{
    CFArrayRef targetsList = NULL;
    CFMutableDictionaryRef discoveryPortals = iSCSIPreferencesGetSendTargetsDiscoveryPortals(preferences,false);

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

CFMutableDictionaryRef iSCSIPreferencesGetTargets(iSCSIPreferencesRef preferences,Boolean createIfMissing)
{
    CFMutableDictionaryRef targetsDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKTargets);
    
    if(createIfMissing && !targetsDict) {
        targetsDict = iSCSIPreferencesCreateTargetsDict();
        CFDictionarySetValue(preferences,kiSCSIPKTargets,targetsDict);
    }
    
    return targetsDict;
}



CFMutableDictionaryRef iSCSIPreferencesGetTargetDict(iSCSIPreferencesRef preferences,
                                            CFStringRef targetIQN,
                                            Boolean createIfMissing)
{
    // Get list of targets
    CFMutableDictionaryRef targetsList = iSCSIPreferencesGetTargets(preferences,createIfMissing);
    
    if(targetsList)
    {
        if(createIfMissing && CFDictionaryGetCountOfKey(targetsList,targetIQN) == 0)
        {
            CFMutableDictionaryRef targetDict = iSCSIPreferencesCreateTargetDict();
            CFDictionarySetValue(targetsList,targetIQN,targetDict);
            CFRelease(targetDict);
        }
        
        return (CFMutableDictionaryRef)CFDictionaryGetValue(targetsList,targetIQN);
    }
    return NULL;
}

CFMutableDictionaryRef iSCSIPreferencesGetPortalsList(iSCSIPreferencesRef preferences,
                                             CFStringRef targetIQN,
                                             Boolean createIfMissing)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,createIfMissing);

    
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
void iSCSIPreferencesSetMaxConnectionsForTarget(iSCSIPreferencesRef preferences,
                                       CFStringRef targetIQN,
                                       UInt32 maxConnections)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&maxConnections);
    CFDictionarySetValue(targetDict,kiSCSIPKMaxConnections,value);
}

/*! Sets the error recovery level to use for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param level the error recovery level. */
void iSCSIPreferencesSetErrorRecoveryLevelForTarget(iSCSIPreferencesRef preferences,
                                           CFStringRef targetIQN,
                                           enum iSCSIErrorRecoveryLevels level)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&level);
    CFDictionarySetValue(targetDict,kiSCSIPKErrorRecoveryLevel,value);
}

/*! Gets the maximum number of connections for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the maximum number of connections for the target. */
UInt32 iSCSIPreferencesGetMaxConnectionsForTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);
    CFNumberRef value = CFDictionaryGetValue(targetDict,kiSCSIPKMaxConnections);

    UInt32 maxConnections = kRFC3720_MaxConnections;
    CFNumberGetValue(value,kCFNumberIntType,&maxConnections);
    return maxConnections;
}

/*! Gets the error recovery level to use for the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the error recovery level. */
enum iSCSIErrorRecoveryLevels iSCSIPreferencesGetErrorRecoveryLevelForTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    // Get the target information dictionary
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);
    CFNumberRef value = CFDictionaryGetValue(targetDict,kiSCSIPKErrorRecoveryLevel);

    enum iSCSIErrorRecoveryLevels errorRecoveryLevel = kRFC3720_ErrorRecoveryLevel;
    CFNumberGetValue(value,kCFNumberIntType,&errorRecoveryLevel);
    return errorRecoveryLevel;
}

iSCSIPortalRef iSCSIPreferencesCopyPortalForTarget(iSCSIPreferencesRef preferences,
                                                   CFStringRef targetIQN,
                                                   CFStringRef portalAddress)
{
    // Get list of portals for this target
    CFMutableDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,false);

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

iSCSITargetRef iSCSIPreferencesCopyTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    iSCSIMutableTargetRef target = NULL;

    if(iSCSIUtilsValidateIQN(targetIQN)) {

        CFMutableDictionaryRef targetsDict = iSCSIPreferencesGetTargets(preferences,false);

        if(targetsDict) {
            if(CFDictionaryContainsKey(targetsDict,targetIQN)) {
                target = iSCSITargetCreateMutable();
                iSCSITargetSetIQN(target,targetIQN);
            }
        }
    }

    return target;
}

enum iSCSIDigestTypes iSCSIPreferencesGetDataDigestForTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    // Get the dictionary containing information about the target
    CFDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);

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

void iSCSIPreferencesSetDataDigestForTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN,enum iSCSIDigestTypes digestType)
{
    // Get the dictionary containing information about the target
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);

    if(targetDict)
    {
        CFStringRef digest = NULL;

        switch(digestType)
        {
            case kiSCSIDigestNone: digest = kiSCSIPVDigestNone; break;
            case kiSCSIDigestCRC32C: digest = kiSCSIPVDigestCRC32C; break;
            case kiSCSIDigestInvalid: break;
        };

        if(digest) {
            CFDictionarySetValue(targetDict,kiSCSIPKDataDigest,digest);
        }
    }
}

enum iSCSIDigestTypes iSCSIPreferencesGetHeaderDigestForTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    // Get the dictionary containing information about the target
    CFDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);

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

void iSCSIPreferencesSetHeaderDigestForTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN,enum iSCSIDigestTypes digestType)
{
    // Get the dictionary containing information about the target
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);

    if(targetDict)
    {
        CFStringRef digest = NULL;

        switch(digestType)
        {
            case kiSCSIDigestNone: digest = kiSCSIPVDigestNone; break;
            case kiSCSIDigestCRC32C: digest = kiSCSIPVDigestCRC32C; break;
            case kiSCSIDigestInvalid: break;
        };

        if(digest) {
            CFDictionarySetValue(targetDict,kiSCSIPKHeaderDigest,digest);
        }
    }
}

/*! Sets authentication method to be used by initiator. */
void iSCSIPreferencesSetInitiatorAuthenticationMethod(iSCSIPreferencesRef preferences,enum iSCSIAuthMethods authMethod)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPreferencesGetInitiatorDict(preferences,true);

    if(authMethod == kiSCSIAuthMethodNone)
        CFDictionarySetValue(initiatorDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    else if(authMethod == kiSCSIAuthMethodCHAP)
        CFDictionarySetValue(initiatorDict,kiSCSIPKAuth,kiSCSIPVAuthCHAP);
}

/*! Gets the current authentication method used by the initiator. */
enum iSCSIAuthMethods iSCSIPreferencesGetInitiatorAuthenticationMethod(iSCSIPreferencesRef preferences)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPreferencesGetInitiatorDict(preferences,true);
    CFStringRef auth = CFDictionaryGetValue(initiatorDict,kiSCSIPKAuth);
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;

    if(CFStringCompare(auth,kiSCSIPVAuthNone,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodNone;
    else if(CFStringCompare(auth,kiSCSIPVAuthCHAP,0) == kCFCompareEqualTo)
        authMethod = kiSCSIAuthMethodCHAP;

    return authMethod;
}

/*! Sets the CHAP name associated with the initiator. */
void iSCSIPreferencesSetInitiatorCHAPName(iSCSIPreferencesRef preferences,CFStringRef name)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPreferencesGetInitiatorDict(preferences,true);

    // Change CHAP name in preferences
    CFDictionarySetValue(initiatorDict,kiSCSIPKAuthCHAPName,name);
}

/*! Copies the CHAP name associated with the initiator. */
CFStringRef iSCSIPreferencesCopyInitiatorCHAPName(iSCSIPreferencesRef preferences)
{
    CFMutableDictionaryRef initiatorDict = iSCSIPreferencesGetInitiatorDict(preferences,true);
    CFStringRef name = CFDictionaryGetValue(initiatorDict,kiSCSIPKAuthCHAPName);
    return CFStringCreateCopy(kCFAllocatorDefault,name);
}

/*! Sets the CHAP secret associated with the initiator.
 *  @return status indicating the result of the operation. */
OSStatus iSCSIPreferencesSetInitiatorCHAPSecret(iSCSIPreferencesRef preferences,CFStringRef secret)
{
    CFStringRef initiatorIQN = iSCSIPreferencesCopyInitiatorIQN(preferences);

    OSStatus status = iSCSIKeychainSetCHAPSecretForNode(initiatorIQN,secret);
    CFRelease(initiatorIQN);

    return status;
}

/*! Copies the CHAP secret associated with the initiator. */
CFStringRef iSCSIPreferencesCopyInitiatorCHAPSecret(iSCSIPreferencesRef preferences)
{
    CFStringRef initiatorIQN = iSCSIPreferencesCopyInitiatorIQN(preferences);

    CFStringRef secret = iSCSIKeychainCopyCHAPSecretForNode(initiatorIQN);
    CFRelease(initiatorIQN);
    return secret;
}

/*! Gets whether a CHAP secret exists for the initiator.
 *  @return true if a CHAP secret exists for the initiator. */
Boolean iSCSIPreferencesExistsInitiatorCHAPSecret(iSCSIPreferencesRef preferences)
{
    CFStringRef initiatorIQN = iSCSIPreferencesCopyInitiatorIQN(preferences);
    Boolean exists = iSCSIKeychainContainsCHAPSecretForNode(initiatorIQN);
    CFRelease(initiatorIQN);
    return exists;
}

void iSCSIPreferencesSetPortalForTarget(iSCSIPreferencesRef preferences,
                               CFStringRef targetIQN,
                               iSCSIPortalRef portal)
{
    // Get list of portals for this target (only if it exists, otherwise NULL)
    CFMutableDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,false);

    if(portal && portalsList) {
        CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFDictionarySetValue(portalsList,portalAddress,portalDict);
        CFRelease(portalDict);
    }
}

void iSCSIPreferencesRemovePortalForTarget(iSCSIPreferencesRef preferences,
                                  CFStringRef targetIQN,
                                  CFStringRef portalAddress)
{
    CFMutableDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,false);
    
    // Remove target if only one portal is left...
    if(portalsList) {
        if(CFDictionaryGetCount(portalsList) == 1)
            iSCSIPreferencesRemoveTarget(preferences,targetIQN);
        else {
            CFDictionaryRemoveValue(portalsList,portalAddress);
        }
    }
}

/*! Sets whether the target should be logged in during startup.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param autoLogin true to auto login, false otherwise. */
void iSCSIPreferencesSetAutoLoginForTarget(iSCSIPreferencesRef preferences,
                                  CFStringRef targetIQN,
                                  Boolean autoLogin)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
    
    if(targetDict) {
        if(autoLogin)
            CFDictionarySetValue(targetDict,kiSCSIPKAutoLogin,kCFBooleanTrue);
        else
            CFDictionarySetValue(targetDict,kiSCSIPKAutoLogin,kCFBooleanFalse);
    }
}

/*! Gets whether the target should be logged in during startup.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
Boolean iSCSIPreferencesGetAutoLoginForTarget(iSCSIPreferencesRef preferences,
                                     CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
    Boolean autoLogin = false;

    if(targetDict) {
        if(CFDictionaryGetValue(targetDict,kiSCSIPKAutoLogin) == kCFBooleanTrue)
            autoLogin = true;
    }
    
    return autoLogin;
}

/*! Sets whether the a connection to the target should be re-established
 *  in the event of an interruption.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param persistent true to make target persistent, false otherwise. */
void iSCSIPreferencesSetPersistenceForTarget(iSCSIPreferencesRef preferences,
                                             CFStringRef targetIQN,
                                             Boolean persistent)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
    
    if(targetDict) {
        if(persistent)
            CFDictionarySetValue(targetDict,kiSCSIPKPersistent,kCFBooleanTrue);
        else
            CFDictionarySetValue(targetDict,kiSCSIPKPersistent,kCFBooleanFalse);
    }
}

/*! Gets whether the a connection to the target should be re-established
 *  in the event of an interruption.
 *  @param preferences an iSCSI preferences object.
 *  @param targetIQN the target iSCSI qualified name (IQN). */
Boolean iSCSIPreferencesGetPersistenceForTarget(iSCSIPreferencesRef preferences,
                                                CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
    Boolean persistent = false;
    
    if(targetDict) {
        if(CFDictionaryGetValue(targetDict,kiSCSIPKPersistent) == kCFBooleanTrue)
            persistent = true;
    }
    
    return persistent;
}

/*! Adds a target object with a specified portal.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set. */
void iSCSIPreferencesAddStaticTarget(iSCSIPreferencesRef preferences,
                            CFStringRef targetIQN,
                            iSCSIPortalRef portal)
{
    // Only proceed if the target does not exist; otherwise do nothing
    if(!iSCSIPreferencesContainsTarget(preferences,targetIQN)) {

        // Create list of target portals for a specific target (since the target
        // does not exist it is created)
        CFMutableDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,true);

        // Add portal to newly create list of portals for this target
        CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFDictionarySetValue(portalsList,portalAddress,portalDict);
        CFRelease(portalDict);

        // Set target config
        iSCSIPreferencesSetTargetConfigType(preferences,targetIQN,kiSCSITargetConfigStatic);
    }
}

/*! Adds a target object with a specified portal, and associates it with
 *  a particular SendTargets portal that manages the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portal the portal object to set.
 *  @param sendTargetsPortal the discovery portal address with which to
 *  associate the managed target. */
void iSCSIPreferencesAddDynamicTargetForSendTargets(iSCSIPreferencesRef preferences,
                                           CFStringRef targetIQN,
                                           iSCSIPortalRef portal,
                                           CFStringRef sendTargetsPortal)
{
    // Only proceed if the target does not exist; otherwise do nothing
    if(!iSCSIPreferencesContainsTarget(preferences,targetIQN)) {

        // Get target dictionary (since the target does not exist it is created)
        CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
        CFDictionarySetValue(targetDict,kiSCSIPKSendTargetsPortal,sendTargetsPortal);

        // Create list of target portals for a specific target (since the target
        // does not exist it is created)
        CFMutableDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,true);

        // Add portal to newly create list of portals for this target
        CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFDictionarySetValue(portalsList,portalAddress,portalDict);
        CFRelease(portalDict);

        // Set target config
        iSCSIPreferencesSetTargetConfigType(preferences,targetIQN,kiSCSITargetConfigDynamicSendTargets);
    }

    // Ensure target is associated with the specified iSCSI discovery portal
    CFMutableArrayRef targetList = (CFMutableArrayRef)iSCSIPreferencesGetDynamicTargetsForSendTargets(preferences,sendTargetsPortal,true);

    CFIndex idx = 0, targetCount = CFArrayGetCount(targetList);
    for(idx = 0; idx < targetCount; idx++) {
        if(CFStringCompare(targetIQN,CFArrayGetValueAtIndex(targetList,idx),0) == kCFCompareEqualTo)
            break;
    }

    // Target was not associated with the discovery portal, add it
    if(idx == targetCount) {
        CFArrayAppendValue(targetList,targetIQN);
    }
}

void iSCSIPreferencesRemoveTarget(iSCSIPreferencesRef preferences,
                         CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetsList = iSCSIPreferencesGetTargets(preferences,false);
    
    if(targetsList) {
        CFDictionaryRemoveValue(targetsList,targetIQN);
    }

    // Remove CHAP secret if one exists
    iSCSIKeychainDeleteCHAPSecretForNode(targetIQN);
}

/*! Copies the initiator name from iSCSI preferences to a CFString object.
 *  @return the initiator name. */
CFStringRef iSCSIPreferencesCopyInitiatorIQN(iSCSIPreferencesRef preferences)
{
    CFMutableDictionaryRef initiatorDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKInitiator);
    
    if(!initiatorDict) {
        initiatorDict = iSCSIPreferencesCreateInitiatorDict();
    }
    
    // Lookup and copy the initiator name from the dictionary
    CFStringRef initiatorIQN = CFStringCreateCopy(
        kCFAllocatorDefault,CFDictionaryGetValue(initiatorDict,kiSCSIPKInitiatorIQN));
    
    return initiatorIQN;
}

/*! Sets the initiator name in iSCSI preferences.
 *  @param initiatorIQN the initiator name to set. */
void iSCSIPreferencesSetInitiatorIQN(iSCSIPreferencesRef preferences,CFStringRef initiatorIQN)
{
    CFMutableDictionaryRef initiatorDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKInitiator);
    
    if(!initiatorDict) {
        initiatorDict = iSCSIPreferencesCreateInitiatorDict();
        CFDictionarySetValue(preferences,kiSCSIPKInitiator,initiatorDict);
    }

    // Update keychain if necessary
    CFStringRef existingIQN = iSCSIPreferencesCopyInitiatorIQN(preferences);

    CFStringRef secret = iSCSIKeychainCopyCHAPSecretForNode(existingIQN);

    if(secret) {
        iSCSIKeychainSetCHAPSecretForNode(initiatorIQN,secret);
        CFRelease(secret);
        iSCSIKeychainDeleteCHAPSecretForNode(existingIQN);
    }

    // Update initiator name
    CFDictionarySetValue(initiatorDict,kiSCSIPKInitiatorIQN,initiatorIQN);
    CFRelease(existingIQN);
}

/*! Copies the initiator alias from iSCSI preferences to a CFString object.
 *  @return the initiator alias. */
CFStringRef iSCSIPreferencesCopyInitiatorAlias(iSCSIPreferencesRef preferences)
{
    CFMutableDictionaryRef initiatorDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKInitiator);
    
    if(!initiatorDict) {
        initiatorDict = iSCSIPreferencesCreateInitiatorDict();
    }
    
    // Lookup and copy the initiator alias from the dictionary
    CFStringRef initiatorAlias = CFStringCreateCopy(
        kCFAllocatorDefault,CFDictionaryGetValue(initiatorDict,kiSCSIPKInitiatorAlias));
    
    return initiatorAlias;
}

/*! Sets the initiator alias in iSCSI preferences.
 *  @param initiatorAlias the initiator alias to set. */
void iSCSIPreferencesSetInitiatorAlias(iSCSIPreferencesRef preferences,CFStringRef initiatorAlias)
{
    CFMutableDictionaryRef initiatorDict = (CFMutableDictionaryRef)CFDictionaryGetValue(preferences,kiSCSIPKInitiator);
    
    if(!initiatorDict) {
        initiatorDict = iSCSIPreferencesCreateInitiatorDict();
        CFDictionarySetValue(preferences,kiSCSIPKInitiator,initiatorDict);
    }
    
    // Update initiator alias
    CFDictionarySetValue(initiatorDict,kiSCSIPKInitiatorAlias,initiatorAlias);
}

/*! Gets whether a target is defined in preferences.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return true if the target exists, false otherwise. */
Boolean iSCSIPreferencesContainsTarget(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    CFDictionaryRef targetsList = iSCSIPreferencesGetTargets(preferences,true);
    return CFDictionaryContainsKey(targetsList,targetIQN);
}

/*! Gets whether a portal is defined in preferences.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param portalAddress the name of the portal.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPreferencesContainsPortalForTarget(iSCSIPreferencesRef preferences,
                                       CFStringRef targetIQN,
                                       CFStringRef portalAddress)
{
    CFDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,false);
    return (portalsList && CFDictionaryContainsKey(portalsList,portalAddress));
}

/*! Gets whether a SendTargets discovery portal is defined in preferences.
 *  @param portalAddress the discovery portal address.
 *  @return true if the portal exists, false otherwise. */
Boolean iSCSIPreferencesContainsPortalForSendTargetsDiscovery(iSCSIPreferencesRef preferences,
                                                     CFStringRef portalAddress)
{
    Boolean exists = false;
    CFDictionaryRef discoveryPortals = iSCSIPreferencesGetSendTargetsDiscoveryPortals(preferences,false);

    if(discoveryPortals)
        exists = CFDictionaryContainsKey(discoveryPortals,portalAddress);

    return exists;
}

/*! Creates an array of target iSCSI qualified name (IQN)s
 *  defined in preferences.
 *  @return an array of target iSCSI qualified name (IQN)s. */
CFArrayRef iSCSIPreferencesCreateArrayOfTargets(iSCSIPreferencesRef preferences)
{
    CFDictionaryRef targetsList = iSCSIPreferencesGetTargets(preferences,false);
    
    if(!targetsList)
        return NULL;

    const CFIndex keyCount = CFDictionaryGetCount(targetsList);
    
    if(keyCount == 0)
        return NULL;
  
    const void * keys[keyCount];
    CFDictionaryGetKeysAndValues(targetsList,keys,NULL);
    
    return CFArrayCreate(kCFAllocatorDefault,keys,keyCount,&kCFTypeArrayCallBacks);
}

/*! Creates an array of iSCSI targets (IQNs) that were dynamically configured
 *  using SendTargets over a specific discovery portal.
 *  @param portalAddress the address of the discovery portal that corresponds
 *  to the dynamically configured discovery targets.
 *  @return an array of iSCSI targets (IQNs) that were discovered using
 *  SendTargets over the specified portal. */
CFArrayRef iSCSIPreferencesCreateArrayOfDynamicTargetsForSendTargets(iSCSIPreferencesRef preferences,CFStringRef portalAddress)
{
    CFArrayRef targetsList = iSCSIPreferencesGetDynamicTargetsForSendTargets(preferences,portalAddress,false);

    // Create a copy to return
    if(targetsList)
        targetsList = CFArrayCreateCopy(kCFAllocatorDefault,targetsList);

    return targetsList;
}

/*! Creates an array of portal names for a given target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return an array of portal names for the specified target. */
CFArrayRef iSCSIPreferencesCreateArrayOfPortalsForTarget(iSCSIPreferencesRef preferences,
                                                CFStringRef targetIQN)
{
    CFMutableDictionaryRef portalsList = iSCSIPreferencesGetPortalsList(preferences,targetIQN,false);
    
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
void iSCSIPreferencesSetTargetIQN(iSCSIPreferencesRef preferences,
                         CFStringRef existingIQN,
                         CFStringRef newIQN)
{
    // Do not allow a change in IQN for dynamically configured targets
    if(iSCSIPreferencesGetTargetConfigType(preferences,existingIQN) != kiSCSITargetConfigStatic)
        return;
    
    CFMutableDictionaryRef targetNodes = iSCSIPreferencesGetTargets(preferences,false);
    CFMutableDictionaryRef target = iSCSIPreferencesGetTargetDict(preferences,existingIQN,false);

    if(target && targetNodes)
    {
        CFDictionarySetValue(targetNodes,newIQN,target);
        CFDictionaryRemoveValue(targetNodes,existingIQN);

        // Rename keychain if required
        if(iSCSIKeychainContainsCHAPSecretForNode(existingIQN)) {
            CFStringRef secret = iSCSIKeychainCopyCHAPSecretForNode(existingIQN);
            iSCSIKeychainSetCHAPSecretForNode(newIQN,secret);
            CFRelease(secret);

            iSCSIKeychainDeleteCHAPSecretForNode(existingIQN);
        }
    }
}

/*! Sets the alias for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param alias the alias to associate with the specified target. */
void iSCSIPreferencesSetTargetAlias(iSCSIPreferencesRef preferences,
                           CFStringRef targetIQN,
                           CFStringRef alias)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);
    
    if(targetDict)
        CFDictionarySetValue(targetDict,kiSCSIPKTargetAlias,alias);
}

/*! Gets the alias for the specified target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the alias associated with the specified target. */
CFStringRef iSCSIPreferencesGetTargetAlias(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);

    if(targetDict)
        return CFDictionaryGetValue(targetDict,kiSCSIPKTargetAlias);
    
    return kiSCSIUnspecifiedTargetAlias;
}

/*! Sets authentication method to be used by target. */
void iSCSIPreferencesSetTargetAuthenticationMethod(iSCSIPreferencesRef preferences,
                                          CFStringRef targetIQN,
                                          enum iSCSIAuthMethods authMethod)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);

    if(authMethod == kiSCSIAuthMethodNone)
        CFDictionarySetValue(targetDict,kiSCSIPKAuth,kiSCSIPVAuthNone);
    else if(authMethod == kiSCSIAuthMethodCHAP)
        CFDictionarySetValue(targetDict,kiSCSIPKAuth,kiSCSIPVAuthCHAP);
}

/*! Gets the current authentication method used by the target. */
enum iSCSIAuthMethods iSCSIPreferencesGetTargetAuthenticationMethod(iSCSIPreferencesRef preferences,
                                                           CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
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
void iSCSIPreferencesSetTargetConfigType(iSCSIPreferencesRef preferences,
                                CFStringRef targetIQN,
                                enum iSCSITargetConfigTypes configType)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
    CFStringRef configTypeString = NULL;

    switch(configType) {

        case kiSCSITargetConfigStatic:
            configTypeString = kiSCSIPVTargetConfigTypeStatic; break;
        case kiSCSITargetConfigDynamicSendTargets:
            configTypeString = kiSCSIPVTargetConfigTypeDiscovery; break;
        case kiSCSITargetConfigInvalid: break;
    };

    if(configTypeString) {
        // Change CHAP name in iSCSI preferences
        CFDictionarySetValue(targetDict,kiSCSIPKTargetConfigType,configTypeString);
    }
}

/*! Gets target configuration type.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the target configuration type. */
enum iSCSITargetConfigTypes iSCSIPreferencesGetTargetConfigType(iSCSIPreferencesRef preferences,
                                                       CFStringRef targetIQN)
{
    enum iSCSITargetConfigTypes configType = kiSCSITargetConfigInvalid;
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);

    if(targetDict) {
        CFStringRef configTypeString = CFDictionaryGetValue(targetDict,kiSCSIPKTargetConfigType);

        // If target exists but configuration string doesn't, assume it is
        // a static target and repair preferences
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

/*! Gets the SendTargets discovery portal associated with the dynamic target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return address of the discovery portal that manages the target. */
CFStringRef iSCSIPreferencesGetDiscoveryPortalForTarget(iSCSIPreferencesRef preferences,
                                               CFStringRef targetIQN)
{
    CFStringRef discoveryPortal = NULL;
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,false);
    
    if(targetDict)
        discoveryPortal = CFDictionaryGetValue(targetDict,kiSCSIPKSendTargetsPortal);
    
    return discoveryPortal;
}

/*! Sets the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @param name the CHAP name associated with the target. */
void iSCSIPreferencesSetTargetCHAPName(iSCSIPreferencesRef preferences,CFStringRef targetIQN,CFStringRef name)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);

    // Change CHAP name in iSCSI preferences
    CFDictionarySetValue(targetDict,kiSCSIPKAuthCHAPName,name);
}

/*! Copies the CHAP name associated with the target.
 *  @param targetIQN the target iSCSI qualified name (IQN).
 *  @return the CHAP name associated with the target. */
CFStringRef iSCSIPreferencesCopyTargetCHAPName(iSCSIPreferencesRef preferences,CFStringRef targetIQN)
{
    CFMutableDictionaryRef targetDict = iSCSIPreferencesGetTargetDict(preferences,targetIQN,true);
    CFStringRef name = CFDictionaryGetValue(targetDict,kiSCSIPKAuthCHAPName);
    return CFStringCreateCopy(kCFAllocatorDefault,name);
}

/*! Adds an iSCSI discovery portal to the list of discovery portals.
 *  @param portal the discovery portal to add. */
void iSCSIPreferencesAddSendTargetsDiscoveryPortal(iSCSIPreferencesRef preferences,iSCSIPortalRef portal)
{
    CFMutableDictionaryRef discoveryPortals = iSCSIPreferencesGetSendTargetsDiscoveryPortals(preferences,true);

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
        }
    }
}

/*! Removes the specified iSCSI discovery portal.
 *  @param portal the discovery portal to remove. */
void iSCSIPreferencesRemoveSendTargetsDiscoveryPortal(iSCSIPreferencesRef preferences,iSCSIPortalRef portal)
{
    CFMutableDictionaryRef discoveryPortals = iSCSIPreferencesGetSendTargetsDiscoveryPortals(preferences,false);

    if(discoveryPortals && portal) {

        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

        if(CFDictionaryContainsKey(discoveryPortals,portalAddress)) {

            // Remove all dynamic targets associated with this portal, if any...
            CFArrayRef targetsList = iSCSIPreferencesGetDynamicTargetsForSendTargets(preferences,portalAddress,false);

            if(targetsList) {
                CFIndex count = CFArrayGetCount(targetsList);

                for(CFIndex idx = 0; idx < count; idx++)
                {
                    CFStringRef targetIQN = CFArrayGetValueAtIndex(targetsList,idx);
                    iSCSIPreferencesRemoveTarget(preferences,targetIQN);
                }
            }

            // Remove discovery portal from SendTargets dictionary
            CFDictionaryRemoveValue(discoveryPortals,portalAddress);
        }
    }
}

/*! Copies a portal object for the specified discovery portal.
 *  @param poralAddress the portal name (IPv4, IPv6 or DNS name).
 *  @return portal the portal object to set. */
iSCSIPortalRef iSCSIPreferencesCopySendTargetsDiscoveryPortal(iSCSIPreferencesRef preferences,CFStringRef portalAddress)
{
    iSCSIMutablePortalRef portal = NULL;

    CFMutableDictionaryRef discoveryPortals = iSCSIPreferencesGetSendTargetsDiscoveryPortals(preferences,false);

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
 *  corresponding portal object by calling iSCSIPreferencesCopySendTargetsPortal(). */
CFArrayRef iSCSIPreferencesCreateArrayOfPortalsForSendTargetsDiscovery(iSCSIPreferencesRef preferences)
{
    CFMutableDictionaryRef discoveryPortals = iSCSIPreferencesGetSendTargetsDiscoveryPortals(preferences,false);

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
void iSCSIPreferencesSetSendTargetsDiscoveryEnable(iSCSIPreferencesRef preferences,Boolean enable)
{
    CFMutableDictionaryRef discoveryDict = iSCSIPreferencesGetDiscoveryDict(preferences,true);

    if(enable)
        CFDictionarySetValue(discoveryDict,kiSCSIPKSendTargetsEnabled,kCFBooleanTrue);
    else
        CFDictionarySetValue(discoveryDict,kiSCSIPKSendTargetsEnabled,kCFBooleanFalse);
}

/*! Gets whether SendTargets discovery is set ot disabled or enabled.
 *  @return True if send targets discovery is set to enabled, false otherwise. */
Boolean iSCSIPreferencesGetSendTargetsDiscoveryEnable(iSCSIPreferencesRef preferences)
{
    CFMutableDictionaryRef discoveryDict = iSCSIPreferencesGetDiscoveryDict(preferences,true);
    return CFBooleanGetValue(CFDictionaryGetValue(discoveryDict,kiSCSIPKSendTargetsEnabled));
}

/*! Sets SendTargets discovery interval.
 *  @param interval the discovery interval, in seconds. */
void iSCSIPreferencesSetSendTargetsDiscoveryInterval(iSCSIPreferencesRef preferences,CFIndex interval)
{
    CFMutableDictionaryRef discoveryDict = iSCSIPreferencesGetDiscoveryDict(preferences,true);
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault,kCFNumberCFIndexType,&interval);
    CFDictionarySetValue(discoveryDict,kiSCSIPKDiscoveryInterval,value);
}

/*! Gets SendTargets disocvery interval.
 *  @return the discovery interval, in seconds. */
CFIndex iSCSIPreferencesGetSendTargetsDiscoveryInterval(iSCSIPreferencesRef preferences)
{
    CFIndex interval = 0;
    CFMutableDictionaryRef discoveryDict = iSCSIPreferencesGetDiscoveryDict(preferences,true);
    CFNumberRef value = CFDictionaryGetValue(discoveryDict,kiSCSIPKDiscoveryInterval);
    CFNumberGetValue(value,kCFNumberCFIndexType,&interval);
    return interval;
}

/*! Resets iSCSI preferences, removing all defined targets and
 *  configuration parameters. */
void iSCSIPreferencesReset(iSCSIPreferencesRef preferences)
{
    CFDictionaryRemoveAllValues(preferences);
}

/*! Create a dictionary representation of the preferences object.
 *  @param preferences the iSCSI preferences object.
 *  @return a dictionary representation containing key-value pairs of preferences values. */
CFDictionaryRef iSCSIPreferencesCreateDictionary(iSCSIPreferencesRef preferences)
{
    return (CFDictionaryRef)preferences;
}

/*! Creates a binary representation of an iSCSI preferences object.
 *  @param data the data used to create the preferences object.
 *  @return a binary data representation of a preferences object. */
CFDataRef iSCSIPreferencesCreateData(iSCSIPreferencesRef preferences)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,
                                    preferences,
                                    kCFPropertyListBinaryFormat_v1_0,0,NULL);
}

/*! Creates a new iSCSI preferences object.
 *  @return a new (empty) preferences object. */
iSCSIPreferencesRef iSCSIPreferencesCreate()
{
    iSCSIPreferencesRef preferences = CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                                                &kCFTypeDictionaryKeyCallBacks,
                                                                &kCFTypeDictionaryValueCallBacks);
    return preferences;
}

/*! Creates a new iSCSI preferences object from using values stored in system preferences.
 *  @param kAppId the application identification string.
 *  @return a new preferences object containing stored iSCSI preferences. */
iSCSIPreferencesRef iSCSIPreferencesCreateFromAppValues()
{
    iSCSIPreferencesRef preferences = iSCSIPreferencesCreate();
    iSCSIPreferencesUpdateWithAppValues(preferences);
    return preferences;
}

/*! Creates a new iSCSI preferences object from a dictionary object from a dictionary representation.
 *  @param dict a dictionary used to construct an iSCSI preferences object.
 *  @return an iSCSI preferences object or NULL if object creation failed. */
iSCSIPreferencesRef iSCSIPreferencesCreateWithDictionary(CFDictionaryRef dict)
{
    if(!dict)
        return NULL;
    
    // Create a deep copy to make the dictionary mutable
    CFMutableDictionaryRef mutablePropertyList = (CFMutableDictionaryRef)CFPropertyListCreateDeepCopy(
            kCFAllocatorDefault,dict,kCFPropertyListMutableContainersAndLeaves);
    
    return mutablePropertyList;
}

/*! Creates a new iSCSI preferences object from binary data.
 *  @param data the data used to create the preferences object.
 *  @return a new preferences object containing stored iSCSI preferences. */
iSCSIPreferencesRef iSCSIPreferencesCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    
    iSCSIPreferencesRef preferences = (iSCSIPreferencesRef)CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return preferences;
    
    CFRelease(preferences);
    return NULL;
}

/*! Updates the contents of the iSCSI preferences with application values.
 *  @param preferences an iSCSI preferences object. */
void iSCSIPreferencesUpdateWithAppValues(iSCSIPreferencesRef preferences)
{
    // Refresh from preferences
    CFDictionaryRef dict = NULL;
    
    dict = iSCSIPreferencesCopyPropertyDict(kiSCSIPKAppId,kiSCSIPKInitiator);
    
    if(dict) {
        CFDictionarySetValue(preferences,kiSCSIPKInitiator,dict);
        CFRelease(dict);
        dict = NULL;
    }
    
    dict = iSCSIPreferencesCopyPropertyDict(kiSCSIPKAppId,kiSCSIPKTargets);
    
    if(dict) {
        CFDictionarySetValue(preferences,kiSCSIPKTargets,dict);
        CFRelease(dict);
        dict = NULL;
    }
    
    dict = iSCSIPreferencesCopyPropertyDict(kiSCSIPKAppId,kiSCSIPKDiscovery);
    
    if(dict) {
        CFDictionarySetValue(preferences,kiSCSIPKDiscovery,dict);
        CFRelease(dict);
        dict = NULL;
    }
}

/*! Synchronizes application values with those in the preferences object.
 *  @param preferences an iSCSI preferences object.
 *  @return true if application values were successfully updated. */
Boolean iSCSIPreferencesSynchronzeAppValues(iSCSIPreferencesRef preferences)
{
    CFPreferencesSetMultiple((CFDictionaryRef)preferences,0,kiSCSIPKAppId,kCFPreferencesAnyUser,kCFPreferencesCurrentHost);
    return CFPreferencesSynchronize(kiSCSIPKAppId,kCFPreferencesAnyUser,kCFPreferencesCurrentHost);
}



/*! Releasese an iSCSI preferences object.
 *  @param preferences the preferences object to release. */
void iSCSIPreferencesRelease(iSCSIPreferencesRef preferences)
{
    CFRelease(preferences);
}
