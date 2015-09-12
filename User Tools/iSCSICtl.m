/*!
 * @author		Nareg Sinenian
 * @file		iSCSICtl.m
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI management utility.
 */

#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>

#include "iSCSITypes.h"
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

    /*! Invalid mode of operation. */
    kiSCSICtlCmdInvalid
};


/*! Sub-modes of operation for this utility. */
enum iSCSICtlSubCmds {

    /*! Sub mode for initiator operations. */
    kiSCSICtlSubCmdInitiator,

    /*! Sub mode for target operations. */
    kiSCSICtlSubCmdTarget,

    /*! Sub mode for discovery portal operations. */
    kiSCSICtlSubCmdDiscovery,

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

/*! Autostart command-line option. */
CFStringRef kOptAutostart = CFSTR("autostart");

/*! Digest command-line options. */
CFStringRef kOptKeyDigest = CFSTR("digest");



/*! Authentication method to use. */
CFStringRef kOptKeyAutMethod = CFSTR("authentication");

/*! CHAP authentication value for authentication key. */
CFStringRef kOptValueAuthMethodCHAP = CFSTR("CHAP");

/*! No authentication value for authentication key. */
CFStringRef kOptValueAuthMethodNone = CFSTR("None");

/*! User (CHAP) command-line option. */
CFStringRef kOptKeyCHAPUser = CFSTR("CHAP-name");

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



/*! Name of this command-line executable. */
const char * executableName;

/*! The standard out stream, used by various functions to write. */
CFWriteStreamRef stdoutStream = NULL;




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
    CFDictionaryAddValue(subModesDict,CFSTR("initiator"),(const void *)kiSCSICtlSubCmdInitiator);
    CFDictionaryAddValue(subModesDict,CFSTR("target"),(const void *)kiSCSICtlSubCmdTarget);
    CFDictionaryAddValue(subModesDict,CFSTR("discovery-portal"),(const void *)kiSCSICtlSubCmdDiscovery);
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

/*! Displays a list of valid command-line options. */
void iSCSICtlDisplayUsage()
{
// TODO: print usage string
    CFStringRef usage = CFSTR(
        "usage: iscisctl ...\n");
    
    iSCSICtlDisplayString(usage);
    CFRelease(usage);
}

/*! Displays error for a missing option.
 *  @param option the missing option. */
void iSCSICtlDisplayMissingOptionError(CFStringRef option)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,
        NULL,
        CFSTR("%s: required option -%s\n"),
        executableName,
        CFStringGetCStringPtr(option,kCFStringEncodingASCII));
    
    iSCSICtlDisplayString(error);
    CFRelease(error);
}

/*! Displays a generic error message.
 *  @param errorString the error message to display. */
void iSCSICtlDisplayError(const char * errorString)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %s\n"),executableName,errorString);
    
    iSCSICtlDisplayString(error);
    CFRelease(error);
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
            return CFSTR(".\n");

        case kiSCSILoginUnsupportedVer:
            return CFSTR("Target is incompatible with the initiator");

        case kiSCSILoginInvalidStatusCode:
        default:
            return CFSTR("Unknown error occurred");
    };
    return CFSTR("");
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
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef loginStatus;

    if (portal) {
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFStringRef portalPort    = iSCSIPortalGetPort(portal);
        CFStringRef hostInterface = iSCSIPortalGetHostInterface(portal);
        if (statusCode == kiSCSILogoutSuccess) {
            loginStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                CFSTR("Login to [ target: %@ portal: %@:%@ if: %@ ] successful.\n"),
                targetIQN,portalAddress,portalPort,hostInterface);
        }
        else
        {
            loginStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                CFSTR("Login to [ target: %@ portal: %@:%@ if: %@ ] failed: %@.\n"),
                targetIQN,portalAddress,portalPort,
                hostInterface,iSCSICtlGetStringForLoginStatus(statusCode));
        }

    } else {
        if (statusCode == kiSCSILogoutSuccess) {
            loginStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                    CFSTR("Login to [ target: %@ ] successful.\n"),targetIQN);
        } else {
            loginStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                    CFSTR("Login to [ target: %@ portal: <all> ] failed: %@.\n"),
                                                    targetIQN,
                                                    iSCSICtlGetStringForLoginStatus(statusCode));
        }
    }
    iSCSICtlDisplayString(loginStatus);
    CFRelease(loginStatus);
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
        case kiSCSIAuthMethodCHAP: probeStatus = CFSTR("CHAP required"); break;
        case kiSCSIAuthMethodNone: probeStatus = CFSTR("No authentication required"); break;
        case kiSCSIAuthMethodInvalid:
        default:
            probeStatus = CFSTR("Target requires an invalid authentication method"); break;
    };

    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    probeStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
        CFSTR("Probing [target: %@ portal: %@:%@ if: %@]: %@.\n"),
        targetIQN,portalAddress,portalPort,portalInterface,probeStatus);
    
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

