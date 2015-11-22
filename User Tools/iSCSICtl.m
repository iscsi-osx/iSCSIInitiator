/*!
 * @author		Nareg Sinenian
 * @file		iSCSICtl.m
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI management utility.
 */

#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>

#include <readpassphrase.h>

#include "iSCSITypes.h"
#include "iSCSIRFC3720Keys.h"
#include "iSCSIPropertyList.h"
#include "iSCSIDaemonInterface.h"
#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"
#include "iSCSIUtils.h"

/*! Modes of operation for this utility. */
enum iSCSICtlCmds {

    /*! Add (target, discovery portal, etc). */
    kiSCSICtlCmdAdd,

    /*! Modify (initiator, target, discovery portal, etc). */
    kiSCSICtlCmdModify,

    /*! Remove (target, discovery portal, etc). */
    kiSCSICtlCmdRemove,

    /*! List (targets, discovery portal, luns, etc). */
    kiSCSICtlCmdList,

    /*! Login (target). */
    kiSCSICtlCmdLogin,

    /*! Logout (target). */
    kiSCSICtlCmdLogout,

    /*! Mount (target LUNs). */
    kiSCSICtlCmdMount,

    /*! Unmount (target LUNs). */
    kiSCSICtlCmdUnmount,

    /*! Probe (target). */
    kiSCSICtlCmdProbe,

    /*! Reset. */
    kiSCSICtlCmdReset,

    /*! Invalid mode of operation. */
    kiSCSICtlCmdInvalid
};


/*! Sub-modes of operation for this utility. */
enum iSCSICtlSubCmds {

    /*! Sub mode for initiator configuration operations. */
    kiSCSICtlSubCmdInitiatorConfig,

    /*! Sub mode for target  operations. */
    kiSCSICtlSubCmdTarget,

    /*! Sub mode for target configuration operations. */
    kiSCSICtlSubCmdTargetConfig,

    /*! Sub mode for targets (list of) operations. */
    kiSCSICtlSubCmdTargets,

    /*! Sub mode for discovery portal operations. */
    kiSCSICtlSubCmdDiscoveryPortal,

    /*! Sub mode for discovery operations. */
    kiSCSICtlSubCmdDiscoveryConfig,

    /*! Sub mode for LUN operations. */
    kiSCSICtlSubCmdLUNs,

    /*! Invalid sub-mode. */
    kiSCSICtlSubCmdInvalid
};


/*! Target command-line option. */
CFStringRef kOptKeyTarget = CFSTR("target");

/*! Portal command-line option. */
CFStringRef kOptKeyPortal = CFSTR("portal");

/*! Port command-line option. */
CFStringRef kOptKeyPort = CFSTR("port");

/*! Interface command-line option. */
CFStringRef kOptKeyInterface = CFSTR("interface");

/*! Auto login command-line option. */
CFStringRef kOptKeyAutoLogin = CFSTR("auto-login");

/*! Auto-login enable/disable command-line value. */
CFStringRef kOptValueAutoLoginEnable = CFSTR("enable");

/*! Auto login enable/disable command-line value. */
CFStringRef kOptValueAutoLoginDisable = CFSTR("disable");

/*! Digest command-line options. */
CFStringRef kOptKeyDigest = CFSTR("digest");

/*! Authentication method to use. */
CFStringRef kOptKeyAutMethod = CFSTR("authentication");

/*! CHAP authentication value for authentication key. */
CFStringRef kOptValueAuthMethodCHAP = CFSTR("CHAP");

/*! No authentication value for authentication key. */
CFStringRef kOptValueAuthMethodNone = CFSTR("None");

/*! User (CHAP) command-line option. */
CFStringRef kOptKeyCHAPName = CFSTR("CHAP-name");

/*! Secret (CHAP) command-line option. */
CFStringRef kOptKeyCHAPSecret = CFSTR("CHAP-secret");

/*! Node name command-line option. */
CFStringRef kOptKeyNodeName = CFSTR("node-name");

/*! Node alias command-line option. */
CFStringRef kOptKeyNodeAlias = CFSTR("node-alias");

/*! Max connections command line option. */
CFStringRef kOptKeyMaxConnections = CFSTR("MaxConnections");

/*! Error recovery level command line option. */
CFStringRef kOptKeyErrorRecoveryLevel = CFSTR("ErrorRecoveryLevel");

/*! Header digest command line option. */
CFStringRef kOptKeyHeaderDigest = CFSTR("HeaderDigest");

/*! Data digest command line option. */
CFStringRef kOptKeyDataDigest = CFSTR("DataDigest");

/*! Digest value for no digest. */
CFStringRef kOptValueDigestNone = CFSTR("None");

/*! Digest value for CRC32C digest. */
CFStringRef kOptValueDigestCRC32C = CFSTR("CRC32C");

/*! Discovery (SendTargets) enable/disable command-line option. */
CFStringRef kOptKeySendTargetsEnable = CFSTR("SendTargets");

/*! Discovery enable/disable command-line value. */
CFStringRef kOptValueDiscoveryEnable = CFSTR("enable");

/*! Discovery enable/disable command-line value. */
CFStringRef kOptValueDiscoveryDisable = CFSTR("disable");

/*! Discovery interval command-line option. */
CFStringRef kOptKeyDiscoveryInterval = CFSTR("interval");

/*! Empty value. */
CFStringRef kOptValueEmpty = CFSTR("");

/*! Maximum number of attempts to enter a CHAP shared secret. */
const int MAX_SECRET_RETRY_ATTEMPTS = 3;

/*! Name of this command-line executable. */
const char * executableName;

/*! The standard out stream, used by various functions to write. */
CFWriteStreamRef stdoutStream = NULL;

/*! Used to display the device tree (luns and target information) for a 
 *  particular iSCSI target object in the system's IO registry. */
errno_t displayTargetDeviceTree(io_object_t);


enum iSCSICtlCmds iSCSICtlGetCmdFromArguments(CFArrayRef arguments)
{
    CFMutableDictionaryRef modesDict = CFDictionaryCreateMutable(kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,0);
    CFDictionarySetValue(modesDict,CFSTR("add"),(const void *)kiSCSICtlCmdAdd);
    CFDictionarySetValue(modesDict,CFSTR("modify"),(const void *)kiSCSICtlCmdModify);
    CFDictionarySetValue(modesDict,CFSTR("remove"),(const void *)kiSCSICtlCmdRemove);
    CFDictionarySetValue(modesDict,CFSTR("list") ,(const void *)kiSCSICtlCmdList);
    CFDictionarySetValue(modesDict,CFSTR("login"),(const void *)kiSCSICtlCmdLogin);
    CFDictionarySetValue(modesDict,CFSTR("logout"),(const void *)kiSCSICtlCmdLogout);
    CFDictionarySetValue(modesDict,CFSTR("mount"),(const void *)kiSCSICtlCmdMount);
    CFDictionarySetValue(modesDict,CFSTR("unmount"),(const void *)kiSCSICtlCmdUnmount);
    CFDictionarySetValue(modesDict,CFSTR("probe"),(const void *)kiSCSICtlCmdProbe);
    CFDictionarySetValue(modesDict,CFSTR("reset"),(const void *)kiSCSICtlCmdReset);

    // If a mode was supplied (first argument after executable name)
    if(CFArrayGetCount(arguments) > 1) {
        CFStringRef arg = CFArrayGetValueAtIndex(arguments,1);
        return (enum iSCSICtlCmds) CFDictionaryGetValue(modesDict,arg);
    }
 
    return kiSCSICtlCmdInvalid;
}

enum iSCSICtlSubCmds iSCSICtlGetSubCmdFromArguments(CFArrayRef arguments)
{
    CFMutableDictionaryRef subModesDict = CFDictionaryCreateMutable(kCFAllocatorDefault,0,&kCFTypeDictionaryKeyCallBacks,0);
    CFDictionaryAddValue(subModesDict,CFSTR("initiator-config"),(const void *)kiSCSICtlSubCmdInitiatorConfig);
    CFDictionaryAddValue(subModesDict,CFSTR("target"),(const void *)kiSCSICtlSubCmdTarget);
    CFDictionaryAddValue(subModesDict,CFSTR("target-config"),(const void *)kiSCSICtlSubCmdTargetConfig);
    CFDictionaryAddValue(subModesDict,CFSTR("targets"),(const void *)kiSCSICtlSubCmdTargets);
    CFDictionaryAddValue(subModesDict,CFSTR("discovery-portal"),(const void *)kiSCSICtlSubCmdDiscoveryPortal);
    CFDictionaryAddValue(subModesDict,CFSTR("discovery-config"),(const void *)kiSCSICtlSubCmdDiscoveryConfig);
    CFDictionaryAddValue(subModesDict,CFSTR("luns"),(const void *)kiSCSICtlSubCmdLUNs);

    // If a mode was supplied (first argument after executable name)
    enum iSCSICtlSubCmds subCmd = kiSCSICtlSubCmdInvalid;

    if(CFArrayGetCount(arguments) > 2) {
        CFDictionaryGetValueIfPresent(subModesDict,
                                      CFArrayGetValueAtIndex(arguments,2),
                                      (const void **)&subCmd);
    }

    return subCmd;
}

