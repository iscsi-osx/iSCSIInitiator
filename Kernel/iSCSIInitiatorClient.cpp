/*!
 * @author		Nareg Sinenian
 * @file		iSCSIInitiatorClient.cpp
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#include "iSCSIVirtualHBA.h"
#include "iSCSIInitiatorClient.h"
#include "iSCSITypesShared.h"
#include "iSCSITypesKernel.h"
#include <IOKit/IOLib.h>

/*! Required IOKit macro that defines the constructors, destructors, etc. */
OSDefineMetaClassAndStructors(iSCSIInitiatorClient,IOUserClient);

/*! The superclass is defined as a macro to follow IOKit conventions. */
#define super IOUserClient

/*! Array of methods that can be called by user-space. */
const IOExternalMethodDispatch iSCSIInitiatorClient::methods[kiSCSIInitiatorNumMethods] = {
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::OpenInitiator,
		0,                                  // Scalar input count
		0,                                  // Structure input size
		0,                                  // Scalar output count
		0                                   // Structure output size
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::CloseInitiator,
		0,
		0,
        0,
		0
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::CreateSession,
		0,
		kIOUCVariableStructureSize,         // Address structures
		3,                                  // Returned identifiers, error code
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::ReleaseSession,
		1,                                  // Session ID
		0,
		0,
        0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::SetSessionOptions,
		1,                                  // Session ID
        sizeof(iSCSIKernelSessionCfg),        // Options to set
		0,
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::GetSessionOptions,
		1,                                  // Session ID
		0,
		0,
		sizeof(iSCSIKernelSessionCfg)         // Options to get
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::CreateConnection,
		1,                                  // Session ID
		kIOUCVariableStructureSize,         // Address structures
		2,                                  // Returned connection identifier, error code
		0
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::ReleaseConnection,
		2,                                  // Session ID, connection ID
		0,
		0,
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::ActivateConnection,
		2,                                  // Session ID, connection ID
		0,
		0,
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::ActivateAllConnections,
		1,                                  // Session ID
		0,
		0,                                  // Return value
		0
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::DeactivateConnection,
		2,                                  // Session ID, connection ID
		0,
		0,                                  // Return value
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::DeactivateAllConnections,
		1,                                  // Session ID
		0,
		0,                                  // Return value
		0
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::SendBHS,
		0,                                  // Session ID, connection ID
		sizeof(struct __iSCSIPDUCommonBHS), // Buffer to send
		0,                                  // Return value
		0
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::SendData,
        2,                                  // Session ID, connection ID
		kIOUCVariableStructureSize,         // Data is a variable size block
		0,
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::RecvBHS,
        2,                                  // Session ID, connection ID
		0,
		0,
		sizeof(struct __iSCSIPDUCommonBHS), // Receive buffer
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::RecvData,
        2,                                  // Session ID, connection ID
		0,
		0,
		kIOUCVariableStructureSize,         // Receive buffer
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::SetConnectionOptions,
		2,                                  // Session ID, connection ID
        sizeof(iSCSIKernelConnectionCfg),     // Options to set
		0,
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::GetConnectionOptions,
		2,                                  // Session ID, connection ID
		0,
		0,
		sizeof(iSCSIKernelConnectionCfg)      // Options to get
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::GetConnection,
		1,                                  // Session ID
		0,
		1,                                  // Returned connection identifier
        0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::GetNumConnections,
		1,                                  // Session ID
		0,
		1,                                  // Returned number of connections
        0
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetSessionIdForTargetIQN,
        0,
        kIOUCVariableStructureSize,         // Target name
        1,                                  // Returned session identifier
        0
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetConnectionIdForAddress,
        1,                                  // Session ID
        kIOUCVariableStructureSize,         // Connection address structure
        1,                                  // Returned connection identifier
        0
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetSessionIds,
        0,
        0,
        1,                                  // Returned session count
        kIOUCVariableStructureSize    // List of session identifiers
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetConnectionIds,
        1,                                  // Session ID
        0,
        1,                                  // Returned connection count
        kIOUCVariableStructureSize // List of connection ids
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetTargetIQNForSessionId,
        1,                                  // Session ID
        0,
        0,                                  // Returned connection count
        kIOUCVariableStructureSize // Target name
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetAddressForConnectionId,
        2,                                  // Session ID, connection ID
        0,
        0,                                  // Returned connection count
        kIOUCVariableStructureSize // connection address structures
    }
};

