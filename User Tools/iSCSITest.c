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
#include <Security/Security.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>


#include <stdio.h>
#include <IOKit/IOKitLib.h>

#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"
#include "iSCSITypes.h"
#include "iSCSIPropertyList.h"

int main(int argc, const char * argv[]) {

    /*

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
    
    
    iSCSIConnectionConfigSetHeaderDigest(connCfg, false);
    iSCSIConnectionConfigSetDataDigest(connCfg, false);
       errno_t error = iSCSILoginSession(target,portal,iSCSIAuthCreateCHAP(CFSTR("user"),CFSTR("passwordpassword"),NULL,NULL),sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
    //  enum iSCSILogoutStatusCode sc;
    //  iSCSILogoutSession(0,&sc);
    iSCSIDiscoveryRecRef discRec;
    //   errno_t error = iSCSIQueryPortalForTargets(portal,iSCSIAuthCreateNone(),&discRec,&statusCode);
    iSCSICreateCFPropertiesForSession(target);

    
    
    iSCSICleanup();
     

     */
    /*
    SecKeychainRef keychain;
    SecKeychainItemRef item;
    OSStatus status;
    status = SecKeychainCopyDomainDefault(kSecPreferencesDomainSystem,&keychain);
    SecKeychainUnlock(keychain, 0, NULL, false);
    status = SecKeychainAddGenericPassword(keychain,25,"iSCSI CHAP Shared Secret",31,"iqn.2013-06.com.example:target0",3,"abc",&item);

    */
    iSCSIPLSynchronize();
    iSCSIPLSetInitiatorIQN(CFSTR("iqn.test2.com"));

    iSCSIAuthRef auth = iSCSIAuthCreateCHAP(CFSTR("usera"),CFSTR("secreta"));
    //    iSCSIPLSetAuthenticationForInitiator(auth);
    iSCSIAuthRef auth2 = iSCSIPLCopyAuthenticationForInitiator();

    CFStringRef user,secret;
    iSCSIAuthGetCHAPValues(auth2,&user,&secret);
    CFShow(user);
    CFShow(secret);

    iSCSIPLSynchronize();



    return 0;
}
