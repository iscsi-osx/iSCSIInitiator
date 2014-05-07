/**
 * @author		Nareg Sinenian
 * @file		iSCSIKernelInterface.h
 * @date		March 14, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_KERNEL_INTERFACE_H__
#define __ISCSI_KERNEL_INTERFACE_H__

#include "iSCSIInterfaceShared.h"
#include "iSCSIPDUShared.h"
#include <sys/socket.h>
#include <sys/errno.h>

#include <IOKit/IOKitLib.h>
#include <stdint.h>
	
/** Opens a connection to the iSCSI initiator.  A connection must be
 *  successfully opened before any of the supporting functions below can be
 *  called. */
kern_return_t iSCSIKernelInitialize();

/** Closes a connection to the iSCSI initiator. */
kern_return_t iSCSIKernelCleanUp();

/** Allocates a new iSCSI session and returns a session qualifier ID.
 *  @return a valid session qualifier (part of the ISID, see RF3720) or
 *  0 if a new session could not be created. */
UInt16 iSCSIKernelCreateSession();

/** Releases an iSCSI session, including all connections associated with that
 *  session.
 *  @param sessionId the session qualifier part of the ISID. */
void iSCSIKernelReleaseSession(UInt16 sessionId);

/** Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetSessionOptions(UInt16 sessionId,
                                     iSCSISessionOptions * options);

/** Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionOptions(UInt16 sessionId,
                                     iSCSISessionOptions * options);

/** Allocates a new iSCSI connection associated with the particular session.
 *  @param sessionId the session to create a new connection for. 
 *  @param domain the IP domain (e.g., AF_INET or AF_INET6). 
 *  @param targetAddress the BSD socket structure used to identify the target. 
 *  @param hostAddress the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param connectionId the identifier of the new connection.
 *  @return a connection identifier using the last parameter, or an error code
 *  if a valid connection could not be created. */
errno_t iSCSIKernelCreateConnection(UInt16 sessionId,
                                    int domain,
                                    const struct sockaddr * targetAddress,
                                    const struct sockaddr * hostAddress,
                                    UInt32 * connectionId);

/** Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
void iSCSIKernelReleaseConnection(UInt16 sessionId,UInt32 connectionId);

/** Sends data over a kernel socket associated with iSCSI. The data sent should
 *  be specified by the buffer pointer to by data, with a length given by 
 *  length.  The size of the buffer (length) should include any padding required
 *  to achieve 4-byte alignment.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment to send over the connection.
 *  @param data the data segment of the PDU to send over the connection.
 *  @param length the length of the data block to send over the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSend(UInt16 sessionId,
                        UInt32 connectionId,
                        iSCSIPDUInitiatorBHS * bhs,
                        void * data,
                        size_t length);

/** Receives data over a kernel socket associated with iSCSI.  Function will
 *  receive the ENTIRE data segment of a PDU at once, and return the length
 *  of the buffer in length. This length includes any padding required to
 *  achieve 4-byte alignement required of PDUs.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received over the connection.
 *  @param data the data segment of the PDU received over the connection.
 *  @param length the length of the data block received.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelRecv(UInt16 sessionId,
                        UInt32 connectionId,
                        iSCSIPDUTargetBHS * bhs,
                        void * * data,
                        size_t * length);

/** Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetConnectionOptions(UInt16 sessionId,
                                        UInt32 connectionId,
                                        iSCSIConnectionOptions * options);

/** Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionOptions(UInt16 sessionId,
                                        UInt32 connectionId,
                                        iSCSIConnectionOptions * options);

/** Gets the connection Id for any active connection associated with session.
 *  This function can be used when a connection is required to service a
 *  session.
 *  @param sessionId the session for which to retreive a connection.
 *  @return an active connection Id for the specified session. */
UInt32 iSCSIKernelGetActiveConnection(UInt16 sessionId);

/** Activates an iSCSI connection.  Lets the
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateConnection(UInt16 sessionId,UInt32 connectionId);

/** Dectivates an iSCSI session.
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateConnection(UInt16 sessionId,UInt32 connectionId);

#endif /* defined(__ISCSI_KERNEL_INTERFACE_H__) */
