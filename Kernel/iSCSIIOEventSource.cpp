/*!
 * @author		Nareg Sinenian
 * @file		iSCSIIOEventSource.cpp
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#include <sys/ioctl.h>

#include "iSCSIIOEventSource.h"
#include "iSCSIVirtualHBA.h"

#define super IOEventSource

struct iSCSITask {
    queue_chain_t queueChain;
    UInt32 initiatorTaskTag;
};

OSDefineMetaClassAndStructors(iSCSIIOEventSource,IOEventSource);

bool iSCSIIOEventSource::init(iSCSIVirtualHBA * owner,
                              iSCSIIOEventSource::Action action,
                              iSCSISession * session,
                              iSCSIConnection * connection)
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
    if(eventSource && eventSource->getWorkLoop())
        eventSource->signalWorkAvailable();
}

bool iSCSIIOEventSource::checkForWork()
{
    if(!isEnabled())
        return false;
    
    // First check to ensure that the reason we've been called is because
    // actual data is available at the port (as opposed to other socket events)
    iSCSIVirtualHBA * hba = (iSCSIVirtualHBA*)owner;

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