IOReturn iSCSIInitiatorClient::externalMethod(uint32_t selector,
											  IOExternalMethodArguments * args,
											  IOExternalMethodDispatch * dispatch,
											  OSObject * target,
											  void * ref)
{
	// Sanity check the selector
	if(selector >= kiSCSIInitiatorNumMethods)
		return kIOReturnUnsupported;
	
	
	// Call the appropriate function for the current instance of the class
	return super::externalMethod(selector,
								 args,
								 (IOExternalMethodDispatch *)&iSCSIInitiatorClient::methods[selector],
								 this,
								 ref);
}


// Called as a result of user-space call to IOServiceOpen()
bool iSCSIInitiatorClient::initWithTask(task_t owningTask,
										void * securityToken,
										UInt32 type,
										OSDictionary * properties)
{
	// Save owning task, securty token and type so that we can validate user
	// as a root user (UID 0) for secure operations (e.g., adding an iSCSI
	// target requires privileges).
	this->owningTask = owningTask;
	this->securityToken = securityToken;
	this->type = type;
        
	// Perform any initialization tasks here
	return super::initWithTask(owningTask,securityToken,type,properties);
}

//Called after initWithTask as a result of call to IOServiceOpen()
bool iSCSIInitiatorClient::start(IOService * provider)
{
	// Check to ensure that the provider is actually an iSCSI initiator
	if((this->provider = OSDynamicCast(iSCSIVirtualHBA,provider)) == NULL)
		return false;

	return super::start(provider);
}

void iSCSIInitiatorClient::stop(IOService * provider)
{
	super::stop(provider);
}

// Called as a result of user-space call to IOServiceClose()
IOReturn iSCSIInitiatorClient::clientClose()
{
    // Tell HBA to release any resources that aren't active (e.g.,
    // connections we started to establish but didn't activate)
    
	// Ensure that the connection has been closed (in case the user calls
	// IOServiceClose() before calling our close() method
	close();
    
	
	// Terminate ourselves
	terminate();
	
	return kIOReturnSuccess;
}

// Called if the user-space client is terminated without calling
// IOServiceClose() or close()
IOReturn iSCSIInitiatorClient::clientDied()
{
    // Tell HBA to release any resources that aren't active (e.g.,
    // connections we started to establish but didn't activate)
    
    
    // Close the provider (decrease retain count)
    close();
    
	return super::clientDied();
}

// Invoked from user space remotely by calling iSCSIInitiatorOpen()
IOReturn iSCSIInitiatorClient::open()
{
	// Ensure that we are attached to a provider
	if(isInactive() || provider == NULL)
		return kIOReturnNotAttached;
	
	// Open the provider (iSCSIInitiator) for this this client
	if(provider->open(this))
		return kIOReturnSuccess;
    
	// At this point we couldn't open the client for the provider for some
	// other reason
	return kIOReturnNotOpen;
}

// Invoked from user space remotely by calling iSCSIInitiatorClose()
IOReturn iSCSIInitiatorClient::close()
{
	// If we're not active or have no provider we're not attached
	if(isInactive() || provider == NULL)
		return kIOReturnNotAttached;
	
	// If the provider isn't open for us then return not open
	else if(!provider->isOpen(this))
		return kIOReturnNotOpen;

	// At this point we're attached & open, close the connection
	provider->close(this);
	
	return kIOReturnSuccess;
}

/*! Dispatched function called from the device interface to this user
 *	client .*/
IOReturn iSCSIInitiatorClient::OpenInitiator(iSCSIInitiatorClient * target,
                                             void * reference,
                                             IOExternalMethodArguments * args)
{
    return target->open();
}

/*! Dispatched function called from the device interface to this user
 *	client .*/
IOReturn iSCSIInitiatorClient::CloseInitiator(iSCSIInitiatorClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args)
{
    return target->close();
}