void iSCSICtlParseTargetAndPortalToDictionary(CFArrayRef arguments,
                                              CFMutableDictionaryRef optDictionary)
{
    // The target, portal, or target-portal combination is specified after
    // the sub-command, if one was specified.
    CFIndex targetAndPortalIdx = 3;
    if(iSCSICtlGetSubCmdFromArguments(arguments) == kiSCSICtlSubCmdInvalid)
        targetAndPortalIdx--;

    // Ensure enough arguments were provided
    if(CFArrayGetCount(arguments) > targetAndPortalIdx)
    {
        // Target and portal
        CFStringRef arg = CFArrayGetValueAtIndex(arguments,targetAndPortalIdx);

        // Find the comma (,) separating the two, if it exists
        CFArrayRef argParts = CFStringCreateArrayBySeparatingStrings(
            kCFAllocatorDefault,arg,CFSTR(","));

        // If there are two parts, separate target & portal
        if(CFArrayGetCount(argParts) > 1) {
            CFDictionaryAddValue(optDictionary,kOptKeyTarget,CFArrayGetValueAtIndex(argParts,0));
            CFDictionaryAddValue(optDictionary,kOptKeyPortal,CFArrayGetValueAtIndex(argParts,1));
        }
        // Need to determine whether a portal or target was specified
        else {
            // Targets must start with IQN or EUI
            CFRange matchIQN = CFStringFind(arg,CFSTR("iqn."),kCFCompareCaseInsensitive);
            CFRange matchEUI = CFStringFind(arg,CFSTR("eui."),kCFCompareCaseInsensitive);

            if(matchIQN.location == kCFNotFound && matchEUI.location == kCFNotFound)
                CFDictionaryAddValue(optDictionary,kOptKeyPortal,arg);
            else
                CFDictionaryAddValue(optDictionary,kOptKeyTarget,arg);
        }
    }
}

void iSCSICtlParseSwitchesToDictionary(CFArrayRef arguments,
                                       CFMutableDictionaryRef optDictionary)
{
    // Search for swtiches without arguments & add them to options dictionary
    CFIndex argCount = CFArrayGetCount(arguments);
    CFStringRef thisArg = NULL, nextArg = NULL;
    for(CFIndex idx = 0; idx < argCount; idx++)
    {
        thisArg = (CFStringRef)CFArrayGetValueAtIndex(arguments,idx);

        if(CFStringGetCharacterAtIndex(thisArg,0) == '-')
        {
            if((idx + 1) < argCount)
                nextArg = (CFStringRef)CFArrayGetValueAtIndex(arguments,idx+1);

            if(!nextArg || CFStringGetCharacterAtIndex(nextArg,0) == '-')
            {
                CFMutableStringRef arg = CFStringCreateMutableCopy(kCFAllocatorDefault,0,thisArg);
                CFStringTrim(arg,CFSTR("-"));
                CFDictionarySetValue(optDictionary,arg,kOptValueEmpty);
                CFRelease(arg);
            }

            if(nextArg)
                CFRelease(nextArg);
        }
        CFRelease(thisArg);
    }
}

/*! Writes a string to stdout. */
void iSCSICtlDisplayString(CFStringRef string)
{
    if(!string || !stdoutStream)
        return;
    
    CFIndex maxBufLen = 255;
    CFIndex usedBufLen = 0;
    UInt8 buffer[maxBufLen];
    
    CFStringGetBytes(string,
                     CFRangeMake(0,CFStringGetLength(string)),
                     kCFStringEncodingASCII,
                     0,
                     false,
                     buffer,
                     maxBufLen,
                     &usedBufLen);
    
    CFWriteStreamWrite(stdoutStream,buffer,usedBufLen);
}

/*! Displays a generic error message.
 *  @param errorString the error message to display. */
void iSCSICtlDisplayError(CFStringRef errorString)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %@\n"),executableName,errorString);
    
    iSCSICtlDisplayString(error);
    CFRelease(error);
}

/*! Displays a C error code. 
 *  @param errorCode the C error code. */
void iSCSICtlDisplayErrorCode(errno_t errorCode)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %s\n"),executableName,strerror(errorCode));

    iSCSICtlDisplayString(error);
    CFRelease(error);
}

void iSCSICtlDisplayPermissionsError()
{
    iSCSICtlDisplayError(CFSTR("Permission denied"));
}

void iSCSICtlDisplayUsage()
{
    iSCSICtlDisplayString(CFSTR("Usage: iscsictl add     target <target>,<portal> [-interface <interface>]\n"
                                "       iscsictl remove  target <target>[,<portal>]\n\n"));
    
    
    iSCSICtlDisplayString(CFSTR("       iscsictl login   <target>[,<portal>]\n"
                                "       iscsictl logout  <target>[,<portal>]\n"
                                "       iscsictl mount   <target>\n"
                                "       iscsictl unmount <target>\n\n"));

    iSCSICtlDisplayString(CFSTR("       iscsictl modify initiator-config [...]\n"
                                "       iscsictl modify target-config <target>[,<portal>] [...]\n"
                                "       iscsictl modify discovery-config [...]\n\n"));
                                        
    iSCSICtlDisplayString(CFSTR("       iscsictl list initiator-config\n"
                                "       iscsictl list target-config <target>\n"
                                "       iscsictl list discovery-config\n\n"));
                                        
    iSCSICtlDisplayString(CFSTR("       iscsictl add discovery-portal <portal> [-interface <interface>]\n"
                                "       iscsictl remove discovery-portal <portal>\n\n"));
                                        
    iSCSICtlDisplayString(CFSTR("       iscsictl list targets\n"
                                "       iscsictl list luns\n"));
}

CFStringRef iSCSICtlCreateSecretFromInput(CFIndex retries)
{
    CFStringRef secret = NULL;

    const int MAX_SECRET_LENGTH = 256;
    char buffer[MAX_SECRET_LENGTH];
    char verify[MAX_SECRET_LENGTH];

    // Retry as required...
    for(CFIndex idx = 0; idx < retries; idx++)
    {
        // Print a message if this is not the first attempt
        if(idx != 0)
            iSCSICtlDisplayString(CFSTR("Shared secrets do not match. Try again.\n"));

        if(readpassphrase("Enter shared secret: ",(char*)buffer,sizeof(buffer),RPP_ECHO_OFF))
        {
            if(readpassphrase("Verify shared secret: ",(char*)verify,sizeof(buffer),RPP_ECHO_OFF))
            {
                if(strncmp((const char*)buffer,(const char*)verify,MAX_SECRET_LENGTH) == 0)
                {
                    memset(verify,0,sizeof(verify));
                    secret = CFStringCreateWithCString(kCFAllocatorDefault,
                                                       (const char*)buffer,
                                                       kCFStringEncodingASCII);
                    break;
                }
                else {
                    memset(buffer,0,sizeof(buffer));
                    memset(verify,0,sizeof(verify));
                }
            }
            // Clear password buffer
            memset(buffer,0,sizeof(buffer));
        }
    }

    if(!secret) {
        iSCSICtlDisplayString(CFSTR("Exceeded the maximum number of attempts\n"));
    }
    return secret;
}

CFStringRef iSCSICtlGetStringForDigestType(enum iSCSIDigestTypes digestType)
{
    switch(digestType)
    {
        case kiSCSIDigestCRC32C:
            return CFSTR("crc32c"); break;
        default:
            return CFSTR("none"); break;
    };
}

CFStringRef iSCSICtlGetStringForLoginStatus(enum iSCSILoginStatusCode statusCode)
{
    switch(statusCode)
    {
        case kiSCSILoginSuccess:
            break;
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


void iSCSICtlDisplayiSCSILoginError(enum iSCSILoginStatusCode statusCode)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %@\n"),executableName,iSCSICtlGetStringForLoginStatus(statusCode));
    
    iSCSICtlDisplayString(error);
    CFRelease(error);
}

/*! Helper function used to display login status.
 *  @param statusCode the status code indicating the login result.
 *  @param target the target.
 *  @param portal the portal used to logon to the target.  Specify NULL if all
 *  portals were used. */
void iSCSICtlDisplayLoginStatus(enum iSCSILoginStatusCode statusCode,
                                iSCSITargetRef target,
                                iSCSIPortalRef portal)
{
    if(statusCode != kiSCSILoginSuccess) {
        iSCSICtlDisplayiSCSILoginError(statusCode);
        return;
    }
    
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef logoutStatus;
    if (portal) {
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFStringRef portalPort    = iSCSIPortalGetPort(portal);
        CFStringRef hostInterface = iSCSIPortalGetHostInterface(portal);
        
        logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                CFSTR("Added <%@:%@ interface %@> to the session\n"),
                                                portalAddress,portalPort,hostInterface);
        iSCSICtlDisplayString(logoutStatus);
        CFRelease(logoutStatus);
    }
    else {
        iSCSICtlDisplayString(CFSTR("Attached "));
        
        io_object_t targetObj = iSCSIIORegistryGetTargetEntry(targetIQN);
        displayTargetDeviceTree(targetObj);
    }
}


/*! Helper function used to display login status.
 *  @param statusCode the status code indicating the login result.
 *  @param target the target
 *  @param portal the portal used to logon to the target. */
void iSCSICtlDisplayProbeTargetLoginStatus(enum iSCSILoginStatusCode statusCode,
                                           iSCSITargetRef target,
                                           iSCSIPortalRef portal,
                                           enum iSCSIAuthMethods authMethod)
{
    CFStringRef probeStatus = NULL;

