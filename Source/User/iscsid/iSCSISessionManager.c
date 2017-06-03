/*
 * Copyright (c) 2016, Nareg Sinenian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "iSCSISessionManager.h"

#include "iSCSISession.h"
#include "iSCSIHBAInterface.h"

/*! Name of the initiator. */
CFStringRef kiSCSIInitiatorIQN = CFSTR("iqn.2015-01.com.localhost");

/*! Alias of the initiator. */
CFStringRef kiSCSIInitiatorAlias = CFSTR("default");

struct __iSCSISessionManager
{
    CFAllocatorRef allocator;
    iSCSIHBAInterfaceRef hbaInterface;
    iSCSISessionManagerCallBacks callbacks;
    CFStringRef initiatorName;
    CFStringRef initiatorAlias;
    
};

/*! This function is called handle session or connection network timeouts.
 *  When a timeout occurs the kernel deactivates the session and connection.
 *  The session layer (this layer) must release the connection after propogating 
 *  the notification onto the user of the session manager. */
 void iSCSIHBANotificationTimeoutMessageHandler(iSCSISessionManagerRef managerRef,
                                                iSCSIHBANotificationMessage * msg)
 {
     // Retrieve the target name and portal address associated with
     // the timeout. Pass information along to pre-designated runloop
     // so that clients of this layer can act
     iSCSITargetRef target = iSCSISessionCopyTargetForId(managerRef,msg->sessionId);
     iSCSIPortalRef portal = iSCSISessionCopyPortalForConnectionId(managerRef,msg->sessionId,msg->connectionId);
     
     // Release the stale session/connection
     iSCSIHBAInterfaceReleaseConnection(managerRef->hbaInterface,msg->sessionId,msg->connectionId);
     
     // Call user-defined callback function if one exists
     if(managerRef->callbacks.timeoutCallback)
         managerRef->callbacks.timeoutCallback(target,portal);
     
     iSCSITargetRelease(target);
     iSCSIPortalRelease(portal);
 }
 
/*! Called to handle asynchronous events that involve dropped sessions, connections, 
 *  logout requests and parameter negotiation. This function is not called for asynchronous 
 *  SCSI messages or vendor-specific messages. */
void iSCSIHBANotificationAsyncMessageHandler(iSCSISessionManagerRef managerRef,
                                             iSCSIHBANotificationAsyncMessage * msg)
{
    enum iSCSIPDUAsyncMsgEvent asyncEvent = (enum iSCSIPDUAsyncMsgEvent)msg->asyncEvent;
    enum iSCSILogoutStatusCode statusCode;
    
    switch (asyncEvent) {
            
        // We are required to issue a logout request
        case kiSCSIPDUAsyncMsgLogout:
            iSCSISessionRemoveConnection(managerRef,msg->sessionId,msg->connectionId,&statusCode);
            break;
            
        // We have been asked to re-negotiate parameters for this connection
        // (this is currently unsupported and we logout)
        case kiSCSIPDUAsyncMsgNegotiateParams:
            iSCSISessionRemoveConnection(managerRef,msg->sessionId,msg->connectionId,&statusCode);
            break;
            
        default:
            break;
    }
}

static void iSCSIHBANotificationHandler(iSCSIHBAInterfaceRef interface,
                                        enum iSCSIHBANotificationTypes type,
                                        iSCSIHBANotificationMessage * msg,void * info)
{
    iSCSISessionManagerRef managerRef = (iSCSISessionManagerRef)info;
    
    // Process an asynchronous message
    switch(type)
    {
            // The kernel received an iSCSI asynchronous event message
        case kiSCSIHBANotificationAsyncMessage:
            iSCSIHBANotificationAsyncMessageHandler(managerRef,(iSCSIHBANotificationAsyncMessage *)msg);
            break;
        case kiSCSIHBANotificationTimeout:
            iSCSIHBANotificationTimeoutMessageHandler(managerRef,(iSCSIHBANotificationMessage *)msg);
            break;
        case kiSCSIHBANotificationTerminate: break;
        default: break;
    };
}

