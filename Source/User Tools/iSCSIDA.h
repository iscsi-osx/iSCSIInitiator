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
#include "iSCSITypes.h"

/*! Result of a mount or unmount disk operation. */
enum iSCSIDAOperationResult {
    
    /*! All volumes were successfully mounted or unmounted. */
    kiSCSIDAOperationSuccess,

    /*! Some volumes were successfully mounted or unmounted. */
    kISCSIDAOperationPartialSuccess,

    /*! No volumes were successfully mounted or unmounted. */
    kiSCSIDAOperationFail
};

/*! Mount and unmount operation callback function. */
typedef void (*iSCSIDACallback)(iSCSITargetRef,enum iSCSIDAOperationResult,void *);

/*! Mounts all IOMedia associated with a particular iSCSI session, and
 *  calls the specified callback function with a context parameter when
 *  all existing volumes have been mounted. */
void iSCSIDAMountForTarget(DASessionRef session,
                           DADiskUnmountOptions options,
                           iSCSITargetRef target,
                           iSCSIDACallback callback,
                           void * context);

/*! Unmounts all media associated with a particular iSCSI session, and
 *  calls the specified callback function with a context parameter when
 *  all mounted volumes have been unmounted. */
void iSCSIDAUnmountForTarget(DASessionRef session,
                             DADiskUnmountOptions options,
                             iSCSITargetRef target,
                             iSCSIDACallback callback,
                             void * context);


#endif /* defined(__ISCSI_DA_H__) */
