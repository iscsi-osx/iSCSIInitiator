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
#include "iSCSIIOEventSource.h"
#include "iSCSITaskQueue.h"
#include "iSCSITypesKernel.h"
#include "iSCSIRFC3720Defaults.h"
#include "iSCSIHBAUserClient.h"
#include "crc32c.h"

#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/select.h>

#include <IOKit/IORegistryEntry.h>

// Use DBLog() for debug outputs and IOLog() for all outputs
// DBLog() is only enabled for debug builds
#ifdef DEBUG
#define DBLog(...) IOLog(__VA_ARGS__)
#else
#define DBLog(...)
#endif

#define super IOSCSIParallelInterfaceController

#define ISCSI_PRODUCT_NAME              "iSCSI Virtual Host Bus Adapter"
#define ISCSI_PRODUCT_REVISION_LEVEL    "1.0"

using namespace iSCSIPDU;

/*! Maximum number of connections allowed per session. */
const UInt16 iSCSIVirtualHBA::kMaxConnectionsPerSession = kiSCSIMaxConnectionsPerSession;

/*! Maximum number of session allowed (globally). */
const UInt16 iSCSIVirtualHBA::kMaxSessions = kiSCSIMaxSessions;

/*! Highest LUN supported by the virtual HBA.  Due to internal design 
 *  contraints, this number should never exceed 2**8 - 1 or 255 (8-bits). */
const SCSILogicalUnitNumber iSCSIVirtualHBA::kHighestLun = 63;

/*! Highest SCSI device ID supported by the HBA.  SCSI device identifiers are
 *  just the session identifiers. */
const SCSIDeviceIdentifier iSCSIVirtualHBA::kHighestSupportedDeviceId = kMaxSessions - 1;

/*! Maximum number of SCSI tasks the HBA can handle.  Increasing this number will
 *  increase the wired memory consumed by this kernel extension. */
const UInt32 iSCSIVirtualHBA::kMaxTaskCount = 10;

/*! Number of bytes that are transmitted before we calculate an average speed
 *  for the connection (1024^2 = 1048576). */
const UInt32 iSCSIVirtualHBA::kNumBytesPerAvgBW = 1048576;

/*! Default task timeout for new tasks (milliseconds). */
const UInt32 iSCSIVirtualHBA::kiSCSITaskTimeoutMs = 20000;

/*! Default TCP timeout for new connections (seconds). */
const UInt32 iSCSIVirtualHBA::kiSCSITCPTimeoutSec = 1;


OSDefineMetaClassAndStructors(iSCSIVirtualHBA,IOSCSIParallelInterfaceController);

SCSILogicalUnitNumber iSCSIVirtualHBA::ReportHBAHighestLogicalUnitNumber()
{
	return kHighestLun;
}

bool iSCSIVirtualHBA::DoesHBASupportSCSIParallelFeature(SCSIParallelFeature theFeature)
{
    bool supported = false; // return false for any unimplemented or unknown features
    
    switch (theFeature) {
        case kSCSIParallelFeature_WideDataTransfer:
            supported = true;
            break;
        case kSCSIParallelFeature_SynchronousDataTransfer:
            supported = true;
            break;
        case kSCSIParallelFeature_QuickArbitrationAndSelection:
            supported = true;
            break;
        case kSCSIParallelFeature_DoubleTransitionDataTransfers:
            supported = true;
            break;
        case kSCSIParallelFeature_InformationUnitTransfers:
            supported = true;
            break;
        default: break;
    }
    
    return supported;
}

bool iSCSIVirtualHBA::InitializeTargetForID(SCSITargetIdentifier targetId)
{
    // Find and set the IQN of the target in the IORegistry.  First we need
    // to iterate over the target names and find the one that matches our
    // target identifiers (session identifier).  Next, we copy the existing
    // protocol dictionary and add a custom property for the IQN.
    
    OSCollectionIterator * iterator = OSCollectionIterator::withCollection(targetList);
    
    if(!iterator)
        return false;
    
    OSObject * object;
    
    while((object = iterator->getNextObject()))
    {
        OSString * targetIQN = OSDynamicCast(OSString,object);
        OSNumber * sessionIdNumber = OSDynamicCast(OSNumber,targetList->getObject(targetIQN));
        
        if(sessionIdNumber->unsigned16BitValue() == targetId)
            break;
    }
    
    // Set the name of the target in the IORegistry
    IOService * device;
    if(!(device = (IOService*)GetTargetForID(targetId)))
        return false;
    
    OSDictionary * copyDict, * protocolDict;
    
    if(!(copyDict = OSDynamicCast(OSDictionary,device->getProperty(kIOPropertyProtocolCharacteristicsKey))))
        return false;
    
    if((protocolDict = OSDynamicCast(OSDictionary,copyDict->copyCollection())))
    {
        OSString * targetIQN = OSDynamicCast(OSString,object);
        
        if(targetIQN) {
            protocolDict->setObject("iSCSI Qualified Name",targetIQN);
        }
        
        protocolDict->setObject(kIOPropertyPhysicalInterconnectTypeKey,OSString::withCString("iSCSI"));
        device->setProperty(kIOPropertyProtocolCharacteristicsKey,protocolDict);
        
        protocolDict->release();
    }
    
  	return true;
}

SCSIServiceResponse iSCSIVirtualHBA::AbortTaskRequest(SCSITargetIdentifier targetId,
													  SCSILogicalUnitNumber LUN,
													  SCSITaggedTaskIdentifier taggedTaskID)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iscsi: Abort task request (TID: %llu, LUN: %llu)\n",targetId,LUN);

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncAbortTask);
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTask;
    bhs.referencedTaskTag = OSSwapHostToBigInt32((UInt32)taggedTaskID);

    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::AbortTaskSetRequest(SCSITargetIdentifier targetId,
														 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iscsi: Abort task set request (TID: %llu, LUN: %llu)\n",targetId,LUN);

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncAbortTaskSet);
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTaskSet;
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::ClearACARequest(SCSITargetIdentifier targetId,
													 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iscsi: Clear ACA request (TID: %llu, LUN: %llu)\n",targetId,LUN);

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncClearACA);
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearACA;
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::ClearTaskSetRequest(SCSITargetIdentifier targetId,
														 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iscsi: Clear task set request (TID: %llu, LUN: %llu)\n",targetId,LUN);

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncClearTaskSet);
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearTaskSet;
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::LogicalUnitResetRequest(SCSITargetIdentifier targetId,
															 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iscsi: LUN reset request (TID: %llu, LUN: %llu)\n",targetId,LUN);

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncLUNReset);
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncLUNReset;
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::TargetResetRequest(SCSITargetIdentifier targetId)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iscsi: Target reset request (TID: %llu)\n",targetId);

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncTargetWarmReset;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeTaskMgmt,0,kiSCSIPDUTaskMgmtFuncTargetWarmReset);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIInitiatorIdentifier iSCSIVirtualHBA::ReportInitiatorIdentifier()
{
    // Random number generated each time this kext loads
	return kInitiatorId;
}

SCSIDeviceIdentifier iSCSIVirtualHBA::ReportHighestSupportedDeviceID()
{
	return kHighestSupportedDeviceId;
}

UInt32 iSCSIVirtualHBA::ReportMaximumTaskCount()
{
	return kMaxTaskCount;
}

UInt32 iSCSIVirtualHBA::ReportHBASpecificTaskDataSize()
{
    // Due to a bug (feature?) in the SCSI family driver, this value cannot
    // be zero, even if task data is not required.
	return 1;
}

UInt32 iSCSIVirtualHBA::ReportHBASpecificDeviceDataSize()
{
    // Due to a bug (feature?) in the SCSI family driver, this value cannot
    // be zero, even if device data is not required.
	return 1;
}

bool iSCSIVirtualHBA::DoesHBAPerformDeviceManagement()
{
	// Lets the superclass know that we are going to create and destroy
	// our own targets as we setup/teardown iSCSI connections
	return true;
}

bool iSCSIVirtualHBA::InitializeController()
{
    DBLog("iscsi: Initializing virtual HBA\n");
    
    // Initialize CRC32C
    crc32c_init();
    
    // Setup session & target list
    sessionList = (iSCSISession **)IOMalloc(kMaxSessions*sizeof(iSCSISession*));
    targetList  = OSDictionary::withCapacity(kMaxSessions);
    
    if(!sessionList)
        return false;
    
    memset(sessionList,0,kMaxSessions*sizeof(iSCSISession *));
    
    // Set product name.
    SetHBAProperty(kIOPropertyProductNameKey,OSString::withCString(ISCSI_PRODUCT_NAME));
    SetHBAProperty(kIOPropertyProductRevisionLevelKey,OSString::withCString(ISCSI_PRODUCT_REVISION_LEVEL));

    // Generate an initiator id using a random number (per RFC3720)
    kInitiatorId = random();
    
    // Make ourselves discoverable to user clients (we do this last after
    // everything is initialized).
    registerService();

    
	// Successfully initialized controller
	return true;
}

void iSCSIVirtualHBA::TerminateController()
{
    DBLog("iscsi: Terminating virtual HBA\n");
    
    ReleaseAllSessions();
    
    // Free up our list of sessions and targets
    IOFree(sessionList,kMaxSessions*sizeof(iSCSISession*));
    targetList->free();
}

bool iSCSIVirtualHBA::StartController()
{
	// Successfully started controller
	return true;
}

