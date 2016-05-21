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

#include "iSCSISession.h"
#include "iSCSIPDUUser.h"
#include "iSCSIKernelInterface.h"
#include "iSCSIKernelInterfaceShared.h"
#include "iSCSIAuth.h"
#include "iSCSIQueryTarget.h"
#include "iSCSITypes.h"
#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"
#include "iSCSIRFC3720Defaults.h"

/*! Name of the initiator. */
CFStringRef kiSCSIInitiatorIQN = CFSTR("iqn.2015-01.com.localhost");

/*! Alias of the initiator. */
CFStringRef kiSCSIInitiatorAlias = CFSTR("default");

/*! Maximum number of key-value pairs supported by a dictionary that is used
 *  to produce the data section of text and login PDUs. */
const unsigned int kiSCSISessionMaxTextKeyValuePairs = 100;

/*! Helper function used during session negotiation.  Returns true if BOTH
 *  the command and the response strings are "Yes" */
Boolean iSCSILVGetEqual(CFStringRef cmdStr,CFStringRef rspStr)
{
    if(CFStringCompare(cmdStr,rspStr,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;

    return false;
}

/*! Helper function used during session negotiation.  Returns true if BOTH
 *  the command and the response strings are "Yes" */
Boolean iSCSILVGetAnd(CFStringRef cmdStr,CFStringRef rspStr)
{
    CFComparisonResult cmd, rsp;
    cmd = CFStringCompare(cmdStr,kRFC3720_Value_Yes,kCFCompareCaseInsensitive);
    rsp = CFStringCompare(rspStr,kRFC3720_Value_Yes,kCFCompareCaseInsensitive);
    
    if(cmd == kCFCompareEqualTo && rsp == kCFCompareEqualTo)
        return true;
    
    return false;
}

/*! Helper function used during session negotiation.  Returns true if either
 *  one of the command or response strings are "Yes" */
Boolean iSCSILVGetOr(CFStringRef cmdStr,CFStringRef rspStr)
{
    if(CFStringCompare(cmdStr,kRFC3720_Value_Yes,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return true;
    
    if(CFStringCompare(rspStr,kRFC3720_Value_Yes,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
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
Boolean iSCSILVRangeInvalid(UInt32 value,UInt32 min, UInt32 max)
{
    return (value < min || value > max);
}

/*! Helper function used by iSCSINegotiateSession to build a dictionary
 *  of session options (key-value pairs) that will be sent to the target.
 *  @param sessCfg a session configuration object.
 *  @param sessCmd a dictionary of key-value pairs used to negotiate session
 *  parameters during the iSCSI operational login negotiation. */
void iSCSINegotiateBuildSWDictNormal(iSCSISessionConfigRef sessCfg,
                                     CFMutableDictionaryRef sessCmd)
{
    CFStringRef value;
    
    // If the maximum number of connections was specified in the target,
    // use it.  Else default to RFC3720 value
    UInt32 maxConnections = iSCSISessionConfigGetMaxConnections(sessCfg);
    
    if(maxConnections == 0)
        value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxConnections);
    else
        value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),maxConnections);
    
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_MaxConnections,value);
    CFRelease(value);
    
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_InitialR2T,kRFC3720_Value_Yes);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_ImmediateData,kRFC3720_Value_Yes);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxBurstLength);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_MaxBurstLength,value);
    CFRelease(value);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_FirstBurstLength);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_FirstBurstLength,value);
    CFRelease(value);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxOutstandingR2T);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_MaxOutstandingR2T,value);
    CFRelease(value);
    
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_DataPDUInOrder,kRFC3720_Value_Yes);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_DataSequenceInOrder,kRFC3720_Value_Yes);
}

/*! Helper function used by iSCSINegotiateSession to build a dictionary
 *  of session options (key-value pairs) that will be sent to the target.
 *  @param sessCfg a session configuration object.
 *  @param sessCmd a dictionary of key-value pairs used to negotiate session
 *  parameters during the iSCSI operational login negotiation. */
void iSCSINegotiateBuildSWDictCommon(iSCSISessionConfigRef sessCfg,
                                     CFMutableDictionaryRef sessCmd)
{
    CFStringRef value;
    
    // Add key-value pair for time to retain and time to wait
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_DefaultTime2Wait);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_DefaultTime2Wait,value);
    CFRelease(value);
    
    value = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_DefaultTime2Retain);
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_DefaultTime2Retain,value);
    CFRelease(value);
    
    // Add key-value pair for supported error recovery level.  Use the error
    // recovery level specified by the target.  If the value was invalid, then
    // use the RFC3720 default value of session-level instead.
    enum iSCSIErrorRecoveryLevels errorRecoveryLevel = iSCSISessionConfigGetErrorRecoveryLevel(sessCfg);
    
    switch (errorRecoveryLevel) {
        case kiSCSIErrorRecoverySession:
            value = kRFC3720_Value_ErrorRecoveryLevelSession;
            break;
        case kiSCSIErrorRecoveryDigest:
            value = kRFC3720_Value_ErrorRecoveryLevelDigest;
            break;
        case kiSCSIErrorRecoveryConnection:
            value = kRFC3720_Value_ErrorRecoveryLevelConnection;
            break;
        default:
            value = kRFC3720_Value_ErrorRecoveryLevelSession;
            break;
    }
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_ErrorRecoveryLevel,value);
}

errno_t iSCSINegotiateParseSWDictCommon(SID sessionId,
                                        CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Grab minimum of default time 2 retain
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DefaultTime2Retain,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DefaultTime2Retain);
        UInt32 defaultTime2Retain = CFStringGetIntValue(targetRsp);

        // Range-check value...
        if(iSCSILVRangeInvalid(defaultTime2Retain,kRFC3720_DefaultTime2Retain_Min,kRFC3720_DefaultTime2Retain_Max))
            return ENOTSUP;
        
        defaultTime2Retain = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSODefaultTime2Retain,
                                 &defaultTime2Retain,sizeof(defaultTime2Retain));
    }
    else
        return ENOTSUP;
    
    
    // Grab maximum of default time 2 wait
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DefaultTime2Wait,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DefaultTime2Wait);
        UInt32 defaultTime2Wait = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(defaultTime2Wait,kRFC3720_DefaultTime2Wait_Min,kRFC3720_DefaultTime2Wait_Max))
            return ENOTSUP;
        
        defaultTime2Wait = iSCSILVGetMin(initCmd,targetRsp);;
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSODefaultTime2Wait,
                                 &defaultTime2Wait,sizeof(defaultTime2Wait));
    }
    else
        return ENOTSUP;

    // Grab minimum value of error recovery level
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_ErrorRecoveryLevel,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_ErrorRecoveryLevel);
        UInt8 errorRecoveryLevel = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(errorRecoveryLevel,kRFC3720_ErrorRecoveryLevel_Min,kRFC3720_ErrorRecoveryLevel_Max))
            return ENOTSUP;
        
        errorRecoveryLevel = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOErrorRecoveryLevel,
                                 &errorRecoveryLevel,sizeof(errorRecoveryLevel));
    }
    else
        return ENOTSUP;
    
    // Success
    return 0;
}

