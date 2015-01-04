/*!
 * @author		Nareg Sinenian
 * @file		iSCSISession.h
 * @date		July 24, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI session management functions.  This library
 *              depends on the user-space iSCSI PDU library to login, logout
 *              and perform discovery functions on iSCSI target nodes.
 */

#include "iSCSISession.h"
#include "iSCSIPDUUser.h"
#include "iSCSIKernelInterface.h"

/*! Name of the initiator. */
CFStringRef kiSCSIInitiatorName = CFSTR("default");

/*! Alias of the initiator. */
CFStringRef kiSCSIInitiatorAlias = CFSTR("default");

/*! The iSCSI error code that is set to a particular value if session management
 *  functions cannot be completed successfully.  This is distinct from errors
 *  associated with system function calls.  If no system error has occured
 *  (e.g., socket API calls) an iSCSI error may still have occured. */
static UInt16 iSCSILoginResponseCode = 0;

/*! Authentication function defined in the authentication module
 *  (in the file iSCSIAuth.h). */
errno_t iSCSIAuthNegotiate(iSCSITarget * const target,
                           UInt16 sessionId,
                           UInt32 connectionId,
                           iSCSISessionOptions * sessionOptions);

/*! Authentication function defined in the authentication module
 *  (in the file iSCSIAuth.h). */
errno_t iSCSIAuthInterrogate(iSCSITarget * const target,
                             UInt16 sessionId,
                             UInt32 connectionId,
                             iSCSISessionOptions * sessionOptions,
                             CFStringRef * authMethods);


/*! Maximum number of key-value pairs supported by a dictionary that is used
 *  to produce the data section of text and login PDUs. */
const unsigned int kiSCSISessionMaxTextKeyValuePairs = 100;

static const unsigned int kRFC3720_kiSCSISessionTimeoutMs = 1000;


/////////// RFC3720 ALLOWED VALUES FOR SESSION & CONNECTION PARAMETERS /////////

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


/////////// RFC3720 ALLOWED KEYS FOR SESSION & CONNECTION NEGOTIATION //////////


CFStringRef kiSCSILKHeaderDigest = CFSTR("HeaderDigest");
CFStringRef kiSCSILVHeaderDigestNone = CFSTR("None");
CFStringRef kiSCSILVHeaderDigestCRC32C = CFSTR("CRC32C");

CFStringRef kiSCSILKDataDigest = CFSTR("DataDigest");
CFStringRef kiSCSILVDataDigestNone = CFSTR("None");
CFStringRef kiSCSILVDataDigestCRC32C = CFSTR("CRC32C");

CFStringRef kiSCSILKMaxConnections = CFSTR("MaxConnections");
CFStringRef kiSCSILKTargetGroupPortalTag = CFSTR("TargetGroupPortalTag");

CFStringRef kiSCSILKInitialR2T = CFSTR("InitialR2T");

CFStringRef kiSCSILKImmediateData = CFSTR("ImmediateData");

CFStringRef kiSCSILKMaxRecvDataSegmentLength = CFSTR("MaxRecvDataSegmentLength");
CFStringRef kiSCSILKMaxBurstLength = CFSTR("MaxBurstLength");
CFStringRef kiSCSILKFirstBurstLength = CFSTR("FirstBurstLength");
CFStringRef kiSCSILKDefaultTime2Wait = CFSTR("DefaultTime2Wait");
CFStringRef kiSCSILKDefaultTime2Retain = CFSTR("DefaultTime2Retain");
CFStringRef kiSCSILKMaxOutstandingR2T = CFSTR("MaxOutstandingR2T");

CFStringRef kiSCSILKDataPDUInOrder = CFSTR("DataPDUInOrder");

CFStringRef kiSCSILKDataSequenceInOrder = CFSTR("DataSequenceInOrder");

CFStringRef kiSCSILKErrorRecoveryLevel = CFSTR("ErrorRecoveryLevel");
CFStringRef kiSCSILVErrorRecoveryLevelSession = CFSTR("0");
CFStringRef kiSCSILVErrorRecoveryLevelDigest = CFSTR("1");
CFStringRef kiSCSILVErrorRecoveryLevelConnection = CFSTR("2");

CFStringRef kiSCSILKIFMarker = CFSTR("IFMarker");
CFStringRef kiSCSILKOFMarker = CFSTR("OFMarker");

/*! The following text commands  and corresponding possible values are used
 *  as key-value pairs during the full-feature phase of the connection. */
CFStringRef kiSCSITKSendTargets = CFSTR("SendTargets");
CFStringRef kiSCSITVSendTargetsAll = CFSTR("All");

