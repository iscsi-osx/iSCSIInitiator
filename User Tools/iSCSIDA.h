/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDA.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI disks.
 */


#ifndef __ISCSI_DA_H__
#define __ISCSI_DA_H__

#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>
#include "iSCSITypesShared.h"
#include <IOKit/scsi/SCSITask.h>


/*! Mounts all IOMedia associated with a particular iSCSI session.
 *  @param targetIQN the name of the iSCSI target. */
void iSCSIDAMountIOMediaForTarget(CFStringRef targetIQN);

/*! Unmounts all media associated with a particular iSCSI session.
 *  @param targetIQN the name of the iSCSI target. */
void iSCSIDAUnmountIOMediaForTarget(CFStringRef targetIQN);

/*! Rescans the iSCSI bus (all targets) for new LUNs. */
void iSCSIDARescanBus();

/*! Rescans the iSCSI bus for new LUNs for a particular target only.
 *  @param tagetIQN the name of the iSCSI target. */
void iSCSIDARescanBusForTarget(CFStringRef targetIQN);

#endif /* defined(__ISCSI_DA_H__) */
