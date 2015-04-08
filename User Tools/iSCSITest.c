
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

    iSCSIKernelInitialize();


    iSCSIAuthRef auth = iSCSIAuthCreateNone();
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    
    iSCSITargetSetName(target,CFSTR("iqn.2015-03.com.test:test1"));
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.115"));
    iSCSIPortalSetPort(portal,CFSTR("3260"));
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    
    enum iSCSILoginStatusCode statusCode;
    enum iSCSIAuthMethods authMethod;
    iSCSIMutableDiscoveryRecRef discRec;
    iSCSIQueryPortalForTargets(portal,auth,&discRec,&statusCode);
    
    
    CFDataRef data = iSCSIDiscoveryRecCreateData(discRec);
    
    iSCSIDiscoveryRecRef a = iSCSIDiscoveryRecCreateMutableWithData(data);
    
    int b = CFDataGetLength(data);
    
    CFPreferencesSetAppValue(CFSTR("Discovery"),a,CFSTR("com.NSinenian.iSCSI"));
    
    
    SID sessionId;
    CID connectionId;
    
/*
    iSCSILoginSession(target,portal,auth,iSCSISessionConfigCreateMutable(),iSCSIConnectionConfigCreateMutable(),&sessionId,&connectionId,&statusCode);
    */
    iSCSIKernelCleanUp();
    return 0;
}
