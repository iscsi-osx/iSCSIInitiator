//
//  main.cpp
//  iSCSITest
//
//  Created by Nareg Sinenian on 11/5/13.
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



int main(int argc, const char * argv[])
{
/*    CC_MD5_CTX md5;
    CC_MD5_Init(&md5);
    
    CFStringRef secret = CFSTR("0x2390ef4984efad289");
    
    UInt8 * bytes;
    size_t len = CreateByteArrayFromHexString(secret,&bytes);
    
    CC_MD5_Update(&md5, bytes,len);
    
    
    char digest[CC_MD5_DIGEST_LENGTH];
    CC_MD5_Final(digest,&md5);
    
    CFStringRef d = CreateHexStringFromByteArray(digest,CC_MD5_DIGEST_LENGTH);
    
    int b = 0;
*/

   if(iSCSIKernelInitialize() == kIOReturnSuccess)
        printf("Connected");
    
    iSCSISessionInfo sessionOpts;
    sessionOpts.maxConnections = 1;
    
    
    iSCSIConnectionInfo connOpts;
    connOpts.initiatorAlias = CFSTR("Test");
    connOpts.initiatorName = CFSTR("iqn.2014-01.com.os:host");
    connOpts.targetName = CFSTR("iqn.1995-05.com.lacie:nas-vault:nareg");
//    connOpts.targetName = NULL;

    connOpts.hostAddress = CFSTR("192.168.1.147");
    connOpts.targetAddress = CFSTR("192.168.1.115");
    connOpts.targetPort = CFSTR("3260");
    connOpts.useHeaderDigest = false;
    connOpts.useDataDigest = false;


    connOpts.authMethod = iSCSIAuthCreateCHAP(CFSTR("nareg"),CFSTR("test2test2test2"),
                                              CFSTR("nareg"),CFSTR("testtesttest"));
    
    UInt16 sessionQualifier;
    UInt32 connectionID;
    iSCSISessionCreate(&sessionOpts,&connOpts);
//    CFMutableDictionaryRef targetList;
//    iSCSISessionGetTargetList(sessionQualifier,0,&targetList);
    iSCSISessionRelease(0);
    iSCSISessionRelease(1);
        iSCSISessionRelease(2);
    iSCSIKernelCleanUp();
	
//    CFRelease(targetList);
    
   
    
    
    
    return 0;
}

