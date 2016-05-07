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
		1,                                  // Number of parameters in struct
		kIOUCVariableStructureSize,         // Packed parameters for session
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
		(IOExternalMethodAction) &iSCSIInitiatorClient::SetSessionOption,
		3,                                  // Session ID, option ID, option value
        0,
		0,
		0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::GetSessionOption,
		2,                                  // Session ID, option ID
		0,
		1,                                  // Option to get
        0
	},
	{
		(IOExternalMethodAction) &iSCSIInitiatorClient::CreateConnection,
		2,                                  // Session ID, number of params
		kIOUCVariableStructureSize,         // Packed parameters for connection
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
		(IOExternalMethodAction) &iSCSIInitiatorClient::SetConnectionOption,
        4,                                  // Session ID, connection ID, option ID, option value
        0,
        0,
        0
	},
    {
		(IOExternalMethodAction) &iSCSIInitiatorClient::GetConnectionOption,
        3,                                  // Session ID, connection ID, option ID
        0,
        1,                                  // Option to get
        0
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
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetConnectionIdForPortalAddress,
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
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetPortalAddressForConnectionId,
        2,                                  // Session ID, connection ID
        0,
        0,
        kIOUCVariableStructureSize // Portal address (C string)
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetPortalPortForConnectionId,
        2,                                  // Session ID, connection ID
        0,
        0,                                  // Returned connection count
        kIOUCVariableStructureSize // connection address structures
    },
    {
        (IOExternalMethodAction) &iSCSIInitiatorClient::GetHostInterfaceForConnectionId,
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
    this->accessLock = IOLockAlloc();
        
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
    
    if(accessLock) {
        IOLockFree(accessLock);
        accessLock = NULL;
    }
	
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

/*! Invoked when a user-space application registers a notification port
 *  with this user client.
 *  @param port the port associated with the client connection.
 *  @param type the type.
 *  @param refCon a user reference value.
 *  @return an error code indicating the result of the operation. */
IOReturn iSCSIInitiatorClient::registerNotificationPort(mach_port_t port,
                                                        UInt32 type,
                                                        io_user_reference_t refCon)
{
    notificationPort = port;
    return kIOReturnSuccess;
}

/*! Send a notification message to the user-space application.
 *  @param message details regarding the notification message.
 *  @return an error code indicating the result of the operation. */
IOReturn iSCSIInitiatorClient::sendNotification(iSCSIKernelNotificationMessage * message)
{
    if(notificationPort == MACH_PORT_NULL)
        return kIOReturnNotOpen;
    
    message->header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,0);
    message->header.msgh_size = sizeof(message);
    message->header.msgh_remote_port = notificationPort;
    message->header.msgh_local_port = MACH_PORT_NULL;
    message->header.msgh_reserved = 0;
    message->header.msgh_id = 0;
    
    mach_msg_send_from_kernel_proper(&message->header,sizeof(message));
    return kIOReturnSuccess;
}

/*! Sends a notification message to the user indicating that an
 *  iSCSI asynchronous event has occured.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @param event the asynchronsou event.
 *  @return an error code indicating the result of the operation. */
IOReturn iSCSIInitiatorClient::sendAsyncMessageNotification(SID sessionId,
                                                            CID connectionId,
                                                            enum iSCSIPDUAsyncMsgEvent event)
{
    iSCSIKernelNotificationAsyncMessage message;
    message.notificationType = kiSCSIKernelNotificationAsyncMessage;
    message.asyncEvent = event;
    message.sessionId = sessionId;
    message.connectionId = connectionId;
    
    return sendNotification((iSCSIKernelNotificationMessage*)&message);
}

/*! Sends a notification message to the user indicating that the kernel
 *  extension will be terminating.
 *  @return an error code indicating the result of the operation. */
