/*!
 * @author		Nareg Sinenian
 * @file		iSCSIKernelInterfaceShared.h
 * @date		March 18, 2014
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI kernel extension definitions that are shared between
 *              kernel and user space.  These definitions are used by the
 *              kernel extension's client in user to access the iSCSI virtual
 *              host bus adapter (initiator).
 */

#ifndef __ISCSI_KERNEL_INTERFACE_SHARED_H__
#define __ISCSI_KERNEL_INTERFACE_SHARED_H__

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
enum iSCSIKernelNotificationTypes {
    
    /*! An asynchronous iSCSI message. */
    kiSCSIKernelNotificationAsyncMessage,
    
    /*! Notifies clients that the kernel extension or controller is going
     *  shut down.  Clients should release all resources. */
    kISCSIKernelNotificationTerminate,
    
    /*! Invalid notification message. */
    kiSCSIKernelNotificationInvalid
};


/*! Used to pass notifications from the kernel to the user-space daemon.
 *  The notification type is one of the notification types listed in 
 *  the enumerated type iSCSINotificationTypes. */
typedef struct {
    
    /*! Message haeder. */
    mach_msg_header_t header;
    
    /*! The notification type. */
    UInt8 notificationType;
    
    /*! Parameter associated with the notification (notificiation-specific). */
    UInt64 parameter1;
    
    /*! Parameter associated with the notification (notificiation-specific). */
    UInt64 parameter2;
    
    /*! Session identifier. */
    SID sessionId;
    
    /*! Connection identifier. */
    CID connectionId;
    
} iSCSIKernelNotificationMessage;


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
    SID sessionId;
    
    /*! Connection identifier. */
    CID connectionId;
    
} iSCSIKernelNotificationAsyncMessage;


/*! Function pointer indices.  These are the functions that can be called
 *	indirectly by calling IOCallScalarMethod(). */
enum functionNames {
	kiSCSIOpenInitiator,
	kiSCSICloseInitiator,
    kiSCSICreateSession,
    kiSCSIReleaseSession,
    kiSCSISetSessionOption,
    kiSCSIGetSessionOption,
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
    kiSCSISetConnectionOption,
    kiSCSIGetConnectionOption,
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

#endif /* defined(__ISCSI_KERNEL_INTERFACE_SHARED_H__) */