errno_t iSCSINegotiateParseSWDictNormal(SID sessionId,
                                        CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxConnections,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxConnections);
        UInt32 maxConnections = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxConnections,kRFC3720_MaxConnections_Min,kRFC3720_MaxConnections_Max))
            return ENOTSUP;
        
        maxConnections = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOMaxConnections,
                                 &maxConnections,sizeof(maxConnections));

    }

    // Grab the OR for initialR2T command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_InitialR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_InitialR2T);
        Boolean initialR2T = iSCSILVGetOr(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOInitialR2T,
                                 &initialR2T,sizeof(initialR2T));
    }
    
    // Grab the AND for immediate data command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_ImmediateData,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_ImmediateData);
        Boolean immediateData = iSCSILVGetAnd(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOImmediateData,
                                 &immediateData,sizeof(immediateData));
    }

    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DataPDUInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DataPDUInOrder);
        Boolean dataPDUInOrder = iSCSILVGetAnd(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSODataPDUInOrder,
                                 &dataPDUInOrder,sizeof(dataPDUInOrder));
    }
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DataSequenceInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DataSequenceInOrder);
        Boolean dataSequenceInOrder = iSCSILVGetAnd(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSODataSequenceInOrder,
                                 &dataSequenceInOrder,sizeof(dataSequenceInOrder));
    }

    // Grab minimum of max burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxBurstLength);
        UInt32 maxBurstLength = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOMaxBurstLength,
                                 &maxBurstLength,sizeof(maxBurstLength));
    }

    // Grab minimum of first burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_FirstBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_FirstBurstLength);
        UInt32 firstBurstLength = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(firstBurstLength,kRFC3720_FirstBurstLength_Min,kRFC3720_FirstBurstLength_Max))
            return ENOTSUP;
        
        firstBurstLength = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOFirstBurstLength,
                                 &firstBurstLength,sizeof(firstBurstLength));
    }

    // Grab minimum of max outstanding R2T
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxOutstandingR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxOutstandingR2T);
        UInt32 maxOutStandingR2T = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxOutStandingR2T,kRFC3720_MaxOutstandingR2T_Min,kRFC3720_MaxOutstandingR2T_Max))
            return ENOTSUP;
        
        maxOutStandingR2T = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOMaxOutstandingR2T,
                                 &maxOutStandingR2T,sizeof(maxOutStandingR2T));
    }
    
    // Success
    return 0;
}

/*! Helper function used by iSCSISessionNegotiateCW to build a dictionary
 *  of connection options (key-value pairs) that will be sent to the target.
 *  @param connCfg a connection configuration object.
 *  @param connCmd  a dictionary that is populated with key-value pairs that
 *  will be used to negotiate connection parameters. */
void iSCSINegotiateBuildCWDict(iSCSIConnectionConfigRef connCfg,
                               CFMutableDictionaryRef connCmd)
{
    // Setup digest options
    if(iSCSIConnectionConfigGetDataDigest(connCfg))
        CFDictionaryAddValue(connCmd,kRFC3720_Key_DataDigest,kRFC3720_Value_DataDigestCRC32C);
    else
        CFDictionaryAddValue(connCmd,kRFC3720_Key_DataDigest,kRFC3720_Value_DataDigestNone);
    
    if(iSCSIConnectionConfigGetHeaderDigest(connCfg))
        CFDictionaryAddValue(connCmd,kRFC3720_Key_HeaderDigest,kRFC3720_Value_HeaderDigestCRC32C);
    else
        CFDictionaryAddValue(connCmd,kRFC3720_Key_HeaderDigest,kRFC3720_Value_HeaderDigestNone);
    
    // Setup maximum received data length
    CFStringRef maxRecvLength = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%u"),kRFC3720_MaxRecvDataSegmentLength);
    
    CFDictionaryAddValue(connCmd,kRFC3720_Key_MaxRecvDataSegmentLength,maxRecvLength);
    
    CFRelease(maxRecvLength);
}

/*! Helper function used by iSCSISessionNegotiateCW to parse a dictionary
 *  of connection options received from the target.  This function stores
 *  those options with the kernel.
 *  @param sessionId the session qualifier.
 *  @param connCmd a dictionary of connection commands (key-value pairs)
 *  that were sent to the target.
 *  @param connRsp a dictionary of connection respones (key-value pairs)
 *  @param connInfo a connection information object.
 *  @param connCfgKernel a connection options object used to store options with
 *  the iSCSI kernel extension
 *  there were received from the target. */
errno_t iSCSINegotiateParseCWDict(SID sessionId,
                                  CID connectionId,
                                  CFDictionaryRef connCmd,
                                  CFDictionaryRef connRsp)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Flag that indicates that target and initiator agree on a parameter
    Boolean agree = false;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_DataDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kRFC3720_Key_DataDigest),targetRsp);
    
    // If we wanted to use data digest and target didn't set connInfo
    Boolean useDataDigest = agree && iSCSILVGetEqual(targetRsp,kRFC3720_Value_DataDigestCRC32C);
    
    // Store value
    iSCSIKernelSetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOUseDataDigest,
                                &useDataDigest,sizeof(useDataDigest));
    
    // Reset agreement flag
    agree = false;
    
    // Get header digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_HeaderDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kRFC3720_Key_HeaderDigest),targetRsp);
    
    // If we wanted to use header digest and target didn't set connInfo
    Boolean useHeaderDigest = agree && iSCSILVGetEqual(targetRsp,kRFC3720_Value_HeaderDigestCRC32C);
    
    // Store value
    iSCSIKernelSetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOUseHeaderDigest,
                                &useHeaderDigest,sizeof(useHeaderDigest));

    
    // This option is declarative; we sent the default length, and the target
    // must accept our choice as it is within a valid range
    UInt32 maxRecvDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    iSCSIKernelSetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOMaxRecvDataSegmentLength,
                                &maxRecvDataSegmentLength,sizeof(maxRecvDataSegmentLength));

    
    // This is the declaration made by the target as to the length it can
    // receive.  Accept the value if it is within the RFC3720 allowed range
    // otherwise, terminate the connection...
    UInt32 maxSendDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_MaxRecvDataSegmentLength,(void*)&targetRsp))
    {
        UInt32 maxSendDataSegmentLengthRsp = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxSendDataSegmentLengthRsp,kRFC3720_MaxRecvDataSegmentLength_Min,kRFC3720_MaxRecvDataSegmentLength_Max))
            return ENOTSUP;
        
        maxSendDataSegmentLength = maxSendDataSegmentLengthRsp;
    }
    
    // Store value
    iSCSIKernelSetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOMaxSendDataSegmentLength,
                                &maxSendDataSegmentLength,sizeof(maxSendDataSegmentLength));
    return 0;
}

