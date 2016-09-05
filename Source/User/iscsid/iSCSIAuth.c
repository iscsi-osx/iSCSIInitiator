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

#include "iSCSIAuth.h"
#include "iSCSIPDUUser.h"
#include "iSCSISession.h"
#include "iSCSIHBAInterface.h"
#include "iSCSIQueryTarget.h"


/*! Defined by the session layer and used during authentication here. */
extern unsigned int kiSCSISessionMaxTextKeyValuePairs;

extern CFStringRef kiSCSIInitiatorIQN;
extern CFStringRef kiSCSIInitiatorAlias;

/*! Helper function.  Create a byte array (CFDataRef object) that holds the
 *  value represented by the hexidecimal string. Handles strings with or
 *  without a 0x prefix. */
CFDataRef CFDataCreateWithHexString(CFStringRef hexString)
{
    if(!hexString)
        return NULL;

    // Get length and pointer to hex string
    CFIndex hexStringLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(hexString),kCFStringEncodingASCII) + sizeof('\0');
    char hexStringBuffer[hexStringLength];
    CFStringGetCString(hexString,hexStringBuffer,hexStringLength,kCFStringEncodingASCII);

    // Byte length stars off as the number of hex characters, and is adjusted
    // to reflect the number of bytes depending on the format of the hex string
    // (does not include null terminator)
    CFIndex byteLength = CFStringGetLength(hexString);

    // The index we'll start processing the hex string
    unsigned int startIndex = 0;
    
    // Check for the "0x" or "x" prefix, ignore it if present...
    if(hexStringLength >= 2 && hexStringBuffer[0] == '0' && hexStringBuffer[1] == 'x') {
        startIndex+=2;
        byteLength-=2;
    }
    else if(hexStringLength >= 1 && hexStringBuffer[0] == 'x') {
        startIndex++;
        byteLength--;
    }

    // At this point we have the number of hex characters (ignoring prefix,
    // if present).  Divide by two and take ceiling to get length
    byteLength = ceil(byteLength / 2.0);

    // Keeps track of which byte we're processing in the byte array
    unsigned int byteIdx = 0;

    CFMutableDataRef data  = CFDataCreateMutable(kCFAllocatorDefault,byteLength);
    CFDataSetLength(data,byteLength);
    UInt8 * bytes = CFDataGetMutableBytePtr(data);
    int buffer = 0;

    // If an odd number of hex characters, process first one differently...
    if(hexStringLength % 2 != 0) {
        // Pick off the first character and convert differently
        sscanf(&hexStringBuffer[startIndex],"%01x",&buffer);
        bytes[byteIdx] = buffer;
        startIndex++;
        byteIdx++;
    }

    // Process remaining characters in pairs (2 hex characters = 1 byte)
    for(unsigned int idx = startIndex; idx < hexStringLength; idx+=2) {
        sscanf(&hexStringBuffer[idx],"%02x",&buffer);
        bytes[byteIdx] = buffer;
        byteIdx++;
    }
    
    return data;
}

/*! Helper function.  Creates a CFString object that holds the hexidecimal
 *  representation of the values contained in the byte array.  Use CFRelease()
 *  to free the CFString object created by this function (this follows the
 *  Core Foundation "Create" rule). */
CFStringRef CreateHexStringWithBytes(const UInt8 * bytes, size_t length)
{
    // Pad string by 3 bytes to leave room for "0x" prefix and null terminator
    const long hexStrLength = length * 2 + 3;
    
    char hexStr[hexStrLength];
    hexStr[0] = '0';
    hexStr[1] = 'x';
    
    // Print byte array into a hex string
    for(unsigned int i = 0; i < length; i++)
        sprintf(&hexStr[i*2+2], "%02x", bytes[i]);
    
    // Null terminate
    hexStr[hexStrLength-1] = 0;
    
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
    CFIndex secretLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(secret),kCFStringEncodingASCII) + sizeof('\0');
    char secretBuffer[secretLength];
    CFStringGetCString(secret,secretBuffer,secretLength,kCFStringEncodingASCII);
    
    CC_MD5_Update(&md5,secretBuffer,(CC_LONG)CFStringGetLength(secret));

    // Hash in the challenge
    CFDataRef challengeData = CFDataCreateWithHexString(challenge);

    CC_MD5_Update(&md5,CFDataGetBytePtr(challengeData),(CC_LONG)CFDataGetLength(challengeData));

    // Finalize and get the hash string
    CC_MD5_Final(md5Hash,&md5);

    CFRelease(challengeData);
    
    return CreateHexStringWithBytes(md5Hash,CC_MD5_DIGEST_LENGTH);
}

