/*!
 * @author		Nareg Sinenian
 * @file		iSCSIVirtualHBA.cpp
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#include "iSCSIVirtualHBA.h"
#include "iSCSIIOEventSource.h"
#include "iSCSITaskQueue.h"
#include "iSCSITypesKernel.h"
#include "crc32c.h"

#include <sys/ioctl.h>
#include <sys/unistd.h>


#undef DEBUG

// Use DBLog() for debug outputs and IOLog() for all outputs
// DBLog() is only enabled for debug builds
#ifdef DEBUG
#define DBLog(...) IOLog(__VA_ARGS__)
#else
#define DBLog(...)
#endif

#define super IOSCSIParallelInterfaceController

#define ISCSI_PRODUCT_NAME  "iSCSI Virtual Host Bus Adapter"
#define ISCSI_PRODUCT_REVISION_LEVEL "1.0"

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

/*! Number of PDUs that are transmitted before we calculate an average speed
 *  for the connection (1024^2 = 1048576). */
const UInt32 iSCSIVirtualHBA::kNumBytesPerAvgBW = 1048576;

OSDefineMetaClassAndStructors(iSCSIVirtualHBA,IOSCSIParallelInterfaceController);

SCSILogicalUnitNumber iSCSIVirtualHBA::ReportHBAHighestLogicalUnitNumber()
{
	return kHighestLun;
}

bool iSCSIVirtualHBA::DoesHBASupportSCSIParallelFeature(SCSIParallelFeature theFeature)
{
	return true;
}

bool iSCSIVirtualHBA::InitializeTargetForID(SCSITargetIdentifier targetId)
{
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

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTask;
    bhs.referencedTaskTag = OSSwapHostToBigInt32((UInt32)taggedTaskID);
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncAbortTask);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iSCSI: Abort task request\n");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::AbortTaskSetRequest(SCSITargetIdentifier targetId,
														 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTaskSet;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncAbortTaskSet);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iSCSI: Abort task set request\n");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::ClearACARequest(SCSITargetIdentifier targetId,
													 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearACA;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncClearACA);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iSCSI: Clear ACA request\n");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::ClearTaskSetRequest(SCSITargetIdentifier targetId,
														 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearTaskSet;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncClearTaskSet);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iSCSI: Clear task set request\n");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::LogicalUnitResetRequest(SCSITargetIdentifier targetId,
															 SCSILogicalUnitNumber LUN)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncLUNReset;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagTaskMgmt,LUN,kiSCSIPDUTaskMgmtFuncLUNReset);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;

    DBLog("iSCSI: LUN reset request\n");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::TargetResetRequest(SCSITargetIdentifier targetId)
{
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;

    // Create a SCSI target management PDU and send
    iSCSIPDUTaskMgmtReqBHS bhs = iSCSIPDUTaskMgmtReqBHSInit;
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncTargetWarmReset;
    bhs.initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagTaskMgmt,0,kiSCSIPDUTaskMgmtFuncTargetWarmReset);
    
    if(SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0))
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    DBLog("iSCSI: Target reset request\n");
    
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
    DBLog("iSCSI: Initializing virtual HBA\n");
    
    // Initialize CRC32C
    crc32c_init();
    
    // Setup session & target list
    sessionList = (iSCSISession **)IOMalloc(kMaxSessions*sizeof(iSCSISession*));
    targetList  = OSDictionary::withCapacity(kMaxSessions);
    
    if(!sessionList)
        return false;
    
    memset(sessionList,0,kMaxSessions*sizeof(iSCSISession *));
    
    /* Set product name. */
    SetHBAProperty(kIOPropertyProductNameKey,OSString::withCString(ISCSI_PRODUCT_NAME));
    SetHBAProperty(kIOPropertyProductRevisionLevelKey,OSString::withCString(ISCSI_PRODUCT_REVISION_LEVEL));
    SetHBAProperty(kIOPropertyPortDescriptionKey,OSString::withCString("iqn.name.com"));

    // Make ourselves discoverable to user clients (we do this last after
    // everything is initialized).
    registerService();
    
    // Generate an initiator id using a random number (per RFC3720)
    kInitiatorId = random();
    
    
	// Successfully initialized controller
	return true;
}

