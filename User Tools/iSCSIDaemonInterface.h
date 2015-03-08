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
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginSession(iSCSIDaemonHandle handle,
                                iSCSIPortalRef portal,
                                iSCSITargetRef target,
                                iSCSIAuthRef auth,
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
 *  @param portal specifies the portal to use for the connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginConnection(iSCSIDaemonHandle handle,
                                   iSCSIPortalRef portal,
                                   iSCSITargetRef target,
                                   iSCSIAuthRef auth,
                                   SID sessionId,
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
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonQueryPortalForTargets(iSCSIDaemonHandle handle,
                                         iSCSIPortalRef portal,
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
                                            CFStringRef targetName,
                                            enum iSCSIAuthMethods * authMethod,
                                            enum iSCSILoginStatusCode * statusCode);

/*! Retreives the initiator session identifier associated with this target.
 *  @param handle a handle to a daemon connection.
 *  @param targetName the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonGetSessionIdForTarget(iSCSIDaemonHandle handle,
                                         CFStringRef targetName,
                                         SID * sessionId);

/*! Looks up the connection identifier associated with a particular connection address.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param address the name used when adding the connection (e.g., IP or DNS).
 *  @param connectionId the associated connection identifier.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionIdForPortal(iSCSIDaemonHandle handle,
                                              SID sessionId,
                                              CFStringRef address,
                                              CID * connectionId);

/*! Gets an array of session identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionIds an array of session identifiers.
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxSessions.
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetSessionIds(iSCSIDaemonHandle handle,
                                 SID * sessionIds,
                                 UInt16 * sessionCount);

/*! Gets an array of connection identifiers for each session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  This array must be user-allocated with a capacity defined by kiSCSIMaxConnectionsPerSession.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionIds(iSCSIDaemonHandle handle,
                                    SID sessionId,
                                    UInt32 * connectionIds,
                                    UInt32 * connectionCount);

/*! Gets information about a particular session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param options the optionts for the specified session.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetSessionConfig(iSCSIDaemonHandle handle,
                                  SID sessionId,
                                  iSCSIKernelSessionCfg * options);

/*! Gets information about a particular session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @param options the optionts for the specified session.
 *  @return error code indicating result of operation. */
errno_t iSCSIDaemonGetConnectionConfig(iSCSIDaemonHandle handle,
                                     SID sessionId,
                                     CID connectionId,
                                     iSCSIKernelConnectionCfg * options);

#endif /* defined(__ISCSI_DAEMON_INTERFACE__) */
