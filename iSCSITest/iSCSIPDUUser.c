/**
 * @author		Nareg Sinenian
 * @file		iSCSIPDUUser.h
 * @date		April 8, 2014
 * @version		1.0
 * @copyright	(c) 2013-2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI PDU functions.  These functions cannot be used
 *              within the kernel and are intended for use within a daemon on
 *              Mac OS.  They make extensive use of Core Foundation and allow
 *              for allocation, deallocation, transmission and reception of
 *              iSCSI PDU components, including definitions of basic header
 *              segments for various PDUs and data segments.
 */

#include "iSCSIPDUUser.h"

const iSCSIPDULogoutReqBHS iSCSIPDULogoutReqBHSInit = {
    .opCodeAndDeliveryMarker = (kiSCSIPDUOpCodeLogoutReq | kiSCSIPDUImmediateDeliveryFlag) };

const iSCSIPDUTextReqBHS iSCSIPDUTextReqBHSInit = {
    .opCodeAndDeliveryMarker = (kiSCSIPDUOpCodeTextReq | kiSCSIPDUImmediateDeliveryFlag) };

const iSCSIPDULoginReqBHS iSCSIPDULoginReqBHSInit = {
    .opCodeAndDeliveryMarker = (kiSCSIPDUOpCodeLoginReq | kiSCSIPDUImmediateDeliveryFlag),
    .loginStage = 0,
    .versionMax = 0,
    .versionMin = 0,
    .ISIDa = 0x80,    // Use "random" format for ISID
    .ISIDb = 0x000,   // This comes from the initiator (kernel)
    .ISIDc = 0x000};  // This comes from the initiator (kernel)


////////////////////////////  LOGIN BHS DEFINITIONS ////////////////////////////
// This section various constants that are used only for the login PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.


/** Bit offsets here start with the low-order bit (e.g., a 0 here corresponds
 *  to the LSB and would correspond to bit 7 if the data was in big-endian
 *  format (this representation is endian neutral with bitwise operators). */

/** Next login stage bit offset of the login stage byte. */
const unsigned short kiSCSIPDULoginNSGBitOffset = 0;

/** Current login stage bit offset of the login stage byte. */
const unsigned short kiSCSIPDULoginCSGBitOffset = 2;

/** Continue the current stage bit offset. */
const UInt8 kiSCSIPDULoginContinueFlag = 0x40;

/** Transit to next stage bit offset. */
const UInt8 kiSCSIPDULoginTransitFlag = 0x80;

///////////////////////////  LOGOUT BHS DEFINITIONS ////////////////////////////
// This section various constants that are used only for the logout PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.


/** Flag that must be applied to the reason code byte of the logout PDU. */
const unsigned short kISCSIPDULogoutReasonCodeFlag = 0x80;


////////////////////////  TEXT REQUEST BHS DEFINITIONS /////////////////////////
// This section various constants that are used only for the logout PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.

/** Bit offset for the final bit indicating this is the last PDU in the
 *  text request. */
const unsigned short kiSCSIPDUTextReqFinalFlag = 0x80;

/** Bit offset for the continue bit indicating more text commands are to
 *  follow for this text request. */
const unsigned short kiSCSIPDUTextReqContinueFlag = 0x40;

/** Parses key-value pairs to a dictionary.
 *  @param data the data segment (from a PDU) to parse.
 *  @param length the length of the data segment.
 *  @param textDict a dictionary of key-value pairs. */
void iSCSIPDUDataParseToDict(void * data,size_t length,CFMutableDictionaryRef textDict)
{
    if(!data || length == 0)
        return;   
    
    // Parse the text response
    UInt8 * currentByte = data;

    UInt8 * lastByte = currentByte + length;
    UInt8 * tokenStartByte = currentByte;

    CFStringRef keyString, valueString;
    
    // Search through bytes and look for key=value pairs.  Convert key and
    // value strings to CFStrings and add to a dictionary
    while(currentByte <= lastByte)
    {
        if(*currentByte == '=')
        {
            keyString = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                tokenStartByte,
                                                currentByte-tokenStartByte,
                                                kCFStringEncodingUTF8,false);
            // Advance the starting point to skip the '='
            tokenStartByte = currentByte + 1;
        }
        // Second boolean expression is required for the case of null-padded
        // datasegments (per RFC3720 PDUs are padded up to the nearest word)
        else if(*currentByte == 0) // && *tokenStartByte != 0)
        {
            valueString = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                  tokenStartByte,
                                                  currentByte-tokenStartByte,
                                                  kCFStringEncodingUTF8,false);
            // Advance the starting point to skip the '='
            tokenStartByte = currentByte + 1;
            CFDictionaryAddValue(textDict,keyString,valueString);
        }
        currentByte++;
    }
}