void iSCSIVirtualHBA::TerminateController()
{
    DBLog("iSCSI: Terminating virtual HBA\n");
    
    // Go through every connection for each session, and close sockets,
    // remove event sources, etc
    for(SID index = 0; index < kMaxSessions; index++)
    {
        if(!sessionList[index])
            continue;

        ReleaseSession(index);
    }
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
    SID sessionId = (UInt16)GetTargetIdentifier(task);
    CID connectionId = *((UInt32*)GetHBADataPointer(task));
    
    if(connectionId >= kMaxConnectionsPerSession)
        return;
    
    iSCSISession * session = sessionList[sessionId];
    if(!session)
        return;
    
    iSCSIConnection * connection = session->connections[connectionId];
    if(!connection)
        return;
    
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
void iSCSIVirtualHBA::HandleConnectionTimeout(SID sessionId,CID connectionId)
{
    
    
    
}

SCSIServiceResponse iSCSIVirtualHBA::ProcessParallelTask(SCSIParallelTaskIdentifier parallelTask)
{
    // Here we set an (iSCSI) initiator task tag for the SCSI task and queue
    // the iSCSI task for later processing
    SCSITargetIdentifier targetId   = GetTargetIdentifier(parallelTask);
    SCSILogicalUnitNumber LUN       = GetLogicalUnitNumber(parallelTask);
    SCSITaggedTaskIdentifier taskId = GetTaggedTaskIdentifier(parallelTask);
    
    iSCSISession * session = sessionList[(UInt16)targetId];
    
    if(!session)
        return kSCSIServiceResponse_FUNCTION_REJECTED;
    
    // Determine which connection this task should be assigned to based on
    // bitrate and processing load; we do this by looking at the amount of
    // data each connection
    iSCSIConnection * connection = session->connections[0];
    
    if(!connection || !connection->dataRecvEventSource)
        return kSCSIServiceResponse_FUNCTION_REJECTED;
    
    // Associate a connection identifier with this task; this is used to
    // maintain the connection associated with a task when only task information
    // is available (e.g., in the case of a task timeout).
    *((UInt32*)GetHBADataPointer(parallelTask)) = 0;
    
    // Add the amount of data that we need to transfer to this connection
    OSAddAtomic(GetRequestedDataTransferCount(parallelTask),&connection->dataToTransfer);

    // Build and set iSCSI initiator task tag
    UInt32 initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagSCSITask,LUN,taskId);
    SetControllerTaskIdentifier(parallelTask,initiatorTaskTag);
    
    DBLog("iSCSI: Transfer size: %d\n",connection->dataToTransfer);
    
    // Queue task in the event source (we'll remove it from the queue when were
    // done processing the task)
    connection->taskQueue->queueTask(initiatorTaskTag);
    
    DBLog("iSCSI: Queued task %llx\n",taskId);
    return kSCSIServiceResponse_Request_In_Process;
}

