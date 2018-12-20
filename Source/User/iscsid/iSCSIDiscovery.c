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

#include "iSCSIDiscovery.h"

errno_t iSCSIDiscoveryAddTargetForSendTargets(iSCSIPreferencesRef preferences,
                                              CFStringRef targetIQN,
                                              iSCSIDiscoveryRecRef discoveryRec,
                                              CFStringRef discoveryPortal)
{
    CFArrayRef portalGroups = iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(discoveryRec,targetIQN);
    CFIndex portalGroupCount = CFArrayGetCount(portalGroups);

    // Iterate over portal groups for this target
    for(CFIndex portalGroupIdx = 0; portalGroupIdx < portalGroupCount; portalGroupIdx++)
    {
        CFStringRef portalGroupTag = CFArrayGetValueAtIndex(portalGroups,portalGroupIdx);
        CFArrayRef portals = iSCSIDiscoveryRecGetPortals(discoveryRec,targetIQN,portalGroupTag);
        CFIndex portalsCount = CFArrayGetCount(portals);

        iSCSIPortalRef portal = NULL;

        // Iterate over portals within this group
        for(CFIndex portalIdx = 0; portalIdx < portalsCount; portalIdx++)
        {
            if(!(portal = CFArrayGetValueAtIndex(portals,portalIdx)))
               continue;

            // Add portal to target, or add target as necessary
            if(iSCSIPreferencesContainsTarget(preferences,targetIQN))
                iSCSIPreferencesSetPortalForTarget(preferences,targetIQN,portal);
            else
                iSCSIPreferencesAddDynamicTargetForSendTargets(preferences,targetIQN,portal,discoveryPortal);
        }
    }
    
    CFRelease(portalGroups);

    return 0;
}

/*! Updates an iSCSI preference sobject with information about targets as
 *  contained in the provided discovery record.
 *  @param preferences an iSCSI preferences object.
 *  @param discoveryPortal the portal (address) that was used to perform discovery.
 *  @param discoveryRec the discovery record resulting from the discovery operation.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIDiscoveryUpdatePreferencesWithDiscoveredTargets(iSCSISessionManagerRef managerRef,
                                                             iSCSIPreferencesRef preferences,
                                                             CFStringRef discoveryPortal,
                                                             iSCSIDiscoveryRecRef discoveryRec)
{
    CFArrayRef targets = iSCSIDiscoveryRecCreateArrayOfTargets(discoveryRec);
    
    if(!targets)
        return EINVAL;
    
    CFIndex targetCount = CFArrayGetCount(targets);

    CFMutableDictionaryRef discTargets = CFDictionaryCreateMutable(
        kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,0);

    for(CFIndex targetIdx = 0; targetIdx < targetCount; targetIdx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(targets,targetIdx);

        // Target exists with static (or other configuration).  In
        // this case we do nothing, log a message and move on.
        if(iSCSIPreferencesContainsTarget(preferences,targetIQN) &&
           iSCSIPreferencesGetTargetConfigType(preferences,targetIQN) != kiSCSITargetConfigDynamicSendTargets)
        {
            CFStringRef statusString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("discovered target %@ already exists with static configuration."),
                targetIQN);

            CFIndex statusStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(statusString),kCFStringEncodingASCII) + sizeof('\0');
            char statusStringBuffer[statusStringLength];
            CFStringGetCString(statusString,statusStringBuffer,statusStringLength,kCFStringEncodingASCII);

            asl_log(NULL, NULL, ASL_LEVEL_INFO, "%s", statusStringBuffer);
            
            CFRelease(statusString);
        }
        // Target doesn't exist, or target exists with SendTargets
        // configuration (add or update as necessary)
        else {
            iSCSIDiscoveryAddTargetForSendTargets(preferences,targetIQN,discoveryRec,discoveryPortal);
            CFStringRef statusString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("discovered target %@ over discovery portal %@."),
                targetIQN,discoveryPortal);

            CFIndex statusStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(statusString),kCFStringEncodingASCII) + sizeof('\0');
            char statusStringBuffer[statusStringLength];
            CFStringGetCString(statusString,statusStringBuffer,statusStringLength,kCFStringEncodingASCII);
            
            asl_log(NULL, NULL, ASL_LEVEL_INFO, "%s", statusStringBuffer);
            
            CFRelease(statusString);
        }

        // As we process each target we'll add it to a temporary dictionary
        // for cross-checking against targets that exist in our database
        // which have been removed.
        CFDictionaryAddValue(discTargets,targetIQN,0);
    }

    // Are there any targets that must be removed?  Cross-check existing
    // list against the list we just built...
    CFArrayRef existingTargets = iSCSIPreferencesCreateArrayOfDynamicTargetsForSendTargets(preferences,discoveryPortal);
    targetCount = CFArrayGetCount(existingTargets);

    for(CFIndex targetIdx = 0; targetIdx < targetCount; targetIdx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(existingTargets,targetIdx);

        // If we have a target that was not discovered, then we need to remove
        // it from our property list...
        if(!CFDictionaryContainsKey(discTargets,targetIQN)) {

            // If the target is logged in, logout of the target and remove it
            SessionIdentifier sessionId = iSCSISessionGetSessionIdForTarget(managerRef,targetIQN);
            enum iSCSILogoutStatusCode statusCode;
            if(sessionId != kiSCSIInvalidSessionId)
                iSCSISessionLogout(managerRef,sessionId,&statusCode);

            iSCSIPreferencesRemoveTarget(preferences,targetIQN);
        }
    }

    CFRelease(targets);
    CFRelease(discTargets);
    CFRelease(existingTargets);
    
    return 0;
}

/*! Scans all iSCSI discovery portals found in iSCSI preferences
 *  for targets (SendTargets). Returns a dictionary of key-value pairs
 *  with discovery record objects as values and discovery portal names
 *  as keys.
 *  @param preferences an iSCSI preferences object.
 *  @return a dictionary key-value pairs of dicovery portal names (addresses)
 *  and the discovery records associated with the result of SendTargets
 *  discovery of those portals. */