void iSCSICtlDisplayLogoutStatus(enum iSCSILogoutStatusCode statusCode,
                                 iSCSITargetRef target,
                                 iSCSIPortalRef portal)
{
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef logoutStatus;
    if (portal) {
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
        CFStringRef portalPort    = iSCSIPortalGetPort(portal);
        CFStringRef hostInterface = iSCSIPortalGetHostInterface(portal);
        if (statusCode == kiSCSILogoutSuccess) {
            logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                    CFSTR("Logout of [ target: %@ portal: %@:%@ if: %@ ] successful.\n"),
                                                    targetIQN,portalAddress,portalPort,hostInterface);
        } else {
            logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                    CFSTR("Logout of [ target: %@ portal: %@:%@ if: %@ ] failed: %@.\n"),
                                                    targetIQN,portalAddress,portalPort,
                                                    hostInterface,iSCSICtlGetStringForLogoutStatus(statusCode));
        }
        
    } else {
        if (statusCode == kiSCSILogoutSuccess) {
            logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                    CFSTR("Logout of [ target: %@ ] successful.\n"),targetIQN);
        } else {
            logoutStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                    CFSTR("Logout of [ target: %@ portal: <all> ] failed: %@.\n"),
                                                    targetIQN,
                                                    iSCSICtlGetStringForLogoutStatus(statusCode));
        }
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
        iSCSICtlDisplayError("a target must be specified using a valid IQN or EUI-64 identifier.");
        return NULL;
    }
    
    if(!iSCSIUtilsValidateIQN(targetIQN)) {
        iSCSICtlDisplayError("the specified target name is not a valid IQN or EUI-64 identifier.");
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
        iSCSICtlDisplayError("a portal must be specified.");
        return NULL;
    }
    
    // Returns an array of hostname, port strings (port optional)
    CFArrayRef portalParts = iSCSIUtilsCreateArrayByParsingPortalParts(portalAddress);
    
    if(!portalParts) {
        iSCSICtlDisplayError("the specified portal is invalid.");
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
            iSCSICtlDisplayError("the specified port is invalid.");
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

/*! Modifies an existing connection configuration parameters object
 *  based on user-supplied command-line switches.
 *  @param options command-line options.
 *  @param connCfg the connection configuration parameters object.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlModifyConnectionConfigFromOptions(CFDictionaryRef options,
                                                  iSCSIMutableConnectionConfigRef connCfg)
{
    if(!connCfg)
        return EINVAL;
    
    CFStringRef digest;
    if(CFDictionaryGetValueIfPresent(options,kOptKeyDigest,(const void**)&digest))
    {
        if(CFStringCompare(digest,CFSTR("on"),kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
            iSCSIConnectionConfigSetHeaderDigest(connCfg,true);
            iSCSIConnectionConfigSetDataDigest(connCfg,true);
        }
        else if(CFStringCompare(digest,CFSTR("off"),kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
            iSCSIConnectionConfigSetHeaderDigest(connCfg,false);
            iSCSIConnectionConfigSetDataDigest(connCfg,false);
        }
    }

    return 0;
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

    // If autostart was cahnged, update it
    if(CFDictionaryContainsKey(options,kOptAutostart))
        iSCSIPortalSetAutostart(portal,iSCSIPortalGetAutostart(portalUpdates));

    return 0;
}


errno_t iSCSICtlLogin(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;

    errno_t error = EINVAL;
    iSCSITargetRef target = NULL;
    
    // Create target object from user input (may be null if user didn't specify)
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return error;
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    CFStringRef targetIQN = iSCSITargetGetIQN(target);

    if(!iSCSIPLContainsTarget(targetIQN)) {
        iSCSICtlDisplayError("the specified target does not exist.");
        iSCSITargetRelease(target);
        return error;
    }

    enum iSCSILoginStatusCode statusCode;

    // Determine whether we're logging into a single portal or all portals...
    if(iSCSICtlIsPortalSpecified(options))
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
                    iSCSICtlDisplayError("the specified target has an active "
                                         "session over the specified portal.");
                else {
                    error = iSCSIDaemonLogin(handle,target,portal,&statusCode);

                    if(!error)
                        iSCSICtlDisplayLoginStatus(statusCode,target,portal);
                }
            }
            else
                iSCSICtlDisplayError("the specified portal does not exist.");

            iSCSIPortalRelease(portal);
        }
    }
    else {
        error = iSCSIDaemonLogin(handle,target,NULL,&statusCode);

        if(!error)
            iSCSICtlDisplayLoginStatus(statusCode,target,NULL);
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
        iSCSICtlDisplayError("the specified target has no active session.");
        error = EINVAL;
    }
    
    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
            error = EINVAL;
    }
    
    // If the portal was specified and a connection doesn't exist for it...
    if(!error && portal && !iSCSIDaemonIsPortalActive(handle,target,portal))
    {
        iSCSICtlDisplayError("the specified portal has no active connections.");
        error = EINVAL;
    }
    
    // At this point either the we logout the session or just the connection
    // associated with the specified portal, if one was specified
    if(!error)
    {
        enum iSCSILogoutStatusCode statusCode;
        error = iSCSIDaemonLogout(handle, target, portal, &statusCode);

        if(!error)
            iSCSICtlDisplayLogoutStatus(statusCode,target,portal);
        else
            iSCSICtlDisplayError(strerror(error));
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
                iSCSIPLSynchronize();
            }
            else if(!iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
                iSCSIPLSetPortalForTarget(targetIQN,portal);
                iSCSIPLSynchronize();
            }
            else {
                iSCSICtlDisplayError("the specified target and portal already exist.");
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
        iSCSICtlDisplayError("the specified target does not exist.");
        error = EINVAL;
    }

    // Verify that the target is not a dynamic target managed by iSCSI
    // discovery
    if(iSCSIPLGetTargetConfigType(targetIQN) == kiSCSITargetConfigDynamicSendTargets) {
        iSCSICtlDisplayError("the specified target is dynamically configured "
                             "using iSCSI discovery and cannot be removed.");
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
        iSCSICtlDisplayError("the specified portal does not exist.");
        error = EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    if(!error) {
        if(portal)
            if(iSCSIDaemonIsPortalActive(handle,target,portal))
                iSCSICtlDisplayError("the specified portal is connected and cannot be removed.");
            else {
                iSCSIPLRemovePortalForTarget(targetIQN,iSCSIPortalGetAddress(portal));
                iSCSIPLSynchronize();
            }
        else
            if(iSCSIDaemonIsTargetActive(handle,target))
                iSCSICtlDisplayError("the specified target has an active session and cannot be removed.");
            else {
                iSCSIPLRemoveTarget(targetIQN);
                iSCSIPLSynchronize();

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

    // Check for CHAP user name
    if(CFDictionaryGetValueIfPresent(options,kOptKeyCHAPUser,(const void **)&value))
        iSCSIPLSetInitiatorCHAPName(value);

    // Check for CHAP shared secret
    if(CFDictionaryGetValueIfPresent(options,kOptKeyCHAPSecret,(const void **)&value))
        iSCSIPLSetInitiatorCHAPSecret(value);

    // Check for authentication method
    if(CFDictionaryGetValueIfPresent(options,kOptKeyAutMethod,(const void**)&value))
    {
        if(CFStringCompare(value,kOptValueAuthMethodNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetInitiatorAuthenticationMethod(kiSCSIAuthMethodNone);
        else if(CFStringCompare(value,kOptValueAuthMethodCHAP,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetInitiatorAuthenticationMethod(kiSCSIAuthMethodCHAP);
        else
            iSCSICtlDisplayError("the specified authentication method is invalid.");
    }

    // Check for initiator alias
    if(CFDictionaryGetValueIfPresent(options,kOptKeyNodeAlias,(const void**)&value))
        iSCSIPLSetInitiatorAlias(value);

    // Check for initiator IQN
    if(CFDictionaryGetValueIfPresent(options,kOptKeyNodeName,(const void **)&value))
    {
        // Validate the chosen initiator IQN
        if(iSCSIUtilsValidateIQN(value))
            iSCSIPLSetInitiatorIQN(value);
        else
            iSCSICtlDisplayError("the specified name is not a valid IQN or EUI-64 identifier.");
    }

    iSCSIPLSynchronize();

    return 0;
}

errno_t iSCSICtlModifyTargetFromOptions(CFDictionaryRef options,
                                        iSCSITargetRef target,
                                        iSCSIPortalRef portal)
{
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef value = NULL;

    iSCSIPLSynchronize();

    // Check for CHAP user name
    if(CFDictionaryGetValueIfPresent(options,kOptKeyCHAPUser,(const void **)&value))
        iSCSIPLSetTargetCHAPName(targetIQN,value);

    // Check for CHAP shared secret
    if(CFDictionaryGetValueIfPresent(options,kOptKeyCHAPSecret,(const void **)&value))
        iSCSIPLSetTargetCHAPSecret(targetIQN,value);

    // Check for authentication method
    enum iSCSIAuthMethods authMethod = kiSCSIAuthMethodInvalid;

    if(CFDictionaryGetValueIfPresent(options,kOptKeyAutMethod,(const void**)&value))
    {
        if(CFStringCompare(value,kOptValueAuthMethodNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            authMethod = kiSCSIAuthMethodNone;
        else if(CFStringCompare(value,kOptValueAuthMethodCHAP,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            authMethod = kiSCSIAuthMethodCHAP;
        else
            iSCSICtlDisplayError("the specified authentication method is invalid.");
    }

    iSCSIPLSetTargetAuthenticationMethod(targetIQN,authMethod);

    // Check for target IQN
    if(CFDictionaryGetValueIfPresent(options,kOptKeyNodeName,(const void **)&value))
    {
        // Validate the chosen target IQN
        if(iSCSIUtilsValidateIQN(value))
            iSCSIPLSetTargetIQN(targetIQN,value);
        else
            iSCSICtlDisplayError("the specified name is not a valid IQN or EUI-64 identifier.");
    }

    // Check for maximum connections
    if(CFDictionaryGetValueIfPresent(options,kOptKeyMaxConnections,(const void **)&value))
    {
        UInt32 maxConnections = CFStringGetIntValue(value);
        if(maxConnections < kRFC3720_MaxConnections_Min)
            iSCSICtlDisplayError("specified maximum number of connections is exceeds the maximum.");
        else if(maxConnections > kRFC3720_MaxConnections_Max)
            iSCSICtlDisplayError("specified maximum number of connections is not sufficient.");
        else
            iSCSIPLSetMaxConnectionsForTarget(targetIQN,maxConnections);
    }

    // Check for error recovery level
    if(CFDictionaryGetValueIfPresent(options,kOptKeyErrorRecoveryLevel,(const void **)&value))
    {
        enum iSCSIErrorRecoveryLevels level = CFStringGetIntValue(value);

        if(level < kRFC3720_ErrorRecoveryLevel_Min || level > kRFC3720_ErrorRecoveryLevel_Max)
            iSCSICtlDisplayError("she specified error recovery level is invalid.");

        iSCSIPLSetErrorRecoveryLevelForTarget(targetIQN,level);
    }

    // Check for header digest
    if(CFDictionaryGetValueIfPresent(options,kOptKeyHeaderDigest,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueDigestNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetHeaderDigestForTarget(targetIQN,kiSCSIDigestNone);
        else if(CFStringCompare(value,kOptValueDigestCRC32C,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetHeaderDigestForTarget(targetIQN,kiSCSIDigestCRC32C);
        else
            iSCSICtlDisplayError("she specified digest type is invalid.");
    }

    // Check for data digest
    if(CFDictionaryGetValueIfPresent(options,kOptKeyDataDigest,(const void **)&value))
    {
        if(CFStringCompare(value,kOptValueDigestNone,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetDataDigestForTarget(targetIQN,kiSCSIDigestNone);
        else if(CFStringCompare(value,kOptValueDigestCRC32C,kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            iSCSIPLSetDataDigestForTarget(targetIQN,kiSCSIDigestCRC32C);
        else
            iSCSICtlDisplayError("she specified digest type is invalid.");
    }

    iSCSIPLSynchronize();

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
        iSCSICtlDisplayError("the specified target does not exist.");
        error = EINVAL;
    }

    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            error = EINVAL;
        }
    }

    // Verify that portal exists in property list
    if(!error && portal && !iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayError("the specified portal does not exist.");
        error = EINVAL;
    }

    // Check for active sessions or connections before allowing modification
    if(!error) {
        if(portal) {
            if(iSCSIDaemonIsPortalActive(handle,target,portal))
                iSCSICtlDisplayError("the specified portal is connected and cannot be modified.");
            else {
                iSCSICtlModifyPortalFromOptions(options,portal);
                iSCSIPLSetPortalForTarget(targetIQN,portal);
            }

        }
        // Else we're modifying target parameters
        else {
            if(iSCSIDaemonIsTargetActive(handle,target))
                iSCSICtlDisplayError("the specified target has an active session and cannot be modified.");
            else {
                iSCSICtlModifyTargetFromOptions(options,target,portal);
            }
        }
    }

    if(!error)
        iSCSIPLSynchronize();

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
            targetConfig = CFSTR("Static"); break;
        case kiSCSITargetConfigDynamicSendTargets:
            targetConfig = CFSTR("Dynamic"); break;
        case kiSCSITargetConfigInvalid:
        default:
            targetConfig = CFSTR("");
    };
    
    if(properties)
        targetState = CFSTR("Active");
    else
        targetState = CFSTR("Inactive");

    CFStringRef status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                  CFSTR("+-o %-50s [ %@, %@ ]\n"),
                                                  CFStringGetCStringPtr(targetIQN,kCFStringEncodingASCII),
                                                  targetState,targetConfig);

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
            CFSTR("  +-o %-15s [ Not Connected, Port: %5s, Interface: %@ ]\n"),
            CFStringGetCStringPtr(portalAddress,kCFStringEncodingASCII),
            CFStringGetCStringPtr(iSCSIPortalGetPort(portal),kCFStringEncodingASCII),
            iSCSIPortalGetHostInterface(portal));
    }
    else {
        portalStatus = CFStringCreateWithFormat(
            kCFAllocatorDefault,NULL,
            CFSTR("  +-o %-15s [ Connected,     Port: %5s, Interface: %@ ]\n"),
            CFStringGetCStringPtr(portalAddress,kCFStringEncodingASCII),
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
        iSCSICtlDisplayString(CFSTR("No persistent targets are defined.\n"));
        return 0;
    }
    
    iSCSICtlDisplayString(CFSTR("\n"));
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
        
        iSCSICtlDisplayString(CFSTR("\n"));
        
        iSCSITargetRelease(target);
        CFRelease(portalsList);
    }

    CFRelease(targetsList);
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
        CFStringRef status = NULL;
        CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

        if(!iSCSIPLContainsPortalForSendTargetsDiscovery(portalAddress)) {
            iSCSIPLAddSendTargetsDiscoveryPortal(portal);
            iSCSIPLSynchronize();
            status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                              CFSTR("The discovery portal %@ has been added.\n"),
                                              iSCSIPortalGetAddress(portal));
        }
        else {
            status = CFSTR("The specified discovery portal already exists.\n");
        }

        iSCSICtlDisplayString(status);
        CFRelease(status);
    }
    return 0;
}

/*! Modifies a discovery portal for SendTargets discovery.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlModifyDiscoveryPortal(iSCSIDaemonHandle handle,CFDictionaryRef options)
{


    return 0;
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
            iSCSIPLRemoveSendTargetsDiscoveryPortal(portal);
            iSCSIPLSynchronize();
            status = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                              CFSTR("The discovery portal %@ has been removed.\n"),
                                              iSCSIPortalGetAddress(portal));
        }
        else {
            status = CFSTR("The specified discovery portal does not exist.\n");
        }

        iSCSICtlDisplayString(status);
        CFRelease(status);
    }
    return 0;
}

void displayLUNProperties(CFDictionaryRef propertiesDict)
{
    if(!propertiesDict)
        return;
    
    CFNumberRef SCSILUNIdentifier = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSILogicalUnitNumberKey));
    CFNumberRef peripheralType = CFDictionaryGetValue(propertiesDict,CFSTR(kIOPropertySCSIPeripheralDeviceType));
    
    // Get a string description of the peripheral device type
    UInt8 peripheralTypeCode;
    CFNumberGetValue(peripheralType,kCFNumberIntType,&peripheralTypeCode);
    
    CFStringRef peripheralDesc = iSCSIUtilsGetSCSIPeripheralDeviceDescription(peripheralTypeCode);
    CFStringRef properties = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                      CFSTR("  +-o LUN: %@ [ Type: 0x%02x (%@) ]\n"),
                                                      SCSILUNIdentifier,peripheralTypeCode,peripheralDesc);
    iSCSICtlDisplayString(properties);
    CFRelease(properties);
}

void displayIOMediaProperties(CFDictionaryRef propertiesDict,Boolean lastLUN)
{
    if(!propertiesDict)
        return;
    
    CFStringRef BSDName = CFDictionaryGetValue(propertiesDict,CFSTR(kIOBSDNameKey));
    CFNumberRef size = CFDictionaryGetValue(propertiesDict,CFSTR(kIOMediaSizeKey));
    
    long long capacity;
    CFNumberGetValue(size,kCFNumberLongLongType,&capacity);
    CFStringRef capacityString = (__bridge CFStringRef)
        ([NSByteCountFormatter stringFromByteCount:capacity countStyle:NSByteCountFormatterCountStyleFile]);

    CFStringRef templateString = NULL;
    if(lastLUN)
        templateString = CFSTR("    +-o %@ [ Capacity: %@ ]\n");
    else
        templateString = CFSTR("  | +-o %@ [ Capacity: %@ ]\n");
    
    CFStringRef properties = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                      templateString,
                                                      BSDName,capacityString);
    iSCSICtlDisplayString(properties);
    CFRelease(properties);
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
    
    io_object_t LUN = IO_OBJECT_NULL;
    io_iterator_t LUNIterator = IO_OBJECT_NULL;
    
    iSCSIIORegistryGetTargets(&targetIterator);
    target = IOIteratorNext(targetIterator);

    if(target == IO_OBJECT_NULL) {
        iSCSICtlDisplayString(CFSTR("No active targets with LUNs exist.\n"));
        IOObjectRelease(targetIterator);
        return 0;
    }
    
    do
    {
        // Do nothing if the entry was not a valid target
        CFDictionaryRef targetDict;
        if(!(targetDict = iSCSIIORegistryCreateCFPropertiesForTarget(target)))
            break;
        
        CFStringRef targetIQN = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertyiSCSIQualifiedNameKey));
        CFStringRef targetVendor = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSIVendorIdentification));
        CFStringRef targetProduct = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSIProductIdentification));
        CFNumberRef targetId = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSITargetIdentifierKey));
        
        CFStringRef targetStr = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                         CFSTR("\n+-o %-50s [ Id: %@, Vendor: %@, Model: %@ ]\n"),
                                                         CFStringGetCStringPtr(targetIQN,kCFStringEncodingASCII),
                                                         targetId,targetVendor,targetProduct);
        iSCSICtlDisplayString(targetStr);

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
            
            bool lastLUN = false;
            LUN = IOIteratorNext(LUNIterator);
            if(LUN == IO_OBJECT_NULL)
                lastLUN = true;


            if(IOMedia != IO_OBJECT_NULL) {
                CFDictionaryRef properties = iSCSIIORegistryCreateCFPropertiesForIOMedia(IOMedia);
                displayIOMediaProperties(properties,lastLUN);
                CFRelease(properties);
            }
            CFRelease(properties);
        }
        
        CFRelease(targetStr);
        CFRelease(targetDict);
        
        IOObjectRelease(target);
        IOObjectRelease(LUNIterator);
        
        iSCSICtlDisplayString(CFSTR("\n"));
    }
    while((target = IOIteratorNext(targetIterator)) != IO_OBJECT_NULL);
    
    IOObjectRelease(targetIterator);
    return 0;
}

errno_t iSCSICtlListDiscoveryPortals(iSCSIDaemonHandle handle,
                                     CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;

    iSCSIPLSynchronize();

    CFArrayRef portals = iSCSIPLCreateArrayOfPortalsForSendTargetsDiscovery();
    CFIndex portalCount = 0;

    if(portals)
        portalCount = CFArrayGetCount(portals);

    if(portalCount == 0)
        iSCSICtlDisplayString(CFSTR("No discovery portals are defined.\n"));
    else
        iSCSICtlDisplayString(CFSTR("\nThe following discovery portal(s) are defined:\n"));

    for(CFIndex idx = 0; idx < portalCount; idx++) {
        CFStringRef portalAddress = CFArrayGetValueAtIndex(portals,idx);
        iSCSIPortalRef portal = iSCSIPLCopySendTargetsDiscoveryPortal(portalAddress);

        CFStringRef entry =
            CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                     CFSTR("\t %-15s [ Port: %5s, Interface: %@ ]\n"),
                                     CFStringGetCStringPtr(portalAddress,kCFStringEncodingASCII),
                                     CFStringGetCStringPtr(iSCSIPortalGetPort(portal),kCFStringEncodingASCII),
                                     iSCSIPortalGetHostInterface(portal));
        iSCSICtlDisplayString(entry);
        CFRelease(entry);
    }

    if(portalCount != 0)
        iSCSICtlDisplayString(CFSTR("\n"));

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
    else
        iSCSICtlDisplayError(strerror(error));
    
    return error;
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
            else if(subCmd == kiSCSICtlSubCmdDiscovery)
                error = iSCSICtlAddDiscoveryPortal(handle,optDictionary);
            break;

        case kiSCSICtlCmdModify:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlModifyTarget(handle,optDictionary);
            if(subCmd == kiSCSICtlSubCmdInitiator)
                error = iSCSICtlModifyInitiator(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscovery)
                error = iSCSICtlModifyDiscoveryPortal(handle,optDictionary);

            break;
        case kiSCSICtlCmdRemove:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlRemoveTarget(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscovery)
                error = iSCSICtlRemoveDiscoveryPortal(handle,optDictionary);
            break;


        case kiSCSICtlCmdList:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlListTargets(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdLUNs)
                error = iSCSICtlListLUNs(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscovery)
                error = iSCSICtlListDiscoveryPortals(handle,optDictionary);
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
        case kiSCSICtlCmdInvalid:
            iSCSICtlDisplayUsage();
    };

    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
}
    
    return error;
}