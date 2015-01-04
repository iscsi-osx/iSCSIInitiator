/*!
 * @author		Nareg Sinenian
 * @file		iSCSISession.h
 * @date		March 15, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI session management functions.  This library
 *              depends on the user-space iSCSI PDU library to login, logout
 *              and perform discovery functions on iSCSI target nodes.
 */

#ifndef __ISCSI_SESSION_H__
#define __ISCSI_SESSION_H__

#include <CoreFoundation/CoreFoundation.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "iSCSIAuth.h"

/*! iSCSI portal address and port.  This is used when establishing a new 
 *  session or connection. */
typedef struct iSCSIPortal {
    
    /*! The portal address to query. */
    CFStringRef address;
    
    /*! The TCP port to use. */
    CFStringRef port;
    
    /*! The host adapter to use when connecting to the target. */
    CFStringRef hostInterface;
    
} iSCSIPortal;

/*! Information used to establish a connection to a target. */
typedef struct iSCSITarget {
    
    /*! Name of target to connect. */
    CFStringRef targetName;

    /*! Authentication method to use for the connection. */
    iSCSIAuthMethodRef authMethod;
    
    /*! Whether to try to use a header digest for the connection. */
    bool useHeaderDigest;
    
    /*! Whether to try to use a data digest for the connection. */
    bool useDataDigest;
    
} iSCSITarget;

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param portal specifies the portal to use for the new session.
 *  @param target specifies the target and connection parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSICreateSession(iSCSIPortal * const portal,
                           iSCSITarget * const target,
                           UInt16 * sessionId,
                           UInt32 * connectionId);

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param sessionId the session to free. */
errno_t iSCSIReleaseSession(UInt16 sessionId);

/*! Adds a new connection to an iSCSI session.
 *  @param portal specifies the portal to use for the connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIAddConnection(iSCSIPortal * const portal,
                           iSCSITarget * const target,
                           UInt16 sessionId,
                           UInt32 * connectionId);

/*! Removes a connection from an existing session.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove. */
errno_t iSCSIRemoveConnection(UInt16 sessionId,
                              UInt32 connectionId);

/*! Queries a portal for available targets.
 *  @param portal the iSCSI portal to query.
 *  @param targets an array of strings, where each string contains the name,
 *  alias, and portal associated with each target.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryPortalForTargets(iSCSIPortal * const portal,
                                   CFArrayRef * targets);

/*! Retrieves a list of targets available from a give portal.
 *  @param portal the iSCSI portal to look for targets.
 *  @param authMethods a comma-separated list of authentication methods
 *  as defined in the RFC3720 standard (version 1).
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIQueryTargetForAuthMethods(iSCSIPortal * const portal,
                                       CFStringRef targetName,
                                       CFStringRef * authMethods);

/*! Retreives the initiator session identifier associated with this target.
 *  @param targetName the name of the target.
 *  @param sessionId the session identiifer.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIGetSessionIdForTaget(CFStringRef targetName,
                                  UInt16 * sessionId);

/*! Sets the name of this initiator.  This is the IQN-format name that is
 *  exchanged with a target during negotiation.
 *  @param initiatorName the initiator name.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISetInitiatiorName(CFStringRef initiatorName);

/*! Sets the alias of this initiator.  This is the IQN-format alias that is
 *  exchanged with a target during negotiation.
 *  @param initiatorAlias the initiator alias.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISetInitiatorAlias(CFStringRef initiatorAlias);



    

#endif
