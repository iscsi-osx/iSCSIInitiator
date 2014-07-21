/**
 * @author		Nareg Sinenian
 * @file		iSCSISession.h
 * @date		November 16, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI session management functions.  This library
 *              depends on the user-space iSCSI PDU library to login, logout
 *              and perform discovery functions on iSCSI target nodes.
 */

#include "iSCSISession.h"
#include "iSCSIPDUUser.h"
#include "iSCSIKernelInterface.h"

/** The iSCSI error code that is set to a particular value if session management
 *  functions cannot be completed successfully.  This is distinct from errors
 *  associated with system function calls.  If no system error has occured
 *  (e.g., socket API calls) an iSCSI error may still have occured. */
static UInt16 iSCSILoginResponseCode = 0;

/** Authentication function defined in the authentication modules
 *  (in the file iSCSIAuth.h). */
extern errno_t iSCSIAuthNegotiate(UInt16 sessionId,
                                  iSCSIConnectionInfo * connInfo,
                                  iSCSISessionOptions * sessOptions,
                                  iSCSIConnectionOptions * connOptions);

const unsigned int kiSCSISessionMaxTextKeyValuePairs = 100;
const unsigned int kiSCSISessionTimeoutMs = 500;
const unsigned int kMaxRecvDataSegmentLength = 16384;
//const UInt32 kLoginQueryTaskTag = 0;


/** iSCSI default data sequ
static const bool kiSCSIDataSequenceInOrder =
static const bool kRFC3720DataPDUInOrder =
*/

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

/** The following text commands  and corresponding possible values are used
 *  as key-value pairs during the full-feature phase of the connection. */
CFStringRef kiSCSITKSendTargets = CFSTR("SendTargets");
CFStringRef kiSCSITVSendTargetsAll = CFSTR("All");

CFStringRef kiSCSILVYes = CFSTR("Yes");
CFStringRef kiSCSILVNo = CFSTR("No");

errno_t iSCSILogoutResponseToErrno(enum iSCSIPDULogoutRsp response)
{
    return 0;
}

/** Helper function used during session negotiation.  Returns true if BOTH
 *  the command and the response strings are "Yes" */
