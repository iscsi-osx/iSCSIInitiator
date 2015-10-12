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

typedef UInt32 CFLength;

/*! Generic iSCSI daemon-client message (basis for commands and responses. */
typedef struct __iSCSIDMsgGeneric {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;

} __attribute__((packed)) iSCSIDMsgGeneric;


/*! Generic iSCSI daemon command header. */
typedef struct __iSCSIDMsgCmd {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;
    
} __attribute__((packed)) iSCSIDMsgCmd;

/*! Generic iSCSI daemon response header. */
typedef struct __iSCSIDMsgRsp {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;
    
} __attribute__((packed)) iSCSIDMsgRsp;

/*! Command to shutdown the daemon. */
typedef struct __iSCSIDMsgShutdownCmd {

    UInt16 funcCode;
    UInt16 reserved;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    UInt32 reserved6;
    
} __attribute__((packed)) iSCSIDMsgShutdownCmd;

/*! Default initialization for a shutdown command. */
extern const iSCSIDMsgShutdownCmd iSCSIDMsgShutdownCmdInit;

/*! Command to login. */
typedef struct __iSCSIDMsgLoginCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    CFLength  portalLength;
    CFLength  targetLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;

} __attribute__((packed)) iSCSIDMsgLoginCmd;

/*! Default initialization for a login command. */
extern const iSCSIDMsgLoginCmd iSCSIDMsgLoginCmdInit;

/*! Response to a login  command. */
typedef struct __iSCSIDMsgLoginRsp {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgLoginRsp;

/*! Command to logout. */
typedef struct __iSCSIDMsgLogoutCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    CFLength  portalLength;
    CFLength  targetLength;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;

} __attribute__((packed)) iSCSIDMsgLogoutCmd;

/*! Default initialization for a logout command. */
extern const iSCSIDMsgLogoutCmd iSCSIDMsgLogoutCmdInit;

/*! Response to a login command. */
typedef struct __iSCSIDMsgLogoutRsp {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgLogoutRsp;

/*! Command to get active targets. */
typedef struct __iSCSIDMsgCreateArrayOfActiveTargetsCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;
} __attribute__((packed)) iSCSIDMsgCreateArrayOfActiveTargetsCmd;

/*! Default initialization for command to get active targets. */
extern const iSCSIDMsgCreateArrayOfActiveTargetsCmd iSCSIDMsgCreateArrayOfActiveTargetsCmdInit;

/*! Response to command to get active targets. */
typedef struct __iSCSIDMsgCreateArrayOfActiveTargetsRsp {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgCreateArrayOfActiveTargetsRsp;

/*! Command to get active portals. */
typedef struct __iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;
} __attribute__((packed)) iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd;

/*! Default initialization for command to get active portals. */
extern const iSCSIDMsgCreateArrayOfActivePortalsForTargetCmd iSCSIDMsgCreateArrayOfActivePortalsForTargetCmdInit;

/*! Response to command to get active portals. */
typedef struct __iSCSIDMsgCreateArrayOfActivePortalsForTargetRsp {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgCreateArrayOfActivePortalsForTargetRsp;

/*! Command to test whether target is active. */
typedef struct __iSCSIDMsgIsTargetActiveCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    CFLength  targetLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
} __attribute__((packed)) iSCSIDMsgIsTargetActiveCmd;

/*! Default initialization for command to test whether target is active. */
extern const iSCSIDMsgIsTargetActiveCmd iSCSIDMsgIsTargetActiveCmdInit;

/*! Response to command to test whether target is active. */
typedef struct __iSCSIDMsgIsTargetActiveRsp {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 active;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgIsTargetActiveRsp;

/*! Command to test whether portal is active. */
typedef struct __iSCSIDMsgIsPortalActiveCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    CFLength  portalLength;
    CFLength  targetLength;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
} __attribute__((packed)) iSCSIDMsgIsPortalActiveCmd;

/*! Default initialization for command to test whether portal is active. */
extern const iSCSIDMsgIsPortalActiveCmd iSCSIDMsgIsPortalActiveCmdInit;

