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

#ifndef __ISCSI_IOREGISTRY_H__
#define __ISCSI_IOREGISTRY_H__

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>

#define kIOPropertyiSCSIQualifiedNameKey "iSCSI Qualified Name"

typedef void (*iSCSIIOMediaCallback)(io_object_t entry,void * context);

/*! Gets the iSCSIVirtualHBA object in the IO registry.*/
io_object_t iSCSIIORegistryGetiSCSIHBAEntry(void);

/*! Gets the target object (IOSCSITargetDevice) in the IO registry that
 *  corresponds to the specified target.
 *  @param targetIQN the name of the target.
 *  @return the IO registry object of the IOSCSITargetDevice for this session. */
io_object_t iSCSIIORegistryGetTargetEntry(CFStringRef targetIQN);

/*! Gets an iterator for traversing iSCSI targets in the I/O registry.
 *  As targets are iterated, this function may also return an IOObject that
 *  corresponds to the user client, if one is active.  Users can check for
 *  this by using standard IOSerivce functions.
 *  @param iterator the iterator.
 *  @return a kernel error code indicating the result of the operation. */
kern_return_t iSCSIIORegistryGetTargets(io_iterator_t * iterator);

/*! Gets an iterator for traversing iSCSI LUNs for a specified target in the
 *  I/O registry.
 *  @param targetIQN the name of the target.
 *  @param iterator the iterator used to traverse LUNs for the specified target.
 *  @return a kernel error code indicating the result of the operation. */
kern_return_t iSCSIIORegistryGetLUNs(CFStringRef targetIQN,io_iterator_t * iterator);

/*! Applies a callback function all IOMedia objects of a particular target.
 *  @param root search children of this node for IOMedia objects.
 *  @param callback the callback function to call on each IOMedia object.
 *  @param context a user-defined parameter to pass to the callback function. */
void iSCSIIORegistryIOMediaApplyFunction(io_object_t root,
                                         iSCSIIOMediaCallback callback,
                                         void * context);

/*! Finds the IOMedia object associated with the LUN object.
 *  @param LUN the IO registry object corresponding to a LUN.
 *  @return the IOMedia object for the LUN. */
io_object_t iSCSIIORegistryFindIOMediaForLUN(io_object_t LUN);

/*! Creates a dictionary of properties associated with the target.  These
 *  include the following keys (not exhaustive, see OS X documentation):
 *
 *  kIOPropertySCSIVendorIdentification (CFStringRef)
 *  kIOPropertySCSIProductIdentification (CFStringRef)
 *  kIOPropertySCSIProductRevisionLevel (CFStringRef)
 *  kIOPropertySCSIINQUIRYUnitSerialNumber (CFStringRef)
 *
 *  The following keys are defined in the protocol characteristics dictionary:
 *
 *      kIOPropertyiSCSIQualifiedNameKey (CFStringRef)
 *      kIOPropertySCSITargetIdentifierKey (CFNumberRef)
 *      kIOPropertySCSIDomainIdentifierKey (CFNumberRef)
 *
 *  The dictionary is embedded within the properties dictionary and
 *  may be accessed using the kIOPropertyProtocolCharacteristicsKey.
 *
 *  @param target the target IO registry object.
 *  @return a dictionary of values for the properties, or NULL if the object
 *  could not be found. */
CFDictionaryRef iSCSIIORegistryCreateCFPropertiesForTarget(io_object_t target);

/*! Creates a dictionary of properties associated with the LUN.  These
 *  include the following keys (not exhaustive, see OS X documentation):
 *
 *  kIOPropertySCSIVendorIdentification (CFStringRef)
 *  kIOPropertySCSIProductIdentification (CFStringRef)
 *  kIOPropertySCSIProductRevisionLevel (CFStringRef)
 *  kIOPropertySCSILogicalUnitNumberKey (CFNumberRef)
 *  kIOPropertySCSIPeripheralDeviceType (CFNumberRef)
 *
 *  @param LUN the target IO registry object.
 *  @return a dictionary of values for the properties, or NULL if the object
 *  could not be found. */
CFDictionaryRef iSCSIIORegistryCreateCFPropertiesForLUN(io_object_t LUN);

/*! Creates a dictionary of properties associated with the LUN.  These
 *  include the following keys (not exhausitve, see OS X documentation):
 *
 *  kIOBSDNameKey (CFStringRef)
 *  kIOMediaSizeKey (CFNumberRef)
 *  kIOMediaPreferredBlockSizeKey (CFNumberRef)
 *
 *  @param IOMedia the IOMedia IO registry object.
 *  @return a dictionary of values for the properties, or NULL if the object
 *  could not be found. */
CFDictionaryRef iSCSIIORegistryCreateCFPropertiesForIOMedia(io_object_t IOMedia);

#endif
