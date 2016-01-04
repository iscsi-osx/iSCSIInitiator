/*!
 * @author		Nareg Sinenian
 * @file		iSCSIRFC3720Defaults.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Default values of various parameters as defined in RFC3720
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
