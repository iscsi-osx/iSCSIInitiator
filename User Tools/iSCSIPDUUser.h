/*!
 * @author		Nareg Sinenian
 * @file		iSCSIPDUUser.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI PDU functions.  These functions cannot be used
 *              within the kernel and are intended for use within a daemon on
 *              Mac OS.  They make extensive use of Core Foundation and allow
 *              for allocation, deallocation, transmission and reception of
 *              iSCSI PDU components, including definitions of basic header
 *              segments for various PDUs and data segments.
 */

#ifndef __ISCSI_PDU_USER_H__
#define __ISCSI_PDU_USER_H__

#include "iSCSIPDUShared.h"
#include <CoreFoundation/CoreFoundation.h>


/////////// RFC3720 ALLOWED KEYS FOR SESSION & CONNECTION NEGOTIATION //////////

// Literals used for initial authentication step
static CFStringRef kiSCSILKInitiatorName = CFSTR("InitiatorName");
static CFStringRef kiSCSILKInitiatorAlias = CFSTR("InitiatorAlias");
static CFStringRef kiSCSILKTargetName = CFSTR("TargetName");
static CFStringRef kiSCSILKTargetAlias = CFSTR("TargetAlias");
static CFStringRef kiSCSILKTargetAddress = CFSTR("TargetAddress");

// Literals used to indicate session type
static CFStringRef kiSCSILKSessionType = CFSTR("SessionType");
static CFStringRef kiSCSILVSessionTypeDiscovery = CFSTR("Discovery");
static CFStringRef kiSCSILVSessionTypeNormal = CFSTR("Normal");

// Literals used to indicate different authentication methods
static CFStringRef kiSCSILKAuthMethod = CFSTR("AuthMethod");
static CFStringRef kiSCSILVAuthMethodAll = CFSTR("None,CHAP,KRB5,SPKM1,SPKM2,SRP");
static CFStringRef kiSCSILVAuthMethodNone = CFSTR("None");
static CFStringRef kiSCSILVAuthMethodCHAP = CFSTR("CHAP");

// Literals used during CHAP authentication
static CFStringRef kiSCSILKAuthCHAPDigest = CFSTR("CHAP_A");
static CFStringRef kiSCSILVAuthCHAPDigestMD5 = CFSTR("5");
static CFStringRef kiSCSILKAuthCHAPId = CFSTR("CHAP_I");
static CFStringRef kiSCSILKAuthCHAPChallenge = CFSTR("CHAP_C");
static CFStringRef kiSCSILKAuthCHAPResponse = CFSTR("CHAP_R");
static CFStringRef kiSCSILKAuthCHAPName = CFSTR("CHAP_N");

// Used for grouping connections together (multiple connections must have the
// same group tag or authentication will fail).
static CFStringRef kiSCSILKTargetPortalGroupTag = CFSTR("TargetPortalGroupTag");


static CFStringRef kiSCSILKHeaderDigest = CFSTR("HeaderDigest");
static CFStringRef kiSCSILVHeaderDigestNone = CFSTR("None");
static CFStringRef kiSCSILVHeaderDigestCRC32C = CFSTR("CRC32C");

static CFStringRef kiSCSILKDataDigest = CFSTR("DataDigest");
static CFStringRef kiSCSILVDataDigestNone = CFSTR("None");
static CFStringRef kiSCSILVDataDigestCRC32C = CFSTR("CRC32C");

static CFStringRef kiSCSILKMaxConnections = CFSTR("MaxConnections");
static CFStringRef kiSCSILKTargetGroupPortalTag = CFSTR("TargetGroupPortalTag");

static CFStringRef kiSCSILKInitialR2T = CFSTR("InitialR2T");

static CFStringRef kiSCSILKImmediateData = CFSTR("ImmediateData");

