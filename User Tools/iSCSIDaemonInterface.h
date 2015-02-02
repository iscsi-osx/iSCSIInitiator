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
#include <MacTypes.h>

typedef int iSCSIDaemonHandle;



iSCSIDaemonHandle iSCSIDConnectToDaemon();

/*! Login session specified by a target name as configured in the database. */
errno_t iSCSIDLoginSession(iSCSIDaemonHandle handle,CFStringRef targetName);

/*! Logout session specified by a target name as configured in the database. */
errno_t iSCSIDLogoutSession(iSCSIDaemonHandle handle,CFStringRef targetName);


void iSCSIDDisconnectFromDaemon(iSCSIDaemonHandle handle);






#endif /* defined(__ISCSI_DAEMON_INTERFACE__) */
