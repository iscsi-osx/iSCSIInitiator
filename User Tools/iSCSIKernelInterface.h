/*!
 * @author		Nareg Sinenian
 * @file		iSCSIKernelInterface.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_KERNEL_INTERFACE_H__
#define __ISCSI_KERNEL_INTERFACE_H__

#include "iSCSIKernelInterfaceShared.h"
#include "iSCSITypesShared.h"
#include "iSCSIPDUShared.h"
#include <sys/socket.h>
#include <sys/errno.h>

#include <IOKit/IOKitLib.h>
#include <stdint.h>
	
/*! Opens a connection to the iSCSI initiator.  A connection must be
 *  successfully opened before any of the supporting functions below can be
 *  called. */
kern_return_t iSCSIKernelInitialize();

/*! Closes a connection to the iSCSI initiator. */
kern_return_t iSCSIKernelCleanUp();

/*! Allocates a new iSCSI session in the kernel and creates an associated
 *  connection to the target portal. Additional connections may be added to the
 *  session by calling iSCSIKernelCreateConnection().
 *  @param targetName the name of the target, or NULL if discovery session.
 *  @param domain the IP domain (e.g., AF_INET or AF_INET6).
 *  @param targetAddress the BSD socket structure used to identify the target.
 *  @param hostAddress the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param sessionId the session identifier for the new session (returned).
 *  @param connectionId the identifier of the new connection (returned).
 *  @return An error code if a valid session could not be created. */
errno_t iSCSIKernelCreateSession(const char * targetName,
                                 int domain,
                                 const struct sockaddr * targetAddress,
                                 const struct sockaddr * hostAddress,
                                 UInt16 * sessionId,
                                 UInt32 * connectionId);

/*! Releases an iSCSI session, including all connections associated with that
 *  session (there is no requirement to release connections individually).
 *  @param sessionId the session qualifier part of the ISID. */
void iSCSIKernelReleaseSession(UInt16 sessionId);

/*! Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetSessionOptions(UInt16 sessionId,
                                     iSCSISessionOptions * options);

/*! Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionOptions(UInt16 sessionId,
                                     iSCSISessionOptions * options);

/*! Allocates an additional iSCSI connection for a particular session.
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

/*! Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
void iSCSIKernelReleaseConnection(UInt16 sessionId,UInt32 connectionId);

/*! Sends data over a kernel socket associated with iSCSI. The data sent should
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

/*! Receives data over a kernel socket associated with iSCSI.  Function will
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

/*! Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetConnectionOptions(UInt16 sessionId,
                                        UInt32 connectionId,
                                        iSCSIConnectionOptions * options);

/*! Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionOptions(UInt16 sessionId,
                                        UInt32 connectionId,
                                        iSCSIConnectionOptions * options);

/*! Activates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateConnection(UInt16 sessionId,UInt32 connectionId);

/*! Activates all iSCSI connections associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateAllConnections(UInt16 sessionId);

/*! Dectivates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to deactivate.
 *  @param connectionId  connection to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateConnection(UInt16 sessionId,UInt32 connectionId);

/*! Dectivates all iSCSI sessions associated with a session.
 *  @param sessionId session associated with connections to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateAllConnections(UInt16 sessionId);

/*! Gets the first connection (the lowest connectionId) for the
 *  specified session.
 *  @param sessionId obtain an connectionId for this session.
 *  @param connectionId the identifier of the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnection(UInt16 sessionId,UInt32 * connectionId);

/*! Gets the connection count for the specified session.
 *  @param sessionId obtain the connection count for this session.
 *  @param numConnections the connection count.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetNumConnections(UInt16 sessionId,UInt32 * numConnections);

/*! Looks up the session identifier associated with a particular target name.
 *  @param targetName the IQN name of the target (e.q., iqn.2015-01.com.example)
 *  @param sessionId the session identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionIdFromTargetName(const char * targetName,
                                              UInt16 * sessionId);

/*! Looks up the connection identifier associated with a particular connection address.
 *  @param sessionId the session identifier.
 *  @param address the name used when adding the connection (e.g., IP or DNS).
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionIdFromAddress(UInt16 sessionId,
                                              const char * address,
                                              UInt32 * connectionId);

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionIds(UInt16 ** sessionIds,
                                 UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionIds(UInt16 sessionId,
                                    UInt32 ** connectionIds,
                                    UInt32 * connectionCount);





#endif /* defined(__ISCSI_KERNEL_INTERFACE_H__) */