/*! Dispatched function invoked from user-space to create new session. */
IOReturn iSCSIInitiatorClient::CreateSession(iSCSIInitiatorClient * target,
                                             void * reference,
                                             IOExternalMethodArguments * args)
{
    // Create a new session and return session ID
    SID sessionId;
    CID connectionId;
    
    // The target string is packed at the end of the input structure
    const char * targetCString = (const char *)((const sockaddr_storage *)args->structureInput+2);
    
    OSString * targetIQN = OSString::withCString(targetCString);
    
    // Create a connection
    errno_t error = target->provider->CreateSession(
        targetIQN,
        (const sockaddr_storage *)args->structureInput,     // Target
        (const sockaddr_storage *)args->structureInput+1,   // Host interface
        &sessionId,
        &connectionId);
    
    args->scalarOutput[0] = sessionId;
    args->scalarOutput[1] = connectionId;
    args->scalarOutput[2] = error;
    args->scalarOutputCount = 3;
    
    targetIQN->release();
    
    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to release session. */
IOReturn iSCSIInitiatorClient::ReleaseSession(iSCSIInitiatorClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args)
{
    // Release the session with the specified ID
    target->provider->ReleaseSession(*args->scalarInput);
    return kIOReturnSuccess;
}

// TODO: Set session options only once, when session is still inactive...
IOReturn iSCSIInitiatorClient::SetSessionOptions(iSCSIInitiatorClient * target,
                                                 void * reference,
                                                 IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIKernelSessionCfg * options = (iSCSIKernelSessionCfg*)args->structureInput;
    session->opts = *options;
    
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetSessionOptions(iSCSIInitiatorClient * target,
                                                 void * reference,
                                                 IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    
    // Range-check input
    if(sessionId == kiSCSIInvalidSessionId)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIKernelSessionCfg * options = (iSCSIKernelSessionCfg*)args->structureOutput;
    *options = session->opts;
    
    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to create new connection. */
IOReturn iSCSIInitiatorClient::CreateConnection(iSCSIInitiatorClient * target,
                                                void * reference,
                                                IOExternalMethodArguments * args)
{
    CID connectionId;
    
    // Create a connection
    errno_t error = target->provider->CreateConnection(
            (SID)args->scalarInput[0],       // Session qualifier
            (const sockaddr_storage *)args->structureInput,
            (const sockaddr_storage *)args->structureInput+1,
            &connectionId);
    
    args->scalarOutput[0] = connectionId;
    args->scalarOutput[1] = error;
    args->scalarOutputCount = 2;

    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to release connection. */
IOReturn iSCSIInitiatorClient::ReleaseConnection(iSCSIInitiatorClient * target,
                                                 void * reference,
                                                 IOExternalMethodArguments * args)
{
    target->provider->ReleaseConnection((SID)args->scalarInput[0],
                                        (CID)args->scalarInput[1]);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::ActivateConnection(iSCSIInitiatorClient * target,
                                                  void * reference,
                                                  IOExternalMethodArguments * args)
{
    *args->scalarOutput =
        target->provider->ActivateConnection((SID)args->scalarInput[0],
                                             (CID)args->scalarInput[1]);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::ActivateAllConnections(iSCSIInitiatorClient * target,
                                                      void * reference,
                                                      IOExternalMethodArguments * args)
{
    *args->scalarOutput =
        target->provider->ActivateAllConnections((SID)args->scalarInput[0]);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::DeactivateConnection(iSCSIInitiatorClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args)
{
    *args->scalarOutput =
        target->provider->DeactivateConnection((SID)args->scalarInput[0],
                                               (CID)args->scalarInput[1]);

    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::DeactivateAllConnections(iSCSIInitiatorClient * target,
                                                        void * reference,
                                                        IOExternalMethodArguments * args)
{
    *args->scalarOutput =
        target->provider->DeactivateAllConnections((SID)args->scalarInput[0]);
    
    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to send data
 *  over an existing, active connection. */
IOReturn iSCSIInitiatorClient::SendBHS(iSCSIInitiatorClient * target,
                                       void * reference,
                                       IOExternalMethodArguments * args)
{
    // Validate input
    if(args->structureInputSize != kiSCSIPDUBasicHeaderSegmentSize)
        return kIOReturnNoSpace;
    
    memcpy(&target->bhsBuffer,args->structureInput,kiSCSIPDUBasicHeaderSegmentSize);
    
    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to send data
 *  over an existing, active connection. */
IOReturn iSCSIInitiatorClient::SendData(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return kIOReturnNotFound;
    
    const void * data = args->structureInput;
    size_t length = args->structureInputSize;
    
    // Send data and return the result
    if(hba->SendPDU(session,connection,&(target->bhsBuffer),nullptr,data,length))
        return kIOReturnError;
    
    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to receive data
 *  over an existing, active connection, and to retrieve the size of
 *  a user-space buffer that is required to hold the data. */
IOReturn iSCSIInitiatorClient::RecvBHS(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args)
{
    // Verify user-supplied buffer is large enough to hold BHS
    if(args->structureOutputSize != kiSCSIPDUBasicHeaderSegmentSize)
        return kIOReturnNoSpace;
    
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return kIOReturnNotFound;

    // Receive data and return the result
    iSCSIPDUTargetBHS * bhs = (iSCSIPDUTargetBHS*)args->structureOutput;

    if(hba->RecvPDUHeader(session,connection,bhs,MSG_WAITALL))
        return kIOReturnIOError;


    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to receive data
 *  over an existing, active connection, and to retrieve the size of
 *  a user-space buffer that is required to hold the data. */
IOReturn iSCSIInitiatorClient::RecvData(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return kIOReturnNotFound;
    
    // Receive data and return the result
    void * data = (void *)args->structureOutput;
    size_t length = args->structureOutputSize;

    if(hba->RecvPDUData(session,connection,data,length,MSG_WAITALL))
        return kIOReturnIOError;
                           
    return kIOReturnSuccess;
}

// TODO: Only allow user to set options when connection is inactive
// TODO: optimize socket parameters based on connection options
IOReturn iSCSIInitiatorClient::SetConnectionOptions(iSCSIInitiatorClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return kIOReturnNotFound;
    
    iSCSIKernelConnectionCfg * options = (iSCSIKernelConnectionCfg*)args->structureInput;
    connection->opts = *options;
    
    // Set the maximum amount of immediate data we can send on this connection
    connection->immediateDataLength = min(options->maxSendDataSegmentLength,
                                          session->opts.firstBurstLength);
    
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetConnectionOptions(iSCSIInitiatorClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args)
{
    // Validate buffer is large enough to hold options
    if(args->structureOutputSize < sizeof(iSCSIKernelConnectionCfg))
        return kIOReturnMessageTooLarge;
    
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return kIOReturnNotFound;

    iSCSIKernelConnectionCfg * options = (iSCSIKernelConnectionCfg*)args->structureOutput;
    *options = connection->opts;
    
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetConnection(iSCSIInitiatorClient * target,
                                             void * reference,
                                             IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    args->scalarOutputCount = 1;
    CID * connectionId = (CID *)args->scalarOutput;
    
    for(CID connectionIdx = 0; connectionIdx < kiSCSIMaxConnectionsPerSession; connectionIdx++)
    {
        if(session->connections[connectionIdx])
        {
            *connectionId = connectionIdx;
            return kIOReturnSuccess;
        }
    }
    *connectionId = kiSCSIInvalidConnectionId;
    
    return kIOReturnNotFound;
}

IOReturn iSCSIInitiatorClient::GetNumConnections(iSCSIInitiatorClient * target,
                                                 void * reference,
                                                 IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    // Iterate over list of connections to see how many are valid
    CID connectionCount = 0;
    for(CID connectionId = 0; connectionId < kiSCSIMaxConnectionsPerSession; connectionId++)
        if(session->connections[connectionId])
            connectionCount++;
    
    *args->scalarOutput = connectionCount;
    args->scalarOutputCount = 1;

    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetSessionIdForTargetIQN(iSCSIInitiatorClient * target,
                                                         void * reference,
                                                         IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    const char * targetIQN = (const char *)args->structureInput;
    
    OSNumber * identifier = (OSNumber*)(hba->targetList->getObject(targetIQN));
    
    if(!identifier)
        return kIOReturnNotFound;
    
    *args->scalarOutput = identifier->unsigned16BitValue();;
    args->scalarOutputCount = 1;

    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetConnectionIdForAddress(iSCSIInitiatorClient * target,
                                                          void * reference,
                                                          IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];

    // Range-check input
    if(sessionId == kiSCSIInvalidSessionId)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    sockaddr_storage * address = (sockaddr_storage*)args->structureInput;
    
    if(!address)
        return kIOReturnBadArgument;
    
    iSCSIConnection * connection = NULL;
    
    // Iterate over connections to find a matching address structure
    for(CID connectionId = 0; connectionId < kiSCSIMaxConnectionsPerSession; connectionId++)
    {
        if(!(connection = session->connections[connectionId]))
            continue;
        
        sockaddr_storage peername;
        socklen_t peernamelen = sizeof(peername); 
        sock_getpeername(connection->socket,(sockaddr*)&peername,peernamelen);
        
        // Ensure family matches (IPv4/IPv6)
        if(peername.ss_family != address->ss_family)
            continue;
        
        if(peername.ss_family == AF_INET)
        {
            struct sockaddr_in * peer = (struct sockaddr_in *)&peername;
            struct sockaddr_in * addr = (struct sockaddr_in *)address;
            
            if(peer->sin_addr.s_addr != addr->sin_addr.s_addr)
                continue;
            
            if(peer->sin_port != addr->sin_port)
                continue;
        }
        else if(peername.ss_family == AF_INET6)
        {
            struct sockaddr_in6 * peer = (struct sockaddr_in6 *)&peername;
            struct sockaddr_in6 * addr = (struct sockaddr_in6 *)address;
            
            if(memcmp(&peer->sin6_addr,&addr->sin6_addr,sizeof(addr->sin6_addr)) != 0)
                continue;
            
            if(peer->sin6_port != addr->sin6_port)
                continue;
        }
        else
            continue;
        
        *args->scalarOutput = connectionId;
        args->scalarOutputCount = 1;
            
        return kIOReturnSuccess;
    }
    
    *args->scalarOutput = kiSCSIInvalidConnectionId;
    args->scalarOutputCount = 1;
    
    return kIOReturnNotFound;
}

IOReturn iSCSIInitiatorClient::GetSessionIds(iSCSIInitiatorClient * target,
                                             void * reference,
                                             IOExternalMethodArguments * args)
{
    if(args->structureOutputSize < sizeof(SID)*kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionCount = 0;
    SID * sessionIds = (SID *)args->structureOutput;
    
    for(SID sessionIdx = 0; sessionIdx < kiSCSIMaxSessions; sessionIdx++)
    {
        if(hba->sessionList[sessionIdx])
        {
            sessionIds[sessionCount] = sessionIdx;
            sessionCount++;
        }
    }

    args->scalarOutputCount = 1;
    *args->scalarOutput = sessionCount;

    return  kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetConnectionIds(iSCSIInitiatorClient * target,
                                                void * reference,
                                                IOExternalMethodArguments * args)
{
    if(args->structureOutputSize < sizeof(CID)*kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;

    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;

    CID connectionCount = 0;
    CID * connectionIds = (CID *)args->structureOutput;
    
    // Find an empty connection slot to use for a new connection
    for(CID index = 0; index < kiSCSIMaxConnectionsPerSession; index++)
    {
        if(session->connections[index])
        {
            connectionIds[connectionCount] = index;
            connectionCount++;
        }
    }
    
    args->scalarOutputCount = 1;
    *args->scalarOutput = connectionCount;

    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::GetTargetIQNForSessionId(iSCSIInitiatorClient * target,
                                                         void * reference,
                                                         IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    // Iterate over list of target name and find a matching session identifier
    OSCollectionIterator * iterator = OSCollectionIterator::withCollection(hba->targetList);
    
    if(!iterator)
        return kIOReturnNotFound;
    
    OSObject * object;
    
    while((object = iterator->getNextObject()))
    {
        OSString * targetIQN = OSDynamicCast(OSString,object);
        OSNumber * sessionIdNumber = OSDynamicCast(OSNumber,hba->targetList->getObject(targetIQN));

        if(sessionIdNumber->unsigned16BitValue() == sessionId)
        {
            // Minimum length (either buffer size or size of
            // target name, whichever is shorter)
            size_t size = min(targetIQN->getLength(),args->structureOutputSize);
            memcpy(args->structureOutput,targetIQN->getCStringNoCopy(),size);

            return kIOReturnSuccess;
        }
    }
    return kIOReturnNotFound;
}

IOReturn iSCSIInitiatorClient::GetAddressForConnectionId(iSCSIInitiatorClient * target,
                                                         void * reference,
                                                         IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    if(!session)
        return kIOReturnNotFound;
    
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return kIOReturnNotFound;
    
    socklen_t sockaddrSize = sizeof(sockaddr_storage);
    
    sockaddr_storage * targetAddress = (sockaddr_storage *)args->structureOutput;
    sockaddr_storage * hostAddress = (sockaddr_storage *)( ((UInt8 *)args->structureOutput) + sockaddrSize);
    
    sock_getpeername(connection->socket,(sockaddr*)targetAddress,sockaddrSize);
    sock_getsockname(connection->socket,(sockaddr*)hostAddress,sockaddrSize);

    return kIOReturnSuccess;
}