errno_t iSCSINegotiateSession(iSCSIMutableTargetRef target,
                              SID sessionId,
                              CID connectionId,
                              iSCSISessionConfigRef sessCfg,
                              iSCSIConnectionConfigRef connCfg,
                              enum iSCSILoginStatusCode * statusCode)
{
    // Create a new dictionary for connection parameters we want to send
    CFMutableDictionaryRef sessCmd = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Add session parameters common to all session types
    iSCSINegotiateBuildSWDictCommon(sessCfg,sessCmd);
    
    // If target name is specified, this is a normal session; add parameters
    if(iSCSITargetGetIQN(target) != NULL)
        iSCSINegotiateBuildSWDictNormal(sessCfg,sessCmd);
    
    // Add connection parameters
    iSCSINegotiateBuildCWDict(connCfg,sessCmd);
    
    // Create a dictionary to store query response
    CFMutableDictionaryRef sessRsp = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    struct iSCSILoginQueryContext context;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDULoginOperationalNegotiation;
    context.nextStage    = kiSCSIPDUFullFeaturePhase;
    context.targetSessionId = 0;

    enum iSCSIPDURejectCode rejectCode;
    
    // Send session-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           sessCmd,sessRsp);
    
    // Parse dictionaries and store session parameters if no I/O error occured
    if(*statusCode == kiSCSILoginSuccess) {
        
        // The TSIH was recorded by iSCSISessionLoginQuery since we're
        // entering the full-feature phase (see iSCSISessionLoginQuery documentation)
        iSCSIKernelSetSessionOpt(sessionId,kiSCSIKernelSOTargetSessionId,
                                 &context.targetSessionId,sizeof(context.targetSessionId));
        if(!error)
            error = iSCSINegotiateParseSWDictCommon(sessionId,sessCmd,sessRsp);
    
        if(!error && iSCSITargetGetIQN(target) != NULL)
            error = iSCSINegotiateParseSWDictNormal(sessionId,sessCmd,sessRsp);
    
        if(!error)
            error = iSCSINegotiateParseCWDict(sessionId,connectionId,sessCmd,sessRsp);
    }
    
    // If no error and the target returned an alias save it...
    CFStringRef targetAlias;
    if(!error && CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_TargetAlias,(const void **)&targetAlias)) {
        iSCSITargetSetAlias(target,targetAlias);
    }
    
    CFRelease(sessCmd);
    CFRelease(sessRsp);
    return error;
}

/*! Helper function.  Negotiates operational parameters for a connection
 *  as part of the login and connection instantiation process. */
errno_t iSCSINegotiateConnection(iSCSITargetRef target,
                                 SID sessionId,
                                 CID connectionId,
                                 enum iSCSILoginStatusCode * statusCode)
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

    struct iSCSILoginQueryContext context;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDULoginOperationalNegotiation;
    context.nextStage    = kiSCSIPDULoginOperationalNegotiation;

    // Addd in targetSessionId from kernel (there may already be an
    // active session if we are simply adding a connection, so we
    // can't assume that this is the leading login and therefore
    // cannot set TSIH to 0.
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOTargetSessionId,
                             &context.targetSessionId,sizeof(context.targetSessionId));
    
    // If the target session ID is non-zero, we're simply adding a new
    // connection and we can enter the full feature after this negotiation.
    if(context.targetSessionId != 0)
        context.nextStage = kiSCSIPDUFullFeaturePhase;

    enum iSCSIPDURejectCode rejectCode;

    // Send connection-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           connCmd,
                                           connRsp);

    // If no error, parse received dictionary and store connection options
    if(!error && *statusCode == kiSCSILoginSuccess)
        error = iSCSINegotiateParseCWDict(sessionId,connectionId,connCmd,connRsp);
    
    CFRelease(connCmd);
    CFRelease(connRsp);
    return error;
}

/*! Helper function used to log out of connections and sessions. */
errno_t iSCSISessionLogoutCommon(SID sessionId,
                                 CID connectionId,
                                 enum iSCSIPDULogoutReasons logoutReason,
                                 enum iSCSILogoutStatusCode * statusCode)
{
    if(sessionId >= kiSCSIInvalidSessionId || connectionId >= kiSCSIInvalidConnectionId)
        return EINVAL;
    
    errno_t error = 0;
    
    // Create a logout PDU and log out of the session
    iSCSIPDULogoutReqBHS cmd = iSCSIPDULogoutReqBHSInit;
    cmd.reasonCode = logoutReason | kISCSIPDULogoutReasonCodeFlag;
    
    if((error = iSCSIKernelSend(sessionId,connectionId,(iSCSIPDUInitiatorBHS *)&cmd,NULL,0)))
        return error;
    
    // Get response from iSCSI portal
    iSCSIPDULogoutRspBHS rsp;
    void * data = NULL;
    size_t length = 0;
    