CFStringRef kiSCSILVYes = CFSTR("Yes");
CFStringRef kiSCSILVNo = CFSTR("No");


errno_t iSCSILogoutResponseToErrno(enum iSCSIPDULogoutRsp response)
{
    return 0;
}

/*! Helper function used during session negotiation.  Returns true if BOTH
 *  the command and the response strings are "Yes" */
bool iSCSILVGetEqual(CFStringRef cmdStr,CFStringRef rspStr)
{
    if(CFStringCompare(cmdStr,rspStr,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;

    return false;
}

/*! Helper function used during session negotiation.  Returns true if BOTH
 *  the command and the response strings are "Yes" */
bool iSCSILVGetAnd(CFStringRef cmdStr,CFStringRef rspStr)
{
    CFComparisonResult cmd, rsp;
    cmd = CFStringCompare(cmdStr,kiSCSILVYes,kCFCompareCaseInsensitive);
    rsp = CFStringCompare(rspStr,kiSCSILVYes,kCFCompareCaseInsensitive);
    
    if(cmd == kCFCompareEqualTo && rsp == kCFCompareEqualTo)
        return true;
    
    return false;
}

/*! Helper function used during session negotiation.  Returns true if either
 *  one of the command or response strings are "Yes" */
bool iSCSILVGetOr(CFStringRef cmdStr,CFStringRef rspStr)
{
    if(CFStringCompare(cmdStr,kiSCSILVYes,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;
    
    if(CFStringCompare(rspStr,kiSCSILVYes,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;
    
    return false;
}

/*! Helper function used during session negotiation.  Converts values in 
 *  the command and response strings to numbers and returns the minimum. */
UInt32 iSCSILVGetMin(CFStringRef cmdStr, CFStringRef rspStr)
{
    // Convert string to unsigned integers
    UInt32 cmdInt = (UInt32)CFStringGetIntValue(cmdStr);
    UInt32 rspInt = (UInt32)CFStringGetIntValue(rspStr);
    
    // Compare & return minimum
    if(cmdInt < rspInt)
        return cmdInt;
    return rspInt;
}

/*! Helper function used during session negotiation.  Converts values in
 *  the command and response strings to numbers and returns the minimum. */
UInt32 iSCSILVGetMax(CFStringRef cmdStr, CFStringRef rspStr)
{
    // Convert string to unsigned integers
    UInt32 cmdInt = (UInt32)CFStringGetIntValue(cmdStr);
    UInt32 rspInt = (UInt32)CFStringGetIntValue(rspStr);
    
    // Compare & return minimum
    if(cmdInt > rspInt)
        return cmdInt;
    return rspInt;
}

/*! Helper function used during session negotiation.  Checks range of
 *  a particular value associated with a given key. */
bool iSCSILVRangeInvalid(UInt32 value,UInt32 min, UInt32 max)
{
    return (value < min || value > max);
}

/*! Helper function used throughout the login process to query the target.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate login PDU to the target.  It will then receive one or more
 *  login response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  parse the iSCSI error and express it in terms of a system errno_t as
 *  the system would treat any other device.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param sessionOptions options associated with the new session.
 *  @param currentStage the current stage of the login process.
 *  @param nextStage the next stage of the login process.
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionLoginQuery(UInt16 sessionId,
                               UInt32 connectionId,
                               iSCSISessionOptions * sessionOptions,
                               enum iSCSIPDULoginStages currentStage,
                               enum iSCSIPDULoginStages nextStage,
                               CFDictionaryRef   textCmd,
                               CFMutableDictionaryRef  textRsp)
{
    // Create a new login request basic header segment
    iSCSIPDULoginReqBHS cmd = iSCSIPDULoginReqBHSInit;
    cmd.TSIH  = CFSwapInt16HostToBig(sessionOptions->TSIH);
    cmd.CID   = CFSwapInt32HostToBig(connectionId);
    cmd.ISIDd = CFSwapInt16HostToBig(sessionId);
    cmd.loginStage  = (nextStage << kiSCSIPDULoginNSGBitOffset);
    cmd.loginStage |= (currentStage << kiSCSIPDULoginCSGBitOffset);

    // If stages aren't the same then we are going to transition
    if(currentStage != nextStage)
        cmd.loginStage |= kiSCSIPDULoginTransitFlag;
    
    // Create a data segment based on text commands (key-value pairs)
    void * data;
    size_t length;
    iSCSIPDUDataCreateFromDict(textCmd,&data,&length);
    
    errno_t error = iSCSIKernelSend(sessionId,connectionId,
                                    (iSCSIPDUInitiatorBHS *)&cmd,
                                    data,length);
    iSCSIPDUDataRelease(&data);
    
    if(error) {
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDULoginRspBHS rsp;
    
    do {
        if((error = iSCSIKernelRecv(sessionId,connectionId,
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

/*! Helper function used by iSCSINegotiateSession to build a dictionary
 *  of session options (key-value pairs) that will be sent to the target.
 *  @param sessionInfo a session information object.
 *  @return a dictionary of key-value pairs used to negotiate session
 *  parameters during the iSCSI operational login negotiation. */
void iSCSINegotiateBuildSWDictNormal(CFMutableDictionaryRef sessCmd)
{
    CFStringRef value;
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxConnections);
    CFDictionaryAddValue(sessCmd,kiSCSILKMaxConnections,value);
    
    CFDictionaryAddValue(sessCmd,kiSCSILKInitialR2T,kiSCSILVNo);
    CFDictionaryAddValue(sessCmd,kiSCSILKImmediateData,kiSCSILVYes);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxBurstLength);
    CFDictionaryAddValue(sessCmd,kiSCSILKMaxBurstLength,value);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_FirstBurstLength);
    CFDictionaryAddValue(sessCmd,kiSCSILKFirstBurstLength,value);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxOutstandingR2T);
    CFDictionaryAddValue(sessCmd,kiSCSILKMaxOutstandingR2T,value);
    
    CFDictionaryAddValue(sessCmd,kiSCSILKDataPDUInOrder,kiSCSILVYes);
    CFDictionaryAddValue(sessCmd,kiSCSILKDataSequenceInOrder,kiSCSILVYes);
}

/*! Helper function used by iSCSINegotiateSession to build a dictionary
 *  of session options (key-value pairs) that will be sent to the target.
 *  @param sessionInfo a session information object.
 *  @return a dictionary of key-value pairs used to negotiate session
 *  parameters during the iSCSI operational login negotiation. */
void iSCSINegotiateBuildSWDictCommon(CFMutableDictionaryRef sessCmd)
{
    CFStringRef value;
    
    // Add key-value pair for time to retain and time to wait
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_DefaultTime2Wait);
    CFDictionaryAddValue(sessCmd,kiSCSILKDefaultTime2Wait,value);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_DefaultTime2Retain);
    CFDictionaryAddValue(sessCmd,kiSCSILKDefaultTime2Retain,value);
    
    // Add key-value pair for error recovery level supported
    CFDictionaryAddValue(sessCmd,
                         kiSCSILKErrorRecoveryLevel,
                         kiSCSILVErrorRecoveryLevelDigest);
}

