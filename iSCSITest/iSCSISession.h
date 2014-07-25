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
#include "iSCSIAuth.h"

/*! Session options that are passed in when creating a new session.
 *  Some parameters may be negotiated and this struct is returned to the user
 *  with the final outcome of those negotiations. */
typedef struct iSCSISessionInfo {
    
    /*! Session qualifier (not required for new session, but required
     *  for a new connection for an existing session).  For new sessions,
     *  a value is assigned to this field. */
    UInt16 sessionId;
    
    /*! Maximum number of connections allowed this session. */
    UInt16 maxConnections;

}  iSCSISessionInfo;


/*! Connection options that are passed in when creating a new connection.
 *  Some parameters may be negotiated and this struct is returned to the user
 *  with the final outcome of those negotiations. */
typedef struct iSCSIConnectionInfo {
    
    /*! Connection ID number (not required for new session or connection).
     *  For new connections, a value is assigned to this field after
     *  a connection has been established. */
    UInt32 connectionId;
    
    /*! Returned error code (not required for new session or connection).
     *  For new connections, a value is assigned to this field after
     *  a connection has been established. */
    UInt16 status;

    /*! Whether to use a header digest (CRC32C is used if enabled). */
    bool useHeaderDigest;
    
    /*! Whether to use a data digest (CRC32C is used if enabled). */
    bool useDataDigest;
    
    /*! The host IP address to bind the target name to use. */
    CFStringRef hostAddress;
    
    /*! The target name to use. */
    CFStringRef targetAddress;
    
    /*! The TCP port to use. */
    CFStringRef targetPort;
    
    /*! The initiator name to use. */
    CFStringRef initiatorName;
    
    /*! The target name to use. */
    CFStringRef targetName;
    
    /*! The initiator alias. */
    CFStringRef initiatorAlias;
    
    /*! Authentication block. */
    iSCSIAuthMethodRef authMethod;
    
} iSCSIConnectionInfo;

/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param sessionInfo parameters associated with the normal session. A new
 *  sessionId is assigned and returned.
 *  @param connInfo parameters associated with the connection. A new
 *  connectionId is assigned and returned.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSICreateSession(iSCSISessionInfo * sessionInfo,
                           iSCSIConnectionInfo * connInfo);


/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param sessionId the session to free. */
errno_t iSCSIReleaseSession(UInt16 sessionId);

/*! Adds a new connection to an iSCSI session.
 *  @param sessionId the session to add a connection to.
 *  @param connectionId the ID of the new connection.
 *  @return an error code indicating whether the operation was successful. */
//errno_t iSCSIAddConnection(UInt16 sessionId,
//                                  iSCSIConnectionInfo * connInfo);

    
/*! Removes a connection from an existing session.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove. */
//errno_t iSCSIRemoveConnection(UInt16 sessionId,
//                                     UInt16 connectionId);

/*! Gets a list of targets associated with a particular session.
 *  @param sessionId the session (discovery or normal) to use.
 *  @param connectionId the connection ID to use.
 *  @param targetList a list of targets retreived from teh iSCSI node.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSISessionGetTargetList(UInt16 sessionId,
                                  UInt32 connectionId,
                                  CFMutableDictionaryRef targetList);
    

#endif
