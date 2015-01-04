/*!
 * @author		Nareg Sinenian
 * @file		iSCSIAuth.c
 * @date		April 14, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI authentication functions.  This library
 *              depends on the user-space iSCSI PDU library and augments the
 *              session library by providing authentication for the target
 *              and the initiator.
 */

#include "iSCSIAuth.h"
#include "iSCSIPDUUser.h"
#include "iSCSISession.h"
#include "iSCSIKernelInterface.h"


/*! Defined by the session layer and used during authentication here. */
extern unsigned int kiSCSISessionMaxTextKeyValuePairs;
extern errno_t iSCSISessionLoginQuery(UInt16 sessionId,
                               UInt32 connectionId,
                               iSCSISessionOptions * sessionOptions,
                               enum iSCSIPDULoginStages currentStage,
                               enum iSCSIPDULoginStages nextStage,
                               CFDictionaryRef   textCmd,
                               CFMutableDictionaryRef  textRsp);

extern CFStringRef kiSCSIInitiatorName;
extern CFStringRef kiSCSIInitiatorAlias;

// Literals used for initial authentication step
CFStringRef kiSCSILKInitiatorName = CFSTR("InitiatorName");
CFStringRef kiSCSILKInitiatorAlias = CFSTR("InitiatorAlias");
CFStringRef kiSCSILKTargetName = CFSTR("TargetName");
CFStringRef kiSCSILKTargetAlias = CFSTR("TargetAlias");

// Literals used to indicate session type
CFStringRef kiSCSILKSessionType = CFSTR("SessionType");
CFStringRef kiSCSILVSessionTypeDiscovery = CFSTR("Discovery");
CFStringRef kiSCSILVSessionTypeNormal = CFSTR("Normal");

// Literals used to indicate different authentication methods
CFStringRef kiSCSILKAuthMethod = CFSTR("AuthMethod");
CFStringRef kiSCSILVAuthMethodAll = CFSTR("None,CHAP,KRB5,SPKM1,SPKM2,SRP");
CFStringRef kiSCSILVAuthMethodNone = CFSTR("None");
CFStringRef kiSCSILVAuthMethodCHAP = CFSTR("CHAP");

// Literals used during CHAP authentication
CFStringRef kiSCSILKAuthCHAPDigest = CFSTR("CHAP_A");
CFStringRef kiSCSILVAuthCHAPDigestMD5 = CFSTR("5");
CFStringRef kiSCSILKAuthCHAPId = CFSTR("CHAP_I");
CFStringRef kiSCSILKAuthCHAPChallenge = CFSTR("CHAP_C");
CFStringRef kiSCSILKAuthCHAPResponse = CFSTR("CHAP_R");
CFStringRef kiSCSILKAuthCHAPName = CFSTR("CHAP_N");

// Used for grouping connections together (multiple connections must have the
// same group tag or authentication will fail).
CFStringRef kiSCSILKTargetPortalGroupTag = CFSTR("TargetPortalGroupTag");


/*! Authentication methods to be used by the authentication method struct. */
enum iSCSIAuthMethods {
    
    /*! No authentication. */
    kiSCSIAuthNone = 0,
    
    /*! CHAP authentication. */
    kiSCSIAuthCHAP = 1,
};


/*! Struct used during the authentication process. */
typedef struct __iSCSIAuthMethod
{
    /*! The authentication type. */
    enum iSCSIAuthMethods authMethod;
    
} iSCSIAuthMethod;


/*! Struct used for CHAP authentication.  The target secret and user name
 *  are mandatory and are used by the target to authenticate the initiator.
 *  The initiator fields may be left blank (empty string) in which case the 
 *  target will authenticate the initiator, and the target won't be
 *  authenticated by the initiator. */
typedef struct __iSCSIAuthMethodCHAP
{
    /*! The authentication method (this is always kiSCSIAuthCHAP). */
    enum iSCSIAuthMethods authMethod;
    
    /*! Target password used to authenticate initiator (required). */
    CFStringRef targetSecret;
    
    /*! Target user name used to authenticate initiator (required). */
    CFStringRef targetUser;
    
    /*! Initiator password used to authenticate target (optional). */
    CFStringRef initiatorSecret;
    
    /*! Initiator user name used to authenticate target  (optional). */
    CFStringRef initiatorUser;

    
} iSCSIAuthMethodCHAP;


/*! Creates an authentication method block for use with CHAP.  If both secrets
 *  are used, two-way authentication is used.  Otherwise, 1-way authentication
 *  is used depending on which secret is omitted.  To omitt a secret, pass in
 *  an emptry string for either the user or the secret.
 *  @param initiatorUser the initiator user name (used to authenticate target)
 *  @param initiatorSecret the initiator secret  (used to authenticate target)
 *  @param targetUser the target user name (used to authenticate initiator)
 *  @param targetSecret the target secret  (used to authenticate initiator)
 *  @return an authentication method block for CHAP. */