static CFStringRef kiSCSILKMaxRecvDataSegmentLength = CFSTR("MaxRecvDataSegmentLength");
static CFStringRef kiSCSILKMaxBurstLength = CFSTR("MaxBurstLength");
static CFStringRef kiSCSILKFirstBurstLength = CFSTR("FirstBurstLength");
static CFStringRef kiSCSILKDefaultTime2Wait = CFSTR("DefaultTime2Wait");
static CFStringRef kiSCSILKDefaultTime2Retain = CFSTR("DefaultTime2Retain");
static CFStringRef kiSCSILKMaxOutstandingR2T = CFSTR("MaxOutstandingR2T");

static CFStringRef kiSCSILKDataPDUInOrder = CFSTR("DataPDUInOrder");

static CFStringRef kiSCSILKDataSequenceInOrder = CFSTR("DataSequenceInOrder");

static CFStringRef kiSCSILKErrorRecoveryLevel = CFSTR("ErrorRecoveryLevel");
static CFStringRef kiSCSILVErrorRecoveryLevelSession = CFSTR("0");
static CFStringRef kiSCSILVErrorRecoveryLevelDigest = CFSTR("1");
static CFStringRef kiSCSILVErrorRecoveryLevelConnection = CFSTR("2");

static CFStringRef kiSCSILKIFMarker = CFSTR("IFMarker");
static CFStringRef kiSCSILKOFMarker = CFSTR("OFMarker");

/*! The following text commands  and corresponding possible values are used
 *  as key-value pairs during the full-feature phase of the connection. */
static CFStringRef kiSCSITKSendTargets = CFSTR("SendTargets");
static CFStringRef kiSCSITVSendTargetsAll = CFSTR("All");

static CFStringRef kiSCSILVYes = CFSTR("Yes");
static CFStringRef kiSCSILVNo = CFSTR("No");


/*! Basic header segment for a login request PDU. */
typedef struct __iSCSIPDULoginReqBHS {
    const UInt8 opCodeAndDeliveryMarker;
    UInt8 loginStage;
    UInt8 versionMax;
    UInt8 versionMin;
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt8 ISIDa;
    UInt16 ISIDb;
    UInt8 ISIDc;
    UInt16 ISIDd;
    UInt16 TSIH;
    UInt32 initiatorTaskTag;
    UInt16 CID;
    UInt16 reserved;
    UInt32 cmdSN;
    UInt32 expStatSN;
} __attribute__((packed)) iSCSIPDULoginReqBHS;

/*! Basic header segment for a login response PDU. */
typedef struct __iSCSIPDULoginRspBHS {
    const UInt8 opCode;
    UInt8 loginStage;
    UInt8 versionMax;
    UInt8 versionActive;
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt8 ISIDa;
    UInt16 ISIDb;
    UInt8 ISIDc;
    UInt16 ISIDd;
    UInt16 TSIH;
    UInt32 initiatorTaskTag;
    UInt32 reserved;
    UInt32 statSN;
    UInt32 expCmdSN;
    UInt32 maxCmdSN;
    UInt8 statusClass;
    UInt8 statusDetail;
} __attribute__((packed)) iSCSIPDULoginRspBHS;

/*! Basic header segment for a logout request PDU. */
typedef struct __iSCSIPDULogoutReqBHS {
    const UInt8 opCodeAndDeliveryMarker;
    UInt8 reasonCode;
    UInt16 reserved1;
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 reserved2;
    UInt32 initiatorTaskTag;
    UInt16 CID;
    UInt16 reserved3;
    UInt32 cmdSN;
    UInt32 expStatSN;
} __attribute__((packed)) iSCSIPDULogoutReqBHS;

/*! Basic header segment for a logout response PDU. */
typedef struct __iSCSIPDULogoutRspBHS {
    const UInt8 opCode;
    UInt8 reserved1;
    UInt8 response;
    UInt8 reserved2;
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 reserved3;
    UInt32 initiatorTaskTag;
    UInt32 reserved4;
    UInt32 statSN;
    UInt32 expCmdSN;
    UInt32 maxCmdSN;
    UInt32 reserved5;
    UInt16 time2Wait;
    UInt16 time2Retain;
} __attribute__((packed)) iSCSIPDULogoutRspBHS;