CFStringRef iSCSIAuthNegotiateCHAPCreateChallenge()
{
    // Open /dev/random and read 16 bytes (CHAP standard)
    const unsigned int challengeLength = 16;
    UInt8 challenge[challengeLength];

    FILE * fRandom = fopen("/dev/random","r");
    
    for(unsigned int i = 0; i < challengeLength; i++)
        challenge[i] = fgetc(fRandom);
    
    fclose(fRandom);
    
    return CreateHexStringWithBytes(challenge,challengeLength);
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
errno_t iSCSIAuthNegotiateCHAP(iSCSISessionManagerRef managerRef,
                               iSCSIMutableTargetRef target,
                               iSCSIAuthRef initiatorAuth,
                               iSCSIAuthRef targetAuth,
                               SessionIdentifier sessionId,
                               ConnectionIdentifier connectionId,
                               TargetSessionIdentifier targetSessionId,
                               enum iSCSILoginStatusCode * statusCode)
{
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
    CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthCHAPDigest,kRFC3720_Value_AuthCHAPDigestMD5);
    
    struct iSCSILoginQueryContext context;
    context.interface       = iSCSISessionManagerGetHBAInterface(managerRef);
    context.sessionId       = sessionId;
    context.connectionId    = connectionId;
    context.targetSessionId = targetSessionId;
    context.nextStage       = kiSCSIPDUSecurityNegotiation;
    context.currentStage    = kiSCSIPDUSecurityNegotiation;
    
    enum iSCSIPDURejectCode rejectCode;

    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           authCmd,authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(error || *statusCode != kiSCSILoginSuccess) {
        CFRelease(authCmd);
        CFRelease(authRsp);
        return error;
    }

    CFDictionaryRemoveAllValues(authCmd);
    
    // Get CHAP parameters
    CFStringRef targetUser = NULL, targetSecret = NULL;
    CFStringRef initiatorUser = NULL, initiatorSecret = NULL;
    iSCSIAuthGetCHAPValues(initiatorAuth,&initiatorUser,&initiatorSecret);
    iSCSIAuthGetCHAPValues(targetAuth,&targetUser,&targetSecret);
    
    // Get identifier and challenge & calculate the response
    CFStringRef identifier = NULL, challenge = NULL;
    
    if(initiatorUser && initiatorSecret)
    {
        if(CFDictionaryGetValueIfPresent(authRsp,kRFC3720_Key_AuthCHAPId,(void*)&identifier) &&
           CFDictionaryGetValueIfPresent(authRsp,kRFC3720_Key_AuthCHAPChallenge,(void*)&challenge))
        {
            CFStringRef response = iSCSIAuthNegotiateCHAPCreateResponse(identifier,initiatorSecret,challenge);
        
            // Send back our name and response
            CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthCHAPResponse,response);
            CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthCHAPName,initiatorUser);

            // Dictionary retains response, we can release it
            CFRelease(response);
        }
    }

    // If we must authenticate the target, generate id, challenge & send
    if(targetUser && targetSecret) {
        
        // Generate an identifier & challenge
        identifier = iSCSIAuthNegotiateCHAPCreateId();
        challenge = iSCSIAuthNegotiateCHAPCreateChallenge();

        CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthCHAPId,identifier);
        CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthCHAPChallenge,challenge);
    }

    context.nextStage = kiSCSIPDULoginOperationalNegotiation;

    error = iSCSISessionLoginQuery(&context,
                                   statusCode,
                                   &rejectCode,
                                   authCmd,authRsp);
    
    if(*statusCode != kiSCSILoginSuccess)
        error = EAUTH;
    
    // If target authenticated us successfully, perform target authentication (we authenticate target)
    if(!error && targetUser && targetSecret)
    {
        // Calculate the response we expect to get
        CFStringRef expResponse = iSCSIAuthNegotiateCHAPCreateResponse(
                                        identifier,targetSecret,challenge);
        if(expResponse) {
            // We don't need these after calculating the response
            CFRelease(identifier);
            CFRelease(challenge);

            // Compare to the actual received response
            CFStringRef response;
            if(CFDictionaryGetValueIfPresent(authRsp,kRFC3720_Key_AuthCHAPResponse,(void*)&response))
            {
                if(CFStringCompare(response,expResponse,kCFCompareCaseInsensitive) != kCFCompareEqualTo)
                    error = EAUTH;
            }
            else
                error = EAUTH;
            
            CFRelease(expResponse);
        }
    }
    
    // If no error and the target returned an alias save it...
    CFStringRef targetAlias;
    if(!error && CFDictionaryGetValueIfPresent(authRsp,kRFC3720_Key_TargetAlias,(const void **)&targetAlias)) {
        iSCSITargetSetAlias(target,targetAlias);
    }
    
    CFRelease(authCmd);
    CFRelease(authRsp);

    return error;
}

