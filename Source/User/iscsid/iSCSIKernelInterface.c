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

#include "iSCSIKernelInterface.h"
#include "iSCSIKernelInterfaceShared.h"
#include "iSCSIPDUUser.h"
#include "iSCSIKernelClasses.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/IOReturn.h>

static io_connect_t connection;
static CFMachPortRef     notificationPort = NULL;
static iSCSIKernelNotificationCallback callback;

/*! Select error codes used by the iSCSI user client. */
errno_t IOReturnToErrno(kern_return_t result)
{
    switch(result)
    {
        case kIOReturnSuccess: return 0;
        case kIOReturnBadArgument: return EINVAL;
        case kIOReturnBusy: return EBUSY;
        case kIOReturnIOError: return EIO;
        case kIOReturnUnsupported: return ENOTSUP;
        case kIOReturnNotPermitted: return EAUTH;
        case kIOReturnNoMemory: return ENOMEM;
        case kIOReturnNotFound: return ENODEV;
        case kIOReturnDeviceError: return EIO;
        case kIOReturnTimeout: return ETIME;
        case kIOReturnNotResponding: return EBUSY;
        case kIOReturnNoResources: return EAGAIN;
    };
    return EIO;
}

/*! Handles messages sent from the kernel. */
void iSCSIKernelNotificationHandler(CFMachPortRef port,void * msg,CFIndex size,void * info)
{
    // The parameter is a notification message
    iSCSIKernelNotificationMessage * notificationMsg;
    if(!(notificationMsg = msg))
        return;

    // Process notification type and return if invalid
    enum iSCSIKernelNotificationTypes type = (enum iSCSIKernelNotificationTypes)notificationMsg->notificationType;
    
    if(type == kiSCSIKernelNotificationInvalid)
        return;
    
    // Call the callback function with the message type and body
    if(callback)
        callback(type,msg);
}

/*! Creates a run loop source that is used to run the the notification callback
 *  function that was registered during the call to iSCSIKernelInitialize(). */
CFRunLoopSourceRef iSCSIKernelCreateRunLoopSource()
{
    if(notificationPort)
        return CFMachPortCreateRunLoopSource(kCFAllocatorDefault,notificationPort,0);
    
    return NULL;
}

/*! Opens a connection to the iSCSI initiator.  A connection must be
 *  successfully opened before any of the supporting functions below can be
 *  called. */
errno_t iSCSIKernelInitialize(iSCSIKernelNotificationCallback callback)
{
    kern_return_t result;
     	
	// Create a dictionary to match iSCSIkext
	CFMutableDictionaryRef matchingDict = NULL;
	matchingDict = IOServiceMatching(kiSCSIVirtualHBA_IOClassName);
    
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault,matchingDict);
    
	// Check to see if the driver was found in the I/O registry
	if(service == IO_OBJECT_NULL)
		return kIOReturnNotFound;
    
	// Using the service handle, open a connection
    result = IOServiceOpen(service,mach_task_self(),0,&connection);

    // No longer need a reference to the service
    IOObjectRelease(service);
	
	if(result != kIOReturnSuccess)
        return kIOReturnNotFound;
    
    CFMachPortContext notificationContext;
    notificationContext.info = (void *)&notificationContext;
    notificationContext.version = 0;
    notificationContext.release = NULL;
    notificationContext.retain  = NULL;
    notificationContext.copyDescription = NULL;
    
    // Create a mach port to receive notifications from the kernel
    notificationPort = CFMachPortCreate(kCFAllocatorDefault,
                                        iSCSIKernelNotificationHandler,
                                        &notificationContext,NULL);
    IOConnectSetNotificationPort(connection,0,CFMachPortGetPort(notificationPort),0);
    

    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIOpenInitiator,0,0,0,0));
}

/*! Closes a connection to the iSCSI initiator. */
errno_t iSCSIKernelCleanup()
{
    kern_return_t kernResult =
        IOConnectCallScalarMethod(connection,kiSCSICloseInitiator,0,0,0,0);
    
	// Clean up (now that we have a connection we no longer need the object)
    IOServiceClose(connection);
    
    if(notificationPort)
        CFRelease(notificationPort);
    
    return IOReturnToErrno(kernResult);
}

