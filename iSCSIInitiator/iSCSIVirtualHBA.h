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

/** Forward declaration of session object used by HBA. */
struct iSCSISession;
struct iSCSIConnection;

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
     *  @param address the BSD socket structure used to identify the target.
     *  @return a connection ID, or 0 if a connection could not be created. */
    UInt32 CreateConnection(UInt16 sessionId,
                            int domain,
                            const struct sockaddr * targetAddress,
                            const struct sockaddr * hostAddress);
    
    /** Frees a given iSCSI connection associated with a given session.
     *  The session should be logged out using the appropriate PDUs. */
    void ReleaseConnection(UInt16 sessionId,UInt32 connectionId);
    
    /** Sends data over a kernel socket associated with iSCSI.
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
                    size_t dataLength);
    
    /** Receives a basic header segment over a kernel socket.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param bhs the basic header segment received.
     *  @return error code indicating result of operation. */
    errno_t RecvPDUHeader(iSCSISession * session,
                          iSCSIConnection * connection,
                          iSCSIPDUTargetBHS * bhs);

    /** Receives a data segment over a kernel socket.
     *  @param sessionId the qualifier part of the ISID (see RFC3720).
     *  @param connectionId the connection associated with the session.
     *  @param data the data received.
     *  @param length the length of the data buffer.
     *  @return error code indicating result of operation. */
    errno_t RecvPDUData(iSCSISession * session,
                        iSCSIConnection * connection,
                        void * data,
                        size_t length);
    
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
    
	//////////  FUNCTIONS REQUIRED BY IOSCSIPARALLELINTERFACECONTROLLER  ///////

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

    /** Gets the SCSI initiator ID. */
    virtual SCSIInitiatorIdentifier ReportInitiatorIdentifier();

	/** Gets the highest SCSI ID that the HBA can address.
	 *	@return highest addressable SCSI device. */
	virtual SCSIDeviceIdentifier ReportHighestSupportedDeviceID();

	virtual UInt32 ReportMaximumTaskCount();

	virtual UInt32 ReportHBASpecificTaskDataSize();

	virtual UInt32 ReportHBASpecificDeviceDataSize();

	/** Gets whether the virtual HBA creates and removes targets on its own.
	 *  @return true whether HBA creates and remove targtes on its own. */
	virtual bool DoesHBAPerformDeviceManagement();

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

	/** */
	virtual SCSIServiceResponse ProcessParallelTask(SCSIParallelTaskIdentifier parallelTask);
    

    static void CompleteTaskOnWorkloopThread(iSCSIVirtualHBA * owner,
                                             iSCSISession * session,
                                             iSCSIConnection * connection);
                                              
private:
	
    /** Maximum allowable sessions. */
    static const UInt16 kMaxSessions;
    
    /** Maximum allowable connections per session. */
    static const UInt16 kMaxConnectionsPerSession;
    
    /** Highest LUN supported by the virtual HBA. */
    static const SCSILogicalUnitNumber kHighestLun;
    
    /** Initiator ID of the virtual HBA. */
    static const SCSIInitiatorIdentifier kInitiatorId;
    
    /** Highest SCSI device ID supported by the HBA. */
    static const SCSIDeviceIdentifier kHighestSupportedDeviceId;
    
    /** Maximum number of SCSI tasks the HBA can handle. */
    static const UInt32 kMaxTaskCount;
    
    /** Amount of memory (in bytes) required for a single SCSI task. */
    static const UInt32 kTaskDataSize;
    
    /** Amount of memory (in bytes) required for each target device. */
    static const UInt32 kDeviceDataSize;
	
	/** Lookup table that maps SCSI sessions to ISID qualifiers 
     *  (session qualifier IDs). */
    iSCSISession * * sessionList;
    
    IOLock * sessionListLock;
};


#endif /* defined(__iSCSI_Initiator__iSCSIInitiatorVirtualHBA__) */



