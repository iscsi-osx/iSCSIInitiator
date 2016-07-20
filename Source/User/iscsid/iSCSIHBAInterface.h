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
 * LIABLE FOR ANY DIRECT, INDIRECT, INConnectionIdentifierENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ISCSI_HBA_INTERFACE_H__
#define __ISCSI_HBA_INTERFACE_H__

#include "iSCSIHBATypes.h"
#include "iSCSITypesShared.h"
#include "iSCSIPDUShared.h"
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <IOKit/IOKitLib.h>
#include <stdint.h>


typedef struct __iSCSIHBAInterface * iSCSIHBAInterfaceRef;

/*! Notification context that is used when creating a new HBA
 *  instance. This struct is used to pass user-defined data
 *  when the iSCSIHBANotification callback functions are called. */
typedef struct __iSCSIHBANotificationContext {
    
    /*! Version of this struct (set to 0). */
    CFIndex version;
    
    /*! User-defined data. */
    void * info;
    
    /*! Retain callback function (may be NULL). */
    CFAllocatorRetainCallBack retain;
    
    /*! Release callback function (may be NULL). */
    CFAllocatorReleaseCallBack release;
    
    /*! Copy description callback function (may be NULL). */
    CFAllocatorCopyDescriptionCallBack copyDescription;
    
}  iSCSIHBANotificationContext;


/*! Callback function used to relay notifications from the kernel. */
typedef void (*iSCSIHBANotificationCallBack)(iSCSIHBAInterfaceRef interface,
                                             enum iSCSIHBANotificationTypes type,
                                             iSCSIHBANotificationMessage * msg,
                                             void * info);

/*! Opens a connection to the iSCSI initiator.  A connection must be
 *  successfully opened before any of the supporting functions below can be
 *  called.  A callback function is used to process notifications from the
 *  iSCSI kernel extension.
 *  @param allocator the allocator to use.
 *  @param callback the callback function to process notifications.
 *  @return a new iSCSI HBA interface or NULL if one could not be created. */
iSCSIHBAInterfaceRef iSCSIHBAInterfaceCreate(CFAllocatorRef allocator,
                                             iSCSIHBANotificationCallBack callback,
                                             iSCSIHBANotificationContext * context);

/*! Closes a connection to the iSCSI initiator.
 *  @return error code indicating the result of the operation. */
void iSCSIHBAInterfaceRelease(iSCSIHBAInterfaceRef interface);

/*! Schedules execution of various tasks, including handling of kernel notifications
 *  on for the specified interface instance over the designated runloop.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param runLoop the runloop to schedule
 *  @param runLoopMode the execution mode for the runLoop. */
void iSCSIHBAInterfaceScheduleWithRunloop(iSCSIHBAInterfaceRef interface,
                                          CFRunLoopRef runLoop,
                                          CFStringRef runLoopMode);

/*! Unschedules execution of various tasks, including handling of kernel notifications
 *  on for the specified interface instance over the designated runloop.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param runLoop the runloop to schedule
 *  @param runLoopMode the execution mode for the runLoop. */
void iSCSIHBAInterfaceUnscheduleWithRunloop(iSCSIHBAInterfaceRef interface,
                                            CFRunLoopRef runLoop,
                                            CFStringRef runLoopMode);

