/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemonInterfaceShared.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Defines interface used by client applications to access
 *              the iSCSIDaemon.  These definitions are shared between kernel
 *              and user space.
 *
 *
 * Daemon commands and responses consist of a 24-byte header followed by
 * data.  The first two bytes of the header indicate the command or response
 * type (these values match for commands and responses for the same function).
 * The type of data that follows the command or respond header depends on the
 * particular type of command or response. Generally, if data follows a command
 * or response the length of that data is specified in the command or response
 * header (in bytes).  For example, the login session command has the following
 * header:
 *
 *      const UInt16 funcCode;
 *      UInt16  reserved;
 *      UInt32  reserved2;
 *      UInt32  connectionId;
 *      UInt32  portalLength;
 *      UInt32  targetLength;
 *      UInt32  authLength;
 *
 * This indicates that the header is followed by three objects: a portal, a
 * target and an authentication object, each of which have a length specified
 * in the header command (e.g., portalLength).  The order in which these data
 * follow the header is specified by the order in which they appear in the
 * the header.  The same is true for responses the daemon sends to clients.
 */


#ifndef __ISCSI_DAEMON_INTERFACE_SHARED_H__
#define __ISCSI_DAEMON_INTERFACE_SHARED_H__

#include "iSCSITypes.h"
#include "iSCSITypesShared.h"

/*! Generic iSCSI daemon command header. */
typedef struct iSCSIDCmd {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;
    
} __attribute__((packed)) iSCSIDCmd;

/*! Generic iSCSI daemon response header. */
typedef struct iSCSIDRsp {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;
    
} __attribute__((packed)) iSCSIDRsp;

/*! Command to shutdown the daemon. */
typedef struct iSCSIDCmdShutdown {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;
    
} __attribute__((packed)) iSCSIDCmdShutdown;

/*! Default initialization for a shutdown command. */
extern const iSCSIDCmdShutdown iSCSIDCmdShutdownInit;

/*! Command to login a particular session. */
typedef struct iSCSIDCmdLoginSession {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  portalLength;
    UInt32  targetLength;
    UInt32  authLength;
    UInt32  sessCfgLength;
    UInt32  connCfgLength;
    
} __attribute__((packed)) iSCSIDCmdLoginSession;

/*! Default initialization for a login request command. */
extern const iSCSIDCmdLoginSession iSCSIDCmdLoginSessionInit;

/*! Response to a session login command. */
typedef struct iSCSIDRspLoginSession {
    
    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 sessionId;
    UInt32 connectionId;
    UInt32 reserved2;
    UInt32 reserved3;
    
} __attribute__((packed)) iSCSIDRspLoginSession;



/*! Command to logout a particular session. */
typedef struct iSCSIDCmdLogoutSession {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    
} __attribute__((packed)) iSCSIDCmdLogoutSession;

/*! Default initialization for a logout request command. */
extern const iSCSIDCmdLogoutSession iSCSIDCmdLogoutSessionInit;

/*! Response to a session logout command. */
typedef struct iSCSIDRspLogoutSession {
    
    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    
} __attribute__((packed)) iSCSIDRspLogoutSession;



/*! Command to add a connection. */
typedef struct iSCSIDCmdLoginConnection {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  portalLength;
    UInt32  authLength;
    UInt32  connCfgLength;
    UInt32  reserved2;
    
} __attribute__((packed)) iSCSIDCmdLoginConnection;

/*! Default initialization for an add connection command. */
extern const iSCSIDCmdLoginConnection iSCSIDCmdLoginConnectionInit;

/*! Response to add an connection command. */
typedef struct iSCSIDRspLoginConnection {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 connectionId;
    UInt32 reserved3;
    UInt32 reserved4;
    
} __attribute__((packed)) iSCSIDRspLoginConnection;



/*! Command to remove a connection. */
typedef struct iSCSIDCmdLogoutConnection {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  connectionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    
} __attribute__((packed)) iSCSIDCmdLogoutConnection;

/*! Default initialization for a remove connection command. */
extern const iSCSIDCmdLogoutConnection iSCSIDCmdLogoutConnectionInit;

/*! Response to add an connection command. */
typedef struct iSCSIDRspLogoutConnection {
    
    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    
} __attribute__((packed)) iSCSIDRspLogoutConnection;



