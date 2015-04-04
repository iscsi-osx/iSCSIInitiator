/*!
 * @author		Nareg Sinenian
 * @file		iSCSIUtils.h
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI utility functions.
 */

#include "iSCSIUtils.h"

/*! Verifies whether specified iSCSI qualified name (IQN) is valid per RFC3720.
 *  This function also validates 64-bit EUI names expressed as strings that
 *  start with the "eui" prefix.
 *  @param IQN the iSCSI qualified name.
 *  @return true if the name is valid, false otherwise. */
Boolean iSCSIUtilsValidateIQN(CFStringRef IQN)
{
    // IEEE regular expression for matching IQN name
    const char pattern[] =  "^iqn\.[0-9]\{4\}-[0-9]\{2\}\.[[:alnum:]]\{3\}\."
                            "[A-Za-z0-9.]\{1,255\}:[A-Za-z0-9.]\{1,255\}"
                            "|^eui\.[[:xdigit:]]\{16\}$";
    
    Boolean validName = false;
    regex_t preg;
    regcomp(&preg,pattern,REG_EXTENDED | REG_NOSUB);
    
    if(regexec(&preg,CFStringGetCStringPtr(IQN,kCFStringEncodingASCII),0,NULL,0) == 0)
        validName = true;
    
    regfree(&preg);
    return validName;
}

/*! Validates and parses an expression of the form <host>:<port> into its
 *  hostname (or IPv4/IPv6 address) and port.  This function will return
 *  NULL if the specified expression is malformed, or an array containing
 *  either one or two elements (one if the portal is absent, two if it was
 *  specified.
 *  @param portal a string of the form <host>:<port>
 *  @return an array containing one or both portal parts, or NULL if the
 *  specified portal was malformed. */
