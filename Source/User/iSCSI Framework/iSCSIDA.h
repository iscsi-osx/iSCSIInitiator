/*
 * Copyright (c) 2016, Nareg Sinenian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
