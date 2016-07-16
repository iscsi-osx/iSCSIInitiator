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
typedef UInt16 SessionIdentifier;

/*! Connection identifier. */
typedef UInt32 ConnectionIdentifier;

/*! Target portal group tag. */
typedef UInt16 TargetPortalGroupTag;

/*! Target session identifier. */
typedef UInt16 TargetSessionIdentifier;

/*! Session qualifier value for an invalid session. */
static const UInt16 kiSCSIInvalidSessionId = 0xFFFF;

/*! Connection ID for an invalid connection. */
static const UInt32 kiSCSIInvalidConnectionId = 0xFFFFFFFF;

/*! Max number of sessions. */
static const UInt16 kiSCSIMaxSessions = 16;

/*! Max number of connections per session. */
static const UInt32 kiSCSIMaxConnectionsPerSession = 2;

/*! An enumeration of configurable session parameters. */
enum iSCSIHBASessionParameters {
    
    /*! Time to retain (UInt16). */
    kiSCSIHBASODefaultTime2Retain,
    
    /*! Time to wait (UInt16). */
    kiSCSIHBASODefaultTime2Wait,
    
    /*! Error recovery level (UInt8). */
    kiSCSIHBASOErrorRecoveryLevel,
    
    /*! Max connections supported by target (UInt32). */
    kiSCSIHBASOMaxConnections,
    
    /*! Send data immediately (bool). */
    kiSCSIHBASOImmediateData,
    
    /*!  Expect an initial R2T from target (bool). */
    kiSCSIHBASOInitialR2T,
    
    /*! Data PDUs in order (bool). */
    kiSCSIHBASODataPDUInOrder,
    
    /*! Data sequence in order (bool). */
    kiSCSIHBASODataSequenceInOrder,
    
    /*! Number of outstanding R2Ts allowed (UInt16). */
    kiSCSIHBASOMaxOutstandingR2T,
    
    /*! Maximum data burst length in bytes (UInt32). */
    kiSCSIHBASOMaxBurstLength,
    
    /*! First data burst length in bytes (UInt32). */
    kiSCSIHBASOFirstBurstLength,
    
    /*! Target session identifying handle (TSIH). */
    kiSCSIHBASOTargetSessionId,
    
    /*! Target portal group tag (TPGT). */
    kiSCSIHBASOTargetPortalGroupTag,
    
};


/*! An enumeration of configurable connection parameters. */
enum iSCSIHBAConnectionParameters {
    
    /*! Flag that indicates if this connection uses header digests (bool). */
    kiSCSIHBACOUseHeaderDigest,
    
    /*! Flag that indicates if this connection uses data digests (bool). */
    kiSCSIHBACOUseDataDigest,
    
    /*! Flag that indicates if this connection uses IF markers (bool). */
    kiSCSIHBACOUseIFMarker,
    
    /*! Flag that indicates if this connection uses OF markers (bool). */
    kiSCSIHBACOUseOFMarker,
    
    /*! Interval for OF marker (UInt16). */
    kiSCSIHBACOOFMarkInt,
    
    /*! Interval for IF marker (UInt16). */
    kiSCSIHBACOIFMarkInt,
    
    /*! Maximum data segment length allowed by the target (UInt32). */
    kiSCSIHBACOMaxSendDataSegmentLength,
    
    /*! Maximum data segment length initiator can receive (UInt32). */
    kiSCSIHBACOMaxRecvDataSegmentLength,
    
    /*! Initial expStatSN. */
    kiSCSIHBACOInitialExpStatSN
    
};


#endif