    // Receive response PDU...
    if((error = iSCSIKernelRecv(sessionId,connectionId,(iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        return error;
    
    if(rsp.opCode == kiSCSIPDUOpCodeLogoutRsp)
        *statusCode = rsp.response;
    
    // For this case some other kind of PDU or invalid data was received
    else if(rsp.opCode == kiSCSIPDUOpCodeReject)
        error = EINVAL;
    
    iSCSIPDUDataRelease(data);
    return error;
}

/*! Helper function used to resolve target nodes as specified by connInfo.
 *  The target nodes specified in connInfo may be a DNS name, an IPv4 or
 *  IPv6 address. */
errno_t iSCSISessionResolveNode(iSCSIPortalRef portal,
                                struct sockaddr_storage * ssTarget,
                                struct sockaddr_storage * ssHost)
{
    if (!portal || !ssTarget || !ssHost)
        return EINVAL;
    
    errno_t error = 0;
    
    // Resolve the target node first and get a sockaddr info for it
    const char * targetAddr, * targetPort;

    targetAddr = CFStringGetCStringPtr(iSCSIPortalGetAddress(portal),kCFStringEncodingUTF8);
    targetPort = CFStringGetCStringPtr(iSCSIPortalGetPort(portal),kCFStringEncodingUTF8);

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    };
    
    struct addrinfo * aiTarget = NULL;
    if((error = getaddrinfo(targetAddr,targetPort,&hints,&aiTarget)))
        return error;
    
    // Copy the sock_addr structure into a sockaddr_storage structure (this
    // may be either an IPv4 or IPv6 sockaddr structure)
    memcpy(ssTarget,aiTarget->ai_addr,aiTarget->ai_addrlen);

    freeaddrinfo(aiTarget);

    // If the default interface is to be used, prepare a structure for it
    CFStringRef hostIface = iSCSIPortalGetHostInterface(portal);

    if(CFStringCompare(hostIface,kiSCSIDefaultHostInterface,0) == kCFCompareEqualTo)
    {
        ssHost->ss_family = ssTarget->ss_family;

        // For completeness, setup the sockaddr_in structure
        if(ssHost->ss_family == AF_INET)
        {
            struct sockaddr_in * sa = (struct sockaddr_in *)ssHost;
            sa->sin_port = 0;
            sa->sin_addr.s_addr = htonl(INADDR_ANY);
            sa->sin_len = sizeof(struct sockaddr_in);
        }

// TODO: test IPv6 functionality
        else if(ssHost->ss_family == AF_INET6)
        {
            struct sockaddr_in6 * sa = (struct sockaddr_in6 *)ssHost;
            sa->sin6_addr = in6addr_any;
        }

        return error;
    }

    // Otherwise we have to search the list of all interfaces for the specified
    // interface and copy the corresponding address structure
    struct ifaddrs * interfaceList;
    
    if((error = getifaddrs(&interfaceList)))
        return error;
    
    error = EAFNOSUPPORT;
    struct ifaddrs * interface = interfaceList;

    while(interface)
    {
        // Check if interface supports the targets address family (e.g., IPv4)
        if(interface->ifa_addr->sa_family == ssTarget->ss_family)
        {
            CFStringRef currIface = CFStringCreateWithCStringNoCopy(
                kCFAllocatorDefault,
                interface->ifa_name,
                kCFStringEncodingUTF8,
                kCFAllocatorNull);

            Boolean ifaceNameMatch =
                CFStringCompare(currIface,hostIface,kCFCompareCaseInsensitive) == kCFCompareEqualTo;
            CFRelease(currIface);
            // Check if interface names match...
            if(ifaceNameMatch)
            {
                memcpy(ssHost,interface->ifa_addr,interface->ifa_addr->sa_len);
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
 *  @param sessionId the new session identifier.
 *  @param portal specifies the portal to use for the connection.
 *  @param initiatorAuth specifies the initiator authentication parameters.
 *  @param targetAuth specifies the target authentication parameters.
 *  @param connCfg the connection configuration parameters to use.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILoginConnection(SID sessionId,
                             iSCSIPortalRef portal,
                             iSCSIAuthRef initiatorAuth,
                             iSCSIAuthRef targetAuth,
                             iSCSIConnectionConfigRef connCfg,
                             CID * connectionId,
                             enum iSCSILoginStatusCode * statusCode)
{

    if(!portal || sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;

    // Reset connection ID by default
    *connectionId = kiSCSIInvalidConnectionId;

    errno_t error = 0;
    
    // Resolve information about the target
    struct sockaddr_storage ssTarget, ssHost;
    
    if((error = iSCSISessionResolveNode(portal,&ssTarget,&ssHost)))
        return error;
    
    // If both target and host were resolved, grab a connection
    error = iSCSIKernelCreateConnection(sessionId,
                                        iSCSIPortalGetAddress(portal),
                                        iSCSIPortalGetPort(portal),
                                        iSCSIPortalGetHostInterface(portal),
                                        &ssTarget,
                                        &ssHost,connectionId);
    
    // If we can't accomodate a new connection quit; try again later
    if(error || *connectionId == kiSCSIInvalidConnectionId)
        return EAGAIN;
    
    iSCSITargetRef targetTemp = iSCSICreateTargetForSessionId(sessionId);
    iSCSIMutableTargetRef target = iSCSITargetCreateMutableCopy(targetTemp);
    iSCSITargetRelease(targetTemp);
    
    // If no error, authenticate (negotiate security parameters)
    if(!error && *statusCode == kiSCSILoginSuccess)
       error = iSCSIAuthNegotiate(target,initiatorAuth,targetAuth,sessionId,*connectionId,statusCode);
    
    if(!error && *statusCode == kiSCSILoginSuccess)
        iSCSIKernelActivateConnection(sessionId,*connectionId);
    else
        iSCSIKernelReleaseConnection(sessionId,*connectionId);
    
    iSCSITargetRelease(target);
    return 0;
}

errno_t iSCSILogoutConnection(SID sessionId,
                              CID connectionId,
                              enum iSCSILogoutStatusCode * statusCode)
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
        return iSCSILogoutSession(sessionId,statusCode);
    
    // Deactivate connection before we remove it (this is optional but good
    // practice, as the kernel will deactivate the connection for us).
    if(!(error = iSCSIKernelDeactivateConnection(sessionId,connectionId))) {
        // Logout the connection or session, as necessary
        error = iSCSISessionLogoutCommon(sessionId,connectionId,
                                         kISCSIPDULogoutCloseConnection,statusCode);
    }
    // Release the connection in the kernel
    iSCSIKernelReleaseConnection(sessionId,connectionId);
    
    return error;
}

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param target specifies the target and connection parameters to use.
 *  @param portal specifies the portal to use for the new session.
 *  @param initiatorAuth specifies the initiator authentication parameters.
 *  @param targetAuth specifies the target authentication parameters.
 *  @param sessCfg the session configuration parameters to use.
 *  @param connCfg the connection configuration parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILoginSession(iSCSIMutableTargetRef target,
                          iSCSIPortalRef portal,
                          iSCSIAuthRef initiatorAuth,
                          iSCSIAuthRef targetAuth,
                          iSCSISessionConfigRef sessCfg,
                          iSCSIConnectionConfigRef connCfg,
                          SID * sessionId,
                          CID * connectionId,
                          enum iSCSILoginStatusCode * statusCode)
{
    if(!target || !portal || !sessCfg || !connCfg || !sessionId || !connectionId ||
       !statusCode || !initiatorAuth || !targetAuth)
        return EINVAL;
    
    // Store errno from helpers and pass back to up the call chain
    errno_t error = 0;
    
    // Resolve the target address
    struct sockaddr_storage ssTarget, ssHost;
    
    if((error = iSCSISessionResolveNode(portal,&ssTarget,&ssHost)))
        return error;
    

    // Create a new session in the kernel.  This allocates session and
    // connection identifiers
    error = iSCSIKernelCreateSession(iSCSITargetGetIQN(target),
                                     iSCSIPortalGetAddress(portal),
                                     iSCSIPortalGetPort(portal),
                                     iSCSIPortalGetHostInterface(portal),
                                     &ssTarget,&ssHost,sessionId,connectionId);
    
    // If session couldn't be allocated were maxed out; try again later
    if(!error && (*sessionId == kiSCSIInvalidSessionId || *connectionId == kiSCSIInvalidConnectionId))
        return EAGAIN;

    // If no error, authenticate (negotiate security parameters)
    if(!error) {
        error = iSCSIAuthNegotiate(target,initiatorAuth,targetAuth,
                                   *sessionId,*connectionId,statusCode);
    }

    // Negotiate session & connection parameters
    if(!error && *statusCode == kiSCSILoginSuccess)
        error = iSCSINegotiateSession(target,*sessionId,*connectionId,sessCfg,connCfg,statusCode);

    // Only activate connections for kernel use if no errors have occurred and
    // the session is not a discovery session
    if(error || *statusCode != kiSCSILoginSuccess)
        iSCSIKernelReleaseSession(*sessionId);
    else if(CFStringCompare(iSCSITargetGetIQN(target),kiSCSIUnspecifiedTargetIQN,0) != kCFCompareEqualTo)
            iSCSIKernelActivateConnection(*sessionId,*connectionId);
    
    return error;
}

/*! Closes the iSCSI session by deactivating and removing all connections. Any
 *  pending or current data transfers are aborted. This function may be called 
 *  on a session with one or more connections that are either inactive or 
 *  active. The session identifier is released and may be reused by other
 *  sessions the future.
 *  @param sessionId the session to release.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSILogoutSession(SID sessionId,
                           enum iSCSILogoutStatusCode * statusCode)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return  EINVAL;
    
    errno_t error = 0;

    // First deactivate all of the connections
    if((error = iSCSIKernelDeactivateAllConnections(sessionId)))
        return error;
    
    // Grab a handle to any connection so we can logout of the session
    CID connectionId = kiSCSIInvalidConnectionId;
    if(!(error = iSCSIKernelGetConnection(sessionId,&connectionId)))
        error = iSCSISessionLogoutCommon(sessionId, connectionId,kiSCSIPDULogoutCloseSession,statusCode);

    // Release all of the connections in the kernel by releasing the session
    iSCSIKernelReleaseSession(sessionId);
    
    return error;
}

/*! Callback function used by iSCSIQueryPortalForTargets to parse discovery
 *  data into an iSCSIDiscoveryRec object. */
void iSCSIPDUDataParseToDiscoveryRecCallback(void * keyContainer,CFStringRef key,
                                             void * valContainer,CFStringRef val)
{
    static CFStringRef targetIQN = NULL;
    
    // If the discovery data has a "TargetName = xxx" field, we're starting
    // a record for a new target
    if(CFStringCompare(key,kRFC3720_Key_TargetName,0) == kCFCompareEqualTo)
    {
        targetIQN = CFStringCreateCopy(kCFAllocatorDefault,val);
        iSCSIMutableDiscoveryRecRef discoveryRec = (iSCSIMutableDiscoveryRecRef)(valContainer);
        iSCSIDiscoveryRecAddTarget(discoveryRec,targetIQN);
    }
    // Otherwise we're dealing with a portal entry. Per RFC3720, this is
    // of the form "TargetAddress = <address>:<port>,<portalGroupTag>
    else if(CFStringCompare(key,kRFC3720_Key_TargetAddress,0) == kCFCompareEqualTo)
    {
        // Split the value string to extract the portal group tag
        CFArrayRef targetAddress = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault,val,CFSTR(","));
        CFStringRef addressAndPort = CFArrayGetValueAtIndex(targetAddress,0);
        CFStringRef portalGroupTag = CFArrayGetValueAtIndex(targetAddress,1);
        
        // Split the address and port, construct an iSCSIPortal object (do the
        // search for a ":" backwards since IPv6 addresses use ":" as
        // separators and the address can be IPv4/IPv6 or a domain name.
        CFRange sepRange = CFStringFind(addressAndPort,CFSTR(":"),kCFCompareBackwards);
        CFRange addressRange = CFRangeMake(0,sepRange.location);
        CFRange portRange = CFRangeMake(sepRange.location+1,CFStringGetLength(addressAndPort)-(sepRange.location+1));
        
        CFStringRef address = CFStringCreateWithSubstring(kCFAllocatorDefault,addressAndPort,addressRange);
        CFStringRef port = CFStringCreateWithSubstring(kCFAllocatorDefault,addressAndPort,portRange);
        
        iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
        iSCSIPortalSetAddress(portal,address);
        iSCSIPortalSetPort(portal,port);
        iSCSIPortalSetHostInterface(portal,kiSCSIDefaultHostInterface);
    
        iSCSIMutableDiscoveryRecRef discoveryRec = (iSCSIMutableDiscoveryRecRef)(valContainer);
        iSCSIDiscoveryRecAddPortal(discoveryRec,targetIQN,portalGroupTag,portal);
        
        CFRelease(port);
        CFRelease(address);
        CFRelease(targetAddress);
        if (targetIQN) {
            CFRelease(targetIQN);
            targetIQN = NULL;
        }
        iSCSIPortalRelease(portal);
    }
}

/*! Queries a portal for available targets (utilizes iSCSI SendTargets).
 *  @param portal the iSCSI portal to query.
 *  @param auth specifies the authentication parameters to use.
 *  @param discoveryRec a discovery record, containing the query results.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryPortalForTargets(iSCSIPortalRef portal,
                                   iSCSIAuthRef initiatorAuth,
                                   iSCSIMutableDiscoveryRecRef * discoveryRec,
                                   enum iSCSILoginStatusCode * statusCode)
{
    if(!portal || !discoveryRec)
        return EINVAL;
    
    // Create a discovery session to the portal (empty target name is assumed to
    // be a discovery session)
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetIQN(target,kiSCSIUnspecifiedTargetIQN);
    
    SID sessionId;
    CID connectionId;
    
    iSCSIMutableSessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
    iSCSIMutableConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();

    iSCSIAuthRef targetAuth = iSCSIAuthCreateNone();

    errno_t error = iSCSILoginSession(target,portal,initiatorAuth,targetAuth,
                                      sessCfg,connCfg,&sessionId,
                                      &connectionId,statusCode);

    iSCSIAuthRelease(targetAuth);
    iSCSITargetRelease(target);
    iSCSISessionConfigRelease(sessCfg);
    iSCSIConnectionConfigRelease(connCfg);
    
    if(error)
        return error;
    
    // Place text commands to get target list into a dictionary
    CFMutableDictionaryRef textCmd =
        CFDictionaryCreateMutable(kCFAllocatorDefault,
                                  kiSCSISessionMaxTextKeyValuePairs,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
    
    // Can't use a text query; must manually send/receive as the received
    // keys will be duplicates and CFDictionary doesn't support them
    CFDictionarySetValue(textCmd,kRFC3720_Key_SendTargets,kRFC3720_Value_SendTargetsAll);
     
    // Create a data segment based on text commands (key-value pairs)
    void * data;
    size_t length;
    iSCSIPDUDataCreateFromDict(textCmd,&data,&length);
    
    iSCSIPDUTextReqBHS cmd = iSCSIPDUTextReqBHSInit;
    cmd.textReqStageFlags |= kiSCSIPDUTextReqFinalFlag;
    cmd.targetTransferTag = kiSCSIPDUTargetTransferTagReserved;
     
    error = iSCSIKernelSend(sessionId,connectionId,(iSCSIPDUInitiatorBHS *)&cmd,data,length);
    
    iSCSIPDUDataRelease(&data);
     
    if(error)
    {
        enum iSCSILogoutStatusCode statusCode;
        iSCSILogoutSession(sessionId,&statusCode);
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDUTextRspBHS rsp;
    
    *discoveryRec = iSCSIDiscoveryRecCreateMutable();

    do {
        if((error = iSCSIKernelRecv(sessionId,connectionId,(iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);

            enum iSCSILogoutStatusCode statusCode;
            iSCSILogoutSession(sessionId,&statusCode);

            return error;
        }
     
        if(rsp.opCode == kiSCSIPDUOpCodeTextRsp)
        {
            iSCSIPDUDataParseCommon(data,length,NULL,*discoveryRec,
                                    &iSCSIPDUDataParseToDiscoveryRecCallback);
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
    
    enum iSCSILogoutStatusCode logoutStatusCode;
    iSCSILogoutSession(sessionId,&logoutStatusCode);

    // Per RFC3720, the "TargetAddress" key is optional in a SendTargets
    // discovery operation.  Therefore, certain targets may respond with
    // a "TargetName" only, implying that the portal used for discovery
    // can also be used for access to the target.  For these targets, we must
    // add the discovery portal to the discovery record.
    CFArrayRef targets;

    if(!(targets = iSCSIDiscoveryRecCreateArrayOfTargets(*discoveryRec)))
       return error;

    for(CFIndex idx = 0; idx < CFArrayGetCount(targets); idx++) {
        CFStringRef targetIQN;

        if(!(targetIQN = CFArrayGetValueAtIndex(targets,idx)))
            continue;

        CFArrayRef portalGroups = iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(
            *discoveryRec,targetIQN);

        // If at least one portal group exists then we can skip this target...
        if(CFArrayGetCount(portalGroups) != 0) {
            CFRelease(portalGroups);
            continue;
        }

        // Otherwise we need to create a new portal group and add the discovery
        // portal for this target
        iSCSIDiscoveryRecAddPortal(*discoveryRec,targetIQN,CFSTR("0"),portal);
        CFRelease(portalGroups);
    }

    CFRelease(targets);
    return error;
}

/*! Retrieves a list of targets available from a give portal.
 *  @param portal the iSCSI portal to look for targets.
 *  @param initiatorAuth specifies the initiator authentication parameters.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryTargetForAuthMethod(iSCSIPortalRef portal,
                                      CFStringRef targetIQN,
                                      enum iSCSIAuthMethods * authMethod,
                                      enum iSCSILoginStatusCode * statusCode)
{
    if(!portal || !authMethod)
        return EINVAL;
    
    // Store errno from helpers and pass back up the call chain
    errno_t error = 0;
    
    // Resolve information about the target
    struct sockaddr_storage ssTarget, ssHost;
    
    if((error = iSCSISessionResolveNode(portal,&ssTarget,&ssHost)))
        return error;
    
    // Create a discovery session to the portal
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetIQN(target,targetIQN);
    
    // Create session (incl. qualifier) and a new connection (incl. Id)
    // Reset qualifier and connection ID by default
    SID sessionId;
    CID connectionId;
    error = iSCSIKernelCreateSession(targetIQN,
                                     iSCSIPortalGetAddress(portal),
                                     iSCSIPortalGetPort(portal),
                                     iSCSIPortalGetHostInterface(portal),
                                     &ssTarget,
                                     &ssHost,
                                     &sessionId,
                                     &connectionId);
    
    // If no error, authenticate (negotiate security parameters)
    if(!error)
        error = iSCSIAuthInterrogate(target,
                                     sessionId,
                                     connectionId,
                                     authMethod,
                                     statusCode);

    iSCSIKernelReleaseSession(sessionId);
    iSCSITargetRelease(target);
    
    return error;
}

/*! Gets the session identifier associated with the specified target.
 *  @param targetIQN the name of the target.
 *  @return the session identiifer. */
SID iSCSIGetSessionIdForTarget(CFStringRef targetIQN)
{
    return iSCSIKernelGetSessionIdForTargetIQN(targetIQN);
}

/*! Gets the connection identifier associated with the specified portal.
 *  @param sessionId the session identifier.
 *  @param portal the portal connected on the specified session.
 *  @return the associated connection identifier. */
CID iSCSIGetConnectionIdForPortal(SID sessionId,iSCSIPortalRef portal)
{
    return iSCSIKernelGetConnectionIdForPortalAddress(sessionId,iSCSIPortalGetAddress(portal));
}

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  @return an array of session identifiers. */
CFArrayRef iSCSICreateArrayOfSessionIds()
{
    SID sessionIds[kiSCSIMaxSessions];
    UInt16 sessionCount;
    
    if(iSCSIKernelGetSessionIds(sessionIds,&sessionCount))
        return NULL;
    
    return CFArrayCreate(kCFAllocatorDefault,(const void **) sessionIds,sessionCount,NULL);
}

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @return an array of connection identifiers. */
CFArrayRef iSCSICreateArrayOfConnectionsIds(SID sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    CID connectionIds[kiSCSIMaxConnectionsPerSession];
    UInt32 connectionCount;
    
    if(iSCSIKernelGetConnectionIds(sessionId,connectionIds,&connectionCount))
        return NULL;

    return CFArrayCreate(kCFAllocatorDefault,(const void **)&connectionIds,connectionCount,NULL);
}

/*! Creates a target object for the specified session.
 *  @param sessionId the session identifier.
 *  @return target the target object. */
iSCSITargetRef iSCSICreateTargetForSessionId(SID sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    CFStringRef targetIQN = iSCSIKernelCreateTargetIQNForSessionId(sessionId);
    
    if(!targetIQN)
        return NULL;
    
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetIQN(target,targetIQN);

    CFRelease(targetIQN);
    
    return target;
}

/*! Creates a connection object for the specified connection.
 *  @param sessionId the session identifier.
 *  @param connectionId the connection identifier.
 *  @return portal information about the portal. */
iSCSIPortalRef iSCSICreatePortalForConnectionId(SID sessionId,CID connectionId)
                                      
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    CFStringRef address,port,hostInterface;
    
    if(!(address = iSCSIKernelCreatePortalAddressForConnectionId(sessionId,connectionId)))
        return NULL;

    if(!(port = iSCSIKernelCreatePortalPortForConnectionId(sessionId,connectionId)))
    {
        CFRelease(address);
        return NULL;
    }
    
    if(!(hostInterface = iSCSIKernelCreateHostInterfaceForConnectionId(sessionId,connectionId)))
    {
        CFRelease(address);
        CFRelease(port);
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,address);
    iSCSIPortalSetPort(portal,port);
    iSCSIPortalSetHostInterface(portal,hostInterface);
    CFRelease(address);
    CFRelease(port);
    CFRelease(hostInterface);
    
    return portal;
}

/*! Creates a dictionary of session parameters for the session associated with
 *  the specified target, if one exists.
 *  @param handle a handle to a daemon connection.
 *  @param target the target to check for associated sessions to generate
 *  a dictionary of session parameters.
 *  @return a dictionary of session properties. */
CFDictionaryRef iSCSICreateCFPropertiesForSession(iSCSITargetRef target)
{
    if(!target)
        return NULL;

    CFDictionaryRef dictionary = NULL;
    SID sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target));
    
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    // Get session options from kernel

    UInt32 optVal32 = 0;
    
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOMaxConnections,&optVal32,sizeof(optVal32));
    CFNumberRef maxConnections = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOMaxBurstLength,&optVal32,sizeof(optVal32));
    CFNumberRef maxBurstLength = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOFirstBurstLength,&optVal32,sizeof(optVal32));
    CFNumberRef firstBurstLength = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOMaxOutstandingR2T,&optVal32,sizeof(optVal32));
    CFNumberRef maxOutStandingR2T = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSODefaultTime2Retain,&optVal32,sizeof(optVal32));
    CFNumberRef defaultTime2Retain = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);
    
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSODefaultTime2Wait,&optVal32,sizeof(optVal32));
    CFNumberRef defaultTime2Wait = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);
    

    TPGT tpgt = 0;
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOTargetPortalGroupTag,&tpgt,sizeof(TPGT));
    CFNumberRef targetPortalGroupTag = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt16Type,&tpgt);

    TSIH tsih = 0;
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOTargetSessionId,&tsih,sizeof(TSIH));
    CFNumberRef targetSessionId = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt16Type,&tsih);

    UInt8 optVal8 = 0;
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOErrorRecoveryLevel,&optVal8,sizeof(optVal8));
    CFNumberRef errorRecoveryLevel = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt8Type,&optVal8);

    CFNumberRef sessionIdentifier = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt16Type,&sessionId);
    
    CFStringRef initialR2T = kRFC3720_Value_No;
    CFStringRef immediateData = kRFC3720_Value_No;
    CFStringRef dataPDUInOrder = kRFC3720_Value_No;
    CFStringRef dataSequenceInOrder = kRFC3720_Value_No;

    Boolean optValBool = false;
    
    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOImmediateData,&optValBool,sizeof(Boolean));
    if(optValBool)
        immediateData = kRFC3720_Value_Yes;

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSOInitialR2T,&optValBool,sizeof(Boolean));
    if(optValBool)
        initialR2T = kRFC3720_Value_Yes;

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSODataPDUInOrder,&optValBool,sizeof(Boolean));
    if(optValBool)
        dataPDUInOrder = kRFC3720_Value_Yes;

    iSCSIKernelGetSessionOpt(sessionId,kiSCSIKernelSODataSequenceInOrder,&optValBool,sizeof(Boolean));
    if(optValBool)
        dataSequenceInOrder = kRFC3720_Value_Yes;

    const void * keys[] = {
        kRFC3720_Key_InitialR2T,
        kRFC3720_Key_ImmediateData,
        kRFC3720_Key_DataPDUInOrder,
        kRFC3720_Key_DataSequenceInOrder,
        kRFC3720_Key_MaxConnections,
        kRFC3720_Key_MaxBurstLength,
        kRFC3720_Key_FirstBurstLength,
        kRFC3720_Key_MaxOutstandingR2T,
        kRFC3720_Key_DefaultTime2Retain,
        kRFC3720_Key_DefaultTime2Wait,
        kRFC3720_Key_TargetPortalGroupTag,
        kRFC3720_Key_TargetSessionId,
        kRFC3720_Key_ErrorRecoveryLevel,
        kRFC3720_Key_SessionId
    };

    const void * values[] = {
        initialR2T,
        immediateData,
        dataPDUInOrder,
        dataSequenceInOrder,
        maxConnections,
        maxBurstLength,
        firstBurstLength,
        maxOutStandingR2T,
        defaultTime2Retain,
        defaultTime2Wait,
        targetPortalGroupTag,
        targetSessionId,
        errorRecoveryLevel,
        sessionIdentifier
    };

    dictionary = CFDictionaryCreate(kCFAllocatorDefault,keys,values,
                                    sizeof(keys)/sizeof(void*),
                                    &kCFTypeDictionaryKeyCallBacks,
                                    &kCFTypeDictionaryValueCallBacks);

    return dictionary;
}

