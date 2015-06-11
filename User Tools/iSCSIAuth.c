/*!
 * @author		Nareg Sinenian
 * @file		iSCSIAuth.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI authentication functions.  This library
 *              depends on the user-space iSCSI PDU library and augments the
 *              session library by providing authentication for the target
 *              and the initiator.
 */

#include "iSCSIAuth.h"
#include "iSCSIPDUUser.h"
#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"
#include "iSCSIQueryTarget.h"


/*! Defined by the session layer and used during authentication here. */
extern unsigned int kiSCSISessionMaxTextKeyValuePairs;

extern CFStringRef kiSCSIInitiatorIQN;
extern CFStringRef kiSCSIInitiatorAlias;

/*! Helper function.  Create a null-terminated byte array that holds the
 *  value represented by the hexidecimal string. Handles strings with or
 *  without a 0x prefix.  Use free() to free the allocated byte array. */
size_t CreateByteArrayFromHexString(CFStringRef hexStr,UInt8 * * bytes)
{
    if(!hexStr || !bytes)
        return 0;
    
    long hexStrLen = CFStringGetLength(hexStr);
    const char * hexStrPtr = CFStringGetCStringPtr(hexStr,kCFStringEncodingASCII);
    
    size_t byteLength = hexStrLen;
    unsigned int startIndex = 0;
    
    // Check for the "0x" prefix
    if(hexStrLen >= 2 && hexStrPtr[0] == '0' && hexStrPtr[1] == 'x') {
        startIndex+=2;
        byteLength-=2;
    }
    else if(hexStrLen >= 1 && hexStrPtr[0] == 'x') {
        startIndex++;
        byteLength--;
    }
    
    unsigned int byteIdx = 0;
    
    // Account for odd number of hex characters (leading zero omitted)
    if(byteLength % 2 != 0) {
        byteLength++;
        byteLength /= 2;
        *bytes = (UInt8 *)malloc(byteLength);
        
        // Pick off the first character and convert differently
        sscanf(&hexStrPtr[startIndex],"%01x",&(*bytes)[byteIdx]);
        startIndex++;
        byteIdx++;
    }
    else {
        byteLength /= 2;
        *bytes = (UInt8 *)malloc(byteLength);
    }
    
    for(unsigned int idx = startIndex; idx < hexStrLen; idx+=2) {
        sscanf(&hexStrPtr[idx],"%02x",&(*bytes)[byteIdx]);
        byteIdx++;
    }
    return byteLength;
}

/*! Helper function.  Creates a CFString object that holds the hexidecimal
 *  representation of the values contained in the byte array.  Use CFRelease()
 *  to free the CFString object created by this function (this follows the
 *  Core Foundation "Create" rule). */
CFStringRef CreateHexStringFromByteArray(UInt8 * bytes, size_t length)
{
    // Pad string by 3 bytes to leave room for "0x" prefix and null terminator
    const long hexStrLen = length * 2 + 3;
    
    char hexStr[hexStrLen];
    hexStr[0] = '0';
    hexStr[1] = 'x';
    
    // Print byte array into a hex string
    for(unsigned int i = 0; i < length; i++)
        sprintf(&hexStr[i*2+2], "%02x", bytes[i]);
    
    // Null terminate
    hexStr[hexStrLen-1] = 0;
    
    // This copies our buffer into a CFString object
    return CFStringCreateWithCString(kCFAllocatorDefault,hexStr,kCFStringEncodingASCII);
}

/*! Helper function.  Creates a CHAP response from a given identifier, 
 *  secret and challenge (see RFC1994). Use CFRelease() to free the 
 *  returned CHAP response string. */
CFStringRef iSCSIAuthNegotiateCHAPCreateResponse(CFStringRef identifier,
                                                 CFStringRef secret,
                                                 CFStringRef challenge)
{
    // Holds our md5hash
    UInt8 md5Hash[CC_MD5_DIGEST_LENGTH];

    // Initialize md5 object
    CC_MD5_CTX md5;
    CC_MD5_Init(&md5);

    // Hash in the identifier
    UInt8 id = CFStringGetIntValue(identifier);
    CC_MD5_Update(&md5,&id,(CC_LONG)sizeof(id));

    // Hash in the secret
    const UInt8 * byteSecret = (const UInt8*)CFStringGetCStringPtr(secret,kCFStringEncodingASCII);
    CC_MD5_Update(&md5,byteSecret,(CC_LONG)CFStringGetLength(secret));

    // Hash in the challenge
    UInt8 * byteChallenge;
    size_t challengeLen = CreateByteArrayFromHexString(challenge,&byteChallenge);
    CC_MD5_Update(&md5,byteChallenge,(CC_LONG)challengeLen);

    // Finalize and get the hash string
    CC_MD5_Final(md5Hash,&md5);

    free(byteChallenge);

    return CreateHexStringFromByteArray(md5Hash,CC_MD5_DIGEST_LENGTH);
}

