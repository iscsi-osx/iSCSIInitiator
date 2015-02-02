/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDatabase.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions to read and write
 *              daemon configuration property list
 */

#ifndef __ISCSI_DATABASE_H__
#define __ISCSI_DATABASE_H__


#include <CoreFoundation/CoreFoundation.h>
#include <errno.h>

#include "iSCSITypes.h"



/*! Copys the initiator name from the database into a CFString object.
 *  @param initiatorName the initiator name retrieved.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBCopyInitiatorName(CFStringRef * initiatorName);

/*! Sets the initiator name in the database.
 *  @param initiatorName the initiator name to set.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBSetInitiatorName(CFStringRef initiatorName);

/*! Copies a target record into a target struct.
 *  @param targetName the name of the target to retrieve.
 *  @param target the target struct created.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBCopyTargetRecord(CFStringRef targetName,
                                iSCSITargetRef * target);

/*! Sets target record values from a target struct.
 *  @param targetName the name of the target to set.
 *  @param target the target struct containing values to set.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBSetTargetRecord(CFStringRef targetName,
                               iSCSITargetRef target);

/*! Copies a portal record into a portal struct.
 *  @param targetName the name of the target that has the associated portal.
 *  @param portalIndex the portal index with the associated target.
  *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBCopyPortalRecord(CFStringRef targetName,
                                CFIndex portalIndex,
                                iSCSIPortalRef * portal);

//errno_t iSCSIDBCopyAllPortalRecords(CFStringRef targetName,
//                                    iSCSIPortal * portals);

/*! Copies a portal record into a portal struct.
 *  @param targetName the name of the target that has the associated portal.
 *  @param portalIndex the portal index with the associated target.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBSetPortalRecord(CFStringRef targetName,
                               CFIndex portalIndex,
                               iSCSIPortalRef portal);

/*! Removes a portal record.
 *  @param targetName the name of the target that has the associated portal.
 *  @param portalIndex the portal index with the associated target.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBRemovePortalRecord(CFStringRef targetName,
                                  CFIndex portalIndex);

/*! Removes a target record, including all portals associated with the target.
 *  @param targetName the name of the target that has the associated portal.
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBRRemoveTargetRecord(CFStringRef targetName);

/*! Gets Names of targets that have been marked as persistent.
 *  @param targetNames array of CFStringRef objects (target names).
 *  @return an error code indiciating the result of the operation. */
errno_t iSCSIDBCopyPersistentTargetNames(CFArrayRef * targetNames);

/*!


*/

#endif