void iSCSIAuthNegotiateBuildDict(iSCSITargetRef target,
                                 iSCSIAuthRef initiatorAuth,
                                 iSCSIAuthRef targetAuth,
                                 CFMutableDictionaryRef authCmd)
{
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    if(CFStringCompare(targetIQN,kiSCSIUnspecifiedTargetIQN,0) == kCFCompareEqualTo)
        CFDictionaryAddValue(authCmd,kRFC3720_Key_SessionType,kRFC3720_Value_SessionTypeDiscovery);
    else {
        CFDictionaryAddValue(authCmd,kRFC3720_Key_SessionType,kRFC3720_Value_SessionTypeNormal);
        CFDictionaryAddValue(authCmd,kRFC3720_Key_TargetName,iSCSITargetGetIQN(target));
    }

    // Read global variables for initiator name & alias and add them to dict.
    CFDictionaryAddValue(authCmd,kRFC3720_Key_InitiatorName,kiSCSIInitiatorIQN);
    CFDictionaryAddValue(authCmd,kRFC3720_Key_InitiatorAlias,kiSCSIInitiatorAlias);

    // Determine authentication method used and add to dictionary
    enum iSCSIAuthMethods initiatorAuthMethod = iSCSIAuthGetMethod(initiatorAuth);
    enum iSCSIAuthMethods targetAuthMethod = iSCSIAuthGetMethod(targetAuth);
    
    // Add authentication key(s) to dictionary
    if(initiatorAuthMethod == kiSCSIAuthMethodCHAP) {

        // Unidirectional CHAP (target authenticates initiator)
        if(targetAuthMethod == kiSCSIAuthMethodNone) {
            // In case the target doesn't wish to authenticate us, we can
            // include an additional option of no authentication.
            const CFTypeRef values[] = {
                kRFC3720_Value_AuthMethodNone,
                kRFC3720_Value_AuthMethodCHAP
            };

            CFArrayRef methods = CFArrayCreate(kCFAllocatorDefault,
                                               (const void**)values,sizeof(values)/sizeof(CFTypeRef),
                                               &kCFTypeArrayCallBacks);

            CFStringRef methodStrings = CFStringCreateByCombiningStrings(
                kCFAllocatorDefault,methods,CFSTR(","));
            CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthMethod,methodStrings);
            CFRelease(methodStrings);
            CFRelease(methods);
        }
        // Bidirectional CHAP
        else {
            // Here we insist that we authenticate the target and therefore
            // no authentication is not an option.
            CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthMethod,kRFC3720_Value_AuthMethodCHAP);
        }
    }
    else
        CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthMethod,kRFC3720_Value_AuthMethodNone);
}

/*! Helper function.  Called by session or connection creation functions to
 *  begin authentication between the initiator and a selected target.  If the
 *  target name is set to blank (e.g., by a call to iSCSITargetSetIQN()) or 
 *  never set at all, a discovery session is assumed for authentication. */
