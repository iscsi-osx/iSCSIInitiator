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

/*! Command to login. */
typedef struct iSCSIDCmdLogin {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  portalLength;
    UInt32  targetLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;

} __attribute__((packed)) iSCSIDCmdLogin;

/*! Default initialization for a login command. */
extern const iSCSIDCmdLogin iSCSIDCmdLoginInit;

/*! Response to a login  command. */
typedef struct iSCSIDRspLogin {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;

} __attribute__((packed)) iSCSIDRspLogin;

/*! Command to logout. */
typedef struct iSCSIDCmdLogout {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  portalLength;
    UInt32  targetLength;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;

} __attribute__((packed)) iSCSIDCmdLogout;

/*! Default initialization for a logout command. */
extern const iSCSIDCmdLogout iSCSIDCmdLogoutInit;

/*! Response to a login command. */
typedef struct iSCSIDRspLogout {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;

} __attribute__((packed)) iSCSIDRspLogout;

/*! Command to get active targets. */
typedef struct iSCSIDCmdCreateArrayOfActiveTargets {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;
} __attribute__((packed)) iSCSIDCmdCreateArrayOfActiveTargets;

/*! Default initialization for command to get active targets. */
extern const iSCSIDCmdCreateArrayOfActiveTargets iSCSIDCmdCreateArrayOfActiveTargetsInit;

/*! Response to command to get active targets. */
typedef struct iSCSIDRspCreateArrayOfActiveTargets {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 dataLength;

} __attribute__((packed)) iSCSIDRspCreateArrayOfActiveTargets;

/*! Command to get active portals. */
typedef struct iSCSIDCmdCreateArrayOfActivePortalsForTarget {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;
} __attribute__((packed)) iSCSIDCmdCreateArrayOfActivePortalsForTarget;

/*! Default initialization for command to get active portals. */
extern const iSCSIDCmdCreateArrayOfActivePortalsForTarget iSCSIDCmdCreateArrayOfActivePortalsForTargetInit;

/*! Response to command to get active portals. */
typedef struct iSCSIDRspCreateArrayOfActivePortalsForTarget {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 dataLength;

} __attribute__((packed)) iSCSIDRspCreateArrayOfActivePortalsForTarget;

/*! Command to test whether target is active. */
typedef struct iSCSIDCmdIsTargetActive {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  targetLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
} __attribute__((packed)) iSCSIDCmdIsTargetActive;

/*! Default initialization for command to test whether target is active. */
extern const iSCSIDCmdIsTargetActive iSCSIDCmdIsTargetActiveInit;

/*! Response to command to test whether target is active. */
typedef struct iSCSIDRspIsTargetActive {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 active;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;

} __attribute__((packed)) iSCSIDRspIsTargetActive;

/*! Command to test whether portal is active. */
typedef struct iSCSIDCmdIsPortalActive {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  portalLength;
    UInt32  targetLength;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
} __attribute__((packed)) iSCSIDCmdIsPortalActive;

/*! Default initialization for command to test whether portal is active. */
extern const iSCSIDCmdIsPortalActive iSCSIDCmdIsPortalActiveInit;

/*! Response to command to test whether portal is active. */
typedef struct iSCSIDRspIsPortalActive {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 active;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;

} __attribute__((packed)) iSCSIDRspIsPortalActive;


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

/*! Command to get information about a session. */
typedef struct iSCSIDCmdCreateCFPropertiesForSession {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  targetLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    
} __attribute__((packed)) iSCSIDCmdCreateCFPropertiesForSession;

/*! Default initialization for a get session information command. */
extern const iSCSIDCmdCreateCFPropertiesForSession iSCSIDCmdCreateCFPropertiesForSessionInit;

/*! Response to command to get information about a session. */
typedef struct iSCSIDRspCreateCFPropertiesForSession {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 dataLength;
    UInt32 reserved5;
    
} __attribute__((packed)) iSCSIDRspCreateCFPropertiesForSession;

/*! Command to get information about a connection. */
typedef struct iSCSIDCmdCreateCFPropertiesForConnection {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  targetLength;
    UInt32  portalLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    
} __attribute__((packed)) iSCSIDCmdCreateCFPropertiesForConnection;

/*! Default initialization for a get connection information command. */
extern const iSCSIDCmdCreateCFPropertiesForConnection iSCSIDCmdCreateCFPropertiesForConnectionInit;

/*! Response to command to get information about a connection. */
typedef struct iSCSIDRspCreateCFPropertiesForConnection {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 dataLength;
    UInt32 reserved5;

} __attribute__((packed)) iSCSIDRspCreateCFPropertiesForConnection;


/*! Command update discovery. */
typedef struct iSCSIDCmdUpdateDiscovery {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;

} __attribute__((packed)) iSCSIDCmdUpdateDiscovery;

/*! Default initialization update discovery command. */
extern const iSCSIDCmdUpdateDiscovery iSCSIDCmdUpdateDiscoveryInit;

/*! Response to command update discovery. */
typedef struct iSCSIDRspUpdateDiscovery {

    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;

} __attribute__((packed)) iSCSIDRspUpdateDiscovery;


////////////////////////////// DAEMON FUNCTIONS ////////////////////////////////

enum iSCSIDFunctionCodes {

    /* Login to a target over one or more portals. */
    kiSCSIDLogin = 0,

    /* Logout of a target or portal. */
    kiSCSIDLogout = 1,

    /* Get a list of connected targets. */
    kiSCSIDCreateArrayOfActiveTargets = 2,

    /* Get a list of portals for the connected target. */
    kiSCSIDCreateArrayOfActivePortalsForTarget = 3,

    /* Get whether a target has an active session. */
    kiSCSIDIsTargetActive = 4,

    /* Get whether a portal has an active connection. */
    kiSCSIDIsPortalActive = 5,

    /* Get negotiated parameters for the connected target. */
    kiSCSIDCreateCFPropertiesForSession = 6,

    /* Get negotiated parameters for the connected portal. */
    kiSCSIDCreateCFPropertiesForConnection = 7,

    /* Query a portal for targets. */
    kiSCSIDQueryPortalForTargets = 8,

    /*! Query a target for supported authentication methods. */
    kiSCSIDQueryTargetForAuthMethod = 9,

    /*! Update discovery parameters. */
    kiSCSIDUpdateDiscovery = 10,

    /*! Set the initiator IQN. */
    kiSCSIDSetInitiatorIQN = 11,

    /*! Set the intiator alias. */
    kiSCSIDSetInitiatorAlias = 12,

    /*! Shut down the daemon. */
    kiSCSIDShutdownDaemon = 13,

    /*! Invalid daemon command. */
    kiSCSIDInvalidFunctionCode
};


/*! Helper function. Reads data from a socket of the specified length and
 *  calls a constructor function on the data to return an object of the
 *  appropriate type. */
void * iSCSIDCreateObjectFromSocket(int fd,UInt32 length,void *(* objectCreator)(CFDataRef))
{
    if(length == 0)
        return NULL;
    
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
