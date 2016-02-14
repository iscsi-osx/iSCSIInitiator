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

#ifndef __ISCSI_RFC3720_DEFAULTS_H__
#define __ISCSI_RFC3720_DEFAULTS_H__

/*! Default max connections value per RFC3720. */
static const unsigned int kRFC3720_MaxConnections = 1;

/*! Maximum max connections value per RFC3720. */
static const unsigned int kRFC3720_MaxConnections_Min = 1;

/*! Minimum max connections value per RFC3720. */
static const unsigned int kRFC3720_MaxConnections_Max = 65535;

/*! Default initialR2T connections value per RFC3720. */
static const bool kRFC3720_InitialR2T = true;

/*! Default immediate data value per RFC3720. */
static const bool kRFC3720_ImmediateData = true;

/*! Default header digest value per RFC3720. */
static const bool kRFC3720_HeaderDigest = false;

/*! Default data digest value per RFC3720. */
static const bool kRFC3720_DataDigest = false;

/*! Default IF marker value per RFC3720. */
static const bool kRFC3720_IFMarker = false;

/*! Default OF marker value per RFC3720. */
static const bool kRFC3720_OFMarker = false;

/*! Default IF marker interval value (in bytes) per RFC3720. */
static const unsigned int kRFC3720_IFMarkInt = 8192;

/*! Default OF marker interval value (in bytes) per RFC3720. */
static const unsigned int kRFC3720_OFMarkInt = 8192;

/*! Default maximum received data segment length value per RFC3720. */
static const unsigned int kRFC3720_MaxRecvDataSegmentLength = 8192;

/*! Minimum allowed received data segment length value per RFC3720. */
static const unsigned int kRFC3720_MaxRecvDataSegmentLength_Min = 512;

/*! Maximum allowed received data segment length value per RFC3720. */
static const unsigned int kRFC3720_MaxRecvDataSegmentLength_Max = (2e24-1);

/*! Default maximum burst length value per RFC3720. */
static const unsigned int kRFC3720_MaxBurstLength = 262144;

/*! Minimum maximum burst length value per RFC3720. */
static const unsigned int kRFC3720_MaxBurstLength_Min = 512;

/*! Maximum maximum burst length value per RFC3720. */
static const unsigned int kRFC3720_MaxBurstLength_Max = (2e24-1);

/*! Default first burst length value per RFC3720. */
static const unsigned int kRFC3720_FirstBurstLength = 65536;

/*! Minimum first burst length value per RFC3720. */
static const unsigned int kRFC3720_FirstBurstLength_Min = 512;

/*! Maximum first burst length value per RFC3720. */
static const unsigned int kRFC3720_FirstBurstLength_Max = (2e24-1);

/*! Default time to wait value per RFC3720. */
static const unsigned int kRFC3720_DefaultTime2Wait = 2;

/*! Minimum time to wait value per RFC3720. */
static const unsigned int kRFC3720_DefaultTime2Wait_Min = 0;

/*! Maximum time to wait value per RFC3720. */
static const unsigned int kRFC3720_DefaultTime2Wait_Max = 3600;

/*! Default time to retain value per RFC3720. */
static const unsigned int kRFC3720_DefaultTime2Retain = 20;

/*! Minimum time to retain value per RFC3720. */
static const unsigned int kRFC3720_DefaultTime2Retain_Min = 0;

/*! Maximum time to retain value per RFC3720. */
static const unsigned int kRFC3720_DefaultTime2Retain_Max = 3600;

/*! Default maximum outstanding R2T value per RFC3720. */
static const unsigned int kRFC3720_MaxOutstandingR2T = 1;

/*! Minimum maximum outstanding R2T value per RFC3720. */
static const unsigned int kRFC3720_MaxOutstandingR2T_Min = 1;

/*! Maximum maximum outstanding R2T value per RFC3720. */
static const unsigned int kRFC3720_MaxOutstandingR2T_Max = 65535;

/*! Default data PDU in order value per RFC3720. */
static const bool kRFC3720_DataPDUInOrder = true;

/*! Default data segment in order value per RFC3720. */
static const bool kRFC3720_DataSequenceInOrder = true;

/*! Default error recovery level per RFC3720. */
static const unsigned int kRFC3720_ErrorRecoveryLevel = 0;

/*! Minimum error recovery level per RFC3720. */
static const unsigned int kRFC3720_ErrorRecoveryLevel_Min = 0;

/*! Maximum error recovery level per RFC3720. */
static const unsigned int kRFC3720_ErrorRecoveryLevel_Max = 2;

// The following are not defined by RFC3720, but are defaults used by this
// initiator for various operations in a similar vein.

/*! Minimum discovery interval (seconds). The discovery interval specifies
 *  how often the initiator polls various discovery mechanisms for updates
 *  on target and portal information. A value of 0 indicates that discovery
 *  is done on-demand only. */
static const unsigned int kiSCSIInitiator_DiscoveryInterval_Min = 30;

/*! Maximum discovery interval (seconds). The discovery interval specifies
 *  how often the initiator polls various discovery mechanisms for updates
 *  on target and portal information. A value of 0 indicates that discovery
 *  is done on-demand only. */
static const unsigned int kiSCSIInitiator_DiscoveryInterval_Max = 32767;

/*! Default discovery interval (seconds). The discovery interval specifies
 *  how often the initiator polls various discovery mechanisms for updates
 *  on target and portal information. A value of 0 indicates that discovery
 *  is done on-demand only. */
static const unsigned int kiSCSIInitiator_DiscoveryInterval = 300;

#endif
