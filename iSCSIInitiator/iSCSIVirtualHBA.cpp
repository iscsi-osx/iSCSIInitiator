/**
 * @author		Nareg Sinenian
 * @file		iSCSIVirtualHBA.cpp
 * @date		October 13, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 */

#include "iSCSIVirtualHBA.h"
#include "iSCSIIOEventSource.h"

using namespace iSCSIPDU;

/** Maximum number of connections allowed per session. */
const UInt16 iSCSIVirtualHBA::kMaxConnectionsPerSession = 1;

/** Maximum number of session allowed (globally). */
const UInt16 iSCSIVirtualHBA::kMaxSessions = 16;

/** Highest LUN supported by the virtual HBA.  Due to internal design 
 *  contraints, this number should never exceed 2**24-1 (24-bits). */
const SCSILogicalUnitNumber iSCSIVirtualHBA::kHighestLun = 64;

/** Highest SCSI device ID supported by the HBA. */
const SCSIDeviceIdentifier iSCSIVirtualHBA::kHighestSupportedDeviceId = kMaxSessions - 1;

/** Maximum number of SCSI tasks the HBA can handle. */
const UInt32 iSCSIVirtualHBA::kMaxTaskCount = 10;

/** Amount of memory (in bytes) required for a single SCSI task. */
//const UInt32 iSCSIVirtualHBA::kTaskDataSize = 10;

/** Amount of memory (in bytes) required for each target device. */
//const UInt32 iSCSIVirtualHBA::kDeviceDataSize = 10;


/** Definition of a single connection that is associated with a particular
 *  iSCSI session. */
typedef struct iSCSIConnection {
    
    /** Status sequence number last received from the target. */
    UInt32 statSN;
    
    /** Status sequence number expected by the initiator. */
    UInt32 expStatSN;
    
    /** Connection ID. */
    UInt32 CID;
    
    /** Initiator tag for current task. */
    UInt32 initiatorTaskTag;
    
    /** Target tag for current transfer. */
    UInt32 targetTransferTag;
    
    /** Socket used for communication. */
    socket_t socket;
    
    /** Used to keep track of data PDUs. */
//    UInt32 dataSN;
    
    /** Used to keep track of R2T PDUs. */
    UInt32 R2TSN;
    
    /** Event source used to signal the Virtual HBA that data has been 
     *  received and needs to be processed. */
    iSCSIIOEventSource * eventSource;
    
    /** Options associated with this connection. */
    iSCSIConnectionOptions opts;
    
    /** Flag that indicates whether this connection is active.  An active
     *  connection is one where the user-space code has performed login
     *  and negotiation and placed that connection into full-feature phase. */
    bool active;

    
} iSCSIConnection;

/** Definition of a single iSCSI session.  Each session is comprised of one
 *  or more connections as defined by the struct iSCSIConnection.  Each session
 *  is further associated with an initiator session ID (ISID), a target session
 *  ID (TSIH), a target IP address, a target name, and a target alias. */
