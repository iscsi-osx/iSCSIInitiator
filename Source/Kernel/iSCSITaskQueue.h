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

#ifndef __ISCSI_TASK_QUEUE_H__
#define __ISCSI_TASK_QUEUE_H__

#include <IOKit/IOService.h>
#include <IOKit/IOEventSource.h>
#include <kern/queue.h>

#include "iSCSIKernelClasses.h"
#include "iSCSITypesKernel.h"
#include "iSCSIVirtualHBA.h"

struct iSCSITask;

/*! Provides an iSCSI task queue for an iSCSI HBA.  The HBA queues tasks as
 *  it receives them from the SCSI layer by calling queueTask().
 *  This queue will invoke a callback function gated against
 *  the HBA workloop to process new tasks as existing tasks are completed.
 *  Once the task is processed, the HBA should call completeCurrentTask() to 
 *  let the queue know that the task has been processed. */
class iSCSITaskQueue : public IOEventSource
{
    OSDeclareDefaultStructors(iSCSITaskQueue);

public:
    
    /*! Pointer to the method that is called (within the driver's workloop)
	 *	when data becomes available at a network socket. */
    typedef bool (*Action) (iSCSIVirtualHBA * owner,
                            iSCSISession * session,
                            iSCSIConnection * connection,
                            UInt32 initiatorTaskTag);
	
	/*! Initializes the event source with an owner and an action.
	 *	@param owner the owner that this event source will be attached to.
	 *	@param action pointer to a function to call when processing
	 *	interrupts.  This function is called by checkForWork() and executes in
	 *	the owner's workloop.
	 *	@param session the session object.
     *  @param connection the connection object.
	 *	@return true if the event source was successfully initialized. */
	virtual bool init(iSCSIVirtualHBA * owner,
                      iSCSITaskQueue::Action action,
                      iSCSISession * session,
                      iSCSIConnection * connection);
    
    /*! Queues a new iSCSI task for delayed processing. 
     *  @param initiatorTaskTag the iSCSI task tag associated with the task. */
    void queueTask(UInt32 initiatorTaskTag);
    
    /*! Removes a task from the queue (either the task has been successfully
     *  completed or aborted).
     *  @return the iSCSI task tag for the task that was just completed. */
    UInt32 completeCurrentTask();
    
    /*! Removes all tasks from the queue. */
    void clearTasksFromQueue();
    
    /*! Gets the iSCSI task tag of the task that is current being processed.
     *  @return iSCSI task tag of the current task. */
    UInt32 getCurrentTask();
    
protected:
    
    /*! Called by the attached work loop to check if there is any processing
	 *	to be completed.  This fu	nction will call the action method pointed
	 *	to by this object.
	 *	@return true if there was work, false otherwise. */
	virtual bool checkForWork();

private:
    
    /*! The iSCSI session associated with this event source. */
    iSCSISession * session;
    
    /*! The iSCSI connection associated with this event source. */
    iSCSIConnection * connection;
    
    queue_head_t taskQueue;
    
    bool newTask;
    
};

#endif