CFStringRef iSCSIAuthNegotiateCHAPCreateChallenge()
{
    // Open /dev/random and read 16 bytes
    const unsigned int challengeLength = 16;
    UInt8 challenge[challengeLength];

    FILE * fRandom = fopen("/dev/random","r");
    
    for(unsigned int i = 0; i < challengeLength; i++)
        challenge[i] = fgetc(fRandom);
    
    fclose(fRandom);
    
    return CreateHexStringFromByteArray(challenge,challengeLength);
}

CFStringRef iSCSIAuthNegotiateCHAPCreateId()
{
    // Open /dev/random and read a single byte
    FILE * fRandom = fopen("/dev/random","r");
    UInt8 id = fgetc(fRandom);
    fclose(fRandom);
    
    return CFStringCreateWithFormat(kCFAllocatorDefault,NULL,CFSTR("%d"),id);
}

/*! Helper function for iSCSIConnectionSecurityNegotiate.  Once it has been
 *  determined that a CHAP session is to be used, this function will perform
 *  the CHAP authentication. */
errno_t iSCSIAuthNegotiateCHAP(iSCSITargetRef target,
                               iSCSIAuthRef auth,
                               SID sessionId,
                               CID connectionId,
                               TSIH targetSessionId,
                               enum iSCSILoginStatusCode * statusCode)
{
    if(!target || !auth || sessionId == kiSCSIInvalidConnectionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    // Setup dictionary CHAP authentication information
    CFMutableDictionaryRef authCmd = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    if(!authCmd)
        return EINVAL;
    
    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    if(!authRsp) {
        CFRelease(authCmd);
        return EINVAL;
    }
    
    // Target must first offer the authentication method (5 = MD5)
    // This key starts authentication process - target authenticates us
    CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPDigest,kiSCSILVAuthCHAPDigestMD5);

    
    struct iSCSILoginQueryContext context;
    context.sessionId       = sessionId;
    context.connectionId    = connectionId;
    context.targetSessionId = targetSessionId;
    context.nextStage       = kiSCSIPDUSecurityNegotiation;
    context.currentStage    = kiSCSIPDUSecurityNegotiation;
    
    enum iSCSIRejectCode rejectCode;

    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           authCmd,authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(error) {
        CFRelease(authCmd);
        CFRelease(authRsp);
        return error;
    }

    CFDictionaryRemoveAllValues(authCmd);
    
    // Get CHAP parameters
    CFStringRef targetUser,targetSecret,initiatorUser,initiatorSecret;
    iSCSIAuthGetCHAPValues(auth,&targetUser,&targetSecret,&initiatorUser,&initiatorSecret);
    
    // Get identifier and challenge & calculate the response
    CFStringRef identifier = NULL, challenge = NULL;
    
    if(CFDictionaryGetValueIfPresent(authRsp,kiSCSILKAuthCHAPId,(void*)&identifier) &&
       CFDictionaryGetValueIfPresent(authRsp,kiSCSILKAuthCHAPChallenge,(void*)&challenge))
    {
        CFStringRef response = iSCSIAuthNegotiateCHAPCreateResponse(identifier,targetSecret,challenge);
        
        // Send back our name and response
        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPResponse,response);
        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPName,targetUser);
        
        // Dictionary retains response, we can release it
        CFRelease(response);
    }
    
    // If we must authenticate the target, generate id, challenge & send
    if(initiatorUser && initiatorSecret) {
        
        // Generate an identifier & challenge
        identifier = iSCSIAuthNegotiateCHAPCreateId();
        challenge = iSCSIAuthNegotiateCHAPCreateChallenge();

        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPId,identifier);
        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPChallenge,challenge);
    }
    
    context.nextStage = kiSCSIPDULoginOperationalNegotiation;

    error = iSCSISessionLoginQuery(&context,
                                   statusCode,
                                   &rejectCode,
                                   authCmd,authRsp);
    
    // Now perform target authentication (we authenticate target)
    if(initiatorUser && initiatorSecret)
    {
        // Calculate the response we expect to get
        CFStringRef expResponse = iSCSIAuthNegotiateCHAPCreateResponse(
                        identifier,initiatorSecret,challenge);
    
        // We don't need these after calculating the response
        CFRelease(identifier);
        CFRelease(challenge);
        
        // Compare to the actual received response
        CFStringRef response;
        if(CFDictionaryGetValueIfPresent(authRsp,kiSCSILKAuthCHAPResponse,(void*)&response))
        {
            if(CFStringCompare(response,expResponse,kCFCompareCaseInsensitive) != kCFCompareEqualTo)
                error = EAUTH;
        }
        else
            error = EAUTH;
        
        CFRelease(expResponse);
    }
    
    CFRelease(authCmd);
    CFRelease(authRsp);
    return error;
}

