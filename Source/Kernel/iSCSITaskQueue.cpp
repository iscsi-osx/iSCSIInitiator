/*!
 * @author      Nareg Sinenian
 * @file        iSCSITaskQueue.cpp
 * @version     1.0
 * @copyright   (c) 2014-2015 Nareg Sinenian. All rights reserved.
 */

#include "iSCSITaskQueue.h"

#define super IOEventSource

struct iSCSITask {
    queue_chain_t queueChain;
    UInt32 initiatorTaskTag;
};

OSDefineMetaClassAndStructors(iSCSITaskQueue,IOEventSource);

bool iSCSITaskQueue::init(iSCSIVirtualHBA * owner,
                          iSCSITaskQueue::Action action,
                          iSCSISession * session,
                          iSCSIConnection * connection)
{
	// Initialize superclass, check validity and store socket handle
	if(!super::init(owner,(IOEventSource::Action) action))
        return false;
	
    iSCSITaskQueue::session = session;
    iSCSITaskQueue::connection = connection;
    
    // Initialize task queue to store parallel SCSI tasks for processing
    queue_init(&taskQueue);

    newTask = false;
    
	return true;
}

/*! Queues a new iSCSI task for delayed processing.
 *  @param initiatorTaskTag the iSCSI task tag associated with the task. */
void iSCSITaskQueue::queueTask(UInt32 initiatorTaskTag)
{
    // Signal the workloop thread that work is available only if this is
    // the only task in the queue (otherwise the task preceding this is
    // being processed; we'll get to this once that's done).
    iSCSITask * task = (iSCSITask*)IOMalloc(sizeof(iSCSITask));
    task->initiatorTaskTag = initiatorTaskTag;
    
    if(!onThread())
        OSDynamicCast(iSCSIVirtualHBA,owner)->GetCommandGate();
    
    bool firstTaskInQueue = false;
    if(queue_empty(&taskQueue))
        firstTaskInQueue = true;
    
    queue_enter(&taskQueue,task,iSCSITask *,queueChain);
    
    // Signal the workloop to process a new task...
    if(firstTaskInQueue) {
        newTask = true;
        
        if(getWorkLoop())
            signalWorkAvailable();
    }
}

/*! Removes a task from the queue (either the task has been successfully
 *  completed or aborted).
 *  @return the iSCSI task tag for the task that was just completed. */
UInt32 iSCSITaskQueue::completeCurrentTask()
{
    UInt32 taskTag = 0;
    iSCSITask * task = NULL;
    
    // Do nothing if the queue is empty
    if(queue_empty(&taskQueue))
        return taskTag;
    
    // Remove the completed task (at the head of the queue) and then
    // move onto the next task if one exists
    if(!onThread())
        OSDynamicCast(iSCSIVirtualHBA,owner)->GetCommandGate();

    queue_remove_first(&taskQueue,task,iSCSITask *, queueChain);

    if(task) {
        taskTag = task->initiatorTaskTag;
        IOFree(task,sizeof(iSCSITask));
    }
    
    // If there are still tasks to process let the HBA know...
    if(!queue_empty(&taskQueue)) {
        newTask = true;
        if(getWorkLoop())
            signalWorkAvailable();
    }
    return taskTag;
}

/*! Gets the iSCSI task tag of the task that is current being processed.
 *  @return iSCSI task tag of the current task. */
UInt32 getCurrentTask()
{
    return 0;
}

bool iSCSITaskQueue::checkForWork()
{
    if(!isEnabled())
        return false;
    
    // Check task flag before proceeding
    if(!newTask)
        return false;
    
    newTask = false;

    // Validate action & owner, then call action on our owner & pass in socket
    // this function will continue processing the task
    if(action && owner) {
 
        UInt32 taskTag;
        
        if(!onThread())
            OSDynamicCast(iSCSIVirtualHBA,owner)->GetCommandGate();
        
        if(queue_empty(&taskQueue))
            return false;
        
        iSCSITask * task = (iSCSITask *)queue_first(&taskQueue);
        taskTag = task->initiatorTaskTag;
        
        (*action)(owner,session,connection,taskTag);
    }
   
    // Tell workloop thread not to call us again until we signal again...
	return false;
}

/*! Removes all tasks from the queue. */
void iSCSITaskQueue::clearTasksFromQueue()
{
    // Ensure the event source is disabled before proceeding...
    disable();
    
    // Iterate over queue and clear all tasks (free memory for each task)
    iSCSITask * task = NULL;
    
    if(!onThread())
        OSDynamicCast(iSCSIVirtualHBA,owner)->GetCommandGate();
    
    while(!queue_empty(&taskQueue))
    {
        queue_remove_first(&taskQueue,task,iSCSITask *, queueChain);
        if(task)
            IOFree(task,sizeof(iSCSITask));
    }
}

