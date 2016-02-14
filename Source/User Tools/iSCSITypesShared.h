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