errno_t iSCSINegotiateParseSWDictCommon(CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp,
                                        iSCSISessionOptions * sessionOptions)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Grab minimum of default time 2 retain
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDefaultTime2Retain,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDefaultTime2Retain);
        UInt32 defaultTime2Retain = CFStringGetIntValue(targetRsp);

        // Range-check value...
        if(iSCSILVRangeInvalid(defaultTime2Retain,kRFC3720_DefaultTime2Retain_Min,kRFC3720_DefaultTime2Retain_Max))
            return ENOTSUP;
        
        sessionOptions->defaultTime2Retain = iSCSILVGetMin(initCmd,targetRsp);;
    }
    else
        return ENOTSUP;
    
    
    // Grab maximum of default time 2 wait
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDefaultTime2Wait,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDefaultTime2Wait);
        UInt32 defaultTime2Wait = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(defaultTime2Wait,kRFC3720_DefaultTime2Wait_Min,kRFC3720_DefaultTime2Wait_Max))
            return ENOTSUP;
        
        sessionOptions->defaultTime2Wait = iSCSILVGetMin(initCmd,targetRsp);;
    }
    else
        return ENOTSUP;


    // Grab minimum value of error recovery level
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKErrorRecoveryLevel,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKErrorRecoveryLevel);
        UInt32 errorRecoveryLevel = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(errorRecoveryLevel,kRFC3720_ErrorRecoveryLevel_Min,kRFC3720_ErrorRecoveryLevel_Max))
            return ENOTSUP;
        
        sessionOptions->errorRecoveryLevel = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    // Success
    return 0;
}

errno_t iSCSINegotiateParseSWDictNormal(CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp,
                                        iSCSISessionOptions * sessionOptions)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKMaxConnections,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKMaxConnections);
        UInt32 maxConnections = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxConnections,kRFC3720_MaxConnections_Min,kRFC3720_MaxConnections_Max))
            return ENOTSUP;
        
        sessionOptions->maxConnections = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    // Return value to caller by setting info (it is both an input & output)