void iSCSIVirtualHBA::StopController()
{
}

void iSCSIVirtualHBA::HandleInterruptRequest()
{
	// We don't use physical interrupts (this is a virtual HBA)
}

/*! Handles task timeouts.
 *  @param task the task that timed out. */
void iSCSIVirtualHBA::HandleTimeout(SCSIParallelTaskIdentifier task)
{
    // Determine the target identifier (session identifier) and connection
    // associated with this task and remove the task from the task queue.
    SessionIdentifier sessionId = (UInt16)GetTargetIdentifier(task);
    ConnectionIdentifier connectionId = *((UInt32*)GetHBADataPointer(task));
    
    if(connectionId >= kMaxConnectionsPerSession)
        return;
    
    iSCSISession * session = sessionList[sessionId];
    if(!session)
        return;
    
    iSCSIConnection * connection = session->connections[connectionId];
    if(!connection)
        return;

    // Note: task tag is always 32-bits, even though the SCSI stack allows for 64-bit storage of the tag
    DBLog("iscsi: Task timeout for task %#x (sid: %d, cid: %d)\n",(UInt32)GetControllerTaskIdentifier(task),sessionId,connectionId);

    
    // If the task timeout is due to a broken connection, handle it.
    // Otherwise the target may be taking too long, just report it up the
    // driver stack
    struct sockaddr peername;
    if(sock_getpeername(connection->socket,&peername,sizeof(peername))) {
        HandleConnectionTimeout(sessionId,connectionId);
        return;
    }

    // Let task queue know that the last (current) task should be removed
    connection->taskQueue->completeCurrentTask();
    
    // Notify the SCSI stack that the task could not be delivered
    CompleteParallelTask(session,
                         connection,
                         task,
                         kSCSITaskStatus_DeliveryFailure,
                         kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE);
}

/*! Handles connection timeouts.
 *  @param sessionId the session associated with the timed-out connection.
 *  @param connectionId the connection that timed out. */
void iSCSIVirtualHBA::HandleConnectionTimeout(SessionIdentifier sessionId,ConnectionIdentifier connectionId)
{
    // If this is the last connection, release the session...
    iSCSISession * session;
    
    if(!(session = sessionList[sessionId]))
       return;

    DBLog("iscsi: Connection timeout (sid: %d, cid: %d)\n",sessionId,connectionId);
    
    ConnectionIdentifier connectionCount = 0;
    for(ConnectionIdentifier connectionId = 0; connectionId < kiSCSIMaxConnectionsPerSession; connectionId++)
        if(session->connections[connectionId])
            connectionCount++;
    
    // In the future add recovery here...
    if(connectionCount > 1)
        DeactivateConnection(sessionId,connectionId);
    else
        DeactivateAllConnections(sessionId);

    // Send a notification to the daemon; if the daemon does not respond then
    // release the session or connection as appropriate
    iSCSIHBAUserClient * client = (iSCSIHBAUserClient*)getClient();
    
    if(client)
        client->sendTimeoutMessageNotification(sessionId,connectionId);
    else
    {
        if(connectionCount > 1)
            ReleaseConnection(sessionId,connectionId);
        else
            ReleaseSession(sessionId);
    }
}

SCSIServiceResponse iSCSIVirtualHBA::ProcessParallelTask(SCSIParallelTaskIdentifier parallelTask)
{
    // Here we set an (iSCSI) initiator task tag for the SCSI task and queue
    // the iSCSI task for later processing
    SCSITargetIdentifier targetId   = GetTargetIdentifier(parallelTask);
    SCSILogicalUnitNumber LUN       = GetLogicalUnitNumber(parallelTask);
    SCSITaggedTaskIdentifier taskId = GetTaggedTaskIdentifier(parallelTask);
    
    iSCSISession * session = sessionList[(SessionIdentifier)targetId];
    
    if(!session)
        return kSCSIServiceResponse_FUNCTION_REJECTED;
    
    // Determine which connection this task should be assigned to based on
    // bitrate and processing load; we do this by looking at the amount of
    // data each connection needs to transfer
    iSCSIConnection * connection = NULL;
    size_t minTimeToTransfer = INT64_MAX;
    
    for(UInt32 idx = 0; idx < kiSCSIMaxConnectionsPerSession; idx++)
    {
        iSCSIConnection * conn = session->connections[idx];
        
        // If this connection slot doesn't exist or isn't enabled, move on...
        if(!conn || !conn->taskQueue->isEnabled())
            continue;
        
        if(conn->bytesPerSecond == 0) {
            connection = conn;
            break;
        }
        
        size_t timeToTransfer = conn->dataToTransfer / conn->bytesPerSecond;
        
        if(timeToTransfer <= minTimeToTransfer) {
            minTimeToTransfer = timeToTransfer;
            connection = conn;
        }
    }
    
    connection = session->connections[0];
    if(!connection || !connection->dataRecvEventSource)
        return kSCSIServiceResponse_FUNCTION_REJECTED;
    
    // Associate a connection identifier with this task; this is used to
    // maintain the connection associated with a task when only task information
    // is available (e.g., in the case of a task timeout).
    *((UInt32*)GetHBADataPointer(parallelTask)) = 0;
    
    // Add the amount of data that we need to transfer to this connection
    OSAddAtomic64(GetRequestedDataTransferCount(parallelTask),&connection->dataToTransfer);

    // Build and set iSCSI initiator task tag
    UInt32 initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeSCSITask,LUN,taskId);
    SetControllerTaskIdentifier(parallelTask,initiatorTaskTag);
    
    DBLog("iscsi: Transfer size: %llu (sid: %d, cid: %d)\n",
          connection->dataToTransfer,session->sessionId,connection->cid);
    
    // Queue task in the event source (we'll remove it from the queue when were
    // done processing the task)
    connection->taskQueue->queueTask(initiatorTaskTag);
    
    DBLog("iscsi: Queued task %#x (sid: %d, cid: %d)\n",
          initiatorTaskTag,session->sessionId,connection->cid);

    return kSCSIServiceResponse_Request_In_Process;
}

void iSCSIVirtualHBA::BeginTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                                iSCSISession * session,
                                                iSCSIConnection * connection,
                                                UInt32 initiatorTaskTag)
{
    // Task tag corresponding to a connection timeout measurement
    if(owner->ParseInitiatorTaskTagForTaskType(initiatorTaskTag) == kInitiatorTaskTypeLatency)  {
        owner->MeasureConnectionLatency(session,connection);
        return;
    }
    
    // Grab parallel task associated with this iSCSI task
    SCSIParallelTaskIdentifier parallelTask =
        owner->FindTaskForControllerIdentifier(session->sessionId,initiatorTaskTag);
    
    if(!parallelTask)  {
        DBLog("iscsi: Task not found, flushing stream (BeginTaskOnWorkloopThread) (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        return;
    }
    
    // Extract information about this SCSI task
    SCSITaskAttribute attribute     = owner->GetTaskAttribute(parallelTask);
    UInt8   transferDirection       = owner->GetDataTransferDirection(parallelTask);
    UInt32  transferSize            = (UInt32)owner->GetRequestedDataTransferCount(parallelTask);
    UInt8   cdbSize                 = owner->GetCommandDescriptorBlockSize(parallelTask);
    
    DBLog("iscsi: Starting task %#x (sid: %d, cid: %d)\n",
          initiatorTaskTag,session->sessionId,connection->cid);
    
    // Timestamp the connection indicating when we started processing the task
    clock_get_system_microtime(&(connection->taskStartTimeSec),
                               &(connection->taskStartTimeUSec));
    
    iSCSIPDUSCSICmdBHS bhs  = iSCSIPDUSCSICmdBHSInit;
    bhs.dataTransferLength  = OSSwapHostToBigInt32(transferSize);
    
    owner->GetLogicalUnitBytes(parallelTask,(SCSILogicalUnitBytes*)&bhs.LUN);

    // The initiator task tag is just LUN and task identifier
    bhs.initiatorTaskTag = initiatorTaskTag;
    
    if(transferDirection == kSCSIDataTransfer_FromInitiatorToTarget)
        bhs.flags |= kiSCSIPDUSCSICmdFlagWrite;
    else if(transferDirection == kSCSIDataTransfer_FromTargetToInitiator)
        bhs.flags |= kiSCSIPDUSCSICmdFlagRead;
    
    // For CDB sizes less than 16 bytes, plug directly into SCSI command PDU
    // (Currently, OS X doesn't support CDB's larger than 16 bytes, so
    // there's no need for an iSCSI additional header segment for spillover).
    switch(cdbSize) {
        case kSCSICDBSize_6Byte:
        case kSCSICDBSize_10Byte:
        case kSCSICDBSize_12Byte:
        case kSCSICDBSize_16Byte:
            owner->GetCommandDescriptorBlock(parallelTask,&bhs.CDB);
            break;
    };
    
    // Setup the task attribute for this PDU
    switch(attribute) {
        case kSCSITask_ACA:
            bhs.flags |= kiSCSIPDUSCSICmdTaskAttrACA; break;
        case kSCSITask_HEAD_OF_QUEUE:
            bhs.flags |= kiSCSIPDUSCSICmdTaskAttrHead; break;
        case kSCSITask_ORDERED:
            bhs.flags |= kiSCSIPDUSCSICmdTaskAttrOrdered; break;
        case kSCSITask_SIMPLE:
            bhs.flags |= kiSCSIPDUSCSICmdTaskAttrSimple; break;
    };
    
    // Default timeout for new tasks...
    owner->SetTimeoutForTask(parallelTask,kiSCSITaskTimeoutMs);
    
    // For non-WRITE commands, send off SCSI command PDU immediately.
    if(transferDirection != kSCSIDataTransfer_FromInitiatorToTarget) {
        bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;
        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
        return;
    }
    
    // If there is no unsolicited data to send, simply send the WRITE
    // command and return.
    if(session->initialR2T && !session->immediateData) {
        bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;
        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
        return;
    }
    
    // At this point either immediate data, data-out PDUs or both
    // are going to be sent out.
    IOMemoryDescriptor  * dataDesc  = owner->GetDataBuffer(parallelTask);
    UInt32 dataOffset = 0, dataLength = 0;
    
    // First use immediate data to send data with command PDU...
    if(session->immediateData) {
        
        // Either send the max allowed data (immediate data length) or
        // all of the data if it is lesser than the max allowed limit
        dataLength = min(connection->immediateDataLength,transferSize);
        
        UInt8 * data = (UInt8*)IOMalloc(dataLength);
        dataDesc->readBytes(dataOffset,data,dataLength);
        
        // If we need to wait for an R2T or we've transferred all data
        // as immediate data then no additional data will follow this PDU...
        if(session->initialR2T || dataLength == transferSize)
            bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;

        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,data,dataLength);
        dataOffset += dataLength;
        
        owner->IncrementRealizedDataTransferCount(parallelTask,dataLength);
        connection->dataToTransfer -= dataLength;
        
        IOFree(data,dataLength);
    }
    else {
        // No immediate data (but there will be data-out following this)
        // just send the WRITE command without immediate data
        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    }

    // Follow up with data out PDUs up to the firstBurstLength bytes if...
    if(!session->initialR2T &&                     // Initial R2T = No
       dataOffset < session->firstBurstLength &&   // Haven't hit burst limit
       dataOffset < transferSize)                  // Data left to send
    {
        // Determine amount of data left to transfer and send data out PDUs
        dataLength = min(session->firstBurstLength-dataOffset,transferSize-dataOffset);
        
        owner->ProcessDataOutForTask(session,connection,parallelTask,dataOffset,dataLength,bhs.LUN,
                                     initiatorTaskTag,kiSCSIPDUTargetTransferTagReserved);
    }
}