/*! Allocates a new iSCSI session in the kernel and creates an associated
 *  connection to the target portal. Additional connections may be added to the
 *  session by calling iSCSIKernelCreateConnection().
 *  @param targetIQN the name of the target.
 *  @param portalAddress the portal address (IPv4/IPv6, or DNS name).
 *  @param portalPort the TCP port used to connect to the portal.
 *  @param hostInterface the name of the host interface adapter to use.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param sessionId the session identifier for the new session (returned).
 *  @param connectionId the identifier of the new connection (returned).
 *  @return An error code if a valid session could not be created. */
errno_t iSCSIKernelCreateSession(CFStringRef targetIQN,
                                 CFStringRef portalAddress,
                                 CFStringRef portalPort,
                                 CFStringRef hostInterface,
                                 const struct sockaddr_storage * portalSockAddr,
                                 const struct sockaddr_storage * hostSockAddr,
                                 SID * sessionId,
                                 CID * connectionId)
{
    // Check parameters
    if(!portalAddress || !portalPort || !hostInterface || !portalSockAddr ||
       !hostSockAddr || !sessionId || !connectionId)
        return EINVAL;
    
    // Pack the input parameters into a single buffer to send to the kernel
    const int kNumParams = 6;
    void * params[kNumParams];
    size_t paramSize[kNumParams];
    
    // Add one for string lengths to copy the NULL character (CFGetStringLength
    // does not include the length of the NULL terminator)
    paramSize[0] = CFStringGetLength(targetIQN) + 1;
    paramSize[1] = CFStringGetLength(portalAddress) + 1;
    paramSize[2] = CFStringGetLength(portalPort) + 1;
    paramSize[3] = CFStringGetLength(hostInterface) + 1;
    paramSize[4] = sizeof(struct sockaddr_storage);
    paramSize[5] = sizeof(struct sockaddr_storage);
    
    // Populate parameters
    params[0] = malloc(paramSize[0]);
    params[1] = malloc(paramSize[1]);
    params[2] = malloc(paramSize[2]);
    params[3] = malloc(paramSize[3]);
    params[4] = (void*)portalSockAddr;
    params[5] = (void*)hostSockAddr;
    
    CFStringGetCString(targetIQN,params[0],paramSize[0],kCFStringEncodingASCII);
    CFStringGetCString(portalAddress,params[1],paramSize[1],kCFStringEncodingASCII);
    CFStringGetCString(portalPort,params[2],paramSize[2],kCFStringEncodingASCII);
    CFStringGetCString(hostInterface,params[3],paramSize[3],kCFStringEncodingASCII);
    
    // The input buffer will first have eight bytes to denote the length of
    // the portion that follows.  So for each of the six input parameters,
    // we'll have six UInt64 blocks that indicate the size up front.
    size_t header = kNumParams*sizeof(UInt64);
    size_t inputStructSize = header;
    
    CFIndex paramIdx = 0;
    while(paramIdx < kNumParams) {
        inputStructSize += paramSize[paramIdx];
        paramIdx++;
    }
    
    UInt8 * inputStruct = (UInt8*)malloc(inputStructSize);
    UInt8 * inputStructPos = inputStruct + header;

    paramIdx = 0;
    while(paramIdx < kNumParams) {
        memcpy(inputStructPos,params[paramIdx],paramSize[paramIdx]);
        inputStructPos += paramSize[paramIdx];
        
        UInt64 * header = (UInt64*)(inputStruct + sizeof(UInt64)*paramIdx);
        *header = paramSize[paramIdx];
        paramIdx++;
    }
    
    const UInt32 inputCnt = 1;
    UInt64 inputs[inputCnt];
    inputs[0] = kNumParams;
    
    const UInt32 expOutputCnt = 3;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    kern_return_t result =
        IOConnectCallMethod(connection,kiSCSICreateSession,inputs,inputCnt,
                            inputStruct,inputStructSize,output,&outputCnt,0,0);
    
    // Free allocated memory
    free(params[0]);
    free(params[1]);
    free(params[2]);
    free(params[3]);
    free(inputStruct);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt) {
        *sessionId    = (UInt16)output[0];
        *connectionId = (UInt32)output[1];
        
        errno_t error = (errno_t)output[2];
        return error;
    }
    
    return IOReturnToErrno(result);
}

