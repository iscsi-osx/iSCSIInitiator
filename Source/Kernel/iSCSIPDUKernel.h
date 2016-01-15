/*!
 * @author		Nareg Sinenian
 * @file		iSCSIPDUKernel.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Kernel-space iSCSI PDU functions.  These functions cannot be
 *              used within user-space.
 */


#ifndef __ISCSI_PDU_USER_H__
#define __ISCSI_PDU_USER_H__

#include "iSCSIPDUShared.h"
#include <sys/socket.h>
#include <sys/errno.h>
#include <IOKit/IOLib.h>


namespace iSCSIPDU {

    ///////////////////// For use with several types of PDUS ///////////////////
    
    /*! Size of a normal SCSI command descriptor block (CDB). */
    static const UInt8 kiSCSIPDUCDBSize = 16;
    
    /*! Digests are 4 bytes. */
    static const UInt8 kiSCSIPDUDigestSize = 4;
    
    /*! Flag used in some incoming and outgoing PDUs in the first reserved
     *  following the opCode and delivery marker byte (e.g., NOPOut, R2T, ...) */
    static const UInt8 kiSCSIPDUReservedFlag = 0x80;

    
    ///////////////////// For use with SCSI command PDUs ///////////////////////
    
    /*! No unsolicited data out PDU follows the SCSI command. */
    static const UInt8 kiSCSIPDUSCSICmdFlagNoUnsolicitedData = 0x80;

    /*! Indicates a SCSI write command. */
    static const UInt8 kiSCSIPDUSCSICmdFlagWrite = 0x20;

    /*! Indicates a SCSI read command. */
    static const UInt8 kiSCSIPDUSCSICmdFlagRead = 0x40;
    
    /*! An untagged SCSI command. */
    static const UInt8 kiSCSIPDUSCSICmdTaskAttrUntagged = 0x00;

    /*! A simple SCSI command. */
    static const UInt8 kiSCSIPDUSCSICmdTaskAttrSimple = 0x01;

    /*! An ordered SCSI command. */
    static const UInt8 kiSCSIPDUSCSICmdTaskAttrOrdered = 0x02;

    /*! A head of queue SCSI command. */
    static const UInt8 kiSCSIPDUSCSICmdTaskAttrHead = 0x03;

    /*! An ACA SCSI command. */
    static const UInt8 kiSCSIPDUSCSICmdTaskAttrACA = 0x04;
    
    
    ///////////////// For for use with task management PDUs ////////////////////
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncFlag = 0x80;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncAbortTask = 0x01;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncAbortTaskSet = 0x02;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncClearACA = 0x03;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncClearTaskSet = 0x04;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncLUNReset = 0x05;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncTargetWarmReset = 0x06;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncTargetColdReset = 0x07;
    
    static const UInt8 kiSCSIPDUTaskMgmtFuncTaskReassign = 0x08;
    
    enum iSCSIPDUTaskMgmtRspCodes {
        kiSCSIPDUTaskMgmtFuncComplete = 0x00,
        kiSCSIPDUTaskMgmtInvalidTask  = 0x01,
        kiSCSIPDUTaskMgmtInvalidLUN   = 0x02,
        kiSCSIPDUTaskMgmtTaskAllegiant = 0x03,
        kiSCSIPDUTaskMgmtReassignUnsupported = 0x04,
        kiSCSIPDUTaskMgmtFuncUnsupported = 0x05,
        kiSCSIPDUTaskMgmtAuthFail = 0x06,
        kiSCSIPDUTaskMgmtFuncRejected = 0xFF
    };
    
    
    ////////////////////// For for use with data out PDUs //////////////////////

    static const UInt8 kiSCSIPDUDataOutFinalFlag = 0x80;
    
    
    ////////////////////// For for use with data in PDUs ///////////////////////
    
    static const UInt8 kiSCSIPDUDataInFinalFlag = 0x80;
    
    static const UInt8 kiSCSIPDUDataInAckFlag = 0x40;

    static const UInt8 kiSCSIPDUDataInStatusFlag = 0x01;
    
