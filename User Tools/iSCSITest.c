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

int main(int argc, const char * argv[]) {
    
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