/*! Response to command to test whether portal is active. */
typedef struct __iSCSIDMsgIsPortalActiveRsp {

    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 active;
    UInt16 reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgIsPortalActiveRsp;


/*! Command to query target for authentication method. */
typedef struct __iSCSIDMsgQueryTargetForAuthMethodCmd {
    
    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    CFLength  portalLength;
    CFLength  targetLength;
    UInt32  reserved4;
} __attribute__((packed)) iSCSIDMsgQueryTargetForAuthMethodCmd;

/*! Default initialization for a portal query command. */
extern const iSCSIDMsgQueryTargetForAuthMethodCmd iSCSIDMsgQueryTargetForAuthMethodCmdInit;

/*! Response to query a portal for authentication method. */
typedef struct __iSCSIDMsgQueryTargetForAuthMethodRsp {
    
    const UInt8 funcCode;
    UInt8 reserved;
    UInt32 errorCode;
    UInt16  statusCode;
    UInt32 reserved2;
    UInt32 reserved3;
    UInt32 authMethod;
    CFLength dataLength;
    
} __attribute__((packed)) iSCSIDMsgQueryTargetForAuthMethodRsp;

/*! Command to get information about a session. */
typedef struct __iSCSIDMsgCreateCFPropertiesForSessionCmd {
    
    const UInt16 funcCode;
    UInt16  reserved;
    CFLength  targetLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    
} __attribute__((packed)) iSCSIDMsgCreateCFPropertiesForSessionCmd;

/*! Default initialization for a get session information command. */
extern const iSCSIDMsgCreateCFPropertiesForSessionCmd iSCSIDMsgCreateCFPropertiesForSessionCmdInit;

/*! Response to command to get information about a session. */
typedef struct __iSCSIDMsgCreateCFPropertiesForSessionRsp {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;
    
} __attribute__((packed)) iSCSIDMsgCreateCFPropertiesForSessionRsp;

/*! Command to get information about a connection. */
typedef struct __iSCSIDMsgCreateCFPropertiesForConnectionCmd {
    
    const UInt16 funcCode;
    UInt16  reserved;
    CFLength  targetLength;
    CFLength  portalLength;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    
} __attribute__((packed)) iSCSIDMsgCreateCFPropertiesForConnectionCmd;

/*! Default initialization for a get connection information command. */
extern const iSCSIDMsgCreateCFPropertiesForConnectionCmd iSCSIDMsgCreateCFPropertiesForConnectionCmdInit;

/*! Response to command to get information about a connection. */
typedef struct __iSCSIDMsgCreateCFPropertiesForConnectionRsp {
    
    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgCreateCFPropertiesForConnectionRsp;


/*! Command update discovery. */
typedef struct __iSCSIDMsgUpdateDiscoveryCmd {

    const UInt16 funcCode;
    UInt16  reserved;
    UInt32  reserved2;
    UInt32  reserved3;
    UInt32  reserved4;
    UInt32  reserved5;
    UInt32  reserved6;

} __attribute__((packed)) iSCSIDMsgUpdateDiscoveryCmd;

/*! Default initialization update discovery command. */
extern const iSCSIDMsgUpdateDiscoveryCmd iSCSIDMsgUpdateDiscoveryCmdInit;

/*! Response to command update discovery. */
typedef struct __iSCSIDMsgUpdateDiscoveryRsp {

    const UInt8 funcCode;
    UInt16 reserved;
    UInt32 errorCode;
    UInt8  reserved2;
    UInt32 reserved3;
    UInt32 reserved4;
    UInt32 reserved5;
    CFLength dataLength;

} __attribute__((packed)) iSCSIDMsgUpdateDiscoveryRsp;


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

/*! Helper function.  Sends an iSCSI daemon command followed by optional
 *  data in the form of CFDataRef objects. */
errno_t iSCSIDaemonSendMsg(int fd,iSCSIDMsgGeneric * msg,...)
{
    va_list argList;
    va_start(argList,msg);

    struct iovec iov[IOV_MAX];
    int iovIdx = 0;

    iov[iovIdx].iov_base = msg;
    iov[iovIdx].iov_len = sizeof(iSCSIDMsgGeneric);
    iovIdx++;

    // Iterate over data and add each one to the message
    CFDataRef data = NULL;
    UInt32 totalLength = sizeof(iSCSIDMsgGeneric);
    while((data = va_arg(argList,CFDataRef)))
    {
        CFIndex length = CFDataGetLength(data);
        iov[iovIdx].iov_base = (void*)CFDataGetBytePtr(data);
        iov[iovIdx].iov_len = length;
        totalLength += length;
        iovIdx++;
    }
    va_end(argList);

    struct msghdr message;
    bzero(&message,sizeof(struct msghdr));
    message.msg_iov = iov;
    message.msg_iovlen = iovIdx;

    if(sendmsg(fd,&message,0) != totalLength)
       return EIO;

    return 0;
}

errno_t iSCSIDaemonRecvMsg(int fd,iSCSIDMsgGeneric * msg,...)
{
    va_list argList;
    va_start(argList,msg);

    struct iovec iov[IOV_MAX];
    int iovIdx = 0;

    // If the message (header) has already been retrieve, and this function
    // is being called only to retrieve data then ignore the message
    if(msg) {
        iov[iovIdx].iov_base = msg;
        iov[iovIdx].iov_len = sizeof(iSCSIDMsgGeneric);
        iovIdx++;
    }

    // Iterate over data and add each one to the message
    CFMutableDataRef * data = NULL;
    while((data = va_arg(argList,CFMutableDataRef *)))
    {
        UInt32 length = va_arg(argList,UInt32);

        if(length == 0)
            continue;

        *data = CFDataCreateMutable(kCFAllocatorDefault,length);
        CFDataSetLength(*data,length);

        iov[iovIdx].iov_base = (void*)CFDataGetMutableBytePtr(*data);
        iov[iovIdx].iov_len = length;
        iovIdx++;
    }
    va_end(argList);

    struct msghdr message;
    bzero(&message,sizeof(struct msghdr));
    message.msg_iov = iov;
    message.msg_iovlen = iovIdx;

    if(recvmsg(fd,&message,0) < sizeof(msg))
        return EIO;

    return 0;
}


#endif /* defined(__ISCSI_DAEMON_INTERFACE_SHARED_H__) */