typedef struct iSCSISession {
    
    /** The initiator session ID, which is also used as the target ID within
     *  this kernel extension since there is a 1-1 mapping. */
    UInt16 sessionId;
    
    /** The target session identifying handle. */
    UInt16 TSIH;
    
    /** Command sequence number to be used for the next initiator command. */
    UInt32 cmdSN;
    
    /** Command seqeuence number expected by the target. */
    UInt32 expCmdSN;

    /** Connections associated with this session. */
    iSCSIConnection * * connections;
    
    /** Options associated with this session. */
    iSCSISessionOptions opts;
    
    /** Number of active connections. */
    UInt32 activeConnections;
    
    /** Indicates whether session is active, which means that a SCSI target
     *  exists and is backing the the iSCSI session. */
    bool active;
    
} iSCSISession;


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
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTask;
    bhs.referencedTaskTag = OSSwapHostToBigInt32((UInt32)taggedTaskID);
    bhs.initiatorTaskTag = (UInt32)(LUN + kMaxTaskCount);
    bhs.initiatorTaskTag |= ((UInt32)kiSCSIPDUTaskMgmtFuncAbortTask)<<24;
    bhs.initiatorTaskTag = OSSwapHostToBigInt32(bhs.initiatorTaskTag);
    
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
 //   SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    
    IOLog("abort task request");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::AbortTaskSetRequest(SCSITargetIdentifier targetId,
														 SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTaskSet;
    bhs.initiatorTaskTag = (UInt32)(LUN + kMaxTaskCount);
    bhs.initiatorTaskTag |= ((UInt32)kiSCSIPDUTaskMgmtFuncAbortTaskSet)<<24;
    bhs.initiatorTaskTag = OSSwapHostToBigInt32(bhs.initiatorTaskTag);
    
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
//    SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    
        IOLog("abort task set request");
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::ClearACARequest(SCSITargetIdentifier targetId,
													 SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearACA;
    bhs.initiatorTaskTag = (UInt32)(LUN + kMaxTaskCount);
    bhs.initiatorTaskTag |= ((UInt32)kiSCSIPDUTaskMgmtFuncClearACA)<<24;
    bhs.initiatorTaskTag = OSSwapHostToBigInt32(bhs.initiatorTaskTag);

    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
        IOLog("clear ACA request");
    
//    SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::ClearTaskSetRequest(SCSITargetIdentifier targetId,
														 SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearTaskSet;
    bhs.initiatorTaskTag = (UInt32)(LUN + kMaxTaskCount);
    bhs.initiatorTaskTag |= ((UInt32)kiSCSIPDUTaskMgmtFuncClearTaskSet)<<24;
    bhs.initiatorTaskTag = OSSwapHostToBigInt32(bhs.initiatorTaskTag);
    
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
        IOLog("clear task set request");
//    SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::LogicalUnitResetRequest(SCSITargetIdentifier targetId,
															 SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncLUNReset;
    bhs.initiatorTaskTag = (UInt32)(LUN + kMaxTaskCount);
    bhs.initiatorTaskTag |= ((UInt32)kiSCSIPDUTaskMgmtFuncLUNReset)<<24;
    bhs.initiatorTaskTag = OSSwapHostToBigInt32(bhs.initiatorTaskTag);
    
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    IOLog("LUN reset request");
    
 //   SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::TargetResetRequest(SCSITargetIdentifier targetId)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncTargetWarmReset;
    bhs.initiatorTaskTag = (UInt32)(kMaxTaskCount);
    bhs.initiatorTaskTag |= ((UInt32)kiSCSIPDUTaskMgmtFuncTargetWarmReset)<<24;
    bhs.initiatorTaskTag = OSSwapHostToBigInt32(bhs.initiatorTaskTag);
    
    // Grab session and connection, send task managment request
    iSCSISession * session = sessionList[targetId];
    if(session == NULL)
        return kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    IOLog("target reset request");
    
//    SendPDU(session,session->connections[0],(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIInitiatorIdentifier iSCSIVirtualHBA::ReportInitiatorIdentifier()
{
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
	return 1;
}

UInt32 iSCSIVirtualHBA::ReportHBASpecificDeviceDataSize()
{
	return 1;
}

bool iSCSIVirtualHBA::DoesHBAPerformDeviceManagement()
{
	// Lets the superclass know that we are going to create and destroy
	// our own targets
	return true;
}

bool iSCSIVirtualHBA::InitializeController()
{
    IOLog("Begin init");
    // Setup session list
    
    sessionList = (iSCSISession **)IOMalloc(kMaxSessions*sizeof(iSCSISession*));//[kMaxSessions];
    
    if(!sessionList)
        return false;
    
    IOLog("End init");
    
    memset(sessionList,0,kMaxSessions*sizeof(iSCSISession *));

    // Setup a task to accept and hold parallel tasks received from
    // the SCSI subsystem
    taskArray = OSArray::withCapacity(kMaxTaskCount);
	
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
    // Stop all tasks and free task array
    for(UInt16  index = 0; index< kMaxTaskCount; index++) {
        
        // Grab task from our task list
        SCSIParallelTaskIdentifier parallelTask = ((SCSIParallelTaskIdentifier)taskArray->getObject(index));
        
        if(!parallelTask)
            continue;
        
        // Let SAM know that we couldn't finish this task
        CompleteParallelTask(parallelTask,kSCSITaskStatus_DeviceNotPresent,
                             kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE);
    }
    
    // Free up our list of parallel tasks */
    taskArray->free();

    // Go through every connection for each session, and close sockets,
    // remove event sources, etc
    for(UInt16 index = 0; index < kMaxSessions; index++)
    {
        if(!sessionList[index])
            continue;
        
        ReleaseSession(index);
    }
    IOFree(sessionList,kMaxSessions*sizeof(iSCSISession*));
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

SCSIServiceResponse iSCSIVirtualHBA::ProcessParallelTask(SCSIParallelTaskIdentifier parallelTask)
{
    // Extract information about this SCSI task
    SCSITargetIdentifier targetId   = GetTargetIdentifier(parallelTask);
    SCSILogicalUnitNumber LUN       = GetLogicalUnitNumber(parallelTask);
    SCSITaskAttribute attribute     = GetTaskAttribute(parallelTask);
    SCSITaggedTaskIdentifier taskId = GetTaggedTaskIdentifier(parallelTask);
    UInt8   transferDirection       = GetDataTransferDirection(parallelTask);
    UInt32  transferSize            = (UInt32)GetRequestedDataTransferCount(parallelTask);
    UInt8   cdbSize                 = GetCommandDescriptorBlockSize(parallelTask);

    // Create a SCSI request PDU
    iSCSIPDUSCSICmdBHS bhs  = iSCSIPDUSCSICmdBHSInit;
    bhs.dataTransferLength  = OSSwapHostToBigInt32(transferSize);
    bhs.LUN                 = OSSwapHostToBigInt64(LUN);
    bhs.initiatorTaskTag    = OSSwapHostToBigInt32((UInt32)taskId);
    
    if(transferDirection == kSCSIDataTransfer_FromInitiatorToTarget)
        bhs.flags |= kiSCSIPDUSCSICmdFlagWrite;
    else
        bhs.flags |= kiSCSIPDUSCSICmdFlagRead;
    
    // For CDB sizes less than 16 bytes, plug directly into SCSI command PDU
    // (Currently, Mac OS X doesn't support CDB's larger than 16 bytes, so
    // there's no need for additional header segment to contain spillover).
    switch(cdbSize) {
        case kSCSICDBSize_6Byte:
        case kSCSICDBSize_10Byte:
        case kSCSICDBSize_12Byte:
        case kSCSICDBSize_16Byte:
            GetCommandDescriptorBlock(parallelTask,&bhs.CDB);
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
    
    // Target id and session identifier are one and the same
    iSCSISession * session = sessionList[(UInt16)targetId];
    
    if(!session)
        return kSCSIServiceResponse_FUNCTION_REJECTED;
    
    iSCSIConnection * conn = session->connections[0];
    
    if(!conn)
        return kSCSIServiceResponse_FUNCTION_REJECTED;
    
    // Store the parallel task in our task array (indexed by its task id or
    // equivalently the initiator task tag).
    taskArray->setObject((UInt32)taskId,parallelTask);
    
    // Sets the timeout in milliseconds for processing the current task
//    SetTimeoutForTask(parallelTask,100);
    
    // For non-WRITE commands, send off SCSI command PDU immediately.
    if(transferDirection != kSCSIDataTransfer_FromInitiatorToTarget)
    {
        bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;
        SendPDU(session,conn,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
        return kSCSIServiceResponse_Request_In_Process;
    }
    
    // For SCSI WRITE command PDUs, send PDU if initial R2T examine initial R2T and immediate data
    // options for the session
    IOMemoryDescriptor  * dataDesc  = GetDataBuffer(parallelTask);
    IOMemoryMap         * dataMap   = dataDesc->map();
    void                * data      = (void *)dataMap->getAddress();
    
    // Prefer to use immediate data for writes if possible
    if(session->opts.immediateData)
        SendPDU(session,conn,(iSCSIPDUInitiatorBHS *)&bhs,NULL,data,session->opts.firstBurstLength);
    // If immediate data can't be used, follow up with a data out PDU
    else if(session->opts.initialR2T)
        SendPDU(session,conn,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    // Else just send off the write command
    else {
        bhs.flags |= kiSCSIPDUSCSICmdFlagNoUnsolicitedData;
        SendPDU(session,conn,(iSCSIPDUInitiatorBHS *)&bhs,NULL,NULL,0);
    }
    
	return kSCSIServiceResponse_Request_In_Process;
}

void iSCSIVirtualHBA::CompleteTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                                   iSCSISession * session,
                                                   iSCSIConnection * connection)
{
    // Quit if the connection isn't active (if it is not in full feature phase)
    if(!owner || !session || !connection || !connection->active)
        return;
    
    // Grab incoming bhs (we are guaranteed to have a basic header at this
    // point (iSCSIIOEventSource ensures that this is the case)
    iSCSIPDUTargetBHS bhs;
    if(owner->RecvPDUHeader(session,connection,&bhs) != 0)
        return;
    
    // Determine the kind of PDU that was received and process accordingly
    enum iSCSIPDUTargetOpCodes opCode = (iSCSIPDUTargetOpCodes)bhs.opCode;
    switch(opCode)
    {
        // Process a SCSI response
        case kiSCSIPDUOpCodeSCSIRsp:
            owner->ProcessSCSIResponse(session,connection,(iSCSIPDUSCSIRspBHS*)&bhs);
        break;
        case kiSCSIPDUOpCodeDataIn:
 //           owner->ProcessDataIn(session,connection,(iSCSIPDUDataInBHS*)&bhs);
        break;
        case kiSCSIPDUOpCodeAsyncMsg:
            break;
        case kiSCSIPDUOpCodeNOPIn:
            break;
            
        case kiSCSIPDUOpCodeR2T:
   //         owner->ProcessR2T(session,connection,(iSCSIPDUR2TBHS*)&bhs);
            break;
        case kiSCSIPDUOpCodeReject:
            break;
            
        case kiSCSIPDUOpCodeTaskMgmtRsp:
        //iSCSIPDUTargetMgmtRspBHS * rsp = (iSCSIPDUTargetMgmtRspBHS *)&bhs;
//        owner->CompleteTaskMgmtRequest(session,connection,rsp->response,taskTag);
                break;
            
        // Catch-all for anything else...
        default: break;
    };
}


/** Helper function used to complete a task management request.
 *  This function is called by CompleteTaskOnWorkloopThread(). */
void iSCSIVirtualHBA::ProcessTaskMgmtResponse(iSCSISession * session,
                                              iSCSIConnection * connection,
                                              UInt8 taskMgmtResponse,
                                              UInt32 initiatorTaskTag)
{
    
// NEED TO ENCODE THE ORIGINAL INITIATOR TASK ID SO WE CAN REMOVE IT FROM
// OUR TASK QUEUUE....
    
    
    // Extract LUN and function code from task tag
    UInt8 function = (initiatorTaskTag & 0xFF000000)>>24;
    UInt32 LUN = (initiatorTaskTag & 0x00FFFFFF) - kMaxTaskCount;
    
    // Convert resposne to a SCSI service response
//    if(taskMgmtResponse == ) {}
    

    if(function == kiSCSIPDUTaskMgmtFuncAbortTask)
    {
    
    }
    
    else if (function == kiSCSIPDUTaskMgmtFuncAbortTaskSet) {}
    else if (function == kiSCSIPDUTaskMgmtFuncClearACA) {}
    else if (function == kiSCSIPDUTaskMgmtFuncClearTaskSet) {}
    else if (function == kiSCSIPDUTaskMgmtFuncLUNReset) {}
    else if (function == kiSCSIPDUTaskMgmtFuncTargetWarmReset) {}
}


void iSCSIVirtualHBA::ProcessSCSIResponse(iSCSISession * session,
                                          iSCSIConnection * connection,
                                          iSCSIPDU::iSCSIPDUSCSIRspBHS * bhs)
{
    const UInt8 senseDataHeaderSize = 2;
    
    // Padded length of the data segment of the PDU
    size_t length = iSCSIPDUGetDataSegmentLength((iSCSIPDUCommonBHS*)&bhs);
    
    // Receive PDU data into a sense buffer
    if(length < senseDataHeaderSize) {
        IOLog("Recv SCSI rsp length 0");
        return;
    }
    
    UInt8 data[length];

    RecvPDUData(session,connection,data,length);
    
    IOLog("iSCSI: Received Sense Data");

    // Grab parallel task associated with this PDU, indexed by task tag
    UInt32 taskTag = OSSwapBigToHostInt32(bhs->initiatorTaskTag);
    
    if(taskTag >= kMaxTaskCount)
        return;
    
    SCSIParallelTaskIdentifier parallelTask =
        (SCSIParallelTaskIdentifier)taskArray->getObject(taskTag);
    
    if(!parallelTask)
        return;

    // First two bytes of the data segment are the size of the sense data
    UInt16 senseDataLength = *((UInt16*)&data[0]);
    senseDataLength = OSSwapBigToHostInt16(senseDataLength);
    
    if(length < senseDataLength + senseDataHeaderSize) {
        IOLog("iSCSI: Received sense data invalid");
    }
    
    // Remaining data is sense data, advance pointer by two bytes to get this
    SCSI_Sense_Data * newSenseData = (SCSI_Sense_Data *)(data + senseDataHeaderSize);
    
    
    IOLog("iSCSI: Processed sesne data");

 
/// something above caused crash
    // Incorporate sense data into the task
    SetAutoSenseData(parallelTask,newSenseData,senseDataLength);

    
    // Set the SCSI completion status and service response
    SCSITaskStatus completionStatus = (SCSITaskStatus)bhs->status;
    SCSIServiceResponse serviceResponse;

    if(bhs->response == kiSCSIPDUSCSICmdCompleted)
        serviceResponse = kSCSIServiceResponse_TASK_COMPLETE;
    else
        serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    
    CompleteParallelTask(parallelTask,completionStatus,serviceResponse);
}

void iSCSIVirtualHBA::ProcessDataIn(iSCSISession * session,
                                    iSCSIConnection * connection,
                                    iSCSIPDU::iSCSIPDUDataInBHS * bhs)
{
    // Length of the data segment of the PDU
    size_t length = iSCSIPDUGetDataSegmentLength((iSCSIPDUCommonBHS*)&bhs);
    
    IOLog("iSCSI: Received data in PDU");
    
    // Grab parallel task associated with this PDU, indexed by task tag
    UInt32 taskTag = OSSwapBigToHostInt32(bhs->initiatorTaskTag);
    
    SCSIParallelTaskIdentifier parallelTask =
        (SCSIParallelTaskIdentifier)taskArray->getObject(taskTag);
    
    // Create a mapping to the task's data buffer
    IOMemoryDescriptor  * dataDesc  = GetDataBuffer(parallelTask);
    IOMemoryMap         * dataMap   = dataDesc->map();
    UInt8               * data      = (UInt8 *)dataMap->getAddress();

    // Write data received into the parallelTask data structure
    UInt32 dataOffset = OSSwapBigToHostInt32(bhs->bufferOffset);
    data = data + dataOffset;
    if(dataOffset + length <= dataMap->getLength())
        RecvPDUData(session,connection,data,length);
    
    // Release the mapping object (this leaves the descriptor and buffer intact)
    dataMap->release();
}

void iSCSIVirtualHBA::ProcessR2T(iSCSISession * session,
                                 iSCSIConnection * connection,
                                 iSCSIPDU::iSCSIPDUR2TBHS * bhs)
{
    IOLog("iSCSI: Received R2T PDU");
    
    // Grab parallel task associated with this PDU, indexed by task tag
    UInt32 taskTag = OSSwapBigToHostInt32(bhs->initiatorTaskTag);
    
    SCSIParallelTaskIdentifier parallelTask =
        (SCSIParallelTaskIdentifier)taskArray->getObject(taskTag);
    
    // Create a mapping to the task's data buffer.  This is the data that
    // we will read and pack into a sequence of PDUs to send to the target.
    IOMemoryDescriptor  * dataDesc   = GetDataBuffer(parallelTask);
    IOMemoryMap         * dataMap    = dataDesc->map();
    UInt8               * data       = (UInt8 *)dataMap->getAddress();

    // Obtain requested data offset and requested lengths
    UInt32 dataOffset         = OSSwapBigToHostInt32(bhs->bufferOffset);
    UInt32 desiredDataLength  = OSSwapBigToHostInt32(bhs->desiredDataLength);

    // Ensure that our data buffer contains all of the requested data
    if(dataOffset + desiredDataLength > (UInt32)dataMap->getLength())
    {
        dataMap->release();
        return;
    }
    
    // Amount of data to transfer in each iteration
    UInt32 maxTransferLength = connection->opts.maxSendDataSegmentLength;
    
    // Create data PDUs and send them until all desired data has been sent
    iSCSIPDUDataOutBHS bhsDataOut = iSCSIPDUDataOutBHSInit;
    bhsDataOut.LUN = OSSwapHostToBigInt64(GetLogicalUnitNumber(parallelTask));
    bhsDataOut.initiatorTaskTag = OSSwapHostToBigInt32((UInt32)GetTaggedTaskIdentifier(parallelTask));
    
    // Let target know that this data out sequence is in response to the
    // transfer tag the target gave us with the R2TSN (both in high-byte order)
    bhsDataOut.targetTransferTag = bhs->targetTransferTag;
    
    UInt32 dataSN = 0;

    while(desiredDataLength != 0)
    {
        bhsDataOut.bufferOffset = OSSwapHostToBigInt32(dataOffset);
        bhsDataOut.dataSN = OSSwapHostToBigInt32(dataSN);
        
        if(desiredDataLength > maxTransferLength) {
            SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsDataOut,NULL,data,maxTransferLength);
            
            desiredDataLength   -= maxTransferLength;
            data                += maxTransferLength;
            dataOffset          += maxTransferLength;
        }
        // This is the final PDU of the sequence
        else {
            bhsDataOut.flags = kiSCSIPDUDataOutFinalFlag;
            SendPDU(session,connection,(iSCSIPDUInitiatorBHS*)&bhsDataOut,NULL,data,desiredDataLength);
            break;
        }
        
        // Increment the data sequence number
        dataSN++;
    }
    // Release the mapping object (this leaves the descriptor and buffer intact)
    dataMap->release();
}

//////////////////////////////// ISCSI FUNCTIONS ///////////////////////////////

/** Allocates a new iSCSI session and returns a session qualifier ID.
 *  @return a valid session qualifier (part of the ISID, see RF3720) or
 *  0 if a new session could not be created. */
UInt16 iSCSIVirtualHBA::CreateSession()
{
    // Find an open session slot
    UInt16 sessionId;
    for(sessionId = 0; sessionId < kMaxSessions; sessionId++)
        if(!sessionList[sessionId])
            break;
    
    // If no slots were available...
    if(sessionId == kMaxSessions)
        return kiSCSIInvalidSessionId;
    
    // Alloc new session, validate
    iSCSISession * newSession = (iSCSISession*)IOMalloc(sizeof(iSCSISession));
    if(!newSession)
        return kiSCSIInvalidSessionId;

    // Setup connections array for new session
    newSession->connections = (iSCSIConnection **)IOMalloc(kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    
    if(!newSession->connections) {
        IOFree(newSession,sizeof(iSCSISession));
        return kiSCSIInvalidSessionId;
    }
    
    // Reset all connections
    memset(newSession->connections,0,kMaxConnectionsPerSession*sizeof(iSCSIConnection*));
    
    // Setup session parameters with defaults
    newSession->sessionId = sessionId;
    newSession->activeConnections = 0;
    newSession->cmdSN = 0;
    newSession->expCmdSN = 0;
    newSession->active = false;
    newSession->TSIH = 0;
    
    // Retain new session
    sessionList[sessionId] = newSession;

    return sessionId;
}

/** Releases an iSCSI session, including all connections associated with that
 *  session.
 *  @param sessionId the session qualifier part of the ISID. */
void iSCSIVirtualHBA::ReleaseSession(UInt16 sessionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions)
        return;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return;
    
    // Disconnect all active connections
    for(UInt32 index = 0; index < kMaxConnectionsPerSession; index++)
        ReleaseConnection(sessionId,index);
        
    IOFree(sessionList[sessionId], sizeof(iSCSIConnection*));
    sessionList[sessionId] = NULL;
}

/** Activates an iSCSI session that has been properly configured.
 *  @param sessionId the session to activate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::ActivateSession(UInt16 sessionId)
{
    if(sessionId >= kMaxSessions)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
   
    // Ensure that the session has at least one active connection
    if(theSession->activeConnections == 0 || !CreateTargetForID(sessionId))
            return EINVAL;
    
    theSession->active = true;
    return 0;
}

/** Deactivates an iSCSI session that has been properly configured.
 *  @param sessionId the session to activate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::DeactivateSession(UInt16 sessionId)
{
    if(sessionId >= kMaxSessions)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;

    DestroyTargetForID(sessionId);

    theSession->active = false;
    return 0;
}

/** Sets options associated with a particular session.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::SetSessionOptions(UInt16 sessionId,
                                           iSCSISessionOptions * options)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || !options)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession;
    if(!(theSession = sessionList[sessionId]))
       return EINVAL;
    
    // Copy options into the session struct
    theSession->opts = *options;

    // Success
    return 0;
}

/** Gets options associated with a particular session.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::GetSessionOptions(UInt16 sessionId,
                                           iSCSISessionOptions * options)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || !options)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession;
    if(!(theSession = sessionList[sessionId]))
        return EINVAL;
    
    // Copy session options to options struct
    *options = theSession->opts;
    
    // Success
    return 0;
}

/** Allocates a new iSCSI connection associated with the particular session.
 *  @param sessionId the session to create a new connection for.
 *  @param domain the IP domain (e.g., AF_INET or AF_INET6).
 *  @param address the BSD socket structure used to identify the target.
 *  @return a connection ID, or 0 if a connection could not be created. */
errno_t iSCSIVirtualHBA::CreateConnection(UInt16 sessionId,
                                          int domain,
                                          const struct sockaddr * targetAddress,
                                          const struct sockaddr * hostAddress,
                                          UInt32 * connectionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || !targetAddress || !hostAddress || !connectionId)
        return EINVAL;
    
    // Retrieve the session from the session list, validate
    iSCSISession * theSession = sessionList[sessionId];
    if(!theSession)
        return EINVAL;
    
    // Find an empty connection slot to use for a new connection
    UInt32 index;
    for(index = 0; index < kMaxConnectionsPerSession; index++)
        if(!theSession->connections[index])
            break;
    
    // If empty slot wasn't found tell caller to try again later
    if(index == kMaxConnectionsPerSession)
        return EAGAIN;

    // Create a new connection
    iSCSIConnection * newConn = (iSCSIConnection*)IOMalloc(sizeof(iSCSIConnection));
    if(!newConn)
        return EAGAIN;
    
    // Sockets connected and bound add event source to driver workloop
    newConn->active = false;
    theSession->connections[index] = newConn;
    *connectionId = index;
    
    // Initialize default error (try again)
    errno_t error = EAGAIN;
    
    if(!(newConn->eventSource = OSTypeAlloc(iSCSIIOEventSource)))
        goto EVENTSOURCE_ALLOC_FAILURE;
    
    // Initialize event source, quit if it fails
    if(!newConn->eventSource->init(this,(iSCSIIOEventSource::Action)&CompleteTaskOnWorkloopThread,theSession,newConn))
        goto EVENTSOURCE_INIT_FAILURE;
    
    if(GetWorkLoop()->addEventSource(newConn->eventSource) != kIOReturnSuccess)
        goto EVENTSOURCE_ADD_FAILURE;
        
    // Create a new socket (per RFC3720, only TCP sockets are used.
    // Domain can vary between IPv4 or IPv6.
    if((error = sock_socket(domain,SOCK_STREAM,IPPROTO_TCP,(sock_upcall)&iSCSIIOEventSource::socketCallback,
                        newConn->eventSource,&newConn->socket)))
        goto SOCKET_CREATE_FAILURE;

    // Connect the socket to the target node
    if((error = sock_connect(newConn->socket,targetAddress,0)))
        goto SOCKET_CONNECT_FAILURE;
    
    // Bind socket to a particular host connection
//    if((error = sock_bind(newConn->socket,hostAddress)))
  //      goto SOCKET_BIND_FAILURE;

    return 0;
    
SOCKET_BIND_FAILURE:
    theSession->connections[index] = 0;
    
    
SOCKET_CONNECT_FAILURE:
    sock_close(newConn->socket);
    
SOCKET_CREATE_FAILURE:
    GetWorkLoop()->removeEventSource(newConn->eventSource);
    
EVENTSOURCE_ADD_FAILURE:
    
EVENTSOURCE_INIT_FAILURE:
    newConn->eventSource->release();
        
EVENTSOURCE_ALLOC_FAILURE:
    IOFree(newConn,sizeof(iSCSIConnection));

    return error;
}

/** Frees a given iSCSI connection associated with a given session.
 *  The session should be logged out using the appropriate PDUs. */
void iSCSIVirtualHBA::ReleaseConnection(UInt16 sessionId,
                                        UInt32 connectionId)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return;

    iSCSIConnection * theConn = theSession->connections[connectionId];
        
    if(!theConn)
        return;
    
    IOLog("iSCSI: Stopped Connection");

    GetWorkLoop()->removeEventSource(theConn->eventSource);
    sock_close(theConn->socket);
    theConn->eventSource->release();
    IOFree(theConn,sizeof(iSCSIConnection));
    theSession->connections[connectionId] = NULL;
}

/** Activates an iSCSI connection, indicating to the kernel that the iSCSI
 *  daemon has negotiated security and operational parameters and that the
 *  connection is in the full-feature phase.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::ActivateConnection(UInt16 sessionId,UInt32 connectionId)
{
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    // Do nothing if connection doesn't exist
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;
    
    theConn->active = true;
    theSession->activeConnections++;
    return 0;
}

/** Deactivates an iSCSI connection so that parameters can be adjusted or
 *  negotiated by the iSCSI daemon.
 *  @param sessionId the session to deactivate.
 *  @param connectionId the connection to deactivate.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::DeactivateConnection(UInt16 sessionId,UInt32 connectionId)
{
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    // Do nothing if connection doesn't exist
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;

    theConn->active = false;
    theSession->activeConnections--;

    return 0;
}

/** Sends data over a kernel socket associated with iSCSI.  If the specified 
 *  data segment length is not a multiple of 4-bytes, padding bytes will be 
 *  added to the data segment of the PDU per RF3720 specification.
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
                                 void * data,
                                 size_t length)
{
    // Range-check inputs
    if(!session || !connection || !bhs)
        return EINVAL;
    
    // Set the command sequence number & expected status sequence number
    bhs->cmdSN = OSSwapHostToBigInt32(session->cmdSN);
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
/*
    // Leave room for a header digest
    if(theConn->opts.useHeaderDigest)    {
        UInt32 headerDigest;
        iovec[iovecCnt].iov_base = &headerDigest;
        iovec[iovecCnt].iov_len  = sizeof(headerDigest);
        iovecCnt++;
    }
 */
    if(data)
    {
        // Add data segment
        iovec[iovecCnt].iov_base = data;
        iovec[iovecCnt].iov_len  = length;
        iovecCnt++;
        
        // Add padding bytes if required
        UInt32 paddingLen = 4-(length % 4);
        if(paddingLen != 4)
        {
            UInt32 padding;
            iovec[iovecCnt].iov_base  = &padding;
            iovec[iovecCnt].iov_len   = paddingLen;
            iovecCnt++;
        }
 /*
        // Leave room for a data digest
        if(theConn->opts.useDataDigest) {
            UInt32 headerDigest;
            iovec[iovecCnt].iov_base = &headerDigest;
            iovec[iovecCnt].iov_len  = sizeof(headerDigest);
            iovecCnt++;
        }*/
    }
    // Update io vector count, send data
    msg.msg_iovlen = iovecCnt;
    size_t bytesSent = 0;
    return sock_send(connection->socket,&msg,0,&bytesSent);
}


/** Receives a basic header segment over a kernel socket.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::RecvPDUHeader(iSCSISession * session,
                                       iSCSIConnection * connection,
                                       iSCSIPDUTargetBHS * bhs)
{
    // Range-check inputs
    if(!session || !connection || !bhs)
        return EINVAL;
    
    // Receive data over the network
    struct msghdr msg;
    struct iovec  iovec;
    memset(&msg,0,sizeof(struct msghdr));

    msg.msg_iov     = &iovec;
    msg.msg_iovlen  = 1;
    iovec.iov_base  = bhs;
    iovec.iov_len   = kiSCSIPDUBasicHeaderSegmentSize;
    
    // Bytes received from sock_receive call
    size_t bytesRecv;
    errno_t result = sock_receive(connection->socket,&msg,0,&bytesRecv);

    // Verify length; incoming PDUS from a target should have no AHS, verify.
    if(bytesRecv < kiSCSIPDUBasicHeaderSegmentSize || bhs->totalAHSLength != 0)
        return EIO;
    
    return result;
}

/** Receives a data segment over a kernel socket.  If the specified length is 
 *  not a multiple of 4-bytes, the padding bytes will be discarded per 
 *  RF3720 specification (all data segment are multiples of 4 bytes).
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param data the data received.
 *  @param length the length of the data buffer.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::RecvPDUData(iSCSISession * session,
                                     iSCSIConnection * connection,
                                     void * data,
                                     size_t length)
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
    if(paddingLen != 4)
    {
       UInt32 padding;
       iovec[iovecCnt].iov_base  = &padding;
       iovec[iovecCnt].iov_len   = paddingLen;
       iovecCnt++;
    }

    msg.msg_iovlen = iovecCnt;
    
    size_t bytesRecv;
    errno_t result = sock_receive(connection->socket,&msg,0,&bytesRecv);
    
    
    // I
/*
    // Strip data digest next if digest was used
    if(theConn->opts.useDataDigest) {
        UInt32 dataDigest;
        iovec.iov_base = &dataDigest;
        iovec.iov_len  = sizeof(dataDigest);
        sock_receive(theConn->socket,&msg,0,&bytesRecv);
    }*/
    return result;
}