/*! Allocates a new iSCSI session in the kernel and creates an associated
 *  connection to the target portal. Additional connections may be added to the
 *  session by calling iSCSIHBAInterfaceCreateConnection().
 *  @param interface an instance of an iSCSIHBAInterface.
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
IOReturn iSCSIHBAInterfaceCreateSession(iSCSIHBAInterfaceRef interface,
                                        CFStringRef targetIQN,
                                        CFStringRef portalAddress,
                                        CFStringRef portalPort,
                                        CFStringRef hostInterface,
                                        const struct sockaddr_storage * remoteAddress,
                                        const struct sockaddr_storage * localAddress,
                                        SessionIdentifier * sessionId,
                                        ConnectionIdentifier * connectionId);

/*! Releases an iSCSI session, including all connections associated with that
 *  session (there is no requirement to release connections individually).
 *  @param sessionId the session qualifier part of the ISID.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceReleaseSession(iSCSIHBAInterfaceRef interface,
                                         SessionIdentifier sessionId);

/*! Sets parameter associated with a particular session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param parameter the parameter to set.
 *  @param paramVal the value for the specified parameter.
 *  @param paramSize the size, in bytes of paramVal.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceSetSessionParameter(iSCSIHBAInterfaceRef interface,
                                              SessionIdentifier sessionId,
                                              enum iSCSIHBASessionParameters parameter,
                                              void * paramVal,
                                              size_t paramSize);

/*! Gets parameter associated with a particular session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param parameter the parameter to get.
 *  @param paramVal the returned value for the specified parameter.
 *  @param paramSize the size, in bytes of paramVal.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceGetSessionParameter(iSCSIHBAInterfaceRef interface,
                                              SessionIdentifier sessionId,
                                              enum iSCSIHBASessionParameters parameter,
                                              void * paramVal,
                                              size_t paramSize);

/*! Allocates an additional iSCSI connection for a particular session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param portalAddress the portal address (IPv4/IPv6, or DNS name).
 *  @param portalPort the TCP port used to connect to the portal.
 *  @param hostInterface the name of the host interface adapter to use.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param connectionId the identifier of the new connection.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceCreateConnection(iSCSIHBAInterfaceRef interface,
                                           SessionIdentifier sessionId,
                                           CFStringRef portalAddress,
                                           CFStringRef portalPort,
                                           CFStringRef hostInterface,
                                           const struct sockaddr_storage * remoteAddress,
                                           const struct sockaddr_storage * localAddress,
                                           ConnectionIdentifier * connectionId);

/*! Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720). */
IOReturn iSCSIHBAInterfaceReleaseConnection(iSCSIHBAInterfaceRef interface,
                                            SessionIdentifier sessionId,
                                            ConnectionIdentifier connectionId);

/*! Sends data over a kernel socket associated with iSCSI. The data sent should
 *  be specified by the buffer pointer to by data, with a length given by
 *  length.  The size of the buffer (length) should include any padding required
 *  to achieve 4-byte alignment.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment to send over the connection.
 *  @param data the data segment of the PDU to send over the connection.
 *  @param length the length of the data block to send over the connection.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceSend(iSCSIHBAInterfaceRef interface,
                               SessionIdentifier sessionId,
                               ConnectionIdentifier connectionId,
                               iSCSIPDUInitiatorBHS * bhs,
                               void * data,
                               size_t length);

/*! Receives data over a kernel socket associated with iSCSI.  Function will
 *  receive the ENTIRE data segment of a PDU at once, and return the length
 *  of the buffer in length. This length includes any padding required to
 *  achieve 4-byte alignement required of PDUs.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received over the connection.
 *  @param data the data segment of the PDU received over the connection.
 *  @param length the length of the data block received.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceReceive(iSCSIHBAInterfaceRef interface,
                                  SessionIdentifier sessionId,
                                  ConnectionIdentifier connectionId,
                                  iSCSIPDUTargetBHS * bhs,
                                  void * * data,
                                  size_t * length);

/*! Sets parameter associated with a particular connection.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param parameter the parameter to set.
 *  @param paramVal the value for the specified parameter.
 *  @param paramSize the size, in bytes of paramVal.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceSetConnectionParameter(iSCSIHBAInterfaceRef interface,
                                                 SessionIdentifier sessionId,
                                                 ConnectionIdentifier connectionId,
                                                 enum iSCSIHBAConnectionParameters parameter,
                                                 void * paramVal,
                                                 size_t paramSize);

/*! Gets parameter associated with a particular connection.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param parameter the parameter to get.
 *  @param paramVal the returned value for the specified parameter.
 *  @param paramSize the size, in bytes of paramVal.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceGetConnectionParameter(iSCSIHBAInterfaceRef interface,
                                                 SessionIdentifier sessionId,
                                                 ConnectionIdentifier connectionId,
                                                 enum iSCSIHBAConnectionParameters parameter,
                                                 void * paramVal,
                                                 size_t paramSize);

/*! Activates an iSCSI connection associated with a session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceActivateConnection(iSCSIHBAInterfaceRef interface,
                                             SessionIdentifier sessionId,
                                             ConnectionIdentifier connectionId);

/*! Activates all iSCSI connections associated with a session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session associated with connection to activate.
 *  @return error code inidicating result of operation. */
IOReturn iSCSIHBAInterfaceActivateAllConnections(iSCSIHBAInterfaceRef interface,
                                                 SessionIdentifier sessionId);

