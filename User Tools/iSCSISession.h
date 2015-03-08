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
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use for the new session.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessCfg the session configuration parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILoginSession(iSCSITargetRef target,
                          iSCSIPortalRef portal,
                          iSCSIAuthRef auth,
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
 *  @param auth specifies the authentication parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILoginConnection(SID sessionId,
                             iSCSIPortalRef portal,
                             iSCSIAuthRef auth,
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

/*! Queries a portal for available targets.
 *  @param portal the iSCSI portal to query.
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryPortalForTargets(iSCSIPortalRef portal,
                                   iSCSIMutableDiscoveryRecRef * discoveryRec,
                                   enum iSCSILoginStatusCode * statuscode);

/*! Retrieves a list of targets available from a give portal.
 *  @param portal the iSCSI portal to look for targets.
 *  @param authMethod the preferred authentication method.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryTargetForAuthMethod(iSCSIPortalRef portal,
                                      CFStringRef targetName,
                                      enum iSCSIAuthMethods * authMethod,
                                      enum iSCSILoginStatusCode * statusCode);

/*! Gets the session identifier associated with the specified target.
 *  @param targetName the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIGetSessionIdForTarget(CFStringRef targetName,
                                   SID * sessionId);

/*! Gets the connection identifier associated with the specified portal.
 *  @param sessionId the session identifier.
 *  @param portal the portal connected on the specified session.
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetConnectionIdForPortal(SID sessionId,
                                      iSCSIPortalRef portal,
                                      CID * connectionId);

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxSessions.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetSessionIds(SID * sessionIds,UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.  
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxConnectionsPerSession.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetConnectionIds(SID sessionId,
                              UInt32 * connectionIds,
                              UInt32 * connectionCount);

/*! Gets the target object associated with the specified session.
 *  @param sessionId the session identifier.
 *  @param target the target object.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetTargetForSessionId(SID sessionId,iSCSITargetRef * target);

/*! Gets the portal object associated with a particular connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @param portal information about the portal.
 *  @return error code indicating result of operation. */
errno_t iSCSIGetPortalForConnectionId(SID sessionId,
                                      CID connectionId,
                                      iSCSIPortalRef * portal);

/*! Gets configuration associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param config the configuration to get.  The user of this function is
 *  responsible for allocating and freeing the configuration struct.
 *  @return error code indicating result of operation. */
errno_t iSCSICopySessionConfig(SID sessionId,iSCSISessionConfigRef * config);

/*! Gets configuration associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param config the configurations to get.  The user of this function is
 *  responsible for allocating and freeing the configuration struct.
 *  @return error code indicating result of operation. */
errno_t iSCSICopyConnectionConfig(SID sessionId,
                                  CID connectionId,
                                  iSCSIConnectionConfigRef * config);

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