    switch(authMethod)
    {
        case kiSCSIAuthMethodCHAP: probeStatus = CFSTR("Target requires CHAP authentication\n"); break;
        case kiSCSIAuthMethodNone: probeStatus = CFSTR("Target does not require authentication\n"); break;
        case kiSCSIAuthMethodInvalid:
        default:
            probeStatus = CFSTR("Target requires an unsupported authentication method\n"); break;
    };
    
    iSCSICtlDisplayString(probeStatus);
    CFRelease(probeStatus);
}


CFStringRef iSCSICtlGetStringForLogoutStatus(enum iSCSILogoutStatusCode statusCode)
{
    switch(statusCode)
    {
        case kiSCSILogoutSuccess:
            break;
            
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

void iSCSICtlDisplayiSCSILogoutError(enum iSCSILogoutStatusCode statusCode)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %@\n"),executableName,iSCSICtlGetStringForLogoutStatus(statusCode));
    
    iSCSICtlDisplayString(error);
    CFRelease(error);
}

void iSCSICtlDisplayLogoutStatus(enum iSCSILogoutStatusCode statusCode,
                                 iSCSITargetRef target,
                                 iSCSIPortalRef portal)
{
    if(statusCode != kiSCSILogoutSuccess) {
        iSCSICtlDisplayiSCSILogoutError(statusCode);
        return;
    }
    

    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef logoutStatus;
    if (portal) {
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFStringRef portalPort    = iSCSIPortalGetPort(portal);
        CFStringRef hostInterface = iSCSIPortalGetHostInterface(portal);

        logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                CFSTR("Removed <%@:%@ interface %@> from the session\n"),
                                                portalAddress,portalPort,hostInterface);
    }
    else {
        logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                CFSTR("Detached %@\n"),targetIQN);
    }

    iSCSICtlDisplayString(logoutStatus);
    CFRelease(logoutStatus);
}

/*! Helper function.  Returns true if the user attempted to specify a target
 *  as a command line argument (even if it was unsuccessful or malformed).
 *  @param options command-line options.
 *  @return true if a target was specified. */
Boolean iSCSICtlIsTargetSpecified(CFDictionaryRef options)
{
    return CFDictionaryContainsKey(options,kOptKeyTarget);
}

/*! Creates a target object from command-line options.  If the target is not
 *  well-formatted or missing then the function displays an error message and
 *  returns NULL.
 *  @param options command-line options.
 *  @return the target object, or NULL. */
iSCSIMutableTargetRef iSCSICtlCreateTargetFromOptions(CFDictionaryRef options)
{
    CFStringRef targetIQN;
    if(!CFDictionaryGetValueIfPresent(options,kOptKeyTarget,(const void **)&targetIQN))
    {
        iSCSICtlDisplayError(CFSTR("A target must be specified using a valid IQN or EUI-64 identifier"));
        return NULL;
    }
    
    if(!iSCSIUtilsValidateIQN(targetIQN)) {
        iSCSICtlDisplayError(CFSTR("The specified target name is not a valid IQN or EUI-64 identifier"));
        return NULL;
    }

    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetIQN(target,targetIQN);

    return target;
}

/*! Helper function.  Returns true if the user attempted to specify a portal
 *  as a command line argument (even if it was unsuccessful or malformed).
 *  @param options command-line options.
 *  @return true if a portal was specified. */
Boolean iSCSICtlIsPortalSpecified(CFDictionaryRef options)
{
    return CFDictionaryContainsKey(options,kOptKeyPortal);
}

/*! Creates a portal object from command-line options.  If the portal is not
 *  well-formatted or missing then the function displays an error message and
 *  returns NULL.
 *  @param options command-line options.
 *  @return the portal object, or NULL. */
iSCSIMutablePortalRef iSCSICtlCreatePortalFromOptions(CFDictionaryRef options)
{
    CFStringRef portalAddress, hostInterface;
    
    if(!CFDictionaryGetValueIfPresent(options,kOptKeyPortal,(const void **)&portalAddress))
    {
        iSCSICtlDisplayError(CFSTR("A portal must be specified"));
        return NULL;
    }
    
    // Returns an array of hostname, port strings (port optional)
    CFArrayRef portalParts = iSCSIUtilsCreateArrayByParsingPortalParts(portalAddress);
    
    if(!portalParts) {
        iSCSICtlDisplayError(CFSTR("The specified portal is invalid"));
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    
    // If port was specified as part of portal (e.g., 192.168.1.100:3260)
    if(CFArrayGetCount(portalParts) > 1) {
        iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    }
    // If port was specified separately (e.g., -port 3260)
    else if (CFDictionaryContainsKey(options,kOptKeyPort)) {
        CFStringRef port = CFDictionaryGetValue(options,kOptKeyPort);
        if(iSCSIUtilsValidatePort(port))
            iSCSIPortalSetPort(portal,port);
        else {
            iSCSICtlDisplayError(CFSTR("The specified port is invalid"));
            return NULL;
        }
    }
    else
        iSCSIPortalSetPort(portal,kiSCSIDefaultPort);
    
    CFRelease(portalParts);

    // If an interface was not specified, use the default interface
    if(!CFDictionaryGetValueIfPresent(options,kOptKeyInterface,(const void **)&hostInterface))
        iSCSIPortalSetHostInterface(portal,kiSCSIDefaultHostInterface);
    else
        iSCSIPortalSetHostInterface(portal,hostInterface);

    return portal;
}

errno_t iSCSICtlModifyPortalFromOptions(CFDictionaryRef options,
                                        iSCSIMutablePortalRef portal)
{
    iSCSIPortalRef portalUpdates = iSCSICtlCreatePortalFromOptions(options);

    // If a port was explicity specified, update it
    if(CFDictionaryContainsKey(options,kOptKeyPort))
        iSCSIPortalSetPort(portal,iSCSIPortalGetPort(portalUpdates));

    // If the interface was explicitly specified, update it
    if(CFDictionaryContainsKey(options,kOptKeyInterface))
        iSCSIPortalSetHostInterface(portal,iSCSIPortalGetHostInterface(portalUpdates));

    return 0;
}

errno_t iSCSICtlLogin(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;

    errno_t error = 0;
    iSCSITargetRef target = NULL;
    CFStringRef targetIQN = NULL;
    
    // Create target object from user input (may be null if user didn't specify)
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        error = EINVAL;

    if(!error) {

        // Synchronize the database with the property list on disk
        iSCSIPLSynchronize();

        // Verify that the target exists in the property list
        targetIQN = iSCSITargetGetIQN(target);

        if(!iSCSIPLContainsTarget(targetIQN)) {
            iSCSICtlDisplayString(CFSTR("The specified target does not exist\n"));
            error = EINVAL;
        }
    }

    // Check if the number of connections is maxed out for this session
    UInt32 maxConnections = 0, activeConnections = 0;
    if(!error) {
        CFDictionaryRef properties = NULL;
        if((properties = iSCSIDaemonCreateCFPropertiesForSession(handle,target)))
        {
            CFNumberRef number = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
            CFNumberGetValue(number,kCFNumberSInt32Type,&maxConnections);

            CFArrayRef portals = iSCSIDaemonCreateArrayOfActivePortalsForTarget(handle,target);
            activeConnections = (UInt32)CFArrayGetCount(portals);

            if(activeConnections == maxConnections) {
                iSCSICtlDisplayString(CFSTR("The specified target cannot support additional connections\n"));
                error = EINVAL;
            }
        }
    }

    enum iSCSILoginStatusCode statusCode;

    // Determine whether we're logging into a single portal or all portals...
    if(!error && iSCSICtlIsPortalSpecified(options))
    {
        iSCSIPortalRef portal = NULL;

        // Ensure portal was not malformed
        if((portal = iSCSICtlCreatePortalFromOptions(options)))
        {
            if(iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal)))
            {
                // The user may have specified only the portal address, so
                // get the portal from the property list to get port & interface
                CFStringRef portalAddress = CFStringCreateCopy(
                    kCFAllocatorDefault,iSCSIPortalGetAddress(portal));

                iSCSIPortalRelease(portal);
                portal = iSCSIPLCopyPortalForTarget(targetIQN,portalAddress);
                CFRelease(portalAddress);

                if(iSCSIDaemonIsPortalActive(handle,target,portal))
                    iSCSICtlDisplayString(CFSTR("The specified target has an active "
                                         "session over the specified portal\n"));
                else {
                    error = iSCSIDaemonLogin(handle,target,portal,&statusCode);

                    if(!error) {
                        if(activeConnections == 0)
                            iSCSICtlDisplayLoginStatus(statusCode,target,NULL);
                        else
                            iSCSICtlDisplayLoginStatus(statusCode,target,portal);
                    }
                }
            }
            else
                iSCSICtlDisplayString(CFSTR("The specified portal does not exist\n"));

            iSCSIPortalRelease(portal);
        }
    }
    else if(!error) {
        error = iSCSIDaemonLogin(handle,target,NULL,&statusCode);

        if(!error)
            iSCSICtlDisplayLoginStatus(statusCode,target,NULL);
        else
            iSCSICtlDisplayErrorCode(error);
    }

    iSCSITargetRelease(target);
    return error;
}

