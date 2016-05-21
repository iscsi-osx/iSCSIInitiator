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

#ifndef __ISCSI_AUTH_RIGHTS_H__
#define __ISCSI_AUTH_RIGHTS_H__

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPreferences.h>

#include <Security/Security.h>

enum iSCSIAuthRights {
    
    /*! The right to login and logout. */
    kiSCSIAuthLoginRight,
    
    /*! The right to make modifications to the initiator, including
     *  modification of settings, addition/removal of targets, etc. */
    kiSCSIAuthModifyRight,
    
    /*! All authentication rights. */
    kiSCSIAuthAllRights
};

/*! Creates all necessary rights if they are missing.
 *  @param authorization authorization used to create rights. */
OSStatus iSCSIAuthRightsInitialize(AuthorizationRef authorization);

/*! Used to acquire a right.
 *  @param authorization the authorization to associated with the acquired right.
 *  @param authRight the right to acquire.
 *  @return an status code indicating the result of the operation. */
OSStatus iSCSIAuthRightsAcquire(AuthorizationRef authorization,enum iSCSIAuthRights authRight);


#endif /* __ISCSI_AUTH_RIGHTS_H__ */