/*! Releases an iSCSI session, including all connections associated with that
 *  session.
 *  @param sessionId the session qualifier part of the ISID.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelReleaseSession(SID sessionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;

    // Tell the kernel to drop this session and all of its related resources
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIReleaseSession,&input,inputCnt,0,0));
}

/*! Sets option associated with a particular session.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param option the option to set.
 *  @param optVal the value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetSessionOpt(SID sessionId,
                                 enum iSCSIKernelSessionOptTypes option,
                                 void * optVal,
                                 size_t optSize)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !optVal || optSize == 0)
        return EINVAL;
    
    UInt64 optValCopy = 0;
    memcpy(&optValCopy,optVal,optSize);
    
    const UInt32 inputCnt = 3;
    const UInt64 input[] = {sessionId,option,optValCopy};
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSISetSessionOption,input,inputCnt,0,0));
}

/*! Gets option associated with a particular session.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param option the option to get.
 *  @param optVal the returned value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionOpt(SID sessionId,
                                 enum iSCSIKernelSessionOptTypes option,
                                 void * optVal,
                                 size_t optSize)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !optVal || optSize == 0)
        return EINVAL;
    
    const UInt32 inputCnt = 2;
    const UInt64 input[] = {sessionId,option};
    
    UInt32 outputCnt = 1;
    UInt64 output;

    kern_return_t error = IOConnectCallScalarMethod(connection,kiSCSIGetSessionOption,
                                                    input,inputCnt,&output,&outputCnt);
    
    if(error == kIOReturnSuccess)
        memcpy(optVal,&output,optSize);
    
    return IOReturnToErrno(error);
}

/*! Allocates an additional iSCSI connection for a particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param portalAddress the portal address (IPv4/IPv6, or DNS name).
 *  @param portalPort the TCP port used to connect to the portal.
 *  @param hostInterface the name of the host interface adapter to use.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host. This
 *  specifies the interface that the connection will be bound to.
 *  @param connectionId the identifier of the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelCreateConnection(SID sessionId,
                                    CFStringRef portalAddress,
                                    CFStringRef portalPort,
                                    CFStringRef hostInterface,
                                    const struct sockaddr_storage * portalSockAddr,
                                    const struct sockaddr_storage * hostSockAddr,
                                    CID * connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !portalAddress || !portalPort || !hostInterface || !portalSockAddr || !connectionId)
        return EINVAL;
    
    // Pack the input parameters into a single buffer to send to the kernel
    const int kNumParams = 5;

    void * params[kNumParams];
    size_t paramSize[kNumParams];
    
    // Add one for string lengths to copy the NULL character (CFGetStringLength
    // does not include the length of the NULL terminator)
    paramSize[0] = CFStringGetLength(portalAddress) + 1;
    paramSize[1] = CFStringGetLength(portalPort) + 1;
    paramSize[2] = CFStringGetLength(hostInterface) + 1;
    paramSize[3] = sizeof(struct sockaddr_storage);
    paramSize[4] = sizeof(struct sockaddr_storage);
    
    params[0] = malloc(paramSize[0]);
    params[1] = malloc(paramSize[1]);
    params[2] = malloc(paramSize[2]);
    params[3] = (void*)portalSockAddr;
    params[4] = (void*)hostSockAddr;
    
    CFStringGetCString(portalAddress,params[0],paramSize[0],kCFStringEncodingASCII);
    CFStringGetCString(portalPort,params[1],paramSize[1],kCFStringEncodingASCII);
    CFStringGetCString(hostInterface,params[2],paramSize[2],kCFStringEncodingASCII);
    
    // The input buffer will first have eight bytes to denote the length of
    // the portion that follows.  So for each of the six input parameters,
    // we'll have six UInt64 blocks that indicate the size up front.
    size_t header = kNumParams*sizeof(UInt64);
    size_t inputStructSize = header;
    
    CFIndex paramIdx = 0;
    while(paramIdx < kNumParams) {
        inputStructSize += paramSize[paramIdx];
        paramIdx++;
    }
    
    UInt8 * inputStruct = (UInt8*)malloc(inputStructSize);
    UInt8 * inputStructPos = inputStruct + header;
    
    paramIdx = 0;
    while(paramIdx < kNumParams) {
        memcpy(inputStructPos,params[paramIdx],paramSize[paramIdx]);
        inputStructPos += paramSize[paramIdx];
        
        UInt64 * header = (UInt64*)(inputStruct + sizeof(UInt64)*paramIdx);
        *header = paramSize[paramIdx];
        paramIdx++;
    }
    
    // Tell the kernel to drop this session and all of its related resources
    const UInt32 inputCnt = 2;
    const UInt64 inputs[] = {sessionId,kNumParams};
    
    const UInt32 expOutputCnt = 2;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    kern_return_t result =
        IOConnectCallMethod(connection,kiSCSICreateConnection,inputs,inputCnt,inputStruct,
                            inputStructSize,output,&outputCnt,0,0);
    
    // Free memory
    free(params[0]);
    free(params[1]);
    free(params[2]);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt) {
        *connectionId = (UInt32)output[0];
        errno_t error = (errno_t)output[1];
        
        return error;
    }

    return IOReturnToErrno(result);
}

/*! Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelReleaseConnection(SID sessionId,CID connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;

    // Tell kernel to drop this connection
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIReleaseConnection,inputs,inputCnt,0,0));
}

/*! Sends data over a kernel socket associated with iSCSI.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment to send over the connection.
 *  @param data the data segment of the PDU to send over the connection.
 *  @param length the length of the data block to send over the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSend(SID sessionId,
                        CID connectionId,
                        iSCSIPDUInitiatorBHS * bhs,
                        void * data,
                        size_t length)
{
    // Check parameters
    if(sessionId    == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId || !bhs || (!data && length > 0))
        return EINVAL;
    
    // Setup input scalar array
    const UInt32 inputCnt = 2;
    const UInt64 inputs[] = {sessionId, connectionId};
    
    // Call kernel method to send (buffer) bhs and then data
    kern_return_t result;
    result = IOConnectCallStructMethod(connection,kiSCSISendBHS,bhs,
                                       sizeof(iSCSIPDUInitiatorBHS),NULL,NULL);
    
    if(result != kIOReturnSuccess)
        return IOReturnToErrno(result);
    
    return IOReturnToErrno(IOConnectCallMethod(connection,kiSCSISendData,inputs,inputCnt,
                                               data,length,NULL,NULL,NULL,NULL));
}

/*! Receives data over a kernel socket associated with iSCSI.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received over the connection.
 *  @param data the data segment of the PDU received over the connection.
 *  @param length the length of the data block received.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelRecv(SID sessionId,
                        CID connectionId,
                        iSCSIPDUTargetBHS * bhs,
                        void ** data,
                        size_t * length)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId || !bhs)
        return EINVAL;
    
    // Setup input scalar array
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    size_t bhsLength = sizeof(iSCSIPDUTargetBHS);

    // Call kernel method to determine how much data there is to receive
    // The inputs are the sesssion qualifier and connection ID
    // The output is the size of the buffer we need to allocate to hold the data
    kern_return_t result;
    result = IOConnectCallMethod(connection,kiSCSIRecvBHS,inputs,inputCnt,NULL,0,
                                 NULL,NULL,bhs,&bhsLength);
    
    if(result != kIOReturnSuccess)
        return IOReturnToErrno(result);
    
    // Determine how much data to allocate for the data buffer
    *length = iSCSIPDUGetDataSegmentLength((iSCSIPDUCommonBHS *)bhs);
    
    // If no data, were done at this point
    if(*length == 0)
        return 0;
    
    *data = iSCSIPDUDataCreate(*length);
        
    if(*data == NULL)
        return EIO;
    
    // Call kernel method to get data from a receive buffer
    result = IOConnectCallMethod(connection,kiSCSIRecvData,inputs,inputCnt,NULL,0,
                                 NULL,NULL,*data,length);

    // If we failed, free the temporary buffer and quit with error
    if(result != kIOReturnSuccess)
        iSCSIPDUDataRelease(data);
    
    return IOReturnToErrno(result);
}

/*! Sets option associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param option the option to set.
 *  @param optVal the value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelSetConnectionOpt(SID sessionId,
                                    CID connectionId,
                                    enum iSCSIKernelConnectionOptTypes option,
                                    void * optVal,
                                    size_t optSize)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId || !optVal || optSize == 0)
        return EINVAL;
    
    UInt64 optValCopy = 0;
    memcpy(&optValCopy,optVal,optSize);
    
    const UInt32 inputCnt = 4;
    const UInt64 inputs[] = {sessionId,connectionId,option,optValCopy};
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSISetConnectionOption,inputs,inputCnt,0,0));
}

/*! Gets option associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param option the option to get.
 *  @param optVal the returned value for the specified option.
 *  @param optSize the size, in bytes of optVal.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionOpt(SID sessionId,
                                    CID connectionId,
                                    enum iSCSIKernelConnectionOptTypes option,
                                    void * optVal,
                                    size_t optSize)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId  == kiSCSIInvalidConnectionId || !optVal || optSize == 0)
        return EINVAL;
    
    const UInt32 inputCnt = 3;
    const UInt64 input[] = {sessionId,connectionId,option};
    
    UInt32 outputCnt = 1;
    UInt64 output;
    
    kern_return_t error = IOConnectCallScalarMethod(connection,kiSCSIGetConnectionOption,
                                                    input,inputCnt,&output,&outputCnt);
    
    if(error == kIOReturnSuccess)
        memcpy(optVal,&output,optSize);
        
    return IOReturnToErrno(error);
}

/*! Activates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @param connectionId  connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateConnection(SID sessionId,CID connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Tell kernel to drop this connection
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIActivateConnection,
                                                     inputs,inputCnt,NULL,NULL));
}

/*! Activates all iSCSI connections associated with a session.
 *  @param sessionId session associated with connection to activate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelActivateAllConnections(SID sessionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIActivateAllConnections,
                                                     &input,inputCnt,NULL,NULL));
}

/*! Dectivates an iSCSI connection associated with a session.
 *  @param sessionId session associated with connection to deactivate.
 *  @param connectionId  connection to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateConnection(SID sessionId,CID connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Tell kernel to drop this connection
    const UInt32 inputCnt = 2;
    UInt64 inputs[] = {sessionId,connectionId};
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIDeactivateConnection,
                                                     inputs,inputCnt,NULL,NULL));
}

/*! Dectivates all iSCSI sessions associated with a session.
 *  @param sessionId session associated with connections to deactivate.
 *  @return error code inidicating result of operation. */