bool iSCSILVGetEqual(CFStringRef cmdStr,CFStringRef rspStr)
{
    if(CFStringCompare(cmdStr,rspStr,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;

    return false;
}

/** Helper function used during session negotiation.  Returns true if BOTH
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

/** Helper function used during session negotiation.  Returns true if either
 *  one of the command or response strings are "Yes" */
bool iSCSILVGetOr(CFStringRef cmdStr,CFStringRef rspStr)
{
    if(CFStringCompare(cmdStr,kiSCSILVYes,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;
    
    if(CFStringCompare(rspStr,kiSCSILVYes,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;
    
    return false;
}

/** Helper function used during session negotiation.  Converts values in 
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

/** Helper function used during session negotiation.  Converts values in
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

/** Helper function used throughout the login process to query the target.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate login PDU to the target.  It will then receive one or more
 *  login response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  parse the iSCSI error and express it in terms of a system errno_t as
 *  the system would treat any other device.
 *  @param sessionId the session qualifier.
 *  @param targetSessionId the target session identifying handle.
 *  @param connInfo a connection information object.
 *  @param currentStage the current stage of the login process.
 *  @param nextStage the next stage of the login process.
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionLoginQuery(UInt16 sessionId,
                               UInt16 * targetSessionId,
                               iSCSIConnectionInfo * connInfo,
                               enum iSCSIPDULoginStages currentStage,
                               enum iSCSIPDULoginStages nextStage,
                               CFDictionaryRef   textCmd,
                               CFMutableDictionaryRef  textRsp)
{
    // Create a new login request basic header segment
    iSCSIPDULoginReqBHS cmd = iSCSIPDULoginReqBHSInit;
    cmd.TSIH  = CFSwapInt16HostToBig(*targetSessionId);
    cmd.CID   = CFSwapInt32HostToBig(connInfo->connectionId);
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
    
    errno_t error = iSCSIKernelSend(sessionId,
                                    connInfo->connectionId,
                                    (iSCSIPDUInitiatorBHS *)&cmd,
                                    data,length);
    iSCSIPDUDataRelease(&data);
    
    if(error) {
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDULoginRspBHS rsp;
    
    do {
        if((error = iSCSIKernelRecv(sessionId,connInfo->connectionId,
                                    (iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);
            return error;
        }

        if(rsp.opCode == kiSCSIPDUOpCodeLoginRsp)
        {
            connInfo->status = *((UInt16*)(&rsp.statusClass));

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

/** Helper function used by iSCSISessionNegotiateSW to build a dictionary
 *  of session options (key-value pairs) that will be sent to the target.
 *  @param sessionInfo a session information object.
 *  @return a dictionary of key-value pairs used to negotiate session
 *  parameters during the iSCSI operational login negotiation. */
void iSCSISessionNegotiateSWBuildDictNormal(iSCSISessionInfo * sessionInfo,
                                            CFMutableDictionaryRef sessCmd)
{
    CFDictionaryAddValue(sessCmd,kiSCSILKMaxConnections,CFSTR("1"));
    CFDictionaryAddValue(sessCmd,kiSCSILKInitialR2T,kiSCSILVNo);
    CFDictionaryAddValue(sessCmd,kiSCSILKImmediateData,kiSCSILVYes);
    CFDictionaryAddValue(sessCmd,kiSCSILKMaxBurstLength,CFSTR("262144"));
    CFDictionaryAddValue(sessCmd,kiSCSILKFirstBurstLength,CFSTR("65535"));
    CFDictionaryAddValue(sessCmd,kiSCSILKMaxOutstandingR2T,CFSTR("1"));
    CFDictionaryAddValue(sessCmd,kiSCSILKDataPDUInOrder,kiSCSILVYes);
    CFDictionaryAddValue(sessCmd,kiSCSILKDataSequenceInOrder,kiSCSILVYes);
}

/** Helper function used by iSCSISessionNegotiateSW to build a dictionary
 *  of session options (key-value pairs) that will be sent to the target.
 *  @param sessionInfo a session information object.
 *  @return a dictionary of key-value pairs used to negotiate session
 *  parameters during the iSCSI operational login negotiation. */
void iSCSISessionNegotiateSWBuildDictCommon(iSCSISessionInfo * sessionInfo,
                                            CFMutableDictionaryRef sessCmd)
{
    // Add key-value pair for time to retain and time to wait
    CFDictionaryAddValue(sessCmd,kiSCSILKDefaultTime2Wait,CFSTR("0"));
    CFDictionaryAddValue(sessCmd,kiSCSILKDefaultTime2Retain,CFSTR("0"));
    
    // Add key-value pair for error recovery level supported
    CFDictionaryAddValue(sessCmd,
                         kiSCSILKErrorRecoveryLevel,
                         kiSCSILVErrorRecoveryLevelDigest);
}

void iSCSISessionNegotiateSWParseDictCommon(iSCSISessionInfo * sessionInfo,
                                            CFDictionaryRef sessCmd,
                                            CFDictionaryRef sessRsp,
                                            iSCSISessionOptions * sessOptions)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Grab minimum of default time 2 retain
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDefaultTime2Retain,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDefaultTime2Retain);
        sessOptions->defaultTime2Retain = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        sessOptions->defaultTime2Retain = 1; // Default time to retain

    
    // Grab maximum of default time 2 wait
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDefaultTime2Wait,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDefaultTime2Wait);
        sessOptions->defaultTime2Wait = iSCSILVGetMax(initCmd,targetRsp);
    }
    else
        sessOptions->defaultTime2Wait = 1; // Default time to retain


    // Grab minimum value of error recovery level
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKErrorRecoveryLevel,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKErrorRecoveryLevel);
        sessOptions->errorRecoveryLevel = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        sessOptions->errorRecoveryLevel = 1; // Default time to retain
}

void iSCSISessionNegotiateSWParseDictNormal(iSCSISessionInfo * sessionInfo,
                                            CFDictionaryRef sessCmd,
                                            CFDictionaryRef sessRsp,
                                            iSCSISessionOptions * sessOptions)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKMaxConnections,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKMaxConnections);
        sessOptions->maxConnections = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        sessOptions->maxConnections = 1; // Default max
    
    // Return value to caller by setting info (it is both an input & output)
    sessionInfo->maxConnections = sessOptions->maxConnections;
    
   
    // Grab the OR for initialR2T command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKInitialR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKInitialR2T);
        sessOptions->initialR2T = iSCSILVGetOr(initCmd,targetRsp);
    }
    else
        sessOptions->initialR2T = true; // Default initial R2T
    
    
    // Grab the AND for immediate data command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKImmediateData,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKImmediateData);
        sessOptions->immediateData = iSCSILVGetAnd(initCmd,targetRsp);
    }
    else
        sessOptions->immediateData = true; // Default immediate data
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDataPDUInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDataPDUInOrder);
        sessOptions->dataPDUInOrder = iSCSILVGetAnd(initCmd,targetRsp);
    }
    else
        sessOptions->dataPDUInOrder = true; // Default data PDU in order
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKDataSequenceInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKDataSequenceInOrder);
        sessOptions->dataSequenceInOrder = iSCSILVGetAnd(initCmd,targetRsp);
    }
    else
        sessOptions->dataSequenceInOrder = true; // Default data sequence

    // Grab minimum of max burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKMaxBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKMaxBurstLength);
        sessOptions->maxBurstLength = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        sessOptions->maxBurstLength = 1; // Default max

    // Grab minimum of first burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKFirstBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKFirstBurstLength);
        sessOptions->firstBurstLength = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        sessOptions->firstBurstLength = 1; // Default max

    // Grab minimum of max outstanding R2T
    if(CFDictionaryGetValueIfPresent(sessRsp,kiSCSILKMaxOutstandingR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kiSCSILKMaxOutstandingR2T);
        sessOptions->maxOutStandingR2T = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        sessOptions->maxOutStandingR2T = 1; // Default max
}