CFArrayRef iSCSIUtilsCreateArrayByParsingPortalParts(CFStringRef portal)
{
    // Regular expressions to match valid IPv4, IPv6 and DNS portal strings
    const char IPv4Pattern[] = "^((((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|([0-9])?[0-9])[.])\{3\}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|([0-9])?[0-9]))(:([0-9]\{1,5\}))?)$";
    
    const char IPv6Pattern[] = "^([[]?(([A-Fa-f0-9]\{0,4\}:)\{1,7\}[A-Fa-f0-9]\{0,4\})([\]]:([0-9]\{1,5\})?)?)$";
    
    const char DNSPattern[] = "^((([A-Za-z0-9]\{1,63\}[.])\{1,3\}[A-Za-z0-9]\{1,63\})(:([0-9]\{1,5\}))?)$";
    
    // Array of patterns to iterate, the indices of the matches that
    // correspond to the hostname, port and the maximum number of matches
    // Start with IPv4 as that is the most restrictive pattern, then work
    // down to least restrictive (DNS names)
    const char * patterns[] = {IPv4Pattern, IPv6Pattern, DNSPattern};
    const int  maxMatches[] = {10, 6, 6};
    const int   hostIndex[] = {2, 2, 2};
    const int   portIndex[] = {9, 5, 5};
    
    int index = 0;
    
    do
    {
        regex_t preg;
        regcomp(&preg,patterns[index],REG_EXTENDED);
        
        regmatch_t matches[maxMatches[index]];
        memset(matches,0,sizeof(regmatch_t)*maxMatches[index]);
        
        // Match against pattern[index]
        if(regexec(&preg,CFStringGetCStringPtr(portal,kCFStringEncodingASCII),maxMatches[index],matches,0))
        {
            regfree(&preg);
            index++;
            continue;
        }
        
        CFMutableArrayRef portalParts = CFArrayCreateMutable(kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
        
        // Get the host name
        if(matches[hostIndex[index]].rm_so != -1)
        {
            CFRange rangeHost = CFRangeMake(matches[hostIndex[index]].rm_so,
                                            matches[hostIndex[index]].rm_eo - matches[hostIndex[index]].rm_so);
            CFStringRef host = CFStringCreateWithSubstring(kCFAllocatorDefault,
                                                           portal,rangeHost);
            
            CFArrayAppendValue(portalParts,host);
            CFRelease(host);
        }
        
        // Is the port available?  If so, set it...
        if(matches[portIndex[index]].rm_so != -1)
        {
            CFRange rangePort = CFRangeMake(matches[portIndex[index]].rm_so,
                                            matches[portIndex[index]].rm_eo - matches[portIndex[index]].rm_so);
            CFStringRef port = CFStringCreateWithSubstring(kCFAllocatorDefault,
                                                           portal,rangePort);
            CFArrayAppendValue(portalParts,port);
            CFRelease(port);
        }
        
        regfree(&preg);
        return portalParts;
        
    } while(index < sizeof(patterns)/sizeof(const char *));
    
    return NULL;
}

/*! Gets the SCSI peripheral description from a peripheral device type code.
 *  @param peripheralDeviceType the single byte peripheral device descriptor
 *  as outlined in the SPC-4 r36d.
 *  @return a string describing the device (guaranteed to be a valid string). */

CFStringRef iSCSIUtilsGetSCSIPeripheralDeviceDescription(UInt8 peripheralDeviceType)
{
    switch(peripheralDeviceType)
    {
        case kINQUIRY_PERIPHERAL_TYPE_DirectAccessSBCDevice:
            return CFSTR("Block device");
        case kINQUIRY_PERIPHERAL_TYPE_SequentialAccessSSCDevice:
            return CFSTR("Sequential device");
        case kINQUIRY_PERIPHERAL_TYPE_PrinterSSCDevice:
            return CFSTR("Printer");
        case kINQUIRY_PERIPHERAL_TYPE_ProcessorSPCDevice:
            return CFSTR("Processor");
        case kINQUIRY_PERIPHERAL_TYPE_WriteOnceSBCDevice:
            return CFSTR("Write-once device");
        case kINQUIRY_PERIPHERAL_TYPE_CDROM_MMCDevice:
            return CFSTR("CD/DVD-ROM");
        case kINQUIRY_PERIPHERAL_TYPE_ScannerSCSI2Device:
            return CFSTR("Scanner");
        case kINQUIRY_PERIPHERAL_TYPE_OpticalMemorySBCDevice:
            return CFSTR("Optical memory device");
        case kINQUIRY_PERIPHERAL_TYPE_MediumChangerSMCDevice:
            return CFSTR("Medium changer");
        case kINQUIRY_PERIPHERAL_TYPE_CommunicationsSSCDevice:
            return CFSTR("Communications device");
        /* 0x0A - 0x0B ASC IT8 Graphic Arts Prepress Devices */
        case kINQUIRY_PERIPHERAL_TYPE_StorageArrayControllerSCC2Device:
            return CFSTR("Storage array controller");
        case kINQUIRY_PERIPHERAL_TYPE_EnclosureServicesSESDevice:
            return CFSTR("Enclosure services device");
        case kINQUIRY_PERIPHERAL_TYPE_SimplifiedDirectAccessRBCDevice:
            return CFSTR("Simplified direct-access device");
        case kINQUIRY_PERIPHERAL_TYPE_OpticalCardReaderOCRWDevice:
            return CFSTR("Optical card reader/writer");
        /* 0x10 - 0x1E Reserved Device Types */
        case kINQUIRY_PERIPHERAL_TYPE_ObjectBasedStorageDevice:
            return CFSTR("Object-based storage device");
        case kINQUIRY_PERIPHERAL_TYPE_AutomationDriveInterface:
            return CFSTR("Automation drive interface");
        case kINQUIRY_PERIPHERAL_TYPE_WellKnownLogicalUnit:
            return CFSTR("Well known logical unit");
        case kINQUIRY_PERIPHERAL_TYPE_UnknownOrNoDeviceType:
        default:
            return CFSTR("Unknown or no device");
    };
}