errno_t iSCSIKernelDeactivateAllConnections(SID sessionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    return IOReturnToErrno(IOConnectCallScalarMethod(connection,kiSCSIDeactivateAllConnections,
                                                     &input,inputCnt,NULL,NULL));
}

/*! Gets the first connection (the lowest connectionId) for the
 *  specified session.
 *  @param sessionId obtain an connectionId for this session.
 *  @param connectionId the identifier of the connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnection(SID sessionId,CID * connectionId)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const UInt32 expOutputCnt = 1;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    kern_return_t result =
        IOConnectCallScalarMethod(connection,kiSCSIGetConnection,&input,inputCnt,
                                  output,&outputCnt);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt)
        *connectionId = (UInt32)output[0];

    return IOReturnToErrno(result);
}

/*! Gets the connection count for the specified session.
 *  @param sessionId obtain the connection count for this session.
 *  @param numConnections the connection count.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetNumConnections(SID sessionId,UInt32 * numConnections)
{
    // Check parameters
    if(sessionId == kiSCSIInvalidSessionId || !numConnections)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const UInt32 expOutputCnt = 1;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    kern_return_t result = IOConnectCallScalarMethod(
        connection,kiSCSIGetNumConnections,&input,inputCnt,output,&outputCnt);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt)
        *numConnections = (UInt32)output[0];

    return IOReturnToErrno(result);
}

/*! Looks up the session identifier associated with a particular target name.
 *  @param targetIQN the IQN name of the target (e.q., iqn.2015-01.com.example)
 *  @return sessionId the session identifier. */
