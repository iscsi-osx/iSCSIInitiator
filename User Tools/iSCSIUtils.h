/*!
 * @author		Nareg Sinenian
 * @file		iSCSIUtils.h
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI utility functions.
 */

#ifndef __ISCSI_UTILS_H__
#define __ISCSI_UTILS_H__

#include <CoreFoundation/CoreFoundation.h>
#include <regex.h>
#include <IOKit/scsi/SCSICmds_INQUIRY_Definitions.h>

/*! Verifies whether specified iSCSI qualified name (IQN) is valid per RFC3720.
 *  This function also validates 64-bit EUI names expressed as strings that
 *  start with the "eui" prefix.
 *  @param IQN the iSCSI qualified name.
 *  @return true if the name is valid, false otherwise. */
Boolean iSCSIUtilsValidateIQN(CFStringRef IQN);

/*! Validates and parses an expression of the form <host>:<port> into its
 *  hostname (or IPv4/IPv6 address) and port.  This function will return
 *  NULL if the specified expression is malformed, or an array containing
 *  either one or two elements (one if the portal is absent, two if it was
 *  specified).
 *  @param portal a string of the form <host>:<port>
 *  @return an array containing one or both portal parts, or NULL if the 
 *  specified portal was malformed. */
CFArrayRef iSCSIUtilsCreateArrayByParsingPortalParts(CFStringRef portal);

/*! Gets the SCSI peripheral description from a peripheral device type code.
 *  @param peripheralDeviceType the single byte peripheral device descriptor
 *  as outlined in the SPC-4 r36d.
 *  @return a string describing the device (guaranteed to be a valid string). */
CFStringRef iSCSIUtilsGetSCSIPeripheralDeviceDescription(UInt8 peripheralDeviceType);

#endif /* defined(__ISCSI_UTILS_H__) */
