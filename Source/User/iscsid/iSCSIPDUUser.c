/*
 * Copyright (c) 2016, Nareg Sinenian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "iSCSIPDUUser.h"


const iSCSIPDULogoutReqBHS iSCSIPDULogoutReqBHSInit = {
    .opCodeAndDeliveryMarker = (kiSCSIPDUOpCodeLogoutReq | kiSCSIPDUImmediateDeliveryFlag) };

const iSCSIPDUTextReqBHS iSCSIPDUTextReqBHSInit = {
    .opCodeAndDeliveryMarker    = (kiSCSIPDUOpCodeTextReq | kiSCSIPDUImmediateDeliveryFlag),
    .textReqStageFlags          = 0,
    .reserved                   = 0,
    .totalAHSLength             = 0,
    .LUNorOpCodeFields          = 0,
    .initiatorTaskTag           = 0,
    .reserved2                  = 0,
    .reserved3                  = 0
};

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


/*! Bit offsets here start with the low-order bit (e.g., a 0 here corresponds
 *  to the LSB and would correspond to bit 7 if the data was in big-endian
 *  format (this representation is endian neutral with bitwise operators). */

/*! Next login stage bit offset of the login stage byte. */
const unsigned short kiSCSIPDULoginNSGBitOffset = 0;

/*! Current login stage bit offset of the login stage byte. */
const unsigned short kiSCSIPDULoginCSGBitOffset = 2;

/*! Continue the current stage bit offset. */
const UInt8 kiSCSIPDULoginContinueFlag = 0x40;

/*! Transit to next stage bit offset. */
const UInt8 kiSCSIPDULoginTransitFlag = 0x80;

///////////////////////////  LOGOUT BHS DEFINITIONS ////////////////////////////
// This section various constants that are used only for the logout PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.


/*! Flag that must be applied to the reason code byte of the logout PDU. */
const unsigned short kISCSIPDULogoutReasonCodeFlag = 0x80;


////////////////////////  TEXT REQUEST BHS DEFINITIONS /////////////////////////
// This section various constants that are used only for the logout PDU.
// Definitions that are used for more than one type of PDU can be found in
// the header iSCSIPDUShared.h.

/*! Bit offset for the final bit indicating this is the last PDU in the
 *  text request. */
const unsigned short kiSCSIPDUTextReqFinalFlag = 0x80;

/*! Bit offset for the continue bit indicating more text commands are to
 *  follow for this text request. */
const unsigned short kiSCSIPDUTextReqContinueFlag = 0x40;

void iSCSIPDUDataParseCommon(void * data,size_t length,
                             void * keyContainer,
                             void * valContainer,
                             void (*callback)(void * keyContainer,CFStringRef key,
                                              void * valContainer,CFStringRef val))
{
    if(!data || length == 0 || !callback)
        return;
    
    // Parse the text response
    UInt8 * currentByte = data;
    
    UInt8 * lastByte = currentByte + length;
    UInt8 * tokenStartByte = currentByte;
    
    CFStringRef keyString = NULL, valString = NULL;
    
    bool equalFound = false;
    
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
            
            // We've crossed from key to value
            equalFound = true;
        }
        // Second boolean expression is required for the case of null-padded
        // datasegments (per RFC3720 PDUs are padded up to the nearest word)
        else if(*currentByte == 0 && equalFound)
        {
            valString = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                tokenStartByte,
                                                currentByte-tokenStartByte,
                                                kCFStringEncodingUTF8,false);
            // Advance the starting point to skip the '='
            tokenStartByte = currentByte + 1;
            (*callback)(keyContainer,keyString,valContainer,valString);

            CFRelease(keyString);
            CFRelease(valString);
            keyString = valString = NULL;
            
            // Reset for next key-value pair (this allows extra 0's for padding
            // if the string should contain any)
            equalFound = false;
        }
        currentByte++;
    }

    if(keyString) {
        CFRelease(keyString);
        keyString = NULL;
    }
    
    if(valString) {
        CFRelease(valString);
        valString = NULL;
    }
}

void iSCSIPDUDataParseToDictCallback(void * keyContainer,CFStringRef keyString,
                                     void * valContainer,CFStringRef valString)
{
    CFDictionaryAddValue(keyContainer,keyString,valString);
}


