/*!
 * @author		Nareg Sinenian
 * @file		iSCSIInterfaceShared.h
 * @date		March 18, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI kernel extension definitions that are shared between
 *              kernel and user space.  These definitions are used by the
 *              kernel extension's client in user to access the iSCSI virtual
 *              host bus adapter (initiator).
 */

#ifndef __ISCSI_SHARED_H__
#define __ISCSI_SHARED_H__

// If used in user-space, this header will need to include additional
// headers that define primitive fixed-size types.  If used with the kernel,
// IOLib must be included for kernel memory allocation
#ifdef KERNEL
#include <IOKit/IOLib.h>
#else
#include <stdlib.h>
#include <MacTypes.h>
#endif

/*! Function pointer indices.  These are the functions that can be called
 *	indirectly by calling IOCallScalarMethod(). */
enum functionNames {
	kiSCSIOpenInitiator,
	kiSCSICloseInitiator,
    kiSCSICreateSession,
    kiSCSIReleaseSession,
    kiSCSISetSessionOptions,
    kiSCSIGetSessionOptions,
    kiSCSICreateConnection,
    kiSCSIReleaseConnection,
    kiSCSIActivateConnection,
    kiSCSIActivateAllConnections,
    kiSCSIDeactivateConnection,
    kiSCSIDeactivateAllConnections,
    kiSCSISendBHS,
    kiSCSISendData,
    kiSCSIRecvBHS,
    kiSCSIRecvData,
    kiSCSISetConnectionOptions,
    kiSCSIGetConnectionOptions,
    kiSCSIGetConnection,
    kiSCSIGetNumConnections,
	kiSCSIInitiatorNumMethods
};

/*! Session qualifier value for an invalid session. */
static const UInt16 kiSCSIInvalidSessionId = 0xFFFF;

/*! Connection ID for an invalid connection. */
static const UInt32 kiSCSIInvalidConnectionId = 0xFFFFFFFF;

/*! Struct used to set session-wide options in the kernel. */
typedef struct iSCSISessionOptions
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
    UInt32 TSIH;
    
    /*! Target portal group tag. */
    UInt32 TPGT;

    
} iSCSISessionOptions;

/*! Struct used to set connection-wide options in the kernel. */
typedef struct iSCSIConnectionOptions
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
    UInt16 OFMarkerInt;
    
    /*! Interval for IF marker. */
    UInt16 IFMarkerInt;
    
    /*! Maximum data segment length allowed by the target. */
    UInt32 maxSendDataSegmentLength;
    
    /*! Maximum data segment length initiator can receive. */
    UInt32 maxRecvDataSegmentLength;
    
} iSCSIConnectionOptions;

#endif /* defined(__ISCSI_SHARED_H__) */
