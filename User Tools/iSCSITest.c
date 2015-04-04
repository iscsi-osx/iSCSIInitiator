
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




int main(int argc, const char * argv[]) {

    iSCSIKernelInitialize();


    iSCSIAuthRef auth = iSCSIAuthCreateNone();
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    
    iSCSITargetSetName(target,CFSTR(""));
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.115"));
    iSCSIPortalSetPort(portal,CFSTR("100"));
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    
    enum iSCSILoginStatusCode statusCode;
    enum iSCSIAuthMethods authMethod;
    iSCSIMutableDiscoveryRecRef discRec;
    iSCSIQueryPortalForTargets(portal,auth,&discRec,&statusCode);
    
    iSCSIKernelCleanUp();
    return 0;
}