errno_t iSCSICtlLogout(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;

    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    errno_t error = 0;
    
    // See if there exists an active session for this target
    if(!iSCSIDaemonIsTargetActive(handle,target))
    {
        iSCSICtlDisplayString(CFSTR("The specified target has no active session\n"));
        error = EINVAL;
    }
    
    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
            error = EINVAL;
    }
    
    // If the portal was specified and a connection doesn't exist for it...
    if(!error && portal && !iSCSIDaemonIsPortalActive(handle,target,portal))
    {
        iSCSICtlDisplayString(CFSTR("The specified portal has no active connections\n"));
        error = EINVAL;
    }
    
    // At this point either the we logout the session or just the connection
    // associated with the specified portal, if one was specified
    if(!error)
    {
        enum iSCSILogoutStatusCode statusCode;
        error = iSCSIDaemonLogout(handle, target, portal, &statusCode);

        if(!error)
            iSCSICtlDisplayLogoutStatus(statusCode,target,NULL);
        else
            iSCSICtlDisplayErrorCode(error);
    }

    if(portal)
        iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);
    
    return error;
}

/*! Adds a new target and portal to the database but does not login to the
 *  target.  If the specified target exists and the portal does not, the portal
 *  is added to the existing target.  If both the target and portal exist, this
 *  function has no effect.  At least one portal must be specified along with
 *  the target name.
 *  @param handle a handle to the iSCSI daemon.
 *  @param options the command-line options.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlAddTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    errno_t error = EINVAL; // default error result
    if (handle && options) {
        iSCSITargetRef target = iSCSICtlCreateTargetFromOptions(options);
        iSCSIPortalRef portal = NULL;

        if(target)
            portal = iSCSICtlCreatePortalFromOptions(options);
        
        if (target && portal) {
            // Synchronize the database with the property list on disk
            iSCSIPLSynchronize();
            
            // If portal and target both exist then do nothing, otherwise
            // add target and or portal with user-specified options
            CFStringRef targetIQN = iSCSITargetGetIQN(target);

            if(!iSCSIPLContainsTarget(targetIQN)) {
                iSCSIPLAddStaticTarget(targetIQN,portal);
                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else
                    iSCSICtlDisplayString(CFSTR("The specified target has been added\n"));
            }
            else if(!iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
                iSCSIPLSetPortalForTarget(targetIQN,portal);
                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else
                    iSCSICtlDisplayString(CFSTR("The specified portal has been added\n"));
            }
            else {
                iSCSICtlDisplayString(CFSTR("The specified target and portal already exist\n"));
            }
            iSCSIPortalRelease(portal);
        }
        if(target)
            iSCSITargetRelease(target);
    }
    return error;
}

/*! Removes a target or portal from the database.  If only the target
 *  name is specified the target and all of its portals are removed. If a 
 *  specific portal is specific then that portal is removed.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary. */
errno_t iSCSICtlRemoveTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    errno_t error = 0;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
       return EINVAL;
       
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
        iSCSICtlDisplayString(CFSTR("The specified target does not exist\n"));
        error = EINVAL;
    }

    // Verify that the target is not a dynamic target managed by iSCSI
    // discovery
    if(iSCSIPLGetTargetConfigType(targetIQN) == kiSCSITargetConfigDynamicSendTargets) {
        iSCSICtlDisplayString(CFSTR("The target is configured using iSCSI discovery. "
                                    "Remove the discovery portal to remove the target\n"));
        error = EINVAL;
    }

    // If a portal was specified, verify that it is valid.
    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            error = EINVAL;
        }
    }

    // Verify that portal exists in property list
    if(!error && portal && !iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayString(CFSTR("The specified portal does not exist\n"));
        error = EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    if(!error) {
        if(portal)
            if(iSCSIDaemonIsPortalActive(handle,target,portal))
                iSCSICtlDisplayString(CFSTR("The specified portal is connected and cannot be removed\n"));
            else {
                iSCSIPLRemovePortalForTarget(targetIQN,iSCSIPortalGetAddress(portal));
                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else
                    iSCSICtlDisplayString(CFSTR("The specified portal has been removed\n"));
            }
        else
            if(iSCSIDaemonIsTargetActive(handle,target))
                iSCSICtlDisplayString(CFSTR("The specified target has an active session and cannot be removed\n"));
            else {
                iSCSIPLRemoveTarget(targetIQN);
                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else
                    iSCSICtlDisplayString(CFSTR("The specified target has been removed\n"));
            }
    }

    if(portal)
        iSCSIPortalRelease(portal);
    if(target)
        iSCSITargetRelease(target);
    
    return 0;
}

errno_t iSCSICtlModifyInitiator(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    CFStringRef value = NULL;

    iSCSIPLSynchronize();
    errno_t error = 0;

    // Check for CHAP user name
    if(CFDictionaryGetValueIfPresent(options,kOptKeyCHAPName,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueEmpty,0) != kCFCompareEqualTo)
            iSCSIPLSetInitiatorCHAPName(value);
    }

    // Check for CHAP shared secret
    if(CFDictionaryContainsKey(options,kOptKeyCHAPSecret)) {
        CFStringRef secret = iSCSICtlCreateSecretFromInput(MAX_SECRET_RETRY_ATTEMPTS);

        if(secret != NULL) {
            if(iSCSIPLSetInitiatorCHAPSecret(secret) != 0) {
                iSCSICtlDisplayPermissionsError();
                error = EAUTH;
            }
            CFRelease(secret);
        }
        else
            error = EINVAL;
    }

    // Check for authentication method
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyAutMethod,(const void**)&value))
    {
        if(CFStringCompare(value,kOptValueAuthMethodNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetInitiatorAuthenticationMethod(kiSCSIAuthMethodNone);
        else if(CFStringCompare(value,kOptValueAuthMethodCHAP,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetInitiatorAuthenticationMethod(kiSCSIAuthMethodCHAP);
        else {
            iSCSICtlDisplayError(CFSTR("The specified authentication method is invalid"));
            error = EINVAL;
        }
    }

    // Check for initiator alias
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyNodeAlias,(const void**)&value))
        iSCSIPLSetInitiatorAlias(value);

    // Check for initiator IQN
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyNodeName,(const void **)&value))
    {
        // Validate the chosen initiator IQN
        if(iSCSIUtilsValidateIQN(value))
            iSCSIPLSetInitiatorIQN(value);
        else {
            iSCSICtlDisplayError(CFSTR("The specified name is not a valid IQN or EUI-64 identifier"));
            error = EINVAL;
        }
    }

    if(!error) {
        if(!iSCSIPLSynchronize())
            iSCSICtlDisplayPermissionsError();
        else
            iSCSICtlDisplayString(CFSTR("Initiator settings have been updated\n"));
    }

    return 0;
}