    /*! Basic header segment for a data in PDU. */
    typedef struct __iSCSIPDUDataInBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt8 reserved;
        UInt8 status;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 targetTransferTag;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt32 dataSN;
        UInt32 bufferOffset;
        UInt32 residualCount;
    } __attribute__((packed)) iSCSIPDUDataInBHS;
    
    /*! Basic header segment for a data out PDU. */
    typedef struct __iSCSIPDUDataOutBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt16 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 targetTransferTag;
        UInt32 reserved2;
        UInt32 expStatSN;
        UInt32 reserved3;
        UInt32 dataSN;
        UInt32 bufferOffset;
        UInt32 reserved4;
    } __attribute__((packed)) iSCSIPDUDataOutBHS;
    
    /*! Basic header segment for a SCSI command PDU. */
    typedef struct __iSCSIPDUSCSICmdBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt16 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 dataTransferLength;
        UInt32 cmdSN;
        UInt32 expStatSN;
        UInt8 CDB[kiSCSIPDUCDBSize];
    } __attribute__((packed)) iSCSIPDUSCSICmdBHS;
    
    /*! Basic header segment for a SCSI response PDU. */
    typedef struct __iSCSIPDUSCSIRspBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt8 response;
        UInt8 status;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 reserved2;
        UInt32 initiatorTaskTag;
        UInt32 SNACKTag;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt32 expDataSN;
        UInt32 biReadResidualCount;
        UInt32 residualCount;
    } __attribute__((packed)) iSCSIPDUSCSIRspBHS;
    
    /*! Basic header segment for a target management request PDU. */
    typedef struct __iSCSIPDUTaskMgmtReqBHS {
        const UInt8 opCode;
        UInt8 function;
        UInt16 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 referencedTaskTag;
        UInt32 cmdSN;
        UInt32 expStatSN;
        UInt32 refCmdSN;
        UInt32 expDataSN;
        UInt64 reserved2;
    } __attribute__((packed)) iSCSIPDUTaskMgmtReqBHS;
    
    /*! Basic header segment for a target management response PDU. */
    typedef struct __iSCSIPDUTaskMgmtRspBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt8 response;
        UInt8 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 reserved2;
        UInt32 initiatorTaskTag;
        UInt32 reserved3;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt64 reserved4;
        UInt32 reserved5;
    } __attribute__((packed)) iSCSIPDUTaskMgmtRspBHS;
    
    /*! Basic header segment for an R2T PDU. */
    typedef struct __iSCSIPDUR2TBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt16 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 targetTransferTag;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt32 R2TSN;
        UInt32 bufferOffset;
        UInt32 desiredDataLength;
    } __attribute__((packed)) iSCSIPDUR2TBHS;
    
    /*! Basic header segment for a SNACK request PDU. */
    typedef struct __iSCSIPDUSNACKReqBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt16 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 targetTransferTag;
        UInt32 reserved2;
        UInt32 expStatSN;
        UInt64 reserved3;
        UInt32 begRun;
        UInt32 runLength;
    } __attribute__((packed)) iSCSIPDUSNACKReqBHS;
    
    /*! Basic header segment for a reject PDU. */
    typedef struct __iSCSIPDURejectBHS {
        const UInt8 opCode;
        UInt8 reserved;
        UInt8 reason;
        UInt8 reserved2;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 reserved3;
        UInt32 reserved4;
        UInt32 flag;
        UInt32 reserved5;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt32 dataSNorR2TSN;
        UInt32 reserved6;
        UInt32 reserved7;
    } __attribute__((packed)) iSCSIPDURejectBHS;
    
    /*! Basic header segment for an asynchronous message PDU. */
    typedef  struct __iSCSIPDUAsyncMsgBHS {
        const UInt8 opCode;
        UInt8 reserved;
        UInt16 reserved2;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 flag;
        UInt32 reserved3;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt8 asyncEvent;
        UInt8 asyncVCode;
        UInt16 parameter1;
        UInt16 parameter2;
        UInt16 parameter3;
        UInt32 reserved6;
    } __attribute__((packed)) iSCSIPDUAsyncMsgBHS;
    
    /*! Basic header segment for a NOP out PDU. */
    typedef struct __iSCSIPDUNOPOutBHS {
        UInt8 opCode;
        UInt8 reserved;
        UInt8 reserved2;
        UInt8 reserved3;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 targetTransferTag;
        UInt32 cmdSN;
        UInt32 expStatSN;
        UInt64 reserved4;
        UInt64 reserved5;
    } __attribute__((packed)) iSCSIPDUNOPOutBHS;
    
    /*! Basic header segment for an NOP in PDU. */
    typedef struct __iSCSIPDUNOPInBHS {
        const UInt8 opCode;
        UInt8 flags;
        UInt16 reserved;
        UInt8 totalAHSLength;
        UInt8 dataSegmentLength[kiSCSIPDUDataSegmentLengthSize];
        UInt64 LUN;
        UInt32 initiatorTaskTag;
        UInt32 targetTransferTag;
        UInt32 statSN;
        UInt32 expCmdSN;
        UInt32 maxCmdSN;
        UInt32 reserved2;
        UInt64 reserved3;
    } __attribute__((packed)) iSCSIPDUNOPInBHS;
    
    /*! Additional header segment format common to all AHS. */
    typedef struct __iSCSIPDUCommonAHS {
        UInt16 ahsLength;
        const UInt8  ahsType;
        UInt8  reserved;
    } __attribute__((packed)) iSCSIPDUCommonAHS;
    
    /*! Additional header segmetn for extended CDBs. */
    typedef struct __iSCSIPDUExtCDBAHS {
        UInt16 ahsLength;
        const UInt8  ahsType;
        UInt8  reserved;
        UInt8  * extendedCDB;
    } __attribute__((packed)) iSCSIPDUExtCDBAHS;
    
    /*! Additional header segment for bi-directional read AHS. */
    typedef struct __iSCSIPDUBiReadAHS {
        UInt16 ahsLength;
        const UInt8  ahsType;
        UInt8  reserved;
        UInt32 readDataLength;
    } __attribute__((packed)) iSCSIPDUBiReadAHS;
    
    /*! Additional header segment types for PDUs. */
    enum iSCSIPDUAHSTypes {
        
        /*! Extended CDB AHS. */
        kiSCSIPDUAHSExtCDB = 0x01,
        
        /*! Bi-directional read AHS. */
        kiSCSIPDUAHSBiRead = 0x02,
    };
    
    /*! SCSI response PDU valid values for the response field. */
    enum iSCSIPDUSCSIRspBHSResponse {
        
        /*! Command was completed at the target. */
        kiSCSIPDUSCSICmdCompleted = 0x00,
        
        /*! Target failure. */
        kiSCSIPDUSCSICmdTargetFailure = 0x01
    };
    
    inline size_t iSCSIPDUGetDataSegmentLength(iSCSIPDUTargetBHS * bhs)
    {
        UInt32 length = 0;
        memcpy(&length,bhs->dataSegmentLength,kiSCSIPDUDataSegmentLengthSize);
        length = OSSwapBigToHostInt32(length<<8);
        return length;
    }
    
    inline size_t iSCSIPDUGetPaddedDataSegmentLength(iSCSIPDUTargetBHS * bhs)
    {
        size_t length = iSCSIPDUGetDataSegmentLength(bhs);
        UInt32 padding = (UInt32)(length - length % 4);
        
        if(padding != 4)
            return length+padding;
        return length;
    }
    
    extern const iSCSIPDUDataOutBHS iSCSIPDUDataOutBHSInit;
    extern const iSCSIPDUSCSICmdBHS iSCSIPDUSCSICmdBHSInit;
    extern const iSCSIPDUTaskMgmtReqBHS iSCSIPDUTaskMgmtReqBHSInit;
    extern const iSCSIPDUSNACKReqBHS iSCSIPDUSNACKReqBHSInit;
    extern const iSCSIPDUNOPOutBHS iSCSIPDUNOPOutBHSInit;
    extern const iSCSIPDUExtCDBAHS iSCSIPDUExtCDBAHSInit;
    extern const iSCSIPDUBiReadAHS iSCSIPDUBiReadAHSInit;
};

#endif
