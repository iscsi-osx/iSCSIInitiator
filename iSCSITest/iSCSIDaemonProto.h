//
//  File.c
//  iSCSI_Initiator
//
//  Created by Nareg Sinenian on 3/15/14.
//
//

#include <stdio.h>

const unsigned int kiSCSISessionMaxStringSize = 100;

/** Session options that are passed in when creating a new session.  */
struct iSCSISessionOpts {
    
    /** The initiator name to use. */
    UInt8 initiatorName[kiSCSISessionMaxStringSize];
    
    /** The target name to use. */
    UInt8 initiatorAlias[kiSCSISessionMaxStringSize];
    
    /** Maximum number of connections allowed this session. */
    UInt16 maxConnections;
    
}  __attribute__((packed));

typedef struct iSCSISessionOpts iSCSISessionOpts;


// Authentication methods to be used by the iSCSIConnectionOpts struct.

/** No authentication. */
const UInt8 kiSCSIAuthMethodNone = 0;

/** CHAP authentication. */
const UInt8 kiSCSIAuthMethodCHAP = 1;

/** Connection options that are passed in when creating a new connection. */
struct iSCSIConnectionOpts {
    
    /** Authentication method to use. */
    UInt8 authMethod;
    
    /** Whether to use a header digest (CRC32C is used if enabled). */
    UInt8 useHeaderDigest;
    
    /** Whether to use a data digest (CRC32C is used if enabled). */
    UInt8 useDataDigest;
    
    /** The host IP address to bind the target name to use. */
    UInt8 hostAddress[kiSCSISessionMaxStringSize];
    
    /** The target name to use. */
    UInt8 targetAddress[kiSCSISessionMaxStringSize];
    
    /** The TCP port to use. */
    UInt32 targetPort;
    
} __attribute__((packed));


/** Detailed login respones from a target that supplement the
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