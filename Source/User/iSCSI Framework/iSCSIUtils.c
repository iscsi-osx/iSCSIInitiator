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

#include "iSCSIUtils.h"
#include <ifaddrs.h>

/*! Minimum TCP port. */
static int PORT_MIN = 0;

/*! Maximum TCP port. */
static int PORT_MAX = (1 << sizeof(in_port_t)*8) -1;

/*! Verifies whether specified iSCSI qualified name (IQN) is valid per RFC3720.
 *  This function also validates 64-bit EUI names expressed as strings that
 *  start with the "eui" prefix.
 *  @param IQN the iSCSI qualified name.
 *  @return true if the name is valid, false otherwise. */
Boolean iSCSIUtilsValidateIQN(CFStringRef IQN)
{
    // IEEE regular expression for matching IQN name
    const char pattern[] =  "^iqn[.][0-9]{4}-[0-9]{2}[.][[:alnum:]]{1,}[.]"
                            "[-A-Za-z0-9.]{1,255}"
                            "|^eui[.][[:xdigit:]]{16}$";
    
    Boolean validName = false;
    regex_t preg;
    regcomp(&preg,pattern,REG_EXTENDED | REG_NOSUB);
    
    CFIndex IQNLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(IQN),kCFStringEncodingASCII) + sizeof('\0');
    char IQNBuffer[IQNLength];
    CFStringGetCString(IQN,IQNBuffer,IQNLength,kCFStringEncodingASCII);

    if(regexec(&preg,IQNBuffer,0,NULL,0) == 0)
        validName = true;
    
    regfree(&preg);
    return validName;
}

/*! Validates the TCP port.
 *  @param port the TCP port to validate.
 *  @return true if the specified port is valid, false otherwise. */
Boolean iSCSIUtilsValidatePort(CFStringRef port)
{
    SInt32 portNumber = CFStringGetIntValue(port);
    return (portNumber >= PORT_MIN && portNumber <= PORT_MAX);
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
    const char IPv4Pattern[] = "^((((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|([0-9])?[0-9])[.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|([0-9])?[0-9]))(:([0-9]{1,5}))?)$";
    
    const char IPv6Pattern[] = "^([[]?(([A-Fa-f0-9]{0,4}:){1,7}[A-Fa-f0-9]{0,4})([]]:([0-9]{1,5})?)?)$";
    
    const char DNSPattern[] = "^((([A-Za-z0-9]{1,63}[.]){1,3}[A-Za-z0-9]{1,63})(:([0-9]{1,5}))?)$";
    
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
       
        CFIndex portalLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(portal),kCFStringEncodingASCII) + sizeof('\0');
        char portalBuffer[portalLength];
        CFStringGetCString(portal,portalBuffer,portalLength,kCFStringEncodingASCII);

        // Match against pattern[index]
        if(regexec(&preg,portalBuffer,maxMatches[index],matches,0))
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

/*! Gets a string describing the iSCSI login status.
 *  @param statusCode the login status code.
 *  @return a string describing the login status (guaranteed to be a valid string). */
CFStringRef iSCSIUtilsGetStringForLoginStatus(enum iSCSILoginStatusCode statusCode)
{
    switch(statusCode)
    {
        case kiSCSILoginSuccess:
            return CFSTR("Login successful");
        case kiSCSILoginAccessDenied:
            return CFSTR("The target has denied access");
        case kiSCSILoginAuthFail:
            return CFSTR("Authentication failure");
        case kiSCSILoginCantIncludeInSeession:
            return CFSTR("Can't include the portal in the session");
        case kiSCSILoginInitiatorError:
            return CFSTR("An initiator error has occurred");
        case kiSCSILoginInvalidReqDuringLogin:
            return CFSTR("The initiator made an invalid request");
        case kiSCSILoginMissingParam:
            return CFSTR("Missing login parameters");
        case kiSCSILoginNotFound:
            return CFSTR("Target was not found");
        case kiSCSILoginOutOfResources:
            return CFSTR("Target is out of resources");
        case kiSCSILoginServiceUnavailable:
            return CFSTR("Target services unavailable");
        case kiSCSILoginSessionDoesntExist:
            return CFSTR("Session doesn't exist");
        case kiSCSILoginSessionTypeUnsupported:
            return CFSTR("Target doesn't support login");
        case kiSCSILoginTargetHWorSWError:
            return CFSTR("Target software or hardware error has occured");
        case kiSCSILoginTargetMovedPerm:
            return CFSTR("Target has permanently moved");
        case kiSCSILoginTargetMovedTemp:
            return CFSTR("Target has temporarily moved");
        case kiSCSILoginTargetRemoved:
            return CFSTR("Target has been removed");
        case kiSCSILoginTooManyConnections:
            return CFSTR("The session cannot support additional connections");
        case kiSCSILoginUnsupportedVer:
            return CFSTR("Target is incompatible with the initiator");
        case kiSCSILoginInvalidStatusCode:
        default:
            return CFSTR("Unknown error occurred");
    };
    return CFSTR("");
}