/*! Command to query a portal for targets. */
typedef struct iSCSIDCmdQueryPortalForTargets {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  portalLength;
    UInt32  reserved4;
    UInt32  reserved5;

} __attribute__((packed)) iSCSIDCmdQueryPortalForTargets;

/*! Default initialization for a portal query command. */
extern const iSCSIDCmdQueryPortalForTargets iSCSIDCmdQueryPortalForTargetsInit;

/*! Response to query a portal for targets.  This response typedef struct is followed
 *  by a discovery record (an iSCSIDiscoveryRec object).  */
typedef struct iSCSIDRspQueryPortalForTargets {
    
    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 discoveryLength;
    UInt32 reserved4;
    
} __attribute__((packed)) iSCSIDRspQueryPortalForTargets;



/*! Command to query target for authentication method. */
typedef struct iSCSIDCmdQueryTargetForAuthMethod {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  portalLength;
    UInt32  targetLength;
    UInt32  reserved4;
} __attribute__((packed)) iSCSIDCmdQueryTargetForAuthMethod;

/*! Default initialization for a portal query command. */
extern const iSCSIDCmdQueryTargetForAuthMethod iSCSIDCmdQueryTargetForAuthMethodInit;

/*! Response to query a portal for authentication method. */
typedef struct iSCSIDRspQueryTargetForAuthMethod {
    
    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 authMethod;
    
} __attribute__((packed)) iSCSIDRspQueryTargetForAuthMethod;



/*! Command to get session identifier for a target (using target name). */
typedef struct iSCSIDCmdGetSessionIdForTarget {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  targetLength;
    UInt32  reserved5;
} __attribute__((packed)) iSCSIDCmdGetSessionIdForTarget;

/*! Default initialization for command to get session id for a target. */
extern const iSCSIDCmdGetSessionIdForTarget iSCSIDCmdGetSessionIdForTargetInit;

/*! Response to get session identifier for a target (using target name). */
typedef struct iSCSIDRspGetSessionIdForTarget {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 sessionId;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    
} __attribute__((packed)) iSCSIDRspGetSessionIdForTarget;




/*! Command to get connection identifier for a session (using address). */
typedef struct iSCSIDCmdGetConnectionIdForPortal {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  reserved2;
    UInt32  portalLength;
    UInt32  reserved3;
    UInt32  reserved4;
} __attribute__((packed)) iSCSIDCmdGetConnectionIdForPortal;

/*! Default initialization for command to get connection ID for a session. */
extern const iSCSIDCmdGetConnectionIdForPortal iSCSIDCmdGetConnectionIdForPortalInit;

/*! Response to get connection identifier for a session (using address). */
typedef struct iSCSIDRspGetConnectionIdForPortal {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 connectionId;
    UInt32 reserved4;
    UInt32 reserved5;

} __attribute__((packed)) iSCSIDRspGetConnectionIdForPortal;

/*! Command to get all session identifiers. */
typedef struct iSCSIDCmdGetSessionIds {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;
} __attribute__((packed)) iSCSIDCmdGetSessionIds;

/*! Default initialization for command to get all session identifiers.  This 
 *  typedef struct is followed by an array of SIDs with length specified by 
 *  sessionCount. */
extern const iSCSIDCmdGetSessionIds iSCSIDCmdGetSessionIdsInit;

/*! Response to get all session identifiers. */
typedef struct iSCSIDRspGetSessionIds {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 dataLength;

} __attribute__((packed)) iSCSIDRspGetSessionIds;



/*! Command to get all connection identifiers for a session. */
typedef struct iSCSIDCmdGetConnectionIds {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    
} __attribute__((packed)) iSCSIDCmdGetConnectionIds;

/*! Default initialization for command to get all connection IDs for a session. */
extern const iSCSIDCmdGetConnectionIds iSCSIDCmdGetConnectionIdsInit;

/*! Response to get all connection identifiers for a session.   This typedef struct
 *  is followed by an array of CIDs with length specified by connectionCount. */
typedef struct iSCSIDRspGetConnectionIds {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 dataLength;

} __attribute__((packed)) iSCSIDRspGetConnectionIds;

/*! Command to create a target object for a session identifier. */
typedef struct iSCSIDCmdCreateTargetForSessionId {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    
} __attribute__((packed)) iSCSIDCmdCreateTargetForSessionId;

/*! Default initialization for command to create a target object 
 *  for a session identifier . */
