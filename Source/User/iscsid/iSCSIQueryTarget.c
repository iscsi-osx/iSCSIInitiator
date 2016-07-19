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

#include "iSCSIQueryTarget.h"
#include "iSCSIHBAInterface.h"

errno_t iSCSISessionLoginSingleQuery(struct iSCSILoginQueryContext * context,
                                     enum iSCSILoginStatusCode * statusCode,
                                     enum iSCSIPDURejectCode * rejectCode,
                                     CFDictionaryRef   textCmd,
                                     CFMutableDictionaryRef  textRsp)
{
    // Create a new login request basic header segment
    iSCSIPDULoginReqBHS cmd = iSCSIPDULoginReqBHSInit;
    cmd.TSIH  = CFSwapInt16HostToBig(context->targetSessionId);
    cmd.CID   = CFSwapInt32HostToBig(context->connectionId);
    cmd.ISIDd = CFSwapInt16HostToBig(context->sessionId);
    cmd.loginStage  = (context->nextStage << kiSCSIPDULoginNSGBitOffset);
    cmd.loginStage |= (context->currentStage << kiSCSIPDULoginCSGBitOffset);
    
    // If stages aren't the same then we are going to transition
    if(context->currentStage != context->nextStage)
        cmd.loginStage |= kiSCSIPDULoginTransitFlag;
    
    // Create a data segment based on text commands (key-value pairs)
    void * data = NULL;
    size_t length = 0;
    iSCSIPDUDataCreateFromDict(textCmd,&data,&length);

    errno_t error = iSCSIHBAInterfaceSend(context->interface,context->sessionId,context->connectionId,
                                          (iSCSIPDUInitiatorBHS *)&cmd,data,length);
    iSCSIPDUDataRelease(&data);
    
    if(error) {
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDULoginRspBHS rsp;
    
    do {
        if((error = iSCSIHBAInterfaceReceive(context->interface,context->sessionId,context->connectionId,
                                             (iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);
            return error;
        }
        
        if(rsp.opCode == kiSCSIPDUOpCodeLoginRsp)
        {
            // Per RFC3720, the status and detail together make up the code
            // where the class is the high byte and the detail is the low
            *statusCode = ((((UInt16)rsp.statusClass)<<8) | rsp.statusDetail);

            if(*statusCode != kiSCSILoginSuccess)
                break;
            
            iSCSIPDUDataParseToDict(data,length,textRsp);
            
            // Save & return the TSIH if this is the leading login
            if(context->targetSessionId == 0 && context->nextStage == kiSCSIPDUFullFeaturePhase) {
                context->targetSessionId = CFSwapInt16BigToHost(rsp.TSIH);
            }
            
            // Save the status sequence number and expected
            // command sequence number
            context->statSN = rsp.statSN;
            context->expCmdSN = rsp.expCmdSN;
            context->transitNextStage = (rsp.loginStage & kiSCSIPDULoginTransitFlag);
        }
        // For this case some other kind of PDU or invalid data was received
        else if(rsp.opCode == kiSCSIPDUOpCodeReject)
        {
            error = EOPNOTSUPP;
            break;
        }
    }
    while(rsp.loginStage & kiSCSIPDUTextReqContinueFlag);
    
    iSCSIPDUDataRelease(&data);
    return error;
}


/*! Helper function used throughout the login process to query the target.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate login PDU to the target.  It will then receive one or more
 *  login response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  return the C error code.  If communications are successful but the iSCSI
 *  layer experiences errors, it will return an iSCSI error code, either in the
 *  form of a login status code or a PDU rejection code in addition to
 *  a standard C error code. If the nextStage field of the context struct
 *  specifies the full feature phase, this function will return a valid TSIH.
 *  @param context the context to query (session identifier, etc)
 *  @param statusCode the iSCSI status code returned by the target
 *  @param rejectCode the iSCSI reject code, if the command was rejected
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionLoginQuery(struct iSCSILoginQueryContext * context,
                               enum iSCSILoginStatusCode * statusCode,
                               enum iSCSIPDURejectCode * rejectCode,
                               CFDictionaryRef   textCmd,
                               CFMutableDictionaryRef  textRsp)
{
    // Try a single query first
    errno_t error = iSCSISessionLoginSingleQuery(context,statusCode,rejectCode,textCmd,textRsp);
    
    // If error occured, do nothing
    if(error || *statusCode != kiSCSILoginSuccess)
        return error;
    
    // If we are not transitioning stages, or we are and the target agreed to
    // transition, then we can move on...
    if(context->currentStage == context->nextStage || context->transitNextStage)
        return error;

    // If we were expecting the target to advance to the next stage; make sure
    // it agrees.  If it does not, we need to send login queries until it does
    // (with a maximum count specified below).  See RFC3720 at Section 5.4.
    CFIndex maxRetryCount = 5;
    CFIndex retryCount = 0;
    
    for(retryCount = 0; retryCount < maxRetryCount; retryCount++)
    {
        // Retries are blank, to get target to advance (per RFC3720)
        error = iSCSISessionLoginSingleQuery(context,statusCode,rejectCode,NULL,NULL);
        
        if(context->transitNextStage)
            break;
    }
    
    // If the target refuses to advance after max. retry count,
    // set an iSCSI error and quit
    if(retryCount == maxRetryCount)
        *statusCode = kiSCSILoginInvalidReqDuringLogin;
    
    return error;
}

/*! Helper function used during the full feature phase of a connection to
 *  send and receive text requests and responses.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate text request PDU to the target.  It will then receive one or more
 *  text response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  parse the iSCSI error and express it in terms of a system errno_t as
 *  the system would treat any other device.
 *  @param sessionId the session identifier.
 *  @param connectionId a connection identifier.
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionTextQuery(SessionIdentifier sessionId,
                              ConnectionIdentifier connectionId,
                              CFDictionaryRef   textCmd,
                              CFMutableDictionaryRef  textRsp)
{
    // Create a new login request basic header segment
    iSCSIPDUTextReqBHS cmd = iSCSIPDUTextReqBHSInit;
    cmd.textReqStageFlags = 0;
    
    // Create a data segment based on text commands (key-value pairs)
    void * data;
    size_t length;
    iSCSIPDUDataCreateFromDict(textCmd,&data,&length);
    
    errno_t error = iSCSIHBAInterfaceSend(0,sessionId,
                                    connectionId,
                                    (iSCSIPDUInitiatorBHS *)&cmd,
                                    data,length);
    iSCSIPDUDataRelease(&data);
    
    if(error) {
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDUTextRspBHS rsp;
    
    do {
        if((error = iSCSIHBAInterfaceReceive(0,sessionId,connectionId,
                                    (iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);
            return error;
        }
        
        if(rsp.opCode == kiSCSIPDUOpCodeTextRsp)
            iSCSIPDUDataParseToDict(data,length,textRsp);
        
        // For this case some other kind of PDU or invalid data was received
        else if(rsp.opCode == kiSCSIPDUOpCodeReject)
        {
            error = EIO;
            break;
        }
    }
    while(rsp.textReqStageBits & kiSCSIPDUTextReqContinueFlag);
    
    iSCSIPDUDataRelease(&data);
    return error;
}