iSCSIAuthMethodRef iSCSIAuthCreateCHAP(CFStringRef initiatorUser,
                                       CFStringRef initiatorSecret,
                                       CFStringRef targetUser,
                                       CFStringRef targetSecret)
{
    if(!targetUser || !targetSecret)
        return NULL;
    
    iSCSIAuthMethodCHAP * authMeth;
    authMeth = (iSCSIAuthMethodCHAP *)malloc(sizeof(iSCSIAuthMethodCHAP));
    
    authMeth->authMethod = kiSCSIAuthCHAP;
    
    if(initiatorUser && initiatorSecret) {
        CFRetain(initiatorUser);
        CFRetain(initiatorSecret);
        
        authMeth->initiatorUser   = initiatorUser;
        authMeth->initiatorSecret = initiatorSecret;
    }
    else {
        authMeth->initiatorUser = NULL;
        authMeth->initiatorSecret = NULL;
    }
    
    CFRetain(targetUser);
    CFRetain(targetSecret);

    authMeth->targetUser    = targetUser;
    authMeth->targetSecret  = targetSecret;
    
    return (iSCSIAuthMethodRef)authMeth;
}

/*! Releases an authentication method block, freeing associated resources.
 *  @param auth the authentication method block to release. */
void iSCSIAuthRelease(iSCSIAuthMethodRef auth)
{
    if(!auth)
        return;

    // Release CF objects as required
    if(auth->authMethod == kiSCSIAuthCHAP) {
        iSCSIAuthMethodCHAP * authMethod = (iSCSIAuthMethodCHAP *)auth;
        
        CFRelease(authMethod->targetUser);
        CFRelease(authMethod->targetSecret);
        
        if(authMethod->initiatorUser)
            CFRelease(authMethod->initiatorUser);
        
        if(authMethod->initiatorSecret)
            CFRelease(authMethod->initiatorSecret);
    }
    free(auth);
}


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
errno_t iSCSIAuthNegotiateCHAP(iSCSITarget * const target,
                               UInt16 sessionId,
                               UInt32 connectionId,
                               iSCSISessionOptions * sessionOptions)
{
    if(!target || !sessionOptions || sessionId == kiSCSIInvalidConnectionId ||
       connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;
    
    iSCSIAuthMethodCHAP * authMethod = (iSCSIAuthMethodCHAP*)target->authMethod;
    
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

    errno_t error = iSCSISessionLoginQuery(sessionId,
                                           connectionId,
                                           sessionOptions,
                                           kiSCSIPDUSecurityNegotiation,
                                           kiSCSIPDUSecurityNegotiation,
                                           authCmd,authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(error) {
        CFRelease(authCmd);
        CFRelease(authRsp);
        return error;
    }
    
    CFDictionaryRemoveAllValues(authCmd);
    
    // Get identifier and challenge & calculate the response
    CFStringRef identifier, challenge;
    
    if(CFDictionaryGetValueIfPresent(authRsp,kiSCSILKAuthCHAPId,(void*)&identifier) &&
       CFDictionaryGetValueIfPresent(authRsp,kiSCSILKAuthCHAPChallenge,(void*)&challenge))
    {
        CFStringRef response = iSCSIAuthNegotiateCHAPCreateResponse(identifier,
                                             authMethod->targetSecret,
                                             challenge);
        
        // Send back our name and response
        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPResponse,response);
        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPName,authMethod->targetUser);
        
        // Dictionary retains response, we can release it
        CFRelease(response);
    }
    
    // If we must authenticate the target, generate id, challenge & send
    if(authMethod->initiatorUser && authMethod->initiatorSecret) {
        
        // Generate an identifier & challenge
        identifier = iSCSIAuthNegotiateCHAPCreateId();
        challenge = iSCSIAuthNegotiateCHAPCreateChallenge();

        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPId,identifier);
        CFDictionaryAddValue(authCmd,kiSCSILKAuthCHAPChallenge,challenge);
    }
    
    error = iSCSISessionLoginQuery(sessionId,connectionId,sessionOptions,
                                   kiSCSIPDUSecurityNegotiation,
                                   kiSCSIPDULoginOperationalNegotiation,
                                   authCmd,authRsp);
    
    // Now perform target authentication (we authenticate target)
    if(authMethod->initiatorUser && authMethod->initiatorSecret)
    {
        // Calculate the response we expect to get
        CFStringRef expResponse = iSCSIAuthNegotiateCHAPCreateResponse(
                        identifier,authMethod->initiatorSecret,challenge);
    
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

void iSCSIAuthNegotiateBuildDict(const iSCSITarget * target,
                                 CFMutableDictionaryRef authCmd)
{
    if(target->targetName == NULL)
        CFDictionaryAddValue(authCmd,kiSCSILKSessionType,kiSCSILVSessionTypeDiscovery);
    else {
        CFDictionaryAddValue(authCmd,kiSCSILKSessionType,kiSCSILVSessionTypeNormal);
        CFDictionaryAddValue(authCmd,kiSCSILKTargetName,target->targetName);
    }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorName,kiSCSIInitiatorName);
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorAlias,kiSCSIInitiatorAlias);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Add authentication key(s) to dictionary
    CFStringRef authMeth = kiSCSILVAuthMethodNone;
    
    if(target->authMethod) {
        if(target->authMethod->authMethod)
            authMeth = kiSCSILVAuthMethodCHAP;
    }
    
    CFDictionaryAddValue(authCmd,kiSCSILKAuthMethod,authMeth);
}

