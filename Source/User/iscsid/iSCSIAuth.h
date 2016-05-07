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

#ifndef __ISCSI_AUTHMETHOD_H__
#define __ISCSI_AUTHMETHOD_H__

#include <CoreFoundation/CoreFoundation.h>
#include <CommonCrypto/CommonDigest.h>

#include "iSCSITypes.h"
#include "iSCSIKernelInterfaceShared.h"

/*! Authentication function defined in the authentication module
 *  (in the file iSCSIAuth.h). */
errno_t iSCSIAuthNegotiate(iSCSIMutableTargetRef target,
                           iSCSIAuthRef initiatorAuth,
                           iSCSIAuthRef targetAuth,
                           SID sessionId,
                           CID connectionId,
                           enum iSCSILoginStatusCode * statusCode);

/*! Authentication function defined in the authentication module
 *  (in the file iSCSIAuth.h). */
errno_t iSCSIAuthInterrogate(iSCSITargetRef target,
                             SID sessionId,
                             CID connectionId,
                             enum iSCSIAuthMethods * authMethod,
                             enum iSCSILoginStatusCode * statusCode);

#endif
