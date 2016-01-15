/*!
 * @author		Nareg Sinenian
 * @file		iSCSITypesKernel
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 *
 *              iSCSI class names (follows the reverse DNS notation per
 *              Apple standards)
 */

#ifndef __ISCSI_KERNEL_CLASSES_H__
#define __ISCSI_KERNEL_CLASSES_H__

#define NAME_CONCAT(PREFIX, SUFFIX) PREFIX##_##SUFFIX
#define NAME_EVAL(PREFIX,SUFFIX)    NAME_CONCAT(PREFIX,SUFFIX)
#define ADD_PREFIX(NAME)            NAME_EVAL(NAME_PREFIX_U,NAME)

#define STRINGIFY(NAME)             #NAME
#define STRINGIFY_EVAL(NAME)        STRINGIFY(NAME)

#define iSCSIVirtualHBA         ADD_PREFIX(iSCSIVirtualHBA)
#define iSCSITaskQueue          ADD_PREFIX(iSCSITaskQueue)
#define iSCSIIOEventSource      ADD_PREFIX(iSCSIIOEventSource)
#define iSCSIInitiatorClient    ADD_PREFIX(iSCSIInitiatorClient)
#define iSCSIInitiator          ADD_PREFIX(iSCSIInitiator)

#define kiSCSIVirtualHBA_IOClassName STRINGIFY_EVAL(iSCSIVirtualHBA)

#endif
