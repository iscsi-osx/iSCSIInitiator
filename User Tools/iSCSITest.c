/*!
 * @author		Nareg Sinenian
 * @file		iSCSITest.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI session management functions.  This library
 *              depends on the user-space iSCSI PDU library to login, logout
 *              and perform discovery functions on iSCSI target nodes.  It
 *              also relies on the kernel layer for access to kext.
 */

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

    // Test commands here
    enum iSCSILoginStatusCode statusCode;
    iSCSISessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
    iSCSIConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();
    CID connectionId;
    SID sessionId;
    iSCSITargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetName(target,CFSTR("iqn.2012-06.com.example:target0"));
    
    iSCSISetInitiatiorName(CFSTR("iqn.2015-01.com.test"));
    iSCSISetInitiatorAlias(CFSTR("default"));
    
    iSCSIPortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,CFSTR("192.168.56.2"));
    iSCSIPortalSetHostInterface(portal,CFSTR("vboxnet0"));
    iSCSIPortalSetPort(portal,CFSTR("3260"));
    
    
    
    iSCSILoginSession(target,portal,iSCSIAuthCreateNone(),sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
    
    
    iSCSICleanup();
    return 0;
}
