/**
 * @author		Nareg Sinenian
 * @file		iSCSIIOEventSource.cpp
 * @date		October 13, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 */

#include "iSCSIIOEventSource.h"
#include <sys/ioctl.h>
#include "iSCSIVirtualHBA.h"

#define super IOEventSource

struct iSCSITask {
    queue_chain_t queueChain;
    UInt32 initiatorTaskTag;
};

OSDefineMetaClassAndStructors(iSCSIIOEventSource,IOEventSource);

bool iSCSIIOEventSource::init(iSCSIVirtualHBA * owner,
                              iSCSIIOEventSource::Action action,
                              iSCSIVirtualHBA::iSCSISession * session,
                              iSCSIVirtualHBA::iSCSIConnection * connection)
{
	// Initialize superclass, check validity and store socket handle
	if(!super::init(owner,(IOEventSource::Action) action))
        return false;
	
    iSCSIIOEventSource::session = session;
    iSCSIIOEventSource::connection = connection;
    
    // Initialize task queue to store parallel SCSI tasks for processing
    queue_init(&taskQueue);
    taskQueueLock = IOSimpleLockAlloc();
    
	return true;
}

void iSCSIIOEventSource::socketCallback(socket_t so,
										iSCSIIOEventSource * eventSource,
										int waitf)
{
	// Wake up the workloop thread that this event source is attached to.
	// The workloop thread will call checkForWork(), which will then dispatch
	// the action method to process data on the correct socket.
    if(eventSource)
        eventSource->signalWorkAvailable();
}

void iSCSIIOEventSource::addTaskToQueue(UInt32 initiatorTaskTag)
{
    // Signal the workloop thread that work is available only if this is
    // the only task in the queue (otherwise the task preceding this is
    // being processed; we'll get to this once that's done).
    iSCSITask * task = (iSCSITask*)IOMalloc(sizeof(iSCSITask));
    task->initiatorTaskTag = initiatorTaskTag;
    
    IOSimpleLockLock(taskQueueLock);
    
    if(queue_empty(&taskQueue))
        newTask = true;
    
    queue_enter(&taskQueue,task,iSCSITask *,queueChain);
    
    IOSimpleLockUnlock(taskQueueLock);
    
    if(newTask) {
        IOLog("iSCSI: First task, processing now.\n");
        signalWorkAvailable();
    }
}

void iSCSIIOEventSource::removeTaskFromQueue()
{
    iSCSITask * task = NULL;
    
    // Remove the completed task (at the head of the queue) and then
    // move onto the next task if one exists
    IOSimpleLockLock(taskQueueLock);
    queue_remove_first(&taskQueue,task,iSCSITask *, queueChain);
    
    IOSimpleLockUnlock(taskQueueLock);

    if(task)
        IOFree(task,sizeof(iSCSITask));
    
    if(!queue_empty(&taskQueue)) {
        newTask = true;
        IOLog("iSCSI: Moving to new task.\n");
        signalWorkAvailable();
    }
}

bool iSCSIIOEventSource::checkForWork()
{
    // First check to ensure that the reason we've been called is because
    // actual data is available at the port (as opposed to other socket events)
    iSCSIVirtualHBA * hba = (iSCSIVirtualHBA*)owner;
    
    // Process a new task, reset flag
    if(newTask)
    {
        newTask = false;
        iSCSITask * task = NULL;
        
        IOSimpleLockLock(taskQueueLock);

        if(!queue_empty(&taskQueue))
            task = (iSCSITask*)queue_first(&taskQueue);
        
        IOSimpleLockUnlock(taskQueueLock);
        
        if(task)
            hba->BeginTaskOnWorkloopThread(session,connection,task->initiatorTaskTag);
    }

    if(hba->isPDUAvailable(connection))
    {
        // Validate action & owner, then call action on our owner & pass in socket
        if(action && owner)
            (*action)(owner,session,connection);
        
        // Tell workloop thread to call us again (gives it a chance to handle
        // other requests first)
        if(hba->isPDUAvailable(connection))
            return true;
    }
    
    // Tell workloop thread not to call us again until we signal again...
	return false;
}