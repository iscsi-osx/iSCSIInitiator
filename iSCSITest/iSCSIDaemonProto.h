/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemonProto.h
 * @date		December 15, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		Defines the protocol for use in communicating with the
 *              iSCSI daemon.
 */

#ifndef __ISCSI_DAEMONPROTO_H__
#define __ISCSI_DAEMONPROTO_H__

#include <CoreFoundation/CoreFoundation.h>

////////////////////////////// DAEMON FUNCTIONS ////////////////////////////////

/*!  . */
CFStringRef kiSCSIDaemonFuncKey = CFSTR("Function");

CFStringRef kiSCSIDaemonFuncCreateSession = CFSTR("CreateSession");

CFStringRef kiSCSIDaemonFuncReleaseSession = CFSTR("ReleaseSession");

CFStringRef kiSCSIDaemonFuncAddConnection = CFSTR("AddConnection");

CFStringRef kiSCSIDaemonFuncRemoveConnection = CFSTR("RemoveConnection");


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











/*! Detailed login respones from a target that supplement the
 *  general responses defined by iSCSIPDULoginRspStatusClass. */
enum iSCSIConnectionStatus {
    
    kiSCSIPDULDSuccess = 0x0000,
    kiSCSIPDULDTargetMovedTemp = 0x0101,
    kiSCSIPDULDTargetMovedPerm = 0x0102,
    kiSCSIPDULDInitiatorError = 0x0200,
    kiSCSIPDULDAuthFail = 0x0201,
    kiSCSIPDULDAccessDenied = 0x0202,
    kiSCSIPDULDNotFound = 0x0203,
    kiSCSIPDULDTargetRemoved = 0x0204,
    kiSCSIPDULDUnsupportedVer = 0x0205,
    kiSCSIPDULDTooManyConnections = 0x0206,
    kiSCSIPDULDMissingParam = 0x0207,
    kiSCSIPDULDCantIncludeInSeession = 0x0208,
    kiSCSIPDULDSessionTypeUnsupported = 0x0209,
    kiSCSIPDULDSessionDoesntExist = 0x020a,
    kiSCSIPDULDInvalidReqDuringLogin = 0x020b,
    kiSCSIPDULDTargetHWorSWError = 0x0300,
    kiSCSIPDULDServiceUnavailable = 0x0301,
    kiSCSIPDULDOutOfResources = 0x0302
};

#endif