void iSCSIVirtualHBA::BeginTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                                iSCSISession * session,
                                                iSCSIConnection * connection,
                                                UInt32 initiatorTaskTag)
{
    // Task tag corresponding to a connection timeout measurement
    if(owner->ParseInitiatorTaskTagForID(initiatorTaskTag) == kInitiatorTaskTagLatency)
    {
        // Send a NOP out...
        owner->MeasureConnectionLatency(session,connection);
        return;
    }
    
    // Grab parallel task associated with this iSCSI task
    SCSIParallelTaskIdentifier parallelTask =
        owner->FindTaskForControllerIdentifier(session->sessionId,initiatorTaskTag);
    
    if(!parallelTask)
    {
        DBLog("iSCSI: Task not found, flushing stream (BeginTaskOnWorkloopThread)\n");
        return;
    }
    
    // Extract information about this SCSI task
    SCSILogicalUnitNumber LUN       = owner->GetLogicalUnitNumber(parallelTask);
    SCSITaskAttribute attribute     = owner->GetTaskAttribute(parallelTask);
    UInt8   transferDirection       = owner->GetDataTransferDirection(parallelTask);
    UInt32  transferSize            = (UInt32)owner->GetRequestedDataTransferCount(parallelTask);
    UInt8   cdbSize                 = owner->GetCommandDescriptorBlockSize(parallelTask);
    
    DBLog("iSCSI: Processing task %llx\n",taskId);
    
    // Now that we know task is valid, timestamp the connection indicating
    // when we started processing the task
    clock_get_system_microtime(&(connection->taskStartTimeSec),
                               &(connection->taskStartTimeUSec));
    
    // Create a SCSI request PDU
    iSCSIPDUSCSICmdBHS bhs  = iSCSIPDUSCSICmdBHSInit;
    bhs.dataTransferLength  = OSSwapHostToBigInt32(transferSize);
    bhs.LUN                 = OSSwapHostToBigInt64(LUN);
    
    // The initiator task tag is just LUN and task identifier
    bhs.initiatorTaskTag = initiatorTaskTag;
    
    if(transferDirection == kSCSIDataTransfer_FromInitiatorToTarget)
        bhs.flags |= kiSCSIPDUSCSICmdFlagWrite;
    else
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
    
    // Sets the timeout in milliseconds for processing the current task
    
    ///// Needs to be dynamic - want lower timeout for startup but indefinite(?)
    ///// timeout for tasks where veyr large file transfers occur....
    ///// during startup, if something fails, (e.g., target doesn't respond)
    ///// perhaps set timeout basd on transfer length and adjust it dynamically
    owner->SetTimeoutForTask(parallelTask,600000);
    
    // For non-WRITE commands, send off SCSI command PDU immediately.
    if(transferDirection != kSCSIDataTransfer_FromInitiatorToTarget)
    {
        bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;
        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
        return;
    }
    
    // At this point we have have a write command, determine whether we need
    // to send data at this point or later in response to an R2T
    if(session->opts.initialR2T && !session->opts.immediateData)
    {
        bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;
        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
        return;
    }
    
    // For SCSI WRITE command PDUs, map pointer to IOMemoryDescriptor buffer
    IOMemoryDescriptor  * dataDesc  = owner->GetDataBuffer(parallelTask);
    IOMemoryMap         * dataMap   = dataDesc->map();
    UInt8               * data      = (UInt8 *)dataMap->getAddress();

    // Offset relative to transfer request (not relative to IOMemoryDescriptor)
    UInt32 dataOffset = 0;
    
    // First use immediate data to send as data with command PDU...
    if(session->opts.immediateData) {
        
        // Either we send the max allowed data (immediate data length) or
        // all of the data if it is lesser than the max allowed limit
        UInt32 dataLen = min(connection->immediateDataLength,transferSize);
        
        // If we need to wait for an R2T or we've transferred all data
        // as immediate data then no additional data will follow this PDU...
        if(session->opts.initialR2T || dataLen == transferSize)
            bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;

        owner->SendPDU(session,connection,(iSCSIPDUInitiatorBHS *)&bhs,NULL,data,dataLen);
        dataOffset += dataLen;
        
        owner->SetRealizedDataTransferCount(parallelTask,dataLen);
        connection->dataToTransfer -= dataLen;
    }

    // Follow up with data out PDUs up to the firstBurstLength bytes if R2T=No
    if(!session->opts.initialR2T &&                     // Initial R2T = No
       dataOffset < session->opts.firstBurstLength &&   // Haven't hit burst limit
       dataOffset < transferSize)                       // Data left to send
    {
        iSCSIPDUDataOutBHS bhsDataOut = iSCSIPDUDataOutBHSInit;
        bhsDataOut.LUN              = bhs.LUN;
        bhsDataOut.initiatorTaskTag = bhs.initiatorTaskTag;
        bhsDataOut.targetTransferTag = kiSCSIPDUTargetTransferTagReserved;
        
        UInt32 dataSN = 0;
        UInt32 maxTransferLength = connection->opts.maxSendDataSegmentLength;
        UInt32 remainingDataLength = min(session->opts.firstBurstLength-dataOffset,
                                         transferSize-dataOffset);
        
        while(remainingDataLength != 0)
        {
            bhsDataOut.bufferOffset = OSSwapHostToBigInt32(dataOffset);
            bhsDataOut.dataSN = OSSwapHostToBigInt32(dataSN);
            
            if(maxTransferLength < remainingDataLength) {
                DBLog("iSCSI: Max transfer length: %d\n",maxTransferLength);
                int err = owner->SendPDU(session,connection,
                                         (iSCSIPDUInitiatorBHS*)&bhsDataOut,NULL,
                                         data,maxTransferLength);
                
                if(err != 0) {
                    DBLog("iSCSI: Send error: %d\n",err);
                    dataMap->unmap();
                    dataMap->release();
                    return;
                }
                
                DBLog("iSCSI: dataoffset: %d\n",dataOffset);
                
                remainingDataLength -= maxTransferLength;
                data                += maxTransferLength;
                dataOffset          += maxTransferLength;
            }
            // This is the final PDU of the sequence
            else {
                DBLog("iSCSI: Sending final data out\n");
                bhsDataOut.flags = kiSCSIPDUDataOutFinalFlag;
                int err = owner->SendPDU(session,connection,
                                         (iSCSIPDUInitiatorBHS*)&bhsDataOut,NULL,
                                         data,remainingDataLength);
                
                if(err != 0) {
                    DBLog("iSCSI: Send error: %d\n",err);
                    dataMap->unmap();
                    dataMap->release();
                    return;
                }
                break;
            }
            // Increment the data sequence number
            dataSN++;
        }
    }

    // Release mapping to IOMemoryDescriptor buffer
    dataMap->unmap();
    dataMap->release();
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
        DBLog("iSCSI: Failed to get PDU header\n");
        return true;
    }
    else
        DBLog("iSCSI: Received PDU\n");

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
        UInt32 initiatorTaskTag = BuildInitiatorTaskTag(kInitiatorTaskTagLatency,0,0);
        connection->taskQueue->queueTask(initiatorTaskTag);
    }
    
    // Iterate over last few points, compute peak value
    connection->bytesPerSecond = 0;
    for(UInt8 i = 0; i < connection->kBytesPerSecAvgWindowSize; i++)
        if(connection->bytesPerSecond < connection->bytesPerSecondHistory[i])
            connection->bytesPerSecond = connection->bytesPerSecondHistory[i];
    
    DBLog("iSCSI: Bytes per second: %d\n",connection->bytesPerSecond);

    super::CompleteParallelTask(parallelRequest,completionStatus,serviceResponse);
}


