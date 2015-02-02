//
//  iSCSIDaemonInterfaceShared.h
//  iSCSI_Initiator
//
//  Created by Nareg Sinenian on 1/12/15.
//
//

#ifndef iSCSI_Initiator_iSCSIDaemonInterfaceShared_h
#define iSCSI_Initiator_iSCSIDaemonInterfaceShared_h

#include "iSCSITypes.h"
#include "iSCSITypesShared.h"

#define MAX_TARGET_NAME_LENGTH 256

/*! Command to login a particular session. */
struct iSCSIDCmdLoginSession {
    
    const UInt8 funcCode;
    UInt32  portalLength;
    UInt32  targetLength;
    UInt32  authLength;
    
} __attribute__((packed));

/*! Response to a session login command. */
struct iSCSIDRspLoginSession {
    
    const UInt8 funcCode;
    UInt8 iSCSIStatusCode;
    UInt16 sessionId;
    
} __attribute__((packed));



/*! Command to logout a particular session. */
struct iSCSIDCmdLogoutSession {
    
    const UInt8 funcCode;
    UInt16 sessionId;
    
} __attribute__((packed));

/*! Response to a session logout command. */
struct iSCSIDRspLogoutSession {
    
    const UInt8 funcCode;
    UInt8 iSCSIStatusCode;
    
} __attribute__((packed));



/*! Command to add a connection. */
struct iSCSIDCmdAddConnection {
    
    const UInt8 funcCode;
    UInt8 sessionId;
    UInt32 portalLength;
    UInt32 targetLength;
    UInt32 authLength;
    
} __attribute__((packed));

/*! Response to add an connection command. */
struct iSCSIDRspAddConnection {
    
    const UInt8 funcCode;
    UInt32 connectionId;
    
} __attribute__((packed));



/*! Command to query a portal for targets. */
struct iSCSIDCmdQueryPortalForTargets {
    
    const UInt8 funcCode;
    UInt32 portalLength;
    
} __attribute__((packed));

/*! Command to query a portal for targets. */
struct iSCSIDRspQueryPortalForTargets {
    
    const UInt8 funcCode;
    UInt8 iSCSIStatusCode;
    UInt32 targetsLength;
    
} __attribute__((packed));



/*! Command to list session identifiers. */
struct iSCSIDCmdGetSessionIds {
    
    const UInt8 funcCode;
    
} __attribute__((packed));

/*! Response to command to list session identifiers. */
struct iSCSIDRspGetSessionIds {
    
    const UInt8 funcCode;
    UInt8 iSCSIStatusCode;
    
    
} __attribute__((packed));



/*! Command to get information about a session. */
struct iSCSIDCmdGetSessionInfo {
    
    const UInt8 funcCode;
    UInt16 sessionId;
    
} __attribute__((packed));

/*! Response to command to get information about a session. */
struct iSCSIDRspGetSessionInfo {
    
    const UInt8 funcCode;
    UInt8 iSCSIStatusCode;
    
} __attribute__((packed));





struct iSCSIDCmdLoginAllSessions {
    const UInt8 cmdCode;
};

struct iSCSIDCmdLogoutAllSessions {
    const UInt8 cmdCode;
};

////////////////////////////// DAEMON FUNCTIONS ////////////////////////////////

enum iSCSIDaemonFunctionCodes {
    
    kiSCSIDaemonLoginSession = 0,
    kiSCSIDaemonLogoutSession = 1,
    kiSCSIDaemonAddConnection = 2,
    kiSCSIDaemonRemoveConnection = 3,
    kiSCSIDaemonQueryPortalForTargets = 4,
    kiSCSIDaemonQueryTargetForAuthMethods = 5,
    kiSCSIDaemonGetSessionIdForTarget = 6,
    kiSCSIDaemonSetInitiatorName = 7,
    kiSCSIDaemonSetInitiatorAlias = 8
};


#endif
