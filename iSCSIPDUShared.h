/**
 * @author		Nareg Sinenian
 * @file		iSCSIPDUShared.h
 * @date		April 8, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI PDU definitions that are shared between kernel and user
 *              space PDU libraries. They are included by the header files of 
 *              both the kernel and user space PDU libraries and are thus 
 *              available to a user of the PDU library.
 */

#ifndef __ISCSI_PDU_SHARED_H__
#define __ISCSI_PDU_SHARED_H__

// Reverse DNS notation is necessary for kernel code to prevent namespace
// collisions. For convenience, we abridge the name here.
#define iSCSIPDU com_NSinenian_iSCSIPDU

// If used in user-space, this header will need to include additional
// headers that define primitive fixed-size types.  If used with the kernel,
// IOLib must be included for kernel memory allocation
#ifdef KERNEL
#include <IOKit/IOLib.h>
#else
#include <stdlib.h>
#include <MacTypes.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

///////////////////// BYTE SIZE OF VARIOUS PDU FIELDS //////////////////////

/** Byte size of the data segment length field in all iSCSI PDUs. */
static const unsigned short kiSCSIPDUDataSegmentLengthSize = 3;

static const unsigned short kiSCSIPDUBasicHeaderSegmentSize = 48;

/** Bit offset within opcode byte for request PDUs that should be set to
 *  '1' to indicate immediate delivery of the PDU. */
static const UInt8 kiSCSIPDUImmediateDeliveryFlag = 0x40;

/** Fields that are common to the basic header segment of all PDUs.
 *  The ordering of these fields must not be changed as it matches
 *  the ordering of respective fields in the PDU. */
typedef struct __iSCSIPDUCommonBHS {
    UInt8 opCodeAndDeliveryMarker;
    UInt8 opCodeFields[3];
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 LUNorOpCodeFields;
    UInt32 initiatorTaskTag;
    UInt64 reserved;
    UInt64 reserved2;
    UInt64 reserved3;
    UInt32 reserved4;
} __attribute__((packed)) iSCSIPDUCommonBHS;

/** Fields that are common to the basic header segment of initiator PDUs.
 *  The ordering of these fields must not be changed as it matches
 *  the ordering of respective fields in the PDU. */
typedef struct __iSCSIPDUInitiatorBHS {
    UInt8 opCodeAndDeliveryMarker;
    UInt8 opCodeFields[3];
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 LUNorOpCodeFields;
    UInt32 initiatorTaskTag;
    UInt32 reserved;
    UInt32 cmdSN;
    UInt32 expStatSN;
    UInt32 reserved2;
    UInt64 reserved3;
    UInt32 reserved4;
} __attribute__((packed)) iSCSIPDUInitiatorBHS;

/** Fields that are common to the basic header segment of target PDUs.
 *  The ordering of these fields must not be changed as it matches
 *  the ordering of respective fields in the PDU. */
typedef struct __iSCSIPDUTargetBHS {
    UInt8 opCode;
    UInt8 opCodeFields[3];
    UInt8 totalAHSLength;
    UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
    UInt64 LUNorOpCodeFields;
    UInt32 initiatorTaskTag;
    UInt32 reserved;
    UInt32 statSN;
    UInt32 expCmdSN;
    UInt32 maxCmdSN;
    UInt64 reserved2;
    UInt32 reserved3;
} __attribute__((packed)) iSCSIPDUTargetBHS;



/**	Op codes are used to code PDUs sent form the initiator to the target.
 *  They specify the type of commands or data encoded witin the PDU. */
enum iSCSIPDUInitiatorOpCodes {
    
    /** Initiator command for a ping. */
    kiSCSIPDUOpCodeNOPOut = 0x00,
    
    /** SCSI command send by the initiator. */
    kiSCSIPDUOpCodeSCSICmd = 0x01,
    
    /** Task management request sent by the initiator. */
    kiSCSIPDUOpCodeTaskMgmtReq= 0x02,
    
    /** Login request sent by the initiator. */
    kiSCSIPDUOpCodeLoginReq = 0x03,
    
    /** Text request sent by the initiator. */
    kiSCSIPDUOpCodeTextReq = 0x04,
    
    /** Data sent to a target. */
    kiSCSIPDUOpCodeDataOut = 0x05,
    
    /** Logout request sent by the initiator. */
    kiSCSIPDUOpCodeLogoutReq = 0x06,
    
    /** SNACK request sent by the initiator. */
    kiSCSIPDUOpCodeSNACKReq = 0x10,
    
    /** Maximum allowable initiator op code for error-checking. */
    kiSCSIPDUMaxInitiatorOpCode
};

/**	Op codes are used to code PDUs sent form the initiator to the target.
 *  They specify the type of commands or data encoded witin the PDU. */
enum iSCSIPDUTargetOpCodes {
    
    /** Target repsonse for a ping from the initiator. */
    kiSCSIPDUOpCodeNOPIn = 0x20,
    
    /** Target reponse for a SCSI command. */
    kiSCSIPDUOpCodeSCSIRsp = 0x21,
    
    /** Target response to a task management request. */
    kiSCSIPDUOpCodeTaskMgmtRsp = 0x22,
    
    /** Target response to a login request. */
    kiSCSIPDUOpCodeLoginRsp = 0x23,
    
    /** Target response to  a text request. */
    kiSCSIPDUOpCodeTextRsp = 0x24,
    
    /** Target response with data (e.g., to a SCSI read request). */
    kiSCSIPDUOpCodeDataIn = 0x25,
    
    /** Target response to a logout request. */
    kiSCSIPDUOpCodeLogoutRsp = 0x26,
    
    /** Target response indicating it is ready to transfer. */
    kiSCSIPDUOpCodeR2T = 0x31,
    
    /** Asynchronous message from the target. */
    kiSCSIPDUOpCodeAsyncMsg = 0x32,
     
    /** Response indicating last PDU was rejected. */
    kiSCSIPDUOpCodeReject = 0x3F,
    
    /** Maximum allowable target op code for error-checking. */
    kiSCSIPDUMaxTargetOpCode
};


#endif
