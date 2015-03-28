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

typedef CFDictionaryRef iSCSIDALUNRef;
typedef CFDictionaryRef iSCSIDATargetRef;


/*! Create an array of iSCSIDATarget objects by traversing the device tree.
 *  @return an array of target objects. */
CFArrayRef iSCSIDACreateArrayOfTargets();
 
/*! Creates an array of LUN objects for a particular target.
 *  @param target the target object.
 *  @return an array of LUN objects associated with the specified target. */
CFArrayRef iSCSIDACreateArrayOfLUNsForTarget(iSCSIDATargetRef target);

/*! Get the name associated with an iSCSIDATarget object.
 *  @param target the iSCSI disk arbitration target object.
 *  @return the IQN associated with the target. */
CFStringRef iSCSIDATargetGetName(iSCSIDATargetRef target);

/*! Returns the SCSI Logical Unit Number for the LUN object.
 *  @param LUN the LUN object.
 *  @return a SCSI Logical Unit Number. */
CFNumberRef iSCSIDAGetLUNIdentifier(iSCSIDALUNRef LUN);

/*! Returns capacity associated with the LUN object.
 *  @param LUN the LUN object.
 *  @return the capacity associated with the LUN object. */
CFNumberRef iSCSIDAGetLUNCapacity(iSCSIDALUNRef LUN);

/*! Returns manufacturer associated with the LUN object.
 *  @param LUN the LUN object.
 *  @return manufacturer associated with the LUN object. */
CFStringRef iSCSIDAGetLUNManufacturer(iSCSIDALUNRef LUN);

/*! Returns model associated with the LUN object.
 *  @param LUN the LUN object.
 *  @return model associated with the LUN object. */
CFStringRef iSCSIDAGetLUNModel(iSCSIDALUNRef LUN);

/*! Returns BSD disk name associated with the LUN object.
 *  @param LUN the LUN object.
 *  @return BSD disk name associated with the LUN object. */
CFStringRef iSCSIDAGetLUNBSDDiskName(iSCSIDALUNRef LUN);

/*! Mounts all IOMedia associated with a particular iSCSI session.
 *  @param targetName the name of the iSCSI target. */
void iSCSIDAMountIOMediaForTarget(CFStringRef targetName);

/*! Unmounts all media associated with a particular iSCSI session.
 *  @param targetName the name of the iSCSI target. */
void iSCSIDAUnmountIOMediaForTarget(CFStringRef targetName);

/*! Creates an array of strings that hold the BSD disk names for a particular
 *  iSCSI session.
 *  @param targetName the name of the iSCSI target.
 *  @return the array of strings of BSD names for the specified session. */
CFArrayRef iSCSIDACreateArrayOfBSDDiskNamesForSession(SID sessionId);


#endif /* defined(__ISCSI_DA_H__) */
