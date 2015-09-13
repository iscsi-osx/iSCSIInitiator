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

/*! Connects to the iSCSI daemon.
 *  @return a handle to the daemon, or -1 if the daemon is not available. */
iSCSIDaemonHandle iSCSIDaemonConnect();

/*! Disconnects from the iSCSI daemon.  The handle is freed.
 *  @param handle the handle of the connection to free. */
void iSCSIDaemonDisconnect(iSCSIDaemonHandle handle);

/*! Logs into a target using a specific portal or all portals in the database.
 *  If an argument is supplied for portal, login occurs over the specified
 *  portal.  Otherwise, the daemon will attempt to login over all portals.
 *  @param handle a handle to a daemon connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use (use NULL for all portals).
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogin(iSCSIDaemonHandle handle,
                         iSCSITargetRef target,
                         iSCSIPortalRef portal,
                         enum iSCSILoginStatusCode * statusCode);

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to free.
 *  @param statusCode iSCSI response code indicating operation status. */
errno_t iSCSIDaemonLogout(iSCSIDaemonHandle handle,
                          iSCSITargetRef target,
                          iSCSIPortalRef portal,
                          enum iSCSILogoutStatusCode * statusCode);

/*! Creates an array of active target objects.
 *  @param handle a handle to a daemon connection.
 *  @return an array of active target objects or NULL if no targets are active. */
CFArrayRef iSCSIDaemonCreateArrayOfActiveTargets(iSCSIDaemonHandle handle);

/*! Creates an array of active portal objects.
 *  @param target the target to retrieve active portals.
 *  @param handle a handle to a daemon connection.
 *  @return an array of active target objects or NULL if no targets are active. */
CFArrayRef iSCSIDaemonCreateArrayOfActivePortalsForTarget(iSCSIDaemonHandle handle,
                                                          iSCSITargetRef target);

/*! Gets whether a target has an active session.
 *  @param target the target to test for an active session.
 *  @return true if the is an active session for the target; false otherwise. */
Boolean iSCSIDaemonIsTargetActive(iSCSIDaemonHandle handle,
                                  iSCSITargetRef target);

/*! Gets whether a portal has an active session.
 *  @param target the target to test for an active session.
 *  @param portal the portal to test for an active connection.
 *  @return true if the is an active connection for the portal; false otherwise. */
Boolean iSCSIDaemonIsPortalActive(iSCSIDaemonHandle handle,
                                  iSCSITargetRef target,
                                  iSCSIPortalRef portal);

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

/*! Creates a dictionary of session parameters for the session associated with
 *  the specified target, if one exists.
 *  @param handle a handle to a daemon connection.
 *  @param target the target to check for associated sessions to generate
 *  a dictionary of session parameters.
 *  @return a dictionary of session properties. */
CFDictionaryRef iSCSIDaemonCreateCFPropertiesForSession(iSCSIDaemonHandle handle,
                                                        iSCSITargetRef target);

/*! Creates a dictionary of connection parameters for the connection associated
 *  with the specified target and portal, if one exists.
 *  @param handle a handle to a daemon connection.
 *  @param target the target associated with the the specified portal.
 *  @param portal the portal to check for active connections to generate
 *  a dictionary of connection parameters.
 *  @return a dictionary of connection properties. */
CFDictionaryRef iSCSIDaemonCreateCFPropertiesForConnection(iSCSIDaemonHandle handle,
                                                           iSCSITargetRef target,
                                                           iSCSIPortalRef portal);

/*! Forces daemon to update discovery parameters from property list.
 *  @param handle a handle to a daemon connection.
 *  @return an error code indicating whether the operationg was successful. */
errno_t iSCSIDaemonUpdateDiscovery(iSCSIDaemonHandle handle);





#endif /* defined(__ISCSI_DAEMON_INTERFACE__) */