/*! Basic header segment for a text request PDU. */
typedef struct __iSCSIPDUTextReqBHS {
    const UInt8 opCodeAndDeliveryMarker;
    UInt8 textReqStageFlags;
    UInt16 reserved;
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 LUNorOpCodeFields;
    UInt32 initiatorTaskTag;
    UInt32 targetTransferTag;
    UInt32 cmdSN;
    UInt32 expStatSN;
    UInt64 reserved2;
    UInt64 reserved3;
} __attribute__((packed)) iSCSIPDUTextReqBHS;

/*! Basic header segment for a text response PDU. */
typedef struct __iSCSIPDUTextRspBHS {
    const UInt8 opCode;
    UInt8 textReqStageBits;
    UInt16 reserved;
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 LUNorOpCodeFields;
    UInt32 initiatorTaskTag;
    UInt32 targetTransferTag;
    UInt32 statSN;
    UInt32 expCmdSN;
    UInt32 maxCmdSN;
    UInt64 reserved2;
    UInt32 reserved3;
} __attribute__((packed)) iSCSIPDUTextRspBHS;

/*! Default initialization for a logout request PDU. */
extern const iSCSIPDULogoutReqBHS iSCSIPDULogoutReqBHSInit;

/*! Default initialization for a login request PDU. */
extern const iSCSIPDULoginReqBHS iSCSIPDULoginReqBHSInit;

/*! Default initialization for a text request PDU. */
extern const iSCSIPDUTextReqBHS iSCSIPDUTextReqBHSInit;


/*! Possible stages of the login process, used with login BHS. */
enum iSCSIPDULoginStages {
    /*! Security negotiation, where initiator/target authenticate
     *  each other. */
    kiSCSIPDUSecurityNegotiation = 0,
    
    /*! Operational negotation, where initiator/target negotiate
     *  whether to use digests, etc. */
    kiSCSIPDULoginOperationalNegotiation = 1,
    
    /*! Full feature phase, where PDUs other than login PDUs can be
     *  sent or received. */
    kiSCSIPDUFullFeaturePhase = 3
};

/*! Reasons for issuing a logout PDU, used with logout BHS. */
enum iSCSIPDULogoutReasons {
    /*! All commands associated with the session are terminated. 
     *  (A session may consist of multiple connections.) */
    kiSCSIPDULogoutCloseSession = 0x00,
    
    /*! All commands associated with the connection are terminated. */
    kISCSIPDULogoutCloseConnection = 0x01,
    
    /*! The connection is removed and commands associated with the
     *  connection are prepared for association with a new connection. */
    kISCSIPDULogoutRemoveConnectionForRecovery = 0x02
};

/*! Responses from a target to a logout request, received within logout BHS. */
enum iSCSIPDULogoutRsp {
    
    /*! The logout was successfully completed. */
    kiSCSIPDULogoutRspSuccess = 0x00,
    
    /*! The connection Id was not found. */
    kiSCSIPDULogoutRspCIDNotFound = 0x01,
    
    /*! Recovery is not supported for this connection or session. */
    kiSCSIPDULogoutRspRecoveryUnsupported = 0x02,
    
    /*! Cleanup failed during logout. */
    kiSCSIPDULogoutRspCleanupFailed = 0x03
};

/*! General login responses from a target, receivd within login BHS. */
enum iSCSIPDULoginRspStatusClass {
    
    /*! Successfully logged onto the target. */
    kiSCSIPDULCSuccess = 0x00,
    
    /*! The target has moved, the response contains redirection
     *  text keys ("TargetAddress=") that can be used to reconnect. */
    kiSCSIPDULCRedirection = 0x01,
    
    /*! Initiator error (e.g., permission denied to requested resource). */
    kiSCSIPDULCInitiatorError = 0x02,
    
    /*! Target error (e.g., target can't fulfill request). */
    kiSCISPDULCTargetError = 0x03
};

////////////////////////////  LOGIN BHS DEFINITIONS ////////////////////////////
// This section various constants that are used only for the login PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.


