/*!
 * @author		Nareg Sinenian
 * @file		iSCSIIORegistry.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space management of iSCSI I/O registry entries.
 */


#ifndef __ISCSI_IOREGISTRY_H__
#define __ISCSI_IOREGISTRY_H__

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define kIOPropertyiSCSIQualifiedNameKey "iSCSI Qualified Name"

typedef void (*iSCSIIOMediaCallback)(io_object_t entry,void * context);

/*! Gets the iSCSIVirtualHBA object in the IO registry.*/
io_object_t iSCSIIORegistryGetiSCSIHBAEntry();

/*! Gets the target object (IOSCSITargetDevice) in the IO registry that
 *  corresponds to the specified target.
 *  @param targetIQN the name of the target.
 *  @return the IO registry object of the IOSCSITargetDevice for this session. */
io_object_t iSCSIIORegistryGetTargetEntry(CFStringRef targetIQN);

/*! Gets an iterator for traversing iSCSI targets in the I/O registry.
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
 *  @param lun the IO registry object corresponding to a LUN. 
 *  @return the IOMedia object for the LUN. */
io_object_t iSCSIIORegistryFindIOMediaForLUN(io_object_t lun);

/*! Creates a dictionary of properties associated with the target.  These
 *  include the following keys:
 *
 *  kIOPropertySCSIVendorIdentification (CFStringRef)
 *  kIOPropertySCSIProductIdentification (CFStringRef)
 *  kIOPropertyiSCSIQualifiedNameKey (CFStringRef)
 *  kIOPropertySCSITargetIdentifierKey (CFNumberRef)
 *
 *  @param target the target IO registry object.
 *  @return a dictionary of values for the properties, or NULL if the object
 *  could not be found. */
CFDictionaryRef iSCSIIORegistryCreateCFPropertiesForTarget(io_object_t target);

/*! Creates a dictionary of properties associated with the LUN.  These
 *  include the following keys:
 *
 *  kIOBSDNameKey (CFStringRef)
 *  kIOMediaSizeKey (CFNumberRef)
 *  kIOMediaPreferredBlockSizeKey (CFNumberRef)
 *  kIOPropertySCSILogicalUnitNumberKey (CFNumberRef)
 *
 *  @param lun the target IO registry object.
 *  @return a dictionary of values for the properties, or NULL if the object
 *  could not be found. */
CFDictionaryRef iSCSIIORegistryCreateCFPropertiesForLUN(io_object_t lun);



#endif