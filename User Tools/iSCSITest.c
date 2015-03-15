
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
#include "iSCSIPropertyList.h"




int main(int argc, const char * argv[]) {

   iSCSIPLSetInitiatorName(CFSTR("iqn.test.com"));
    

    
    CFStringRef blah = iSCSIPLCopyInitiatorName();
    
    CFShow(blah);
    
    iSCSIPLSetInitiatorAlias(CFSTR("SOME ALIAS"));
    CFShow(iSCSIPLCopyInitiatorAlias());
    

    iSCSIPLSynchronize();
    

    
int c = 100;

/*
    // Iterate over sessions
    for(SID sessionId = 0; sessionId < kiSCSIMaxSessions; sessionId++)
    {
        for(CID connectionId = 0; connectionId < 3; connectionId++)
        {
            CFShow(")
        }
    }*/

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
    
    CFStringRef a = CFStringCreateWithCString(kCFAllocatorDefault,"3260",kCFStringEncodingASCII);
//    iSCSIPortalSetPort(portal,CFSTR("3260"));
    
    iSCSIPortalSetPort(portal,a);
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));

    int zz = CFGetRetainCount(a);
    
    CFRelease(a);
    
    int zzz = 0;
    

  



   // iSCSIAuthRef auth = iSCSIAuthCreateNone();
  //  iSCSIDBSetAuthentication(CFSTR("test2"),CFSTR("192.168.1.115"),auth);

 /*
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
    */
/*
    CID connectionId = 100;
    iSCSIGetConnectionIdForAddress(0,portal,&connectionId);
    int c = 100;
  */

    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi59"));

 //   iSCSITargetSetHeaderDigest(target,false);
//    iSCSITargetSetDataDigest(target,false);
 
    iSCSIAuthRef auth = iSCSIAuthCreateNone();


//    iSCSIQueryPortalForTargets(&portal,&targets);

    SID sessionId;
    CID connectionId;

    enum iSCSILoginStatusCode statusCode;
iSCSISessionConfigRef sessCfg = iSCSIMutableSessionConfigCreate();
    iSCSIConnectionConfigRef connCfg = iSCSIMutableConnectionConfigCreate();
    

    iSCSILoginSession(target,portal,auth,sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
    
    iSCSITargetRef tt = iSCSICreateTargetForSessionId(0);
    
    iSCSIPortalRef pp = iSCSICreatePortalForConnectionId(0,0);
    CFShow(iSCSIPortalGetAddress(portal));
    CFShow(iSCSIPortalGetPort(portal));
   
    SID id = 50;
//CFArraySetValueAtIndex(sessionIds,0,(void *)2);
   // id = CFArrayGetValueAtIndex(sessionIds, 0);
   /*
    const void ** sessionIdValues;
    int count = CFArrayGetCount(sessionIds);
    CFArrayGetValues(sessionIds,CFRangeMake(0,CFArrayGetCount(sessionIds)),sessionIdValues);

    SID test = (SID)sessionIdValues[0];
    
    int z = 10;
    
   */ 
    

 /*
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
