/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDA.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI disks.
 */

#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"

typedef struct iSCSIDiskOperationContext {
    DASessionRef session;
    iSCSITargetRef target;
    iSCSIDACallback callback;
    void * context;
    CFIndex diskCount;

} iSCSIDiskOperationContext;

void iSCSIDADiskUnmountComplete(DADiskRef disk,DADissenterRef dissenter,void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    opContext->diskCount--;
    
    if(opContext->diskCount == 0) {
        (*opContext->callback)(opContext->target,opContext->context);
        free(opContext);
    }
}

void iSCSIDADiskMountComplete(DADiskRef disk,DADissenterRef dissenter,void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    opContext->diskCount--;
    
    if(opContext->diskCount == 0) {
        (*opContext->callback)(opContext->target,opContext->context);
        free(opContext);
    }
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAUnmountApplierFunc(io_object_t entry, void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    opContext->diskCount++;
    
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,opContext->session,entry);
    DADiskUnmount(disk,kDADiskUnmountOptionWhole,iSCSIDADiskUnmountComplete,context);
}


/*! Unmounts all media associated with a particular iSCSI session, and
 *  calls the specified callback function with a context parameter when
 *  all mounted volumes have been unmounted. */
void iSCSIDAUnmountForTarget(DASessionRef session,
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
    
    // Queue unmount all IOMedia objects
    if(target != IO_OBJECT_NULL)
        iSCSIIORegistryIOMediaApplyFunction(targetObj,&iSCSIDAUnmountApplierFunc,opContext);
}

/*! Callback function used to unmount all IOMedia objects. */
void iSCSIDAMountApplierFunc(io_object_t entry, void * context)
{
    iSCSIDiskOperationContext * opContext = (iSCSIDiskOperationContext*)context;
    opContext->diskCount++;
    
    DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault,opContext->session,entry);
    DADiskMount(disk,NULL,kDADiskMountOptionWhole,iSCSIDADiskMountComplete,context);
}

/*! Mounts all IOMedia associated with a particular iSCSI session, and
 *  calls the specified callback function with a context parameter when
 *  all existing volumes have been mounted. */
void iSCSIDAMountForTarget(DASessionRef session,
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
    
    // Queue mount all IOMedia objects
    if(target != IO_OBJECT_NULL)
        iSCSIIORegistryIOMediaApplyFunction(targetObj,&iSCSIDAMountApplierFunc,opContext);
}