/*! Creates a dictionary of connection parameters for the connection associated
 *  with the specified target and portal, if one exists.
 *  @param handle a handle to a daemon connection.
 *  @param target the target associated with the the specified portal.
 *  @param portal the portal to check for active connections to generate
 *  a dictionary of connection parameters.
 *  @return a dictionary of connection properties. */
CFDictionaryRef iSCSICreateCFPropertiesForConnection(iSCSITargetRef target,
                                                     iSCSIPortalRef portal)
{
    if(!target || !portal)
        return NULL;

    CFDictionaryRef dictionary = NULL;

    SID sessionId = iSCSIGetSessionIdForTarget(iSCSITargetGetIQN(target));
    CID connectionId = kiSCSIInvalidConnectionId;

    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;

    // Validate connection identifier
    connectionId = iSCSIGetConnectionIdForPortal(sessionId,portal);
    if(connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    // Get connection options from kernel

    UInt32 optVal32 = 0;
    iSCSIKernelGetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOMaxRecvDataSegmentLength,&optVal32,sizeof(optVal32));
    CFNumberRef maxRecvDataSegmentLength = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&optVal32);
    
    CFNumberRef connectionIdentifier = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&connectionId);


    enum iSCSIDigestTypes dataDigestType = kiSCSIDigestNone;
    enum iSCSIDigestTypes headerDigestType = kiSCSIDigestNone;

    Boolean optValBool = false;
    
    iSCSIKernelGetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOUseDataDigest,&optValBool,sizeof(Boolean));
    if(optValBool)
        dataDigestType = kiSCSIDigestCRC32C;

    iSCSIKernelGetConnectionOpt(sessionId,connectionId,kiSCSIKernelCOUseHeaderDigest,&optValBool,sizeof(Boolean));
    if(optValBool)
        dataDigestType = kiSCSIDigestCRC32C;

    CFNumberRef dataDigest = CFNumberCreate(kCFAllocatorDefault,
                                            kCFNumberCFIndexType
                                            ,&dataDigestType);

    CFNumberRef headerDigest = CFNumberCreate(kCFAllocatorDefault,
                                              kCFNumberCFIndexType,
                                              &headerDigestType);
    const void * keys[] = {
        kRFC3720_Key_DataDigest,
        kRFC3720_Key_HeaderDigest,
        kRFC3720_Key_MaxRecvDataSegmentLength,
        kRFC3720_Key_ConnectionId
    };

    const void * values[] = {
        dataDigest,
        headerDigest,
        maxRecvDataSegmentLength,
        connectionIdentifier
    };

    dictionary = CFDictionaryCreate(kCFAllocatorDefault,keys,values,
                                    sizeof(keys)/sizeof(void*),
                                    &kCFTypeDictionaryKeyCallBacks,
                                    &kCFTypeDictionaryValueCallBacks);
    return dictionary;
}