errno_t iSCSISessionNegotiateSW(iSCSISessionInfo * sessionInfo,
                                iSCSIConnectionInfo * connInfo,
                                iSCSISessionOptions * sessOptions)
{
    // Create a new dictionary for connection parameters we want to send
    CFMutableDictionaryRef sessCmd = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Add session parameters common to all session types
    iSCSISessionNegotiateSWBuildDictCommon(sessionInfo,sessCmd);
    
    // If target name is specified, this is a normal session; add parameters
    if(connInfo->targetName != NULL)
        iSCSISessionNegotiateSWBuildDictNormal(sessionInfo,sessCmd);
    
    // Create a dictionary to store query response
    CFMutableDictionaryRef sessRsp = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Send session-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(sessionInfo->sessionId,
                                              &sessOptions->targetSessionId,
                                              connInfo,
                                              kiSCSIPDULoginOperationalNegotiation,
                                              kiSCSIPDUFullFeaturePhase,
                                              sessCmd,sessRsp);
    
    // Parse dictionaries and store session parameters if no I/O error occured
    if(!error) {
        // Parse received dictionary for parameters common to all session types
        iSCSISessionNegotiateSWParseDictCommon(sessionInfo,sessCmd,sessRsp,sessOptions);
    
        if(connInfo->targetName != NULL)
            iSCSISessionNegotiateSWParseDictNormal(sessionInfo,sessCmd,sessRsp,sessOptions);
    }
    
    CFRelease(sessCmd);
    CFRelease(sessRsp);
    return error;
}

/** Helper function used by iSCSISessionNegotiateCW to build a dictionary
 *  of connection options (key-value pairs) that will be sent to the target.
 *  @param connInfo a connection information object.
 *  @param connCmd  a dictionary that is populated with key-value pairs that
 *  will be used to negotiate connection parameters. */
void iSCSISessionNegotiateCWBuildDict(iSCSIConnectionInfo * connInfo,
                                      CFMutableDictionaryRef connCmd)
{
    // Setup digest options
    if(connInfo->useDataDigest)
        CFDictionaryAddValue(connCmd,kiSCSILKDataDigest,kiSCSILVDataDigestCRC32C);
    else
        CFDictionaryAddValue(connCmd,kiSCSILKDataDigest,kiSCSILVDataDigestNone);
    
    if(connInfo->useHeaderDigest)
        CFDictionaryAddValue(connCmd,kiSCSILKHeaderDigest,kiSCSILVHeaderDigestCRC32C);
    else
        CFDictionaryAddValue(connCmd,kiSCSILKHeaderDigest,kiSCSILVHeaderDigestNone);
    
    // Setup maximum received data length
    CFStringRef maxRecvLength = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%u"),kMaxRecvDataSegmentLength);

    CFDictionaryAddValue(connCmd,kiSCSILKMaxRecvDataSegmentLength,maxRecvLength);
    CFRelease(maxRecvLength);
}