CFDictionaryRef iSCSIDiscoveryCreateRecordsWithSendTargets(iSCSISessionManagerRef managerRef,
                                                           iSCSIPreferencesRef preferences)
{
    if(!preferences)
        return NULL;
    
    CFArrayRef portals = iSCSIPreferencesCreateArrayOfPortalsForSendTargetsDiscovery(preferences);
    
    // Quit if no discovery portals are defined
    if(!portals)
        return NULL;
    
    CFIndex portalCount = CFArrayGetCount(portals);

    CFStringRef discoveryPortal = NULL;
    iSCSIPortalRef portal = NULL;

    CFMutableDictionaryRef discoveryRecords = CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                                                        &kiSCSITypeDictionaryKeyCallbacks,
                                                                        &kiSCSITypeDictionaryValueCallbacks);

    for(CFIndex idx = 0; idx < portalCount; idx++)
    {
        discoveryPortal = CFArrayGetValueAtIndex(portals,idx);
        
        if(!discoveryPortal)
            continue;
        
        portal = iSCSIPreferencesCopySendTargetsDiscoveryPortal(preferences,discoveryPortal);
        
        if(!portal)
            continue;
        
        enum iSCSILoginStatusCode statusCode;
        iSCSIMutableDiscoveryRecRef discoveryRec;

        // If there was an error, log it and move on to the next portal
        errno_t error = 0;
        iSCSIAuthRef auth = iSCSIAuthCreateNone();
        if((error = iSCSIQueryPortalForTargets(managerRef,portal,auth,&discoveryRec,&statusCode)))
        {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("system error (code %d) occurred during SendTargets discovery of %@."),
                error,discoveryPortal);

            CFIndex errorStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(errorString),kCFStringEncodingASCII) + sizeof('\0');
            char errorStringBuffer[errorStringLength];
            CFStringGetCString(errorString,errorStringBuffer,errorStringLength,kCFStringEncodingASCII);
            
            asl_log(NULL, NULL, ASL_LEVEL_ERR, "%s", errorStringBuffer);
            
            CFRelease(errorString);
        }
        else if(statusCode != kiSCSILoginSuccess) {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("login failed with (code %d) during SendTargets discovery of %@."),
                statusCode,discoveryPortal);
            
            CFIndex errorStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(errorString),kCFStringEncodingASCII) + sizeof('\0');
            char errorStringBuffer[errorStringLength];
            CFStringGetCString(errorString,errorStringBuffer,errorStringLength,kCFStringEncodingASCII);

            asl_log(NULL, NULL, ASL_LEVEL_ERR, "%s", errorStringBuffer);
            
            CFRelease(errorString);
        }
        else {
            // Queue discovery record so that it can be processed later
            if(discoveryRec) {
                CFDictionarySetValue(discoveryRecords,discoveryPortal,discoveryRec);
                iSCSIDiscoveryRecRelease(discoveryRec);
            }
        }
        
        iSCSIAuthRelease(auth);
        iSCSIPortalRelease(portal);
    }
    
    // Release the array of discovery portals
    CFRelease(portals);
    
    return discoveryRecords;
}