/*! Call to initialize iSCSI session management functions.  This function will
 *  initialize the kernel layer after which other session-related functions
 *  may be called.
 *  @param rl the runloop to use for executing session-related functions.
 *  @return an error code indicating the result of the operation. */
iSCSISessionManagerRef iSCSISessionManagerCreate(CFAllocatorRef allocator,
                                                 iSCSISessionManagerCallBacks callbacks)
{
    iSCSISessionManagerRef managerRef = CFAllocatorAllocate(allocator,sizeof(struct __iSCSISessionManager),0);
    
    iSCSIHBANotificationContext notifyContext;
    notifyContext.version = 0;
    notifyContext.info = managerRef;
    notifyContext.release = 0;
    notifyContext.retain = 0;
    notifyContext.copyDescription = 0;
    
    iSCSIHBAInterfaceRef interface = iSCSIHBAInterfaceCreate(allocator,iSCSIHBANotificationHandler,&notifyContext);
    
    if(interface) {
        managerRef->allocator = allocator;
        managerRef->hbaInterface = interface;
        managerRef->callbacks = callbacks;
        managerRef->initiatorName = kiSCSIInitiatorIQN;
        managerRef->initiatorAlias = kiSCSIInitiatorAlias;
    }
    else {
        CFAllocatorDeallocate(allocator,managerRef);
        managerRef = NULL;
    }
    
    return managerRef;
}

/*! Called to cleanup kernel resources used by the iSCSI session management
 *  functions.  This function will close any connections to the kernel
 *  and stop processing messages related to the kernel.
 *  @param managerRef an instance of an iSCSISessionManagerRef. */
void iSCSISessionManagerRelease(iSCSISessionManagerRef managerRef)
{
    CFAllocatorDeallocate(managerRef->allocator,managerRef);
}

/*! Creates a runloop source used to run the callback functions associated
 *  with the session manager reference. */
/*! Schedules execution of various tasks, including handling of kernel notifications
 *  on for the specified interface instance over the designated runloop.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param runLoop the runloop to schedule
 *  @param runLoopMode the execution mode for the runLoop. */
void iSCSISessionManagerScheduleWithRunLoop(iSCSISessionManagerRef managerRef,
                                            CFRunLoopRef runLoop,
                                            CFStringRef runLoopMode)
{
    iSCSIHBAInterfaceScheduleWithRunloop(managerRef->hbaInterface,runLoop,runLoopMode);
}

/*! Unschedules execution of various tasks, including handling of session notifications
 *  on for the specified interface instance over the designated runloop.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param runLoop the runloop to schedule
 *  @param runLoopMode the execution mode for the runLoop. */
void iSCSISessionManagerUnscheduleWithRunloop(iSCSISessionManagerRef managerRef,
                                              CFRunLoopRef runLoop,
                                              CFStringRef runLoopMode)
{
    iSCSIHBAInterfaceScheduleWithRunloop(managerRef->hbaInterface,runLoop,runLoopMode);
}

/*! Returns a reference to the underlying HBA interface instance.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @return a reference to the underlying HBA interface instance. */
iSCSIHBAInterfaceRef iSCSISessionManagerGetHBAInterface(iSCSISessionManagerRef managerRef)
{
    return managerRef->hbaInterface;
}

/*! Sets the initiator name to use for new sessions. This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param initiatorIQN the initiator name. */
void iSCSISessionManagerSetInitiatorName(iSCSISessionManagerRef managerRef,
                                         CFStringRef initiatorIQN)
{
    CFRelease(managerRef->initiatorName);
    managerRef->initiatorName = CFStringCreateCopy(kCFAllocatorDefault,initiatorIQN);
}

/*! Sets the initiator alias to use for new sessions. This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param initiatorAlias the initiator alias. */
void iSCSISessionManagerSetInitiatorAlias(iSCSISessionManagerRef managerRef,
                                          CFStringRef initiatorAlias)
{
    CFRelease(managerRef->initiatorAlias);
    managerRef->initiatorAlias = CFStringCreateCopy(kCFAllocatorDefault,initiatorAlias);
}