/** Struct used to track the position of the PDU data segment being written. */
typedef struct iSCSIPDUDataSegmentTracker  {
    UInt8 * dataSegmentPosition;
} iSCSIPDUDataSegmentTracker;

/** Helper function used when creating login, logout and text request PDUs.
 *  This function calculates the total string length (byte size since the
 *  strings are UTF-8 encoded) of the key-value pairs in a dictionary.
 *  It also adds to the length for each '=' required for each
 *  key-value pair and the appropriate null terminators per RFC3720. */
void iSCSIPDUCalculateTextCommandByteSize(const void * key,
                                          const void * value,
                                          void * stringByteSize)
{
    // Ensure counter is valid
    if(stringByteSize)
    {
        // Count the length of the key and value strings, and for each pair add
        // 2 to include the length of the '=' (equality between key-value pair)
        // and '\0' (null terminator)
        (*(CFIndex*)stringByteSize) = (*(CFIndex*)stringByteSize)
            + CFStringGetLength((CFStringRef)key)
            + CFStringGetLength((CFStringRef)value)
            + 2;
    }
}

/** Helper function used when creating login, logout and text request PDUs.
 *  This function populates the data segment of a login, logout, or text
 *  request PDUs with the key-value pairs specified by a dictionary.
 *  @param key
 *  @param value
 *  @param posTracker */
void iSCSIPDUPopulateWithTextCommand(const void * key,
                                     const void * value,
                                     void * posTracker)
{
    // Ensure pointer to data segment is valid
    if(posTracker)
    {
        CFIndex keyByteSize = CFStringGetLength((CFStringRef)key);
        CFIndex valueByteSize = CFStringGetLength((CFStringRef)value);
        CFStringEncoding stringEncoding = kCFStringEncodingUTF8;
        
        const char * keyCString =
            CFStringGetCStringPtr((CFStringRef)key,stringEncoding);
        const char * valueCString =
            CFStringGetCStringPtr((CFStringRef)value,stringEncoding);
        
        // If both strings are valid C-strings, copy them into the PDU
        if(keyCString && valueCString)
        {
            iSCSIPDUDataSegmentTracker * position =
                (iSCSIPDUDataSegmentTracker*)posTracker;
            
            // Copy key
            memcpy(position->dataSegmentPosition,keyCString,keyByteSize);
            position->dataSegmentPosition =
                position->dataSegmentPosition + keyByteSize;
            
            // Add '='
            memset(position->dataSegmentPosition,'=',1);
            position->dataSegmentPosition =
                position->dataSegmentPosition + 1;
            
            // Copy value
            memcpy(position->dataSegmentPosition,valueCString,valueByteSize);
            position->dataSegmentPosition =
                position->dataSegmentPosition + valueByteSize;
            
            // Add null terminator
            memset(position->dataSegmentPosition,0,1);
            position->dataSegmentPosition = position->dataSegmentPosition + 1;
        }
    }
}

/** Creates a PDU data segment consisting of key-value pairs from a dictionary.
 *  @param textDict the user-specified dictionary to use.
 *  @param data a pointer to a pointer the data, returned by this function.
 *  @param length the length of the data block, returned by this function. */
void iSCSIPDUDataCreateFromDict(CFDictionaryRef textDict,void * * data,size_t * length)
{
    if(!length || !data)
        return;
    
    // Apply a function to key-value pairs to determine total byte size of
    // the text commands
    CFIndex cmdByteSize = 0;
    CFDictionaryApplyFunction(textDict,
                              &iSCSIPDUCalculateTextCommandByteSize,
                              &cmdByteSize);
    
    // Add padding to command byte size
    cmdByteSize += (4 - (cmdByteSize % 4));
    
    if((*data = malloc(cmdByteSize)) == NULL) {
        *length = 0;
        return;
    }
    
    *length = cmdByteSize;
    
    iSCSIPDUDataSegmentTracker posTracker;
    posTracker.dataSegmentPosition = *data;
    
    // Apply a function to iterate over key-value pairs and add them to this PDU
    CFDictionaryApplyFunction(textDict,&iSCSIPDUPopulateWithTextCommand,&posTracker);
}