errno_t iSCSICtlModifyTargetFromOptions(CFDictionaryRef options,
                                        iSCSITargetRef target,
                                        iSCSIPortalRef portal)
{
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef value = NULL;

    iSCSIPLSynchronize();
    errno_t error = 0;

    // Check for CHAP user name, ensure it is not blank
    if(CFDictionaryGetValueIfPresent(options,kOptKeyCHAPName,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueEmpty,0) != kCFCompareEqualTo)
            iSCSIPLSetTargetCHAPName(targetIQN,value);
    }

    // Check for CHAP shared secret
    if(CFDictionaryContainsKey(options,kOptKeyCHAPSecret)) {
        CFStringRef secret = iSCSICtlCreateSecretFromInput(MAX_SECRET_RETRY_ATTEMPTS);

        if(secret != NULL) {
            iSCSIPLSetTargetCHAPSecret(targetIQN,secret);
            CFRelease(secret);
        }
        else {
            error = EINVAL;
        }
    }

    // Check for authentication method
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;

    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyAutMethod,(const void**)&value))
    {
        if(CFStringCompare(value,kOptValueAuthMethodNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
            authMethod = kiSCSIAuthMethodNone;
            iSCSIPLSetTargetAuthenticationMethod(targetIQN,authMethod);
        }
        else if(CFStringCompare(value,kOptValueAuthMethodCHAP,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        {
            authMethod = kiSCSIAuthMethodCHAP;
            iSCSIPLSetTargetAuthenticationMethod(targetIQN,authMethod);
        }
        else {
            iSCSICtlDisplayError(CFSTR("The specified authentication method is invalid"));
            error = EINVAL;
        }
    }

    // Check for target IQN
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyNodeName,(const void **)&value))
    {
        // Validate the chosen target IQN
        if(iSCSIUtilsValidateIQN(value))
            iSCSIPLSetTargetIQN(targetIQN,value);
        else {
            iSCSICtlDisplayError(CFSTR("The specified name is not a valid IQN or EUI-64 identifier"));
            error = EINVAL;
        }
    }
    
    // Check for auto-login
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyAutoLogin,(const void **)&value))
    {

        if(CFStringCompare(value,kOptValueAutoLoginEnable,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetAutoLoginForTarget(targetIQN,true);
        else if(CFStringCompare(value,kOptValueAutoLoginDisable,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetAutoLoginForTarget(targetIQN,false);
        else {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,CFSTR("Invalid argument for %@"),kOptKeyAutoLogin);
            iSCSICtlDisplayError(errorString);
            CFRelease(errorString);
            error = EINVAL;
        }
    }

    // Check for maximum connections
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyMaxConnections,(const void **)&value))
    {
        UInt32 maxConnections = CFStringGetIntValue(value);
        if(maxConnections < kRFC3720_MaxConnections_Min) {
            iSCSICtlDisplayError(CFSTR("Specified maximum number of connections is exceeds the maximum"));
            error = EINVAL;
        }
        else if(maxConnections > kRFC3720_MaxConnections_Max) {
            iSCSICtlDisplayError(CFSTR("Specified maximum number of connections is not sufficient"));
            error = EINVAL;
        }
        else
            iSCSIPLSetMaxConnectionsForTarget(targetIQN,maxConnections);
    }

    // Check for error recovery level
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyErrorRecoveryLevel,(const void **)&value))
    {
        enum iSCSIErrorRecoveryLevels level = CFStringGetIntValue(value);

        if(level < kRFC3720_ErrorRecoveryLevel_Min || level > kRFC3720_ErrorRecoveryLevel_Max) {
            iSCSICtlDisplayError(CFSTR("The specified error recovery level is invalid"));
            error = EINVAL;
        }
        else {
            iSCSIPLSetErrorRecoveryLevelForTarget(targetIQN,level);
        }
    }

    // Check for header digest
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyHeaderDigest,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueDigestNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetHeaderDigestForTarget(targetIQN,kiSCSIDigestNone);
        else if(CFStringCompare(value,kOptValueDigestCRC32C,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetHeaderDigestForTarget(targetIQN,kiSCSIDigestCRC32C);
        else {
            iSCSICtlDisplayError(CFSTR("The specified digest type is invalid"));
            error = EINVAL;
        }
    }

    // Check for data digest
    if(!error && CFDictionaryGetValueIfPresent(options,kOptKeyDataDigest,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueDigestNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetDataDigestForTarget(targetIQN,kiSCSIDigestNone);
        else if(CFStringCompare(value,kOptValueDigestCRC32C,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetDataDigestForTarget(targetIQN,kiSCSIDigestCRC32C);
        else {
            iSCSICtlDisplayError(CFSTR("The specified digest type is invalid"));
            error = EINVAL;
        }
    }

    return 0;
}

errno_t iSCSICtlModifyTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // First check if the target exists in the property list.  Then check to
    // see if it has an active session.  If it does, target properties cannot
    // be modified.  Do the same for portal (to change connection properties,
    // but this is optional and a portal may not have been specified by the
    // user).

    if(handle < 0 || !options)
        return EINVAL;

    iSCSIMutableTargetRef target = NULL;
    iSCSIMutablePortalRef portal = NULL;
    errno_t error = 0;

    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    CFStringRef targetIQN = iSCSITargetGetIQN(target);

    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();

    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
        iSCSICtlDisplayString(CFSTR("The specified target does not exist\n"));
        error = EINVAL;
    }

    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            error = EINVAL;
        }
    }

    // Verify that portal exists in property list
    if(!error && portal && !iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayString(CFSTR("The specified portal does not exist\n"));
        error = EINVAL;
    }

    // Check for active sessions or connections before allowing modification
    if(!error) {
        if(portal) {
            if(iSCSIDaemonIsPortalActive(handle,target,portal))
                iSCSICtlDisplayString(CFSTR("The specified portal is connected and cannot be modified\n"));
            else {
                iSCSICtlModifyPortalFromOptions(options,portal);
                iSCSIPLSetPortalForTarget(targetIQN,portal);

                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else
                    iSCSICtlDisplayString(CFSTR("Portal settings have been updated\n"));
            }

        }
        // Else we're modifying target parameters
        else {
            if(iSCSIDaemonIsTargetActive(handle,target))
                iSCSICtlDisplayString(CFSTR("The specified target has an active session and cannot be modified\n"));
            else {
                iSCSICtlModifyTargetFromOptions(options,target,portal);
                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else
                    iSCSICtlDisplayString(CFSTR("Target settings have been updated\n"));
            }
        }
    }

    if(portal)
        iSCSIPortalRelease(portal);
    if(target)
        iSCSITargetRelease(target);

    return error;
}

/*! Helper function. Displays information about a target/session. */
void displayTargetInfo(iSCSIDaemonHandle handle,
                       iSCSITargetRef target,
                       CFDictionaryRef properties)
{
    CFStringRef targetState = NULL;
    CFStringRef targetConfig = NULL;
    CFStringRef targetIQN = iSCSITargetGetIQN(target);

    enum iSCSITargetConfigTypes configType = iSCSIPLGetTargetConfigType(targetIQN);

    switch(configType) {
        case kiSCSITargetConfigStatic:
            targetConfig = CFSTR("static"); break;
        case kiSCSITargetConfigDynamicSendTargets:
            targetConfig = CFSTR("dynamic"); break;
        case kiSCSITargetConfigInvalid:
        default:
            targetConfig = CFSTR("");
    };
    
    if(properties)
        targetState = CFSTR("active");
    else
        targetState = CFSTR("inactive");
    
    CFStringRef status = NULL;
    

    if(!properties) {
        status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                          CFSTR("%s <%@, %@>\n"),
                                          CFStringGetCStringPtr(targetIQN,kCFStringEncodingASCII),
                                          targetState,targetConfig);
    }
    else {
        CFNumberRef targetPortalGroupTag = CFDictionaryGetValue(properties,kRFC3720_Key_TargetPortalGroupTag);
        CFNumberRef targetSessionId = CFDictionaryGetValue(properties,kRFC3720_Key_TargetSessionId);
        CFNumberRef sessionId = CFDictionaryGetValue(properties,kRFC3720_Key_SessionId);
        
        TSIH tsih = 0;
        CFNumberGetValue(targetSessionId,kCFNumberSInt16Type,&tsih);

        status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                          CFSTR("%s <%@, %@, sid %@, tpgt %@, tsid %#x>\n"),
                                          CFStringGetCStringPtr(targetIQN,kCFStringEncodingASCII),
                                          targetState,
                                          targetConfig,
                                          sessionId,
                                          targetPortalGroupTag,
                                          tsih);
    }

    iSCSICtlDisplayString(status);
    CFRelease(status);
    CFRelease(targetState);
    CFRelease(targetConfig);
}

/*! Helper function. Displays information about a portal/connection. */
void displayPortalInfo(iSCSIDaemonHandle handle,
                       iSCSITargetRef target,
                       iSCSIPortalRef portal,
                       CFDictionaryRef properties)
{
    CFStringRef portalStatus = NULL;
    CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

    // If not connected
    if(!properties) {
        portalStatus = CFStringCreateWithFormat(
            kCFAllocatorDefault,NULL,
            CFSTR("\t%s <inactive, port %s, interface %@>\n"),
            CFStringGetCStringPtr(portalAddress,kCFStringEncodingASCII),
            CFStringGetCStringPtr(iSCSIPortalGetPort(portal),kCFStringEncodingASCII),
            iSCSIPortalGetHostInterface(portal));
    }
    else {
        
        CFNumberRef connectionId = CFDictionaryGetValue(properties,kRFC3720_Key_ConnectionId);
        
        portalStatus = CFStringCreateWithFormat(
            kCFAllocatorDefault,NULL,
            CFSTR("\t%s <active, cid %@, port %s, interface %@>\n"),
            CFStringGetCStringPtr(portalAddress,kCFStringEncodingASCII),
            connectionId,
            CFStringGetCStringPtr(iSCSIPortalGetPort(portal),kCFStringEncodingASCII),
            iSCSIPortalGetHostInterface(portal));
    }
    
    iSCSICtlDisplayString(portalStatus);
    CFRelease(portalStatus);
}