/*! Gets a string describing the iSCSI logout status.
 *  @param statusCode the logout status code.
 *  @return a string describing the login status (guaranteed to be a valid string). */
CFStringRef iSCSIUtilsGetStringForLogoutStatus(enum iSCSILogoutStatusCode statusCode)
{
    switch(statusCode)
    {
        case kiSCSILogoutSuccess:
            return CFSTR("Logout successful");
        case kiSCSILogoutCIDNotFound:
            return CFSTR("The connection was not found");
        case kiSCSILogoutCleanupFailed:
            return CFSTR("Target cleanup of connection failed");
        case kiSCSILogoutRecoveryNotSupported:
            return CFSTR("Could not recover the connection");
        case kiSCSILogoutInvalidStatusCode:
        default:
            return CFSTR("");
    };
    return CFSTR("");
}

/*! Creates address structures for an iSCSI target and the host (initiator)
 *  given an iSCSI portal reference. This function may be helpful when
 *  interfacing to low-level C networking APIs or other foundation libraries.
 *  @param portal an iSCSI portal.
 *  @param the target address structure (returned by this function).
 *  @param the host address structure (returned by this function). */
errno_t iSCSIUtilsGetAddressForPortal(iSCSIPortalRef portal,
                                     struct sockaddr_storage * remoteAddress,
                                     struct sockaddr_storage * localAddress)
{
    if (!portal || !remoteAddress || !localAddress)
        return EINVAL;
    
    errno_t error = 0;
    
    // Resolve the target node first and get a sockaddr info for it
    CFStringRef targetAddr = iSCSIPortalGetAddress(portal);
    CFIndex targetAddrLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(targetAddr),kCFStringEncodingASCII) + sizeof('\0');
    char targetAddrBuffer[targetAddrLength];
    CFStringGetCString(targetAddr,targetAddrBuffer,targetAddrLength,kCFStringEncodingASCII);
    
    CFStringRef targetPort = iSCSIPortalGetPort(portal);
    CFIndex targetPortLength = CFStringGetMaximumSizeForEncoding(CFStringGetLength(targetPort),kCFStringEncodingASCII) + sizeof('\0');
    char targetPortBuffer[targetPortLength];
    CFStringGetCString(targetPort,targetPortBuffer,targetPortLength,kCFStringEncodingASCII);
    
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    };
    
    struct addrinfo * aiTarget = NULL;
    if((error = getaddrinfo(targetAddrBuffer,targetPortBuffer,&hints,&aiTarget)))
        return error;
    
    // Copy the sock_addr structure into a sockaddr_storage structure (this
    // may be either an IPv4 or IPv6 sockaddr structure)
    memcpy(remoteAddress,aiTarget->ai_addr,aiTarget->ai_addrlen);
    
    freeaddrinfo(aiTarget);
    
    // If the default interface is to be used, prepare a structure for it
    CFStringRef hostIface = iSCSIPortalGetHostInterface(portal);
    
    if(CFStringCompare(hostIface,kiSCSIDefaultHostInterface,0) == kCFCompareEqualTo)
    {
        localAddress->ss_family = remoteAddress->ss_family;
        
        // For completeness, setup the sockaddr_in structure
        if(localAddress->ss_family == AF_INET)
        {
            struct sockaddr_in * sa = (struct sockaddr_in *)localAddress;
            sa->sin_port = 0;
            sa->sin_addr.s_addr = htonl(INADDR_ANY);
            sa->sin_len = sizeof(struct sockaddr_in);
        }
        
        // TODO: test IPv6 functionality
        else if(localAddress->ss_family == AF_INET6)
        {
            struct sockaddr_in6 * sa = (struct sockaddr_in6 *)localAddress;
            sa->sin6_addr = in6addr_any;
        }
        
        return error;
    }
    
    // Otherwise we have to search the list of all interfaces for the specified
    // interface and copy the corresponding address structure
    struct ifaddrs * interfaceList;
    
    if((error = getifaddrs(&interfaceList)))
        return error;
    
    error = EAFNOSUPPORT;
    struct ifaddrs * interface = interfaceList;
    
    while(interface)
    {
        // Check if interface supports the targets address family (e.g., IPv4)
        if(interface->ifa_addr->sa_family == remoteAddress->ss_family)
        {
            CFStringRef currIface = CFStringCreateWithCStringNoCopy(
                                                                    kCFAllocatorDefault,
                                                                    interface->ifa_name,
                                                                    kCFStringEncodingUTF8,
                                                                    kCFAllocatorNull);
            
            Boolean ifaceNameMatch =
            CFStringCompare(currIface,hostIface,kCFCompareCaseInsensitive) == kCFCompareEqualTo;
            CFRelease(currIface);
            // Check if interface names match...
            if(ifaceNameMatch)
            {
                memcpy(localAddress,interface->ifa_addr,interface->ifa_addr->sa_len);
                error = 0;
                break;
            }
        }
        interface = interface->ifa_next;
    }
    
    freeifaddrs(interfaceList);
    return error;
}
