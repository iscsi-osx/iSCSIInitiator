/*!
 * @author		Nareg Sinenian
 * @file		iSCSIKernelInterface.c
 * @date		March 14, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 */

#include "iSCSIKernelInterface.h"
#include "iSCSIPDUUser.h"
#include <IOKit/IOKitLib.h>

static io_service_t service;
static io_connect_t connection;

/*! Opens a connection to the iSCSI initiator.  A connection must be
 *  successfully opened before any of the supporting functions below can be
 *  called. */
kern_return_t iSCSIKernelInitialize()
{
    kern_return_t kernResult;
 	
	// Create a dictionary to match iSCSIkext
	CFMutableDictionaryRef matchingDict = NULL;
	matchingDict = IOServiceMatching("com_NSinenian_iSCSIVirtualHBA");
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault,matchingDict);
    
	// Check to see if the driver was found in the I/O registry
	if(service == IO_OBJECT_NULL)
	{
/*		NSAlert * alert = [[NSAlert alloc] init];
		
		
		[alert setMessageText:@"iSCSI driver has not been loaded"];
		[alert runModal];
*/
		return kIOReturnNotFound;
	}
    
	// Using the service handle, open a connection
	kernResult = IOServiceOpen(service,mach_task_self(),0,&connection);
	
	if(kernResult != kIOReturnSuccess) {
/*		NSAlert * alert = [[NSAlert alloc] init];
		
		
		[alert setMessageText:@"Couldnt open handle to service"];
		[alert runModal];
*/
	}
    return IOConnectCallScalarMethod(connection,kiSCSIOpenInitiator,0,0,0,0);
}

/*! Closes a connection to the iSCSI initiator. */
kern_return_t iSCSIKernelCleanUp()
{
    kern_return_t kernResult =
        IOConnectCallScalarMethod(connection,kiSCSICloseInitiator,0,0,0,0);
    
	// Clean up (now that we have a connection we no longer need the object)
    IOObjectRelease(service);
    IOServiceClose(connection);
    
    return kernResult;
}

/*! Allocates a new iSCSI session in the kernel and creates an associated
 *  connection to the target portal. Additional connections may be added to the
 *  session by calling iSCSIKernelCreateConnection().
 *  @param domain the IP domain (e.g., AF_INET or AF_INET6).
 *  @param targetAddress the BSD socket structure used to identify the target.
 *  @param hostAddress the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param sessionId the session identifier for the new session (returned).
 *  @param connectionId the identifier of the new connection (returned).
 *  @return An error code if a valid session could not be created. */
errno_t iSCSIKernelCreateSession(int domain,
                                 const struct sockaddr * targetAddress,
                                 const struct sockaddr * hostAddress,
                                 UInt16 * sessionId,
                                 UInt32 * connectionId)
{
    // Check parameters
    if(!targetAddress || !hostAddress || !sessionId || !connectionId)
        return EINVAL;
    
    // Tell the kernel to drop this session and all of its related resources
    const UInt32 inputCnt = 1;
    const UInt64 input = domain;
    
    const UInt32 inputStructCnt = 2;
    const struct sockaddr addresses[] = {*targetAddress,*hostAddress};
    
    const UInt32 expOutputCnt = 3;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    if(IOConnectCallMethod(connection,kiSCSICreateSession,&input,inputCnt,
                           addresses,inputStructCnt*sizeof(struct sockaddr),
                           output,&outputCnt,0,0) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
        {
            *sessionId    = (UInt16)output[1];
            *connectionId = (UInt32)output[2];
            return (UInt32)output[0];
        }
    }
    
    // Else we couldn't allocate a connection; quit
    return EINVAL;
}

/*! Releases an iSCSI session, including all connections associated with that
 *  session.
 *  @param sessionId the session qualifier part of the ISID. */
void iSCSIKernelReleaseSession(UInt16 sessionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId)
        return;

    // Tell the kernel to drop this session and all of its related resources
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    IOConnectCallScalarMethod(connection,kiSCSIReleaseSession,&input,inputCnt,0,0);
}