bool iSCSIVirtualHBA::ProcessTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                                  iSCSISession * session,
                                                  iSCSIConnection * connection)
{
    // Quit if the connection isn't active (if it is not in full feature phase)
    if(!owner || !session || !connection)
        return true;
 
    // Grab incoming bhs (we are guaranteed to have a basic header at this
    // point (iSCSIIOEventSource ensures that this is the case)
    iSCSIPDUTargetBHS bhs;
    
    if(owner->RecvPDUHeader(session,connection,&bhs,0))
    {
        DBLog("iscsi: Failed to get PDU header (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        return true;
    }
    else
        DBLog("iscsi: Received PDU type %#x (sid: %d, cid: %d)\n",
              bhs.opCode,session->sessionId,connection->cid);

    // Determine the kind of PDU that was received and process accordingly
    enum iSCSIPDUTargetOpCodes opCode = (iSCSIPDUTargetOpCodes)bhs.opCode;
    switch(opCode)
    {
        // Process a SCSI response
        case kiSCSIPDUOpCodeSCSIRsp:
            owner->ProcessSCSIResponse(session,connection,(iSCSIPDUSCSIRspBHS*)&bhs);
        break;
            
        case kiSCSIPDUOpCodeDataIn:
            owner->ProcessDataIn(session,connection,(iSCSIPDUDataInBHS*)&bhs);
        break;
            
        case kiSCSIPDUOpCodeAsyncMsg:
            owner->ProcessAsyncMsg(session,connection,(iSCSIPDUAsyncMsgBHS*)&bhs);
            break;
            
        case kiSCSIPDUOpCodeNOPIn:
            owner->ProcessNOPIn(session,connection,(iSCSIPDUNOPInBHS*)&bhs);
            break;
            
        case kiSCSIPDUOpCodeR2T:
            owner->ProcessR2T(session,connection,(iSCSIPDUR2TBHS*)&bhs);
            break;
            
        case kiSCSIPDUOpCodeReject:
            owner->ProcessReject(session,connection,(iSCSIPDURejectBHS*)&bhs);
            break;
            
        case kiSCSIPDUOpCodeTaskMgmtRsp:
            owner->ProcessTaskMgmtRsp(session,connection,(iSCSIPDUTaskMgmtRspBHS*)&bhs);
            break;
            
        // Catch-all for anything else...
        default: break;
    };
    return true;
}

/** This function has been overloaded to provide additional task-timing
 *  support for multiple connections.
 *  @param parallelRequest the request to complete.
 *  @param completionStatus status of the request.
 *  @param serviceResponse the SCSI service response. */
void iSCSIVirtualHBA::CompleteParallelTask(iSCSISession * session,
                                           iSCSIConnection * connection,
                                           SCSIParallelTaskIdentifier parallelRequest,
                                           SCSITaskStatus completionStatus,
                                           SCSIServiceResponse serviceResponse)
{
    if(GetDataTransferDirection(parallelRequest) == kSCSIDataTransfer_NoDataTransfer) {
        super::CompleteParallelTask(parallelRequest,completionStatus,serviceResponse);
        return;
    }

    // Compute the time it took to complete this task; first grab the timestamp
    // when task was first started
    clock_usec_t usecs;
    clock_sec_t  secs;
    clock_get_system_microtime(&secs,&usecs);
    
    UInt64 duration_usecs = (secs  - connection->taskStartTimeSec)*1e6 +
                            (usecs - connection->taskStartTimeUSec);
    
    // Calculate transfer speed over entire task...
    UInt64 bytesTransferred = GetRequestedDataTransferCount(parallelRequest);

    // Add newest measurement to list (overwriting oldest one)
    connection->bytesPerSecondHistory[connection->bytesPerSecHistoryIdx]
        = (UInt32)(bytesTransferred / (duration_usecs / 1.0e6));
    
    // Advance index so next oldest record is overwritten next time (roll over)
    connection->bytesPerSecHistoryIdx++;
    if(connection->bytesPerSecHistoryIdx == connection->kBytesPerSecAvgWindowSize)
    {
        connection->bytesPerSecHistoryIdx = 0;
        
        // Queue a latency measurement operation
        UInt32 initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTypeLatency,0,0);
        connection->taskQueue->queueTask(initiatorTaskTag);
    }
    
    // Iterate over last few points, compute peak value
    connection->bytesPerSecond = 0;
    for(UInt8 i = 0; i < connection->kBytesPerSecAvgWindowSize; i++)
        if(connection->bytesPerSecond < connection->bytesPerSecondHistory[i])
            connection->bytesPerSecond = connection->bytesPerSecondHistory[i];
    
    DBLog("iscsi: Bytes per second: %d (sid: %d, cid: %d)\n",
          connection->bytesPerSecond,session->sessionId,connection->cid);

    super::CompleteParallelTask(parallelRequest,completionStatus,serviceResponse);
}

void iSCSIVirtualHBA::ProcessTaskMgmtRsp(iSCSISession * session,
                                         iSCSIConnection * connection,
                                         iSCSIPDU::iSCSIPDUTaskMgmtRspBHS * bhs)
{
    // Extract LUN and function code from task tag
    UInt8 taskMgmtFunction = ParseInitiatorTaskTagForTaskId(bhs->initiatorTaskTag);
    UInt64 LUN = ParseInitiatorTaskTagForLUN(bhs->initiatorTaskTag);
    
    // Setup the SCSI response code based on response from PDU
    SCSIServiceResponse serviceResponse;
    enum iSCSIPDUTaskMgmtRspCodes rspCode = (iSCSIPDUTaskMgmtRspCodes)bhs->response;
    
    switch(rspCode)
    {
        case kiSCSIPDUTaskMgmtFuncComplete:
            serviceResponse = kSCSIServiceResponse_TASK_COMPLETE;
        break;
        case kiSCSIPDUTaskMgmtFuncRejected:
            serviceResponse = kSCSIServiceResponse_FUNCTION_REJECTED;
        break;
        case kiSCSIPDUTaskMgmtInvalidLUN:
        case kiSCSIPDUTaskMgmtAuthFail:
        case kiSCSIPDUTaskMgmtFuncUnsupported:
        case kiSCSIPDUTaskMgmtInvalidTask:
        case kiSCSIPDUTaskMgmtReassignUnsupported:
        case kiSCSIPDUTaskMgmtTaskAllegiant:
        default:
            serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
        break;
    };

    // Tell the SCSI stack that the function completed or failed
    if(taskMgmtFunction == kiSCSIPDUTaskMgmtFuncAbortTask)
        CompleteAbortTask(session->sessionId, LUN, 0, serviceResponse);
    else if (taskMgmtFunction == kiSCSIPDUTaskMgmtFuncAbortTaskSet)
        CompleteAbortTaskSet(session->sessionId, LUN, serviceResponse);
    else if (taskMgmtFunction == kiSCSIPDUTaskMgmtFuncClearACA)
        CompleteClearACA(session->sessionId, LUN, serviceResponse);
    else if (taskMgmtFunction == kiSCSIPDUTaskMgmtFuncClearTaskSet)
        CompleteClearTaskSet(session->sessionId, LUN, serviceResponse);
    else if (taskMgmtFunction == kiSCSIPDUTaskMgmtFuncLUNReset)
        CompleteLogicalUnitReset(session->sessionId, LUN, serviceResponse);
    else if (taskMgmtFunction == kiSCSIPDUTaskMgmtFuncTargetWarmReset)
        CompleteTargetReset(session->sessionId, serviceResponse);
    
    // Task is complete, remove it from the queue
    connection->taskQueue->completeCurrentTask();
}