/** Helper function used by iSCSISessionNegotiateCW to parse a dictionary
 *  of connection options received from the target.  This function stores
 *  those options with the kernel.
 *  @param sessionId the session qualifier.
 *  @param connCmd a dictionary of connection commands (key-value pairs) 
 *  that were sent to the target.
 *  @param connRsp a dictionary of connection respones (key-value pairs)
 *  @param connInfo a connection information object.
 *  @param connOptions a connection options object used to store options with
 *  the iSCSI kernel extension
 *  there were received from the target. */
void iSCSISessionNegotiateCWParseDict(UInt16 sessionId,
                                      CFDictionaryRef connCmd,
                                      CFDictionaryRef connRsp,
                                      iSCSIConnectionInfo * connInfo,
                                      iSCSIConnectionOptions * connOptions)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Flag that indicates that target and initiator agree on a parameter
    bool agree = false;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kiSCSILKDataDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kiSCSILKDataDigest),targetRsp);
    
    // If we wanted to use data digest and target didn't set connInfo
    if(!agree && connInfo->useDataDigest)
        connInfo->useDataDigest = false;
    
    // Reset agreement flag
    agree = false;
    
    // Get header digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kiSCSILKHeaderDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kiSCSILKHeaderDigest),targetRsp);
    
    // If we wanted to use header digest and target didn't set connInfo
    if(!agree && connInfo->useHeaderDigest)
        connInfo->useHeaderDigest = false;
    
    // Set connection options and send to kernel
    connOptions->useDataDigest = connInfo->useDataDigest;
    connOptions->useHeaderDigest = connInfo->useHeaderDigest;
    connOptions->maxRecvDataSegmentLength = kMaxRecvDataSegmentLength;
    
    if(CFDictionaryGetValueIfPresent(connRsp,kiSCSILKMaxRecvDataSegmentLength,(void*)&targetRsp))
        connOptions->maxSendDataSegmentLength = 8192;/////////TOODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    else
        connOptions->maxSendDataSegmentLength = 8192;

}

/** Helper function.  Negotiates operational parameters for a connection
 *  as part of the login and connection instantiation process. */
errno_t iSCSISessionNegotiateCW(UInt16 sessionId,
                                UInt16 targetSessionId,
                                iSCSIConnectionInfo * connInfo,
                                iSCSIConnectionOptions * connOptions)
{
    // Create a dictionary to store query request
    CFMutableDictionaryRef connCmd = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Populate dictionary with connection options based on connInfo
    iSCSISessionNegotiateCWBuildDict(connInfo,connCmd);

    // Create a dictionary to store query response
    CFMutableDictionaryRef connRsp = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // If the target session ID is non-zero, we're simply adding a new
    // connection and we can enter the full feature after this negotiation.
    enum iSCSIPDULoginStages nextStage = kiSCSIPDULoginOperationalNegotiation;
    
    if(targetSessionId != 0)
        nextStage = kiSCSIPDUFullFeaturePhase;
    
    // Send connection-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(sessionId,
                                              &targetSessionId,connInfo,
                                              kiSCSIPDULoginOperationalNegotiation,
                                              nextStage,
                                              connCmd,connRsp);

    // If no error, parse received dictionary and store connection options
    if(!error) {
        // Parse received dictionary, store options with kernel
        iSCSISessionNegotiateCWParseDict(sessionId,connCmd,
                                         connRsp,connInfo,connOptions);
    }
    CFRelease(connCmd);
    CFRelease(connRsp);
    return error;
}

/** Helper function used to log out of connections and sessions. */
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

/** Helper function used to resolve target nodes as specified by connInfo.
 *  The target nodes specified in connInfo may be a DNS name, an IPv4 or
 *  IPv6 address. */
errno_t iSCSISessionResolveNode(iSCSIConnectionInfo * connInfo,
                                int * ai_family,
                                struct sockaddr * sa_target,
                                struct sockaddr * sa_host)
{
    // Resolve IP address (IPv4 or IPv6) or hostname, first get C pointers
    // to the CF string objects
    const char * hostAddr, * targetAddr, * targetPort;

