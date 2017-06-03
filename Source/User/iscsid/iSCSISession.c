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
#include "iSCSISessionManager.h"
#include "iSCSIPDUUser.h"
#include "iSCSIHBAInterface.h"
#include "iSCSIAuth.h"
#include "iSCSIQueryTarget.h"

#include "iSCSI.h"

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

errno_t iSCSINegotiateParseSWDictCommon(iSCSISessionManagerRef managerRef,
                                        SessionIdentifier sessionId,
                                        CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp)

{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    
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
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,
                                             kiSCSIHBASODefaultTime2Retain,
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
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,
                                             kiSCSIHBASODefaultTime2Wait,
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
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOErrorRecoveryLevel,
                                 &errorRecoveryLevel,sizeof(errorRecoveryLevel));
    }
    else
        return ENOTSUP;
    
    // Success
    return 0;
}

errno_t iSCSINegotiateParseSWDictNormal(iSCSISessionManagerRef managerRef,
                                        SessionIdentifier sessionId,
                                        CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp)

{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Holds parameters that are used to process other parameters
    Boolean initialR2T = false, immediateData = false;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxConnections,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxConnections);
        UInt32 maxConnections = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxConnections,kRFC3720_MaxConnections_Min,kRFC3720_MaxConnections_Max))
            return ENOTSUP;
        
        maxConnections = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOMaxConnections,
                                             &maxConnections,sizeof(maxConnections));
    }

    // Grab the OR for initialR2T command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_InitialR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_InitialR2T);
        initialR2T = iSCSILVGetOr(initCmd,targetRsp);
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOInitialR2T,
                                             &initialR2T,sizeof(initialR2T));
    }
    
    // Grab the AND for immediate data command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_ImmediateData,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_ImmediateData);
        immediateData = iSCSILVGetAnd(initCmd,targetRsp);
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOImmediateData,
                                             &immediateData,sizeof(immediateData));
    }

    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DataPDUInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DataPDUInOrder);
        Boolean dataPDUInOrder = iSCSILVGetAnd(initCmd,targetRsp);
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASODataPDUInOrder,
                                 &dataPDUInOrder,sizeof(dataPDUInOrder));
    }
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DataSequenceInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DataSequenceInOrder);
        Boolean dataSequenceInOrder = iSCSILVGetAnd(initCmd,targetRsp);
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASODataSequenceInOrder,
                                 &dataSequenceInOrder,sizeof(dataSequenceInOrder));
    }

    // Grab minimum of max burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxBurstLength);
        UInt32 maxBurstLength = iSCSILVGetMin(initCmd,targetRsp);
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOMaxBurstLength,
                                 &maxBurstLength,sizeof(maxBurstLength));
    }

    // Grab minimum of first burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_FirstBurstLength,(void*)&targetRsp))
    {
        // This parameter is irrelevant when initialR2T = yes and immediateData = no.
        if(!initialR2T || immediateData) {
        
            CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_FirstBurstLength);
            UInt32 firstBurstLength = CFStringGetIntValue(targetRsp);
        
            // Range-check value...
            if(iSCSILVRangeInvalid(firstBurstLength,kRFC3720_FirstBurstLength_Min,kRFC3720_FirstBurstLength_Max))
                return ENOTSUP;
        
            firstBurstLength = iSCSILVGetMin(initCmd,targetRsp);
            iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOFirstBurstLength,
                                                 &firstBurstLength,sizeof(firstBurstLength));
        }
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
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOMaxOutstandingR2T,
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
errno_t iSCSINegotiateParseCWDict(iSCSISessionManagerRef managerRef,
                                  SessionIdentifier sessionId,
                                  ConnectionIdentifier connectionId,
                                  CFDictionaryRef connCmd,
                                  CFDictionaryRef connRsp)

