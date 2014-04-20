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
const UInt16 iSCSIVirtualHBA::kMaxConnectionsPerSession = 5;

/** Maximum number of session allowed (globally). */
const UInt16 iSCSIVirtualHBA::kMaxSessions = 10;

/** Highest LUN supported by the virtual HBA. */
const SCSILogicalUnitNumber iSCSIVirtualHBA::kHighestLun = 10;

/** Initiator ID of the virtual HBA. */
const SCSIInitiatorIdentifier iSCSIVirtualHBA::kInitiatorId = 0x0;

/** Highest SCSI device ID supported by the HBA. */
const SCSIDeviceIdentifier iSCSIVirtualHBA::kHighestSupportedDeviceId = 0xF;

/** Maximum number of SCSI tasks the HBA can handle. */
const UInt32 iSCSIVirtualHBA::kMaxTaskCount = 10;

/** Amount of memory (in bytes) required for a single SCSI task. */
const UInt32 iSCSIVirtualHBA::kTaskDataSize = 10;

/** Amount of memory (in bytes) required for each target device. */
const UInt32 iSCSIVirtualHBA::kDeviceDataSize = 10;

/** Used to queue command and data PDUs for each connection. */
typedef struct iSCSIQueue {
    iSCSIPDUInitiatorBHS * bhs;
    iSCSIPDUCommonAHS    * ahs;
    void * data;
    size_t dataLength;
    iSCSIQueue * next;
} iSCSIQueue;


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
    
    /** Event source used to signal the Virtual HBA that data has been 
     *  received and needs to be processed. */
    iSCSIIOEventSource * eventSource;
    
    /** Options associated with this connection. */
    iSCSIConnectionOptions opts;
    
    /** Queue of outgoing PDUs associated with this connection. */
    iSCSIQueue * next;
    
} iSCSIConnection;

/** Definition of a single iSCSI session.  Each session is comprised of one
 *  or more connections as defined by the struct iSCSIConnection.  Each session
 *  is further associated with an initiator session ID (ISID), a target session
 *  ID (TSIH), a target IP address, a target name, and a target alias. */
typedef struct iSCSISession {
    
    /** The initiator session ID. */
    UInt8 ISIDa;
    
    /** The initiator session ID. */
    UInt16 ISIDb;
    
    /** The initiator session ID. */
    UInt8 ISIDc;
    
    /** The initiator session ID - qualifier. */
    UInt16 ISIDd;
    
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
//    IOSCSIParallelInterfaceDevice * target = GetTargetForID(targetId);

    
	
//	target->setProperty(<#const OSSymbol *aKey#>, <#OSObject *anObject#>)
//	IOSCSITargetDevice::Create(target);
//	IOLog("init target");
	
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


    
	return kSCSIServiceResponse_Request_In_Process;
}

SCSIServiceResponse iSCSIVirtualHBA::AbortTaskSetRequest(SCSITargetIdentifier targetId,
														SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncAbortTaskSet;
    
	return kSCSIServiceResponse_TASK_COMPLETE;
	
}

SCSIServiceResponse iSCSIVirtualHBA::ClearACARequest(SCSITargetIdentifier targetId,
													SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearACA;
    
	return kSCSIServiceResponse_TASK_COMPLETE;
	
}

SCSIServiceResponse iSCSIVirtualHBA::ClearTaskSetRequest(SCSITargetIdentifier targetId,
														SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncClearTaskSet;
	return kSCSIServiceResponse_TASK_COMPLETE;
	
}

SCSIServiceResponse iSCSIVirtualHBA::LogicalUnitResetRequest(SCSITargetIdentifier targetId,
															SCSILogicalUnitNumber LUN)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.LUN = OSSwapHostToBigInt64(LUN);
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncLUNReset;
    
	return kSCSIServiceResponse_TASK_COMPLETE;
}

SCSIServiceResponse iSCSIVirtualHBA::TargetResetRequest(SCSITargetIdentifier targetId)
{
    // Create a SCSI target management PDU and send
    iSCSIPDUTargetMgmtReqBHS bhs = iSCSIPDUTargetMgmtReqBHSInit;
    bhs.function = kiSCSIPDUTaskMgmtFuncFlag | kiSCSIPDUTaskMgmtFuncTargetWarmReset;

    
	return kSCSIServiceResponse_TASK_COMPLETE;
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
	return kTaskDataSize;
}

UInt32 iSCSIVirtualHBA::ReportHBASpecificDeviceDataSize()
{
	return kDeviceDataSize;
}

bool iSCSIVirtualHBA::DoesHBAPerformDeviceManagement()
{
	// Lets the superclass know that we are going to create and destroy
	// our own targets and LUNs
	return true;
}

bool iSCSIVirtualHBA::InitializeController()
{
    // Setup session list
    sessionList = new iSCSISession * [kMaxSessions];
    
    if(!sessionList)
        return false;
    
    memset(sessionList,0,sizeof(iSCSISession *)*kMaxSessions);
    
    // Make ourselves discoverable to user clients (we do this last after
    // everything is initialized).
    registerService();
	
	// Successfully initialized controller
	return true;
}

void iSCSIVirtualHBA::TerminateController()
{
    // Go through every connection for each session, and close sockets,
    // remove event sources, etc
    for(UInt16 index = 0; index < kMaxSessions; index++)
    {
        if(sessionList[index] == nullptr)
            continue;
        
        ReleaseSession(index);
    }
    delete[] sessionList;
}

