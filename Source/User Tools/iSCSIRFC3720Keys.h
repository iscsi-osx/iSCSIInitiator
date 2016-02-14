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

#ifndef __ISCSI_RFC3720_KEYS_H__
#define __ISCSI_RFC3720_KEYS_H__


/////////// RFC3720 ALLOWED KEYS FOR SESSION & CONNECTION NEGOTIATION //////////

// Literals used for initial authentication step
static CFStringRef kRFC3720_Key_InitiatorName = CFSTR("InitiatorName");
static CFStringRef kRFC3720_Key_InitiatorAlias = CFSTR("InitiatorAlias");
static CFStringRef kRFC3720_Key_TargetName = CFSTR("TargetName");
static CFStringRef kRFC3720_Key_TargetAlias = CFSTR("TargetAlias");
static CFStringRef kRFC3720_Key_TargetAddress = CFSTR("TargetAddress");

// Literals used to indicate session type
static CFStringRef kRFC3720_Key_SessionType = CFSTR("SessionType");
static CFStringRef kRFC3720_Value_SessionTypeDiscovery = CFSTR("Discovery");
static CFStringRef kRFC3720_Value_SessionTypeNormal = CFSTR("Normal");

// Literals used to indicate different authentication methods
static CFStringRef kRFC3720_Key_AuthMethod = CFSTR("AuthMethod");
static CFStringRef kRFC3720_Value_AuthMethodAll = CFSTR("None,CHAP,KRB5,SPKM1,SPKM2,SRP");
static CFStringRef kRFC3720_Value_AuthMethodNone = CFSTR("None");
static CFStringRef kRFC3720_Value_AuthMethodCHAP = CFSTR("CHAP");

// Literals used during CHAP authentication
static CFStringRef kRFC3720_Key_AuthCHAPDigest = CFSTR("CHAP_A");
static CFStringRef kRFC3720_Value_AuthCHAPDigestMD5 = CFSTR("5");
static CFStringRef kRFC3720_Key_AuthCHAPId = CFSTR("CHAP_I");
static CFStringRef kRFC3720_Key_AuthCHAPChallenge = CFSTR("CHAP_C");
static CFStringRef kRFC3720_Key_AuthCHAPResponse = CFSTR("CHAP_R");
static CFStringRef kRFC3720_Key_AuthCHAPName = CFSTR("CHAP_N");

// Used for grouping connections together (multiple connections must have the
// same group tag or authentication will fail).
static CFStringRef kRFC3720_Key_TargetPortalGroupTag = CFSTR("TargetPortalGroupTag");

static CFStringRef kRFC3720_Key_HeaderDigest = CFSTR("HeaderDigest");
static CFStringRef kRFC3720_Value_HeaderDigestNone = CFSTR("None");
static CFStringRef kRFC3720_Value_HeaderDigestCRC32C = CFSTR("CRC32C");

static CFStringRef kRFC3720_Key_DataDigest = CFSTR("DataDigest");
static CFStringRef kRFC3720_Value_DataDigestNone = CFSTR("None");
static CFStringRef kRFC3720_Value_DataDigestCRC32C = CFSTR("CRC32C");

static CFStringRef kRFC3720_Key_MaxConnections = CFSTR("MaxConnections");

static CFStringRef kRFC3720_Key_InitialR2T = CFSTR("InitialR2T");

static CFStringRef kRFC3720_Key_ImmediateData = CFSTR("ImmediateData");

static CFStringRef kRFC3720_Key_MaxRecvDataSegmentLength = CFSTR("MaxRecvDataSegmentLength");
static CFStringRef kRFC3720_Key_MaxBurstLength = CFSTR("MaxBurstLength");
static CFStringRef kRFC3720_Key_FirstBurstLength = CFSTR("FirstBurstLength");
static CFStringRef kRFC3720_Key_DefaultTime2Wait = CFSTR("DefaultTime2Wait");
static CFStringRef kRFC3720_Key_DefaultTime2Retain = CFSTR("DefaultTime2Retain");
static CFStringRef kRFC3720_Key_MaxOutstandingR2T = CFSTR("MaxOutstandingR2T");

static CFStringRef kRFC3720_Key_DataPDUInOrder = CFSTR("DataPDUInOrder");

static CFStringRef kRFC3720_Key_DataSequenceInOrder = CFSTR("DataSequenceInOrder");

static CFStringRef kRFC3720_Key_ErrorRecoveryLevel = CFSTR("ErrorRecoveryLevel");
static CFStringRef kRFC3720_Value_ErrorRecoveryLevelSession = CFSTR("0");
static CFStringRef kRFC3720_Value_ErrorRecoveryLevelDigest = CFSTR("1");
static CFStringRef kRFC3720_Value_ErrorRecoveryLevelConnection = CFSTR("2");

static CFStringRef kRFC3720_Key_IFMarker = CFSTR("IFMarker");
static CFStringRef kRFC3720_Key_OFMarker = CFSTR("OFMarker");

/*! The following text commands  and corresponding possible values are used
 *  as key-value pairs during the full-feature phase of the connection. */
static CFStringRef kRFC3720_Key_SendTargets = CFSTR("SendTargets");
static CFStringRef kRFC3720_Value_SendTargetsAll = CFSTR("All");

static CFStringRef kRFC3720_Value_Yes = CFSTR("Yes");
static CFStringRef kRFC3720_Value_No = CFSTR("No");

// Other keys associated with sessions
static CFStringRef kRFC3720_Key_TargetSessionId = CFSTR("TSIH");

// Not technically a RFC3720 key but used to get the initiator's session identifier
static CFStringRef kRFC3720_Key_SessionId = CFSTR("SessionId");

// Not technically a RFC3720 key but used to get the connection identifier
static CFStringRef kRFC3720_Key_ConnectionId = CFSTR("ConnectionId");

#endif
