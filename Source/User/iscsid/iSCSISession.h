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

#ifndef __ISCSI_SESSION_H__
#define __ISCSI_SESSION_H__

#include <CoreFoundation/CoreFoundation.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <asl.h>

#include "iSCSITypes.h"
#include "iSCSIRFC3720Keys.h"

/*! Call to initialize iSCSI session management functions.  This function will
 *  initialize the kernel layer after which other session-related functions
 *  may be called.
 *  @param rl the runloop to use for executing session-related functions.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIInitialize(CFRunLoopRef rl);

/*! Called to cleanup kernel resources used by the iSCSI session management
 *  functions.  This function will close any connections to the kernel
 *  and stop processing messages related to the kernel.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICleanup();

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use for the new session.
 *  @param initiatorAuth specifies the initiator authentication parameters.
 *  @param targetAuth specifies the target authentication parameters.
 *  @param sessCfg the session configuration parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILoginSession(iSCSIMutableTargetRef target,
                          iSCSIPortalRef portal,
                          iSCSIAuthRef initiatorAuth,
                          iSCSIAuthRef targetAuth,
                          iSCSISessionConfigRef sessCfg,
                          iSCSIConnectionConfigRef connCfg,
                          SID * sessionId,
                          CID * connectionId,
                          enum iSCSILoginStatusCode * statusCode);

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param sessionId the session to free. */
errno_t iSCSILogoutSession(SID sessionId,
                           enum iSCSILogoutStatusCode * statusCode);

/*! Adds a new connection to an iSCSI session.
 *  @param sessionId the new session identifier.
 *  @param portal specifies the portal to use for the connection.
 *  @param initiatorAuth specifies the initiator authentication parameters.
 *  @param targetAuth specifies the target authentication parameters.
 *  @param connCfg the connection configuration parameters to use.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILoginConnection(SID sessionId,
                             iSCSIPortalRef portal,
                             iSCSIAuthRef initiatorAuth,
                             iSCSIAuthRef targetAuth,
                             iSCSIConnectionConfigRef connCfg,
                             CID * connectionId,
                             enum iSCSILoginStatusCode * statusCode);

/*! Removes a connection from an existing session.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILogoutConnection(SID sessionId,
                              CID connectionId,
                              enum iSCSILogoutStatusCode * statusCode);

/*! Queries a portal for available targets (utilizes iSCSI SendTargets).
 *  @param portal the iSCSI portal to query.
 *  @param auth specifies the authentication parameters to use.
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryPortalForTargets(iSCSIPortalRef portal,
                                   iSCSIAuthRef initiatorAuth,
                                   iSCSIMutableDiscoveryRecRef * discoveryRec,
                                   enum iSCSILoginStatusCode * statuscode);

/*! Retrieves a list of targets available from a give portal.
 *  @param portal the iSCSI portal to look for targets.
 *  @param initiatorAuth specifies the initiator authentication parameters.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryTargetForAuthMethod(iSCSIPortalRef portal,
                                      CFStringRef targetIQN,
                                      enum iSCSIAuthMethods * authMethod,
                                      enum iSCSILoginStatusCode * statusCode);

/*! Gets the session identifier associated with the specified target.
 *  @param targetIQN the name of the target.
 *  @return the session identiifer. */
SID iSCSIGetSessionIdForTarget(CFStringRef targetIQN);

/*! Gets the connection identifier associated with the specified portal.
 *  @param sessionId the session identifier.
 *  @param portal the portal connected on the specified session.
 *  @return the associated connection identifier. */
CID iSCSIGetConnectionIdForPortal(SID sessionId,iSCSIPortalRef portal);

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  @return an array of session identifiers. */
CFArrayRef iSCSICreateArrayOfSessionIds();

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @return an array of connection identifiers. */
CFArrayRef iSCSICreateArrayOfConnectionsIds(SID sessionId);

/*! Creates a target object for the specified session.
 *  @param sessionId the session identifier.
 *  @return target the target object. */
iSCSITargetRef iSCSICreateTargetForSessionId(SID sessionId);

/*! Creates a connection object for the specified connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @return portal information about the portal. */
iSCSIPortalRef iSCSICreatePortalForConnectionId(SID sessionId,CID connectionId);



/*! Creates a dictionary of session parameters for the session associated with
 *  the specified target, if one exists. The following keys are guaranteed
 *  to be in the dictionary:
 *
 *  kRFC3720_Key_InitialR2T                 (CFBoolean)
 *  kRFC3720_Key_ImmediateData              (CFBoolean)
 *  kRFC3720_Key_DataPDUInOrder             (CFBoolean)
 *  kRFC3720_Key_DataSequenceInOrder        (CFBoolean)
 *  kRFC3720_Key_MaxConnections             (CFNumberRef, kCFNumberIntType)
 *  kRFC3720_Key_MaxBurstLength             (CFNumberRef, kCFNumberIntType)
 *  kRFC3720_Key_FirstBurstLength           (CFNumberRef, kCFNumberIntType)
 *  kRFC3720_Key_MaxOutstandingR2T          (CFNumberRef, kCFNumberIntType)
 *  kRFC3720_Key_DefaultTime2Retain         (CFNumberRef, kCFNumberIntType)
 *  kRFC3720_Key_DefaultTime2Wait           (CFNumberRef, kCFNumberIntType)
 *  kRFC3720_Key_ErrorRecoveryLevel         (CFNumberRef, kCFNumberSInt8Type)
 *  kRFC3720_Key_TargetGroupPortalTag       (CFNumberRef, kCFNumberSInt16Type)
 *  kRFC3720_Key_TargetSessionId            (CFNumberRef, kCFNumberSInt16Type)
 *  kRFC3720_Key_SessionId                  (CFNumberRef, kCFNumberSInt16Type)
 *
 *  @param handle a handle to a daemon connection.
 *  @param target the target to check for associated sessions to generate
 *  a dictionary of session parameters.
 *  @return a dictionary of session properties. */
CFDictionaryRef iSCSICreateCFPropertiesForSession(iSCSITargetRef target);

/*! Creates a dictionary of connection parameters for the connection associated 
 *  with the specified target and portal, if one exists.  The following keys
 *  are guaranteed to be in the dictionary:
 *
 *  kRFC3720_Key_DataDigest                 (CFNumberRef)
 *  kRFC3720_Key_HeaderDigest               (CFNumberRef)
 *  kRFC3720_Key_MaxRecvDataSegmentLength   (CFNumberRef)
 *  kRFC3720_Key_ConnectionId               (CFNumberRef)
 *
 *  @param handle a handle to a daemon connection.
 *  @param target the target associated with the the specified portal.
 *  @param portal the portal to check for active connections to generate
 *  a dictionary of connection parameters.
 *  @return a dictionary of connection properties. */
CFDictionaryRef iSCSICreateCFPropertiesForConnection(iSCSITargetRef target,
                                                     iSCSIPortalRef portal);

/*! Sets the name of this initiator.  This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param initiatorIQN the initiator name. */
void iSCSISetInitiatorName(CFStringRef initiatorIQN);

/*! Sets the alias of this initiator.  This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param initiatorAlias the initiator alias. */
void iSCSISetInitiatorAlias(CFStringRef initiatorAlias);


#endif
