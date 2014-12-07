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

#include <launch.h>


/*! iSCSI daemon entry point. */
//int main(void)
int main(int argc, char * argv[])
{
    
    
    /*
    // Register with launchd so it can manage this daemon
    launch_data_t registration_request = launch_data_new_string(LAUNCH_DATA_STRING);
    
    if(!registration_request)
    {
        // Failed to register
        return -1;
    }
    
    launch_data_t registration_response = launch_msg(registration_request);
    
    // Ensure registration was successful
    if(launch_get_data_type(registration_response) == LAUNCH_DATA_ERRNO)
    {
        // Cleanup
        return -1;
    }
    
    // Grab label and socket dictionary from daemon's property list
    launch_data_t label = launch_data_dict_lookup(registration_response, LAUNCH_JOBKEY_LABEL);
    launch_data_t sockets = launch_data_dict_lookup(registration_response,LAUNCH_JOBKEY_SOCKETS);
    
    if(!label || !sockets)
    {
        // Cleanup
        return -1;
    }
    
    launch_data_t listen_socket_array = launch_data_dict_lookup(sockets,"iscsid");
    
    if(!listen_socket_array || launch_data_array_get_count(listen_socket_array) == 0)
    {
        // Cleanup
        return -1;
    }
    
    // Associate a new kernel event with the socket
    launch_data_t listen_socket = launch_data_array_get_index(listen_socket_array,0);
    
    
    
    launch_data_free(registration_response);
    
    while(true)
    {
        // Accept an incoming connection
        int fd = accept(listen_socket_array, <#struct sockaddr *restrict#>, <#socklen_t *restrict#>)
        
    }

*/
    
    
    

   if(iSCSIKernelInitialize() == kIOReturnSuccess)
        printf("Connected");

//    iSCSIReleaseSession(0);
     iSCSISessionInfo sessionOpts;
    sessionOpts.maxConnections = 2;
    
    
    iSCSIConnectionInfo connOpts;
    connOpts.initiatorAlias = CFSTR("Test");
    connOpts.initiatorName = CFSTR("iqn.2014-01.com.os:host");
    connOpts.targetName = CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi52");
    connOpts.hostInterface = CFSTR("en0");
    connOpts.targetAddress = CFSTR("192.168.1.116");
    connOpts.targetPort = CFSTR("3260");
    connOpts.useHeaderDigest = true;
    connOpts.useDataDigest = false;

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
 
    
    
    iSCSIKernelCleanUp();
	
//    CFRelease(targetList);
 

  
    
 
    
    return 0;
}

