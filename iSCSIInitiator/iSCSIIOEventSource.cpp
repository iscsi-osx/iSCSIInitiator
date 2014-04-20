/**
 * @author		Nareg Sinenian
 * @file		iSCSIIOEventSource.cpp
 * @date		October 13, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 */

#include "iSCSIIOEventSource.h"

#define super IOEventSource

OSDefineMetaClassAndStructors(iSCSIIOEventSource,IOEventSource);

bool iSCSIIOEventSource::init(iSCSIVirtualHBA * owner,
							  IOEventSource::Action action,
							  iSCSISession * session,
                              iSCSIConnection * connection)
{
	// Initialize superclass, check validity and store socket handle
	if(super::init(owner,action))
	{
		iSCSIIOEventSource::session = session;
        iSCSIIOEventSource::connection = connection;
		return true;
	}
	return false;
}

void iSCSIIOEventSource::socketCallback(socket_t so,
										iSCSIIOEventSource * eventSource,
										int waitf)
{
	// Wake up the workloop thread that this event source is attached to.
	// The workloop thread will call checkForWork(), which will then dispatch
	// the action method to process data on the correct socket
	if(eventSource)
		eventSource->signalWorkAvailable();
    
    
}

bool iSCSIIOEventSource::checkForWork()
{
	// Validate action & owner, then call action on our owner & pass in socket
	if(action && owner)
        (*action)(owner,session,connection);
	
	return true;
}