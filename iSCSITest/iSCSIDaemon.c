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
    
    

   if(iSCSIKernelInitialize() == kIOReturnSuccess)
        printf("Connected");

//    iSCSISessionRelease(0);
     iSCSISessionInfo sessionOpts;
    sessionOpts.maxConnections = 1;
    
    
    iSCSIConnectionInfo connOpts;
    connOpts.initiatorAlias = CFSTR("Test");
    connOpts.initiatorName = CFSTR("iqn.2014-01.com.os:host");
 //   connOpts.targetName = CFSTR("iqn.1995-05.com.lacie:nas-vault:nareg");
  //  connOpts.targetName = CFSTR("iqn.1995-05.com.lacie:nas-vault:narreh");
//    connOpts.targetName = NULL;
    connOpts.targetName = CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi23");

    connOpts.hostAddress = CFSTR("192.168.1.147");
    connOpts.targetAddress = CFSTR("192.168.1.115");
    connOpts.targetPort = CFSTR("3260");
    connOpts.useHeaderDigest = false;
    connOpts.useDataDigest = false;


//connOpts.authMethod = iSCSIAuthCreateCHAP(CFSTR("nareg"),CFSTR("test2test2test2"),
  //                                           CFSTR("nareg"),CFSTR("testtesttest"));
    connOpts.authMethod = NULL;
    

    iSCSISessionCreate(&sessionOpts,&connOpts);
//    CFMutableDictionaryRef targetList;
//    iSCSISessionGetTargetList(sessionQualifier,0,&targetList);
//    iSCSIKernelDeactivateConnection(sessionOpts.sessionId,connOpts.connectionId);
    iSCSISessionRelease(0);
    iSCSISessionRelease(1);
    iSCSISessionRelease(2);
  
  
    
    
    iSCSIKernelCleanUp();
	
//    CFRelease(targetList);
 

    
    
 
    
    return 0;
}