void iSCSIVirtualHBA::ProcessTaskMgmtRsp(iSCSISession * session,
                                         iSCSIConnection * connection,
                                         iSCSIPDU::iSCSIPDUTaskMgmtRspBHS * bhs)
{
    // Extract LUN and function code from task tag
    bhs->initiatorTaskTag = OSSwapBigToHostInt32(bhs->initiatorTaskTag);
    UInt64 LUN  = bhs->initiatorTaskTag >> sizeof(UInt16);
    UInt8  taskMgmtFunction = bhs->initiatorTaskTag & 0xFF;
    
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
    // Length of the data segment of the PDU
    UInt32 length = 0;
    memcpy(&length,bhs->dataSegmentLength,kiSCSIPDUDataSegmentLengthSize);
    length = OSSwapBigToHostInt32(length<<8);
    
    // Grab data payload (ping data)
    UInt8 data[length];
    
    if(RecvPDUData(session,connection,data,length,MSG_WAITALL) != 0) {
        DBLog("iSCSI: Failed to retreive NOP in data\n");
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
        clock_usec_t microsecs_stamp, microsecs;
        
        // Grab timestamp from NOP-in PDU
        memcpy(&secs_stamp,data,sizeof(secs_stamp));
        memcpy(&microsecs_stamp,data+sizeof(secs_stamp),sizeof(microsecs_stamp));
        
        // Grab current system uptime
        clock_get_system_microtime(&secs,&microsecs);
    
        UInt32 latency_ms = (secs - secs_stamp)*1e3 + (microsecs - microsecs_stamp)/1e3;
        
        DBLog("iSCSI: Connection latency: %d ms\n",latency_ms);
        
        // Remove latency measurement task from queue
        connection->taskQueue->completeCurrentTask();
    }
    // The target initiated this ping, just copy parameters and respond
    else {
        iSCSIPDUNOPOutBHS bhsRsp = iSCSIPDUNOPOutBHSInit;
        bhsRsp.LUN = bhs->LUN;
        bhsRsp.targetTransferTag = bhs->targetTransferTag;
        
        if(SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsRsp,NULL,data,length))
            DBLog("iSCSI: Failed to send NOP response\n");
    }
}

void iSCSIVirtualHBA::ProcessSCSIResponse(iSCSISession * session,
                                          iSCSIConnection * connection,
                                          iSCSIPDU::iSCSIPDUSCSIRspBHS * bhs)
{
    const UInt8 senseDataHeaderSize = 2;
    
    // Padded length of the data segment of the PDU
    //    size_t length = iSCSIPDUGetDataSegmentLength((iSCSIPDUCommonBHS*)&bhs);
    UInt32 length = 0;
    memcpy(&length,bhs->dataSegmentLength,kiSCSIPDUDataSegmentLengthSize);
    length = OSSwapBigToHostInt32(length<<8);
    
    UInt8 data[length];

    if(length > 0) {
        if(RecvPDUData(session,connection,data,length,MSG_WAITALL))
            DBLog("iSCSI: Error retrieving data segment\n");
        else
            DBLog("iSCSI: Received sense data\n");
    }

    // Grab parallel task associated with this PDU, indexed by task tag
    SCSIParallelTaskIdentifier parallelTask =
        FindTaskForControllerIdentifier(session->sessionId,bhs->initiatorTaskTag);
    
    if(!parallelTask)
    {
        DBLog("iSCSI: Task not found, flushing stream (ProcessSCSIResponse)\n");
        
        // Flush stream
        UInt8 buffer[length];
        RecvPDUData(session,connection,buffer,length,MSG_WAITALL);
        
        return;
    }
    
    SetRealizedDataTransferCount(parallelTask,(UInt32)GetRequestedDataTransferCount(parallelTask));

    // Process sense data if the PDU came with any...
    if(length >= senseDataHeaderSize)
    {
        // First two bytes of the data segment are the size of the sense data
        UInt16 senseDataLength = *((UInt16*)&data[0]);
        senseDataLength = OSSwapBigToHostInt16(senseDataLength);
        
        if(length < senseDataLength + senseDataHeaderSize) {
            DBLog("iSCSI: Received invalid sense data\n");
        }
        else {
        
            // Remaining data is sense data, advance pointer by two bytes to get this
            SCSI_Sense_Data * newSenseData = (SCSI_Sense_Data *)(data + senseDataHeaderSize);
        
            // Incorporate sense data into the task
            SetAutoSenseData(parallelTask,newSenseData,senseDataLength);
            
            DBLog("iSCSI: Processed sense data\n");
        }
    }
    
    // Set the SCSI completion status and service response, let SCSI stack
    // know that we're done with this task...
    
    SCSITaskStatus completionStatus = (SCSITaskStatus)bhs->status;
    SCSIServiceResponse serviceResponse;

    if(bhs->response == kiSCSIPDUSCSICmdCompleted)
        serviceResponse = kSCSIServiceResponse_TASK_COMPLETE;
    else
        serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    CompleteParallelTask(session,connection,parallelTask,completionStatus,serviceResponse);
    
    // Task is complete, remove it from the queue
    connection->taskQueue->completeCurrentTask();
    
    DBLog("iSCSI: Processed SCSI response\n");
}

