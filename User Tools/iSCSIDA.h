/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDA.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI disks.
 */


#ifndef __ISCSI_DA_H__
#define __ISCSI_DA_H__

#include <DiskArbitration/DiskArbitration.h>
#include "iSCSITypesShared.h"

/*! Unmounts all IOMedia associated with a particular iSCSI session.
 *  @param sessionId the session identifier. */
void iSCSIDAUnmountIOMediaForSession(SID sessionId);

/*! Creates an array of strings that hold the BSD disk names for a particular
 *  iSCSI session.
 *  @param sessionId the session identifier.
 *  @return the array of strings of BSD names for the specified session. */
CFArrayRef iSCSIDACreateBSDDiskNamesForSession(SID sessionId);

/*! Creates an array of dictionaries that hold the properties of every IOMedia
 *  object associated with a particular session.
 *  @param sessionId the session identifier.
 *  @return an array of dictionaries of properties of every IOMedia object for
 *  the associated session. */
CFArrayRef iSCSIDACreateIOMediaCFPropertyForSession(SID sessionId);


#endif /* defined(__ISCSI_DA_H__) */