/** Creates a PDU data segment of the specified size.
 *  @param length the byte size of the data segment. */
void * iSCSIPDUDataCreate(size_t length)
{
    length += (4 - (length % 4));
    return malloc(length);
}

/** Releases a PDU data segment created using an iSCSIPDUDataCreate... function.
 *  @param data pointer to the data segment pointer to release. */
void iSCSIPDUDataRelease(void * * data)
{
    if(data && *data)
    {
        free(*data);
        *data = NULL;
    }
}


/*
iSCSIPDULoginReqRef iSCSIPDUCreateLoginRequest(
                            UInt16 sessionQualifier,
                            UInt32 connectionId,
                            enum iSCSIPDULoginStages currentStage,
                            enum iSCSIPDULoginStages nextStage,
                            UInt16 targetSessionId,
                            UInt32 initiatorTaskTag,
                            CFDictionaryRef textCmds,
                            bool transitNextStage)
{
    // Allocate a new PDU and populate the data segment with the login
    // commands (the datasegment size is also populated accordingly)
    iSCSIPDULoginReqRef thePDU =
        (iSCSIPDULoginReqRef)iSCSIPDUCreateWithTextCommands(textCmds,false,false);
    
    if(!thePDU)
        return NULL;
    
    // Setup the opcode; add immediate delivery marker
    thePDU->basicHeaderSegment.opCodeAndDeliveryMarker =
        kiSCSIPDUOpCodeLoginReq | kiSCSIPDUImmediateDeliveryFlag;
    
    // Set the transit, continue and login stage bits
    thePDU->basicHeaderSegment.loginStage =
        (UInt8)currentStage << kiSCSIPDULoginCSGBitOffset   |
        (UInt8)nextStage    << kiSCSIPDULoginNSGBitOffset;
    
    if(transitNextStage)
        thePDU->basicHeaderSegment.loginStage |= kiSCSIPDULoginTransitFlag;
    
    // Set version to 0x00 per first iSCSI draft - see RFC3720
    thePDU->basicHeaderSegment.versionMax = 0x00;
    thePDU->basicHeaderSegment.versionMin = 0x00;
     
    // Set the additional header segment field (no AHS for login PDUs)
    thePDU->basicHeaderSegment.totalAHSLength = 0x00;
    
    // Code ISID 'a' field to indicate that we're not using OUI/IANA
    thePDU->basicHeaderSegment.ISIDa = 0x02;
    
    // Generate a random number for the 'b' and 'c' fields of the ISID
    thePDU->basicHeaderSegment.ISIDb = 0x001;
    thePDU->basicHeaderSegment.ISIDc = 0x002;
    thePDU->basicHeaderSegment.ISIDd = CFSwapInt16HostToBig(sessionQualifier);
    
    // Set TSIH, ITT and CID
    thePDU->basicHeaderSegment.TSIH = CFSwapInt16HostToBig(targetSessionId);
    thePDU->basicHeaderSegment.initiatorTaskTag = CFSwapInt32HostToBig(initiatorTaskTag);
    thePDU->basicHeaderSegment.CID = CFSwapInt16HostToBig(connectionId);
    
    return thePDU;
}


iSCSIPDULogoutReqRef iSCSIPDUCreateLogoutRequest(
                                    UInt16 sessionQualifier,
                                    UInt32 connectionId,
                                    UInt32 initiatorTaskTag,
                                    enum iSCSIPDULogoutReasons reasonCode,
                                    iSCSIConnectionOptions * connOpts)

{
    if(connectionId == kiSCSIInvalidConnectionId || !connOpts)
        return NULL;

    // Allocate a new PDU with no data, additional header, header digest (opt).
    iSCSIPDULogoutReqRef thePDU =
        (iSCSIPDULogoutReqRef)iSCSIPDUAlloc(0,0,connOpts->useHeaderDigest,0);
    
    // Setup the opcode; add immediate delivery marker
    thePDU->basicHeaderSegment.opCodeAndDeliveryMarker =
        kiSCSIPDUOpCodeLogoutReq | kiSCSIPDUImmediateDeliveryFlag;
    
    // Set reason code, initiator task tag & connection ID
    thePDU->basicHeaderSegment.reasonCode = reasonCode | kISCSIPDULogoutReasonCodeFlag;
    thePDU->basicHeaderSegment.initiatorTaskTag = CFSwapInt32HostToBig(initiatorTaskTag);
    thePDU->basicHeaderSegment.CID = CFSwapInt16HostToBig(connectionId);
    
    return thePDU;
}

iSCSIPDUTextReqRef iSCSIPDUCreateTextRequest(CFDictionaryRef textCmds,
                                             iSCSIConnectionOptions * connOpts,
                                             UInt32 initiatorTaskTag,
                                             UInt32 targetTransferTag)

{
    if(!connOpts)
        return NULL;
    
    // Allocate a new PDU and populate the data segment with the login
    // commands (the datasegment size is also populated accordingly)
    iSCSIPDUTextReqRef thePDU = (iSCSIPDUTextReqRef)iSCSIPDUCreateWithTextCommands(
        textCmds,
        connOpts->useHeaderDigest,
        connOpts->useDataDigest);
    
    if(!thePDU)
        return NULL;

    // Setup the opcode; add immediate delivery marker
    thePDU->basicHeaderSegment.opCodeAndDeliveryMarker =
        kiSCSIPDUOpCodeTextReq | kiSCSIPDUImmediateDeliveryFlag;
    
    // Set final request flag; we don't allow incomplete text PDUs
    thePDU->basicHeaderSegment.textReqStageFlags = kiSCSIPDUTextReqFinalFlag;
    
    // Set no AHS for text PDUs, initiator task tag & target transfer
    thePDU->basicHeaderSegment.totalAHSLength = 0x00;
    thePDU->basicHeaderSegment.initiatorTaskTag = CFSwapInt32HostToBig(initiatorTaskTag);
    thePDU->basicHeaderSegment.targetTransferTag = CFSwapInt32HostToBig(targetTransferTag);
    
    return thePDU;
}

bool iSCSIPDUParseLoginResponse(iSCSIPDULoginRspRef thePDU,
                                bool  * finalLoginRspInStage,
                                enum iSCSIPDULoginRspStatusClass * statusClass,
                                enum iSCSIPDULoginRspStatusDetail * statusDetail,
                                UInt16 * targetSessionId,
                                CFMutableDictionaryRef textRsp)
{
    // Validate the PDU and parameters
    if(!thePDU)
        return false;

    if(finalLoginRspInStage)
        *finalLoginRspInStage = !(thePDU->basicHeaderSegment.loginStage & kiSCSIPDULoginContinueFlag);
    
    if(statusClass)
        *statusClass = (enum iSCSIPDULoginRspStatusClass)thePDU->basicHeaderSegment.statusClass;
    
    if(statusDetail)
        *statusDetail = (enum iSCSIPDULoginRspStatusDetail)thePDU->basicHeaderSegment.statusDetail;
    
    if(targetSessionId)
        *targetSessionId = CFSwapInt16BigToHost(thePDU->basicHeaderSegment.TSIH);
    
    // Parse data into a dictionary of key-value pairs and return
    iSCSIPDUParseTextDataToDict((iSCSIPDUTargetRef)thePDU,textRsp);

    return true;
}
    
bool iSCSIPDUParseLogoutResponse(iSCSIPDULogoutRspRef thePDU,
                                 enum iSCSIPDULogoutRsp * response,
                                 UInt16 * time2Wait,
                                 UInt16 * time2Retain)

{
    // Validate the PDU and parameters
    if(!thePDU)
        return false;

    if(response)
        *response = (enum iSCSIPDULogoutRsp)thePDU->basicHeaderSegment.response;

    if(time2Wait)
        *time2Wait = CFSwapInt16BigToHost(thePDU->basicHeaderSegment.time2Wait);
    
    if(time2Retain)
        *time2Retain = CFSwapInt16BigToHost(thePDU->basicHeaderSegment.time2Retain);
    
    return true;
}
    
bool iSCSIPDUParseTextResponse(iSCSIPDUTextRspRef thePDU,
                               bool * finalTextRsp,
                               CFMutableDictionaryRef textRsp)
{
    // Validate PDU and parameters
    if(!thePDU)
        return false;
    
    if(finalTextRsp)
        *finalTextRsp = (thePDU->basicHeaderSegment.textReqStageBits & kiSCSIPDUTextReqFinalFlag);
    
    if(textRsp)
        iSCSIPDUParseTextDataToDict((iSCSIPDUTargetRef)thePDU,textRsp);

    return true;
}
    
void iSCSIPDURelease(iSCSIPDURef thePDU)
{
    // If the PDU is valid, free header & data segments
    if(!thePDU)
        return;
    
    free(thePDU);
}
*/