SID iSCSIKernelGetSessionIdForTargetIQN(CFStringRef targetIQN)
{
    if(!targetIQN)
        return kiSCSIInvalidSessionId;
    
    const UInt32 expOutputCnt = 1;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    const int targetIQNBufferSize = (int)CFStringGetLength(targetIQN)+1;
    char * targetIQNBuffer = (char *)malloc(targetIQNBufferSize);
    if(!CFStringGetCString(targetIQN,targetIQNBuffer,targetIQNBufferSize,kCFStringEncodingASCII))
    {
        free(targetIQNBuffer);
        return kiSCSIInvalidSessionId;
    }

    kern_return_t result = IOConnectCallMethod(
        connection,
        kiSCSIGetSessionIdForTargetIQN,0,0,
        targetIQNBuffer,
        targetIQNBufferSize,
        output,&outputCnt,0,0);
    
    free(targetIQNBuffer);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt)
        return (SID)output[0];
    
    return kiSCSIInvalidSessionId;
}

/*! Looks up the connection identifier associated with a particular portal address.
 *  @param sessionId the session identifier.
 *  @param portalAddress the address passed to iSCSIKernelCreateSession() or
 *  iSCSIKernelCreateConnection() when the connection was created.
 *  @return the associated connection identifier. */
CID iSCSIKernelGetConnectionIdForPortalAddress(SID sessionId,
                                               CFStringRef portalAddress)
{
    if(sessionId == kiSCSIInvalidSessionId || !portalAddress)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const UInt32 expOutputCnt = 1;
    UInt64 output[expOutputCnt];
    UInt32 outputCnt = expOutputCnt;
    
    const int portalAddressBufferSize = (int)CFStringGetLength(portalAddress)+1;
    char * portalAddressBuffer = (char*)malloc(portalAddressBufferSize);
    if(!CFStringGetCString(portalAddress,portalAddressBuffer,portalAddressBufferSize,kCFStringEncodingASCII))
    {
        free(portalAddressBuffer);
        return EINVAL;
    }
    
    kern_return_t result =
        IOConnectCallMethod(connection,kiSCSIGetConnectionIdForPortalAddress,
                            &input,inputCnt,
                            portalAddressBuffer,
                            portalAddressBufferSize,
                            output,&outputCnt,0,0);
    
    free(portalAddressBuffer);
    
    if(result != kIOReturnSuccess || outputCnt != expOutputCnt)
        return kiSCSIInvalidConnectionId;
    
    return (CID)output[0];
}

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.  This MUST be large
 *  enough to hold the maximum number of sessions (kiSCSIMaxSessions).
 *  @param sessionCount number of session identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetSessionIds(SID * sessionIds,
                                 UInt16 * sessionCount)
{
    if(!sessionIds || !sessionCount)
        return EINVAL;
    
    const UInt32 expOutputCnt = 1;
    UInt64 output;
    UInt32 outputCnt = expOutputCnt;
    
    *sessionCount = 0;
    size_t outputStructSize = sizeof(SID)*kiSCSIMaxSessions;
    
    kern_return_t result =
        IOConnectCallMethod(connection,kiSCSIGetSessionIds,0,0,0,0,
                            &output,&outputCnt,sessionIds,&outputStructSize);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt)
        *sessionCount = (UInt16)output;

    return IOReturnToErrno(result);
}

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @param connectionIds an array of connection identifiers for the session.
 *  @param connectionCount number of connection identifiers.
 *  @return error code indicating result of operation. */