/*! Helper function.  Called by session or connection creation functions to
 *  begin authentication between the initiator and a selected target. */
errno_t iSCSIAuthNegotiate(iSCSITarget * const target,
                           UInt16 sessionId,
                           UInt32 connectionId,
                           iSCSISessionOptions * sessionOptions)
{
    if(!target || !sessionOptions || sessionId == kiSCSIInvalidConnectionId ||
       connectionId == kiSCSIInvalidConnectionId)
        return EINVAL;

    // Setup dictionary with target and initiator info for authentication
    CFMutableDictionaryRef authCmd = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    iSCSIAuthNegotiateBuildDict(target,authCmd);
    
    enum iSCSIPDULoginStages nextStage = kiSCSIPDUSecurityNegotiation;
    
    // If no authentication is required, move to next stage
    if(!target->authMethod)
        nextStage = kiSCSIPDULoginOperationalNegotiation;
    
    errno_t error = iSCSISessionLoginQuery(sessionId,
                                           connectionId,
                                           sessionOptions,
                                           kiSCSIPDUSecurityNegotiation,
                                           nextStage,
                                           authCmd,authRsp);
    
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
    
    // Ensure that the target returned a portal group tag (TPGT)...
    if(!CFDictionaryContainsKey(authRsp,kiSCSILKTargetPortalGroupTag)) {
        error = EAUTH;
        goto ERROR_TPGT_MISSING;
    }

    // Extract target portal group tag
    CFStringRef TPGT = (CFStringRef)CFDictionaryGetValue(authRsp,kiSCSILKTargetPortalGroupTag);
    
    // If this is leading login (TSIH = 0 for leading login), store TPGT
    if(sessionOptions->TSIH != 0)
        sessionOptions->TPGT = CFStringGetIntValue(TPGT);
    // Otherwise compare TPGT...
    else {
        if(sessionOptions->TPGT != CFStringGetIntValue(TPGT))
            goto ERROR_AUTHENTICATION;
    }
    
    // Call the appropriate authentication function to proceed
    if(target->authMethod) {
        enum iSCSIAuthMethods authMethod = (enum iSCSIAuthMethods)(target->authMethod->authMethod);
        
        if(authMethod == kiSCSIAuthCHAP)
        {
            if((error = iSCSIAuthNegotiateCHAP(target,sessionId,connectionId,sessionOptions)))
                goto ERROR_AUTHENTICATE_CHAP;
        }
        
        
        
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
errno_t iSCSIAuthInterrogate(iSCSITarget * const target,
                             UInt16 sessionId,
                             UInt32 connectionId,
                             iSCSISessionOptions * sessionOptions,
                             CFStringRef * authMethods)
{
    if(sessionId == kiSCSIInvalidSessionId || connectionId == kiSCSIInvalidConnectionId ||
       !target || !sessionOptions || !authMethods)
        return EINVAL;
    
    // Setup dictionary with target and initiator info for authentication
    CFMutableDictionaryRef authCmd = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    CFDictionaryAddValue(authCmd,kiSCSILKSessionType,kiSCSILVSessionTypeNormal);
    CFDictionaryAddValue(authCmd,kiSCSILKTargetName,target->targetName);

    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorName,kiSCSIInitiatorName);
    CFDictionaryAddValue(authCmd,kiSCSILKInitiatorAlias,kiSCSIInitiatorAlias);
    CFDictionaryAddValue(authCmd,kiSCSILKAuthMethod,kiSCSILVAuthMethodAll);

    // Setup dictionary to receive authentication response
    CFMutableDictionaryRef authRsp = CFDictionaryCreateMutable(
        kCFAllocatorDefault,kiSCSISessionMaxTextKeyValuePairs,
        &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    // Query target with all possible authentication options
    errno_t error = iSCSISessionLoginQuery(sessionId,
                                           connectionId,
                                           sessionOptions,
                                           kiSCSIPDUSecurityNegotiation,
                                           kiSCSIPDUSecurityNegotiation,
                                           authCmd,authRsp);
    
    // Quit if the query failed for whatever reason, release dictionaries
    if(!error) {
        // Grab authentication method that the target chose, if available
        if(CFDictionaryContainsKey(authRsp, kiSCSILKAuthMethod))
        {
            *authMethods = CFDictionaryGetValue(authRsp,kiSCSILKAuthMethod);
            CFRetain(*authMethods);
        }
        // Otherwise the target didn't return an "AuthMethod" key, this means
        // that it doesn't require authentication
        else
            *authMethods = CFStringCreateCopy(kCFAllocatorDefault,kiSCSILVAuthMethodNone);
    }
    
    CFRelease(authCmd);
    CFRelease(authRsp);
    
    return error;
}