IOReturn iSCSIInitiatorClient::sendTerminateMessageNotification()
{
    iSCSIKernelNotificationMessage message;
    message.notificationType = kISCSIKernelNotificationTerminate;
    
    return sendNotification(&message);
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
    
    // Unpack the struct to get targetIQN, portalAddress, etc.
    UInt64 kNumParams = *args->scalarInput;
    
    // We expect six input arguments to CreateSession...
    if(kNumParams < 6)
        return kIOReturnBadArgument;
    
    void * params[kNumParams];
    size_t paramSize[kNumParams];
    size_t header = sizeof(UInt64*)*kNumParams;
    UInt8 * inputPos = ((UInt8*)args->structureInput)+header;
    
    for(int idx = 0; idx < kNumParams; idx++) {
        paramSize[idx] = ((UInt64*)args->structureInput)[idx];
        params[idx] = inputPos;
        inputPos += paramSize[idx];
    }
    
    OSString * targetIQN = OSString::withCString((const char *)params[0]);
    OSString * portalAddress = OSString::withCString((const char *)params[1]);
    OSString * portalPort = OSString::withCString((const char *)params[2]);
    OSString * hostInterface = OSString::withCString((const char *)params[3]);
    const sockaddr_storage * portalSockAddr = (struct sockaddr_storage*)params[4];
    const sockaddr_storage * hostSockAddr = (struct sockaddr_storage*)params[5];
 
    IOLockLock(target->accessLock);
    
    // Create a connection
    errno_t error = target->provider->CreateSession(
        targetIQN,portalAddress,portalPort,hostInterface,portalSockAddr,
        hostSockAddr,&sessionId,&connectionId);
    
    IOLockUnlock(target->accessLock);
    
    args->scalarOutput[0] = sessionId;
    args->scalarOutput[1] = connectionId;
    args->scalarOutput[2] = error;
    args->scalarOutputCount = 3;
    
    targetIQN->release();
    portalAddress->release();
    portalPort->release();
    hostInterface->release();
    
    return kIOReturnSuccess;
}

/*! Dispatched function invoked from user-space to release session. */
IOReturn iSCSIInitiatorClient::ReleaseSession(iSCSIInitiatorClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args)
{
    // Release the session with the specified ID
    IOLockLock(target->accessLock);
    target->provider->ReleaseSession(*args->scalarInput);
    IOLockUnlock(target->accessLock);
    
    return kIOReturnSuccess;
}

// TODO: Set session options only once, when session is still inactive...
IOReturn iSCSIInitiatorClient::SetSessionOption(iSCSIInitiatorClient * target,
                                                void * reference,
                                                IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    if(args->scalarInputCount != 3)
        return kIOReturnBadArgument;
    
    SID sessionId = (SID)args->scalarInput[0];
    enum iSCSIKernelSessionOptTypes optType = (enum iSCSIKernelSessionOptTypes)args->scalarInput[1];
    UInt64 optVal = args->scalarInput[2];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
 
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    IOReturn retVal = kIOReturnSuccess;
    
    if(session)
    {
        switch(optType)
        {
            case kiSCSIKernelSODataPDUInOrder:
                session->dataPDUInOrder = optVal;
                break;
            case kiSCSIKernelSODataSequenceInOrder:
                session->dataSequenceInOrder = optVal;
                break;
            case kiSCSIKernelSODefaultTime2Retain:
                session->defaultTime2Retain = optVal;
                break;
            case kiSCSIKernelSODefaultTime2Wait:
                session->defaultTime2Wait = optVal;
                break;
            case kiSCSIKernelSOErrorRecoveryLevel:
                session->errorRecoveryLevel = optVal;
                break;
            case kiSCSIKernelSOFirstBurstLength:
                session->firstBurstLength = (UInt32)optVal;
                break;
            case kiSCSIKernelSOImmediateData:
                session->immediateData = optVal;
                break;
            case kiSCSIKernelSOMaxConnections:
                session->maxConnections = (CID)optVal;
                break;
            case kiSCSIKernelSOMaxOutstandingR2T:
                session->maxOutStandingR2T = optVal;
                break;
            case kiSCSIKernelSOMaxBurstLength:
                session->maxBurstLength = (UInt32)optVal;
                break;
            case kiSCSIKernelSOInitialR2T:
                session->initialR2T = optVal;
                break;
            case kiSCSIKernelSOTargetPortalGroupTag:
                session->targetPortalGroupTag = optVal;
                break;
            case kiSCSIKernelSOTargetSessionId:
                session->targetSessionId = optVal;
                break;

            default:
                retVal = kIOReturnBadArgument;
        };
    }
    else {
        retVal = kIOReturnBadArgument;
    }
    
    IOLockUnlock(target->accessLock);
    return retVal;
}

