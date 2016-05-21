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

/*! Byte size of the data segment length field in all iSCSI PDUs. */
static const unsigned short kiSCSIPDUDataSegmentLengthSize = 3;

/*! Basic header segment size for a PDU. */
static const unsigned short kiSCSIPDUBasicHeaderSegmentSize = 48;

/*! Each PDU must be a multiple of this many bytes.  If the data contained
 *  in a PDU is less than this value it is padded with zeros. */
static const unsigned short kiSCSIPDUByteAlignment = 4;

/*! Reserved target transfer tag value. */
static const UInt32 kiSCSIPDUTargetTransferTagReserved = 0xFFFFFFFF;

/*! Reserved initiator task tag value. */
static const UInt32 kiSCSIPDUInitiatorTaskTagReserved = 0xFFFFFFFF;

/*! Bit offset within opcode byte for request PDUs that should be set to
 *  '1' to indicate immediate delivery of the PDU. */
static const UInt8 kiSCSIPDUImmediateDeliveryFlag = 0x40;

/*! Fields that are common to the basic header segment of all PDUs.
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

/*! Fields that are common to the basic header segment of initiator PDUs.
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

/*! Fields that are common to the basic header segment of target PDUs.
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

/*! Possible reject codes that may be issued throughout the login process. */
enum iSCSIPDURejectCode {
    
    /*! Reserved reject code (not used). */
    kiSCSIPDURejectReserved = 0x01,
    
    /*! Data digest error (may resend original PDU). */
    kiSCSIPDURejectDataDigestError = 0x02,
    
    /*! Sequence ack was rejected (may resend original PDU). */
    kiSCSIPDURejectSNACKReject = 0x03,
    
    /*! iSCSI protocol error has occured (e.g., SNACK was issued
     *  for something that was already acknowledged). */
    kiSCSIPDURejectProtoError = 0x04,
    
    /*! The command is not supported. */
    kiSCSIPDURejectCmdNotSupported = 0x05,
    
    /*! Too many commands. */
    kiSCSIPDURejectTooManyImmediateCmds = 0x06,
    
    /*! There is a task in progress. */
    kiSCSIPDURejectTaskInProgress = 0x07,
    
    /*! Invalid data acknowledgement. */
    kiSCSIPDURejectInvalidDataACK = 0x08,
    
    /*! A PDU field was invalid. */
    kiSCSIPDURejectInvalidPDUField = 0x09,
    
    /*! Can't generate target transfer tag; out of resources. */
    kiSCSIPDURejectLongOperationReject = 0x0a,
    
    /*! Negotiation was reset. */
    kiSCSIPDURejectNegotiationReset = 0x0b,
    
    /*! Waiting to logout. */
    kiSCSIPDURejectWaitingForLogout = 0x0c
};

/*! Asynchronous iSCSI events to be handled by the session layer. */
enum iSCSIPDUAsyncMsgEvent {
    
    /*! SCSI asynchronous event (with sense data). */
    kiSCSIPDUAsyncMsgSCSIAsyncMsg = 0x00,
    
    /*! Target requests logout. */
    kiSCSIPDUAsyncMsgLogout = 0x01,
    
    /*! Target will drop connection. */
    kiSCSIPDUAsynMsgDropConnection = 0x02,
    
    /*! Target will drop all connections. */
    kiSCSIPDUAsyncMsgDropAllConnections = 0x03,
    
    /*! Target requests parameter renegotiation. */
    kiSCSIPDUAsyncMsgNegotiateParams = 0x04,
    
    /*! Vendor specific event. */
    kiSCSIPDUAsyncMsgVendorCode = 0xFF
};

/*!	Op codes are used to code PDUs sent form the initiator to the target.
 *  They specify the type of commands or data encoded witin the PDU. */
enum iSCSIPDUInitiatorOpCodes {
    
    /*! Initiator command for a ping. */
    kiSCSIPDUOpCodeNOPOut = 0x00,
    
    /*! SCSI command send by the initiator. */
    kiSCSIPDUOpCodeSCSICmd = 0x01,
    
    /*! Task management request sent by the initiator. */
    kiSCSIPDUOpCodeTaskMgmtReq= 0x02,
    
    /*! Login request sent by the initiator. */
    kiSCSIPDUOpCodeLoginReq = 0x03,
    
    /*! Text request sent by the initiator. */
    kiSCSIPDUOpCodeTextReq = 0x04,
    
    /*! Data sent to a target. */
    kiSCSIPDUOpCodeDataOut = 0x05,
    
    /*! Logout request sent by the initiator. */
    kiSCSIPDUOpCodeLogoutReq = 0x06,
    
    /*! SNACK request sent by the initiator. */
    kiSCSIPDUOpCodeSNACKReq = 0x10,
    
    /*! Maximum allowable initiator op code for error-checking. */
    kiSCSIPDUMaxInitiatorOpCode
};

/*!	Op codes are used to code PDUs sent form the initiator to the target.
 *  They specify the type of commands or data encoded witin the PDU. */
enum iSCSIPDUTargetOpCodes {
    
    /*! Target repsonse for a ping from the initiator. */
    kiSCSIPDUOpCodeNOPIn = 0x20,
    
    /*! Target reponse for a SCSI command. */
    kiSCSIPDUOpCodeSCSIRsp = 0x21,
    
    /*! Target response to a task management request. */
    kiSCSIPDUOpCodeTaskMgmtRsp = 0x22,
    
    /*! Target response to a login request. */
    kiSCSIPDUOpCodeLoginRsp = 0x23,
    
    /*! Target response to  a text request. */
    kiSCSIPDUOpCodeTextRsp = 0x24,
    
    /*! Target response with data (e.g., to a SCSI read request). */
    kiSCSIPDUOpCodeDataIn = 0x25,
    
    /*! Target response to a logout request. */
    kiSCSIPDUOpCodeLogoutRsp = 0x26,
    
    /*! Target response indicating it is ready to transfer. */
    kiSCSIPDUOpCodeR2T = 0x31,
    
    /*! Asynchronous message from the target. */
    kiSCSIPDUOpCodeAsyncMsg = 0x32,
     
    /*! Response indicating last PDU was rejected. */
    kiSCSIPDUOpCodeReject = 0x3F,
    
    /*! Maximum allowable target op code for error-checking. */
    kiSCSIPDUMaxTargetOpCode
};


#endif
