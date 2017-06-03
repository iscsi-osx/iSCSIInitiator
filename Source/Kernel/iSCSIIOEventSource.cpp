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