IOReturn iSCSIInitiatorClient::GetSessionOption(iSCSIInitiatorClient * target,
                                                void * reference,
                                                IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    
    if(args->scalarInputCount != 2)
        return kIOReturnBadArgument;
    
    SID sessionId = (SID)args->scalarInput[0];
    enum iSCSIKernelSessionOptTypes optType = (enum iSCSIKernelSessionOptTypes)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    
    IOReturn retVal = kIOReturnSuccess;
    
    UInt64 * optVal = args->scalarOutput;
    
    if(session)
    {
        switch(optType)
        {
            case kiSCSIKernelSODataPDUInOrder:
                *optVal = session->dataPDUInOrder;
                break;
            case kiSCSIKernelSODataSequenceInOrder:
                *optVal = session->dataSequenceInOrder;
                break;
            case kiSCSIKernelSODefaultTime2Retain:
                *optVal = session->defaultTime2Retain;
                break;
            case kiSCSIKernelSODefaultTime2Wait:
                *optVal = session->defaultTime2Wait;
                break;
            case kiSCSIKernelSOErrorRecoveryLevel:
                *optVal = session->errorRecoveryLevel;
                break;
            case kiSCSIKernelSOFirstBurstLength:
                *optVal = session->firstBurstLength;
                break;
            case kiSCSIKernelSOImmediateData:
                *optVal = session->immediateData;
                break;
            case kiSCSIKernelSOMaxConnections:
                *optVal = session->maxConnections;
                break;
            case kiSCSIKernelSOMaxOutstandingR2T:
                *optVal = session->maxOutStandingR2T;
                break;
            case kiSCSIKernelSOMaxBurstLength:
                *optVal = session->maxBurstLength;
                break;
            case kiSCSIKernelSOInitialR2T:
                *optVal = session->initialR2T;
                break;
            case kiSCSIKernelSOTargetPortalGroupTag:
                *optVal = session->targetPortalGroupTag;
                break;
            case kiSCSIKernelSOTargetSessionId:
                *optVal = session->targetSessionId;
                break;
            default:
                retVal = kIOReturnBadArgument;
        };
    }
    else {
        retVal = kIOReturnNotFound;
    }
    
    IOLockUnlock(target->accessLock);
    return retVal;
}

