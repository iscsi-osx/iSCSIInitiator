/*!
 * @author		Nareg Sinenian
 * @file		iSCSISession.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI session management functions.  This library
 *              depends on the user-space iSCSI PDU library to login, logout
 *              and perform discovery functions on iSCSI target nodes.  It 
 *              also relies on the kernel layer for access to kext.
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
    
    CFDictionaryAddValue(sessCmd,kRFC3720_Key_InitialR2T,kRFC3720_Value_No);
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

errno_t iSCSINegotiateParseSWDictCommon(CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp,
                                        iSCSIKernelSessionCfg * sessCfgKernel)

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
        
        sessCfgKernel->defaultTime2Retain = iSCSILVGetMin(initCmd,targetRsp);;
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
        
        sessCfgKernel->defaultTime2Wait = iSCSILVGetMin(initCmd,targetRsp);;
    }
    else
        return ENOTSUP;

    // Grab minimum value of error recovery level
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_ErrorRecoveryLevel,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_ErrorRecoveryLevel);
        UInt32 errorRecoveryLevel = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(errorRecoveryLevel,kRFC3720_ErrorRecoveryLevel_Min,kRFC3720_ErrorRecoveryLevel_Max))
            return ENOTSUP;
        
        sessCfgKernel->errorRecoveryLevel = iSCSILVGetMin(initCmd,targetRsp);
    }
    else
        return ENOTSUP;
    
    // Success
    return 0;
}

errno_t iSCSINegotiateParseSWDictNormal(CFDictionaryRef sessCmd,
                                        CFDictionaryRef sessRsp,
                                        iSCSIKernelSessionCfg * sessCfgKernel)

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
        
        sessCfgKernel->maxConnections = iSCSILVGetMin(initCmd,targetRsp);
    }

    // Grab the OR for initialR2T command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_InitialR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_InitialR2T);
        sessCfgKernel->initialR2T = iSCSILVGetOr(initCmd,targetRsp);
    }

    
    // Grab the AND for immediate data command and response
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_ImmediateData,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_ImmediateData);
        sessCfgKernel->immediateData = iSCSILVGetAnd(initCmd,targetRsp);
    }


    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DataPDUInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DataPDUInOrder);
        sessCfgKernel->dataPDUInOrder = iSCSILVGetAnd(initCmd,targetRsp);
    }
    
    // Get the OR of data PDU in order
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_DataSequenceInOrder,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_DataSequenceInOrder);
        sessCfgKernel->dataSequenceInOrder = iSCSILVGetAnd(initCmd,targetRsp);
    }

    // Grab minimum of max burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxBurstLength);
        sessCfgKernel->maxBurstLength = iSCSILVGetMin(initCmd,targetRsp);
    }

    // Grab minimum of first burst length
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_FirstBurstLength,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_FirstBurstLength);
        UInt32 firstBurstLength = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(firstBurstLength,kRFC3720_FirstBurstLength_Min,kRFC3720_FirstBurstLength_Max))
            return ENOTSUP;
        
        sessCfgKernel->firstBurstLength = iSCSILVGetMin(initCmd,targetRsp);
    }

    // Grab minimum of max outstanding R2T
    if(CFDictionaryGetValueIfPresent(sessRsp,kRFC3720_Key_MaxOutstandingR2T,(void*)&targetRsp))
    {
        CFStringRef initCmd = CFDictionaryGetValue(sessCmd,kRFC3720_Key_MaxOutstandingR2T);
        UInt32 maxOutStandingR2T = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxOutStandingR2T,kRFC3720_MaxOutstandingR2T_Min,kRFC3720_MaxOutstandingR2T_Max))
            return ENOTSUP;
        
        sessCfgKernel->maxOutStandingR2T = iSCSILVGetMin(initCmd,targetRsp);
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
errno_t iSCSINegotiateParseCWDict(CFDictionaryRef connCmd,
                                  CFDictionaryRef connRsp,
                                  iSCSIKernelConnectionCfg * connCfgKernel)

{
    // Holds target value & comparison result for keys that we'll process
    CFStringRef targetRsp;
    
    // Flag that indicates that target and initiator agree on a parameter
    Boolean agree = false;
    
    // Get data digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_DataDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kRFC3720_Key_DataDigest),targetRsp);
    
    // If we wanted to use data digest and target didn't set connInfo
    connCfgKernel->useDataDigest = agree && iSCSILVGetEqual(targetRsp,kRFC3720_Value_DataDigestCRC32C);
    
    // Reset agreement flag
    agree = false;
    
    // Get header digest key and compare to requested value
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_HeaderDigest,(void*)&targetRsp))
        agree = iSCSILVGetEqual(CFDictionaryGetValue(connCmd,kRFC3720_Key_HeaderDigest),targetRsp);
    
    // If we wanted to use header digest and target didn't set connInfo
    connCfgKernel->useHeaderDigest = agree && iSCSILVGetEqual(targetRsp,kRFC3720_Value_HeaderDigestCRC32C);
    
    // This option is declarative; we sent the default length, and the target
    // must accept our choice as it is within a valid range
    connCfgKernel->maxRecvDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    // This is the declaration made by the target as to the length it can
    // receive.  Accept the value if it is within the RFC3720 allowed range
    // otherwise, terminate the connection...
    if(CFDictionaryGetValueIfPresent(connRsp,kRFC3720_Key_MaxRecvDataSegmentLength,(void*)&targetRsp))
    {
        UInt32 maxSendDataSegmentLength = CFStringGetIntValue(targetRsp);
        
        // Range-check value...
        if(iSCSILVRangeInvalid(maxSendDataSegmentLength,kRFC3720_MaxRecvDataSegmentLength_Min,kRFC3720_MaxRecvDataSegmentLength_Max))
            return ENOTSUP;
        
        connCfgKernel->maxSendDataSegmentLength = maxSendDataSegmentLength;
    }
    else // If the target doesn't explicitly declare this, use the default
        connCfgKernel->maxSendDataSegmentLength = kRFC3720_MaxRecvDataSegmentLength;
    
    // Success
    return 0;
}

errno_t iSCSINegotiateSession(iSCSITargetRef target,
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
    
    // Get the kernel session & connection configuration so that negotiation
    // can update the values
    iSCSIKernelSessionCfg sessCfgKernel;
    iSCSIKernelConnectionCfg connCfgKernel;
    iSCSIKernelGetSessionConfig(sessionId,&sessCfgKernel);
    iSCSIKernelGetConnectionConfig(sessionId,connectionId,&connCfgKernel);
    
    struct iSCSILoginQueryContext context;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDULoginOperationalNegotiation;
    context.nextStage    = kiSCSIPDUFullFeaturePhase;
    context.targetSessionId = sessCfgKernel.targetSessionId;
    
    enum iSCSIRejectCode rejectCode;
    
    // Send session-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           sessCmd,sessRsp);
    
    // Parse dictionaries and store session parameters if no I/O error occured
    if(*statusCode == kiSCSILoginSuccess) {
        
        // The TSIH was recorded by iSCSISessionLoginQuery since we're
        // entering the full-feature phase (see iSCSISessionLoginQuery documentation)
        sessCfgKernel.targetSessionId = context.targetSessionId;

        if(!error)
            error = iSCSINegotiateParseSWDictCommon(sessCmd,sessRsp,&sessCfgKernel);
    
        if(!error && iSCSITargetGetIQN(target) != NULL)
            error = iSCSINegotiateParseSWDictNormal(sessCmd,sessRsp,&sessCfgKernel);
    
        if(!error)
            error = iSCSINegotiateParseCWDict(sessCmd,sessRsp,&connCfgKernel);
    }
    
    // Update the kernel session & connection configuration
    iSCSIKernelSetSessionConfig(sessionId,&sessCfgKernel);
    iSCSIKernelSetConnectionConfig(sessionId,connectionId,&connCfgKernel);
    
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
    
    // Get the kernel session & connection configuration so that negotiation
    // can update the values
    iSCSIKernelSessionCfg sessCfgKernel;
    iSCSIKernelConnectionCfg connCfgKernel;
    iSCSIKernelGetSessionConfig(sessionId,&sessCfgKernel);
    iSCSIKernelGetConnectionConfig(sessionId,connectionId,&connCfgKernel);

    struct iSCSILoginQueryContext context;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDULoginOperationalNegotiation;
    context.nextStage    = kiSCSIPDULoginOperationalNegotiation;
    context.targetSessionId = sessCfgKernel.targetSessionId;

    // If the target session ID is non-zero, we're simply adding a new
    // connection and we can enter the full feature after this negotiation.
    if(sessCfgKernel.targetSessionId != 0)
        context.nextStage = kiSCSIPDUFullFeaturePhase;

    enum iSCSIRejectCode rejectCode;

    // Send connection-wide options to target and retreive a response dictionary
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           connCmd,
                                           connRsp);

    // If no error, parse received dictionary and store connection options
    if(!error && *statusCode == kiSCSILoginSuccess)
        error = iSCSINegotiateParseCWDict(connCmd,connRsp,&connCfgKernel);

    // Update the kernel connection configuration
    iSCSIKernelSetConnectionConfig(sessionId,connectionId,&connCfgKernel);

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
    
    // Grab options related to this connection
    iSCSIKernelConnectionCfg connOpts;
    errno_t error;
    
    if((error = iSCSIKernelGetConnectionConfig(sessionId,connectionId,&connOpts)))
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
    
    iSCSITargetRef target = iSCSICreateTargetForSessionId(sessionId);
    
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

/*! Prepares the active sessions in the kernel for a sleep event.  After the
 *  system wakes up, the function iSCSIRestoreForSystemWake() should be
 *  called before using any other functions.  Failure to do so may lead to
 *  undefined behavior.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIPrepareForSystemSleep()
{
// TODO: finish implementing this function, verify, test
    
    CFArrayRef sessionIds = iSCSICreateArrayOfSessionIds();
    
    if(!sessionIds)
        return 0;
    
    CFIndex sessionCount = CFArrayGetCount(sessionIds);

    // Unmount all disk drives associated with each session
    for(CFIndex idx = 0; idx < sessionCount; idx++) {
        SID sessionId = (SID)CFArrayGetValueAtIndex(sessionIds,idx);
        
        // Unmount all media for this session
        iSCSITargetRef target = iSCSICreateTargetForSessionId(sessionId);
        iSCSIDAUnmountIOMediaForTarget(iSCSITargetGetIQN(target));

        iSCSIKernelDeactivateAllConnections(sessionId);
    }
    
    CFRelease(sessionIds);
    return 0;
}

/*! Restores iSCSI sessions after a system has been woken up.  Before
 *  sleeping, the function iSCSIPrepareForSleep() must have been called.
 *  Otherwise, the behavior of this function is undefined.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIRestoreForSystemWake()
{
// TODO:


    return 0;
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
errno_t iSCSILoginSession(iSCSITargetRef target,
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

    // Unmount all media for this session
    iSCSITargetRef target = iSCSICreateTargetForSessionId(sessionId);

    // No need to unmount media if this was a discovery session
    if(target && CFStringCompare(iSCSITargetGetIQN(target),kiSCSIUnspecifiedTargetIQN,0) != kCFCompareEqualTo)
        iSCSIDAUnmountIOMediaForTarget(iSCSITargetGetIQN(target));
    
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
    cmd.targetTransferTag = 0xFFFFFFFF;
     
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
    
    iSCSIKernelSessionCfg sessCfgKernel;
    
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

    if(!error)
        error = iSCSIKernelGetSessionConfig(sessionId,&sessCfgKernel);
    
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

    if(sessionId != kiSCSIInvalidSessionId)
    {
        struct iSCSIKernelSessionCfg config;
        iSCSIKernelGetSessionConfig(sessionId,&config);
        

        CFNumberRef maxConnections = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberIntType,&config.maxConnections);

        CFNumberRef maxBurstLength = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberIntType,&config.maxBurstLength);

        CFNumberRef firstBurstLength = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberIntType,&config.firstBurstLength);

        CFNumberRef maxOutStandingR2T = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberIntType,&config.maxOutStandingR2T);

        CFNumberRef targetPortalGroupTag = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberSInt16Type,&config.targetPortalGroupTag);

        CFNumberRef targetSessionId = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberSInt16Type,&config.targetSessionId);

        CFNumberRef defaultTime2Retain = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberIntType,&config.defaultTime2Retain);

        CFNumberRef defaultTime2Wait = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberIntType,&config.defaultTime2Wait);

        CFNumberRef errorRecoveryLevel = CFNumberCreate(
            kCFAllocatorDefault,kCFNumberSInt8Type,&config.errorRecoveryLevel);


        CFNumberRef sessionIdentifier = CFNumberCreate(
                    kCFAllocatorDefault,kCFNumberIntType,&sessionId);
        

        CFStringRef initialR2T = kRFC3720_Value_No;
        CFStringRef immediateData = kRFC3720_Value_No;
        CFStringRef dataPDUInOrder = kRFC3720_Value_No;
        CFStringRef dataSequenceInOrder = kRFC3720_Value_No;

        if(config.initialR2T)
            initialR2T = kRFC3720_Value_Yes;

        if(config.immediateData)
            immediateData = kRFC3720_Value_Yes;

        if(config.dataPDUInOrder)
            dataPDUInOrder = kRFC3720_Value_Yes;

        if(config.dataSequenceInOrder)
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
    }

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

    if(sessionId != kiSCSIInvalidSessionId)
    {
        connectionId = iSCSIGetConnectionIdForPortal(sessionId,portal);
        if(connectionId != kiSCSIInvalidConnectionId)
        {
            struct iSCSIKernelConnectionCfg config;
            iSCSIKernelGetConnectionConfig(sessionId,connectionId,&config);

            CFNumberRef maxRecvDataSegmentLength = CFNumberCreate(
                kCFAllocatorDefault,kCFNumberIntType,&config.maxRecvDataSegmentLength);
            
            CFNumberRef connectionIdentifier = CFNumberCreate(
                kCFAllocatorDefault,kCFNumberIntType,&connectionId);



            enum iSCSIDigestTypes dataDigestType = kiSCSIDigestNone;
            enum iSCSIDigestTypes headerDigestType = kiSCSIDigestNone;

            if(config.useDataDigest)
                dataDigestType = kiSCSIDigestCRC32C;

            if(config.useHeaderDigest)
                headerDigestType = kiSCSIDigestCRC32C;

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
        }
    }
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

void iSCSISessionHandleNotifications(enum iSCSIKernelNotificationTypes type,
                                     iSCSIKernelNotificationMessage * msg)
{
// TODO: implement this function to handle async PDUs (these are handled
// in user-space).  They might involve dropped connections, etc., which may
// need to be handled differently depending on error recovery levels
// (Note: kernel should handle async SCSI event, this is for iSCSI events only
//  see RFC3720 for more inforation).

    // Process an asynchronous message
    if(type == kiSCSIKernelNotificationAsyncMessage)
    {
        iSCSIKernelNotificationAsyncMessage * asyncMsg = (iSCSIKernelNotificationAsyncMessage *)msg;
     
        // If the asynchronous message is invalid ignore it
        enum iSCSIPDUAsyncMsgEvent asyncEvent = (enum iSCSIPDUAsyncMsgEvent)asyncMsg->asyncEvent;
        
        fprintf(stderr,"Async event occured");
    }
}


/*! Call to initialize iSCSI session management functions.  This function will
 *  initialize the kernel layer after which other session-related functions
 *  may be called.
 *  @param rl the runloop to use for executing session-related functions.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIInitialize(CFRunLoopRef rl)
{
    errno_t error = iSCSIKernelInitialize(&iSCSISessionHandleNotifications);

    CFRunLoopSourceRef source = iSCSIKernelCreateRunLoopSource();
//    CFRunLoopAddSource(rl,source,kCFRunLoopDefaultMode);
    
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
