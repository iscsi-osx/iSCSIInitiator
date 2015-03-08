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
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>

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
 *  @param targetAddress the BSD socket structure used to identify the target.
 *  @param hostAddress the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param sessionId the session identifier for the new session (returned).
 *  @param connectionId the identifier of the new connection (returned).
 *  @return An error code if a valid session could not be created. */
errno_t iSCSIKernelCreateSession(const char * targetName,
                                 const struct sockaddr_storage * targetAddress,
                                 const struct sockaddr_storage * hostAddress,
                                 SID * sessionId,
                                 CID * connectionId);

/*! Releases an iSCSI session, including all connections associated with that
 *  session (there is no requirement to release connections individually).
 *  @param sessionId the session qualifier part of the ISID.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelReleaseSession(SID sessionId);

/*! Sets configuration associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param config the configuration to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetSessionConfig(SID sessionId,iSCSIKernelSessionCfg * config);

/*! Gets configuration associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param config the configuration to get.  The user of this function is
 *  responsible for allocating and freeing the configuration struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionConfig(SID sessionId,iSCSIKernelSessionCfg * config);

/*! Allocates an additional iSCSI connection for a particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param targetAddress the BSD socket structure used to identify the target. 
 *  @param hostAddress the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param connectionId the identifier of the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelCreateConnection(SID sessionId,
                                    const struct sockaddr_storage * targetAddress,
                                    const struct sockaddr_storage * hostAddress,
                                    CID * connectionId);

/*! Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
errno_t iSCSIKernelReleaseConnection(SID sessionId,CID connectionId);

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
errno_t iSCSIKernelSend(SID sessionId,
                        CID connectionId,
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
errno_t iSCSIKernelRecv(SID sessionId,
                        CID connectionId,
                        iSCSIPDUTargetBHS * bhs,
                        void * * data,
                        size_t * length);

/*! Sets configuration associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param config the configuration to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetConnectionConfig(SID sessionId,
                                       CID connectionId,
                                       iSCSIKernelConnectionCfg * config);

/*! Gets configuration associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param config the configurations to get.  The user of this function is
 *  responsible for allocating and freeing the configuration struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionConfig(SID sessionId,
                                       CID connectionId,
                                       iSCSIKernelConnectionCfg * config);

/*! Activates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateConnection(SID sessionId,CID connectionId);

/*! Activates all iSCSI connections associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateAllConnections(SID sessionId);

/*! Dectivates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to deactivate.
 *  @param connectionId  connection to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateConnection(SID sessionId,CID connectionId);

/*! Dectivates all iSCSI sessions associated with a session.
 *  @param sessionId session associated with connections to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateAllConnections(SID sessionId);

/*! Gets the first connection (the lowest connectionId) for the
 *  specified session.
 *  @param sessionId obtain an connectionId for this session.
 *  @param connectionId the identifier of the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnection(SID sessionId,CID * connectionId);

/*! Gets the connection count for the specified session.
 *  @param sessionId obtain the connection count for this session.
 *  @param numConnections the connection count.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetNumConnections(SID sessionId,UInt32 * numConnections);

/*! Looks up the session identifier associated with a particular target name.
 *  @param targetName the IQN name of the target (e.q., iqn.2015-01.com.example)
 *  @param sessionId the session identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionIdFromTargetName(const char * targetName,
                                              SID * sessionId);

/*! Looks up the connection identifier associated with a particular connection address.
 *  @param sessionId the session identifier.
 *  @param address the name used when adding the connection (e.g., IP or DNS).
 *  @param port the port used when adding the connection.
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionIdFromAddress(SID sessionId,
                                              const char * targetAddr,
                                              const char * targetPort,
                                              CID * connectionId);

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionIds(UInt16 * sessionIds,
                                 UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionIds(SID sessionId,
                                    UInt32 * connectionIds,
                                    UInt32 * connectionCount);





#endif /* defined(__ISCSI_KERNEL_INTERFACE_H__) */