bool iSCSIVirtualHBA::StartController()
{
	// Create a dictionary that defines a target type
//	OSDictionary * targetProperties = OSDictionary::withCapacity(10);
//	targetProperties->setO
	
	//CreateTargetForID(1);
	


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
    else if(transferDirection == kSCSIDataTransfer_FromTargetToInitiator)
        bhs.flags |= kiSCSIPDUSCSICmdFlagRead;
    
    // For CDB sizes less than 16 bytes, plug directly into SCSI command PDU
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
    
    // Target id is just the session identifier
    iSCSISession * session = sessionList[(UInt16)targetId];
//    session->connections[
    SendPDU(session, NULL, (iSCSIPDUInitiatorBHS *)&bhs, NULL, NULL,0);
	
	return kSCSIServiceResponse_Request_In_Process;
}

void iSCSIVirtualHBA::CompleteTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                                   iSCSISession * session,
                                                   iSCSIConnection * connection)
{
    
}


//////////////////////////////// ISCSI FUNCTIONS ///////////////////////////////

/** Allocates a new iSCSI session and returns a session qualifier ID.
 *  @return a valid session qualifier (part of the ISID, see RF3720) or
 *  0 if a new session could not be created. */
UInt16 iSCSIVirtualHBA::CreateSession()
{
    for(UInt16 index = 0; index < kMaxSessions; index++)
    {
        if(sessionList[index] == nullptr)
        {
            // Alloc new session
            iSCSISession * newSession = new iSCSISession;
            
            // Code ISID 'a' field to indicate that we're not using OUI/IANA
            newSession->ISIDa = 0x02;
            
            // Generate a random number for the 'b' and 'c' fields of the ISID
            newSession->ISIDb = 0x001;
            newSession->ISIDc = 0x002;
            
            // Set ISID qualifier field
            newSession->ISIDd = index;
            
            // Setup connections array for new session
            newSession->connections =
                new iSCSIConnection * [kMaxConnectionsPerSession];
            
            // Retain new session
            sessionList[index] = newSession;
            
            return index;
        }
    }
    return kiSCSIInvalidSessionId;
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
        
    delete sessionList[sessionId];
    sessionList[sessionId] = nullptr;
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
UInt32 iSCSIVirtualHBA::CreateConnection(UInt16 sessionId,
                                         int domain,
                                         const struct sockaddr * targetAddress,
                                         const struct sockaddr * hostAddress)
{
    // Range-check inputs
    if(sessionId >= kMaxSessions || !targetAddress || !hostAddress)
        return kiSCSIInvalidConnectionId;
    
    iSCSISession * theSession = sessionList[sessionId];
    if(!theSession)
        return kiSCSIInvalidConnectionId;
    
    for(UInt32 index = 0; index < kMaxConnectionsPerSession; index++)
    {
        if(theSession->connections[index] != nullptr)
            continue;
        
            IOLog("Made it again");
        // Alloc new session
        iSCSIConnection * newConn = new iSCSIConnection;
 /*
        // Alloc new event source for session, quit if it fails
        iSCSIIOEventSource * eventSource = OSTypeAlloc(iSCSIIOEventSource);
        
        if(!eventSource ||  !eventSource->init(this,
                            (IOEventSource::Action)&CompleteTaskOnWorkloopThread,
                            theSession,newConn))
        {
            eventSource->release();
            delete newConn;
            return kiSCSIInvalidconnectionId;
        }
*/
        IOLog("Made it again and again");
        
        // Create a new socket (per RFC 3720, sockets used for iSCSI are
        // always stream sockets over TCP.  Only the domain can vary
        // between IPv4 or IPv6.
        errno_t sock_result = sock_socket(
                            domain,
                            SOCK_STREAM,
                            IPPROTO_TCP,
                            (sock_upcall)&iSCSIIOEventSource::socketCallback,
                            newConn->eventSource,
                            &newConn->socket);

        // Connect the socket to the target node
        if(sock_result == 0                             &&
      //     sock_bind(newConn->socket,hostAddress) == 0  &&
           sock_connect(newConn->socket,targetAddress,0) == 0)
        {
//            newConn->eventSource = eventSource;
            theSession->connections[index] = newConn;
            
            // Sockets connected and bound add event source to driver workloop
   //         GetWorkLoop()->addEventSource(newConn->eventSource);
            IOLog("Worked!");
            return index;
        }
        else
        {
            IOLog("Made it again and again and again");
//            eventSource->release();
//            sock_close(newConn->socket);
            delete newConn;
            return kiSCSIInvalidConnectionId;
        }
    }
    return kiSCSIInvalidConnectionId;
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

//    GetWorkLoop()->removeEventSource(theConn->eventSource);
//    theConn->eventSource->release();
    sock_close(theConn->socket);
    delete theConn;
    theSession->connections[connectionId] = nullptr;
}

/** Sends data over a kernel socket associated with iSCSI.
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
                                 size_t dataLength)
{
    // Range-check inputs
    if(!session || !connection || !bhs)
        return EINVAL;
    
    // Set the command sequence number & expected status sequence number
    bhs->cmdSN = OSSwapHostToBigInt32(session->cmdSN);
    bhs->expStatSN = OSSwapHostToBigInt32(connection->expStatSN);
    
    // Set data segment length field
    UInt32 length = (OSSwapHostToBigInt32((UInt32)dataLength)>>8);
    memcpy(bhs->dataSegmentLength,&length,kiSCSIPDUDataSegmentLengthSize);

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
        iovec[iovecCnt].iov_base = data;
        iovec[iovecCnt].iov_len  = dataLength;
        iovecCnt++;
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

/** Receives a data segment over a kernel socket.
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

    
    // Receive data over the network
    struct msghdr msg;
    struct iovec  iovec;
//    memset(&msg,0,sizeof(struct msghdr));
    
    msg.msg_iov     = &iovec;
    msg.msg_iovlen  = 1;
    iovec.iov_base  = data;
    iovec.iov_len   = length;
    
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