void iSCSIVirtualHBA::ProcessDataIn(iSCSISession * session,
                                    iSCSIConnection * connection,
                                    iSCSIPDU::iSCSIPDUDataInBHS * bhs)
{
    // Length of the data segment of the PDU
    UInt32 length = 0;
    memcpy(&length,bhs->dataSegmentLength,kiSCSIPDUDataSegmentLengthSize);
    length = OSSwapBigToHostInt32(length<<8);

    // Grab parallel task associated with this PDU, indexed by task tag
    SCSIParallelTaskIdentifier parallelTask =
        FindTaskForControllerIdentifier(session->sessionId,bhs->initiatorTaskTag);
    
    if(length == 0)
    {
        DBLog("iSCSI: Missing data segment in data-in PDU\n");
        return;
    }
    
    UInt8 buffer[length];
    
    // If task not found, flush stream
    if(!parallelTask)
    {
        DBLog("iSCSI: Task not found\n");
        RecvPDUData(session,connection,buffer,length,MSG_WAITALL);
        return;
    }
    
    // System buffer offset for this PDU data segment...
    UInt32 dataOffset = OSSwapBigToHostInt32(bhs->bufferOffset);
    
    if(RecvPDUData(session,connection,buffer,length,0))
        DBLog("iSCSI: Error in retrieving data segment length.\n");
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
        
        DBLog("iSCSI: Processed data-in PDU\n");
    }
    
    // Send acknowledgement to target if one is required
    if(bhs->flags & kiSCSIPDUDataInAckFlag)
    {}
}

/*! Process an incoming asynchronous message PDU.
 *  @param session the session associated with the async PDU.
 *  @param connection the connection associated with the async PDU.
 *  @param bhs the basic header segment of the async PDU. */
void iSCSIVirtualHBA::ProcessAsyncMsg(iSCSISession * session,
                                      iSCSIConnection * connection,
                                      iSCSIPDU::iSCSIPDUAsyncMsgBHS * bhs)
{
    iSCSIPDUAsyncMsgEvent asyncEvent = (iSCSIPDUAsyncMsgEvent)(bhs->asyncEvent);
    switch(asyncEvent)
    {
        case kiSCSIPDUAsyncMsgDropAllConnections: break;
        case kiSCSIPDUAsyncMsgLogout:   break;
        case kiSCSIPDUAsyncMsgNegotiateParams: break;
        case kiSCSIPDUAsyncMsgSCSIAsyncMsg: break;
        case kiSCSIPDUAsyncMsgVendorCode: break;
        case kiSCSIPDUAsynMsgDropConnection: break;
        default: break;
    };
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
        DBLog("iSCSI: Task not found\n");
        return;
    }
    
    // Create a mapping to the task's data buffer.  This is the data that
    // we will read and pack into a sequence of PDUs to send to the target.
    IOMemoryDescriptor  * dataDesc   = GetDataBuffer(parallelTask);
    IOMemoryMap         * dataMap    = dataDesc->map();
    UInt8               * data       = (UInt8 *)dataMap->getAddress();

    // Obtain requested data offset and requested lengths
    UInt32 dataOffset           = OSSwapBigToHostInt32(bhs->bufferOffset);
    UInt32 remainingDataLength  = OSSwapBigToHostInt32(bhs->desiredDataLength);

    // Ensure that our data buffer contains all of the requested data
    if(dataOffset + remainingDataLength > (UInt32)dataMap->getLength())
    {
        DBLog("iSCSI: Host data buffer doesn't contain requested data");
        dataMap->unmap();
        dataMap->release();
        return;
    }
    
    // Amount of data to transfer in each iteration (per PDU)
    UInt32 maxTransferLength = connection->opts.maxSendDataSegmentLength;
    
    data += dataOffset;
    
    DBLog("iSCSI: dataoffset: %d\n",dataOffset);
    DBLog("iSCSI: desired data length: %d\n",remainingDataLength);
    
    UInt32 dataSN = 0;
    
    // Create data PDUs and send them until all desired data has been sent
    iSCSIPDUDataOutBHS bhsDataOut = iSCSIPDUDataOutBHSInit;
    bhsDataOut.LUN              = bhs->LUN;
    bhsDataOut.initiatorTaskTag = bhs->initiatorTaskTag;
    
    // Let target know that this data out sequence is in response to the
    // transfer tag the target gave us with the R2TSN (both in high-byte order)
    bhsDataOut.targetTransferTag = bhs->targetTransferTag;

    while(remainingDataLength != 0)
    {
        bhsDataOut.bufferOffset = OSSwapHostToBigInt32(dataOffset);
        bhsDataOut.dataSN = OSSwapHostToBigInt32(dataSN);

        if(maxTransferLength < remainingDataLength) {
            DBLog("iSCSI: Max transfer length: %d\n",maxTransferLength);
            int err = SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsDataOut,
                              NULL,data,maxTransferLength);
            
            if(err != 0) {
                DBLog("iSCSI: Send error: %d\n",err);
                dataMap->unmap();
                dataMap->release();
                return;
            }
            
            DBLog("iSCSI: dataoffset: %d\n",dataOffset);
            DBLog("iSCSI: desired data length: %d\n",remainingDataLength);
            
            remainingDataLength -= maxTransferLength;
            data                += maxTransferLength;
            dataOffset          += maxTransferLength;
        }
        // This is the final PDU of the sequence
        else {
            DBLog("iSCSI: Sending final data out\n");
            bhsDataOut.flags = kiSCSIPDUDataOutFinalFlag;
            int err = SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsDataOut,
                              NULL,data,remainingDataLength);
            
            if(err != 0) {
                DBLog("iSCSI: Send error: %d\n",err);
                dataMap->unmap();
                dataMap->release();
                return;
            }
            break;
        }
        // Increment the data sequence number
        dataSN++;
    }

    // Let the driver stack know how much we've transferred (everything)
    UInt32 transferLen = OSSwapBigToHostInt32(bhs->desiredDataLength);
    SetRealizedDataTransferCount(parallelTask,transferLen+dataOffset);
                                              
    connection->dataToTransfer -= transferLen;
    
    // Release the mapping object (this leaves the descriptor and buffer intact)
    dataMap->unmap();
    dataMap->release();
}

