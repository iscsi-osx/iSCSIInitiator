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

errno_t iSCSIDiscoveryAddTargetForSendTargets(CFStringRef targetIQN,
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
            if(iSCSIPLContainsTarget(targetIQN))
                iSCSIPLSetPortalForTarget(targetIQN,portal);
            else
                iSCSIPLAddDynamicTargetForSendTargets(targetIQN,portal,discoveryPortal);
        }
    }

    return 0;
}

errno_t iSCSIDiscoveryProcessSendTargetsResults(CFStringRef discoveryPortal,
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
        if(iSCSIPLContainsTarget(targetIQN) &&
           iSCSIPLGetTargetConfigType(targetIQN) != kiSCSITargetConfigDynamicSendTargets)
        {
            CFStringRef statusString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("discovered target %@ already exists with static configuration."),
                targetIQN);

            asl_log(NULL,NULL,ASL_LEVEL_INFO,"%s",CFStringGetCStringPtr(statusString,kCFStringEncodingASCII));

            CFRelease(statusString);
        }
        // Target doesn't exist, or target exists with SendTargets
        // configuration (add or update as necessary)
        else {
            iSCSIDiscoveryAddTargetForSendTargets(targetIQN,discoveryRec,discoveryPortal);
            CFStringRef statusString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("discovered target %@ over discovery portal %@."),
                targetIQN,discoveryPortal);

            asl_log(NULL,NULL,ASL_LEVEL_INFO,"%s",CFStringGetCStringPtr(statusString,kCFStringEncodingASCII));

            CFRelease(statusString);
        }

        // As we process each target we'll add it to a temporary dictionary
        // for cross-checking against targets that exist in our database
        // which have been removed.
        CFDictionaryAddValue(discTargets,targetIQN,0);
    }

    // Are there any targets that must be removed?  Cross-check existing
    // list against the list we just built...
    CFArrayRef existingTargets = iSCISPLCreateArrayOfDynamicTargetsForSendTargets(discoveryPortal);
    targetCount = CFArrayGetCount(existingTargets);

    for(CFIndex targetIdx = 0; targetIdx < targetCount; targetIdx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(existingTargets,targetIdx);

        // If we have a target that was not discovered, then we need to remove
        // it from our property list...
        if(!CFDictionaryContainsKey(discTargets,targetIQN)) {

            // If the target is logged in, logout of the target and remove it
            SID sessionId = iSCSIGetSessionIdForTarget(targetIQN);
            enum iSCSILogoutStatusCode statusCode;
            if(sessionId != kiSCSIInvalidSessionId)
                iSCSILogoutSession(sessionId,&statusCode);

            iSCSIPLRemoveTarget(targetIQN);
        }
    }

    CFRelease(discTargets);
    CFRelease(existingTargets);

    iSCSIPLSynchronize();
    return 0;
}

void iSCSIDiscoveryRunSendTargets()
{
    // Obtain a list of SendTargets portals from the property list
    iSCSIPLSynchronize();

    CFArrayRef portals = iSCSIPLCreateArrayOfPortalsForSendTargetsDiscovery();
    
    // Quit if no discovery portals are defined
    if(!portals)
        return;
    
    CFIndex portalCount = CFArrayGetCount(portals);

    CFStringRef discoveryPortal = NULL;
    iSCSIPortalRef portal = NULL;

    for(CFIndex idx = 0; idx < portalCount; idx++)
    {
        discoveryPortal = CFArrayGetValueAtIndex(portals,idx);
        
        if(!discoveryPortal)
            continue;
        
        portal = iSCSIPLCopySendTargetsDiscoveryPortal(discoveryPortal);
        
        if(!portal)
            continue;
        
        enum iSCSILoginStatusCode statusCode;
        iSCSIMutableDiscoveryRecRef discoveryRec;

        // If there was an error, log it and move on to the next portal
        errno_t error = 0;
        if((error = iSCSIQueryPortalForTargets(portal,iSCSIAuthCreateNone(),&discoveryRec,&statusCode)))
        {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("system error (code %d) occurred during SendTargets discovery of %@."),
                error,discoveryPortal);

            asl_log(NULL,NULL,ASL_LEVEL_ERR,"%s",CFStringGetCStringPtr(errorString,kCFStringEncodingASCII));
            CFRelease(errorString);
        }
        else if(statusCode != kiSCSILoginSuccess) {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("login failed with (code %d) during SendTargets discovery of %@."),
                statusCode,discoveryPortal);
            
            asl_log(NULL,NULL,ASL_LEVEL_ERR,"%s",CFStringGetCStringPtr(errorString,kCFStringEncodingASCII));
            CFRelease(errorString);
        }
        else {
            // Now parse discovery results, add new targets and remove stale targets
            if(discoveryRec) {
                iSCSIDiscoveryProcessSendTargetsResults(discoveryPortal,discoveryRec);
                iSCSIDiscoveryRecRelease(discoveryRec);
            }
        }
    }
    
    // Release the array of discovery portals
    CFRelease(portals);

    iSCSIPLSynchronize();
}