//    sessionInfo->maxConnections = sessionOptions->maxConnections;
    
   
    // Grab the OR for initialR2T command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKInitialR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKInitialR2T);
        sessionOptions->initialR2T = iSCSILVGetOr(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    
    // Grab the AND for immediate data command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKImmediateData,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKImmediateData);
        sessionOptions->immediateData = iSCSILVGetAnd(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDataPDUInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDataPDUInOrder);
        sessionOptions->dataPDUInOrder = iSCSILVGetAnd(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDataSequenceInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDataSequenceInOrder);
        sessionOptions->dataSequenceInOrder = iSCSILVGetAnd(initCmd,targetRsp);
    }
    else
        return ENOTSUP;

    // Grab minimum of max burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKMaxBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKMaxBurstLength);
        sessionOptions->maxBurstLength = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        return ENOTSUP;

    // Grab minimum of first burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKFirstBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKFirstBurstLength);
        UInt32 firstBurstLength = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(firstBurstLength,kRFC3720_FirstBurstLength_Min,kRFC3720_FirstBurstLength_Max))
            return ENOTSUP;
        
        sessionOptions->firstBurstLength = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        return ENOTSUP;

    // Grab minimum of max outstanding R2T
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKMaxOutstandingR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKMaxOutstandingR2T);
        UInt32 maxOutStandingR2T = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxOutStandingR2T,kRFC3720_MaxOutstandingR2T_Min,kRFC3720_MaxOutstandingR2T_Max))
            return ENOTSUP;
        
        sessionOptions->maxOutStandingR2T = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    // Success
    return 0;
}

/*! Helper function used by iSCSISessionNegotiateCW to build a dictionary
 *  of connection options (key-value pairs) that will be sent to the target.
 *  @param connInfo a connection information object.
 *  @param connCmd  a dictionary that is populated with key-value pairs that
 *  will be used to negotiate connection parameters. */
void iSCSINegotiateBuildCWDict(iSCSITarget * target,
                               CFMutableDictionaryRef connCmd)
{
    // Setup digest options
    if(target->useDataDigest)
        CFDictionaryAddValue(connCmd,kiSCSILKDataDigest,kiSCSILVDataDigestCRC32C);
    else
        CFDictionaryAddValue(connCmd,kiSCSILKDataDigest,kiSCSILVDataDigestNone);
    
    if(target->useHeaderDigest)
        CFDictionaryAddValue(connCmd,kiSCSILKHeaderDigest,kiSCSILVHeaderDigestCRC32C);
    else
        CFDictionaryAddValue(connCmd,kiSCSILKHeaderDigest,kiSCSILVHeaderDigestNone);
    
    // Setup maximum received data length
    CFStringRef maxRecvLength = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxRecvDataSegmentLength);
    
    CFDictionaryAddValue(connCmd,kiSCSILKMaxRecvDataSegmentLength,maxRecvLength);
}

/*! Helper function used by iSCSISessionNegotiateCW to parse a dictionary
 *  of connection options received from the target.  This function stores
 *  those options with the kernel.
 *  @param sessionId the session qualifier.
 *  @param connCmd a dictionary of connection commands (key-value pairs)
 *  that were sent to the target.
 *  @param connRsp a dictionary of connection respones (key-value pairs)
 *  @param connInfo a connection information object.
 *  @param connectionOptions a connection options object used to store options with
 *  the iSCSI kernel extension
 *  there were received from the target. */
errno_t iSCSINegotiateParseCWDict(CFDictionaryRef connCmd,
                                  CFDictionaryRef connRsp,
                                  iSCSIConnectionOptions * connectionOptions)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Flag that indicates that target and initiator agree on a parameter
    bool agree = false;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kiSCSILKDataDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kiSCSILKDataDigest),targetRsp);
    
    // If we wanted to use data digest and target didn't set connInfo
    connectionOptions->useDataDigest = agree && iSCSILVGetEqual(targetRsp,kiSCSILVDataDigestCRC32C);
    
    // Reset agreement flag
    agree = false;
    
    // Get header digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kiSCSILKHeaderDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kiSCSILKHeaderDigest),targetRsp);
    
    // If we wanted to use header digest and target didn't set connInfo
    connectionOptions->useHeaderDigest = agree && iSCSILVGetEqual(targetRsp,kiSCSILVHeaderDigestCRC32C);
    
    // This option is declarative; we sent the default length, and the target
    // must accept our choice as it is within a valid range
    connectionOptions->maxRecvDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    // This is the declaration made by the target as to the length it can
    // receive.  Accept the value if it is within the RFC3720 allowed range
    // otherwise, terminate the connection...
    if(CFDictionaryGetValueIfPresent(connRsp,kiSCSILKMaxRecvDataSegmentLength,(void*)&targetRsp))
    {
        UInt32 maxSendDataSegmentLength = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxSendDataSegmentLength,kRFC3720_MaxRecvDataSegmentLength_Min,kRFC3720_MaxRecvDataSegmentLength_Max))
            return ENOTSUP;
        
        connectionOptions->maxSendDataSegmentLength = maxSendDataSegmentLength;
    }
    else // If the target doesn't explicitly declare this, use the default
        connectionOptions->maxSendDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    // Success
    return 0;
}


