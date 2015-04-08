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

/*! Function pointer indices.  These are the functions that can be called
 *	indirectly by calling IOCallScalarMethod(). */
enum functionNames {
	kiSCSIOpenInitiator,
	kiSCSICloseInitiator,
    kiSCSICreateSession,
    kiSCSIReleaseSession,
    kiSCSISetSessionOptions,
    kiSCSIGetSessionOptions,
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
    kiSCSISetConnectionOptions,
    kiSCSIGetConnectionOptions,
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
