/*!
 * @author		Nareg Sinenian
 * @file		iSCSITypesShared.h
 * @date		January 6, 2015
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI data types shared between user space and kernel space.
 */

#ifndef __ISCSI_TYPES_SHARED_H__
#define __ISCSI_TYPES_SHARED_H__

#if MAC_OS_X_VERSION_MIN_REQUIRED <= MAC_OS_X_VERSION_10_8
typedef int errno_t;
#endif

/*! Session identifier. */
typedef UInt16 SID;

/*! Connection identifier. */
typedef UInt32 CID;

/*! Target portal group tag. */
typedef UInt16 TPGT;

/*! Target session identifier. */
typedef UInt16 TSIH;

/*! Session qualifier value for an invalid session. */
static const UInt16 kiSCSIInvalidSessionId = 0xFFFF;

/*! Connection ID for an invalid connection. */
static const UInt32 kiSCSIInvalidConnectionId = 0xFFFFFFFF;

/*! Max number of sessions. */
static const UInt16 kiSCSIMaxSessions = 16;

/*! Max number of connections per session. */
static const UInt32 kiSCSIMaxConnectionsPerSession = 2;

/*! An enumeration of configurable session parameters. */
enum iSCSIKernelSessionOptTypes {
    
    /*! Time to retain (UInt16). */
    kiSCSIKernelSODefaultTime2Retain,
    
    /*! Time to wait (UInt16). */
    kiSCSIKernelSODefaultTime2Wait,
    
    /*! Error recovery level (UInt8). */
    kiSCSIKernelSOErrorRecoveryLevel,
    
    /*! Max connections supported by target (UInt32). */
    kiSCSIKernelSOMaxConnections,
    
    /*! Send data immediately (bool). */
    kiSCSIKernelSOImmediateData,
    
    /*!  Expect an initial R2T from target (bool). */
    kiSCSIKernelSOInitialR2T,
    
    /*! Data PDUs in order (bool). */
    kiSCSIKernelSODataPDUInOrder,
    
    /*! Data sequence in order (bool). */
    kiSCSIKernelSODataSequenceInOrder,
    
    /*! Number of outstanding R2Ts allowed (UInt16). */
    kiSCSIKernelSOMaxOutstandingR2T,
    
    /*! Maximum data burst length in bytes (UInt32). */
    kiSCSIKernelSOMaxBurstLength,
    
    /*! First data burst length in bytes (UInt32). */
    kiSCSIKernelSOFirstBurstLength,
    
    /*! Target session identifying handle (TSIH). */
    kiSCSIKernelSOTargetSessionId,
    
    /*! Target portal group tag (TPGT). */
    kiSCSIKernelSOTargetPortalGroupTag,
    
};


/*! An enumeration of configurable connection parameters. */
enum iSCSIKernelConnectionOptTypes {
    
    /*! Flag that indicates if this connection uses header digests (bool). */
    kiSCSIKernelCOUseHeaderDigest,
    
    /*! Flag that indicates if this connection uses data digests (bool). */
    kiSCSIKernelCOUseDataDigest,
    
    /*! Flag that indicates if this connection uses IF markers (bool). */
    kiSCSIKernelCOUseIFMarker,
    
    /*! Flag that indicates if this connection uses OF markers (bool). */
    kiSCSIKernelCOUseOFMarker,
    
    /*! Interval for OF marker (UInt16). */
    kiSCSIKernelCOOFMarkInt,
    
    /*! Interval for IF marker (UInt16). */
    kiSCSIKernelCOIFMarkInt,
    
    /*! Maximum data segment length allowed by the target (UInt32). */
    kiSCSIKernelCOMaxSendDataSegmentLength,
    
    /*! Maximum data segment length initiator can receive (UInt32). */
    kiSCSIKernelCOMaxRecvDataSegmentLength,
    
    /*! Initial expStatSN. */
    kiSCSIKernelCOInitialExpStatSN
    
};


#endif
