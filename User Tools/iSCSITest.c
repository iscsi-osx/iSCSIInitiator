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
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreate();
    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.115"));
    iSCSIPortalSetPort(portal,CFSTR("3260"));
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    
    iSCSIMutableTargetRef target = iSCSITargetCreate();
    iSCSITargetSetName(target,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55"));
    iSCSITargetSetHeaderDigest(target,false);
    iSCSITargetSetDataDigest(target,false);
    
    iSCSIAuthRef auth = iSCSIAuthCreateNone();


//    iSCSIQueryPortalForTargets(&portal,&targets);

    UInt16 sessionId;
    UInt32 connectionId;

    enum iSCSIStatusCode statusCode;
    iSCSICreateSession(portal,target,auth,&sessionId,&connectionId,&statusCode);
    
    UInt16 id;
    iSCSIGetSessionIdForTarget(CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55"),&id);
    
    UInt32 connectionCount;
    UInt32 * connectionIds = malloc(kiSCSIMaxConnectionsPerSession*sizeof(UInt32));
    connectionIds[0] = 10;
    iSCSIGetConnectionIds(sessionId, &connectionIds, &connectionCount);
    
    UInt16 sessionCount;
    UInt16 * sessionIds = malloc(kiSCSIMaxSessions*sizeof(UInt16));
    iSCSIGetSessionIds(&sessionIds,&sessionCount);
    
//    iSCSIQueryTargetForAuthMethods(&portal,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi56"),&authMethods);
    iSCSIReleaseSession(0);
 /*
    //    iSCSIReleaseSession(0);
    iSCSISessionInfo sessionOpts;
    sessionOpts.maxConnections = 2;
    
    
    iSCSIConnectionInfo connOpts;
    connOpts.initiatorAlias = CFSTR("Test");
    connOpts.initiatorName = CFSTR("iqn.2014-01.com.os:host");
    connOpts.targetName = CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55");
    connOpts.hostInterface = CFSTR("en0");
    connOpts.targetAddress = CFSTR("192.168.1.116");
    connOpts.targetPort = CFSTR("3260");
    connOpts.useHeaderDigest = false;
    connOpts.useDataDigest = true;
    
    //connOpts.authMethod = iSCSIAuthCreateCHAP(CFSTR("nareg"),CFSTR("test2test2test2"),
    //                                           CFSTR("nareg"),CFSTR("testtesttest"));
    connOpts.authMethod = NULL;
    
    
    iSCSICreateSession(&sessionOpts,&connOpts);
    
    
    //    CFMutableDictionaryRef targetList;
    //    iSCSISessionGetTargetList(sessionQualifier,0,&targetList);
    //    iSCSIKernelDeactivateConnection(sessionOpts.sessionId,connOpts.connectionId);
    iSCSIReleaseSession(0);
    iSCSIReleaseSession(1);
    iSCSIReleaseSession(2);
    
  */  
    
    iSCSIKernelCleanUp();
    
    //    CFRelease(targetList);
    
    
    
    

    
    
    
    return 0;
}