void iSCSIVirtualHBA::ProcessNOPIn(iSCSISession * session,
                                   iSCSIConnection * connection,
                                   iSCSIPDU::iSCSIPDUNOPInBHS * bhs)
{
    const size_t length = GetDataSegmentLength((iSCSIPDUTargetBHS*)bhs);
    
    // Grab data payload (could be ping data or other data, if it exists)
    UInt8 data[length];

    if(length > 0 && RecvPDUData(session,connection,data,length,MSG_WAITALL) != 0) {
        DBLog("iscsi: Failed to retreive NOP in data (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        return;
    }
    
    // Response to a previous ping from this initiator
    if(bhs->targetTransferTag == kiSCSIPDUTargetTransferTagReserved)
    {
        // Will use this to calculate latency; our initiated NOP contained
        // a timestamp that is sent back to us
        if(length != (sizeof(clock_sec_t) + sizeof(clock_usec_t)))
            return;
        
        clock_sec_t secs_stamp, secs;
        clock_usec_t usecs_stamp, usecs;
        
        // Grab timestamp from NOP-in PDU
        memcpy(&secs_stamp,data,sizeof(secs_stamp));
        memcpy(&usecs_stamp,data+sizeof(secs_stamp),sizeof(usecs_stamp));
        
        // Grab current system uptime
        clock_get_system_microtime(&secs,&usecs);
    
        connection->latency_ms = (secs - secs_stamp)*1e3 + (usecs - usecs_stamp)/1e3;
        
        DBLog("iscsi: Connection latency: %d ms (sid: %d, cid: %d)\n",
              connection->latency_ms,session->sessionId,connection->cid);
        
        // Remove latency measurement task from queue
        connection->taskQueue->completeCurrentTask();
    }
    // The target initiated this ping, just copy parameters and respond
    else {

        iSCSIPDUNOPOutBHS bhsRsp = iSCSIPDUNOPOutBHSInit;
        bhsRsp.opCode |= kiSCSIPDUImmediateDeliveryFlag;
        bhsRsp.LUN = bhs->LUN;
        bhsRsp.targetTransferTag = bhs->targetTransferTag;
        bhsRsp.initiatorTaskTag = kiSCSIPDUInitiatorTaskTagReserved;
        
        if(SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsRsp,NULL,data,length))
            DBLog("iscsi: Failed to send NOP response (sid: %d, cid: %d)\n",
                  session->sessionId,connection->cid);
    }
}