/*! Dectivates an iSCSI connection associated with a session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session associated with connection to deactivate.
 *  @param connectionId  connection to deactivate.
 *  @return error code inidicating result of operation. */
IOReturn iSCSIHBAInterfaceDeactivateConnection(iSCSIHBAInterfaceRef interface,
                                               SessionIdentifier sessionId,
                                               ConnectionIdentifier connectionId);

/*! Dectivates all iSCSI sessions associated with a session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session associated with connections to deactivate.
 *  @return error code inidicating result of operation. */
IOReturn iSCSIHBAInterfaceDeactivateAllConnections(iSCSIHBAInterfaceRef interface,
                                                   SessionIdentifier sessionId);

/*! Gets the first connection (the lowest connectionId) for the
 *  specified session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId obtain an connectionId for this session.
 *  @param connectionId the identifier of the connection.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceGetConnection(iSCSIHBAInterfaceRef interface,
                                        SessionIdentifier sessionId,
                                        ConnectionIdentifier * connectionId);

/*! Gets the connection count for the specified session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId obtain the connection count for this session.
 *  @param numConnections the connection count.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceGetNumConnections(iSCSIHBAInterfaceRef interface,
                                            SessionIdentifier sessionId,
                                            UInt32 * numConnections);

/*! Looks up the session identifier associated with a particular target name.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param targetIQN the IQN name of the target (e.q., iqn.2015-01.com.example)
 *  @return sessionId the session identifier. */
SessionIdentifier iSCSIHBAInterfaceGetSessionIdForTargetIQN(iSCSIHBAInterfaceRef interface,
                                                            CFStringRef targetIQN);

/*! Looks up the connection identifier associated with a particular portal address.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId the session identifier.
 *  @param portalAddress the address passed to iSCSIHBAInterfaceCreateSession() or
 *  iSCSIHBAInterfaceCreateConnection() when the connection was created.
 *  @return the associated connection identifier. */
ConnectionIdentifier iSCSIHBAInterfaceGetConnectionIdForPortalAddress(iSCSIHBAInterfaceRef interface,
                                                                      SessionIdentifier sessionId,
                                                                      CFStringRef portalAddress);

/*! Gets an array of session identifiers for each session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionIds an array of session identifiers.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceGetSessionIds(iSCSIHBAInterfaceRef interface,
                                        SessionIdentifier * sessionIds,
                                        UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
IOReturn iSCSIHBAInterfaceGetConnectionIds(iSCSIHBAInterfaceRef interface,
                                           SessionIdentifier sessionId,
                                           ConnectionIdentifier * connectionIds,
                                           UInt32 * connectionCount);

/*! Creates a string containing the target IQN associated with a session.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session identifier.
 *  @param targetIQN the name of the target.
 *  @param size the size of the targetIQNCString buffer.
 *  @return error code indicating result of operation. */
CFStringRef iSCSIHBAInterfaceCreateTargetIQNForSessionId(iSCSIHBAInterfaceRef interface,
                                                         SessionIdentifier sessionId);

/*! Creates a string containing the address of the portal associated with
 *  the specified connection.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the portal address, or NULL if the session or
 *  connection was invalid. */
CFStringRef iSCSIHBAInterfaceCreatePortalAddressForConnectionId(iSCSIHBAInterfaceRef interface,
                                                                SessionIdentifier sessionId,
                                                                ConnectionIdentifier connectionId);

/*! Creates a string containing the TCP port of the portal associated with
 *  the specified connection.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the TCP port of the portal, or NULL if the
 *  session or connection was invalid. */
CFStringRef iSCSIHBAInterfaceCreatePortalPortForConnectionId(iSCSIHBAInterfaceRef interface,
                                                             SessionIdentifier sessionId,
                                                             ConnectionIdentifier connectionId);

/*! Creates a string containing the host interface used for the connection.
 *  @param interface an instance of an iSCSIHBAInterface.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the host interface name, or NULL if the
 *  session or connection was invalid. */
CFStringRef iSCSIHBAInterfaceCreateHostInterfaceForConnectionId(iSCSIHBAInterfaceRef interface,
                                                                SessionIdentifier sessionId,
                                                                ConnectionIdentifier connectionId);


#endif /* defined(__ISCSI_HBA_INTERFACE_H__) */