errno_t iSCSIKernelGetConnectionIds(SID sessionId,
                                    CID * connectionIds,
                                    UInt32 * connectionCount)
{
    if(sessionId == kiSCSIInvalidSessionId || !connectionIds || !connectionCount)
        return EINVAL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const UInt32 expOutputCnt = 1;
    UInt64 output;
    UInt32 outputCnt = expOutputCnt;
    
    *connectionCount = 0;
    size_t outputStructSize = sizeof(CID)*kiSCSIMaxConnectionsPerSession;

    kern_return_t result =
        IOConnectCallMethod(connection,kiSCSIGetConnectionIds,&input,inputCnt,0,0,
                            &output,&outputCnt,connectionIds,&outputStructSize);
    
    if(result == kIOReturnSuccess && outputCnt == expOutputCnt)
        *connectionCount = (UInt32)output;
    
    return IOReturnToErrno(result);
}

/*! Creates a string containing the target IQN associated with a session.
 *  @param sessionId session identifier.
 *  @param targetIQN the name of the target.
 *  @param size the size of the targetIQNCString buffer.
 *  @return error code indicating result of operation. */
CFStringRef iSCSIKernelCreateTargetIQNForSessionId(SID sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    const UInt32 inputCnt = 1;
    UInt64 input = sessionId;
    
    const char targetIQN[NI_MAXHOST];
    size_t targetIQNLength = NI_MAXHOST;
    
    kern_return_t result = IOConnectCallMethod(connection,kiSCSICreateTargetIQNForSessionId,
                                               &input,inputCnt,0,0,0,0,
                                               (void*)targetIQN,&targetIQNLength);
    if(result != kIOReturnSuccess)
        return NULL;
    
    return CFStringCreateWithCString(kCFAllocatorDefault,targetIQN,kCFStringEncodingASCII);
}

