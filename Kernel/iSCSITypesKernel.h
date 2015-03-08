/*!
 * @author		Nareg Sinenian
 * @file		iSCSITypesKernel
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 *
 *              iSCSI data types that are used exclusively by the kernel.
 */

#ifndef __ISCSI_TYPES_KERNEL_H__
#define __ISCSI_TYPES_KERNEL_H__


#include <IOKit/IOLib.h>
#include <sys/socket.h>

#include "iSCSITypesShared.h"

class iSCSITaskQueue;
class iSCSIIOEventSource;

/*! Definition of a single connection that is associated with a particular
 *  iSCSI session. */
typedef struct iSCSIConnection {
    
    /*! Status sequence number expected by the initiator. */
    UInt32 expStatSN;
    
    /*! Connection ID. */
    CID CID; // Might need this for ErrorRecovery (otherwise have to search through list for it)
    
    /*! Target tag for current transfer. */
    //    UInt32 targetTransferTag;  /// NEED THIS???
    
    /*! Socket used for communication. */
    socket_t socket;
    
    /*! Used to keep track of R2T PDUs. */
    UInt32 R2TSN;
    
    /*! Mutex lock used to prevent simultaneous Send/Recv from different
     *  threads (e.g., workloop thread and other threads). */
    IOLock * PDUIOLock;
    
    /*! iSCSI task queue used to manage tasks for this connection. */
    iSCSITaskQueue * taskQueue;

    /*! Event source used to signal the Virtual HBA that data has been
     *  received and needs to be processed. */
    iSCSIIOEventSource * dataRecvEventSource;
    
    /*! Options associated with this connection. */
    iSCSIKernelConnectionCfg opts;
    
    /*! Amount of data, in bytes, that this connection has been requested
     *  to transfer.  This is used for bitrate-based load balancing. */
    UInt32 dataToTransfer;
    
    /*! The maximum length of data allowed for immediate data (data sent as part
     *  of a command PDU).  This parameter is derived by taking the lesser of
     *  the FirstBurstLength and the maxSendDataSegmentLength.  The former
     *  is a session option while the latter is a connection option. */
    UInt32 immediateDataLength;
    
    /*! Keeps track of when processing began for a particualr task, as
     *  represented by the system uptime (seconds component). */
    clock_sec_t taskStartTimeSec;
    
    /*! Keeps track of when processing began for a particualr task, as
     *  represented by the system uptime (microseconds component). */
    clock_usec_t taskStartTimeUSec;
    
    /*! Keeps track of the iSCSI data transfer rate of this connection,
     *  in units of bytes per second.  This number is obtained by averaging
     *  over 5 tasks. */
    UInt32 bytesPerSecond;
    
    /*! Size of moving average (# points) to use to compute average speed. */
    static const UInt8 kBytesPerSecAvgWindowSize = 30;
    
    /*! Last few measurements of bytes per second on this connection. */
    UInt32 bytesPerSecondHistory[kBytesPerSecAvgWindowSize];
    
    /*! Keeps track of the index in the above array should be populated next. */
    UInt8 bytesPerSecHistoryIdx;
} iSCSIConnection;


/*! Definition of a single iSCSI session.  Each session is comprised of one
 *  or more connections as defined by the struct iSCSIConnection.  Each session
 *  is further associated with an initiator session ID (ISID), a target session
 *  ID (TSIH), a target IP address, a target name, and a target alias. */
typedef struct iSCSISession {
    
    /*! The initiator session ID, which is also used as the target ID within
     *  this kernel extension since there is a 1-1 mapping. */
    SID sessionId;
    
    /*! Command sequence number to be used for the next initiator command. */
    UInt32 cmdSN;
    
    /*! Command seqeuence number expected by the target. */
    UInt32 expCmdSN;
    
    /*! Maximum command seqeuence number allowed. */
    UInt32 maxCmdSN;
    
    /*! Connections associated with this session. */
    iSCSIConnection * * connections;
    
    /*! Options associated with this session. */
    iSCSIKernelSessionCfg opts;
    
    /*! Number of active connections. */
    UInt32 numActiveConnections;
    
    //////// CONSIDER REMOVING THIS BELOW.....
    /*! Total number of connections (either active or inactive). */
    //    UInt32 numConnections; NEED THIS?? COMPUTED BY ITERATING IN ONE INSTANCE....
    
    /*! Initiator tag for the newest task. */
    //   UInt32 initiatorTaskTag;  NEED THIS????
    
    /*! Indicates whether session is active, which means that a SCSI target
     *  exists and is backing the the iSCSI session. */
    bool active;
    
} iSCSISession;

#endif /* defined(__ISCSI_TYPES_KERNEL_H__) */
