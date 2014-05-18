/**
 * @author		Nareg Sinenian
 * @file		iSCSIVirtualHBA.h
 * @date		October 13, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_VIRTUAL_HBA_H__
#define __ISCSI_VIRTUAL_HBA_H__

#include <IOKit/IOService.h>
#include <IOKit/scsi/spi/IOSCSIParallelInterfaceController.h>

#include <libkern/c++/OSArray.h>

#include "iSCSIInterfaceShared.h"
#include "iSCSIPDUKernel.h"

// BSD Socket Includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>

#define iSCSIVirtualHBA		com_NSinenian_iSCSIVirtualHBA

/** This class implements the iSCSI virtual host bus adapter (HBA).  The HBA
 *	creates and removes targets and processes SCSI requested by the operating
 *	system. The class maintains state information, including targets and
 *  associated LUNs.  SCSI CDBs that originated from the OS are packaged into
 *  PDUs and then sent over a TCP socket to the specified iSCSI target.
 *	Responses from the target are processes and converted from PDUs to CDBs
 *  and then returned to the OS. */
class iSCSIVirtualHBA : public IOSCSIParallelInterfaceController
{
	OSDeclareDefaultStructors(iSCSIVirtualHBA);

public:
    
    /** Forward delcaration of an iSCSI session. */
    struct iSCSISession;
    
    /** Forward delcaration of an iSCSI connection. */
    struct iSCSIConnection;
	
	/////////  FUNCTIONS REQUIRED BY IOSCSIPARALLELINTERFACECONTROLLER  ////////

	/** Gets the highest logical unit number that the HBA can address.
	 *	@return highest addressable LUN. */
	virtual SCSILogicalUnitNumber ReportHBAHighestLogicalUnitNumber();
	
	/** Gets whether HBA supports a particular SCSI feature.
	 *	@param theFeature the SCSI feature to check.
	 *	@return true if the specified feature is supported. */
	virtual bool DoesHBASupportSCSIParallelFeature(SCSIParallelFeature theFeature);
	
	/** Initializes a new SCSI target. After a call to CreateTargetForID(),
	 *	an instance of IOSCSIParallelInterfaceDevice is created.  Upon creation,
	 *	this callback function is invoked.
	 *	@param targetId the target to initialize.
	 *	@return true if the target was successfully initialized. */
	virtual bool InitializeTargetForID(SCSITargetIdentifier targetId);

	virtual SCSIServiceResponse AbortTaskRequest(SCSITargetIdentifier targetId,
                                                 SCSILogicalUnitNumber LUN,
                                                 SCSITaggedTaskIdentifier taggedTaskID);

    virtual SCSIServiceResponse AbortTaskSetRequest(SCSITargetIdentifier targetId,
                                                    SCSILogicalUnitNumber LUN);

    virtual SCSIServiceResponse ClearACARequest(SCSITargetIdentifier targetId,
                                                SCSILogicalUnitNumber LUN);

    virtual SCSIServiceResponse ClearTaskSetRequest(SCSITargetIdentifier targetId,
                                                    SCSILogicalUnitNumber LUN);

    virtual SCSIServiceResponse LogicalUnitResetRequest(SCSITargetIdentifier targetId,
                                                        SCSILogicalUnitNumber LUN);

    virtual SCSIServiceResponse TargetResetRequest(SCSITargetIdentifier targetId);


    /** Gets the SCSI initiator ID.  This is a random number that is 
     *  generated each time this controller is initialized. */
    virtual SCSIInitiatorIdentifier ReportInitiatorIdentifier();

	/** Gets the highest SCSI ID that the HBA can address.
	 *	@return highest addressable SCSI device. */
	virtual SCSIDeviceIdentifier ReportHighestSupportedDeviceID();

    /** Returns the maximum number of tasks that this virtual HBA can
     *  process at any one time. */
	virtual UInt32 ReportMaximumTaskCount();

    /** Returns the data size associated with a particular task (0). */
	virtual UInt32 ReportHBASpecificTaskDataSize();

    /** Returns the device data size (0). */
	virtual UInt32 ReportHBASpecificDeviceDataSize();

	/** Gets whether the virtual HBA creates and removes targets on its own.
	 *  @return true if HBA creates and remove targtes on its own. */
	virtual bool DoesHBAPerformDeviceManagement();
    
    /** Gets whether the virtual HBA retrieve sense data for each I/O.
     *  @return true if HBA does its own sensing for each I/O operation. */
//    virtual bool DoesHBAPerformAutoSense();

	/** Initializes the virtual HBA.
	 *	@return true if controller was successfully initialized. */
	virtual bool InitializeController();

    /** Frees resources associated with the virtual HBA. */
	virtual void TerminateController();

	/** Starts controller.
	 *	@return true if the controller was started. */
	virtual bool StartController();