/*! Process an incoming reject PDU.
 *  @param session the session associated with the R2T PDU.
 *  @param connection the connection associated with the R2T PDU.
 *  @param bhs the basic header segment of the reject PDU. */
void iSCSIVirtualHBA::ProcessReject(iSCSISession * session,
                                    iSCSIConnection * connection,
                                    iSCSIPDU::iSCSIPDURejectBHS * bhs)
{

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
    const UInt32 length = sizeof(clock_sec_t) + sizeof(clock_usec_t);
    UInt8 data[length];
    clock_get_system_microtime((clock_sec_t*)data,
                               (clock_usec_t*)(data+sizeof(clock_sec_t)));
    
    SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhs,NULL,data,length);
}


//////////////////////////////// iSCSI FUNCTIONS ///////////////////////////////

/*! Allocates a new iSCSI session and returns a session qualifier ID.
 *  @param targetName the name of the target, or NULL if discovery session.
 *  @param targetaddress the BSD socket structure used to identify the target.
 *  @param hostaddress the BSD socket structure used to identify the host adapter.
 *  @param sessionId identifier for the new session.
 *  @param connectionId identifier for the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::CreateSession(const char * targetName,
                                       const struct sockaddr_storage * targetAddress,
                                       const struct sockaddr_storage * hostAddress,
                                       SID * sessionId,
                                       CID * connectionId)
{
    // Validate inputs
    if(!targetAddress || !hostAddress || !sessionId || !connectionId)
        return EINVAL;

    // Default session and connection Ids
    *sessionId = kiSCSIInvalidSessionId;
    *connectionId = kiSCSIInvalidConnectionId;

    // Initialize default error (try again)
    errno_t error = EAGAIN;
    
// LOCK SESSION LIST HERE
    
    // Find an open session slot
    SID sessionIdx;
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
    newSession->cmdSN = 0;
    newSession->expCmdSN = 0;
    newSession->maxCmdSN = 0;
//    newSession->initiatorTaskTag = 0;

    // Retain new session
    sessionList[sessionIdx] = newSession;
    *sessionId = sessionIdx;
    
    // Add target to lookup table...
    targetList->setObject(targetName,OSNumber::withNumber(sessionIdx,sizeof(sessionIdx)*8));

    // Create a connection associated with this session
    if((error = CreateConnection(*sessionId,targetAddress,hostAddress,connectionId)))
        goto SESSION_CREATE_CONNECTION_FAILURE;
    
    
// UNLOCK SESSION HERE...
    
    // Success
    return 0;
    
SESSION_CREATE_CONNECTION_FAILURE:

    // Remove target from lookup table
    targetList->removeObject(targetName);
    
    IOFree(newSession->connections,kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
// LOCK SESSION LIST...

    sessionList[sessionIdx] = nullptr;
// UNLOCK SESSION LIST...
    *sessionId = kiSCSIInvalidSessionId;
 
SESSION_CONNECTION_LIST_ALLOC_FAILURE:
    IOFree(newSession,sizeof(iSCSISession));
   
SESSION_ALLOC_FAILURE:

SESSION_ID_ALLOC_FAILURE:
    // Unlock session list here
    
    return error;
}

/*! Releases an iSCSI session, including all connections associated with that
 *  session.
 *  @param sessionId the session qualifier part of the ISID. */
void iSCSIVirtualHBA::ReleaseSession(SID sessionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions)
        return;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return;
    
    DBLog("iSCSI: Releasing session...\n");
    
    // Disconnect all connections
    for(CID connectionId = 0; connectionId < kMaxConnectionsPerSession; connectionId++)
    {
        if(theSession->connections[connectionId])
            ReleaseConnection(sessionId,connectionId);
    }
    
    // Free connection list and session object
    IOFree(theSession->connections,kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    IOFree(theSession,sizeof(iSCSISession));
    
    // Remove target name from dictionary
    OSCollectionIterator * iter = OSCollectionIterator::withCollection(targetList);
    OSString * targetName;
    while((targetName = (OSString *)iter->getNextObject())) {
        OSNumber * targetId = (OSNumber*)targetList->getObject(targetName);
        if(targetId->unsigned16BitValue() == sessionId)
        {
            targetList->removeObject(targetName);
            break;
        }
    }
    sessionList[sessionId] = NULL;
}

