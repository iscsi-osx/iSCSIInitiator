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

/*! Session identifier. */
typedef UInt16 SID;

/*! Connection identifier. */
typedef UInt32 CID;

/*! Target portal group tag. */
typedef UInt16 TPGT;

/*! Target session identifier. */
typedef UInt32 TSIH;

/*! Session qualifier value for an invalid session. */
static const UInt16 kiSCSIInvalidSessionId = 0xFFFF;

/*! Connection ID for an invalid connection. */
static const UInt32 kiSCSIInvalidConnectionId = 0xFFFFFFFF;

/*! Max number of sessions. */
static const UInt16 kiSCSIMaxSessions = 16;

/*! Max number of connections per session. */
static const UInt32 kiSCSIMaxConnectionsPerSession = 2;

/*! Struct used to set session-wide options in the kernel. */
typedef struct iSCSIKernelSessionCfg
{
    /*! Time to retain. */
    UInt16 defaultTime2Retain;
    
    /*! Time to wait. */
    UInt16 defaultTime2Wait;
    
    /*! Error recovery level. */
    UInt8 errorRecoveryLevel;
    
    /*! Max connections supported by target. */
    UInt32 maxConnections;
    
    /*! Send data immediately. */
    bool immediateData;
    
    /*!  Expect an initial R2T from target. */
    bool initialR2T;
    
    /*! Data PDUs in order. */
    bool dataPDUInOrder;
    
    /*! Data sequence in order. */
    bool dataSequenceInOrder;
    
    /*! Number of outstanding R2Ts allowed. */
    UInt16 maxOutStandingR2T;
    
    /*! Maximum data burst length (in bytes). */
    UInt32 maxBurstLength;
    
    /*! First data burst length (in bytes). */
    UInt32 firstBurstLength;
    
    /*! Target session identifying handle. */
    TSIH targetSessionId;
    
    /*! Target portal group tag. */
    TPGT targetPortalGroupTag;
    
} iSCSIKernelSessionCfg;

/*! Struct used to set connection-wide options in the kernel. */
typedef struct iSCSIKernelConnectionCfg
{
    /*! Flag that indicates if this connection uses header digests. */
    bool useHeaderDigest;
    
    /*! Flag that indicates if this connection uses data digests. */
    bool useDataDigest;
    
    /*! Flag that indicates if this connection uses IF markers. */
    bool useIFMarker;
    
    /*! Flag that indicates if this connection uses OF markers. */
    bool useOFMarker;
    
    /*! Interval for OF marker. */
    UInt16 OFMarkInt;
    
    /*! Interval for IF marker. */
    UInt16 IFMarkInt;
    
    /*! Maximum data segment length allowed by the target. */
    UInt32 maxSendDataSegmentLength;
    
    /*! Maximum data segment length initiator can receive. */
    UInt32 maxRecvDataSegmentLength;
    
} iSCSIKernelConnectionCfg;




#endif
