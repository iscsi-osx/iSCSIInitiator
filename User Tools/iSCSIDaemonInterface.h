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









#endif /* defined(__ISCSI_DAEMON_INTERFACE__) */