/*! Dispatched function invoked from user-space to create new connection. */
IOReturn iSCSIInitiatorClient::CreateConnection(iSCSIInitiatorClient * target,
                                                void * reference,
                                                IOExternalMethodArguments * args)
{
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId;
    
    // Unpack the struct to get targetIQN, portalAddress, etc.
    UInt64 kNumParams = args->scalarInput[1];
    
    // We expect six input arguments to CreateSession...
    if(kNumParams < 5)
        return kIOReturnBadArgument;
    
    void * params[kNumParams];
    size_t paramSize[kNumParams];
    size_t header = sizeof(UInt64*)*kNumParams;
    UInt8 * inputPos = ((UInt8*)args->structureInput)+header;
    
    for(int idx = 0; idx < kNumParams; idx++) {
        paramSize[idx] = ((UInt64*)args->structureInput)[idx];
        params[idx] = inputPos;
        inputPos += paramSize[idx];
    }
    
    OSString * portalAddress = OSString::withCString((const char *)params[0]);
    OSString * portalPort = OSString::withCString((const char *)params[1]);
    OSString * hostInterface = OSString::withCString((const char *)params[2]);
    const sockaddr_storage * portalSockAddr = (struct sockaddr_storage*)params[3];
    const sockaddr_storage * hostSockAddr = (struct sockaddr_storage*)params[4];
    
    IOLockLock(target->accessLock);
    
    // Create a connection
    errno_t error = target->provider->CreateConnection(
            sessionId,portalAddress,portalPort,hostInterface,portalSockAddr,
            hostSockAddr,&connectionId);
    
    IOLockUnlock(target->accessLock);
    
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
    IOLockLock(target->accessLock);

    target->provider->ReleaseConnection((SID)args->scalarInput[0],
                                        (CID)args->scalarInput[1]);
    IOLockUnlock(target->accessLock);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::ActivateConnection(iSCSIInitiatorClient * target,
                                                  void * reference,
                                                  IOExternalMethodArguments * args)
{
    IOLockLock(target->accessLock);

    *args->scalarOutput =
        target->provider->ActivateConnection((SID)args->scalarInput[0],
                                             (CID)args->scalarInput[1]);
    IOLockUnlock(target->accessLock);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::ActivateAllConnections(iSCSIInitiatorClient * target,
                                                      void * reference,
                                                      IOExternalMethodArguments * args)
{
    IOLockLock(target->accessLock);

    *args->scalarOutput =
        target->provider->ActivateAllConnections((SID)args->scalarInput[0]);
    
    IOLockUnlock(target->accessLock);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::DeactivateConnection(iSCSIInitiatorClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args)
{
    IOLockLock(target->accessLock);
    
    *args->scalarOutput =
        target->provider->DeactivateConnection((SID)args->scalarInput[0],
                                               (CID)args->scalarInput[1]);
    
    IOLockUnlock(target->accessLock);
    return kIOReturnSuccess;
}

IOReturn iSCSIInitiatorClient::DeactivateAllConnections(iSCSIInitiatorClient * target,
                                                        void * reference,
                                                        IOExternalMethodArguments * args)
{
    IOLockLock(target->accessLock);
    
    *args->scalarOutput =
        target->provider->DeactivateAllConnections((SID)args->scalarInput[0]);
    
    IOLockUnlock(target->accessLock);
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
    
    IOLockLock(target->accessLock);
    memcpy(&target->bhsBuffer,args->structureInput,kiSCSIPDUBasicHeaderSegmentSize);
    IOLockUnlock(target->accessLock);
    
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
    
    IOLockLock(target->accessLock);

    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    const void * data = args->structureInput;
    size_t length = args->structureInputSize;
    
    // Send data and return the result
    IOReturn retVal = kIOReturnNotFound;
    
    if(connection) {
        if(hba->SendPDU(session,connection,&(target->bhsBuffer),nullptr,data,length))
            retVal = kIOReturnError;
        else
            retVal = kIOReturnSuccess;
    }
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
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
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;
    
    iSCSIPDUTargetBHS * bhs = (iSCSIPDUTargetBHS*)args->structureOutput;
    
    if(connection) {
        if(hba->RecvPDUHeader(session,connection,bhs,MSG_WAITALL))
            retVal = kIOReturnIOError;
        else
            retVal = kIOReturnSuccess;
    }

    IOLockUnlock(target->accessLock);

    return retVal;
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
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;
    
    void * data = (void *)args->structureOutput;
    size_t length = args->structureOutputSize;

    if(hba->RecvPDUData(session,connection,data,length,MSG_WAITALL))
        retVal = kIOReturnIOError;
    else
        retVal = kIOReturnSuccess;
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
}

// TODO: Only allow user to set options when connection is inactive
// TODO: optimize socket parameters based on connection options
IOReturn iSCSIInitiatorClient::SetConnectionOption(iSCSIInitiatorClient * target,
                                                   void * reference,
                                                   IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    if(args->scalarInputCount != 4)
        return kIOReturnBadArgument;
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    enum iSCSIKernelConnectionOptTypes optType = (enum iSCSIKernelConnectionOptTypes)args->scalarInput[2];
    UInt64 optVal = args->scalarInput[3];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;

    if(connection)
    {
        retVal = kIOReturnSuccess;
        
        switch(optType)
        {
            case kiSCSIKernelCOIFMarkInt:
                connection->IFMarkInt = optVal;
                break;
            case kiSCSIKernelCOOFMarkInt:
                connection->OFMarkInt = optVal;
                break;
            case kiSCSIKernelCOUseIFMarker:
                connection->useIFMarker = optVal;
                break;
            case kiSCSIKernelCOUseOFMarker:
                connection->useOFMarker = optVal;
                break;
            case kiSCSIKernelCOUseDataDigest:
                connection->useDataDigest = optVal;
                break;
            case kiSCSIKernelCOUseHeaderDigest:
                connection->useHeaderDigest = optVal;
                break;
            case kiSCSIKernelCOMaxRecvDataSegmentLength:
                connection->maxRecvDataSegmentLength = (UInt32)optVal;
                break;
            case kiSCSIKernelCOMaxSendDataSegmentLength:
                connection->maxSendDataSegmentLength = (UInt32)optVal;
                break;
            case kiSCSIKernelCOInitialExpStatSN:
                connection->expStatSN = (UInt32)optVal;
                break;
                
            default:
                retVal = kIOReturnBadArgument;
        };
    }
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
}

IOReturn iSCSIInitiatorClient::GetConnectionOption(iSCSIInitiatorClient * target,
                                                   void * reference,
                                                   IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    if(args->scalarInputCount != 3)
        return kIOReturnBadArgument;
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    enum iSCSIKernelConnectionOptTypes optType = (enum iSCSIKernelConnectionOptTypes)args->scalarInput[2];
    UInt64 * optVal = args->scalarOutput;
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;

    if(connection) {
        retVal = kIOReturnSuccess;
    
        switch(optType)
        {
            case kiSCSIKernelCOIFMarkInt:
                *optVal = connection->IFMarkInt;
                break;
            case kiSCSIKernelCOOFMarkInt:
                *optVal = connection->OFMarkInt;
                break;
            case kiSCSIKernelCOUseIFMarker:
                *optVal = connection->useIFMarker;
                break;
            case kiSCSIKernelCOUseOFMarker:
                *optVal = connection->useOFMarker;
                break;
            case kiSCSIKernelCOUseDataDigest:
                *optVal = connection->useDataDigest;
                break;
            case kiSCSIKernelCOUseHeaderDigest:
                *optVal = connection->useHeaderDigest;
                break;
            case kiSCSIKernelCOMaxRecvDataSegmentLength:
                *optVal = connection->maxRecvDataSegmentLength;
                break;
            case kiSCSIKernelCOMaxSendDataSegmentLength:
                *optVal = connection->maxSendDataSegmentLength;
                break;
            case kiSCSIKernelCOInitialExpStatSN:
                *optVal = connection->expStatSN;
                break;
                
            default:
                return kIOReturnBadArgument;
        };
    }
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
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
    
    IOLockLock(target->accessLock);
    
    iSCSISession * session = hba->sessionList[sessionId];
    IOReturn retVal = kIOReturnNotFound;
    
    if(session) {
        
        retVal = kIOReturnSuccess;
        
        args->scalarOutputCount = 1;
        CID * connectionId = (CID *)args->scalarOutput;
        
        *connectionId = kiSCSIInvalidConnectionId;
        
        for(CID connectionIdx = 0; connectionIdx < kiSCSIMaxConnectionsPerSession; connectionIdx++)
        {
            if(session->connections[connectionIdx])
            {
                *connectionId = connectionIdx;
                break;
            }
        }
    }
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
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
    
    IOLockLock(target->accessLock);
    
    iSCSISession * session = hba->sessionList[sessionId];
    IOReturn retVal = kIOReturnNotFound;
    CID connectionCount = 0;
    
    if(session) {
        // Iterate over list of connections to see how many are valid
        for(CID connectionId = 0; connectionId < kiSCSIMaxConnectionsPerSession; connectionId++)
            if(session->connections[connectionId])
                connectionCount++;
    }
    
    *args->scalarOutput = connectionCount;
    args->scalarOutputCount = 1;
    
    IOLockUnlock(target->accessLock);

    return retVal;
}

IOReturn iSCSIInitiatorClient::GetSessionIdForTargetIQN(iSCSIInitiatorClient * target,
                                                         void * reference,
                                                         IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    const char * targetIQN = (const char *)args->structureInput;
    
    IOLockLock(target->accessLock);
    OSNumber * identifier = (OSNumber*)(hba->targetList->getObject(targetIQN));
    
    IOReturn retVal = kIOReturnNotFound;
    
    if(identifier) {
        retVal = kIOReturnSuccess;
        *args->scalarOutput = identifier->unsigned16BitValue();;
        args->scalarOutputCount = 1;
    }

    IOLockUnlock(target->accessLock);
    
    return retVal;
}

IOReturn iSCSIInitiatorClient::GetConnectionIdForPortalAddress(iSCSIInitiatorClient * target,
                                                          void * reference,
                                                          IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];

    // Range-check input
    if(sessionId == kiSCSIInvalidSessionId)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);

    iSCSISession * session = hba->sessionList[sessionId];
    IOReturn retVal = kIOReturnNotFound;
    
    if(session) {
    
        retVal = kIOReturnBadArgument;
        
        OSString * portalAddress = OSString::withCString((const char *)args->structureInput);
        
        if(portalAddress)
        {
            retVal = kIOReturnNotFound;
            
            iSCSIConnection * connection = NULL;
            
            *args->scalarOutput = kiSCSIInvalidConnectionId;
            args->scalarOutputCount = 1;
            
            // Iterate over connections to find a matching address structure
            for(CID connectionId = 0; connectionId < kiSCSIMaxConnectionsPerSession; connectionId++)
            {
                if(!(connection = session->connections[connectionId]))
                    continue;
                
                if(!connection->portalAddress->isEqualTo(portalAddress))
                    continue;
                
                *args->scalarOutput = connectionId;
                args->scalarOutputCount = 1;
                    
                retVal = kIOReturnSuccess;
                break;
            }
        }
    }
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
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
    
    IOLockLock(target->accessLock);
    
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
    
    IOLockUnlock(target->accessLock);

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
    
    IOLockLock(target->accessLock);

    iSCSISession * session = hba->sessionList[sessionId];
    IOReturn retVal = kIOReturnNotFound;
    
    if(session)
    {
        retVal = kIOReturnSuccess;

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
    }

    IOLockUnlock(target->accessLock);

    return retVal;
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
    
    IOLockLock(target->accessLock);
    
    iSCSISession * session = hba->sessionList[sessionId];
    IOReturn retVal = kIOReturnNotFound;
    
    // Iterate over list of target name and find a matching session identifier
    OSCollectionIterator * iterator = OSCollectionIterator::withCollection(hba->targetList);
    
    if(session && iterator)
    {
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

                retVal =  kIOReturnSuccess;
                break;
            }
        }
    }
    
    IOLockUnlock(target->accessLock);

    OSSafeRelease(iterator);
    return retVal;
}

