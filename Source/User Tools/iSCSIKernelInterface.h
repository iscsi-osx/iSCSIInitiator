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


/*! Callback function used to relay notifications from the kernel. */
typedef void (*iSCSIKernelNotificationCallback)(enum iSCSIKernelNotificationTypes type,
                                                iSCSIKernelNotificationMessage * msg);

/*! Opens a connection to the iSCSI initiator.  A connection must be
 *  successfully opened before any of the supporting functions below can be
 *  called.  A callback function is used to process notifications from the 
 *  iSCSI kernel extension.
 *  @param callback the callback function to process notifications.
 *  @return error code indicating the result of the operation. */
errno_t iSCSIKernelInitialize(iSCSIKernelNotificationCallback callback);

/*! Closes a connection to the iSCSI initiator.
 *  @return error code indicating the result of the operation. */
errno_t iSCSIKernelCleanup();

/*! Creates a run loop source that is used to run the the notification callback
 *  function that was registered during the call to iSCSIKernelInitialize(). */
CFRunLoopSourceRef iSCSIKernelCreateRunLoopSource();

/*! Allocates a new iSCSI session in the kernel and creates an associated
 *  connection to the target portal. Additional connections may be added to the
 *  session by calling iSCSIKernelCreateConnection().
 *  @param targetIQN the name of the target.
 *  @param portalAddress the portal address (IPv4/IPv6, or DNS name).
 *  @param portalPort the TCP port used to connect to the portal.
 *  @param hostInterface the name of the host interface adapter to use.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param sessionId the session identifier for the new session (returned).
 *  @param connectionId the identifier of the new connection (returned).
 *  @return error code indicating the result of the operation. */
errno_t iSCSIKernelCreateSession(CFStringRef targetIQN,
                                 CFStringRef portalAddress,
                                 CFStringRef portalPort,
                                 CFStringRef hostInterface,
                                 const struct sockaddr_storage * portalSockAddr,
                                 const struct sockaddr_storage * hostSockAddr,
                                 SID * sessionId,
                                 CID * connectionId);

/*! Releases an iSCSI session, including all connections associated with that
 *  session (there is no requirement to release connections individually).
 *  @param sessionId the session qualifier part of the ISID.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelReleaseSession(SID sessionId);

/*! Sets option associated with a particular session.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param option the option to set.
 *  @param optVal the value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetSessionOpt(SID sessionId,
                                 enum iSCSIKernelSessionOptTypes option,
                                 void * optVal,
                                 size_t optSize);

/*! Gets option associated with a particular session.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param option the option to get.
 *  @param optVal the returned value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionOpt(SID sessionId,
                                 enum iSCSIKernelSessionOptTypes option,
                                 void * optVal,
                                 size_t optSize);

/*! Allocates an additional iSCSI connection for a particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param portalAddress the portal address (IPv4/IPv6, or DNS name).
 *  @param portalPort the TCP port used to connect to the portal.
 *  @param hostInterface the name of the host interface adapter to use.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param connectionId the identifier of the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelCreateConnection(SID sessionId,
                                    CFStringRef portalAddress,
                                    CFStringRef portalPort,
                                    CFStringRef hostInterface,
                                    const struct sockaddr_storage * portalSockAddr,
                                    const struct sockaddr_storage * hostSockAddr,
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

/*! Sets option associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param option the option to set.
 *  @param optVal the value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetConnectionOpt(SID sessionId,
                                    CID connectionId,
                                    enum iSCSIKernelConnectionOptTypes option,
                                    void * optVal,
                                    size_t optSize);

/*! Gets option associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param option the option to get.
 *  @param optVal the returned value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionOpt(SID sessionId,
                                    CID connectionId,
                                    enum iSCSIKernelConnectionOptTypes option,
                                    void * optVal,
                                    size_t optSize);

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
 *  @param targetIQN the IQN name of the target (e.q., iqn.2015-01.com.example)
 *  @return sessionId the session identifier. */
SID iSCSIKernelGetSessionIdForTargetIQN(CFStringRef targetIQN);

/*! Looks up the connection identifier associated with a particular portal address.
 *  @param sessionId the session identifier.
 *  @param portalAddress the address passed to iSCSIKernelCreateSession() or
 *  iSCSIKernelCreateConnection() when the connection was created.
 *  @return the associated connection identifier. */
CID iSCSIKernelGetConnectionIdForPortalAddress(SID sessionId,
                                               CFStringRef portalAddress);

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionIds(SID * sessionIds,
                                 UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionIds(SID sessionId,
                                    CID * connectionIds,
                                    UInt32 * connectionCount);

/*! Creates a string containing the target IQN associated with a session.
 *  @param sessionId session identifier.
 *  @param targetIQN the name of the target.
 *  @param size the size of the targetIQNCString buffer.
 *  @return error code indicating result of operation. */
CFStringRef iSCSIKernelCreateTargetIQNForSessionId(SID sessionId);

/*! Creates a string containing the address of the portal associated with
 *  the specified connection.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the portal address, or NULL if the session or
 *  connection was invalid. */
CFStringRef iSCSIKernelCreatePortalAddressForConnectionId(SID sessionId,CID connectionId);

/*! Creates a string containing the TCP port of the portal associated with
 *  the specified connection.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the TCP port of the portal, or NULL if the
 *  session or connection was invalid. */
CFStringRef iSCSIKernelCreatePortalPortForConnectionId(SID sessionId,CID connectionId);

/*! Creates a string containing the host interface used for the connection.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the host interface name, or NULL if the
 *  session or connection was invalid. */
CFStringRef iSCSIKernelCreateHostInterfaceForConnectionId(SID sessionId,CID connectionId);


#endif /* defined(__ISCSI_KERNEL_INTERFACE_H__) */
