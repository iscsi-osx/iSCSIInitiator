/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDA.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI disks.
 */

#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"

void iSCSIDAUnmountCallback(DADiskRef disk,DADissenterRef dissenter,void *context)
{
    CFRunLoopStop(CFRunLoopGetCurrent());
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAUnmountApplierFunc(io_object_t entry, void * context)
{
    DASessionRef session = (DASessionRef)context;
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,session,entry);
    DADiskUnmount(disk,kDADiskUnmountOptionWhole,iSCSIDAUnmountCallback,NULL);
}

/*! Unmounts all IOMedia associated with a particular iSCSI session.
 *  @param targetIQN the name of the iSCSI target. */
void iSCSIDAUnmountIOMediaForTarget(CFStringRef targetIQN)
{
    // Create a disk arbitration session and associate it with current runloop
    DASessionRef diskArbSession = DASessionCreate(kCFAllocatorDefault);
    DASessionScheduleWithRunLoop(diskArbSession,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
    
    // Find the target associated with the session
    io_object_t target = iSCSIIORegistryGetTargetEntry(targetIQN);

    // Queue unmount all IOMedia objects & run CFRunLoop
    if(target != IO_OBJECT_NULL) {
        iSCSIIORegistryIOMediaApplyFunction(target,&iSCSIDAUnmountApplierFunc,diskArbSession);
        CFRunLoopRun();
    }
    
    CFRelease(diskArbSession);
}

void iSCSIDAMountCallback(DADiskRef disk,DADissenterRef dissenter,void *context)
{
    CFRunLoopStop(CFRunLoopGetCurrent());
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAMountApplierFunc(io_object_t entry, void * context)
{
    DASessionRef session = (DASessionRef)context;
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,session,entry);
    DADiskMount(disk,NULL,kDADiskMountOptionWhole,iSCSIDAMountCallback,NULL);
}

/*! Mounts all IOMedia associated with a particular iSCSI session.
 *  @param targetIQN the name of the iSCSI target. */
void iSCSIDAMountIOMediaForTarget(CFStringRef targetIQN)
{
    // Create a disk arbitration session and associate it with current runloop
    DASessionRef diskArbSession = DASessionCreate(kCFAllocatorDefault);
    DASessionScheduleWithRunLoop(diskArbSession,CFRunLoopGetCurrent(),kCFRunLoopCommonModes);
    
    // Find the target associated with the session
    io_object_t target = iSCSIIORegistryGetTargetEntry(targetIQN);
    
    // Queue mount all IOMedia objects & run CFRunLoop
    if(target != IO_OBJECT_NULL) {
        iSCSIIORegistryIOMediaApplyFunction(target,&iSCSIDAMountApplierFunc,diskArbSession);
        CFRunLoopRun();
    }
    
    CFRelease(diskArbSession);
}