errno_t iSCSINegotiateSession(iSCSITarget * const target,
                              UInt16 sessionId,
                              UInt32 connectionId,
                              iSCSISessionOptions * sessionOptions,
                              iSCSIConnectionOptions * connectionOptions)
{
    // Create a new dictionary for connection parameters we want to send
    CFMutableDictionaryRef sessCmd = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Add session parameters common to all session types
    iSCSINegotiateBuildSWDictCommon(sessCmd);
    
    // If target name is specified, this is a normal session; add parameters
    if(target->targetName != NULL)
        iSCSINegotiateBuildSWDictNormal(sessCmd);
    
    // Add connection parameters
    iSCSINegotiateBuildCWDict(target,sessCmd);
    
    // Create a dictionary to store query response
    CFMutableDictionaryRef sessRsp = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Send session-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(sessionId,
                                           connectionId,
                                           sessionOptions,
                                           kiSCSIPDULoginOperationalNegotiation,
                                           kiSCSIPDUFullFeaturePhase,
                                           sessCmd,sessRsp);
    
    // Parse dictionaries and store session parameters if no I/O error occured
    if(!error)
        error = iSCSINegotiateParseSWDictCommon(sessCmd,sessRsp,sessionOptions);
    
    if(!error)
        if(target->targetName != NULL)
            error = iSCSINegotiateParseSWDictNormal(sessCmd,sessRsp,sessionOptions);
    
    if(!error)
        error = iSCSINegotiateParseCWDict(sessCmd,sessRsp,connectionOptions);
    
    CFRelease(sessCmd);
    CFRelease(sessRsp);
    return error;
}


/*! Helper function.  Negotiates operational parameters for a connection
 *  as part of the login and connection instantiation process. */
errno_t iSCSINegotiateConnection(iSCSITarget * const target,
                                 UInt16 sessionId,
                                 UInt32 connectionId,
                                 iSCSISessionOptions * sessionOptions,
                                 iSCSIConnectionOptions * connectionOptions)
{
    // Create a dictionary to store query request
    CFMutableDictionaryRef connCmd = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Populate dictionary with connection options based on connInfo
    iSCSINegotiateBuildCWDict(target,connCmd);

    // Create a dictionary to store query response
    CFMutableDictionaryRef connRsp = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // If the target session ID is non-zero, we're simply adding a new
    // connection and we can enter the full feature after this negotiation.
    enum iSCSIPDULoginStages nextStage = kiSCSIPDULoginOperationalNegotiation;
    
    if(sessionOptions->TSIH != 0)
        nextStage = kiSCSIPDUFullFeaturePhase;
    
    // Send connection-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(sessionId,
                                           connectionId,
                                           sessionOptions,
                                           kiSCSIPDULoginOperationalNegotiation,
                                           nextStage,
                                           connCmd,connRsp);
    

    // If no error, parse received dictionary and store connection options
    if(!error)
        error = iSCSINegotiateParseCWDict(connCmd,connRsp,connectionOptions);


    CFRelease(connCmd);
    CFRelease(connRsp);
    return error;
}

