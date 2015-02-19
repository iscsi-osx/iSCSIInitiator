/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemonSettings.c
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Provides user-space library functions to read and write
 *              daemon configuration property list
 */

#include "iSCSIDatabase.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPreferences.h>

/*! Preference key name for iSCSI initiator settings dictionary. */
CFStringRef kiSCSIPKInitiatorSettingsDict = CFSTR("InitiatorSettings");

/*! Preference key name for iSCSI initiator name. */
CFStringRef kiSCSIPKInitiatorSettingsName = CFSTR("Name");

/*! Preference key name for iSCSI initiator alias. */
CFStringRef kiSCSIPKInitiatorSettingsAlias = CFSTR("Alias");


/*! Preference key name for iSCSI target array. */
CFStringRef kiSCSIPKTargetsArray = CFSTR("Targets");

/*! Preference key name for iSCSI target name. */
CFStringRef kiSCSIPKTargetName = CFSTR("Name");


/*! Pereference key name for iSCSI session context dictionary. */
CFStringRef kiSCSIPKSessionContextDict = CFSTR("SessionContext");

/*! Pereference key name for iSCSI session identifier. */
CFStringRef kiSCSIPKSessionIdentifier = CFSTR("SessionIdentifier");

/*! Pereference key name for iSCSI target session identifying handle. */
CFStringRef kiSCSIPKTSIH = CFSTR("TSIH");

/*! Pereference key name for iSCSI session maximum number of connections. */
CFStringRef kiSCSIPKMaxConnections = CFSTR("MaxConnections");

/*! Pereference key name for iSCSI session target portal group tag. */
CFStringRef kiSCSIPKTargetPortalGroupTag = CFSTR("TargetPortalGroupTag");

/*! Pereference key name for iSCSI target alias (supplied by the target). */
CFStringRef kiSCSIPKTargetAlias = CFSTR("Alias");


/*! Pereference key name for iSCSI session connections array. */
CFStringRef kiSCSIPKConnectionsArray = CFSTR("Connections");


/*! Preference key name for iSCSI portal array. */
CFStringRef kiSCSIPKPortalsArray = CFSTR("Portals");

/*! Preference key name for iSCSI portal address. */
CFStringRef kiSCSIPKPortalAddress = CFSTR("Address");

/*! Preference key name for iSCSI portal port. */
CFStringRef kiSCSIPKPortalPort = CFSTR("Port");

/*! Preference key name for host interface used to connect to this portal. */
CFStringRef kiSCSIPKHostInterface = CFSTR("HostInterface");



/*! Returns the initiator name. */
CFStringRef iSCSIDaemonSettingsCopyInitiatorName();

/*! Sets the initiator name. */