void iSCSIAuthNegotiateBuildDict(iSCSITargetRef target,
                                 iSCSIAuthRef auth,
                                 CFMutableDictionaryRef authCmd)
{
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    if(CFStringCompare(targetIQN,kiSCSIUnspecifiedTargetIQN,0) == kCFCompareEqualTo)
        CFDictionaryAddValue(authCmd,kiSCSILKSessionType,kiSCSILVSessionTypeDiscovery);
    else {
        CFDictionaryAddValue(authCmd,kiSCSILKSessionType,kiSCSILVSessionTypeNormal);
        CFDictionaryAddValue(authCmd,kiSCSILKTargetName,iSCSITargetGetIQN(target));
    }

    // Read global variables for initiator name & alias and add them to dict.
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorName,kiSCSIInitiatorIQN);
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorAlias,kiSCSIInitiatorAlias);

    // Determine authentication method used and add to dictionary
    enum iSCSIAuthMethods authMethod = iSCSIAuthGetMethod(auth);
    
    // Add authentication key(s) to dictionary
    CFStringRef authMeth = kiSCSILVAuthMethodNone;
    
    if(authMethod == kiSCSIAuthMethodCHAP)
            authMeth = kiSCSILVAuthMethodCHAP;
    
    CFDictionaryAddValue(authCmd,kiSCSILKAuthMethod,authMeth);
}

/*! Helper function.  Called by session or connection creation functions to
 *  begin authentication between the initiator and a selected target.  If the
 *  target name is set to blank (e.g., by a call to iSCSITargetSetName()) or 
 *  never set at all, a discovery session is assumed for authentication. */