errno_t iSCSIAuthNegotiate(iSCSISessionManagerRef managerRef,
                           iSCSIMutableTargetRef target,
                           iSCSIAuthRef initiatorAuth,
                           iSCSIAuthRef targetAuth,
                           SessionIdentifier sessionId,
                           ConnectionIdentifier connectionId,
                           enum iSCSILoginStatusCode * statusCode)
{
    iSCSIHBAInterfaceRef hbaInterface = iSCSISessionManagerGetHBAInterface(managerRef);
    
    // Setup dictionary with target and initiator info for authentication
    CFMutableDictionaryRef authCmd = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    iSCSIAuthNegotiateBuildDict(target,initiatorAuth,targetAuth,authCmd);
    
    struct iSCSILoginQueryContext context;
    context.interface    = hbaInterface;
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDUSecurityNegotiation;
    context.nextStage    = kiSCSIPDUSecurityNegotiation;
    
    // Retrieve the TSIH from the kernel
    TargetSessionIdentifier targetSessionId = 0;
    iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOTargetSessionId,
                                         &targetSessionId,sizeof(TargetSessionIdentifier));
    context.targetSessionId = targetSessionId;
    
    enum iSCSIPDURejectCode rejectCode;
    
    // If no authentication is supported (only value we sent was "None"), move on to the next stage
    if(iSCSIAuthGetMethod(initiatorAuth) == kiSCSIAuthMethodNone)
        context.nextStage = kiSCSIPDULoginOperationalNegotiation;
    
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           authCmd,
                                           authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(error || *statusCode != kiSCSILoginSuccess)
        goto ERROR_GENERIC;
    
    // This was the first query of the connection; record the status
    // sequence number provided by the target
    UInt32 expStatSN = context.statSN + 1;
    iSCSIHBAInterfaceSetConnectionParameter(hbaInterface,sessionId,connectionId,kiSCSIHBACOInitialExpStatSN,
                                            &expStatSN,sizeof(expStatSN));
    
    // If this is not a discovery session (the target is not specified for discovery),
    // we expect to receive a target portal group tag (TPGT) and validate it
    if(CFStringCompare(iSCSITargetGetIQN(target),kiSCSIUnspecifiedTargetIQN,0) != kCFCompareEqualTo)
    {
        // Ensure that the target returned a portal group tag (TPGT)...
        if(!CFDictionaryContainsKey(authRsp,kRFC3720_Key_TargetPortalGroupTag)) {
            error = EAUTH;
            goto ERROR_TPGT_MISSING;
        }

        // Extract target portal group tag
        CFStringRef targetPortalGroupRsp = (CFStringRef)CFDictionaryGetValue(authRsp,kRFC3720_Key_TargetPortalGroupTag);
        
        // If this is leading login (TSIH = 0 for leading login), store TPGT,
        // else compare it to the TPGT that we have stored for this session...
        if(targetSessionId == 0) {
            TargetPortalGroupTag targetPortalGroupTag = CFStringGetIntValue(targetPortalGroupRsp);
            
            iSCSIHBAInterfaceSetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOTargetPortalGroupTag,
                                                 &targetPortalGroupTag,sizeof(TargetPortalGroupTag));
        }
        else {
            // Retrieve from kernel
            TargetPortalGroupTag targetPortalGroupTag = 0;
            iSCSIHBAInterfaceGetSessionParameter(hbaInterface,sessionId,kiSCSIHBASOTargetPortalGroupTag,
                                                 &targetPortalGroupTag,sizeof(TargetPortalGroupTag));

            // Validate existing group against TPGT for this connection
            if(targetPortalGroupTag != CFStringGetIntValue(targetPortalGroupRsp))
                goto ERROR_AUTHENTICATION;
        }
    }
    
    // Determine if target supports desired authentication method. Desired method could
    // be a comma-separated list in authCmd, so we check the target's desired method
    // as specified in the response (authRsp) against our list
    CFRange result = CFStringFind(CFDictionaryGetValue(authCmd,kRFC3720_Key_AuthMethod),
                                  CFDictionaryGetValue(authRsp,kRFC3720_Key_AuthMethod),
                                  kCFCompareCaseInsensitive);
    
    // Check if target supported our desired authentication method
    if(result.location == kCFNotFound) {
        error = EAUTH;
        goto ERROR_AUTHENTICATION;
    }
    
    // Get authentication method from response string. We can't rely
    // on the value we specified to the target because for initiator CHAP
    // authenticaiton we always supply a no authentication option in addition
    // to CHAP (just because we offer authentication does not mean the target
    // is obliged to use it). This tests whether the target chose to use it.
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodNone;
    CFStringRef authValue = CFDictionaryGetValue(authRsp,kRFC3720_Key_AuthMethod);
    
    // Target chose to go with CHAP; since we supply a list of options including None,
    // we need to confirm that we're okay with the target's selection to move on...
    if(CFStringCompare(authValue,kRFC3720_Value_AuthMethodCHAP,0) == kCFCompareEqualTo)
    {
        authMethod = kiSCSIAuthMethodCHAP;
    }
    // Target chose to go with None (no authentication)
    else if(CFStringCompare(authValue,kRFC3720_Value_AuthMethodNone,0) == kCFCompareEqualTo)
    {
        // If we originally offered an authentication method and target chose none,
        // we will have to send a PDU to transition to the next stage...
        if(iSCSIAuthGetMethod(initiatorAuth) != kiSCSIAuthMethodNone) {
            context.nextStage = kiSCSIPDULoginOperationalNegotiation;
            iSCSISessionLoginQuery(&context,statusCode,&rejectCode,NULL,NULL);
        }
    }

    if(authMethod == kiSCSIAuthMethodCHAP) {
        error = iSCSIAuthNegotiateCHAP(managerRef,
                                       target,
                                       initiatorAuth,
                                       targetAuth,
                                       sessionId,
                                       connectionId,
                                       targetSessionId,
                                       statusCode);
        if(error || *statusCode != kiSCSILoginSuccess)
            goto ERROR_AUTHENTICATE_CHAP;
    }
    
    // If no error and the target returned an alias save it...
    CFStringRef targetAlias;
    if(!error && CFDictionaryGetValueIfPresent(authRsp,kRFC3720_Key_TargetAlias,(const void**)&targetAlias)) {
        iSCSITargetSetAlias(target,targetAlias);
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
errno_t iSCSIAuthInterrogate(iSCSISessionManagerRef managerRef,
                             iSCSITargetRef target,
                             SessionIdentifier sessionId,
                             ConnectionIdentifier connectionId,
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
    
    CFDictionaryAddValue(authCmd,kRFC3720_Key_SessionType,kRFC3720_Value_SessionTypeNormal);
    CFDictionaryAddValue(authCmd,kRFC3720_Key_TargetName,iSCSITargetGetIQN(target));
    
    CFDictionaryAddValue(authCmd,kRFC3720_Key_InitiatorName,kiSCSIInitiatorIQN);
    CFDictionaryAddValue(authCmd,kRFC3720_Key_InitiatorAlias,kiSCSIInitiatorAlias);
    CFDictionaryAddValue(authCmd,kRFC3720_Key_AuthMethod,kRFC3720_Value_AuthMethodAll);

    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    struct iSCSILoginQueryContext context;
    context.interface    = iSCSISessionManagerGetHBAInterface(managerRef);
    context.sessionId    = sessionId;
    context.connectionId = connectionId;
    context.currentStage = kiSCSIPDUSecurityNegotiation;
    context.nextStage    = kiSCSIPDUSecurityNegotiation;
    context.targetSessionId = 0;
    
    enum iSCSIPDURejectCode rejectCode;
    
    // Query target with all possible authentication options
    errno_t error = iSCSISessionLoginQuery(&context,
                                           statusCode,
                                           &rejectCode,
                                           authCmd,authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(!error && *statusCode == kiSCSILoginSuccess) {
        // Grab authentication method that the target chose, if available
        if(CFDictionaryContainsKey(authRsp, kRFC3720_Key_AuthMethod))
        {
            CFStringRef method = CFDictionaryGetValue(authRsp,kRFC3720_Key_AuthMethod);
            if(CFStringCompare(method,kRFC3720_Value_AuthMethodCHAP,0) == kCFCompareEqualTo)
                *authMethod = kiSCSIAuthMethodCHAP;
            else if(CFStringCompare(method,kRFC3720_Value_AuthMethodNone,0) == kCFCompareEqualTo)
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


