/*!
 * @author		Nareg Sinenian
 * @file		iSCSISession.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI session management functions.  This library
 *              depends on the user-space iSCSI PDU library to login, logout
 *              and perform discovery functions on iSCSI target nodes.
 */

#ifndef __ISCSI_SESSION_H__
#define __ISCSI_SESSION_H__

#include <CoreFoundation/CoreFoundation.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "iSCSITypes.h"

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param portal specifies the portal to use for the new session.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSICreateSession(iSCSIPortalRef portal,
                           iSCSITargetRef target,
                           iSCSIAuthRef auth,
                           UInt16 * sessionId,
                           UInt32 * connectionId,
                           enum iSCSIStatusCode * statusCode);

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param sessionId the session to free. */
errno_t iSCSIReleaseSession(UInt16 sessionId);

/*! Adds a new connection to an iSCSI session.
 *  @param portal specifies the portal to use for the connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIAddConnection(iSCSIPortalRef portal,
                           iSCSITargetRef target,
                           iSCSIAuthRef auth,
                           UInt16 sessionId,
                           UInt32 * connectionId,
                           enum iSCSIStatusCode * statusCode);

/*! Removes a connection from an existing session.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIRemoveConnection(UInt16 sessionId,
                              UInt32 connectionId,
                              enum iSCSIStatusCode * statusCode);

/*! Queries a portal for available targets.
 *  @param portal the iSCSI portal to query.
 *  @param targets an array of strings, where each string contains the name,
 *  alias, and portal associated with each target.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryPortalForTargets(iSCSIPortalRef portal,
                                   CFArrayRef * targets,
                                   enum iSCSIStatusCode * statuscode);

/*! Retrieves a list of targets available from a give portal.
 *  @param portal the iSCSI portal to look for targets.
 *  @param authMethods a comma-separated list of authentication methods
 *  as defined in the RFC3720 standard (version 1).
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryTargetForAuthMethods(iSCSIPortalRef portal,
                                       CFStringRef targetName,
                                       CFStringRef * authMethods,
                                       enum iSCSIStatusCode * statusCode);

/*! Retreives the initiator session identifier associated with this target.
 *  @param targetName the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIGetSessionIdForTarget(CFStringRef targetName,
                                   UInt16 * sessionId);

/*! Looks up the connection identifier associated with a particular connection address.
 *  @param sessionId the session identifier.
 *  @param address the name used when adding the connection (e.g., IP or DNS).
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetConnectionIdFromAddress(UInt16 sessionId,
                                        CFStringRef address,
                                        UInt32 * connectionId);

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxSessions.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetSessionIds(UInt16 ** sessionIds,UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.  
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxConnectionsPerSession.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetConnectionIds(UInt16 sessionId,
                              UInt32 ** connectionIds,
                              UInt32 * connectionCount);

/*! Sets the name of this initiator.  This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param initiatorName the initiator name.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISetInitiatiorName(CFStringRef initiatorName);

/*! Sets the alias of this initiator.  This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param initiatorAlias the initiator alias.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISetInitiatorAlias(CFStringRef initiatorAlias);

#endif