errno_t iSCSIAuthNegotiate(iSCSITargetRef target,
                           iSCSIAuthRef auth,
                           SID sessionId,
                           CID connectionId,
                           enum iSCSILoginStatusCode * statusCode)
{
    if(!target || !auth || sessionId == kiSCSIInvalidConnectionId || connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;

    // Setup dictionary with target and initiator info for authentication
    CFMutableDictionaryRef authCmd = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    iSCSIAuthNegotiateBuildDict(target,auth,authCmd);
    
    struct iSCSILoginQueryContext context;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDUSecurityNegotiation;
    context.nextStage    = kiSCSIPDUSecurityNegotiation;
    
    // Retrieve the TSIH from the kernel
    iSCSIKernelSessionCfg sessCfgKernel;
    iSCSIKernelGetSessionConfig(sessionId,&sessCfgKernel);
    context.targetSessionId = sessCfgKernel.targetSessionId;
    
    enum iSCSIRejectCode rejectCode;
    
    // If no authentication is required, move to next stage
    if(iSCSIAuthGetMethod(auth) == kiSCSIAuthMethodNone)
        context.nextStage = kiSCSIPDULoginOperationalNegotiation;
    
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           authCmd,
                                           authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(error) {
        goto ERROR_GENERIC;
    }
    
    // Determine if target supports desired authentication method
    CFComparisonResult result;
    result = CFStringCompare(CFDictionaryGetValue(authRsp,kiSCSILKAuthMethod),
                             CFDictionaryGetValue(authCmd,kiSCSILKAuthMethod),
                             kCFCompareCaseInsensitive);
    
    // If we wanted to use a particular method and the target doesn't support it
    if(result != kCFCompareEqualTo) {
        error = EAUTH;
        goto ERROR_AUTHENTICATION;
    }
    
    
    // If this is not a discovery session, we expect to receive a target
    // portal group tag (TPGT)...
    if(CFStringCompare(iSCSITargetGetIQN(target),kiSCSIUnspecifiedTargetIQN,0) != kCFCompareEqualTo)
    {
        // Ensure that the target returned a portal group tag (TPGT)...
        if(!CFDictionaryContainsKey(authRsp,kiSCSILKTargetPortalGroupTag)) {
            error = EAUTH;
            goto ERROR_TPGT_MISSING;
        }

        // Extract target portal group tag
        CFStringRef TPGT = (CFStringRef)CFDictionaryGetValue(authRsp,kiSCSILKTargetPortalGroupTag);
        
        // If this is leading login (TSIH = 0 for leading login), store TPGT
        if(sessCfgKernel.targetSessionId == 0) {
            sessCfgKernel.targetPortalGroupTag = CFStringGetIntValue(TPGT);
            
            // Save the configuration since we've updated the TSIH
            iSCSIKernelSetSessionConfig(sessionId,&sessCfgKernel);
        }
        // Otherwise compare TPGT...
        else {
            if(sessCfgKernel.targetPortalGroupTag != CFStringGetIntValue(TPGT))
                goto ERROR_AUTHENTICATION;
        }
    }

    // Call the appropriate authentication function to proceed
    enum iSCSIAuthMethods authMethod = iSCSIAuthGetMethod(auth);
    
    if(authMethod == kiSCSIAuthMethodCHAP) {
        error = iSCSIAuthNegotiateCHAP(target,
                                       auth,
                                       sessionId,
                                       connectionId,
                                       sessCfgKernel.targetSessionId,
                                       statusCode);
        
        if(error)
            goto ERROR_AUTHENTICATE_CHAP;
    }
    
    CFRelease(authCmd);
    CFRelease(authRsp);
    
    return 0;
    
ERROR_AUTHENTICATE_CHAP:
    
ERROR_TPGT_MISSING:
    
ERROR_AUTHENTICATION:
    
ERROR_GENERIC:
    
    CFRelease(authCmd);
    CFRelease(authRsp);

    return error;
}


/*! Helper function.  Called by session or connection creation functions to
 *  determine available authentication options for a given target. */
errno_t iSCSIAuthInterrogate(iSCSITargetRef target,
                             SID sessionId,
                             CID connectionId,
                             enum iSCSIAuthMethods * authMethod,
                             enum iSCSILoginStatusCode * statusCode)
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId || !target || !authMethod)
        return EINVAL;
    
    *authMethod = kiSCSIAuthMethodInvalid;
    
    // Setup dictionary with target and initiator info for authentication
    CFMutableDictionaryRef authCmd = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    CFDictionaryAddValue(authCmd,kiSCSILKSessionType,kiSCSILVSessionTypeNormal);
    CFDictionaryAddValue(authCmd,kiSCSILKTargetName,iSCSITargetGetIQN(target));
    
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorName,kiSCSIInitiatorIQN);
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorAlias,kiSCSIInitiatorAlias);
    CFDictionaryAddValue(authCmd,kiSCSILKAuthMethod,kiSCSILVAuthMethodAll);

    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    struct iSCSILoginQueryContext context;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDUSecurityNegotiation;
    context.nextStage    = kiSCSIPDUSecurityNegotiation;
    context.targetSessionId = 0;
    
    enum iSCSIRejectCode rejectCode;
    
    // Query target with all possible authentication options
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           authCmd,authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(!error) {
        // Grab authentication method that the target chose, if available
        if(CFDictionaryContainsKey(authRsp, kiSCSILKAuthMethod))
        {
            CFStringRef method = CFDictionaryGetValue(authRsp,kiSCSILKAuthMethod);
            if(CFStringCompare(method,kiSCSILVAuthMethodCHAP,0) == kCFCompareEqualTo)
                *authMethod = kiSCSIAuthMethodCHAP;
            else if(CFStringCompare(method,kiSCSILVAuthMethodNone,0) == kCFCompareEqualTo)
                *authMethod = kiSCSIAuthMethodNone;
        }
        // Otherwise the target didn't return an "AuthMethod" key, this means
        // that it doesn't require authentication
        else
            *authMethod = kiSCSIAuthMethodNone;
    }
    
    CFRelease(authCmd);
    CFRelease(authRsp);
    
    return error;
}