/** Wrapper around SendPDU for user-space calls.
 *  Sends data over a kernel socket associated with iSCSI.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment to send.
 *  @param data the data segment to send.
 *  @param length the byte size of the data segment
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::SendPDUUser(UInt16 sessionId,
                                     UInt32 connectionId,
                                     iSCSIPDUInitiatorBHS * bhs,
                                     void * data,
                                     size_t dataLength)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;
    
    return SendPDU(theSession,theConn,bhs,NULL,data,dataLength);
}

/** Wrapper around RecvPDUHeader for user-space calls.
 *  Receives a basic header segment over a kernel socket.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param bhs the basic header segment received.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::RecvPDUHeaderUser(UInt16 sessionId,
                                           UInt32 connectionId,
                                           iSCSIPDUTargetBHS * bhs)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;
    
    return RecvPDUHeader(theSession,theConn,bhs);
}

/** Wrapper around RecvPDUData for user-space calls.
 *  Receives a data segment over a kernel socket.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param data the data received.
 *  @param length the length of the data buffer.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::RecvPDUDataUser(UInt16 sessionId,
                                         UInt32 connectionId,
                                         void * data,
                                         size_t length)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession)
        return EINVAL;
    
    // Do nothing if session doesn't exist
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;
    
    return RecvPDUData(theSession,theConn,data,length);
}

/** Sets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to set.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::SetConnectionOptions(UInt16 sessionId,
                                              UInt32 connectionId,
                                              iSCSIConnectionOptions * options)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession || !options)
        return EINVAL;
    
    // Grab handle to session
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    // Grab handle to connection
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;
    
    // Copy options into the connection struct
    theConn->opts = *options;
    
    // Success
    return 0;
}

/** Gets options associated with a particular connection.
 *  @param sessionId the qualifier part of the ISID (see RFC3720).
 *  @param connectionId the connection associated with the session.
 *  @param options the options to get.  The user of this function is
 *  responsible for allocating and freeing the options struct.
 *  @return error code indicating result of operation. */
errno_t iSCSIVirtualHBA::GetConnectionOptions(UInt16 sessionId,
                                              UInt32 connectionId,
                                              iSCSIConnectionOptions * options)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || connectionId >= kMaxConnectionsPerSession || !options)
        return EINVAL;
    
    // Grab handle to session
    iSCSISession * theSession = sessionList[sessionId];
    
    if(!theSession)
        return EINVAL;
    
    // Grab handle to connection
    iSCSIConnection * theConn = theSession->connections[connectionId];
    
    if(!theConn)
        return EINVAL;
    
    // Copy connection options to options struct
    *options = theConn->opts;
    
    // Success
    return 0;
}
