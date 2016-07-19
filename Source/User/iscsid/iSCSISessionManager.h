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

#ifndef __ISCSI_SESSION_MANAGER_H__
#define __ISCSI_SESSION_MANAGER_H__

#include <CoreFoundation/CoreFoundation.h>
#include "iSCSI.h"
#include "iSCSIHBAInterface.h"

/*! Opaque session manager reference. */
typedef struct __iSCSISessionManager * iSCSISessionManagerRef;

/*! Callback function called when a session or connection timeout occurs. */
typedef void (*iSCSISessionTimeoutCallback)(iSCSITargetRef target,iSCSIPortalRef portal);
    
/*! Callback types used by the session manager. */
typedef struct iSCSISessionManagerCallBacks
{
    iSCSISessionTimeoutCallback timeoutCallback;
    
} iSCSISessionManagerCallBacks;

/*! Call to initialize iSCSI session management functions.  This function will
 *  initialize the kernel layer after which other session-related functions
 *  may be called.
 *  @param rl the runloop to use for executing session-related functions.
 *  @return an error code indicating the result of the operation. */
iSCSISessionManagerRef iSCSISessionManagerCreate(CFAllocatorRef allocator,
                                                 iSCSISessionManagerCallBacks callbacks);

/*! Called to cleanup kernel resources used by the iSCSI session management
 *  functions.  This function will close any connections to the kernel
 *  and stop processing messages related to the kernel.
 *  @param sessionManager an instance of an iSCSISessionManagerRef. */
void iSCSISessionManagerRelease(iSCSISessionManagerRef managerRef);

/*! Creates a runloop source used to run the callback functions associated
 *  with the session manager reference. */
/*! Schedules execution of various tasks, including handling of kernel notifications
 *  on for the specified interface instance over the designated runloop.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param runLoop the runloop to schedule
 *  @param runLoopMode the execution mode for the runLoop. */
void iSCSISessionManagerScheduleWithRunLoop(iSCSISessionManagerRef managerRef,
                                          CFRunLoopRef runLoop,
                                          CFStringRef runLoopMode);

/*! Unschedules execution of various tasks, including handling of session notifications
 *  on for the specified interface instance over the designated runloop.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param runLoop the runloop to schedule
 *  @param runLoopMode the execution mode for the runLoop. */
void iSCSISessionManagerUnscheduleWithRunloop(iSCSISessionManagerRef managerRef,
                                            CFRunLoopRef runLoop,
                                            CFStringRef runLoopMode);

/*! Returns a reference to the underlying HBA interface instance.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @return a reference to the underlying HBA interface instance. */
iSCSIHBAInterfaceRef iSCSISessionManagerGetHBAInterface(iSCSISessionManagerRef managerRef);

/*! Sets the initiator name to use for new sessions. This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param initiatorIQN the initiator name. */
void iSCSISessionManagerSetInitiatorName(iSCSISessionManagerRef managerRef,
                                         CFStringRef initiatorIQN);

/*! Sets the initiator aliias to use for new sessions. This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param managerRef an instance of an iSCSISessionManagerRef.
 *  @param initiatorAlias the initiator alias. */
void iSCSISessionManagerSetInitiatorAlias(iSCSISessionManagerRef managerRef,
                                          CFStringRef initiatorAlias);


#endif
