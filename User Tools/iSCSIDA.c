/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDA.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI disks.
 */

#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAUnmountApplierFunc(io_object_t entry, void * context)
{
    DASessionRef session = (DASessionRef)context;
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,session,entry);
    DADiskUnmount(disk,kDADiskUnmountOptionWhole,NULL,NULL);
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
    iSCSIIORegistryIOMediaApplyFunction(target,&iSCSIDAUnmountApplierFunc,diskArbSession);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,1,true);
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAMountApplierFunc(io_object_t entry, void * context)
{
    DASessionRef session = (DASessionRef)context;
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,session,entry);
    DADiskMount(disk,NULL,kDADiskMountOptionWhole,NULL,NULL);
}

/*! Mounts all IOMedia associated with a particular iSCSI session.
 *  @param targetIQN the name of the iSCSI target. */
void iSCSIDAMountIOMediaForTarget(CFStringRef targetIQN)
{
    // Create a disk arbitration session and associate it with current runloop
    DASessionRef diskArbSession = DASessionCreate(kCFAllocatorDefault);
    DASessionScheduleWithRunLoop(diskArbSession,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
    
    // Find the target associated with the session
    io_object_t target = iSCSIIORegistryGetTargetEntry(targetIQN);
    
    // Queue mount all IOMedia objects & run CFRunLoop
    iSCSIIORegistryIOMediaApplyFunction(target,&iSCSIDAMountApplierFunc,diskArbSession);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,1,true);
}


/*! Rescans the iSCSI bus (target) for new LUNs. */
void iSCSIDARescanBus()
{
    io_object_t iSCSIHBA = iSCSIIORegistryGetiSCSIHBAEntry();
    
    if(iSCSIHBA == IO_OBJECT_NULL)
        return;

    IOServiceRequestProbe(iSCSIHBA,0);
    IOObjectRelease(iSCSIHBA);
}

/*! Rescans the iSCSI bus for new LUNs for a particular target only.
 *  @param tagetIQN the name of the iSCSI target. */
void iSCSIDARescanBusForTarget(CFStringRef targetIQN)
{
    io_object_t target = iSCSIIORegistryGetTargetEntry(targetIQN);
    
    if(target == IO_OBJECT_NULL)
        return;
    
    IOServiceRequestProbe(target,0);
    IOObjectRelease(target);
}


