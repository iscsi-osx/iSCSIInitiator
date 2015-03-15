/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDA.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI disks.
 */

#include "iSCSIDA.h"

/*! Helper function. Finds the target object (IOSCSITargetDevice) in the IO
 *  registry that corresponds to the session Id.
 *  @param sessionId the session identifier.
 *  @return the IO registry object of the IOSCSITargetDevice for this session. */
io_object_t iSCSIDAFindTargetForSession(SID sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return IO_OBJECT_NULL;
    
    io_service_t service;
    
    // Create a dictionary to match the iSCSI initiator kernel extension
    CFMutableDictionaryRef matchingDict = NULL;
    matchingDict = IOServiceMatching("com_NSinenian_iSCSIVirtualHBA");
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault,matchingDict);
    
    // Check to see if the initiator kext (driver) was found in the I/O registry
    if(service == IO_OBJECT_NULL)
        return IO_OBJECT_NULL;
    
    // Iterate over the targets and find the specified session identifier
    // (the target identifier is the same as the session identifier by design)
    io_iterator_t iterator = IO_OBJECT_NULL;
    IORegistryEntryGetChildIterator(service,kIOServicePlane,&iterator);
    
    io_object_t entry;
    while((entry = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        // Grab this entry's protocol characteristics list, if it has one
        CFTypeRef data = IORegistryEntryCreateCFProperty(entry,
                                                         CFSTR("Protocol Characteristics"),
                                                         kCFAllocatorDefault,0);
        
        if(data && CFGetTypeID(data) == CFDictionaryGetTypeID())
        {
            // Get the target identifier and compare to the session identifier
            // (they are one and the same by design, if this target corresponds
            // to the specified session)
            CFNumberRef targetId = CFDictionaryGetValue(data,CFSTR("SCSI Target Identifier"));
            
            int identifier;
            if(CFNumberGetValue(targetId,kCFNumberIntType,&identifier) && identifier == sessionId)
            {
                // The child is the IOSCSITargetDevice object...
                io_object_t child;
                IORegistryEntryGetChildEntry(entry,kIOServicePlane,&child);
                
                CFRelease(data);

                return child;
            }
        }
        
        if(data)
            CFRelease(data);
    }
    return IO_OBJECT_NULL;
}

/*! Helper function. Finds all IOMedia objects that are children of a particular
 *  target.  A callback function is called on every IOMedia object found.  A
 *  user-supplied context is passed along to the callback function.
 *  @param target search children of this node for IOMedia objects.
 *  @param callback the callback function to call on each IOMedia object.
 *  @param context a user-defined parameter to pass to the callback function. */
void iSCSIDAIOMediaApplyFunction(io_object_t target,
                                 void (*applierFunc)(io_object_t entry,void * context),
                                 void * context)
{
    io_object_t entry = IO_OBJECT_NULL;
    io_iterator_t iterator = IO_OBJECT_NULL;
    IORegistryEntryGetChildIterator(target,kIOServicePlane,&iterator);
    
    // Iterate over all children of the target object
    while((entry = IOIteratorNext(iterator)) != IO_OBJECT_NULL)
    {
        // Recursively call this function for each child of the target
        iSCSIDAIOMediaApplyFunction(entry,applierFunc,context);
        
        // Find the IOMedia's root provider class (IOBlockStorageDriver) and
        // get the first child.  This ensures that we grab the IOMedia object
        // for the disk itself and not each individual partition
        CFStringRef providerClass = IORegistryEntryCreateCFProperty(entry,CFSTR("IOClass"),kCFAllocatorDefault,0);
        
        if(providerClass && CFStringCompare(providerClass,CFSTR("IOBlockStorageDriver"),0) == kCFCompareEqualTo)
        {
            // Apply callback function to the child (the IOMedia object that
            // pertains to the whole disk)
            io_object_t child;
            IORegistryEntryGetChildEntry(entry,kIOServicePlane,&child);
            applierFunc(child,context);
        }
        if(providerClass)
            CFRelease(providerClass);
    }
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAUnmountApplierFunc(io_object_t entry, void * context)
{
    DASessionRef session = (DASessionRef)context;
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,session,entry);
    DADiskUnmount(disk,kDADiskUnmountOptionWhole,NULL,NULL);
    
}

/*! Unmounts all IOMedia associated with a particular iSCSI session.
 *  @param sessionId the session identifier. */
void iSCSIDAUnmountIOMediaForSession(SID sessionId)
{
    // Create a disk arbitration session and associate it with current runloop
    DASessionRef diskArbSession = DASessionCreate(kCFAllocatorDefault);
    DASessionScheduleWithRunLoop(diskArbSession,CFRunLoopGetCurrent(),kCFRunLoopDefaultMode);
    
    // Find the target associated with the session
    io_object_t target = iSCSIDAFindTargetForSession(sessionId);
    
    // Queue unmount all IOMedia objects & run CFRunLoop
    iSCSIDAIOMediaApplyFunction(target,&iSCSIDAUnmountApplierFunc,diskArbSession);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,1,true);
}

void iSCSIDACreateBSDDiskNamesApplierFunc(io_object_t entry, void * context)
{
    CFMutableArrayRef diskNames = (CFMutableArrayRef)context;
    
    // Get the disk name for the object and add it to the array
    CFStringRef BSDName = IORegistryEntryCreateCFProperty(entry,CFSTR("BSD Name"),kCFAllocatorDefault,0);
    
    if(BSDName) {
        CFArrayAppendValue(diskNames,BSDName);
        CFRelease(BSDName);
    }
}

/*! Creates an array of strings that hold the BSD disk names for a particular
 *  iSCSI session.
 *  @param sessionId the session identifier.
 *  @return the array of strings of BSD names for the specified session. */
CFArrayRef iSCSIDACreateBSDDiskNamesForSession(SID sessionId)
{
    // Find the target associated with the session
    io_object_t target = iSCSIDAFindTargetForSession(sessionId);
    
    CFMutableArrayRef diskNames = CFArrayCreateMutable(kCFAllocatorDefault,100,&kCFTypeArrayCallBacks);
    
    // Queue unmount all IOMedia objects & run CFRunLoop
    iSCSIDAIOMediaApplyFunction(target,&iSCSIDACreateBSDDiskNamesApplierFunc,diskNames);
    
    return diskNames;
}

void iSCSIDACreateIOMediaCFPropertyApplierFunc(io_object_t entry, void * context)
{
    CFMutableArrayRef IOMediaProperties = (CFMutableArrayRef)context;
    
    // Get the disk name for the object and add it to the array
    CFMutableDictionaryRef properties = NULL;
    if(IORegistryEntryCreateCFProperties(entry,&properties,kCFAllocatorDefault,0) == kIOReturnSuccess)
        CFArrayAppendValue(IOMediaProperties,properties);
}

/*! Creates an array of dictionaries that hold the properties of every IOMedia
 *  object associated with a particular session.
 *  @param sessionId the session identifier.
 *  @return an array of dictionaries of properties of every IOMedia object for
 *  the associated session. */
CFArrayRef iSCSIDACreateIOMediaCFPropertyForSession(SID sessionId)
{
    // Find the target associated with the session
    io_object_t target = iSCSIDAFindTargetForSession(sessionId);
    
    CFMutableArrayRef IOMediaProperties = CFArrayCreateMutable(kCFAllocatorDefault,100,&kCFTypeArrayCallBacks);
    
    // Queue unmount all IOMedia objects & run CFRunLoop
    iSCSIDAIOMediaApplyFunction(target,&iSCSIDACreateIOMediaCFPropertyApplierFunc,IOMediaProperties);
    
    return IOMediaProperties;
}