/*! Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetSessionOptions(UInt16 sessionId,
                                     iSCSISessionOptions * options)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !options)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    const UInt64 input = sessionId;
    
    if(IOConnectCallMethod(connection,kiSCSISetSessionOptions,&input,inputCnt,
                           options,sizeof(struct iSCSISessionOptions),0,0,0,0) == kIOReturnSuccess)
    {
        return 0;
    }
    return EIO;
}

/*! Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionOptions(UInt16 sessionId,
                                     iSCSISessionOptions * options)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !options)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    const UInt64 input = sessionId;
    size_t optionsSize = sizeof(struct iSCSISessionOptions);

    if(IOConnectCallMethod(connection,kiSCSIGetSessionOptions,&input,inputCnt,0,0,0,0,
                           options,&optionsSize) == kIOReturnSuccess)
    {
        return 0;
    }
    return EIO;
}

/*! Allocates an additional iSCSI connection for a particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param domain the IP domain (e.g., AF_INET or AF_INET6).
 *  @param targetAddress the BSD socket structure used to identify the target.
 *  @param hostAddress the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param connectionId the identifier of the new connection (returned).
 *  @return An error code if a valid connection could not be created. */
errno_t iSCSIKernelCreateConnection(UInt16 sessionId,
                                    int domain,
                                    const struct sockaddr * targetAddress,
                                    const struct sockaddr * hostAddress,
                                    UInt32 * connectionId)
{
    // Check parameters
    if(!targetAddress || !hostAddress || sessionId == kiSCSIInvalidSessionId)
        return kiSCSIInvalidConnectionId;
    
    // Tell the kernel to drop this session and all of its related resources
    const UInt32 inputCnt = 2;
    const UInt64 inputs[] = {sessionId,domain};
    
    const UInt32 inputStructCnt = 2;
    const struct sockaddr addresses[] = {*targetAddress,*hostAddress};
    
    const UInt32 expOutputCnt = 2;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    if(IOConnectCallMethod(connection,kiSCSICreateConnection,inputs,inputCnt,
                           addresses,inputStructCnt*sizeof(struct sockaddr),
                           output,&outputCnt,0,0) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
        {
            *connectionId = (UInt32)output[1];
            return (UInt32)output[0];
        }
    }

    // Else we couldn't allocate a connection; quit
    return EINVAL;
}