/*! Sets the name of this initiator.  This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param initiatorIQN the initiator name. */
void iSCSISetInitiatorName(CFStringRef initiatorIQN)
{
    if(!initiatorIQN)
        return;
    
    CFRelease(kiSCSIInitiatorIQN);
    kiSCSIInitiatorIQN = CFStringCreateCopy(kCFAllocatorDefault,initiatorIQN);
}

/*! Sets the alias of this initiator.  This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param initiatorAlias the initiator alias. */
void iSCSISetInitiatorAlias(CFStringRef initiatorAlias)
{
    if(!initiatorAlias)
        return;
    
    CFRelease(kiSCSIInitiatorAlias);
    kiSCSIInitiatorAlias = CFStringCreateCopy(kCFAllocatorDefault,initiatorAlias);
}

/*! The kernel calls this function only for asynchronous events that
 *  involve dropped sessions, connections, logout requests and parameter
 *  negotiation. This function is not called for asynchronous SCSI messages
 *  or vendor-specific messages. */
void iSCSISessionHandleKernelNotificationAsyncMessage(iSCSIKernelNotificationAsyncMessage * msg)
{
    enum iSCSIPDUAsyncMsgEvent asyncEvent = (enum iSCSIPDUAsyncMsgEvent)msg->asyncEvent;
    enum iSCSILogoutStatusCode statusCode;
    
    CFStringRef statusString = CFStringCreateWithFormat(kCFAllocatorDefault,0,
        CFSTR("iSCSI asynchronous message (code %d) received (sid: %d, cid: %d)"),
        asyncEvent,msg->sessionId,msg->connectionId);
    
    asl_log(NULL,NULL,ASL_LEVEL_WARNING,"%s",CFStringGetCStringPtr(statusString,kCFStringEncodingASCII));
    
    CFRelease(statusString);
    
    switch (asyncEvent) {
    
        // We are required to issue a logout request
        case kiSCSIPDUAsyncMsgLogout:
            iSCSILogoutConnection(msg->sessionId,msg->connectionId,&statusCode);
            break;
            
        // We have been asked to re-negotiate parameters for this connection
        // (this is currently unsupported and we logout)
        case kiSCSIPDUAsyncMsgNegotiateParams:
            iSCSILogoutConnection(msg->sessionId,msg->connectionId,&statusCode);
            break;

        default:
            break;
    }
}

