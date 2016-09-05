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

#ifndef __ISCSI_DISCOVERY_H__
#define __ISCSI_DISCOVERY_H__

#include <CoreFoundation/CoreFoundation.h>
#include <asl.h>

#include "iSCSISession.h"
#include "iSCSITypes.h"
#include "iSCSIPreferences.h"


/*! Scans all iSCSI discovery portals found in iSCSI preferences
 *  for targets (SendTargets). Returns a dictionary of key-value pairs
 *  with discovery record objects as values and discovery portal names
 *  as keys.
 *  @param preferences an iSCSI preferences object.
 *  @return a dictionary key-value pairs of dicovery portal names (addresses)
 *  and the discovery records associated with the result of SendTargets
 *  discovery of those portals. */
CFDictionaryRef iSCSIDiscoveryCreateRecordsWithSendTargets(iSCSISessionManagerRef managerRef,
                                                           iSCSIPreferencesRef preferences);

/*! Updates an iSCSI preference sobject with information about targets as
 *  contained in the provided discovery record.
 *  @param preferences an iSCSI preferences object.
 *  @param discoveryPortal the portal (address) that was used to perform discovery.
 *  @param discoveryRec the discovery record resulting from the discovery operation.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSIDiscoveryUpdatePreferencesWithDiscoveredTargets(iSCSISessionManagerRef managerRef,
                                                             iSCSIPreferencesRef preferences,
                                                             CFStringRef discoveryPortal,
                                                             iSCSIDiscoveryRecRef discoveryRec);

#endif /* defined(__ISCSI_DISCOVERY_H__) */