/*! Helper function used to log out of connections and sessions. */
errno_t iSCSISessionLogoutCommon(UInt16 sessionId,
                                 UInt32 connectionId,
                                 enum iSCSIPDULogoutReasons logoutReason)
{
    if(sessionId >= kiSCSIInvalidSessionId || connectionId >= kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Grab options related to this connection
    iSCSIConnectionOptions connOpts;
    errno_t error;
    
    if((error = iSCSIKernelGetConnectionOptions(sessionId,connectionId,&connOpts)))
        return error;
    
    // Create a logout PDU and log out of the session
    iSCSIPDULogoutReqBHS cmd = iSCSIPDULogoutReqBHSInit;
    cmd.reasonCode = logoutReason | kISCSIPDULogoutReasonCodeFlag;
    
    if((error = iSCSIKernelSend(sessionId,connectionId,(iSCSIPDUInitiatorBHS *)&cmd,NULL,0)))
        return error;
    
    // Get response from iSCSI portal
    iSCSIPDULogoutRspBHS rsp;
    void * data = NULL;
    size_t length = 0;
    
    // Receive response PDUs until response is complete
    if((error = iSCSIKernelRecv(sessionId,connectionId,(iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        return error;
    
    if(rsp.opCode == kiSCSIPDUOpCodeLogoutRsp)
        error = iSCSILogoutResponseToErrno(rsp.response);
    
    // For this case some other kind of PDU or invalid data was received
    else if(rsp.opCode == kiSCSIPDUOpCodeReject)
        error = EINVAL;
    
    iSCSIPDUDataRelease(data);
    return error;
}

/*! Helper function used to resolve target nodes as specified by connInfo.
 *  The target nodes specified in connInfo may be a DNS name, an IPv4 or
 *  IPv6 address. */
errno_t iSCSISessionResolveNode(iSCSIPortal * const portal,
                                int * ai_family,
                                struct sockaddr * sa_target,
                                struct sockaddr * sa_host)
{
    if (!portal || !ai_family || !sa_target || !sa_host)
        return EINVAL;
    
    errno_t error = 0;
    
    // Resolve the target node first and get a sockaddr info for it
    const char * targetAddr, * targetPort;

    targetAddr = CFStringGetCStringPtr(portal->address,kCFStringEncodingUTF8);
    targetPort = CFStringGetCStringPtr(portal->port,kCFStringEncodingUTF8);

    struct addrinfo * aiTarget = NULL;
    if((error = getaddrinfo(targetAddr,targetPort,NULL,&aiTarget)))
        return error;
    
    *ai_family = aiTarget->ai_family;
    *sa_target = *aiTarget->ai_addr;

    freeaddrinfo(aiTarget);
    
    
    // Grab a list of interfaces on this system, iterate over them and
    // find the requested interface.
    struct ifaddrs * interfaceList;
    
    if((error = getifaddrs(&interfaceList)))
        return error;
    
    error = EAFNOSUPPORT;
    struct ifaddrs * interface = interfaceList;
    while(interface)
    {
        CFStringRef interfaceName = CFStringCreateWithCString(kCFAllocatorDefault,
                                                              interface->ifa_name,
                                                              kCFStringEncodingUTF8);
        
        // Check if interface names match...
        if(CFStringCompare(interfaceName,portal->hostInterface,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        {
            // Check if the interface supports the target's
            // address family (e.g., IPv4 vs IPv6)
            if(interface->ifa_addr->sa_family == *ai_family)
            {
                *sa_host = *interface->ifa_addr;
                error = 0;
                break;
            }
        }
        interface = interface->ifa_next;
    }
        
    freeifaddrs(interfaceList);
    return error;
}


/*! Adds a new connection to an iSCSI session.
 *  @param portal specifies the portal to use for the connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIAddConnection(iSCSIPortal * const portal,
                           iSCSITarget * const target,
                           UInt16 sessionId,
                           UInt32 * connectionId)
{
    
    if(!portal || !target || sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;

    // Reset connection ID by default
    *connectionId = kiSCSIInvalidConnectionId;

    errno_t error = 0;
    
    // Resolve information about the target
    int ai_family;
    struct sockaddr sa_target, sa_host;
    
    if((error = iSCSISessionResolveNode(portal,&ai_family,&sa_target,&sa_host)))
       return error;
    
    // If both target and host were resolved, grab a session
    error = iSCSIKernelCreateConnection(sessionId,ai_family,&sa_target,&sa_host,connectionId);
    
    // Perform authentication and negotiate connection-level parameters
    iSCSIConnectionOptions connectionOptions;
    memset(&connectionOptions,0,sizeof(connectionOptions));
    
    iSCSISessionOptions sessionOptions;
    iSCSIKernelGetSessionOptions(sessionId,&sessionOptions);
    
    // If no error, authenticate (negotiate security parameters)
  /*  if(!error) {
        error = iSCSIAuthNegotiate(sessionId,
                                   connectionId,
  //                                 targetInfo,
                                   connInfo,
                                   &sessionOptions,
                                   &connectionOptions);
    }
    */
    if(error)
        iSCSIKernelReleaseConnection(sessionId,*connectionId);
    else {
        
        // At this point connection options have been modified/parsed by
        // the helper functions called above; set these options in the kernel
        iSCSIKernelSetConnectionOptions(sessionId,*connectionId,&connectionOptions);
        
        // Activate connection first then the session
        iSCSIKernelActivateConnection(sessionId,*connectionId);
    }
    return error;
}

errno_t iSCSIRemoveConnection(UInt16 sessionId,UInt32 connectionId)
{
    if(sessionId >= kiSCSIInvalidSessionId || connectionId >= kiSCSIInvalidConnectionId)
        return EINVAL;
    
    errno_t error = 0;
    
    // Release the session instead if there's only a single connection
    // for this session
    UInt32 numConnections = 0;
    if((error = iSCSIKernelGetNumConnections(sessionId,&numConnections)))
        return error;
    
    if(numConnections == 1)
        return iSCSIReleaseSession(sessionId);
    
    // Deactivate connection before we remove it (this is optional but good
    // practice, as the kernel will deactivate the connection for us).
    if((error = iSCSIKernelDeactivateConnection(sessionId,connectionId)))
       return error;
    
    // Logout the connection or session, as necessary
    error = iSCSISessionLogoutCommon(sessionId,connectionId,kISCSIPDULogoutCloseConnection);

    // Release the connection in the kernel
    iSCSIKernelReleaseConnection(sessionId,connectionId);
    
    return error;
}

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param portal specifies the portal to use for the new session.
 *  @param target specifies the target and connection parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSICreateSession(iSCSIPortal * const portal,
                           iSCSITarget * const target,
                           UInt16 * sessionId,
                           UInt32 * connectionId)
{
    if(!portal || !target || !sessionId || !connectionId)
        return EINVAL;
    
    // Store errno from helpers and pass back to up the call chain
    errno_t error = 0;
    
    // Resolve information about the target
    int ai_family;
    struct sockaddr sa_target, sa_host;
    
    if((error = iSCSISessionResolveNode(portal,&ai_family,&sa_target,&sa_host)))
        return error;
    
    // Create session (incl. qualifier) and a new connection (incl. Id)
    // Reset qualifier and connection ID by default
    error = iSCSIKernelCreateSession(ai_family,&sa_target,&sa_host,
                                     sessionId,connectionId);
    iSCSISessionOptions sessionOptions;
    iSCSIConnectionOptions connectionOptions;
    
    if(!error) {
        
        // If session couldn't be allocated were maxed out; try again later
        if(*sessionId == kiSCSIInvalidSessionId)
            return EAGAIN;
        
        memset(&sessionOptions,0,sizeof(sessionOptions));
        memset(&connectionOptions,0,sizeof(connectionOptions));
    }

    // If no error, authenticate (negotiate security parameters)
    if(!error)
        error = iSCSIAuthNegotiate(target,*sessionId,*connectionId,&sessionOptions);

    // Negotiate session & connection parameters
    if(!error) {
        error = iSCSINegotiateSession(target,*sessionId,*connectionId,
                                      &sessionOptions,&connectionOptions);
    }
    
    if(error) {
        iSCSIKernelReleaseSession(*sessionId);
    }
    else {
    
        // At this point session & connection options have been modified/parsed by
        // the helper functions called above; set these options in the kernel
        iSCSIKernelSetSessionOptions(*sessionId,&sessionOptions);
    
        iSCSIKernelSetConnectionOptions(*sessionId,*connectionId,&connectionOptions);
        
        // Activate connection inside kernel if it is not a discovery session
        if(target->targetName != NULL) {
            iSCSIKernelActivateConnection(*sessionId,*connectionId);
        }
    }
    
    return error;
}

/*! Closes the iSCSI session by deactivating and removing all connections. Any
 *  pending or current data transfers are aborted. This function may be called 
 *  on a session with one or more connections that are either inactive or 
 *  active. The session identifier is released and may be reused by other
 *  sessions the future.
 *  @param sessionId the session to release.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIReleaseSession(UInt16 sessionId)
{
    if(sessionId >= kiSCSIInvalidSessionId)
        return  EINVAL;
    
    errno_t error = 0;
    
    // First deactivate all of the connections
    if((error = iSCSIKernelDeactivateAllConnections(sessionId)))
        return error;
    
    // Grab a handle to any connection so we can logout of the session
    UInt32 connectionId = kiSCSIInvalidConnectionId;
    if(!(error = iSCSIKernelGetConnection(sessionId,&connectionId)))
        iSCSISessionLogoutCommon(sessionId, connectionId, kiSCSIPDULogoutCloseSession);

    // Release all of the connections in the kernel by releasing the session
    iSCSIKernelReleaseSession(sessionId);
    
    return error;
}

/*! Queries a portal for available targets.
 *  @param portal the iSCSI portal to query.
 *  @param targets an array of strings, where each string contains the name,
 *  alias, and portal associated with each target.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryPortalForTargets(iSCSIPortal * const portal,
                                   CFArrayRef * targets)
{
    if(!portal || !targets)
        return EINVAL;
    
    // Create a discovery session to the portal
    iSCSITarget target;
    target.targetName = NULL;
    target.authMethod = NULL;
    target.useDataDigest = false;
    target.useHeaderDigest = false;
    
    UInt16 sessionId;
    UInt32 connectionId;
    
    errno_t error;
    
    if((error = iSCSICreateSession(portal,&target,&sessionId,&connectionId)))
       return error;
    
    // Place text commands to get target list into a dictionary
    CFMutableDictionaryRef textCmd =
        CFDictionaryCreateMutable(kCFAllocatorDefault,
                                  kiSCSISessionMaxTextKeyValuePairs,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
    
    // Can't use a text query; must manually send/receive as the received
    // keys will be duplicates and CFDictionary doesn't support them
    CFDictionaryAddValue(textCmd,kiSCSITKSendTargets,kiSCSITVSendTargetsAll);
     
    // Create a data segment based on text commands (key-value pairs)
    void * data;
    size_t length;
    iSCSIPDUDataCreateFromDict(textCmd,&data,&length);
    
    iSCSIPDUTextReqBHS cmd = iSCSIPDUTextReqBHSInit;
    cmd.textReqStageFlags |= kiSCSIPDUTextReqFinalFlag;
    cmd.targetTransferTag = 0xFFFFFFFF;
     
    error = iSCSIKernelSend(sessionId,connectionId,(iSCSIPDUInitiatorBHS *)&cmd,data,length);
    
    iSCSIPDUDataRelease(&data);
     
    if(error)
        return error;
     
    // Create response arrays
    CFMutableArrayRef targetsList = (CFMutableArrayRef)targets;
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDUTextRspBHS rsp;
     
    do {
        if((error = iSCSIKernelRecv(sessionId,connectionId,(iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);
            return error;
        }
     
        if(rsp.opCode == kiSCSIPDUOpCodeTextRsp)
        {
     
   //         iSCSIPDUDataParseToArrays(data,length,keys,values);
        }
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

/*! Retrieves a list of targets available from a give portal.
 *  @param portal the iSCSI portal to look for targets.
 *  @param authMethods a comma-separated list of authentication methods
 *  as defined in the RFC3720 standard (version 1).
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryTargetForAuthMethods(iSCSIPortal * const portal,
                                       CFStringRef targetName,
                                       CFStringRef * authMethods)
{
    if(!portal || !authMethods)
        return EINVAL;
    
    // Create a discovery session to the portal
    iSCSITarget target;
    target.targetName = targetName;
    target.authMethod = NULL;
    target.useDataDigest = false;
    target.useHeaderDigest = false;
    
    iSCSISessionOptions sessionOptions;
    
    // Store errno from helpers and pass back to up the call chain
    errno_t error = 0;
    
    // Resolve information about the target
    int ai_family;
    struct sockaddr sa_target, sa_host;
    
    if((error = iSCSISessionResolveNode(portal,&ai_family,&sa_target,&sa_host)))
        return error;
    
    // Create session (incl. qualifier) and a new connection (incl. Id)
    // Reset qualifier and connection ID by default
    UInt16 sessionId;
    UInt32 connectionId;
    error = iSCSIKernelCreateSession(ai_family,&sa_target,&sa_host,&sessionId,&connectionId);
    
    if(!error)
        error = iSCSIKernelGetSessionOptions(sessionId,&sessionOptions);
    
    // If no error, authenticate (negotiate security parameters)
    if(!error)
        error = iSCSIAuthInterrogate(&target,sessionId,connectionId,&sessionOptions,authMethods);

    iSCSIKernelReleaseSession(sessionId);
    
    return 0;
}

/*! Retreives the initiator session identifier associated with this target.
 *  @param targetName the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIGetSessionIdForTaget(CFStringRef targetName,
                                  UInt16 * sessionId)
{
    if(!targetName || sessionId)
        return EINVAL;
    
    // Retrieve session dictionary from the iSCSI initiator
    
    
    
    // Lookup target in the dictionary
    
    
    return 0;
}


/*! Sets the name of this initiator.  This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param initiatorName the initiator name.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISetInitiatiorName(CFStringRef initiatorName)
{
    if(!initiatorName)
        return EINVAL;
    
    CFRelease(kiSCSIInitiatorName);
    kiSCSIInitiatorName = CFStringCreateCopy(kCFAllocatorDefault,initiatorName);
    
    return 0;
}

/*! Sets the alias of this initiator.  This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param initiatorAlias the initiator alias.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISetInitiatorAlias(CFStringRef initiatorAlias)
{
    if(!initiatorAlias)
        return EINVAL;
    
    CFRelease(kiSCSIInitiatorAlias);
    kiSCSIInitiatorAlias = CFStringCreateCopy(kCFAllocatorDefault,initiatorAlias);
    
    return 0;
}