	/** Stops controller. */
	virtual void StopController();

	/** Handles hardware interrupts (not used for this virtual HBA). */
	virtual void HandleInterruptRequest();

	/** Processes a task passed down by SCSI target devices in driver stack.
     *  @param parallelTask the task to process.
     *  @return a response that indicates the processing status of the task. */
	virtual SCSIServiceResponse ProcessParallelTask(SCSIParallelTaskIdentifier parallelTask);
    
    /** Called by our software interrupt source (iSCSIIOEventSource) to let us
     *  know that data has become available for a particular session and
     *  connection.
     *  @param owner an instance of this class.
     *  @param session session associated with connection that received data.
     *  @param connection the connection that received data. */
    static void CompleteTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                             iSCSISession * session,
                                             iSCSIConnection * connection);
    
    /////////////////////  FUNCTIONS TO MANIPULATE ISCSI ///////////////////////
    
    /** Allocates a new iSCSI session and returns a session qualifier ID.
     *  @return a valid session qualifier (part of the ISID, see RF3720) or
     *  0 if a new session could not be created. */
    UInt16 CreateSession();
    
    /** Releases an iSCSI session, including all connections associated with that
     *  session.
     *  @param sessionId the session qualifier part of the ISID. */
    void ReleaseSession(UInt16 sessionId);
        
    /** Sets options associated with a particular session.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param options the options to set.
     *  @return error code indicating result of operation. */
    errno_t SetSessionOptions(UInt16 sessionId,
                              iSCSISessionOptions * options);
    
    /** Gets options associated with a particular session.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param options the options to get.  The user of this function is
     *  responsible for allocating and freeing the options struct.
     *  @return error code indicating result of operation. */
    errno_t GetSessionOptions(UInt16 sessionId,
                              iSCSISessionOptions * options);
    
    /** Allocates a new iSCSI connection associated with the particular session.
     *  @param sessionId the session to create a new connection for.
     *  @param domain the IP domain (e.g., AF_INET or AF_INET6).
     *  @param targetaddress the BSD socket structure used to identify the target.
     *  @param hostaddress the BSD socket structure used to identify the host.
     *  @param connectionId identifier for the new connection.
     *  @return error code indicating result of operation. */
    errno_t CreateConnection(UInt16 sessionId,
                             int domain,
                             const struct sockaddr * targetAddress,
                             const struct sockaddr * hostAddress,
                             UInt32 * connectionId);
    
    /** Frees a given iSCSI connection associated with a given session.
     *  The session should be logged out using the appropriate PDUs. */
    void ReleaseConnection(UInt16 sessionId,UInt32 connectionId);
    
    /** Activates an iSCSI connection, indicating to the kernel that the iSCSI
     *  daemon has negotiated security and operational parameters and that the
     *  connection is in the full-feature phase.
     *  @param sessionId the session to deactivate.
     *  @param connectionId the connection to deactivate.
     *  @return error code indicating result of operation. */
    errno_t ActivateConnection(UInt16 sessionId,UInt32 connectionId);
    
    /** Deactivates an iSCSI connection so that parameters can be adjusted or
     *  negotiated by the iSCSI daemon.
     *  @param sessionId the session to deactivate.
     *  @param connectionId the connection to deactivate.
     *  @return error code indicating result of operation. */
    errno_t DeactivateConnection(UInt16 sessionId,UInt32 connectionId);
    
    /** Sends data over a kernel socket associated with iSCSI.  If the specified
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
    errno_t SendPDU(iSCSISession * session,
                    iSCSIConnection * connection,
                    iSCSIPDUInitiatorBHS * bhs,
                    iSCSIPDU::iSCSIPDUCommonAHS * ahs,
                    void * data,
                    size_t length);

    /** Gets whether a PDU is available for receiption on a particular
     *  connection.
     *  @param the connection to check.
     *  @return true if a PDU is available, false otherwise. */
    static bool isPDUAvailable(iSCSIConnection * connection);
    
    /** Receives a basic header segment over a kernel socket.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param bhs the basic header segment received.
     *  @param flags optional flags to be passed onto sock_recv.
     *  @return error code indicating result of operation. */
    errno_t RecvPDUHeader(iSCSISession * session,
                          iSCSIConnection * connection,
                          iSCSIPDUTargetBHS * bhs,
                          int flags);
    
    /** Receives a data segment over a kernel socket.  If the specified length is
     *  not a multiple of 4-bytes, the padding bytes will be discarded per
     *  RF3720 specification (all data segment are multiples of 4 bytes).
     *  multiple of 4-bytes per RFC3720.  For length
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param data the data received.
     *  @param length the length of the data buffer.
     *  @param flags optional flags to be passed onto sock_recv.
     *  @return error code indicating result of operation. */
    errno_t RecvPDUData(iSCSISession * session,
                        iSCSIConnection * connection,
                        void * data,
                        size_t length,
                        int flags);
    
    /** Wrapper around SendPDU for user-space calls.
     *  Sends data over a kernel socket associated with iSCSI.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param bhs the basic header segment to send.
     *  @param data the data segment to send.
     *  @param length the byte size of the data segment
     *  @return error code indicating result of operation. */
    errno_t SendPDUUser(UInt16 sessionId,
                        UInt32 connectionId,
                        iSCSIPDUInitiatorBHS * bhs,
                        void * data,
                        size_t dataLength);
    
    /** Wrapper around RecvPDUHeader for user-space calls.
     *  Receives a basic header segment over a kernel socket.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param bhs the basic header segment received.
     *  @return error code indicating result of operation. */
    errno_t RecvPDUHeaderUser(UInt16 sessionId,
                              UInt32 connectionId,
                              iSCSIPDUTargetBHS * bhs);
    
    /** Wrapper around RecvPDUData for user-space calls.
     *  Receives a data segment over a kernel socket.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param data the data received.
     *  @param length the length of the data buffer.
     *  @return error code indicating result of operation. */
    errno_t RecvPDUDataUser(UInt16 sessionId,
                            UInt32 connectionId,
                            void * data,
                            size_t length);
    
    /** Sets options associated with a particular connection.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param options the options to set.
     *  @return error code indicating result of operation. */
    errno_t SetConnectionOptions(UInt16 sessionId,
                                 UInt32 connectionId,
                                 iSCSIConnectionOptions * options);
    
    /** Gets options associated with a particular connection.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param options the options to get.  The user of this function is
     *  responsible for allocating and freeing the options struct.
     *  @return error code indicating result of operation. */
    errno_t GetConnectionOptions(UInt16 sessionId,
                                 UInt32 connectionId,
                                 iSCSIConnectionOptions * options);
                                              