/*! Frees a given
 iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
void iSCSIKernelReleaseConnection(UInt16 sessionId,UInt32 connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return;

    // Tell kernel to drop this connection
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    IOConnectCallScalarMethod(connection,kiSCSIReleaseConnection,inputs,inputCnt,0,0);
}


/*! Sends data over a kernel socket associated with iSCSI.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment to send over the connection.
 *  @param data the data segment of the PDU to send over the connection.
 *  @param length the length of the data block to send over the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSend(UInt16 sessionId,
                        UInt32 connectionId,
                        iSCSIPDUInitiatorBHS * bhs,
                        void * data,
                        size_t length)
{
    // Check parameters
    if(sessionId    == kiSCSIInvalidSessionId || !bhs || (!data && length > 0) ||
       connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Setup input scalar array
    const UInt32 inputCnt = 2;
    const UInt64 inputs[] = {sessionId, connectionId};
    
    const UInt32 expOutputCnt = 1;
    UInt32 outputCnt = 1;
    UInt64 output;
    
    // Call kernel method to send (buffer) bhs and then data
    if(IOConnectCallStructMethod(connection,kiSCSISendBHS,bhs,
            sizeof(iSCSIPDUInitiatorBHS),NULL,NULL) != kIOReturnSuccess)
    {
        return EINVAL;
    }
    
    if(IOConnectCallMethod(connection,kiSCSISendData,inputs,inputCnt,
            data,length,&output,&outputCnt,NULL,NULL) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    
    // Return -1 as the BSD socket API normally would if the kernel call fails
    return EINVAL;
}

/*! Receives data over a kernel socket associated with iSCSI.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received over the connection.
 *  @param data the data segment of the PDU received over the connection.
 *  @param length the length of the data block received.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelRecv(UInt16 sessionId,
                        UInt32 connectionId,
                        iSCSIPDUTargetBHS * bhs,
                        void * * data,
                        size_t * length)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId || !bhs)
        return EINVAL;
    
    // Setup input scalar array
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    const UInt32 expOutputCnt = 1;
    UInt32 outputCnt = 1;
    UInt64 output;
    
    size_t bhsLength = sizeof(iSCSIPDUTargetBHS);

    // Call kernel method to determine how much data there is to receive
    // The inputs are the sesssion qualifier and connection ID
    // The output is the size of the buffer we need to allocate to hold the data
    kern_return_t kernResult;
    
    kernResult = IOConnectCallMethod(connection,kiSCSIRecvBHS,inputs,inputCnt,NULL,0,
                                     &output,&outputCnt,bhs,&bhsLength);
    
    if(kernResult != kIOReturnSuccess || outputCnt != expOutputCnt || output != 0)
        return EIO;
    
    // Determine how much data to allocate for the data buffer
    *length = iSCSIPDUGetDataSegmentLength((iSCSIPDUCommonBHS *)bhs);
    
    // If no data, were done at this point
    if(*length == 0)
        return 0;
    
    *data = iSCSIPDUDataCreate(*length);
        
    if(*data == NULL)
        return EIO;
    
    // Call kernel method to get data from a receive buffer
    if(IOConnectCallMethod(connection,kiSCSIRecvData,inputs,inputCnt,NULL,0,
                           &output,&outputCnt,*data,length) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt && output == 0)
            return 0;
    }
    
    // At this point we failed, free the temporary buffer and quit with error
    iSCSIPDUDataRelease(data);
    return EIO;
}


/*! Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetConnectionOptions(UInt16 sessionId,
                                        UInt32 connectionId,
                                        iSCSIConnectionOptions * options)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId ||
       connectionId == kiSCSIInvalidConnectionId || !options)
        return EINVAL;
    
    const UInt32 inputCnt = 2;
    const UInt64 inputs[] = {sessionId,connectionId};
    
    if(IOConnectCallMethod(connection,kiSCSISetConnectionOptions,inputs,inputCnt,
                           options,sizeof(struct iSCSIConnectionOptions),0,0,0,0) == kIOReturnSuccess)
    {
        return 0;
    }
    return EIO;
}

/*! Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionOptions(UInt16 sessionId,
                                        UInt32 connectionId,
                                        iSCSIConnectionOptions * options)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId ||
       connectionId     == kiSCSIInvalidConnectionId || !options)
        return EINVAL;
    
    const UInt32 inputCnt = 2;
    const UInt64 inputs[] = {sessionId,connectionId};

    size_t optionsSize = sizeof(struct iSCSIConnectionOptions);
    
    if(IOConnectCallMethod(connection,kiSCSIGetConnectionOptions,inputs,inputCnt,0,0,0,0,
                           options,&optionsSize) == kIOReturnSuccess)
    {
        return 0;
    }
    return EIO;
}

/*! Activates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateConnection(UInt16 sessionId,UInt32 connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Tell kernel to drop this connection
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    UInt64 output;
    UInt32 outputCnt = 1;
    const UInt32 expOutputCnt = 1;
    
    if(IOConnectCallScalarMethod(connection,kiSCSIActivateConnection,
                                 inputs,inputCnt,&output,&outputCnt) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    return EINVAL;
}

/*! Activates all iSCSI connections associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateAllConnections(UInt16 sessionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    UInt64 output;
    UInt32 outputCnt = 1;
    const UInt32 expOutputCnt = 1;
    
    if(IOConnectCallScalarMethod(connection,kiSCSIActivateAllConnections,
                                 &input,inputCnt,&output,&outputCnt) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    return EINVAL;
}


/*! Dectivates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to deactivate.
 *  @param connectionId  connection to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateConnection(UInt16 sessionId,UInt32 connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Tell kernel to drop this connection
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    UInt64 output;
    UInt32 outputCnt = 1;
    const UInt32 expOutputCnt = 1;
    
    if(IOConnectCallScalarMethod(connection,kiSCSIDeactivateConnection,
                                 inputs,inputCnt,&output,&outputCnt) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    return EINVAL;
}

/*! Dectivates all iSCSI sessions associated with a session.
 *  @param sessionId session associated with connections to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateAllConnections(UInt16 sessionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    UInt64 output;
    UInt32 outputCnt = 1;
    const UInt32 expOutputCnt = 1;
    
    if(IOConnectCallScalarMethod(connection,kiSCSIDeactivateAllConnections,
                                 &input,inputCnt,&output,&outputCnt) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    return EINVAL;
}

/*! Gets the first connection (the lowest connectionId) for the
 *  specified session.
 *  @param sessionId obtain an connectionId for this session.
 *  @param connectionId the identifier of the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnection(UInt16 sessionId,UInt32 * connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const UInt32 expOutputCnt = 2;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    if(IOConnectCallScalarMethod(connection,kiSCSIGetConnection,
                                 &input,inputCnt,output,&outputCnt) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    return EINVAL;

}

/*! Gets the connection count for the specified session.
 *  @param sessionId obtain the connection count for this session.
 *  @param numConnections the connection count.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetNumConnections(UInt16 sessionId,UInt32 * numConnections)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !numConnections)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const UInt32 expOutputCnt = 2;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    if(IOConnectCallScalarMethod(connection,kiSCSIGetNumConnections,
                                 &input,inputCnt,output,&outputCnt) == kIOReturnSuccess)
    {
        if(outputCnt == expOutputCnt)
            return (errno_t)output;
    }
    return EINVAL;
}