{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    
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
    iSCSIHBAInterfaceSetConnectionParameter(hbaInterface,sessionId,connectionId,
                                            kiSCSIHBACOUseDataDigest,
                                            &useDataDigest,sizeof(useDataDigest));
    // Reset agreement flag
    agree = false;
    
    // Get header digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_HeaderDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kRFC3720_Key_HeaderDigest),targetRsp);
    
    // If we wanted to use header digest and target didn't set connInfo
    Boolean useHeaderDigest = agree && iSCSILVGetEqual(targetRsp,kRFC3720_Value_HeaderDigestCRC32C);
    
    // Store value
    iSCSIHBAInterfaceSetConnectionParameter(hbaInterface,sessionId,connectionId,
                                            kiSCSIHBACOUseHeaderDigest,
                                            &useHeaderDigest,sizeof(useHeaderDigest));
    
    // This option is declarative; we sent the default length, and the target
    // must accept our choice as it is within a valid range
    UInt32 maxRecvDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    iSCSIHBAInterfaceSetConnectionParameter(hbaInterface,sessionId,connectionId,
                                            kiSCSIHBACOMaxRecvDataSegmentLength,
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
    iSCSIHBAInterfaceSetConnectionParameter(hbaInterface,sessionId,connectionId,
                                            kiSCSIHBACOMaxSendDataSegmentLength,
                                            &maxSendDataSegmentLength,sizeof(maxSendDataSegmentLength));
    return 0;
}

errno_t iSCSINegotiateSession(iSCSISessionManagerRef managerRef,
                              iSCSIMutableTargetRef target,
                              SessionIdentifier sessionId,
                              ConnectionIdentifier connectionId,
                              iSCSISessionConfigRef sessCfg,
                              iSCSIConnectionConfigRef connCfg,
                              enum iSCSILoginStatusCode * statusCode)
{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    
    // Create a new dictionary for connection parameters we want to send
    CFMutableDictionaryRef sessCmd = CFDictionaryCreateMutable(
                                            kCFAllocatorDefault,
                                            kiSCSISessionMaxTextKeyValuePairs,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    
    // Add session parameters common to all session types
    iSCSINegotiateBuildSWDictCommon(sessCfg,sessCmd);
    
    // If target name is specified, this is a normal session; add parameters
    Boolean discoverySession = CFStringCompare(iSCSITargetGetIQN(target),kiSCSIUnspecifiedTargetIQN,0) == kCFCompareEqualTo;
    if(!discoverySession)
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
    context.interface    = hbaInterface;
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
        iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,
                                             kiSCSIHBASOTargetSessionId,
                                             &context.targetSessionId,sizeof(context.targetSessionId));
        if(!error)
            error = iSCSINegotiateParseSWDictCommon(managerRef,sessionId,sessCmd,sessRsp);
    
        if(!error && iSCSITargetGetIQN(target) != NULL)
            error = iSCSINegotiateParseSWDictNormal(managerRef,sessionId,sessCmd,sessRsp);
    
        if(!error)
            error = iSCSINegotiateParseCWDict(managerRef,sessionId,connectionId,sessCmd,sessRsp);
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
errno_t iSCSINegotiateConnection(iSCSISessionManagerRef managerRef,
                                 iSCSITargetRef target,
                                 SessionIdentifier sessionId,
                                 ConnectionIdentifier connectionId,
                                 enum iSCSILoginStatusCode * statusCode)
{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);

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
    context.interface    = hbaInterface;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDULoginOperationalNegotiation;
    context.nextStage    = kiSCSIPDULoginOperationalNegotiation;

    // Addd in targetSessionId from kernel (there may already be an
    // active session if we are simply adding a connection, so we
    // can't assume that this is the leading login and therefore
    // cannot set TSIH to 0.
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOTargetSessionId,
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
        error = iSCSINegotiateParseCWDict(managerRef,sessionId,connectionId,connCmd,connRsp);
    
    CFRelease(connCmd);
    CFRelease(connRsp);
    return error;
}

/*! Helper function used to log out of connections and sessions. */
errno_t iSCSISessionLogoutCommon(iSCSISessionManagerRef managerRef,
                                 SessionIdentifier sessionId,
                                 ConnectionIdentifier connectionId,
                                 enum iSCSIPDULogoutReasons logoutReason,
                                 enum iSCSILogoutStatusCode * statusCode)
{
    if(sessionId >= kiSCSIInvalidSessionId || connectionId >= kiSCSIInvalidConnectionId)
        return EINVAL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    errno_t error = 0;
    
    // Create a logout PDU and log out of the session
    iSCSIPDULogoutReqBHS cmd = iSCSIPDULogoutReqBHSInit;
    cmd.reasonCode = logoutReason | kISCSIPDULogoutReasonCodeFlag;
    
    if((error = iSCSIHBAInterfaceSend(hbaInterface,sessionId,connectionId,(iSCSIPDUInitiatorBHS *)&cmd,NULL,0)))
        return error;
    
    // Get response from iSCSI portal
    iSCSIPDULogoutRspBHS rsp;
    void * data = NULL;
    size_t length = 0;
    
    // Receive response PDU...
    if((error = iSCSIHBAInterfaceReceive(hbaInterface,sessionId,connectionId,(iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        return error;
    
    if(rsp.opCode == kiSCSIPDUOpCodeLogoutRsp)
        *statusCode = rsp.response;
    
    // For this case some other kind of PDU or invalid data was received
    else if(rsp.opCode == kiSCSIPDUOpCodeReject)
        error = EINVAL;
    
    iSCSIPDUDataRelease(data);
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
errno_t iSCSISessionAddConnection(iSCSISessionManagerRef managerRef,
                                  SessionIdentifier sessionId,
                                  iSCSIPortalRef portal,
                                  iSCSIAuthRef initiatorAuth,
                                  iSCSIAuthRef targetAuth,
                                  iSCSIConnectionConfigRef connCfg,
                                  ConnectionIdentifier * connectionId,
                                  enum iSCSILoginStatusCode * statusCode)
{

    if(!portal || sessionId == kiSCSIInvalidSessionId || !connectionId)
        return EINVAL;

    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    *connectionId = kiSCSIInvalidConnectionId;
    errno_t error = 0;
    
    // Resolve information about the target
    struct sockaddr_storage ssTarget, ssHost;
    
    if((error = iSCSIUtilsGetAddressForPortal(portal,&ssTarget,&ssHost)))
        return error;
    
    // If both target and host were resolved, grab a connection
    error = iSCSIHBAInterfaceCreateConnection(hbaInterface,sessionId,
                                              iSCSIPortalGetAddress(portal),
                                              iSCSIPortalGetPort(portal),
                                              iSCSIPortalGetHostInterface(portal),
                                              &ssTarget,
                                              &ssHost,connectionId);
    
    // If we can't accomodate a new connection quit; try again later
    if(error || *connectionId == kiSCSIInvalidConnectionId)
        return EAGAIN;
    
    iSCSITargetRef targetTemp = iSCSISessionCopyTargetForId(managerRef,sessionId);
    iSCSIMutableTargetRef target = iSCSITargetCreateMutableCopy(targetTemp);
    iSCSITargetRelease(targetTemp);
    
    // If no error, authenticate (negotiate security parameters)
    if(!error && *statusCode == kiSCSILoginSuccess)
       error = iSCSIAuthNegotiate(managerRef,target,initiatorAuth,targetAuth,sessionId,*connectionId,statusCode);
    
    if(!error && *statusCode == kiSCSILoginSuccess)
        iSCSIHBAInterfaceActivateConnection(hbaInterface,sessionId,*connectionId);
    else
        iSCSIHBAInterfaceReleaseConnection(hbaInterface,sessionId,*connectionId);
    
    iSCSITargetRelease(target);
    return 0;
}

errno_t iSCSISessionRemoveConnection(iSCSISessionManagerRef managerRef,
                                     SessionIdentifier sessionId,
                                     ConnectionIdentifier connectionId,
                                     enum iSCSILogoutStatusCode * statusCode)
{
    if(sessionId >= kiSCSIInvalidSessionId || connectionId >= kiSCSIInvalidConnectionId)
        return EINVAL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    errno_t error = 0;
    
    // Release the session instead if there's only a single connection
    // for this session
    UInt32 numConnections = 0;
    if((error = iSCSIHBAInterfaceGetNumConnections(hbaInterface,sessionId,&numConnections)))
        return error;
    
    if(numConnections == 1)
        return iSCSISessionLogout(managerRef,sessionId,statusCode);
    
    // Deactivate connection before we remove it (this is optional but good
    // practice, as the kernel will deactivate the connection for us).
    if(!(error = iSCSIHBAInterfaceDeactivateConnection(hbaInterface,sessionId,connectionId))) {
        // Logout the connection or session, as necessary
        error = iSCSISessionLogoutCommon(managerRef,sessionId,connectionId,
                                         kISCSIPDULogoutCloseConnection,statusCode);
    }
    // Release the connection in the kernel
    iSCSIHBAInterfaceReleaseConnection(hbaInterface,sessionId,connectionId);
    
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
errno_t iSCSISessionLogin(iSCSISessionManagerRef managerRef,
                          iSCSIMutableTargetRef target,
                          iSCSIPortalRef portal,
                          iSCSIAuthRef initiatorAuth,
                          iSCSIAuthRef targetAuth,
                          iSCSISessionConfigRef sessCfg,
                          iSCSIConnectionConfigRef connCfg,
                          SessionIdentifier * sessionId,
                          ConnectionIdentifier * connectionId,
                          enum iSCSILoginStatusCode * statusCode)
{
    if(!target || !portal || !sessCfg || !connCfg || !sessionId || !connectionId ||
       !statusCode || !initiatorAuth || !targetAuth)
        return EINVAL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    errno_t error = 0;
    
    // Resolve the target address
    struct sockaddr_storage ssTarget, ssHost;
    
    if((error = iSCSIUtilsGetAddressForPortal(portal,&ssTarget,&ssHost)))
        return error;

    // Create a new session in the kernel.  This allocates session and
    // connection identifiers
    error = iSCSIHBAInterfaceCreateSession(hbaInterface,
                                           iSCSITargetGetIQN(target),
                                           iSCSIPortalGetAddress(portal),
                                           iSCSIPortalGetPort(portal),
                                           iSCSIPortalGetHostInterface(portal),
                                           &ssTarget,&ssHost,sessionId,connectionId);
    
    // If session couldn't be allocated were maxed out; try again later
    if(!error && (*sessionId == kiSCSIInvalidSessionId || *connectionId == kiSCSIInvalidConnectionId))
        return EAGAIN;

    // If no error, authenticate (negotiate security parameters)
    if(!error) {
        error = iSCSIAuthNegotiate(managerRef,target,initiatorAuth,targetAuth,
                                   *sessionId,*connectionId,statusCode);
    }

    // Negotiate session & connection parameters
    if(!error && *statusCode == kiSCSILoginSuccess)
        error = iSCSINegotiateSession(managerRef,target,*sessionId,*connectionId,sessCfg,connCfg,statusCode);

    // Only activate connections for kernel use if no errors have occurred and
    // the session is not a discovery session
    if(error || *statusCode != kiSCSILoginSuccess)
        iSCSIHBAInterfaceReleaseSession(hbaInterface,*sessionId);
    else if(CFStringCompare(iSCSITargetGetIQN(target),kiSCSIUnspecifiedTargetIQN,0) != kCFCompareEqualTo)
        iSCSIHBAInterfaceActivateConnection(hbaInterface,*sessionId,*connectionId);
    
    return error;
}

/*! Closes the iSCSI session by deactivating and removing all connections. Any
 *  pending or current data transfers are aborted. This function may be called 
 *  on a session with one or more connections that are either inactive or 
 *  active. The session identifier is released and may be reused by other
 *  sessions the future.
 *  @param sessionId the session to release.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISessionLogout(iSCSISessionManagerRef managerRef,
                           SessionIdentifier sessionId,
                           enum iSCSILogoutStatusCode * statusCode)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return  EINVAL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    errno_t error = 0;

    // First deactivate all of the connections
    if((error = iSCSIHBAInterfaceDeactivateAllConnections(hbaInterface,sessionId)))
        return error;
    
    // Grab a handle to any connection so we can logout of the session
    ConnectionIdentifier connectionId = kiSCSIInvalidConnectionId;
    if(!(error = iSCSIHBAInterfaceGetConnection(hbaInterface,sessionId,&connectionId)))
        error = iSCSISessionLogoutCommon(managerRef,sessionId,connectionId,kiSCSIPDULogoutCloseSession,statusCode);

    // Release all of the connections in the kernel by releasing the session
    iSCSIHBAInterfaceReleaseSession(hbaInterface,sessionId);
    
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
errno_t iSCSIQueryPortalForTargets(iSCSISessionManagerRef managerRef,
                                   iSCSIPortalRef portal,
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
    
    SessionIdentifier sessionId;
    ConnectionIdentifier connectionId;
    
    iSCSIMutableSessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
    iSCSIMutableConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();

    iSCSIAuthRef targetAuth = iSCSIAuthCreateNone();

    errno_t error = iSCSISessionLogin(managerRef,target,portal,initiatorAuth,targetAuth,
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
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    error = iSCSIHBAInterfaceSend(hbaInterface,sessionId,connectionId,(iSCSIPDUInitiatorBHS *)&cmd,data,length);
    
    iSCSIPDUDataRelease(&data);
    CFRelease(textCmd);
     
    if(error)
    {
        enum iSCSILogoutStatusCode statusCode;
        iSCSISessionLogout(managerRef,sessionId,&statusCode);
        return error;
    }
    
    // Get response from iSCSI portal, continue until response is complete
    iSCSIPDUTextRspBHS rsp;
    
    *discoveryRec = iSCSIDiscoveryRecCreateMutable();

    do {
        if((error = iSCSIHBAInterfaceReceive(hbaInterface,sessionId,connectionId,(iSCSIPDUTargetBHS *)&rsp,&data,&length)))
        {
            iSCSIPDUDataRelease(&data);

            enum iSCSILogoutStatusCode statusCode;
            iSCSISessionLogout(managerRef,sessionId,&statusCode);

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
    iSCSISessionLogout(managerRef,sessionId,&logoutStatusCode);

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
errno_t iSCSIQueryTargetForAuthMethod(iSCSISessionManagerRef managerRef,
                                      iSCSIPortalRef portal,
                                      CFStringRef targetIQN,
                                      enum iSCSIAuthMethods * authMethod,
                                      enum iSCSILoginStatusCode * statusCode)
{
    if(!portal || !authMethod)
        return EINVAL;

    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    errno_t error = 0;
    
    // Resolve information about the target
    struct sockaddr_storage ssTarget, ssHost;
    
    if((error = iSCSIUtilsGetAddressForPortal(portal,&ssTarget,&ssHost)))
        return error;
    
    // Create a discovery session to the portal
    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetIQN(target,targetIQN);
    
    // Create session (incl. qualifier) and a new connection (incl. Id)
    // Reset qualifier and connection ID by default
    SessionIdentifier sessionId;
    ConnectionIdentifier connectionId;
    error = iSCSIHBAInterfaceCreateSession(hbaInterface,
                                           targetIQN,
                                     iSCSIPortalGetAddress(portal),
                                     iSCSIPortalGetPort(portal),
                                     iSCSIPortalGetHostInterface(portal),
                                     &ssTarget,
                                     &ssHost,
                                     &sessionId,
                                     &connectionId);
    
    // If no error, authenticate (negotiate security parameters)
    if(!error)
        error = iSCSIAuthInterrogate(managerRef,
                                     target,
                                     sessionId,
                                     connectionId,
                                     authMethod,
                                     statusCode);

    iSCSIHBAInterfaceReleaseSession(hbaInterface,sessionId);
    iSCSITargetRelease(target);
    
    return error;
}

/*! Gets the session identifier associated with the specified target.
 *  @param targetIQN the name of the target.
 *  @return the session identiifer. */
SessionIdentifier iSCSISessionGetSessionIdForTarget(iSCSISessionManagerRef managerRef,
                                             CFStringRef targetIQN)
{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    return iSCSIHBAInterfaceGetSessionIdForTargetIQN(hbaInterface,targetIQN);
}

/*! Gets the connection identifier associated with the specified portal.
 *  @param sessionId the session identifier.
 *  @param portal the portal connected on the specified session.
 *  @return the associated connection identifier. */
ConnectionIdentifier iSCSISessionGetConnectionIdForPortal(iSCSISessionManagerRef managerRef,
                                                   SessionIdentifier sessionId,
                                                   iSCSIPortalRef portal)
{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    return iSCSIHBAInterfaceGetConnectionIdForPortalAddress(hbaInterface,sessionId,iSCSIPortalGetAddress(portal));
}

/*! Gets an array of session identifiers for each session.
 *  @param sessionIds an array of session identifiers.
 *  @return an array of session identifiers. */
CFArrayRef iSCSISessionCopyArrayOfSessionIds(iSCSISessionManagerRef managerRef)
{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    SessionIdentifier sessionIds[kiSCSIMaxSessions];
    UInt16 sessionCount;

    if(iSCSIHBAInterfaceGetSessionIds(hbaInterface,sessionIds,&sessionCount))
        return NULL;
    
    return CFArrayCreate(kCFAllocatorDefault,(const void **) sessionIds,sessionCount,NULL);
}

/*! Gets an array of connection identifiers for each session.
 *  @param sessionId session identifier.
 *  @return an array of connection identifiers. */
CFArrayRef iSCSISessionCopyArrayOfConnectionIds(iSCSISessionManagerRef managerRef,
                                            SessionIdentifier sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    ConnectionIdentifier connectionIds[kiSCSIMaxConnectionsPerSession];
    UInt32 connectionCount;
    
    if(iSCSIHBAInterfaceGetConnectionIds(hbaInterface,sessionId,connectionIds,&connectionCount))
        return NULL;

    return CFArrayCreate(kCFAllocatorDefault,(const void **)&connectionIds,connectionCount,NULL);
}

/*! Creates a target object for the specified session.
 *  @param sessionId the session identifier.
 *  @return target the target object. */
iSCSITargetRef iSCSISessionCopyTargetForId(iSCSISessionManagerRef managerRef,SessionIdentifier sessionId)
{
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    CFStringRef targetIQN = iSCSIHBAInterfaceCreateTargetIQNForSessionId(hbaInterface,sessionId);
    
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
iSCSIPortalRef iSCSISessionCopyPortalForConnectionId(iSCSISessionManagerRef managerRef,
                                                SessionIdentifier sessionId,
                                                ConnectionIdentifier connectionId)
                                      
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    CFStringRef address,port,hostInterface;
    
    if(!(address = iSCSIHBAInterfaceCreatePortalAddressForConnectionId(hbaInterface,sessionId,connectionId)))
        return NULL;

    if(!(port = iSCSIHBAInterfaceCreatePortalPortForConnectionId(hbaInterface,sessionId,connectionId)))
    {
        CFRelease(address);
        return NULL;
    }
    
    if(!(hostInterface = iSCSIHBAInterfaceCreateHostInterfaceForConnectionId(hbaInterface,sessionId,connectionId)))
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
CFDictionaryRef iSCSISessionCopyCFPropertiesForTarget(iSCSISessionManagerRef managerRef,
                                                  iSCSITargetRef target)
{
    if(!target)
        return NULL;

    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    CFDictionaryRef dictionary = NULL;
    
    SessionIdentifier sessionId = iSCSISessionGetSessionIdForTarget(managerRef,iSCSITargetGetIQN(target));
    
    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;
    
    // Get session options from kernel
    UInt32 paramVal32 = 0;
    
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOMaxConnections,&paramVal32,sizeof(paramVal32));
    CFNumberRef maxConnections = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOMaxBurstLength,&paramVal32,sizeof(paramVal32));
    CFNumberRef maxBurstLength = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOFirstBurstLength,&paramVal32,sizeof(paramVal32));
    CFNumberRef firstBurstLength = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOMaxOutstandingR2T,&paramVal32,sizeof(paramVal32));
    CFNumberRef maxOutStandingR2T = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASODefaultTime2Retain,&paramVal32,sizeof(paramVal32));
    CFNumberRef defaultTime2Retain = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);
    
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASODefaultTime2Wait,&paramVal32,sizeof(paramVal32));
    CFNumberRef defaultTime2Wait = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);
    

    TargetPortalGroupTag tpgt = 0;
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOTargetPortalGroupTag,&tpgt,sizeof(TargetPortalGroupTag));
    CFNumberRef targetPortalGroupTag = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt16Type,&tpgt);

    TargetSessionIdentifier tsih = 0;
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOTargetSessionId,&tsih,sizeof(TargetSessionIdentifier));
    CFNumberRef targetSessionId = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt16Type,&tsih);

    UInt8 paramVal8 = 0;
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOErrorRecoveryLevel,&paramVal8,sizeof(paramVal8));
    CFNumberRef errorRecoveryLevel = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt8Type,&paramVal8);

    CFNumberRef sessionIdentifier = CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt16Type,&sessionId);
    
    CFStringRef initialR2T = kRFC3720_Value_No;
    CFStringRef immediateData = kRFC3720_Value_No;
    CFStringRef dataPDUInOrder = kRFC3720_Value_No;
    CFStringRef dataSequenceInOrder = kRFC3720_Value_No;

    Boolean paramValBool = false;
    
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOImmediateData,&paramValBool,sizeof(Boolean));
    if(paramValBool)
        immediateData = kRFC3720_Value_Yes;

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOInitialR2T,&paramValBool,sizeof(Boolean));
    if(paramValBool)
        initialR2T = kRFC3720_Value_Yes;

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASODataPDUInOrder,&paramValBool,sizeof(Boolean));
    if(paramValBool)
        dataPDUInOrder = kRFC3720_Value_Yes;

    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASODataSequenceInOrder,&paramValBool,sizeof(Boolean));
    if(paramValBool)
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
CFDictionaryRef iSCSISessionCopyCFPropertiesForPortal(iSCSISessionManagerRef managerRef,
                                                     iSCSITargetRef target,
                                                     iSCSIPortalRef portal)
{
    if(!target || !portal)
        return NULL;

    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    CFDictionaryRef dictionary = NULL;

    SessionIdentifier sessionId = iSCSISessionGetSessionIdForTarget(managerRef,iSCSITargetGetIQN(target));
    ConnectionIdentifier connectionId = kiSCSIInvalidConnectionId;

    if(sessionId == kiSCSIInvalidSessionId)
        return NULL;

    // Validate connection identifier
    connectionId = iSCSISessionGetConnectionIdForPortal(managerRef,sessionId,portal);
    if(connectionId == kiSCSIInvalidConnectionId)
        return NULL;
    
    // Get connection options from kernel

    UInt32 paramVal32 = 0;
    iSCSIHBAInterfaceGetConnectionParameter(hbaInterface,sessionId,connectionId,kiSCSIHBACOMaxRecvDataSegmentLength,&paramVal32,sizeof(paramVal32));
    CFNumberRef maxRecvDataSegmentLength = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&paramVal32);
    
    CFNumberRef connectionIdentifier = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&connectionId);
    

    enum iSCSIDigestTypes dataDigestType = kiSCSIDigestNone;
    enum iSCSIDigestTypes headerDigestType = kiSCSIDigestNone;

    Boolean paramValBool = false;
    
    iSCSIHBAInterfaceGetConnectionParameter(hbaInterface,sessionId,connectionId,kiSCSIHBACOUseDataDigest,&paramValBool,sizeof(Boolean));
    if(paramValBool)
        dataDigestType = kiSCSIDigestCRC32C;

    iSCSIHBAInterfaceGetConnectionParameter(hbaInterface,sessionId,connectionId,kiSCSIHBACOUseHeaderDigest,&paramValBool,sizeof(Boolean));
    if(paramValBool)
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


