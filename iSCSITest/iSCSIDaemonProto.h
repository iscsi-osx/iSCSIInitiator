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