/*! This function is called by the kernel extension to notify the daemon
 *  of an event that has occured within the kernel extension. */
void iSCSISessionHandleKernelNotifications(enum iSCSIKernelNotificationTypes type,
                                     iSCSIKernelNotificationMessage * msg)
{
    // Process an asynchronous message
    switch(type)
    {
        // The kernel received an iSCSI asynchronous event message
        case kiSCSIKernelNotificationAsyncMessage:
            iSCSISessionHandleKernelNotificationAsyncMessage((iSCSIKernelNotificationAsyncMessage *)msg);
            break;
        case kISCSIKernelNotificationTerminate: break;
        default: break;
    };
}

/*! Call to initialize iSCSI session management functions.  This function will
 *  initialize the kernel layer after which other session-related functions
 *  may be called.
 *  @param rl the runloop to use for executing session-related functions.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIInitialize(CFRunLoopRef rl)
{
    errno_t error = iSCSIKernelInitialize(&iSCSISessionHandleKernelNotifications);
    
    if(!error) {
        CFRunLoopSourceRef source;
        
        // Create a run loop source tied to the kernel notification system;
        // if fail then kext may not be loaded, try again later
        if((source = iSCSIKernelCreateRunLoopSource()))
            CFRunLoopAddSource(rl,source,kCFRunLoopDefaultMode);
        else
            error = EAGAIN;
    }
    return error;
}

/*! Called to cleanup kernel resources used by the iSCSI session management
 *  functions.  This function will close any connections to the kernel
 *  and stop processing messages related to the kernel.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICleanup()
{
    return iSCSIKernelCleanup();
}