    hostAddr = CFStringGetCStringPtr(connInfo->hostAddress,kCFStringEncodingUTF8);
    targetAddr = CFStringGetCStringPtr(connInfo->targetAddress,kCFStringEncodingUTF8);
    targetPort = CFStringGetCStringPtr(connInfo->targetPort,kCFStringEncodingUTF8);

    struct addrinfo * aiTarget = NULL;
    struct addrinfo * aiHost = NULL;

    errno_t error = 0;

    if((error = getaddrinfo(hostAddr,NULL,NULL,&aiHost)))
        return error;

    if((error = getaddrinfo(targetAddr,targetPort,NULL,&aiTarget))) {
        freeaddrinfo(aiHost);
        return error;
    }
    
    // Return values
    *ai_family = aiTarget->ai_family;
    *sa_target = *aiTarget->ai_addr;
    *sa_host   = *aiHost->ai_addr;

    // Cleanup
    freeaddrinfo(aiTarget);
    freeaddrinfo(aiHost);
        
    return 0;
}

errno_t iSCSISessionAddConnection(UInt16 sessionId,
                                  iSCSIConnectionInfo * connInfo)
{
    if(sessionId == kiSCSIInvalidSessionId || !connInfo)
        return EINVAL;

    // Reset connection ID by default
    connInfo->connectionId = kiSCSIInvalidConnectionId;

    errno_t error = 0;
    
    // Resolve information about the target
    int ai_family;
    struct sockaddr sa_target, sa_host;
    
    if((error = iSCSISessionResolveNode(connInfo,&ai_family,&sa_target,&sa_host)))
       return error;
    
    // If both target and host were resolved, grab a session
    error = iSCSIKernelCreateConnection(sessionId,
                                        ai_family,
                                        &sa_target,
                                        &sa_host,
                                        &connInfo->connectionId);

    return error;
}

