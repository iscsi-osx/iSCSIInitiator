/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDiscovery.h
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		Discovery functions for use by iscsid.
 */


#ifndef __ISCSI_DISCOVERY_H__
#define __ISCSI_DISCOVERY_H__

#include <CoreFoundation/CoreFoundation.h>

#include "iSCSISession.h"
#include "iSCSITypes.h"
#include "iSCSIPropertyList.h"

errno_t iSCSIDiscoveryRunSendTargets();

#endif /* defined(__ISCSI_DISCOVERY_H__) */