IOReturn iSCSIInitiatorClient::GetPortalAddressForConnectionId(iSCSIInitiatorClient * target,
                                                         void * reference,
                                                         IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;
    
    if(connection) {
        retVal = kIOReturnSuccess;
        
        const char * portalAddress = connection->portalAddress->getCStringNoCopy();
        size_t portalAddressLength = connection->portalAddress->getLength();
        
        memset(args->structureOutput,0,args->structureOutputSize);
        memcpy(args->structureOutput,portalAddress,min(args->structureOutputSize,portalAddressLength));
    }
    
    IOLockUnlock(target->accessLock);
    return retVal;
}

IOReturn iSCSIInitiatorClient::GetPortalPortForConnectionId(iSCSIInitiatorClient * target,
                                                            void * reference,
                                                            IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;
    
    if(connection) {
        retVal = kIOReturnSuccess;
        
        const char * portalPort = connection->portalPort->getCStringNoCopy();
        size_t portalPortLength = connection->portalPort->getLength();
    
        memset(args->structureOutput,0,args->structureOutputSize);
        memcpy(args->structureOutput,portalPort,min(args->structureOutputSize,portalPortLength));
    }
    
    IOLockUnlock(target->accessLock);
    return retVal;
}

IOReturn iSCSIInitiatorClient::GetHostInterfaceForConnectionId(iSCSIInitiatorClient * target,
                                                               void * reference,
                                                               IOExternalMethodArguments * args)
{
    iSCSIVirtualHBA * hba = OSDynamicCast(iSCSIVirtualHBA,target->provider);
    
    SID sessionId = (SID)args->scalarInput[0];
    CID connectionId = (CID)args->scalarInput[1];
    
    // Range-check input
    if(sessionId >= kiSCSIMaxSessions || connectionId >= kiSCSIMaxConnectionsPerSession)
        return kIOReturnBadArgument;
    
    IOLockLock(target->accessLock);
    
    // Do nothing if session doesn't exist
    iSCSISession * session = hba->sessionList[sessionId];
    iSCSIConnection * connection = NULL;
    
    if(session)
        connection = session->connections[connectionId];
    
    // Receive data and return the result
    IOReturn retVal = kIOReturnNotFound;
    
    if(connection) {
        retVal = kIOReturnSuccess;
        
        const char * hostInterface = connection->hostInteface->getCStringNoCopy();
        size_t hostInterfaceLength = connection->hostInteface->getLength();
    
        memcpy(args->structureOutput,hostInterface,min(args->structureOutputSize,hostInterfaceLength+1));
    }
    
    IOLockUnlock(target->accessLock);
    
    return retVal;
}