errno_t iSCSISessionRemoveConnection(UInt16 sessionId,
                                     UInt16 connectionId)
{
    
    if(sessionId >= kiSCSIInvalidSessionId || connectionId >= kiSCSIInvalidConnectionId)
        return EINVAL;
    
    errno_t error = 0;
    
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

/** Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionRelease to close this session and free resources.
 *  @param sessionInfo parameters associated with the normal session. A new
 *  sessionId is assigned and returned.
 *  @param connInfo parameters associated with the connection. A new
 *  connectionId is assigned and returned.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISessionCreate(iSCSISessionInfo * sessionInfo,
                           iSCSIConnectionInfo * connInfo)

{
    if(!sessionInfo || !connInfo)
        return EINVAL;
    
    // Store errno from helpers and pass back to up the call chain
    errno_t error = 0;
    
    // Resolve information about the target
    int ai_family;
    struct sockaddr sa_target, sa_host;
    
    if((error = iSCSISessionResolveNode(connInfo,&ai_family,&sa_target,&sa_host)))
        return error;
    
    // Create session (incl. qualifier) and a new connection (incl. Id)
    // Reset qualifier and connection ID by default
    error = iSCSIKernelCreateSession(ai_family,&sa_target,&sa_host,
                                     &sessionInfo->sessionId,
                                     &connInfo->connectionId);

    iSCSISessionOptions sessOptions;
    iSCSIConnectionOptions connOptions;
    
    if(!error) {
        
        // If session couldn't be allocated were maxed out; try again later
        if(sessionInfo->sessionId == kiSCSIInvalidSessionId)
            return EAGAIN;

        // Grab a session options object and pass it around; at the end, we'll
        // set the session options in the kernel
        iSCSIKernelGetSessionOptions(sessionInfo->sessionId,&sessOptions);
        
        // Grab a connection object and pass it around; at the end, we'll set
        // the connection options in the kernel
        iSCSIKernelGetConnectionOptions(sessionInfo->sessionId,
                                        connInfo->connectionId,&connOptions);
        
        memset(&sessOptions,0,sizeof(sessOptions));
        memset(&connOptions,0,sizeof(connOptions));
    }

    // If no error, authenticate (negotiate security parameters)
    if(!error) {
        error = iSCSIAuthNegotiate(sessionInfo->sessionId,
                                   connInfo,
                                   &sessOptions,&connOptions);
    }

    // If no error, negotiate connection-wide (CW) parameters
    if(!error) {
        error = iSCSISessionNegotiateCW(sessionInfo->sessionId,
                                        sessOptions.targetSessionId,
                                        connInfo,&connOptions);
    }

    // If no error, negotiate session-wide (SW) parameters
    if(!error)
        error = iSCSISessionNegotiateSW(sessionInfo,connInfo,&sessOptions);
    
    if(error) {
        iSCSIKernelReleaseSession(sessionInfo->sessionId);
    }
    else {
    
        // At this point session & connection options have been modified/parsed by
        // the helper functions called above; set these options in the kernel
        iSCSIKernelSetSessionOptions(sessionInfo->sessionId,&sessOptions);
    
        iSCSIKernelSetConnectionOptions(sessionInfo->sessionId,
                                        connInfo->connectionId,&connOptions);
        
        // Activate connection first then the session
        iSCSIKernelActivateConnection(sessionInfo->sessionId,connInfo->connectionId);
    }
    
    return error;
}


/** Closes the iSCSI connection and frees the session qualifier.
 *  @param sessionId the session to free. */
errno_t iSCSISessionRelease(UInt16 sessionId)
{
    if(sessionId >= kiSCSIInvalidSessionId)
        return  EINVAL;
    
    errno_t error = 0;
    
    // TODO: Deactivate session in kernel
    
    
    
    // Grab a connection ID for this session so that we can logout session
    UInt32 connectionId = iSCSIKernelGetActiveConnection(sessionId);
    //    if(connectionId == kiSCSIInvalidConnectionId)
    //        return EINVAL;
    connectionId = 0;
    // Logout the session, including all connections
    error = iSCSISessionLogoutCommon(sessionId,connectionId,kiSCSIPDULogoutCloseSession);
    
    if(!error)
        iSCSIKernelReleaseSession(sessionId);
    
    return error;
}

/** Gets a list of targets associated with a particular session.
 *  @param sessionId the session (discovery or normal) to use.
 *  @param connectionId the connection ID to use.
 *  @param targetList a list of targets retreived from teh iSCSI node.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISessionGetTargetList(UInt16 sessionId,
                                  UInt32 connectionId,
                                  CFMutableDictionaryRef targetList)
{
    // Place text commands to get target list into a dictionary
    CFMutableDictionaryRef textCmd =
        CFDictionaryCreateMutable(kCFAllocatorDefault,
                                  kiSCSISessionMaxTextKeyValuePairs,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
    
   /*
    CFDictionaryAddValue(textCmd,kiSCSITKSendTargets,kiSCSITVSendTargetsAll);
    
    // Retreive connection options for this connection
    iSCSIConnectionOptions options;
    iSCSIKernelGetConnectionOptions(sessionId,connectionId,&options);
    
    // Create a text reqeust PDU
    iSCSIPDUInitiatorRef textPDU =
        (iSCSIPDUInitiatorRef)iSCSIPDUCreateTextRequest(textCmd,&options,0,0xFFFFFFFF);
    
    errno_t error;
    if((error = iSCSIKernelSend(sessionId,connectionId,textPDU)))
    {
        iSCSIPDURelease((iSCSIPDURef)textPDU);
        return error;
    }
    
    // Grab response and parse it for target names and addresses
    iSCSIPDUTargetRef rspPDU;
    bool finalResponse = false;
    
    while(!finalResponse)
    {
        // Get response from iSCSI portal
        if((error = iSCSIKernelRecv(sessionId,connectionId,&rspPDU)))
            return error;
        
        // Parse the PDU
        enum iSCSIPDUTargetOpCodes opCode = iSCSIPDUGetTargetOpCode(rspPDU);
        
        if(opCode == kiSCSIPDUOpCodeTextRsp)
        {
            // The target has responded to our request, parse response
            iSCSIPDUParseTextResponse((iSCSIPDUTextRspRef)rspPDU,&finalResponse,targetList);
            
        }
        else if(opCode == kiSCSIPDUOpCodeReject)
        {
            // Command failed for some reason
            
            
        }
    }
    

*/
    return 0;
}
    

    
///// TODO:
// 1. make key=value function add to an existing dictionary as necessary
// (assume keys dont' break mid-PDU)

        // 2. Finalize structs that serve as arguments to session functions, fix
        // contents and ensure that values to be returned to daemon client are included

// 3. store settings once session negotiation is complete - do this for session
// and for connection.
// 4. add session and connection activation functions (and / or?) decided how
// this will work and add them to kernel, interface, and call them from user
// 5. hide interface layer from damone entirely