/*! Parses key-value pairs to a dictionary.
 *  @param data the data segment (from a PDU) to parse.
 *  @param length the length of the data segment.
 *  @param textDict a dictionary of key-value pairs. */
void iSCSIPDUDataParseToDict(void * data,size_t length,CFMutableDictionaryRef textDict)
{
    if(!data || length == 0 || !textDict)
        return;
    
    iSCSIPDUDataParseCommon(data,length,textDict,textDict,&iSCSIPDUDataParseToDictCallback);
}

void iSCSIPDUDataParseToArraysCallback(void * keyContainer,CFStringRef keyString,
                                       void * valContainer,CFStringRef valString)
{
    CFArrayAppendValue(keyContainer,keyString);
    CFArrayAppendValue(valContainer,valString);
}

/*! Parses key-value pairs to two arrays. This is useful for situations where
 *  the data segment may contain duplicate key names.
 *  @param data the data segment (from a PDU) to parse.
 *  @param length the length of the data segment.
 *  @param keys an array of key values.
 *  @param values an array of corresponding values for each key. */
void iSCSIPDUDataParseToArrays(void * data,size_t length,CFMutableArrayRef keys,CFMutableArrayRef values)
{
    if(!data || length == 0 || !values)
        return;
    
    iSCSIPDUDataParseCommon(data,length,keys,values,&iSCSIPDUDataParseToDictCallback);
}



/*! Struct used to track the position of the PDU data segment being written. */
typedef struct iSCSIPDUDataSegmentTracker  {
    UInt8 * dataSegmentPosition;
} iSCSIPDUDataSegmentTracker;

/*! Helper function used when creating login, logout and text request PDUs.
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

/*! Helper function used when creating login, logout and text request PDUs.
 *  This function populates the data segment of a login, logout, or text
 *  request PDUs with the key-value pairs specified by a dictionary.
 *  @param key
 *  @param value
 *  @param posTracker */
void iSCSIPDUPopulateWithTextCommand(const void * key,
                                     const void * value,
                                     void * posTracker)
{
    const int MAX_KEY_SIZE = 100;
    const int MAX_VAL_SIZE = 100;
    
    // Ensure pointer to data segment is valid
    if(posTracker)
    {
        CFIndex keyByteSize = CFStringGetLength((CFStringRef)key);
        CFIndex valueByteSize = CFStringGetLength((CFStringRef)value);
        CFStringEncoding stringEncoding = kCFStringEncodingUTF8;
        
        char keyCString[MAX_KEY_SIZE];
        char valueCString[MAX_VAL_SIZE];
        
        Boolean validKeyCString = CFStringGetCString(key,keyCString,MAX_KEY_SIZE,stringEncoding);
        Boolean validValueCString = CFStringGetCString(value,valueCString,MAX_VAL_SIZE,stringEncoding);
        
        // If both strings are valid C-strings, copy them into the PDU
        if(validKeyCString && validValueCString)
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

/*! Creates a PDU data segment consisting of key-value pairs from a dictionary.
 *  @param textDict the user-specified dictionary to use.
 *  @param data a pointer to a pointer the data, returned by this function.
 *  @param length the length of the data block, returned by this function. */
void iSCSIPDUDataCreateFromDict(CFDictionaryRef textDict,void ** data,size_t * length)
{
    if(!length || !data || !textDict)
        return;
    
    // Apply a function to key-value pairs to determine total byte size of
    // the text commands
    CFIndex cmdByteSize = 0;
    CFDictionaryApplyFunction(textDict,
                              &iSCSIPDUCalculateTextCommandByteSize,
                              &cmdByteSize);

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

/*! Creates a PDU data segment of the specified size.
 *  @param length the byte size of the data segment. */
void * iSCSIPDUDataCreate(size_t length)
{
    length += (kiSCSIPDUByteAlignment - (length % kiSCSIPDUByteAlignment));
    return malloc(length);
}

/*! Releases a PDU data segment created using an iSCSIPDUDataCreate... function.
 *  @param data pointer to the data segment pointer to release. */
void iSCSIPDUDataRelease(void * * data)
{
    if(data && *data)
    {
        free(*data);
        *data = NULL;
    }
}