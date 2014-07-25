/*!
 * @author		Nareg Sinenian
 * @file		iSCSIAuth.h
 * @date		April 14, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI authentication functions.  This library
 *              depends on the user-space iSCSI PDU library and augments the
 *              session library by providing authentication for the target
 *              and the initiator.
 */

#ifndef __ISCSI_AUTHMETHOD_H__
#define __ISCSI_AUTHMETHOD_H__

#include <CoreFoundation/CoreFoundation.h>
#include <CommonCrypto/CommonDigest.h>

struct __iSCSIAuthMethod;
typedef struct __iSCSIAuthMethod * iSCSIAuthMethodRef;

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
                                       CFStringRef targetSecret);

/*! Releases an authentication method block, freeing associated resources.
 *  @param auth the authentication method block to release. */
void iSCSIAuthRelease(iSCSIAuthMethodRef auth);

#endif