/*! Creates a string containing the address of the portal associated with 
 *  the specified connection.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the portal address, or NULL if the session or
 *  connection was invalid. */
CFStringRef iSCSIKernelCreatePortalAddressForConnectionId(SID sessionId,CID connectionId)
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    const UInt32 inputCnt = 2;
    UInt64 input[] = {sessionId,connectionId};
    
    const char portalAddress[NI_MAXHOST];
    size_t portalAddressLength = NI_MAXHOST;
    
    kern_return_t result = IOConnectCallMethod(connection,kiSCSIGetPortalAddressForConnectionId,
                                               input,inputCnt,0,0,0,0,
                                               (void *)portalAddress,&portalAddressLength);
    if(result != kIOReturnSuccess)
        return NULL;
    
    return CFStringCreateWithCString(kCFAllocatorDefault,portalAddress,kCFStringEncodingASCII);
}

/*! Creates a string containing the TCP port of the portal associated with
 *  the specified connection.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the TCP port of the portal, or NULL if the
 *  session or connection was invalid. */
CFStringRef iSCSIKernelCreatePortalPortForConnectionId(SID sessionId,CID connectionId)
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    const UInt32 inputCnt = 2;
    UInt64 input[] = {sessionId,connectionId};

    const char portalPort[NI_MAXSERV];
    size_t portalPortLength = NI_MAXSERV;
    
    kern_return_t result = IOConnectCallMethod(connection,kiSCSIGetPortalPortForConnectionId,
                                               input,inputCnt,0,0,0,0,
                                               (void *)portalPort,&portalPortLength);
    if(result != kIOReturnSuccess)
        return NULL;
    
    return CFStringCreateWithCString(kCFAllocatorDefault,portalPort,kCFStringEncodingASCII);
}

/*! Creates a string containing the host interface used for the connection.
 *  @param sessionId session identifier.
 *  @param connectionId connection identifier.
 *  @return a string containing the host interface name, or NULL if the 
 *  session or connection was invalid. */
CFStringRef iSCSIKernelCreateHostInterfaceForConnectionId(SID sessionId,CID connectionId)
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    const UInt32 inputCnt = 2;
    UInt64 input[] = {sessionId,connectionId};
    
    
    const char hostInterface[NI_MAXHOST];
    size_t hostInterfaceLength = NI_MAXHOST;

    kern_return_t result = IOConnectCallMethod(connection,kiSCSIGetHostInterfaceForConnectionId,
                                               input,inputCnt,0,0,0,0,
                                               (void *)hostInterface,&hostInterfaceLength);
    if(result != kIOReturnSuccess)
        return NULL;
    
    return CFStringCreateWithCString(kCFAllocatorDefault,hostInterface,kCFStringEncodingASCII);
}



