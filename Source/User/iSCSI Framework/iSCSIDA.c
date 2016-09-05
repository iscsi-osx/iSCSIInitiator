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

#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"

typedef struct iSCSIDiskOperationContext {
    DASessionRef session;
    iSCSITargetRef target;
    iSCSIDACallback callback;
    void * context;
    CFIndex diskCount;
    CFIndex successCount;
    CFIndex processedCount;
    DADiskUnmountOptions options;

} iSCSIDiskOperationContext;

void iSCSIDADiskUnmountComplete(DADiskRef disk,DADissenterRef dissenter,void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    opContext->processedCount++;
    
    if(!dissenter)
        opContext->successCount++;
    
    if(opContext->diskCount == opContext->processedCount) {
        enum iSCSIDAOperationResult result = kISCSIDAOperationPartialSuccess;
        
        if(opContext->processedCount == opContext->successCount)
            result = kiSCSIDAOperationSuccess;
        else if(opContext->successCount == 0)
            result = kiSCSIDAOperationFail;
        
        // If callback was specified...
        if(opContext->callback)
            (*opContext->callback)(opContext->target,result,opContext->context);
        
        free(opContext);
    }
}

void iSCSIDADiskMountComplete(DADiskRef disk,DADissenterRef dissenter,void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    opContext->processedCount++;
    
    if(!dissenter)
        opContext->successCount++;
    
    if(opContext->diskCount == opContext->processedCount) {
        enum iSCSIDAOperationResult result = kISCSIDAOperationPartialSuccess;
        
        if(opContext->processedCount == opContext->successCount)
            result = kiSCSIDAOperationSuccess;
        else if(opContext->successCount == 0)
            result = kiSCSIDAOperationFail;
        
        // If callback was specified...
        if(opContext->callback)
            (*opContext->callback)(opContext->target,result,opContext->context);
        
        free(opContext);
    }
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAUnmountApplierFunc(io_object_t entry, void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,opContext->session,entry);
    
    if(disk) {
        DADiskUnmount(disk,opContext->options,iSCSIDADiskUnmountComplete,context);
        CFRelease(disk);
        opContext->diskCount++;
    }
}


/*! Unmounts all media associated with a particular iSCSI session, and
 *  calls the specified callback function with a context parameter when
 *  all mounted volumes have been unmounted. */
void iSCSIDAUnmountForTarget(DASessionRef session,
                             DADiskUnmountOptions options,
                             iSCSITargetRef target,
                             iSCSIDACallback callback,
                             void * context)
{
    // Find the target associated with the session
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    io_object_t targetObj = iSCSIIORegistryGetTargetEntry(targetIQN);
    
    
    iSCSIDiskOperationContext * opContext = malloc(sizeof(iSCSIDiskOperationContext));
    opContext->callback = callback;
    opContext->session = session;
    opContext->target = (void*)target;
    opContext->context = context;
    opContext->diskCount = 0;
    opContext->processedCount = 0;
    opContext->successCount = 0;
    opContext->options = options;
    
    // Queue unmount all IOMedia objects
    if(target != IO_OBJECT_NULL)
        iSCSIIORegistryIOMediaApplyFunction(targetObj,&iSCSIDAUnmountApplierFunc,opContext);
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAMountApplierFunc(io_object_t entry, void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,opContext->session,entry);
    
    if(disk) {
        DADiskMount(disk,NULL,opContext->options,iSCSIDADiskMountComplete,context);
        CFRelease(disk);
        opContext->diskCount++;
    }
}

/*! Mounts all IOMedia associated with a particular iSCSI session, and
 *  calls the specified callback function with a context parameter when
 *  all existing volumes have been mounted. */
void iSCSIDAMountForTarget(DASessionRef session,
                           DADiskUnmountOptions options,
                           iSCSITargetRef target,
                           iSCSIDACallback callback,
                           void * context)
{
    // Find the target associated with the session
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    io_object_t targetObj = iSCSIIORegistryGetTargetEntry(targetIQN);
    
    iSCSIDiskOperationContext * opContext = malloc(sizeof(iSCSIDiskOperationContext));
    opContext->callback = callback;
    opContext->session = session;
    opContext->target = (void*)target;
    opContext->context = context;
    opContext->diskCount = 0;
    opContext->processedCount = 0;
    opContext->successCount = 0;
    opContext->options = options;
    
    // Queue mount all IOMedia objects
    if(target != IO_OBJECT_NULL)
        iSCSIIORegistryIOMediaApplyFunction(targetObj,&iSCSIDAMountApplierFunc,opContext);
}