void iSCSIVirtualHBA::ProcessSCSIResponse(iSCSISession * session,
                                          iSCSIConnection * connection,
                                          iSCSIPDU::iSCSIPDUSCSIRspBHS * bhs)
{
    // Byte size of sense data (SAM)
    const UInt8 senseDataHeaderSize = 2;
    
    const UInt32 length = GetDataSegmentLength((iSCSIPDUTargetBHS*)bhs);
    UInt8 data[length];
    memset(data, length, 0);
    if(length > 0) {
        if(RecvPDUData(session,connection,data,length,MSG_WAITALL))
            DBLog("iscsi: Error retrieving data segment (sid: %d, cid: %d)\n",
                  session->sessionId,connection->cid);
        else
            DBLog("iscsi: Received sense data (sid: %d, cid: %d)\n",
                  session->sessionId,connection->cid);
    }

    // Grab parallel task associated with this PDU, indexed by task tag
    SCSIParallelTaskIdentifier parallelTask =
        FindTaskForControllerIdentifier(session->sessionId,bhs->initiatorTaskTag);
    
    if(!parallelTask)
    {
        DBLog("iscsi: Task not found, flushing stream (ProcessSCSIResponse) (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        
        // Flush stream
        UInt8 buffer[length];
        RecvPDUData(session,connection,buffer,length,MSG_WAITALL);
        
        return;
    }
    
    SetRealizedDataTransferCount(parallelTask,(UInt32)GetRequestedDataTransferCount(parallelTask));

    // Process sense data if the PDU came with any...
    bool senseDataPresent = false;
    if(length >= senseDataHeaderSize)
    {
        // First two bytes of the data segment are the size of the sense data
        UInt16 senseDataLength = *((UInt16*)&data[0]);
        senseDataLength = OSSwapBigToHostInt16(senseDataLength);
        
        if(length < senseDataLength + senseDataHeaderSize) {
            DBLog("iscsi: Received invalid sense data (sid: %d, cid: %d)\n",
                  session->sessionId,connection->cid);
        }
        else {
        
            // Remaining data is sense data, advance pointer by two bytes to get this
            SCSI_Sense_Data * newSenseData = (SCSI_Sense_Data *)(data + senseDataHeaderSize);
        
            // Incorporate sense data into the task
            SetAutoSenseData(parallelTask,newSenseData,senseDataLength);
            
            senseDataPresent = true;
            
            DBLog("iscsi: Processed sense data (sid: %d, cid: %d)\n",
                  session->sessionId,connection->cid);
        }
    }
    
    // Set the SCSI completion status and service response, let SCSI stack
    // know that we're done with this task...
    
    SCSITaskStatus completionStatus = (SCSITaskStatus)bhs->status;
    
    // If sense data has been included along with a check condition response,
    // the macOS SCSI stack expects that the task status is "GOOD". Otherwise,
    // it queries for auto sense data.
    if(completionStatus == kSCSITaskStatus_CHECK_CONDITION && senseDataPresent)
        completionStatus = kSCSITaskStatus_GOOD;
    
    SCSIServiceResponse serviceResponse;

    if(bhs->response == kiSCSIPDUSCSICmdCompleted)
        serviceResponse = kSCSIServiceResponse_TASK_COMPLETE;
    else
        serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    CompleteParallelTask(session,connection,parallelTask,completionStatus,serviceResponse);
    
    // Task is complete, remove it from the queue
    connection->taskQueue->completeCurrentTask();
    
    DBLog("iscsi: Processed SCSI response (sid: %d, cid: %d)\n",
          session->sessionId,connection->cid);

}

void iSCSIVirtualHBA::ProcessDataIn(iSCSISession * session,
                                    iSCSIConnection * connection,
                                    iSCSIPDU::iSCSIPDUDataInBHS * bhs)
{
    const UInt32 length = GetDataSegmentLength((iSCSIPDUTargetBHS*)bhs);

    // Grab parallel task associated with this PDU, indexed by task tag
    SCSIParallelTaskIdentifier parallelTask =
        FindTaskForControllerIdentifier(session->sessionId,bhs->initiatorTaskTag);
    
    if(length == 0)
    {
        DBLog("iscsi: Missing data segment in data-in PDU (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        return;
    }
    
    UInt8 buffer[length];
    
    // If task not found, flush stream
    if(!parallelTask)
    {
        DBLog("iscsi: Task not found (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        RecvPDUData(session,connection,buffer,length,MSG_WAITALL);
        return;
    }
    
    // System buffer offset for this PDU data segment...
    UInt32 dataOffset = OSSwapBigToHostInt32(bhs->bufferOffset);
    
    if(RecvPDUData(session,connection,buffer,length,0))
        DBLog("iscsi: Error in retrieving data segment length (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
    else {
        IOMemoryDescriptor  * dataDesc = GetDataBuffer(parallelTask);
        dataDesc->writeBytes(dataOffset,buffer,length);
        SetRealizedDataTransferCount(parallelTask,dataOffset+length);
        connection->dataToTransfer -= length;
    }
    
    // If the PDU contains a status response, complete this task
    if((bhs->flags & kiSCSIPDUDataInFinalFlag) && (bhs->flags & kiSCSIPDUDataInStatusFlag))
    {
        SetRealizedDataTransferCount(parallelTask,(UInt32)GetRequestedDataTransferCount(parallelTask));
        
        CompleteParallelTask(session,
                             connection,
                             parallelTask,
                             (SCSITaskStatus)bhs->status,
                             kSCSIServiceResponse_TASK_COMPLETE);
        
        // Task is complete, remove it from the queue
        connection->taskQueue->completeCurrentTask();
        
        DBLog("iscsi: Processed data-in PDU (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
    }
    
    // Send acknowledgement to target if one is required
    if(bhs->flags & kiSCSIPDUDataInAckFlag)
    {
//TODO: error recovery
    }
}

/*! Process an incoming asynchronous message PDU.
 *  @param session the session associated with the async PDU.
 *  @param connection the connection associated with the async PDU.
 *  @param bhs the basic header segment of the async PDU. */
void iSCSIVirtualHBA::ProcessAsyncMsg(iSCSISession * session,
                                      iSCSIConnection * connection,
                                      iSCSIPDU::iSCSIPDUAsyncMsgBHS * bhs)
{
    // Grab any data associated with the PDU (e.g., sense data for SCSI
    // asynchronous message)
    const UInt32 length = GetDataSegmentLength((iSCSIPDUTargetBHS *)bhs);
    UInt8 * data = NULL;
    
    if(length) {
        if(!(data = (UInt8*)IOMalloc(length))) {
            DBLog("iscsi: couldn't allocate memory for PDU data (ProcessAsyncMsg)\n");
            return;
        }
        RecvPDUData(session,connection,data,length,MSG_WAITALL);
    }

    iSCSIPDUAsyncMsgEvent asyncEvent = (iSCSIPDUAsyncMsgEvent)(bhs->asyncEvent);
    
    DBLog("iscsi: Async Message (code %#x) received (sid: %d, cid: %d)\n",
        asyncEvent,session->sessionId,connection->cid);
    
    iSCSIHBAUserClient * client = (iSCSIHBAUserClient*)getClient();
    if(!client) {
        DBLog("iscsi: client is not available; async message may not be processed correctly\n");
    }

    switch(asyncEvent)
    {
        // The target will drop all connections for this session
        case kiSCSIPDUAsyncMsgDropAllConnections:
            ReleaseSession(session->sessionId);
            break;

        // The target will drop the specified connection
        case kiSCSIPDUAsynMsgDropConnection:
            ReleaseConnection(session->sessionId,connection->cid);
            break;
            
        case kiSCSIPDUAsyncMsgLogout:
            DeactivateConnection(session->sessionId,connection->cid);
            break;
            
        // Target requests parameter negotiation
        case kiSCSIPDUAsyncMsgNegotiateParams:
            DeactivateConnection(session->sessionId,connection->cid);
            break;
            
        // No support for asynchronous SCSI messages; do nothing
        case kiSCSIPDUAsyncMsgSCSIAsyncMsg: break;
            
        // No support for proprietary vendor codes; do nothing
        case kiSCSIPDUAsyncMsgVendorCode: break;

        default: break;
    };
    
    // Only send out a notification to the daemon if the
    // message is not vendor-specific or a SCSI message.
    if(asyncEvent != kiSCSIPDUAsyncMsgSCSIAsyncMsg && asyncEvent != kiSCSIPDUAsyncMsgVendorCode)
        client->sendAsyncMessageNotification(session->sessionId,connection->cid,asyncEvent);

    if(data)
        IOFree(data,length);
}

/*! Process an incoming R2T PDU.
 *  @param session the session associated with the R2T PDU.
 *  @param connection the connection associated with the R2T PDU.
 *  @param bhs the basic header segment of the R2T PDU. */
void iSCSIVirtualHBA::ProcessR2T(iSCSISession * session,
                                 iSCSIConnection * connection,
                                 iSCSIPDU::iSCSIPDUR2TBHS * bhs)
{
    // Grab parallel task associated with this PDU, indexed by task tag
    SCSIParallelTaskIdentifier parallelTask =
        FindTaskForControllerIdentifier(session->sessionId,bhs->initiatorTaskTag);
    
    if(!parallelTask)
    {
        DBLog("iscsi: Couldn't find requested task to process (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        return;
    }
    
    // Obtain requested data offset and requested lengths
    UInt32 dataOffset = OSSwapBigToHostInt32(bhs->bufferOffset);
    UInt32 dataLength = OSSwapBigToHostInt32(bhs->desiredDataLength);
    
    ProcessDataOutForTask(session,connection,parallelTask,dataOffset,dataLength,
                          bhs->LUN,bhs->initiatorTaskTag,bhs->targetTransferTag);

}

void iSCSIVirtualHBA::ProcessDataOutForTask(iSCSISession * session,
                                            iSCSIConnection * connection,
                                            SCSIParallelTaskIdentifier parallelTask,
                                            UInt32 dataOffset,
                                            UInt32 dataLength,
                                            UInt64 LUN,
                                            UInt32 initiatorTaskTag,
                                            UInt32 targetTransferTag)
{
    // Keep track of data to transfer each PDU and sequence number
    UInt32 dataSegmentLength = connection->maxSendDataSegmentLength;
    UInt32 dataSN = 0;
    
    DBLog("iscsi: Dataoffset: %d (sid: %d, cid: %d)\n",dataOffset,session->sessionId,connection->cid);
    DBLog("iscsi: Desired data length: %d (sid: %d, cid: %d)\n",dataLength,session->sessionId,connection->cid);
    
    // Create data PDUs and send them until all desired data has been sent
    iSCSIPDUDataOutBHS bhsDataOut = iSCSIPDUDataOutBHSInit;
    bhsDataOut.LUN              = LUN;
    bhsDataOut.initiatorTaskTag = initiatorTaskTag;
    bhsDataOut.targetTransferTag = targetTransferTag;

    // Get descriptor to data to be sent; create buffer for use with each PDU
    IOMemoryDescriptor  * dataDesc   = GetDataBuffer(parallelTask);
    UInt8 * data = (UInt8*)IOMalloc(connection->maxSendDataSegmentLength);
    
    // The amount of data that needs to be transferred...
    while(dataLength != 0)
    {
        bhsDataOut.bufferOffset = OSSwapHostToBigInt32(dataOffset);
        bhsDataOut.dataSN = OSSwapHostToBigInt32(dataSN);
        
        // Special case for the final PDU
        if(dataLength <= dataSegmentLength)
        {
            dataSegmentLength = dataLength;
            bhsDataOut.flags = kiSCSIPDUDataOutFinalFlag;
        }
        
        dataDesc->readBytes(dataOffset,data,dataSegmentLength);
        errno_t error = SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsDataOut,
                                NULL,data,dataSegmentLength);
        
        if(error) {
            DBLog("iscsi: Send error: %d (sid: %d, cid: %d)\n",error,session->sessionId,connection->cid);
            break;
        }
        
        dataLength -= dataSegmentLength;
        dataOffset += dataSegmentLength;
        
        // Update driver stack & connection with amount transferred
        IncrementRealizedDataTransferCount(parallelTask,dataSegmentLength);
        connection->dataToTransfer -= dataSegmentLength;

        // Increment the data sequence number
        dataSN++;
    }
    
    // Cleanup buffer
    IOFree(data,connection->maxSendDataSegmentLength);
}

/*! Process an incoming reject PDU.
 *  @param session the session associated with the R2T PDU.
 *  @param connection the connection associated with the R2T PDU.
 *  @param bhs the basic header segment of the reject PDU. */
void iSCSIVirtualHBA::ProcessReject(iSCSISession * session,
                                    iSCSIConnection * connection,
                                    iSCSIPDU::iSCSIPDURejectBHS * bhs)
{
    const UInt32 length = GetDataSegmentLength((iSCSIPDUTargetBHS*)bhs);
    
    if(length == 0)
    {
        DBLog("iscsi: Missing data segment in data-in PDU (sid: %d, cid: %d)\n",
              session->sessionId,connection->cid);
        return;
    }
    
    UInt8 buffer[length];
    RecvPDUData(session,connection,buffer,length,MSG_WAITALL);
    
    enum iSCSIPDURejectCode rejectCode = (enum iSCSIPDURejectCode)bhs->reason;
    
    switch(rejectCode) {
        case kiSCSIPDURejectReserved: break;
        case kiSCSIPDURejectCmdNotSupported: break;
        case kiSCSIPDURejectDataDigestError: break;
        case kiSCSIPDURejectInvalidDataACK: break;
        case kiSCSIPDURejectInvalidPDUField: break;
        case kiSCSIPDURejectLongOperationReject: break;
        case kiSCSIPDURejectNegotiationReset: break;
        case kiSCSIPDURejectProtoError: break;
        case kiSCSIPDURejectSNACKReject: break;
        case kiSCSIPDURejectTaskInProgress: break;
        case kiSCSIPDURejectTooManyImmediateCmds: break;
        case kiSCSIPDURejectWaitingForLogout: break;
        default: break;
    };
}

/*! Measures the latency of a connection (the iSCSI latency).  This is achieved
 *  by sending out a NOP PDU with a timestamp.  Once the NOP bounces back from
 *  the peer the timestamp is compared to the current system time to determine
 *  the latency.
 *  @param session the session associated with the connection to measure.
 *  @param connection the connection to measure. */
void iSCSIVirtualHBA::MeasureConnectionLatency(iSCSISession * session,
                                               iSCSIConnection * connection)
{
    // Setup a NOP out PDU (LUN field is unused with a value of 0 and the target
    // transfer tag takes on the reserved value fo this type of NOP out)
    iSCSIPDUNOPOutBHS bhs = iSCSIPDUNOPOutBHSInit;
    bhs.targetTransferTag = kiSCSIPDUTargetTransferTagReserved;
    bhs.initiatorTaskTag  = 0;
    
    // Calculate current uptime and send it to the target with this NOP out.
    // The target will echo the value and this allows us to estimate the
    // overall latency of the iSCSI stack and TCP connection
    clock_sec_t secs = 0;
    clock_usec_t usecs = 0;
    clock_get_system_microtime(&secs,&usecs);
    
    const size_t length = (sizeof(clock_sec_t) + sizeof(clock_usec_t));
    UInt8 data[length];
    memcpy(data,&secs,sizeof(clock_sec_t));
    memcpy(data+sizeof(clock_sec_t),&usecs,sizeof(clock_usec_t));
    
    SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhs,NULL,data,length);
}


//////////////////////////////// iSCSI FUNCTIONS ///////////////////////////////

/*! Allocates a new iSCSI session and returns a session qualifier ID.
 *  @param targetIQN the name of the target, or NULL if discovery session.
 *  @param portalAddress the IPv4/IPv6 or hostname of the portal.
 *  @param portalPort the TCP port used for the connection.
 *  @param hostInterface the host interface to use for the connection.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host adapter.
 *  @param sessionId identifier for the new session.
 *  @param connectionId identifier for the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::CreateSession(OSString * targetIQN,
                                       OSString * portalAddress,
                                       OSString * portalPort,
                                       OSString * hostInterface,
                                       const struct sockaddr_storage * portalSockaddr,
                                       const struct sockaddr_storage * hostSockaddr,
                                       SessionIdentifier * sessionId,
                                       ConnectionIdentifier * connectionId)
{
    // Validate inputs
    if(!portalSockaddr || !hostSockaddr || !sessionId || !connectionId)
        return EINVAL;

    // Default session and connection Ids
    *sessionId = kiSCSIInvalidSessionId;
    *connectionId = kiSCSIInvalidConnectionId;

    // Initialize default error (try again)
    errno_t error = EAGAIN;
    
    // Find an open session slot
    SessionIdentifier sessionIdx;
    for(sessionIdx = 0; sessionIdx < kMaxSessions; sessionIdx++)
        if(!sessionList[sessionIdx])
            break;
    
    // If no slots were available tell user to try again later...
    if(sessionIdx == kMaxSessions)
        goto SESSION_ID_ALLOC_FAILURE;

    // Alloc new session, validate
    iSCSISession * newSession;
    
    if(!(newSession = (iSCSISession*)IOMalloc(sizeof(iSCSISession))))
        goto SESSION_ALLOC_FAILURE;

    // Setup connections array for new session
    newSession->connections = (iSCSIConnection **)IOMalloc(kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    
    if(!newSession->connections)
        goto SESSION_CONNECTION_LIST_ALLOC_FAILURE;
    
    // Reset all connections
    memset(newSession->connections,0,kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    
    // Setup session parameters with defaults
    newSession->sessionId = sessionIdx;
    newSession->numActiveConnections = 0;
    newSession->active = false;
    newSession->cmdSN = 0;
    newSession->expCmdSN = 0;
    newSession->maxCmdSN = 0;
    
    newSession->targetPortalGroupTag = 0;
    newSession->targetSessionId = 0;
    
    newSession->dataPDUInOrder = kRFC3720_DataPDUInOrder;
    newSession->dataSequenceInOrder = kRFC3720_DataSequenceInOrder;
    newSession->defaultTime2Retain = kRFC3720_DefaultTime2Retain;
    newSession->defaultTime2Wait = kRFC3720_DefaultTime2Wait;
    newSession->errorRecoveryLevel = kRFC3720_ErrorRecoveryLevel;
    newSession->firstBurstLength = kRFC3720_FirstBurstLength;
    newSession->immediateData = kRFC3720_ImmediateData;
    newSession->initialR2T = kRFC3720_InitialR2T;
    newSession->maxBurstLength = kRFC3720_MaxBurstLength;
    newSession->maxConnections = kRFC3720_MaxConnections;
    newSession->maxOutStandingR2T = kRFC3720_MaxOutstandingR2T;
    
    // Retain new session
    sessionList[sessionIdx] = newSession;
    *sessionId = sessionIdx;

    // Add target to lookup table...
    targetList->setObject(targetIQN->getCStringNoCopy(),OSNumber::withNumber(sessionIdx,sizeof(sessionIdx)*8));

    // Create a connection associated with this session
    if((error = CreateConnection(*sessionId,portalAddress,portalPort,hostInterface,
                                 portalSockaddr,hostSockaddr,connectionId)))
        goto SESSION_CREATE_CONNECTION_FAILURE;

    // Success
    return 0;
    
SESSION_CREATE_CONNECTION_FAILURE:

    // Remove target from lookup table
    targetList->removeObject(targetIQN);
    IOFree(newSession->connections,kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    sessionList[sessionIdx] = nullptr;
    *sessionId = kiSCSIInvalidSessionId;
 
SESSION_CONNECTION_LIST_ALLOC_FAILURE:
    IOFree(newSession,sizeof(iSCSISession));
   
SESSION_ALLOC_FAILURE:

SESSION_ID_ALLOC_FAILURE:
    
    return error;
}

/*! Releases all iSCSI sessions. */
void iSCSIVirtualHBA::ReleaseAllSessions()
{
    // Go through every connection for each session, and close sockets,
    // remove event sources, etc
    for(SessionIdentifier index = 0; index < kMaxSessions; index++)
    {
        if(!sessionList[index])
            continue;
        
        ReleaseSession(index);
    }
}

/*! Releases an iSCSI session, including all connections associated with that
 *  session.
 *  @param sessionId the session qualifier part of the ISID. */
void iSCSIVirtualHBA::ReleaseSession(SessionIdentifier sessionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions)
        return;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return;
    
    DBLog("iscsi: Releasing session (sid %d)\n",sessionId);
    
    // Disconnect all connections
    for(ConnectionIdentifier connectionId = 0; connectionId < kMaxConnectionsPerSession; connectionId++)
    {
        if(theSession->connections[connectionId])
            ReleaseConnection(sessionId,connectionId);
    }
    
    // Prevent others from accessing the session
    sessionList[sessionId] = NULL;
    
    // Free connection list and session object
    IOFree(theSession->connections,kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    IOFree(theSession,sizeof(iSCSISession));
    
    // Remove target name from dictionary
    OSCollectionIterator * iter = OSCollectionIterator::withCollection(targetList);
    OSString * targetIQN;
    
    while((targetIQN = (OSString *)iter->getNextObject()))
    {
        OSNumber * targetId = (OSNumber*)targetList->getObject(targetIQN);
        if(targetId->unsigned16BitValue() == sessionId)
        {
            targetList->removeObject(targetIQN);
            break;
        }
    }
}

/*! Allocates a new iSCSI connection associated with the particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param portalAddress the IPv4/IPv6 or hostname of the portal.
 *  @param portalPort the TCP port used for the connection.
 *  @param hostInterface the host interface to use for the connection.
 *  @param portalSockaddr the BSD socket structure used to identify the target.
 *  @param hostSockaddr the BSD socket structure used to identify the host adapter.
 *  @param connectionId identifier for the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::CreateConnection(SessionIdentifier sessionId,
                                          OSString * portalAddress,
                                          OSString * portalPort,
                                          OSString * hostInterface,
                                          const struct sockaddr_storage * portalSockaddr,
                                          const struct sockaddr_storage * hostSockaddr,
                                          ConnectionIdentifier * connectionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || !portalSockaddr || !hostSockaddr || !connectionId)
        return EINVAL;
    
    // Retrieve the session from the session list, validate
    iSCSISession * session = sessionList[sessionId];
    if(!session)
        return EINVAL;
    
    // Find an empty connection slot to use for a new connection
    ConnectionIdentifier index;
    for(index = 0; index < kMaxConnectionsPerSession; index++)
        if(!session->connections[index])
            break;
    
    // If empty slot wasn't found tell caller to try again later
    if(index == kMaxConnectionsPerSession)
        return EAGAIN;

    // Create a new connection
    iSCSIConnection * newConn = (iSCSIConnection*)IOMalloc(sizeof(iSCSIConnection));
    if(!newConn)
        return EAGAIN;

    newConn->expStatSN = 0;
    newConn->dataToTransfer = 0;
    newConn->bytesPerSecond = 0;
    newConn->cid = index;
    
    newConn->maxRecvDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    newConn->maxSendDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    newConn->useDataDigest = kRFC3720_DataDigest;
    newConn->useHeaderDigest = kRFC3720_HeaderDigest;
    newConn->useOFMarker = kRFC3720_OFMarker;
    newConn->useIFMarker = kRFC3720_IFMarker;
    newConn->OFMarkInt = kRFC3720_OFMarkInt;
    newConn->IFMarkInt = kRFC3720_IFMarkInt;
    
    session->connections[index] = newConn;
    *connectionId = index;
    
    // Initialize default error (try again)
    errno_t error = EAGAIN;

    if(!(newConn->taskQueue = OSTypeAlloc(iSCSITaskQueue)))
        goto TASKQUEUE_ALLOC_FAILURE;
    
    // Initialize event source, quit if it fails
    if(!newConn->taskQueue->init(this,(iSCSITaskQueue::Action)&BeginTaskOnWorkloopThread,session,newConn))
        goto TASKQUEUE_INIT_FAILURE;
    
    if(GetWorkLoop()->addEventSource(newConn->taskQueue) != kIOReturnSuccess)
        goto TASKQUEUE_ADD_FAILURE;
    
    newConn->taskQueue->disable();
    
    if(!(newConn->dataRecvEventSource = OSTypeAlloc(iSCSIIOEventSource)))
        goto EVENTSOURCE_ALLOC_FAILURE;
    
    // Initialize event source, quit if it fails
    if(!newConn->dataRecvEventSource->init(this,(iSCSIIOEventSource::Action)&ProcessTaskOnWorkloopThread,session,newConn))
        goto EVENTSOURCE_INIT_FAILURE;
    
    if(GetWorkLoop()->addEventSource(newConn->dataRecvEventSource) != kIOReturnSuccess)
        goto EVENTSOURCE_ADD_FAILURE;
    
    newConn->dataRecvEventSource->disable();
    
    // Create a new socket (per RFC3720, only TCP sockets are used.
    // Domain can be either IPv4 or IPv6.
    error = sock_socket(portalSockaddr->ss_family,
                        SOCK_STREAM,IPPROTO_TCP,
                        (sock_upcall)&iSCSIIOEventSource::socketCallback,
                        newConn->dataRecvEventSource,
                        &newConn->socket);
    if(error)
        goto SOCKET_CREATE_FAILURE;

    // Set send and receive timeouts for the socket
    struct timeval timeout;
    timeout.tv_sec = kiSCSITCPTimeoutSec;
    timeout.tv_usec = 0;

    // Set connection timeout...
    sock_setsockopt(newConn->socket,IPPROTO_TCP,TCP_CONNECTIONTIMEOUT,(const void*)&timeout,sizeof(struct timeval));

    // Bind socket to a particular host connection
    if((error = sock_bind(newConn->socket,(sockaddr*)hostSockaddr)))
        goto SOCKET_BIND_FAILURE;

    // Connect the socket to the target node
    if((error = sock_connect(newConn->socket,(sockaddr*)portalSockaddr,0)))
        goto SOCKET_CONNECT_FAILURE;

    // Set socket to non-blocking & set timeouts
    sock_setsockopt(newConn->socket,SOL_SOCKET,SO_SNDTIMEO,(const void*)&timeout,sizeof(struct timeval));
    sock_setsockopt(newConn->socket,SOL_SOCKET,SO_RCVTIMEO,(const void*)&timeout,sizeof(struct timeval));

    // Initialize queue that keeps track of connection speed
    memset(newConn->bytesPerSecondHistory,0,sizeof(UInt8)*newConn->kBytesPerSecAvgWindowSize);
    newConn->bytesPerSecHistoryIdx = 0;

    newConn->portalAddress = portalAddress;
    newConn->portalPort = portalPort;
    newConn->hostInteface = hostInterface;
    portalAddress->retain();
    portalPort->retain();
    hostInterface->retain();

    return 0;
    
SOCKET_CONNECT_FAILURE:
    
SOCKET_BIND_FAILURE:
    sock_close(newConn->socket);
    
SOCKET_CREATE_FAILURE:
    GetWorkLoop()->removeEventSource(newConn->dataRecvEventSource);
    
EVENTSOURCE_ADD_FAILURE:
    
EVENTSOURCE_INIT_FAILURE:
    newConn->dataRecvEventSource->release();
    
EVENTSOURCE_ALLOC_FAILURE:
    GetWorkLoop()->removeEventSource(newConn->taskQueue);
    
TASKQUEUE_ADD_FAILURE:
    
TASKQUEUE_INIT_FAILURE:
    newConn->taskQueue->release();
    
TASKQUEUE_ALLOC_FAILURE:

    session->connections[index] = 0;
    IOFree(newConn,sizeof(iSCSIConnection));
    
    return error;
}

/*! Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
void iSCSIVirtualHBA::ReleaseConnection(SessionIdentifier sessionId,
                                        ConnectionIdentifier connectionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = sessionList[sessionId];
    
    if(!session)
        return;

    iSCSIConnection * connection = session->connections[connectionId];
        
    if(!connection)
        return;
    
    // First deactivate connection before proceeding
    if(connection->taskQueue->isEnabled())
        DeactivateConnection(sessionId,connectionId);

    // Prevents other from trying to access this connection...
    session->connections[connectionId] = NULL;
    
    sock_close(connection->socket);

    GetWorkLoop()->removeEventSource(connection->dataRecvEventSource);
    GetWorkLoop()->removeEventSource(connection->taskQueue);
    
    DBLog("iscsi: Removed event sources (sid: %d, cid: %d)\n",sessionId,connectionId);
    
    connection->dataRecvEventSource->release();
    connection->taskQueue->release();
    connection->dataToTransfer = 0;
    
    IOFree(connection,sizeof(iSCSIConnection));
    
    DBLog("iscsi: Released connection (sid: %d, cid: %d)\n",sessionId,connectionId);
}

/*! Activates an iSCSI connection, indicating to the kernel that the iSCSI
 *  daemon has negotiated security and operational parameters and that the
 *  connection is in the full-feature phase.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::ActivateConnection(SessionIdentifier sessionId,ConnectionIdentifier connectionId)
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = sessionList[sessionId];
    
    if(!session)
        return EINVAL;
    
    // Do nothing if connection doesn't exist
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return EINVAL;
    
    // Set the maximum amount of immediate data we can send on this connection
    connection->immediateDataLength = min(connection->maxSendDataSegmentLength,
                                          session->firstBurstLength);
    
    connection->taskQueue->enable();
    connection->dataRecvEventSource->enable();
    
    // If this is the first active connection, mount the target
    if(session->numActiveConnections == 0) {
        if(!CreateTargetForID(sessionId))
        {
            connection->taskQueue->disable();
            connection->dataRecvEventSource->disable();
            return EAGAIN;
        }
    }

    OSIncrementAtomic(&session->numActiveConnections);

    return 0;
}

/*! Activates all iSCSI connections for the session, indicating to the
 *  kernel that the iSCSI daemon has negotiated security and operational
 *  parameters and that the connection is in the full-feature phase.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::ActivateAllConnections(SessionIdentifier sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    errno_t error = 0;
    for(ConnectionIdentifier connectionId = 0; connectionId < kMaxConnectionsPerSession; connectionId++)
        if((error = ActivateConnection(sessionId,connectionId)))
            return error;
    
    return 0;
}

/*! Deactivates an iSCSI connection so that parameters can be adjusted or
 *  negotiated by the iSCSI daemon.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::DeactivateConnection(SessionIdentifier sessionId,ConnectionIdentifier connectionId)
{
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = sessionList[sessionId];
    
    if(!session)
        return EINVAL;
    
    // Do nothing if connection doesn't exist
    iSCSIConnection * connection = session->connections[connectionId];
    
    if(!connection)
        return EINVAL;

    connection->dataRecvEventSource->disable();
    connection->taskQueue->disable();
    
    // Tell driver stack that tasks have been rejected (stack will reattempt
    // the task on a different connection, if one is available)
    UInt32 initiatorTaskTag = 0;
    SCSIParallelTaskIdentifier task;
 
    while((initiatorTaskTag = connection->taskQueue->completeCurrentTask()) != 0)
    {
        task = FindTaskForControllerIdentifier(sessionId, initiatorTaskTag);
        if(!task)
            continue;
        
        // Notify the SCSI driver stack that we couldn't finish these tasks
        // on this connection
        CompleteParallelTask(session,
                             connection,
                             task,
                             kSCSITaskStatus_DeliveryFailure,
                             kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE);
    }

    OSDecrementAtomic(&session->numActiveConnections);
    
    // If this is the last active connection, un-mount the target
    if(session->numActiveConnections == 0)
        DestroyTargetForID(sessionId);
    
    DBLog("iscsi: Deactivated connection (sid: %d, cid: %d)\n",sessionId,connectionId);
    
    return 0;
}

/*! Deactivates all iSCSI connections so that parameters can be adjusted or
 *  negotiated by the iSCSI daemon.
 *  @param sessionId the session to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::DeactivateAllConnections(SessionIdentifier sessionId)
{
    if(sessionId >= kMaxSessions)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = sessionList[sessionId];
    
    if(!session)
        return EINVAL;
    
    errno_t error = 0;
    for(ConnectionIdentifier connectionId = 0; connectionId < kMaxConnectionsPerSession; connectionId++)
    {
        if(session->connections[connectionId])
        {
            if((error = DeactivateConnection(sessionId,connectionId)))
                return error;
        }
    }
    
    return 0;
}

/*! Sends data over a kernel socket associated with iSCSI.  If the specified
 *  data segment length is not a multiple of 4-bytes, padding bytes will be 
 *  added to the data segment of the PDU per RF3720 specification.
 *  This function will automatically calculate the data segment length
 *  field of the PDU and place it in the header using the correct byte order.
 *  It will also assign a command sequence number and expected status sequence
 *  number using values from the session and connection objects to the PDU
 *  header in the correct (network) byte order.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment to send.
 *  @param ahs the additional header segments, if any
 *  @param data the data segment to send.
 *  @param length the byte size of the data segment
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::SendPDU(iSCSISession * session,
                                 iSCSIConnection * connection,
                                 iSCSIPDUInitiatorBHS * bhs,
                                 iSCSIPDUCommonAHS * ahs,
                                 const void * data,
                                 size_t length)
{
    // Range-check inputs
    if(!session || !connection || !bhs)
        return EINVAL;
    
    // Set the command sequence number & expected status sequence number
    if(bhs->opCodeAndDeliveryMarker != kiSCSIPDUOpCodeDataOut) {
        bhs->cmdSN = OSSwapHostToBigInt32(session->cmdSN);
        
        // Advance cmdSN if PDU is not marked for immediate delivery
        if(!(bhs->opCodeAndDeliveryMarker & kiSCSIPDUImmediateDeliveryFlag))
            OSIncrementAtomic(&session->cmdSN);
    }
    
    bhs->expStatSN = OSSwapHostToBigInt32(connection->expStatSN);
    
    SetDataSegmentLength((iSCSIPDUInitiatorBHS*)bhs,(UInt32)length);

    // Send data over the network, return true if all bytes were sent
    struct msghdr msg;
    struct iovec  iovec[5];
    memset(&msg,0,sizeof(struct msghdr));
    
    msg.msg_iov = iovec;
    unsigned int iovecCnt = 0;
    
    // Set basic header segment
    iovec[iovecCnt].iov_base  = bhs;
    iovec[iovecCnt].iov_len   = kiSCSIPDUBasicHeaderSegmentSize;
    iovecCnt++;

    DBLog("iscsi: Sent PDU type %#x (sid: %d, cid: %d)\n",
          bhs->opCodeAndDeliveryMarker,session->sessionId,connection->cid);
    
    // Leave room for a header digest
    if(connection->useHeaderDigest)    {
        UInt32 headerDigest;
        
        // Compute digest
        headerDigest = crc32c(0,bhs,kiSCSIPDUBasicHeaderSegmentSize);
        DBLog("iscsi: Header digest: %#x\n",headerDigest);
        
        iovec[iovecCnt].iov_base = &headerDigest;
        iovec[iovecCnt].iov_len  = sizeof(headerDigest);
        iovecCnt++;
    }
    
    // If theres data to send...
    UInt32 padding = 0;
    
    if(data && length)
    {
        // Add data segment
        iovec[iovecCnt].iov_base = (void*)data;
        iovec[iovecCnt].iov_len  = length;
        iovecCnt++;
  
        // Add padding bytes if required
        UInt32 paddingLen = 4-(length % 4);
        if(paddingLen != 4)
        {
            iovec[iovecCnt].iov_base  = &padding;
            iovec[iovecCnt].iov_len   = paddingLen;
            iovecCnt++;
        }

        DBLog("iscsi: Sending data length: %zu\n",length);

        // Leave room for a data digest
        if(connection->useDataDigest) {
            UInt32 dataDigest;
            
            // Compute digest
            dataDigest = crc32c(0,data,length);
            
            DBLog("iscsi: Data digest: %#x\n",dataDigest);
            
            iovec[iovecCnt].iov_base = &dataDigest;
            iovec[iovecCnt].iov_len  = sizeof(dataDigest);
            iovecCnt++;
        }
    }
    
    // Update io vector count, send data
    msg.msg_iovlen = iovecCnt;
    size_t bytesSent = 0;
    errno_t error;
    
    if((error = sock_send(connection->socket,&msg,0,&bytesSent)))
    {
        DBLog("iscsi: sock_send error returned with code %d (sid: %d, cid: %d)\n",error,session->sessionId,connection->cid);
        HandleConnectionTimeout(session->sessionId,connection->cid);
        return error;
    }
    
    return error;
}


/*! Gets whether a PDU is available for receiption on a particular
 *  connection.
 *  @param the connection to check.
 *  @return true if a PDU is available, false otherwise. */
bool iSCSIVirtualHBA::isPDUAvailable(iSCSIConnection * connection)
{
    int bytesAtSocket;
    sock_ioctl(connection->socket,FIONREAD,&bytesAtSocket);

    // Guarantee that the data equal to a basic header segment is available
    return bytesAtSocket >= kiSCSIPDUBasicHeaderSegmentSize;
}


/*! Receives a basic header segment over a kernel socket.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received.
 *  @param flags optional flags to be passed onto sock_recv.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::RecvPDUHeader(iSCSISession * session,
                                       iSCSIConnection * connection,
                                       iSCSIPDUTargetBHS * bhs,
                                       int flags)
{
    // Range-check inputs
    if(!session || !connection || !bhs)
        return EINVAL;
    
    // Receive data over the network
    struct msghdr msg;
    struct iovec  iovec[2];
    memset(&msg,0,sizeof(struct msghdr));
    
    msg.msg_iov = iovec;
    unsigned int iovecCnt = 0;
    
    // Set basic header segment
    iovec[iovecCnt].iov_base  = bhs;
    iovec[iovecCnt].iov_len   = kiSCSIPDUBasicHeaderSegmentSize;
    iovecCnt++;

    UInt32 headerDigest = 0;
    
    // Retrieve header digest, if one exists
    if(connection->useHeaderDigest)
    {
        iovec[iovecCnt].iov_base = &headerDigest;
        iovec[iovecCnt].iov_len  = sizeof(headerDigest);
        iovecCnt++;
    }
    
    msg.msg_iovlen = iovecCnt;

    // Bytes received from sock_receive call
    size_t bytesRecv;
    errno_t error;

    // Handle connection problems
    if((error = sock_receive(connection->socket,&msg,MSG_WAITALL,&bytesRecv)))
    {
        if(error != EWOULDBLOCK) {
            DBLog("iscsi: sock_receive error returned with code %d (sid: %d, cid: %d)\n",error,session->sessionId,connection->cid);
            HandleConnectionTimeout(session->sessionId,connection->cid);
            return error;
        }
        else
            error = 0;
    }
    
    // Verify length; incoming PDUS from a target should have no AHS, verify.
    if(bytesRecv < kiSCSIPDUBasicHeaderSegmentSize)// || bhs->totalAHSLength != 0)
    {
        DBLog("iscsi: Received incomplete PDU header: %zu bytes (sid: %d, cid: %d)\n",bytesRecv,session->sessionId,connection->cid);
        
// TODO: handle error
        
        return EIO;
    }
    
    // Verify digest if present
    if(headerDigest)
    {
        // Compute digest (should be 0 since we start with the digest)
        if(headerDigest != crc32c(0,bhs,kiSCSIPDUBasicHeaderSegmentSize))
        {
            DBLog("iscsi: Failed header digest (sid: %d, cid: %d)\n",session->sessionId,connection->cid);
            
// TODO: handle error
            
            return EIO;
        }
    }
    
    // Update command sequence numbers only if the PDU was not a data PDU
    // (unless the data PDU contains a SCSI service response)

    if(bhs->opCode == kiSCSIPDUOpCodeDataIn) {
        iSCSIPDUDataInBHS * bhsDataIn = (iSCSIPDUDataInBHS *)bhs;
        if((bhsDataIn->flags & kiSCSIPDUDataInStatusFlag) == 0)
            return error;
    }
 
    // Read and update the command sequence numbers
    bhs->maxCmdSN = OSSwapBigToHostInt32(bhs->maxCmdSN);
    bhs->expCmdSN = OSSwapBigToHostInt32(bhs->expCmdSN);
    bhs->statSN = OSSwapBigToHostInt32(bhs->statSN);
    
    if(bhs->maxCmdSN > session->maxCmdSN)
        OSWriteLittleInt32(&session->maxCmdSN,0,bhs->maxCmdSN);
    if(bhs->expCmdSN > session->expCmdSN)
        OSWriteLittleInt32(&session->expCmdSN,0,bhs->expCmdSN);
    
    if(bhs->opCode != kiSCSIPDUOpCodeR2T && bhs->statSN != 0xffffffff && bhs->initiatorTaskTag != 0xffffffff)
        OSIncrementAtomic(&connection->expStatSN);
    
    return error;
}

/*! Receives a data segment over a kernel socket.  If the specified length is 
 *  not a multiple of 4-bytes, the padding bytes will be discarded per 
 *  RF3720 specification (all data segment are multiples of 4 bytes).
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param data the data received.
 *  @param length the length of the data buffer.
 *  @param flags optional flags to be passed onto sock_recv.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::RecvPDUData(iSCSISession * session,
                                     iSCSIConnection * connection,
                                     void * data,
                                     size_t length,
                                     int flags)
{
    // Range-check inputs
    if(!session || !connection || !data)
        return EINVAL;
    
    // Setup message with required iovec
    struct msghdr msg;
    struct iovec  iovec[4];
    memset(&msg,0,sizeof(struct msghdr));
    msg.msg_iov = iovec;
    unsigned int iovecCnt = 0;

    // Setup to receive data block
    iovec[iovecCnt].iov_base  = data;
    iovec[iovecCnt].iov_len   = length;
    iovecCnt++;
    
    // Setup to receive (and discard) padding bytes, if required
    UInt32 paddingLen = 4-(length % 4);
    UInt32 padding = 0;
    if(paddingLen != 4)
    {
       iovec[iovecCnt].iov_base  = &padding;
       iovec[iovecCnt].iov_len   = paddingLen;
       iovecCnt++;
    }
    
    UInt32 dataDigest = 0;
    
    // Retrieve data digest, if one exists
    if(connection->useDataDigest)
    {
        iovec[iovecCnt].iov_base = &dataDigest;
        iovec[iovecCnt].iov_len  = sizeof(dataDigest);
        iovecCnt++;
    }

    msg.msg_iovlen = iovecCnt;
    
    size_t bytesRecv;
    errno_t error = 0;
    
    // Handle connection problems
    if((error = sock_receive(connection->socket,&msg,MSG_WAITALL,&bytesRecv)))
    {
        if(error != EWOULDBLOCK) {
            DBLog("iscsi: sock_receive error returned with code %d (sid: %d, cid: %d)\n",error,session->sessionId,connection->cid);
            HandleConnectionTimeout(session->sessionId,connection->cid);
            return error;
        }
        else
            error = 0;
    }
    
    // Verify digest if present
    if(connection->useDataDigest)
    {
        // Compute digest including padding...
        UInt32 calcDigest = crc32c(0,data,length);
        
        if(dataDigest != calcDigest)
        {
            DBLog("iscsi: Failed data digest (sid: %d, cid: %d)\n",session->sessionId,connection->cid);
            
// TODO: handle error
            
            return EIO;
        }
    }

    return error;
}