/*! Bit offsets here start with the low-order bit (e.g., a 0 here corresponds
 *  to the LSB and would correspond to bit 7 if the data was in big-endian
 *  format (this representation is endian neutral with bitwise operators). */

/*! Next login stage bit offset of the login stage byte. */
extern const unsigned short kiSCSIPDULoginNSGBitOffset;

/*! Current login stage bit offset of the login stage byte. */
extern const unsigned short kiSCSIPDULoginCSGBitOffset;

/*! Continue the current stage bit offset. */
extern const UInt8 kiSCSIPDULoginContinueFlag;

/*! Transit to next stage bit offset. */
extern const UInt8 kiSCSIPDULoginTransitFlag;

///////////////////////////  LOGOUT BHS DEFINITIONS ////////////////////////////
// This section various constants that are used only for the logout PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.

/*! Flag that must be applied to the reason code byte of the logout PDU. */
extern const unsigned short kISCSIPDULogoutReasonCodeFlag;


////////////////////////  TEXT REQUEST BHS DEFINITIONS /////////////////////////
// This section various constants that are used only for the logout PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.

/*! Bit offset for the final bit indicating this is the last PDU in the
 *  text request. */
extern const unsigned short kiSCSIPDUTextReqFinalFlag;

/*! Bit offset for the continue bit indicating more text commands are to
 *  follow for this text request. */
extern const unsigned short kiSCSIPDUTextReqContinueFlag;

/*! Get the value of the data segment length field of a PDU.
 *  @param bhs the basic header segment of a PDU.
 *  @return the value of the data segment length. */
static inline size_t iSCSIPDUGetDataSegmentLength(iSCSIPDUCommonBHS * bhs)
{
    UInt32 length = 0;
    memcpy(&length,bhs->dataSegmentLength,kiSCSIPDUDataSegmentLengthSize);
    length = CFSwapInt32BigToHost(length<<8);
    return length;
}

/*! Creates a PDU data segment consisting of key-value pairs from a dictionary.
 *  @param textDict the user-specified dictionary to use.
 *  @param data a pointer to a pointer the data, returned by this function.
 *  @param length the length of the data block, returned by this function. */
void iSCSIPDUDataCreateFromDict(CFDictionaryRef textDict,void * * data,size_t * length);

/*! Creates a PDU data segment of the specified size.
 *  @param length the byte size of the data segment. */
void * iSCSIPDUDataCreate(size_t length);

/*! Releases a PDU data segment created using an iSCSIPDUDataCreate... function.
 *  @param data pointer to the data segment pointer to release. */
void iSCSIPDUDataRelease(void * * data);

/*! Parses key-value pairs to a dictionary.
 *  @param data the data segment (from a PDU) to parse.
 *  @param length the length of the data segment.
 *  @param textDict a dictionary of key-value pairs. */
void iSCSIPDUDataParseToDict(void * data,size_t length,CFMutableDictionaryRef textDict);

/*! Parses key-value pairs to two arrays. This is useful for situations where
 *  the data segment may contain duplicate key names.
 *  @param data the data segment (from a PDU) to parse.
 *  @param length the length of the data segment.
 *  @param keys an array of key values.
 *  @param values an array of corresponding values for each key. */
void iSCSIPDUDataParseToArrays(void * data,size_t length,CFMutableArrayRef keys,CFMutableArrayRef values);


/*! Parses key-value pairs using a user-specified function.
 *  @param data the data segmetn (from a PDU) to parse.
 *  @param length the length of the data segment.
 *  @param keyContainer the container to use for storing key strings (optional).
 *  @param valContainer the container to use for storing value strings (optional).
 *  @param callback a user-specified function that accepts key and value 
 *  containers and key and value strings (i.e. the specific parse functions
 *  defined above specify a particular callback that places the keys and values
 *  into a dictionary or into two separate arrays. */
void iSCSIPDUDataParseCommon(void * data,size_t length,
                             void * keyContainer,
                             void * valContainer,
                             void (*callback)(void * keyContainer,CFStringRef key,
                                              void * valContainer,CFStringRef val));

#endif