/*! Displays a list of targets and their associated session and connections.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlListTargets(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // We want to list all defined targets and information about any sessions
    // that may be associated with those targets, including information about
    // various portals and whether they are connected.  Targets from the
    // property list that cannot be matched to an active session are
    // assumed to be disconnected.
    
    // Read property list (synchronize)
    iSCSIPLSynchronize();

    CFArrayRef targetsList;
    
    if(!(targetsList = iSCSIPLCreateArrayOfTargets())) {
        iSCSICtlDisplayString(CFSTR("No persistent targets are defined\n"));
        return 0;
    }

    CFIndex targetCount = CFArrayGetCount(targetsList);
 
    for(CFIndex targetIdx = 0; targetIdx < targetCount; targetIdx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(targetsList,targetIdx);

        iSCSITargetRef target = NULL;
        if(!(target = iSCSIPLCopyTarget(targetIQN)))
            continue;

        CFDictionaryRef properties = NULL;
        if(!(handle < 0))
            properties = iSCSIDaemonCreateCFPropertiesForSession(handle,target);

        displayTargetInfo(handle,target,properties);
        
        CFArrayRef portalsList = iSCSIPLCreateArrayOfPortalsForTarget(targetIQN);
        CFIndex portalCount = CFArrayGetCount(portalsList);
        
        for(CFIndex portalIdx = 0; portalIdx < portalCount; portalIdx++)
        {
            iSCSIPortalRef portal = iSCSIPLCopyPortalForTarget(targetIQN,CFArrayGetValueAtIndex(portalsList,portalIdx));
            
            if(portal) {
                CFDictionaryRef properties = NULL;

                if(!(handle < 0))
                    properties = iSCSIDaemonCreateCFPropertiesForConnection(handle,target,portal);

                displayPortalInfo(handle,target,portal,properties);
                iSCSIPortalRelease(portal);
            }
        }
        
        iSCSITargetRelease(target);
        CFRelease(portalsList);
    }

    CFRelease(targetsList);
    return 0;
}

/*! Lists information about a specific target.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlListTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSIPLSynchronize();
    iSCSITargetRef target = NULL;

    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    // Verify that target exists
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    if(!iSCSIPLContainsTarget(targetIQN))
    {
        iSCSICtlDisplayString(CFSTR("The specified target does not exist\n"));
        return EINVAL;
    }

    CFDictionaryRef properties = iSCSIDaemonCreateCFPropertiesForSession(handle,target);
    displayTargetInfo(handle,target,properties);
    
    // Get information about automatic login
    CFStringRef autoLogin = CFSTR("disabled");
    
    if(iSCSIPLGetAutoLoginForTarget(targetIQN))
        autoLogin = CFSTR("enabled");
    
    CFStringRef targetConfig = CFStringCreateWithFormat(kCFAllocatorDefault,0,CFSTR("\t%@: %@\n"),kOptKeyAutoLogin,autoLogin);
    iSCSICtlDisplayString(targetConfig);
    CFRelease(targetConfig);
    
    if(iSCSIPLGetTargetConfigType(targetIQN) == kiSCSITargetConfigDynamicSendTargets)
    {
        CFStringRef discoveryPortal = iSCSIPLGetDiscoveryPortalForTarget(targetIQN);
        CFStringRef discoveryCfg = CFStringCreateWithFormat(kCFAllocatorDefault,0,CFSTR("\tdiscovery portal: %@\n"),discoveryPortal);
        iSCSICtlDisplayString(discoveryCfg);
        CFRelease(discoveryCfg);
    }

    CFStringRef targetParams, targetAuth;
    CFStringRef format = NULL;

    // Get configured parameter values
    int maxConnectionsCfg = iSCSIPLGetMaxConnectionsForTarget(targetIQN);
    enum iSCSIErrorRecoveryLevels errorRecoveryLevelCfg = iSCSIPLGetErrorRecoveryLevelForTarget(targetIQN);
    CFStringRef headerDigestStr = iSCSICtlGetStringForDigestType(iSCSIPLGetHeaderDigestForTarget(targetIQN));
    CFStringRef dataDigestStr = iSCSICtlGetStringForDigestType(iSCSIPLGetDataDigestForTarget(targetIQN));

    if(properties) {
        format = CFSTR("\tConfiguration:"
                       "\n\t\t%@ %@ (%d)"       // MaxConnections
                       "\n\t\t%@ %@ (%d)"       // ErrorRecoveryLevel
                       "\n\t\t%@ (%@)"          // HeaderDigest
                       "\n\t\t%@ (%@)");        // DataDigest


        CFNumberRef maxConnections = CFDictionaryGetValue(properties,kRFC3720_Key_MaxConnections);
        CFNumberRef errorRecoveryLevel = CFDictionaryGetValue(properties,kRFC3720_Key_ErrorRecoveryLevel);

        targetParams = CFStringCreateWithFormat(kCFAllocatorDefault,0,format,
                        kOptKeyMaxConnections,maxConnections,maxConnectionsCfg,
                        kOptKeyErrorRecoveryLevel,errorRecoveryLevel,errorRecoveryLevelCfg,
                        kOptKeyHeaderDigest,headerDigestStr,
                        kOptKeyDataDigest,dataDigestStr);
    } else {
        format = CFSTR("\tConfiguration:"
                       "\n\t\t%@ (%d)"      // MaxConnections
                       "\n\t\t%@ (%d)"      // ErrorRecoveryLevel
                       "\n\t\t%@ (%@)"      // HeaderDigest
                       "\n\t\t%@ (%@)");    // DataDigest

        targetParams = CFStringCreateWithFormat(kCFAllocatorDefault,0,format,
                        kOptKeyMaxConnections,maxConnectionsCfg,
                        kOptKeyErrorRecoveryLevel,errorRecoveryLevelCfg,
                        kOptKeyHeaderDigest,headerDigestStr,
                        kOptKeyDataDigest,dataDigestStr);
    }

    // Get authentication information
    CFStringRef CHAPName = iSCSIPLCopyTargetCHAPName(targetIQN);

    CFStringRef authMethod = NULL;
    switch(iSCSIPLGetTargetAuthenticationMethod(targetIQN)) {
        case kiSCSIAuthMethodCHAP:
            authMethod = CFSTR("CHAP"); break;
        default:
            authMethod = CFSTR("none"); break;
    };

    CFStringRef CHAPSecret = CFSTR("<specified>");
    if(!iSCSIPLExistsTargetCHAPSecret(targetIQN))
        CHAPSecret = CFSTR("<unspecified>");

    format = CFSTR("\n\tAuthentication: %@"
                   "\n\t\t%@ %@"  // CHAP-name
                   "\n\t\t%@ %@"  // CHAP-secret
                   "\n");

    targetAuth = CFStringCreateWithFormat(kCFAllocatorDefault,0,format,
                         authMethod,
                         kOptKeyCHAPName,CHAPName,
                         kOptKeyCHAPSecret,CHAPSecret);
    CFRelease(CHAPName);

    iSCSICtlDisplayString(targetParams);
    iSCSICtlDisplayString(targetAuth);

    CFArrayRef portals = iSCSIPLCreateArrayOfPortalsForTarget(targetIQN);
    CFIndex count = CFArrayGetCount(portals);

    // Iterate over portals for target and display configuration information
    for(CFIndex idx = 0; idx < count; idx++) {
        CFStringRef portalConfig = NULL;
        CFStringRef portalAddress = CFArrayGetValueAtIndex(portals,idx);
        iSCSIPortalRef portal = iSCSIPLCopyPortalForTarget(targetIQN,portalAddress);

        // Get negotiated portal parameters
        CFDictionaryRef properties = iSCSIDaemonCreateCFPropertiesForConnection(handle,target,portal);

        if(properties) {
            CFNumberRef headerDigest = CFDictionaryGetValue(properties,kRFC3720_Key_HeaderDigest);
            CFNumberRef dataDigest = CFDictionaryGetValue(properties,kRFC3720_Key_DataDigest);
            CFNumberRef maxDataRecvSegLength = CFDictionaryGetValue(properties,kRFC3720_Key_MaxRecvDataSegmentLength);

            enum iSCSIDigestTypes headerDigestType, dataDigestType;
            CFNumberGetValue(headerDigest,kCFNumberSInt32Type,&headerDigestType);
            CFNumberGetValue(dataDigest,kCFNumberSInt32Type,&dataDigestType);

            CFStringRef headerDigestString = iSCSICtlGetStringForDigestType(headerDigestType);
            CFStringRef dataDigestString = iSCSICtlGetStringForDigestType(dataDigestType);

            portalConfig = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,CFSTR("\t\t%@ %@\n"
                                            "\t\t%@ %@\n"
                                            "\t\t%@ %@\n"),
                kRFC3720_Key_HeaderDigest,headerDigestString,
                kRFC3720_Key_DataDigest,dataDigestString,
                kRFC3720_Key_MaxRecvDataSegmentLength,maxDataRecvSegLength);
        }

        displayPortalInfo(handle,target,portal,properties);
        iSCSICtlDisplayString(portalConfig);
        CFRelease(portal);
    }

    CFRelease(targetParams);
    CFRelease(targetAuth);

    return 0;
}

errno_t iSCSICtlListDiscoveryConfig(iSCSIDaemonHandle handle,CFDictionaryRef optDictionary)
{
    iSCSIPLSynchronize();

    Boolean enabled = iSCSIPLGetSendTargetsDiscoveryEnable();
    CFIndex interval = iSCSIPLGetSendTargetsDiscoveryInterval();

    CFStringRef enableString = CFSTR("disabled");
    if(enabled)
        enableString = CFSTR("enabled");

    CFStringRef format = CFSTR("\%@: %@"
                               "\n\tinterval %ld");
    CFStringRef discoveryConfig = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                           format,
                                                           kOptKeySendTargetsEnable,
                                                           enableString,
                                                           interval);
    iSCSICtlDisplayString(discoveryConfig);
    CFRelease(discoveryConfig);

    // Display a list of discovery (SendTargets) portals
    CFArrayRef portals = iSCSIPLCreateArrayOfPortalsForSendTargetsDiscovery();
    CFIndex portalCount = 0;

    if(portals) {
        portalCount = CFArrayGetCount(portals);
    }

    CFStringRef discoveryPortals = CFStringCreateWithFormat(
        kCFAllocatorDefault,0,CFSTR("\n\tdiscovery-portals: %ld\n"),portalCount);
    iSCSICtlDisplayString(discoveryPortals);
    CFRelease(discoveryPortals);


    for(CFIndex idx = 0; idx < portalCount; idx++) {
        CFStringRef portalAddress = CFArrayGetValueAtIndex(portals,idx);
        iSCSIPortalRef portal = iSCSIPLCopySendTargetsDiscoveryPortal(portalAddress);

        CFStringRef entry =
        CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                 CFSTR("\t\t%-15s <port %s, interface %@>\n"),
                                 CFStringGetCStringPtr(portalAddress,kCFStringEncodingASCII),
                                 CFStringGetCStringPtr(iSCSIPortalGetPort(portal),kCFStringEncodingASCII),
                                 iSCSIPortalGetHostInterface(portal));
        iSCSICtlDisplayString(entry);
        CFRelease(entry);
    }

    return 0;
}

errno_t iSCSICtlListInitiatorConfig(iSCSIDaemonHandle handle,CFDictionaryRef optDictionary)
{
    iSCSIPLSynchronize();

    CFStringRef initiatorIQN = iSCSIPLCopyInitiatorIQN();
    CFStringRef alias = iSCSIPLCopyInitiatorAlias();
    CFStringRef CHAPName = iSCSIPLCopyInitiatorCHAPName();

    CFStringRef authMethod = NULL;
    switch(iSCSIPLGetInitiatorAuthenticationMethod()) {
        case kiSCSIAuthMethodCHAP:
            authMethod = CFSTR("CHAP"); break;
        default:
            authMethod = CFSTR("none"); break;
    };

    CFStringRef CHAPSecret = CFSTR("<specified>");
    if(!iSCSIPLExistsInitiatorCHAPSecret())
        CHAPSecret = CFSTR("<unspecified>");


    CFStringRef format = CFSTR("%@"
                               "\n\t%@ %@"
                               "\n\tAuthentication: %@"
                               "\n\t\t%@ %@"  // CHAP-name
                               "\n\t\t%@ %@"  // CHAP-secret
                               "\n");

    CFStringRef initiatorConfig = CFStringCreateWithFormat(
        kCFAllocatorDefault,0,format,
        initiatorIQN,
        kOptKeyNodeAlias,alias,
        authMethod,
        kOptKeyCHAPName,CHAPName,
        kOptKeyCHAPSecret,CHAPSecret);

    CFRelease(initiatorIQN);
    CFRelease(alias);
    CFRelease(CHAPName);

    iSCSICtlDisplayString(initiatorConfig);
    CFRelease(initiatorConfig);

    return 0;
}

/*! Adds a discovery portal for SendTargets discovery.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlAddDiscoveryPortal(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSIPortalRef portal = NULL;
    if((portal = iSCSICtlCreatePortalFromOptions(options)))
    {
        iSCSIPLSynchronize();
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

        if(!iSCSIPLContainsPortalForSendTargetsDiscovery(portalAddress)) {
            iSCSIPLAddSendTargetsDiscoveryPortal(portal);

            if(!iSCSIPLSynchronize())
                iSCSICtlDisplayPermissionsError();
            else {
                CFStringRef status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                              CFSTR("The discovery portal %@ has been added\n"),
                                              iSCSIPortalGetAddress(portal));
                iSCSICtlDisplayString(status);
                CFRelease(status);
            }
        }
        else
            iSCSICtlDisplayString(CFSTR("The specified discovery portal already exists\n"));
    }
    return 0;
}

/*! Modifies discovery settings.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlModifyDiscovery(iSCSIDaemonHandle handle,CFDictionaryRef optDictionary)
{
    iSCSIPLSynchronize();

    CFStringRef value = NULL;
    errno_t error = 0;

    // Check if user enabled or disable a discovery method and act accordingly
    if(CFDictionaryGetValueIfPresent(optDictionary,kOptKeySendTargetsEnable,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueDiscoveryEnable,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        {
            iSCSIPLSetSendTargetsDiscoveryEnable(true);

            if(!iSCSIPLSynchronize())
                iSCSICtlDisplayPermissionsError();
            else
                iSCSICtlDisplayString(CFSTR("SendTargets discovery has been enabled\n"));
        }
        else if(CFStringCompare(value,kOptValueDiscoveryDisable,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        {
            iSCSIPLSetSendTargetsDiscoveryEnable(false);

            if(!iSCSIPLSynchronize())
                iSCSICtlDisplayPermissionsError();
            else
                iSCSICtlDisplayString(CFSTR("SendTargets discovery has been disabled\n"));

        }
        else {
            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,CFSTR("Invalid argument for %@"),kOptKeySendTargetsEnable);
            iSCSICtlDisplayError(errorString);
            CFRelease(errorString);
            error = EINVAL;
        }
    }

    // Check if user modified the discovery interval
    if(!error && CFDictionaryGetValueIfPresent(optDictionary,kOptKeyDiscoveryInterval,(const void **)&value))
    {
        int interval = CFStringGetIntValue(value);
        if(interval < kiSCSIInitiator_DiscoveryInterval_Min || interval > kiSCSIInitiator_DiscoveryInterval_Max) {

            CFStringRef errorString = CFStringCreateWithFormat(
                kCFAllocatorDefault,0,
                CFSTR("The specified discovery interval is invalid. Specify a value between %d - %d seconds"),
                kiSCSIInitiator_DiscoveryInterval_Min,kiSCSIInitiator_DiscoveryInterval_Max);
            iSCSICtlDisplayError(errorString);
            CFRelease(errorString);
        }
        else {
            iSCSIPLSetSendTargetsDiscoveryInterval(interval);
            if(!iSCSIPLSynchronize())
                iSCSICtlDisplayPermissionsError();
            else
                iSCSICtlDisplayString(CFSTR("The discovery interval has been updated\n"));
        }
    }

    if(!error)
        iSCSIDaemonUpdateDiscovery(handle);
    
    return error;
}

/*! Removes a discovery portal from SendTargets discovery.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlRemoveDiscoveryPortal(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSIPortalRef portal = NULL;
    if((portal = iSCSICtlCreatePortalFromOptions(options)))
    {
        iSCSIPLSynchronize();
        CFStringRef status = NULL;
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

        if(iSCSIPLContainsPortalForSendTargetsDiscovery(portalAddress)) {

            // If targets associated with this discovery portal are logged in,
            // notify user and do nothing
            CFMutableArrayRef activeTargets = CFArrayCreateMutable(kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
            CFArrayRef targets = iSCISPLCreateArrayOfDynamicTargetsForSendTargets(portalAddress);

            CFIndex targetsCount = CFArrayGetCount(targets);
            for(CFIndex idx = 0; idx < targetsCount; idx++)
            {
                CFStringRef targetIQN = CFArrayGetValueAtIndex(targets,idx);
                iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
                iSCSITargetSetIQN(target,targetIQN);

                if(iSCSIDaemonIsTargetActive(handle,target))
                    CFArrayAppendValue(activeTargets,targetIQN);

                iSCSITargetRelease(target);
            }

            if((targetsCount = CFArrayGetCount(activeTargets)) > 0)
            {
                iSCSICtlDisplayString(CFSTR("\nThe following active target(s) are associated with the specified discovery portal:\n\n"));

                for(CFIndex idx = 0; idx < targetsCount; idx++)
                {
                    CFStringRef targetIQN = CFArrayGetValueAtIndex(targets,idx);
                    CFStringRef targetString = CFStringCreateWithFormat(kCFAllocatorDefault,0,CFSTR("\t%@\n"),targetIQN);
                    iSCSICtlDisplayString(targetString);
                    CFRelease(targetString);
                }
                iSCSICtlDisplayString(CFSTR("\nThe active targets must be logged out before the discovery portal can be removed\n\n"));
            }
            else {
                iSCSIPLRemoveSendTargetsDiscoveryPortal(portal);
                if(!iSCSIPLSynchronize())
                    iSCSICtlDisplayPermissionsError();
                else {
                    status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                  CFSTR("The discovery portal %@ has been removed\n"),
                                                  iSCSIPortalGetAddress(portal));
                    iSCSICtlDisplayString(status);
                    CFRelease(status);
                }
            }

            CFRelease(activeTargets);
            CFRelease(targets);
        }
        else
            iSCSICtlDisplayString(CFSTR("The specified discovery portal does not exist\n"));
    }
    return 0;
}

void displayTargetProperties(CFDictionaryRef propertiesDict)
{
    CFStringRef targetVendor = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSIVendorIdentification));
    CFStringRef targetProduct = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSIProductIdentification));
    CFStringRef targetRevision = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSIProductRevisionLevel));
    CFStringRef serialNumber = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSIINQUIRYUnitSerialNumber));
    
    CFDictionaryRef protocolDict = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertyProtocolCharacteristicsKey));
    CFNumberRef domainId = CFDictionaryGetValue(protocolDict,CFSTR(kIOPropertySCSIDomainIdentifierKey));
    CFNumberRef targetId = CFDictionaryGetValue(protocolDict,CFSTR(kIOPropertySCSITargetIdentifierKey));
    CFStringRef targetIQN = CFDictionaryGetValue(protocolDict,CFSTR(kIOPropertyiSCSIQualifiedNameKey));
    
    CFStringRef targetStr = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                     CFSTR("%s <scsi domain %@, target %@>\n\t%@ %@ %@\n\tSerial Number %@\n"),
                                                     CFStringGetCStringPtr(targetIQN,kCFStringEncodingASCII),
                                                     domainId,targetId,targetVendor,targetProduct,
                                                     targetRevision,serialNumber);
    iSCSICtlDisplayString(targetStr);
}

void displayLUNProperties(CFDictionaryRef propertiesDict)
{
    if(!propertiesDict)
        return;
    
    CFNumberRef SCSILUNIdentifier = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSILogicalUnitNumberKey));
    CFNumberRef peripheralType = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSIPeripheralDeviceType));
    
    // Get a description of the peripheral device type
    int peripheralTypeCode;
    CFNumberGetValue(peripheralType,kCFNumberIntType,&peripheralTypeCode);

    CFStringRef properties = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                              CFSTR("\tlun %@: type 0x%02x (%@)\n"),
                                              SCSILUNIdentifier,peripheralTypeCode,iSCSIUtilsGetSCSIPeripheralDeviceDescription(peripheralTypeCode));
    iSCSICtlDisplayString(properties);
    CFRelease(properties);
}

void displayIOMediaProperties(CFDictionaryRef propertiesDict)
{
    if(!propertiesDict)
        return;
    
    CFStringRef BSDName = CFDictionaryGetValue(propertiesDict,CFSTR(kIOBSDNameKey));
    CFNumberRef sizeNum = CFDictionaryGetValue(propertiesDict,CFSTR(kIOMediaSizeKey));
    CFNumberRef blockSizeNum = CFDictionaryGetValue(propertiesDict,CFSTR(kIOMediaPreferredBlockSizeKey));
    
    long long size;
    CFNumberGetValue(sizeNum,kCFNumberLongLongType,&size);
    CFStringRef sizeString = (__bridge CFStringRef)
        ([NSByteCountFormatter stringFromByteCount:size countStyle:NSByteCountFormatterCountStyleFile]);
    
    int blockSize;
    CFNumberGetValue(blockSizeNum,kCFNumberIntType,&blockSize);
    
    long long blockCount = size/blockSize;
    CFNumberRef blockCountNum = CFNumberCreate(kCFAllocatorDefault,kCFNumberLongLongType,&blockCount);
    
    CFStringRef properties = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                      CFSTR("\t\t%@: %@ (%@ %@ byte blocks)\n"),
                                                      BSDName,sizeString,blockCountNum,blockSizeNum);
    iSCSICtlDisplayString(properties);
    CFRelease(properties);
}

errno_t displayTargetDeviceTree(io_object_t target)
{
    io_object_t LUN = IO_OBJECT_NULL;
    io_iterator_t LUNIterator = IO_OBJECT_NULL;

    // Do nothing if the entry was not a valid target
    CFDictionaryRef targetDict;
    if(!(targetDict = iSCSIIORegistryCreateCFPropertiesForTarget(target))) {
        return EINVAL;
    }
    
    // At this point we know that the registry entry was
    displayTargetProperties(targetDict);
    
    CFDictionaryRef protocolDict = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertyProtocolCharacteristicsKey));
    CFStringRef targetIQN = CFDictionaryGetValue(protocolDict,CFSTR(kIOPropertyiSCSIQualifiedNameKey));
    
    // Get a dictionary of properties for each LUN and display those
    iSCSIIORegistryGetLUNs(targetIQN,&LUNIterator);
    LUN = IOIteratorNext(LUNIterator);
    
    while(LUN != IO_OBJECT_NULL)
    {
        CFDictionaryRef properties = iSCSIIORegistryCreateCFPropertiesForLUN(LUN);
        displayLUNProperties(properties);
        
        io_object_t IOMedia = iSCSIIORegistryFindIOMediaForLUN(LUN);
        
        // Advance LUN iterator and detect whether this is the last LUN
        IOObjectRelease(LUN);
        LUN = IOIteratorNext(LUNIterator);
        
        if(IOMedia != IO_OBJECT_NULL) {
            CFDictionaryRef properties = iSCSIIORegistryCreateCFPropertiesForIOMedia(IOMedia);
            displayIOMediaProperties(properties);
            CFRelease(properties);
        }
        CFRelease(properties);
    }
    
    CFRelease(targetDict);
    
    IOObjectRelease(target);
    IOObjectRelease(LUNIterator);
    
    return 0;
}

/*! Displays a list of targets and their associated LUNs, including information
 *  about the size of the LUN, manufacturer, etc.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlListLUNs(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    io_object_t target = IO_OBJECT_NULL;
    io_iterator_t targetIterator = IO_OBJECT_NULL;
    
    iSCSIIORegistryGetTargets(&targetIterator);
    target = IOIteratorNext(targetIterator);
    Boolean detectedValidTarget = false;
    
    do
    {
        if(!displayTargetDeviceTree(target))
            detectedValidTarget = true;
    }
    while((target = IOIteratorNext(targetIterator)) != IO_OBJECT_NULL);

    if(!detectedValidTarget)
        iSCSICtlDisplayString(CFSTR("No active targets with LUNs exist\n"));

    IOObjectRelease(targetIterator);
    return 0;
}

errno_t iSCSICtlProbeTargetForAuthMethod(iSCSIDaemonHandle handle,
                                         CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    errno_t error = 0;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
        CFRelease(target);
        return EINVAL;
    }
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
    enum iSCSIAuthMethods authMethod;
    
    error = iSCSIDaemonQueryTargetForAuthMethod(handle,portal,
                                                iSCSITargetGetIQN(target),
                                                &authMethod,
                                                &statusCode);

    if(statusCode != kiSCSILoginInvalidStatusCode)
        iSCSICtlDisplayProbeTargetLoginStatus(statusCode,target,portal,authMethod);
    else {
        CFStringRef errorString = CFStringCreateWithCString(kCFAllocatorDefault,strerror(error),kCFStringEncodingASCII);
        iSCSICtlDisplayError(errorString);
        CFRelease(errorString);
    }
    
    return error;
}

errno_t iSCSICtlReset(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSIPLReset();
    if(!iSCSIPLSynchronize())
        iSCSICtlDisplayPermissionsError();
    else
        iSCSICtlDisplayString(CFSTR("iSCSI settings have been reset\n"));

    return 0;
}

errno_t iSCSICtlMountForTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSITargetRef target = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    iSCSIDAMountIOMediaForTarget(iSCSITargetGetIQN(target));
    CFRelease(target);
    return 0;
}

errno_t iSCSICtlUnmountForTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSITargetRef target = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    iSCSIDAUnmountIOMediaForTarget(iSCSITargetGetIQN(target));
    CFRelease(target);
    return 0;
}


/*! Entry point.  Parses command line arguments, establishes a connection to the
 *  iSCSI deamon and executes requested iSCSI tasks. */
