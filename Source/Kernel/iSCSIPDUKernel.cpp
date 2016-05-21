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

#include "iSCSIPDUKernel.h"

namespace iSCSIPDU {
    
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

    const iSCSIPDUSCSICmdBHS iSCSIPDUSCSICmdBHSInit = {
        .opCode             = kiSCSIPDUOpCodeSCSICmd,
        .flags              = 0,
        .reserved           = 0,
        .totalAHSLength     = 0,
        .LUN                = 0,
        .initiatorTaskTag   = 0,
        .dataTransferLength = 0 };
    
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

    const iSCSIPDUNOPOutBHS iSCSIPDUNOPOutBHSInit = {
        .opCode             = kiSCSIPDUOpCodeNOPOut,
        .reserved           = kiSCSIPDUReservedFlag,
        .reserved2          = 0,
        .reserved3          = 0,
        .totalAHSLength     = 0,
        .LUN                = 0,
        .initiatorTaskTag   = 0,
        .targetTransferTag  = 0,
        .reserved4          = 0,
        .reserved5          = 0 };
    
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


