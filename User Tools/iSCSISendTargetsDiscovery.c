/*!
 * @author		Nareg Sinenian
 * @file		iSCSISendTargetsDiscovery.c
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		Discovery functions for use by iscsid.
 */

#include "iSCSISendTargetsDiscovery.h"

/*! Defined in iSCSIDaemon.c and used to log messages. */
extern void iSCSIDLogError(CFStringRef logEntry);

errno_t iSCSISendTargetsAddTarget(CFStringRef targetIQN,
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

errno_t iSCSISendTargetsProcessResults(CFStringRef discoveryPortal,
                                       iSCSIDiscoveryRecRef discoveryRec)
{
    CFArrayRef targets = iSCSIDiscoveryRecCreateArrayOfTargets(discoveryRec);
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
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("discovered target %@ already exists with static configuration."),
                targetIQN);
            
            iSCSIDLogError(errorString);
            CFRelease(errorString);
        }
        // Target doesn't exist, or target exists with SendTargets
        // configuration (add or update as necessary)
        else {
            iSCSISendTargetsAddTarget(targetIQN,discoveryRec,discoveryPortal);
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
        if(!CFDictionaryContainsKey(discTargets,targetIQN))
            iSCSIPLRemoveTarget(targetIQN);
    }

    CFRelease(discTargets);
    CFRelease(existingTargets);

    iSCSIPLSynchronize();
    return 0;
}

errno_t iSCSISendTargetsRunDiscovery()
{
    // Obtain a list of SendTargets portals from the property list
    iSCSIPLSynchronize();

    CFArrayRef portals = iSCSIPLCreateArrayOfPortalsForSendTargetsDiscovery();
    CFIndex portalCount = CFArrayGetCount(portals);

    CFStringRef discoveryPortal = NULL;
    iSCSIPortalRef portal = NULL;

    for(CFIndex idx = 0; idx < portalCount; idx++)
    {
        discoveryPortal = CFArrayGetValueAtIndex(portals,idx);
        portal = iSCSIPLCopySendTargetsDiscoveryPortal(discoveryPortal);
        enum iSCSILoginStatusCode statusCode;
        iSCSIMutableDiscoveryRecRef discoveryRec;

        // If there was an error, log it and move on to the next portal
        errno_t error = 0;
        if((error = iSCSIQueryPortalForTargets(portal,0,&discoveryRec,&statusCode)))
        {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("system error (code %d) occurred during SendTargets discovery of %@."),
                error,discoveryPortal);

            iSCSIDLogError(errorString);
            CFRelease(errorString);
        }
        else if(statusCode != kiSCSILoginSuccess) {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("login failed with (code %d) during SendTargets discovery of %@."),
                statusCode,discoveryPortal);
            
            iSCSIDLogError(errorString);
            CFRelease(errorString);
        }
        
        // Now parse discovery results, add new targets and remove stale targets
        iSCSISendTargetsProcessResults(discoveryPortal,discoveryRec);
        iSCSIDiscoveryRecRelease(discoveryRec);
    }
    return 0;
}