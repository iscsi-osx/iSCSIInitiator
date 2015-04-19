
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


#include <stdio.h>
#include <IOKit/IOKitLib.h>

#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"
#include "iSCSITypes.h"

int main(int argc, const char * argv[]) {

    iSCSIInitialize(CFRunLoopGetCurrent());


    iSCSIAuthRef auth = iSCSIAuthCreateNone();
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    
//    iSCSITargetSetName(target,CFSTR("iqn.2015-03.com.test:test1"));
    iSCSITargetSetName(target,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi59"));
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
//    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.138"));
    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.115"));
    iSCSIPortalSetPort(portal,CFSTR("3260"));
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    
    iSCSIMutablePortalRef portal2 = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal2,CFSTR("192.168.1.120"));
    iSCSIPortalSetPort(portal2,CFSTR("3260"));
    iSCSIPortalSetHostInterface(portal2,CFSTR("en4"));

    
    enum iSCSILoginStatusCode statusCode;
    enum iSCSIAuthMethods authMethod;
    
    
    
    SID sessionId;
    CID connectionId;

    
    iSCSIMutableSessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
    iSCSISessionConfigSetMaxConnections(sessCfg,2);
    
    iSCSIMutableConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();
    iSCSIConnectionConfigSetDataDigest(connCfg,true);
    iSCSILoginSession(target,portal,auth,sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
    
    
   // iSCSILoginConnection(sessionId, portal2, auth, iSCSIConnectionConfigCreateMutable(), &connectionId, &statusCode);

    
    iSCSICleanup();
    return 0;
}
