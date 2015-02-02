/*!
 * @author		Nareg Sinenian
 * @file		iSCSIPDUKernel.cpp
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Kernel-space iSCSI PDU functions.  These functions cannot be
 *              used within user-space.
 */


#include "iSCSIPDUKernel.h"

namespace iSCSIPDU {
    
    // final
    const iSCSIPDUDataOutBHS iSCSIPDUDataOutBHSInit = {
        .opCode             = kiSCSIPDUOpCodeDataOut,
        .flags              = 0,
        .reserved           = 0,
        .totalAHSLength     = 0,
        .LUN                = 0,
        .initiatorTaskTag   = 0,
        .targetTransferTag  = 0,
        .reserved2          = 0,
        .reserved3          = 0,
        .dataSN             = 0,
        .bufferOffset       = 0,
        .reserved4          = 0 };

    // final
    const iSCSIPDUSCSICmdBHS iSCSIPDUSCSICmdBHSInit = {
        .opCode             = kiSCSIPDUOpCodeSCSICmd,
        .flags              = 0,
        .reserved           = 0,
        .totalAHSLength     = 0,
        .LUN                = 0,
        .initiatorTaskTag   = 0,
        .dataTransferLength = 0 };
    
    // final
    const iSCSIPDUTaskMgmtReqBHS iSCSIPDUTaskMgmtReqBHSInit = {
        .opCode             = kiSCSIPDUOpCodeTaskMgmtReq,
        .function           = 0,
        .reserved           = 0,
        .totalAHSLength     = 0,
        .LUN                = 0,
        .initiatorTaskTag   = 0,
        .referencedTaskTag  = 0,
        .refCmdSN           = 0,
        .reserved2          = 0 };
        
    const iSCSIPDUSNACKReqBHS iSCSIPDUSNACKReqBHSInit {
        .opCode = kiSCSIPDUOpCodeSNACKReq,
        .flags = 0,
        .totalAHSLength = 0};

    // final
    const iSCSIPDUNOPOutBHS iSCSIPDUNOPOutBHSInit = {
        .opCode             = kiSCSIPDUOpCodeNOPOut,
        .flags              = 0,
        .reserved           = 0,
        .totalAHSLength     = 0,
        .LUN                = 0,
        .initiatorTaskTag   = 0,
        .targetTransferTag  = 0,
        .reserved2          = 0,
        .reserved3          = 0 };
    
    const iSCSIPDUExtCDBAHS iSCSIPDUExtCDBAHSInit = {
        .ahsLength = 0,
        .ahsType   = kiSCSIPDUAHSExtCDB,
        .reserved  = 0,
        .extendedCDB = NULL };
    
    /*! Additional header segment for bi-directional read AHS. */
    const iSCSIPDUBiReadAHS iSCSIPDUBiReadAHSInit = {
        .ahsLength = 0,
        .ahsType = kiSCSIPDUAHSBiRead,
        .reserved = 0,
        .readDataLength = 0 };
    
};


