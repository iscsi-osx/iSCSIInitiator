//
//  main.c
//  iSCSITest
//
//  Created by Nareg Sinenian on 12/13/14.
//
//

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"

#include <launch.h>

#include <stdio.h>
#include <IOKit/IOKitLib.h>
#include <DiskArbitration/DiskArbitration.h>

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
        CFTypeRef data = IORegistryEntryCreateCFProperty(entry,CFSTR("Protocol Characteristics"),
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
                return child;
            }
        }
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

    if(BSDName)
        CFArrayAppendValue(diskNames,BSDName);
}

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


CFArrayRef iSCSIDACreateIOMediaCFPropertyForSession(SID sessionId)
{
    // Find the target associated with the session
    io_object_t target = iSCSIDAFindTargetForSession(sessionId);
    
    CFMutableArrayRef IOMediaProperties = CFArrayCreateMutable(kCFAllocatorDefault,100,&kCFTypeArrayCallBacks);
    
    // Queue unmount all IOMedia objects & run CFRunLoop
    iSCSIDAIOMediaApplyFunction(target,&iSCSIDACreateIOMediaCFPropertyApplierFunc,IOMediaProperties);
    
    return IOMediaProperties;
}



int main(int argc, const char * argv[]) {

    

//    CFArrayRef BSDNames = iSCSIDACreateBSDDiskNamesForSession(0);
//    CFShow(CFArrayGetValueAtIndex(BSDNames,0));
    
 //   iSCSIDAUnmountIOMediaForSession(0);
//    int c = 10;
  /*
    
    io_connect_t connection;
    

    kern_return_t kernResult;
    
    // Create a dictionary to match IOMedia object(s) of an iSCSI target
    CFMutableDictionaryRef matchingDict = NULL;
    matchingDict = IOServiceMatching("com_NSinenian_iSCSIVirtualHBA");
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault,matchingDict);
    
    // Check to see if the initiator kext (driver) was found in the I/O registry
    if(service == IO_OBJECT_NULL)
        return kIOReturnNotFound;
    
    // Iterate over the targets and find the specified session identifier
    // (the target identifier is the same as the session identifier by design)
    io_iterator_t iterator = NULL;

    IORegistryEntryGetChildIterator(service,kIOServicePlane,&iterator);

    io_object_t test = findTarget(0,iterator);
    
    int b = 0;
        
    
    
    
    
    // Using the service handle, open a connection
    kernResult = IOServiceOpen(service,mach_task_self(),0,&connection);
    
    if(kernResult != kIOReturnSuccess) {
        IOObjectRelease(service);
        return kIOReturnNotFound;
    }

   */
    
    
    
    
    if(iSCSIKernelInitialize() == kIOReturnSuccess)
        printf("Connected");
    
    
    

 
  //       iSCSILogoutSession(0);
  
    
    iSCSIMutablePortalRef portal = iSCSIMutablePortalCreate();
    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.115"));
    iSCSIPortalSetPort(portal,CFSTR("3260"));
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
 
    CFArrayRef targets;
    enum iSCSILoginStatusCode statusCode;
    iSCSIMutableDiscoveryRecRef discoveryRec = iSCSIMutableDiscoveryRecCreate();
    
    iSCSIQueryPortalForTargets(portal,&discoveryRec,&statusCode);
    
    CFArrayRef targetlist = iSCSIDiscoveryRecCreateArrayOfTargets(discoveryRec);
    
    CFShow(CFSTR("The following targets were detected:\n"));
    CFStringRef targetName = CFArrayGetValueAtIndex(targetlist,0);
    CFShow(targetName);
    
    CFArrayRef TPGT = iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(discoveryRec,targetName);
    CFArrayRef portals = iSCSIDiscoveryRecGetPortals(discoveryRec,targetName,CFArrayGetValueAtIndex(TPGT,0));
    
    iSCSIPortalRef por = CFArrayGetValueAtIndex(portals,0);
    CFShow(iSCSIPortalGetAddress(por));
    

    
  /*
    iSCSIMutableTargetRef target = iSCSITargetCreate();
    iSCSITargetSetName(target,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55"));
    iSCSITargetSetHeaderDigest(target,false);
    iSCSITargetSetDataDigest(target,false);
    
    iSCSIAuthRef auth = iSCSIAuthCreateNone();


//    iSCSIQueryPortalForTargets(&portal,&targets);

    SID sessionId;
    CID connectionId;

    enum iSCSILoginStatusCode statusCode;
    iSCSILoginSession(portal,target,auth,&sessionId,&connectionId,&statusCode);
    
    UInt16 id;
    iSCSIGetSessionIdForTarget(CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55"),&id);
    
    UInt32 connectionCount;
    CID * connectionIds = malloc(kiSCSIMaxConnectionsPerSession*sizeof(CID));
    connectionIds[0] = 10;
    iSCSIGetConnectionIds(sessionId, connectionIds, &connectionCount);
    
    UInt16 sessionCount;
    SID * sessionIds = malloc(kiSCSIMaxSessions*sizeof(SID));
    iSCSIGetSessionIds(sessionIds,&sessionCount);
 
//    iSCSIQueryTargetForAuthMethods(&portal,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi56"),&authMethods);
   
   */
    
    iSCSIKernelCleanUp();
    
    //    CFRelease(targetList);
    
    return 0;
}
