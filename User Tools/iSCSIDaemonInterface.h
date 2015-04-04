/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemonInterface.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Defines interface used by client applications to access
 *              the iSCSIDaemon
 */


#ifndef __ISCSI_DAEMON_INTERFACE__
#define __ISCSI_DAEMON_INTERFACE__

#include <CoreFoundation/CoreFoundation.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <MacTypes.h>

#include "iSCSITypes.h"

typedef int iSCSIDaemonHandle;

iSCSIDaemonHandle iSCSIDaemonConnect();

void iSCSIDaemonDisconnect(iSCSIDaemonHandle handle);


/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param handle a handle to a daemon connection.
 *  @param portal specifies the portal to use for the new session.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessCfg the session configuration parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginSession(iSCSIDaemonHandle handle,
                                iSCSIPortalRef portal,
                                iSCSITargetRef target,
                                iSCSIAuthRef auth,
                                iSCSISessionConfigRef sessCfg,
                                iSCSIConnectionConfigRef connCfg,
                                SID * sessionId,
                                CID * connectionId,
                                enum iSCSILoginStatusCode * statusCode);

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to free.
 *  @param statusCode iSCSI response code indicating operation status. */
errno_t iSCSIDaemonLogoutSession(iSCSIDaemonHandle handle,
                                 SID sessionId,
                                 enum iSCSILogoutStatusCode * statusCode);

/*! Adds a new connection to an iSCSI session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the new session identifier.
 *  @param portal specifies the portal to use for the connection.
 *  @param auth specifies the authentication parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginConnection(iSCSIDaemonHandle handle,
                                   SID sessionId,
                                   iSCSIPortalRef portal,
                                   iSCSIAuthRef auth,
                                   iSCSIConnectionConfigRef connCfg,
                                   CID * connectionId,
                                   enum iSCSILoginStatusCode * statusCode);

/*! Removes a connection from an existing session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogoutConnection(iSCSIDaemonHandle handle,
                                    SID sessionId,
                                    CID connectionId,
                                    enum iSCSILogoutStatusCode * statusCode);

/*! Queries a portal for available targets.
 *  @param handle a handle to a daemon connection.
 *  @param portal the iSCSI portal to query.
 *  @param auth specifies the authentication parameters to use.
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonQueryPortalForTargets(iSCSIDaemonHandle handle,
                                         iSCSIPortalRef portal,
                                         iSCSIAuthRef auth,
                                         iSCSIMutableDiscoveryRecRef * discoveryRec,
                                         enum iSCSILoginStatusCode * statuscode);

/*! Retrieves a list of targets available from a give portal.
 *  @param handle a handle to a daemon connection.
 *  @param portal the iSCSI portal to look for targets.
 *  @param authMethod the preferred authentication method.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonQueryTargetForAuthMethod(iSCSIDaemonHandle handle,
                                            iSCSIPortalRef portal,
                                            CFStringRef targetIQN,
                                            enum iSCSIAuthMethods * authMethod,
                                            enum iSCSILoginStatusCode * statusCode);

/*! Retreives the initiator session identifier associated with this target.
 *  @param handle a handle to a daemon connection.
 *  @param targetIQN the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonGetSessionIdForTarget(iSCSIDaemonHandle handle,
                                         CFStringRef targetIQN,
                                         SID * sessionId);

/*! Looks up the connection identifier associated with a portal.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param portal the iSCSI portal.
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionIdForPortal(iSCSIDaemonHandle handle,
                                            SID sessionId,
                                            iSCSIPortalRef portal,
                                            CID * connectionId);

/*! Gets an array of session identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionIds an array of session identifiers.
 *  @return an array of session identifiers. */
CFArrayRef iSCSIDaemonCreateArrayOfSessionIds(iSCSIDaemonHandle handle);

/*! Gets an array of connection identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId session identifier.
 *  @return an array of connection identifiers. */
CFArrayRef iSCSIDaemonCreateArrayOfConnectionsIds(iSCSIDaemonHandle handle,
                                                  SID sessionId);

/*! Creates a target object for the specified session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @return target the target object. */
iSCSITargetRef iSCSIDaemonCreateTargetForSessionId(iSCSIDaemonHandle handle,
                                                   SID sessionId);

/*! Creates a connection object for the specified connection.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @return portal information about the portal. */
iSCSIPortalRef iSCSIDaemonCreatePortalForConnectionId(iSCSIDaemonHandle handle,
                                                      SID sessionId,
                                                      CID connectionId);

/*! Copies the configuration object associated with a particular session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @return  the configuration object associated with the specified session. */
iSCSISessionConfigRef iSCSIDaemonCopySessionConfig(iSCSIDaemonHandle handle,
                                                   SID sessionId);

/*! Copies the configuration object associated with a particular connection.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @return  the configuration object associated with the specified connection. */
iSCSIConnectionConfigRef iSCSIDaemonCopyConnectionConfig(iSCSIDaemonHandle handle,
                                                         SID sessionId,
                                                         CID connectionId);



#endif /* defined(__ISCSI_DAEMON_INTERFACE__) */