int main(int argc, char * argv[])
{
    errno_t error = 0;

@autoreleasepool {

    // Connect to the daemon
    iSCSIDaemonHandle handle = iSCSIDaemonConnect();

    // Setup a stream for writing to stdout
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                 CFSTR("/dev/stdout"),
                                                 kCFURLPOSIXPathStyle,false);

    stdoutStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault,url);
    CFRelease(url);
    CFWriteStreamOpen(stdoutStream);

    // Retrieve command-line arguments as an NSArray
    NSArray * args = [[NSProcessInfo processInfo] arguments];
    CFArrayRef arguments = (__bridge CFArrayRef)(args);

    // Convert command-line arguments into an options dictionary. This will
    // place key-value pairs for command line options that start with a
    // dash ('-'). Options that do not start with a dash will not appear in
    // this dictionary.
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    CFDictionaryRef options = (__bridge CFDictionaryRef)([defaults dictionaryRepresentation]);

    CFMutableDictionaryRef optDictionary = (CFMutableDictionaryRef)CFPropertyListCreateDeepCopy(
        kCFAllocatorDefault,options,kCFPropertyListMutableContainersAndLeaves);

    iSCSICtlParseTargetAndPortalToDictionary(arguments,optDictionary);
    iSCSICtlParseSwitchesToDictionary(arguments,optDictionary);

    // Save command line executable name for later use
    executableName = argv[0];

    // Get the mode of operation (e.g., add, modify, remove, etc.).
    enum iSCSICtlCmds cmd = iSCSICtlGetCmdFromArguments(arguments);
    enum iSCSICtlSubCmds subCmd = iSCSICtlGetSubCmdFromArguments(arguments);

    switch(cmd)
    {
        case kiSCSICtlCmdAdd:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlAddTarget(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscoveryPortal)
                error = iSCSICtlAddDiscoveryPortal(handle,optDictionary);
            else
                iSCSICtlDisplayError(CFSTR("Invalid subcommand for add"));
            break;

        case kiSCSICtlCmdModify:
            if(subCmd == kiSCSICtlSubCmdTargetConfig)
                error = iSCSICtlModifyTarget(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdInitiatorConfig)
                error = iSCSICtlModifyInitiator(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscoveryConfig)
                error = iSCSICtlModifyDiscovery(handle,optDictionary);
            else
                iSCSICtlDisplayError(CFSTR("Invalid subcommand for modify"));
            break;

        case kiSCSICtlCmdRemove:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlRemoveTarget(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscoveryPortal)
                error = iSCSICtlRemoveDiscoveryPortal(handle,optDictionary);
            else
                iSCSICtlDisplayError(CFSTR("Invalid subcommand for remove"));
            break;

        case kiSCSICtlCmdList:
            if(subCmd == kiSCSICtlSubCmdTargets)
                error = iSCSICtlListTargets(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdTargetConfig)
                error = iSCSICtlListTarget(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdLUNs)
                error = iSCSICtlListLUNs(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscoveryConfig)
                error = iSCSICtlListDiscoveryConfig(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdInitiatorConfig)
                error = iSCSICtlListInitiatorConfig(handle,optDictionary);
            else
                iSCSICtlDisplayError(CFSTR("Invalid subcommand for list"));
            break;

        case kiSCSICtlCmdLogin:
            error = iSCSICtlLogin(handle,optDictionary); break;
        case kiSCSICtlCmdLogout:
            error = iSCSICtlLogout(handle,optDictionary); break;
        case kiSCSICtlCmdMount:
            error = iSCSICtlMountForTarget(handle,optDictionary); break;
        case kiSCSICtlCmdUnmount:
            error = iSCSICtlUnmountForTarget(handle,optDictionary); break;
        case kiSCSICtlCmdProbe:
            error = iSCSICtlProbeTargetForAuthMethod(handle,optDictionary); break;
        case kiSCSICtlCmdReset:
            error = iSCSICtlReset(handle,optDictionary); break;
        case kiSCSICtlCmdInvalid:
            iSCSICtlDisplayUsage();
            
            break;
    };

    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
}
    
    return error;
}