private:
    
    /** Process an incoming task management response PDU.
     *  @param session the session associated with the task mgmt response.
     *  @param connection the connection associated with the task mgmt response.
     *  @param bhs the basic header segment of the task mgmt response. */
    void ProcessTaskMgmtResponse(iSCSISession * session,
                                 iSCSIConnection * connection,
                                 UInt8 taskMgmtResponse,
                                 UInt32 initiatorTaskTag);
    
    /** Process an incoming SCSI response PDU.
     *  @param session the session associated with the SCSI response.
     *  @param connection the connection associated with the SCSI response.
     *  @param bhs the basic header segment of the SCSI response. */
    void ProcessSCSIResponse(iSCSISession * session,
                             iSCSIConnection * connection,
                             iSCSIPDU::iSCSIPDUSCSIRspBHS * bhs);

    /** Process an incoming data PDU.
     *  @param session the session associated with the data PDU.
     *  @param connection the connection associated with the data PDU.
     *  @param bhs the basic header segment of the data PDU. */
    void ProcessDataIn(iSCSISession * session,
                       iSCSIConnection * connection,
                       iSCSIPDU::iSCSIPDUDataInBHS * bhs);

    /** Process an incoming R2T PDU.
     *  @param session the session associated with the R2T PDU.
     *  @param connection the connection associated with the R2T PDU.
     *  @param bhs the basic header segment of the R2T PDU. */
    void ProcessR2T(iSCSISession * session,
                    iSCSIConnection * connection,
                    iSCSIPDU::iSCSIPDUR2TBHS * bhs);
	
    /** Maximum allowable sessions. */
    static const UInt16 kMaxSessions;
    
    /** Maximum allowable connections per session. */
    static const UInt16 kMaxConnectionsPerSession;
    
    /** Highest LUN supported by the virtual HBA. */
    static const SCSILogicalUnitNumber kHighestLun;
    
    /** Highest SCSI device ID supported by the HBA. */
    static const SCSIDeviceIdentifier kHighestSupportedDeviceId;
    
    /** Maximum number of SCSI tasks the HBA can handle. */
    static const UInt32 kMaxTaskCount;
    


    

    
    /** Amount of memory (in bytes) required for a single SCSI task. */
//    static const UInt32 kTaskDataSize;
    
    /** Amount of memory (in bytes) required for each target device. */
 //   static const UInt32 kDeviceDataSize;
    
    /** Initiator ID of the virtual HBA.  This value is auto-generated upon 
     *  initialization of the initiator and the 24 least significant bits
     *  are used to form part of the iSCSI ISID. */
    SCSIInitiatorIdentifier kInitiatorId;
	
	/** Lookup table that maps SCSI sessions to ISID qualifiers 
     *  (session qualifier IDs). */
    iSCSISession * * sessionList;
};


#endif /* defined(__iSCSI_Initiator__iSCSIInitiatorVirtualHBA__) */



