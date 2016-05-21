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

#include "iSCSIAuthRights.h"


/*! Authorization right for logging in and out of targets. */
const char kiSCSIAuthRightLogin[] = "com.github.iscsi-osx.iSCSIInitiator.login-logout";

/*! Authorization right for modifying initiator configuration (discovery, targets, etc). */
const char kiSCSIAuthRightModify[] = "com.github.iscsi-osx.iSCSIInitiator.modify-config";

CFStringRef kRightPromptLogin = CFSTR("For logging into and out of iSCSI targets.");

CFStringRef kRightPromptModify = CFSTR("For modifying initiator, discovery and target settings and adding and removing targets.");


/*! Creates all necessary rights if they are missing.
 *  @param authorization authorization used to create rights. */
OSStatus iSCSIAuthRightsInitialize(AuthorizationRef authorization)
{
    OSStatus error = noErr;
    
    // Login and logout right does not exist, create it
    if(AuthorizationRightGet(kiSCSIAuthRightLogin,NULL) != noErr)
        AuthorizationRightSet(authorization,kiSCSIAuthRightLogin,
                              CFSTR(kAuthorizationRuleClassAllow),kRightPromptLogin,
                              NULL,NULL);
    
    if(AuthorizationRightGet(kiSCSIAuthRightModify,NULL) != noErr)
        AuthorizationRightSet(authorization,kiSCSIAuthRightModify,
                              CFSTR(kAuthorizationRuleClassAllow),kRightPromptModify,
                              NULL,NULL);
    
    return error;
}


/*! Used to acquire a right.
 *  @param authorization the authorization to associated with the acquired right.
 *  @param authRight the right to acquire.
 *  @return an status code indicating the result of the operation. */
OSStatus iSCSIAuthRightsAcquire(AuthorizationRef authorization,enum iSCSIAuthRights authRight)
{
    const char * rightName;
    
    switch(authRight)
    {
        case kiSCSIAuthLoginRight:
            rightName = kiSCSIAuthRightLogin;
            break;
        case kiSCSIAuthModifyRight:
            rightName = kiSCSIAuthRightModify;
            break;
        default:
            return errAuthorizationCanceled;
    }
    
    AuthorizationItem actionRight = { rightName, 0, 0, 0 };
    AuthorizationRights rights = { 1, &actionRight };
    
    OSStatus error = AuthorizationCopyRights(authorization,&rights,NULL,kAuthorizationFlagExtendRights|kAuthorizationFlagInteractionAllowed,NULL);
    
    return error;
}
