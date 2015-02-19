/*!
 * @author		Nareg Sinenian
 * @file		iSCSIAuth.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI authentication functions.  This library
 *              depends on the user-space iSCSI PDU library and augments the
 *              session library by providing authentication for the target
 *              and the initiator.
 */

#ifndef __ISCSI_AUTHMETHOD_H__
#define __ISCSI_AUTHMETHOD_H__

#include <CoreFoundation/CoreFoundation.h>
#include <CommonCrypto/CommonDigest.h>

#include "iSCSITypes.h"
#include "iSCSIKernelInterfaceShared.h"

/*! Authentication function defined in the authentication module
 *  (in the file iSCSIAuth.h). */
errno_t iSCSIAuthNegotiate(iSCSITargetRef target,
                           iSCSIAuthRef auth,
                           SID sessionId,
                           CID connectionId,
                           iSCSISessionOptions * sessionOptions,
                           enum iSCSILoginStatusCode * statusCode);

/*! Authentication function defined in the authentication module
 *  (in the file iSCSIAuth.h). */
errno_t iSCSIAuthInterrogate(iSCSITargetRef target,
                             SID sessionId,
                             CID connectionId,
                             iSCSISessionOptions * sessionOptions,
                             enum iSCSIAuthMethods * authMethod,
                             enum iSCSILoginStatusCode * statusCode);

#endif
