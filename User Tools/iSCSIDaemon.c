/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemon.c
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI daemon
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <launch.h>

#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"


#include <CoreFoundation/CFPreferences.h>

static const CFStringRef applicationId = CFSTR("com.NSinenian.iscsix");

/*! Read target entry from .plist and construct target and portal information. */
//errno_t BuildPortalFrom


/*! Helper function.  Configures the iSCSI initiator based on parameters
 *  that are read from the preferences .plist file. */
void ConfigureiSCSIFromPreferences()
{
    // Connect to the preferences .plist file associated with "iscsid" and
    // read configuration parameters for the initiator
/*    CFStringRef initiatorName =
        CFPreferencesCopyValue(kiSCSIPKInitiatorName,applicationId,
                               kCFPreferencesAnyUser,kCFPreferencesAnyHost);
    
    CFStringRef initiatorAlias =
        CFPreferencesCopyValue(kiSCSIPKInitiatorAlias,applicationId,
                               kCFPreferencesAnyUser,kCFPreferencesAnyHost);
    
    // If initiator name & alias are provided, set them...
    if(initiatorName)
        iSCSISetInitiatiorName(initiatorName);
    
    if(initiatorAlias)
        iSCSISetInitiatorAlias(initiatorAlias);*/
    
}

/*! iSCSI daemon entry point. */
int main(void)
{
    // Verify that the iSCSI kernel extension is running
    if(iSCSIKernelInitialize() != kIOReturnSuccess)
        return ENOTSUP;

    // Configure the initiator by reading preferences file
    ConfigureiSCSIFromPreferences();
    
    
    // Register with launchd so it can manage this daemon
    launch_data_t reg_request = launch_data_new_string(LAUNCH_KEY_CHECKIN);
    
    // Quit if we are unable to checkin...
    if(!reg_request)
        return -1;
    
    launch_data_t reg_response = launch_msg(reg_request);
    
    // Ensure registration was successful
    if((launch_data_get_type(reg_response) == LAUNCH_DATA_ERRNO))
        goto ERROR_LAUNCH_DATA;
    
    // Grab label and socket dictionary from daemon's property list
    launch_data_t label = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_LABEL);
    launch_data_t sockets = launch_data_dict_lookup(reg_response,LAUNCH_JOBKEY_SOCKETS);
    
    if(!label || !sockets)
        goto ERROR_NO_SOCKETS;
    
    launch_data_t listen_socket_array = launch_data_dict_lookup(sockets,"iscsid");
    
    if(!listen_socket_array || launch_data_array_get_count(listen_socket_array) == 0)
        goto ERROR_NO_SOCKETS;
    
    // Grab handle to socket we want to listen on...
    launch_data_t listen_socket = launch_data_array_get_index(listen_socket_array,0);
    
    // Create a new kernel event queues. The socket descriptors are registered
    // with the kernel, and the kernel notifies us (using events) when there
    // are incoming connections on a socket.
    int kernelQueue;
    if((kernelQueue = kqueue()) == -1)
        goto ERROR_KQUEUE_FAIL;
    
    // Associate a new kernel event with the socket
    struct kevent kernelEvent;
    EV_SET(&kernelEvent,launch_data_get_fd(listen_socket),EVFILT_READ,EV_ADD,0,0,NULL);
    if((kevent(kernelQueue,&kernelEvent,1,NULL,0,NULL) == -1))
        goto ERROR_KEVENT_REG_FAIL;
    
    launch_data_free(reg_response);
    
    while(true)
    {
        int numEvents = 0;

        // Wait for the kernel to tell that data has become available on the
        // socket (if no events or if there's an error we quit)
        struct kevent eventList;
        if((numEvents = kevent(kernelQueue,NULL,0,&eventList,1,NULL)) == -1)
            continue;
        else if (numEvents == 0)
            continue;

        // Accept an incoming connection
        struct sockaddr_storage peerAddress;
        socklen_t sizeAddress = sizeof(peerAddress);
        int fd = accept((int)eventList.ident,(struct sockaddr *)&peerAddress,&sizeAddress);
        
        // Receive data 
        UInt8 buffer[10];
        
        if(recv(fd,&buffer,1,0) != -1)
            fprintf(stderr,"Message received");
    }

    // Close our connection to the iSCSI kernel extension
    iSCSIKernelCleanUp();
    
    return 0;
        
ERROR_KQUEUE_FAIL:
    fprintf(stderr,"Failed to create kernel event queue.\n");

ERROR_KEVENT_REG_FAIL:
    fprintf(stderr,"Failed to register socket with kernel.\n");
    
ERROR_NO_SOCKETS:
    fprintf(stderr,"Could not find socket ");

    launch_data_free(reg_response);
    
ERROR_LAUNCH_DATA:
    fprintf(stderr,"Failed to checkin with launchd.\n");
    
    // Close our connection to the iSCSI kernel extension
    iSCSIKernelCleanUp();
    
    return ENOTSUP;
}

