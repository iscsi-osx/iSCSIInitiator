//
//  iSCSIQueryTarget.c
//  iSCSI_Initiator
//
//  Created by Nareg Sinenian on 1/18/15.
//
//

#include "iSCSIQueryTarget.h"

/*! Helper function used throughout the login process to query the target.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate login PDU to the target.  It will then receive one or more
 *  login response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  parse the iSCSI error and express it in terms of a system errno_t as
 *  the system would treat any other device.
 *  @param context the context to query (session identifier, etc)
 *  @param statusCode the iSCSI status code returned by the target
 *  @param rejectCode the iSCSI reject code, if the command was rejected
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionLoginQuery(struct iSCSILoginQueryContext * context,
                               enum iSCSIStatusCode * statusCode,
                               enum iSCSIRejectCode * rejectCode,
                               CFDictionaryRef   textCmd,
                               CFMutableDictionaryRef  textRsp)
{
    // Create a new login request basic header segment
    iSCSIPDULoginReqBHS cmd = iSCSIPDULoginReqBHSInit;
    cmd.TSIH  = CFSwapInt16HostToBig(context->TSIH);
    cmd.CID   = CFSwapInt32HostToBig(context->connectionId);
    cmd.ISIDd = CFSwapInt16HostToBig(context->sessionId);
    cmd.loginStage  = (context->nextStage << kiSCSIPDULoginNSGBitOffset);
    cmd.loginStage |= (context->currentStage << kiSCSIPDULoginCSGBitOffset);
    
    // If stages aren't the same then we are going to transition
    if(context->currentStage != context->nextStage)
        cmd.loginStage |= kiSCSIPDULoginTransitFlag;
    
    // Create a data segment based on text commands (key-value pairs)
    void * data;
    size_t length;
    iSCSIPDUDataCreateFromDict(textCmd,&data,&length);
    
    errno_t error = iSCSIKernelSend(context->sessionId,context->connectionId,
                                    (iSCSIPDUInitiatorBHS *)&cmd,
                                    data,length);
    iSCSIPDUDataRelease(&data);
    
    if(error) {
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDULoginRspBHS rsp;
    
    do {
        if((error = iSCSIKernelRecv(context->sessionId,context->connectionId,
                                    (iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);
            return error;
        }
        
        if(rsp.opCode == kiSCSIPDUOpCodeLoginRsp)
        {
            //            connInfo->status = *((UInt16*)(&rsp.statusClass));
            
            if(error)
                break;
            
            iSCSIPDUDataParseToDict(data,length,textRsp);
        }
        // For this case some other kind of PDU or invalid data was received
        else if(rsp.opCode == kiSCSIPDUOpCodeReject)
        {
            error = EINVAL;
            break;
        }
    }
    while(rsp.loginStage & kiSCSIPDUTextReqContinueFlag);
    
    iSCSIPDUDataRelease(&data);
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
errno_t iSCSISessionTextQuery(UInt16 sessionId,
                              UInt32 connectionId,
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
    
    errno_t error = iSCSIKernelSend(sessionId,
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
        if((error = iSCSIKernelRecv(sessionId,connectionId,
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
            error = EINVAL;
            break;
        }
    }
    while(rsp.textReqStageBits & kiSCSIPDUTextReqContinueFlag);
    
    iSCSIPDUDataRelease(&data);
    return error;
}