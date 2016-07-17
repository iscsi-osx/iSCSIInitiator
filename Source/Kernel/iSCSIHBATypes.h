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

#ifndef __ISCSI_HBA_TYPES_H__
#define __ISCSI_HBA_TYPES_H__

// If used in user-space, this header will need to include additional
// headers that define primitive fixed-size types.  If used with the kernel,
// IOLib must be included for kernel memory allocation
#ifdef KERNEL
#include <IOKit/IOLib.h>
#else
#include <stdlib.h>
#include <MacTypes.h>
#endif

// Kernel and user iSCSI types (shared)
#include "iSCSITypesShared.h"
#include "iSCSIKernelClasses.h"

#include <mach/message.h>

/*! Notification types send from the kernel to the user-space daemon. */
enum iSCSIHBANotificationTypes {
    
    /*! An asynchronous iSCSI message. */
    kiSCSIHBANotificationAsyncMessage,
    
    /*! Notifies clients that the kernel extension or controller is going
     *  shut down.  Clients should release all resources. */
    kiSCSIHBANotificationTerminate,
    
    /*! Notifies clients that a network connnectivity issue has
     *  caused the specified connection and session to be dropped. */
    kiSCSIHBANotificationTimeout,
    
    /*! Invalid notification message. */
    kiSCSIHBANotificationInvalid
};


/*! Used to pass notifications from the kernel to the user-space daemon.
 *  The notification type is one of the notification types listed in 
 *  the enumerated type iSCSINotificationTypes. */
typedef struct {
    
    /*! Message haeder. */
    mach_msg_header_t header;
    
    /*! The notification type. */
    UInt8 notificationType;
    
    /*! Parameter associated with the notification (notification-specific). */
    UInt64 parameter1;
    
    /*! Parameter associated with the notification (notification-specific). */
    UInt64 parameter2;
    
    /*! Session identifier. */
    SessionIdentifier sessionId;
    
    /*! Connection identifier. */
    ConnectionIdentifier connectionId;
    
} iSCSIHBANotificationMessage;


/*! Used to pass notifications from the kernel to the user-space daemon.
 *  The notification type is one of the notification types listed in
 *  the enumerated type iSCSINotificationTypes. */
typedef struct {
    
    /*! The notification type. */
    UInt8 notificationType;
    
    /*! An asynchronous event code, see iSCSIPDUAsyncEvent. */
    UInt64 asyncEvent;
    
    /*! The logical unit identifier associated with the notification (this
     *  field is only populated for SCSI async messages and ignored for all
     *  other types of asyncEvents). */
    UInt64 LUN;
    
    /*! Session identifier. */
    SessionIdentifier sessionId;
    
    /*! Connection identifier. */
    ConnectionIdentifier connectionId;
    
} iSCSIHBANotificationAsyncMessage;


/*! Function pointer indices.  These are the functions that can be called
 *	indirectly by calling IOCallScalarMethod(). */
enum functionNames {
	kiSCSIOpenInitiator,
	kiSCSICloseInitiator,
    kiSCSICreateSession,
    kiSCSIReleaseSession,
    kiSCSISetSessionParameter,
    kiSCSIGetSessionParameter,
    kiSCSICreateConnection,
    kiSCSIReleaseConnection,
    kiSCSIActivateConnection,
    kiSCSIActivateAllConnections,
    kiSCSIDeactivateConnection,
    kiSCSIDeactivateAllConnections,
    kiSCSISendBHS,
    kiSCSISendData,
    kiSCSIRecvBHS,
    kiSCSIRecvData,
    kiSCSISetConnectionParameter,
    kiSCSIGetConnectionParameter,
    kiSCSIGetConnection,
    kiSCSIGetNumConnections,
    kiSCSIGetSessionIdForTargetIQN,
    kiSCSIGetConnectionIdForPortalAddress,
    kiSCSIGetSessionIds,
    kiSCSIGetConnectionIds,
    kiSCSICreateTargetIQNForSessionId,
    kiSCSIGetPortalAddressForConnectionId,
    kiSCSIGetPortalPortForConnectionId,
    kiSCSIGetHostInterfaceForConnectionId,
	kiSCSIInitiatorNumMethods
};

#endif /* defined(__ISCSI_HBA_TYPES_H__) */