/*! Allocates a new iSCSI connection associated with the particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param targetaddress the BSD socket structure used to identify the target.
 *  @param hostaddress the BSD socket structure used to identify the host adapter.
 *  @param connectionId identifier for the new connection.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::CreateConnection(SID sessionId,
                                          const struct sockaddr_storage * targetAddress,
                                          const struct sockaddr_storage * hostAddress,
                                          CID * connectionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || !targetAddress || !hostAddress || !connectionId)
        return EINVAL;
    
    // Retrieve the session from the session list, validate
    iSCSISession * session = sessionList[sessionId];
    if(!session)
        return EINVAL;
    
    // Find an empty connection slot to use for a new connection
    CID index;
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
    
    // Sockets connected and bound add event source to driver workloop
    newConn->expStatSN = 0;
    newConn->dataToTransfer = 0;
    newConn->bytesPerSecond = 0;
    newConn->opts.useHeaderDigest = false;
    newConn->opts.useDataDigest = false;
    session->connections[index] = newConn;
    *connectionId = index;
    
    // Initialize default error (try again)
    errno_t error = EAGAIN;
    
    if(!(newConn->PDUIOLock = IOLockAlloc()))
        goto IOLOCK_ALLOC_FAILURE;
    
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
    // Domain can vary between IPv4 or IPv6.
    error = sock_socket(targetAddress->ss_family,
                        SOCK_STREAM,IPPROTO_TCP,
                        (sock_upcall)&iSCSIIOEventSource::socketCallback,
                        newConn->dataRecvEventSource,
                        &newConn->socket);
    if(error)
        goto SOCKET_CREATE_FAILURE;
    
    // Bind socket to a particular host connection
    if((error = sock_bind(newConn->socket,(sockaddr*)hostAddress)))
        goto SOCKET_BIND_FAILURE;

    // Connect the socket to the target node
    if((error = sock_connect(newConn->socket,(sockaddr*)targetAddress,0)))
        goto SOCKET_CONNECT_FAILURE;

    
    // Keep track of connection count
//    OSIncrementAtomic(&session->numConnections);
    
    // Initialize queue that keeps track of connection speed
    memset(newConn->bytesPerSecondHistory,0,sizeof(UInt8)*newConn->kBytesPerSecAvgWindowSize);
    newConn->bytesPerSecHistoryIdx = 0;
    
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
    IOLockFree(newConn->PDUIOLock);
    
IOLOCK_ALLOC_FAILURE:
    session->connections[index] = 0;
    IOFree(newConn,sizeof(iSCSIConnection));
    
    return error;
}

/*! Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
void iSCSIVirtualHBA::ReleaseConnection(SID sessionId,
                                        CID connectionId)
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
    
    // Keep track of connection count
//    OSDecrementAtomic(&session->numConnections);
    
    IOLockLock(connection->PDUIOLock);
    
    // First deactivate connection before proceeding
    if(connection->taskQueue->isEnabled())
        DeactivateConnection(sessionId,connectionId);

    sock_close(connection->socket);

    DBLog("iSCSI: Deactivated connection.\n");

    GetWorkLoop()->removeEventSource(connection->dataRecvEventSource);
    GetWorkLoop()->removeEventSource(connection->taskQueue);
    
    DBLog("iSCSI: Removed event sources.\n");
    
    connection->dataRecvEventSource->release();
    connection->taskQueue->release();
    connection->dataToTransfer = 0;
    
    IOLockUnlock(connection->PDUIOLock);
    IOLockFree(connection->PDUIOLock);
    IOFree(connection,sizeof(iSCSIConnection));
    session->connections[connectionId] = NULL;
    
    DBLog("iSCSI: Released connection.\n");
}

/*! Activates an iSCSI connection, indicating to the kernel that the iSCSI
 *  daemon has negotiated security and operational parameters and that the
 *  connection is in the full-feature phase.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::ActivateConnection(SID sessionId,CID connectionId)
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
errno_t iSCSIVirtualHBA::ActivateAllConnections(SID sessionId)
{
    if(sessionId >= kMaxSessions)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    errno_t error = 0;
    for(CID connectionId = 0; connectionId < kMaxConnectionsPerSession; connectionId++)
        if((error = ActivateConnection(sessionId,connectionId)))
            return error;
    
    return 0;
}

/*! Deactivates an iSCSI connection so that parameters can be adjusted or
 *  negotiated by the iSCSI daemon.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::DeactivateConnection(SID sessionId,CID connectionId)
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
    
    DBLog("iSCSI: Connection Deactivated");
    
    return 0;
}


/*! Deactivates all iSCSI connections so that parameters can be adjusted or
 *  negotiated by the iSCSI daemon.
 *  @param sessionId the session to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::DeactivateAllConnections(SID sessionId)
{
    if(sessionId >= kMaxSessions)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * session = sessionList[sessionId];
    
    if(!session)
        return EINVAL;
    
    errno_t error = 0;
    for(CID connectionId = 0; connectionId < kMaxConnectionsPerSession; connectionId++)
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

    // Set data segment length field
    UInt32 dataSegLength = (OSSwapHostToBigInt32((UInt32)length)>>8);
    memcpy(bhs->dataSegmentLength,&dataSegLength,kiSCSIPDUDataSegmentLengthSize);

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
/*
    if(ahs) {
        iovec[iovecCnt].iov_base = ahs;
//        iovec[iovecCnt].iov_len  = ahsLength;
        iovecCnt++;
    }
*/
    // Leave room for a header digest
    if(connection->opts.useHeaderDigest)    {
        UInt32 headerDigest;
        
        // Compute digest
        headerDigest = crc32c(0,bhs,kiSCSIPDUBasicHeaderSegmentSize);
        
        iovec[iovecCnt].iov_base = &headerDigest;
        iovec[iovecCnt].iov_len  = sizeof(headerDigest);
        iovecCnt++;
    }
 
    // If theres data to send...
    if(length)
    {
        // Add data segment
        iovec[iovecCnt].iov_base = (void*)data;
        iovec[iovecCnt].iov_len  = length;
        iovecCnt++;
        
        // Add padding bytes if required
        UInt32 paddingLen = 4-(length % 4);
        UInt32 padding = 0;
        if(paddingLen != 4)
        {
            iovec[iovecCnt].iov_base  = &padding;
            iovec[iovecCnt].iov_len   = paddingLen;
            iovecCnt++;
        }
 
        // Leave room for a data digest
        if(connection->opts.useDataDigest) {
            UInt32 dataDigest;
            
            // Compute digest
            dataDigest = crc32c(0,data,length);
            
            // Add padding to digest calculation
            if(paddingLen != 4)
                dataDigest = crc32c(dataDigest,&padding,paddingLen);

            iovec[iovecCnt].iov_base = &dataDigest;
            iovec[iovecCnt].iov_len  = sizeof(dataDigest);
            iovecCnt++;
        }
    }
    
    // Update io vector count, send data
    msg.msg_iovlen = iovecCnt;
    size_t bytesSent = 0;
    IOLockLock(connection->PDUIOLock);
    int result = sock_send(connection->socket,&msg,0,&bytesSent);
    IOLockUnlock(connection->PDUIOLock);
    
    return result;
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
    if(connection->opts.useHeaderDigest)
    {
        iovec[iovecCnt].iov_base = &headerDigest;
        iovec[iovecCnt].iov_len  = sizeof(headerDigest);
        iovecCnt++;
    }
    
    msg.msg_iovlen = iovecCnt;
    
    // Bytes received from sock_receive call
    size_t bytesRecv;
    IOLockLock(connection->PDUIOLock);
    errno_t result = sock_receive(connection->socket,&msg,MSG_WAITALL,&bytesRecv);
    IOLockUnlock(connection->PDUIOLock);
    
    if(result != 0)
        DBLog("iSCSI: sock_receive error returned with code %d\n",result);

    // Verify length; incoming PDUS from a target should have no AHS, verify.
    if(bytesRecv < kiSCSIPDUBasicHeaderSegmentSize || bhs->totalAHSLength != 0)
    {
        DBLog("iSCSI: Received incomplete PDU header: %zu\n bytes",bytesRecv);
        return EIO;
    }
    
    // Verify digest if present
    if(headerDigest)
    {
        // Compute digest (should be 0 since we start with the digest)
        if(headerDigest != crc32c(0,bhs,kiSCSIPDUBasicHeaderSegmentSize))
        {
            // Digest failed, log and quit
            IOLog("iSCSI: Failed header digest.\n");
            //return EIO;
        }
    }
    
    // Update command sequence numbers only if the PDU was not a data PDU
    // (unless the data PDU contains a SCSI service response)
    if(bhs->opCode == kiSCSIPDUOpCodeDataIn) {
        iSCSIPDUDataInBHS * bhsDataIn = (iSCSIPDUDataInBHS *)bhs;
        if((bhsDataIn->flags & kiSCSIPDUDataInStatusFlag) == 0)
            return result;
    }

    // Read and update the command sequence numbers
    bhs->maxCmdSN = OSSwapBigToHostInt32(bhs->maxCmdSN);
    bhs->expCmdSN = OSSwapBigToHostInt32(bhs->expCmdSN);
    bhs->statSN = OSSwapBigToHostInt32(bhs->statSN);
    
    if(bhs->maxCmdSN > session->maxCmdSN)
        OSWriteLittleInt32(&session->maxCmdSN,0,bhs->maxCmdSN);
    if(bhs->expCmdSN > session->expCmdSN)
        OSWriteLittleInt32(&session->expCmdSN,0,bhs->expCmdSN);
    
    if(bhs->opCode != kiSCSIPDUOpCodeDataIn || bhs->statSN != 0)
        OSIncrementAtomic(&connection->expStatSN);

    return result;
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
    if(connection->opts.useDataDigest)
    {
        iovec[iovecCnt].iov_base = &dataDigest;
        iovec[iovecCnt].iov_len  = sizeof(dataDigest);
        iovecCnt++;
    }

    msg.msg_iovlen = iovecCnt;
    
    size_t bytesRecv;
    IOLockLock(connection->PDUIOLock);
    errno_t result = sock_receive(connection->socket,&msg,MSG_WAITALL,&bytesRecv);
    IOLockUnlock(connection->PDUIOLock);
    
    // Verify digest if present
    if(connection->opts.useDataDigest)
    {
        // Compute digest including padding...
        UInt32 calcDigest = crc32c(0,data,length);
        
        if(paddingLen != 4)
            calcDigest = crc32c(calcDigest,&padding,paddingLen);
        
        if(dataDigest != calcDigest)
        {
            // Digest failed, log and quit
            IOLog("iSCSI: Failed data digest.\n");
//            return EIO;
        }
    }

    return result;
}