extern const iSCSIDCmdCreateTargetForSessionId iSCSIDCmdCreateTargetForSessionIdInit;

/*! Response to to create a target object for a session identifier.   
 *  This typedef struct is followed by a target object. */
typedef struct iSCSIDRspCreateTargetForSessionId {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 reserved1;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 targetLength;
    
} __attribute__((packed)) iSCSIDRspCreateTargetForSessionId;


/*! Command to create a portal object for a connection identifier. */
typedef struct iSCSIDCmdCreatePortalForConnectionId {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  connectionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    
} __attribute__((packed)) iSCSIDCmdCreatePortalForConnectionId;

/*! Default initialization for command to create a portal object
 *  for a connection identifier . */
extern const iSCSIDCmdCreatePortalForConnectionId iSCSIDCmdCreatePortalForConnectionIdInit;

/*! Response to to create a portal object for a connection identifier.
 *  This typedef struct is followed by a target object. */
typedef struct iSCSIDRspCreatePortalForConnectionId {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 reserved1;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 portalLength;
    
} __attribute__((packed)) iSCSIDRspCreatePortalForConnectionId;


/*! Command to get information about a session. */
typedef struct iSCSIDCmdCopySessionConfig {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    
} __attribute__((packed)) iSCSIDCmdCopySessionConfig;

/*! Default initialization for a get session information command. */
extern const iSCSIDCmdCopySessionConfig iSCSIDCmdCopySessionConfigInit;

/*! Response to command to get information about a session. */
typedef struct iSCSIDRspCopySessionConfig {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 dataLength;
    UInt32 reserved5;
    
} __attribute__((packed)) iSCSIDRspCopySessionConfig;



/*! Command to get information about a connection. */
typedef struct iSCSIDCmdCopyConnectionConfig {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  sessionId;
    UInt32  connectionId;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    
} __attribute__((packed)) iSCSIDCmdCopyConnectionConfig;

/*! Default initialization for a get connection information command. */
extern const iSCSIDCmdCopyConnectionConfig iSCSIDCmdCopyConnectionConfigInit;

/*! Response to command to get information about a connection. */
typedef struct iSCSIDRspGetConnectionConfig {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 dataLength;
    UInt32 reserved5;

} __attribute__((packed)) iSCSIDRspGetConnectionConfig;


////////////////////////////// DAEMON FUNCTIONS ////////////////////////////////

enum iSCSIDFunctionCodes {
    
    kiSCSIDLoginSession = 0,
    kiSCSIDLogoutSession = 1,
    kiSCSIDLoginConnection = 2,
    kiSCSIDLogoutConnection = 3,
    kiSCSIDQueryPortalForTargets = 4,
    kiSCSIDQueryTargetForAuthMethod = 5,
    kiSCSIDGetSessionIdForTarget = 6,
    kiSCSIDGetConnectionIdForPortal = 7,
    kiSCSIDGetSessionIds = 8,
    kiSCSIDGetConnectionIds = 9,
    kiSCSIDCreateTargetForSessionId = 10,
    kiSCSIDCreatePortalForConnectionId = 11,
    kiSCSIDCopySessionConfig = 12,
    kiSCSIDCopyConnectionConfig = 13,
    kiSCSIDSetInitiatorIQN = 14,
    kiSCSIDSetInitiatorAlias = 15,
    kiSCSIDShutdownDaemon = 16,
    kiSCSIDInvalidFunctionCode
};


/*! Helper function. Reads data from a socket of the specified length and
 *  calls a constructor function on the data to return an object of the
 *  appropriate type. */
void * iSCSIDCreateObjectFromSocket(int fd,UInt32 length,void *(* objectCreator)(CFDataRef))
{
    // Receive iSCSI object data from stream socket
    UInt8 * bytes = (UInt8 *) malloc(length);
    if(!bytes || (recv(fd,bytes,length,0) != length))
    {
        free(bytes);
        return NULL;
    }
    
    // Build a CFData wrapper around the data
    CFDataRef data = NULL;
    
    if(!(data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,bytes,length,kCFAllocatorMalloc)))
    {
        free(bytes);
        return NULL;
    }
    
    // Create an iSCSI object from the data
    void * object = objectCreator(data);
    
    CFRelease(data);
    return object;
}


#endif /* defined(__ISCSI_DAEMON_INTERFACE_SHARED_H__) */
