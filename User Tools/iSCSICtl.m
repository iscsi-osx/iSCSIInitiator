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
CFStringRef kOptTarget = CFSTR("target");

/*! Portal command-line option. */
CFStringRef kOptPortal = CFSTR("portal");

/*! Port command-line option. */
CFStringRef kOptPort = CFSTR("port");

/*! Interface command-line option. */
CFStringRef kOptInterface = CFSTR("interface");

/*! Autostart command-line option. */
CFStringRef kOptAutostart = CFSTR("autostart");

/*! Session identifier command-line option. */
CFStringRef kOptSessionId = CFSTR("session");

/*! Digest command-line options. */
CFStringRef kOptDigest = CFSTR("digest");

/*! User (CHAP) command-line option. */
CFStringRef kOptCHAPUser = CFSTR("CHAP-name");

/*! Secret (CHAP) command-line option. */
CFStringRef kOptCHAPSecret = CFSTR("CHAP-secret");

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
    CFDictionaryAddValue(subModesDict,CFSTR("discovery-portals"),(const void *)kiSCSICtlSubCmdDiscovery);
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
            CFDictionaryAddValue(optDictionary,kOptTarget,CFArrayGetValueAtIndex(argParts,0));
            CFDictionaryAddValue(optDictionary,kOptPortal,CFArrayGetValueAtIndex(argParts,1));
        }
        // Need to determine whether a portal or target was specified
        else {
            // Targets must start with IQN or EUI
            CFRange matchIQN = CFStringFind(arg,CFSTR("iqn."),kCFCompareCaseInsensitive);
            CFRange matchEUI = CFStringFind(arg,CFSTR("eui."),kCFCompareCaseInsensitive);

            if(matchIQN.location == kCFNotFound && matchEUI.location == kCFNotFound)
                CFDictionaryAddValue(optDictionary,kOptPortal,arg);
            else
                CFDictionaryAddValue(optDictionary,kOptTarget,arg);
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
        "usage: iscisctl ...");
    
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
        } else {
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
 *  @param portal the portal used to logon to the target. */
void iSCSICtlDisplayDiscoveryLoginStatus(enum iSCSILoginStatusCode statusCode,
                                         iSCSIPortalRef portal)
{
    CFStringRef portalAddress   = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort      = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    CFStringRef loginStatus;
    
    // Do nothing if successful (print discovery results)
    if(statusCode != kiSCSILoginSuccess)
        return;
    
    loginStatus = CFStringCreateWithFormat(kCFAllocatorDefault,0,
        CFSTR("Discovery using [portal: %@:%@ if: %@] failed: %@.\n"),
        portalAddress,portalPort,portalInterface,
        iSCSICtlGetStringForLoginStatus(statusCode));
    
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
    return CFDictionaryContainsKey(options,kOptTarget);
}

/*! Creates a target object from command-line options.  If the target is not
 *  well-formatted or missing then the function displays an error message and
 *  returns NULL.
 *  @param options command-line options.
 *  @return the target object, or NULL. */
iSCSIMutableTargetRef iSCSICtlCreateTargetFromOptions(CFDictionaryRef options)
{
    CFStringRef targetIQN;
    if(!CFDictionaryGetValueIfPresent(options,kOptTarget,(const void **)&targetIQN))
    {
        iSCSICtlDisplayMissingOptionError(kOptTarget);
        return NULL;
    }
    
    if(!iSCSIUtilsValidateIQN(targetIQN)) {
        iSCSICtlDisplayError("The specified iSCSI target is invalid.");
        return NULL;
    }

    iSCSIMutableTargetRef target = iSCSITargetCreateMutable();
    iSCSITargetSetName(target,targetIQN);

    return target;
}

/*! Helper function.  Returns true if the user attempted to specify a portal
 *  as a command line argument (even if it was unsuccessful or malformed).
 *  @param options command-line options.
 *  @return true if a portal was specified. */
Boolean iSCSICtlIsPortalSpecified(CFDictionaryRef options)
{
    return CFDictionaryContainsKey(options,kOptPortal);
}

/*! Creates a portal object from command-line options.  If the portal is not
 *  well-formatted or missing then the function displays an error message and
 *  returns NULL.
 *  @param options command-line options.
 *  @return the portal object, or NULL. */
iSCSIMutablePortalRef iSCSICtlCreatePortalFromOptions(CFDictionaryRef options)
{
    CFStringRef portalAddress, hostInterface;
    
    if(!CFDictionaryGetValueIfPresent(options,kOptPortal,(const void **)&portalAddress))
    {
        iSCSICtlDisplayMissingOptionError(kOptPortal);
        return NULL;
    }
    
    // Returns an array of hostname, port strings (port optional)
    CFArrayRef portalParts = iSCSIUtilsCreateArrayByParsingPortalParts(portalAddress);
    
    if(!portalParts) {
        iSCSICtlDisplayError("The specified portal is invalid.");
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    
    // If port was specified as part of portal (e.g., 192.168.1.100:3260)
    if(CFArrayGetCount(portalParts) > 1) {
        iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    }
    // If port was specified separately (e.g., -port 3260)
    else if (CFDictionaryContainsKey(options,kOptPort)) {
        CFStringRef port = CFDictionaryGetValue(options,kOptPort);
        if(iSCSIUtilsValidatePort(port))
            iSCSIPortalSetPort(portal,port);
        else {
            iSCSICtlDisplayError("The specified port is invalid.");
            return NULL;
        }
    }
    else
        iSCSIPortalSetPort(portal,kiSCSIDefaultPort);
    
    CFRelease(portalParts);

    // If an interface was not specified, use the default interface
    if(!CFDictionaryGetValueIfPresent(options,kOptInterface,(const void **)&hostInterface))
        iSCSIPortalSetHostInterface(portal,kiSCSIDefaultHostInterface);
    else
        iSCSIPortalSetHostInterface(portal,hostInterface);

    return portal;
}


/*! Creates a authentication object from command-line options.  If authentication
 *  is not well-formatted or missing components then the function displays an 
 *  error message and returns NULL.  If authentication is absent entirely this
 *  function returns a valid authentication object indicating no authentication.
 *  @param options command-line options.
 *  @return the authentication object, or NULL. */
iSCSIAuthRef iSCSICtlCreateAuthFromOptions(CFDictionaryRef options)
{
    if(!options)
        return NULL;
    
    // Setup authentication object from command-line parameters
    CFStringRef user = NULL, secret = NULL;
    CFDictionaryGetValueIfPresent(options,kOptCHAPUser,(const void **)&user);
    CFDictionaryGetValueIfPresent(options,kOptCHAPSecret,(const void **)&secret);

    if(!user && secret) {
        iSCSICtlDisplayMissingOptionError(kOptCHAPUser);
        return NULL;
    }
    else if(user && !secret) {
        iSCSICtlDisplayMissingOptionError(kOptCHAPSecret);
        return NULL;
    }

    // At this point we've validated input combinations, if the user is not
    // present then we're dealing with no authentication.  Otherwise we have
    // either CHAP or mutual CHAP
    if(!user)
        return iSCSIAuthCreateNone();
    else
        return iSCSIAuthCreateCHAP(user,secret);
}

/*! Modifies an existing session configuration parameters object 
 *  based on user-supplied command-line switches.
 *  @param options command-line options.
 *  @param sessCfg the session configuration parameters object.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlModifySessionConfigFromOptions(CFDictionaryRef options,
                                               iSCSIMutableSessionConfigRef sessCfg)
{
    if(!sessCfg)
        return EINVAL;
/*
    CFStringRef errorRecoveryLevel, maxConnections;
    if(CFDictionaryGetValueIfPresent(options,kOptErrorRecoveryLevel,(const void **)&errorRecoveryLevel))
    {
        NSString * errorRecoveryLevelStr = (__bridge NSString*)errorRecoveryLevel;
        enum iSCSIErrorRecoveryLevels errorRecoveryLevel = [errorRecoveryLevelStr intValue];
        
        if(errorRecoveryLevel == kiSCSIErrorRecoveryInvalid) {
            iSCSICtlDisplayError("the specified error recovery level is invalid.");
            return EINVAL;
        }

        iSCSISessionConfigSetErrorRecoveryLevel(sessCfg,errorRecoveryLevel);
    }
    
    if(CFDictionaryGetValueIfPresent(options,kOptMaxConnection,(const void**)&maxConnections))
    {
        NSString * maxConnectionsStr = (__bridge NSString*)maxConnections;
        int maxConnections = [maxConnectionsStr intValue];
        
        if(maxConnections > kRFC3720_MaxConnections_Max || maxConnections < kRFC3720_MaxConnections_Min)
        {
            iSCSICtlDisplayError("the specified number of maximum connections is invalid.");
            return EINVAL;
        }
        
        iSCSISessionConfigSetMaxConnections(sessCfg,maxConnections);
    }*/

    return 0;
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
    if(CFDictionaryGetValueIfPresent(options,kOptDigest,(const void**)&digest))
    {
        if(CFStringCompare(digest,CFSTR("on"),0) == kCFCompareEqualTo) {
            iSCSIConnectionConfigSetHeaderDigest(connCfg,true);
            iSCSIConnectionConfigSetDataDigest(connCfg,true);
        }
        else if(CFStringCompare(digest,CFSTR("off"),0) == kCFCompareEqualTo) {
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
    if(CFDictionaryContainsKey(options,kOptPort))
        iSCSIPortalSetPort(portal,iSCSIPortalGetPort(portalUpdates));

    // If the interface was explicitly specified, update it
    if(CFDictionaryContainsKey(options,kOptInterface))
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
        iSCSICtlDisplayError("The specified target does not exist.");
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
                CFStringRef portalAddress = CFStringCreateCopy(kCFAllocatorDefault,iSCSIPortalGetAddress(portal));
                iSCSIPortalRelease(portal);
                portal = iSCSIPLCopyPortalForTarget(targetIQN,portalAddress);
                CFRelease(portalAddress);

                if(iSCSIDaemonIsPortalActive(handle,target,portal))
                    iSCSICtlDisplayError("The specified target has an active session over the specified portal.");
                else {
                    error = iSCSIDaemonLogin(handle,target,portal,&statusCode);

                    if(!error)
                        iSCSICtlDisplayLoginStatus(statusCode,target,portal);
                }
            }
            else
                iSCSICtlDisplayError("The specified portal does not exist.");

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
        iSCSICtlDisplayError("The specified target has no active session.");
        error = EINVAL;
    }
    
    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
            error = EINVAL;
    }
    
    // If the portal was specified and a connection doesn't exist for it...
    if(!error && portal && !iSCSIDaemonIsPortalActive(handle,target,portal))
    {
        iSCSICtlDisplayError("The specified portal has no active connections.");
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
        iSCSIPortalRef portal = iSCSICtlCreatePortalFromOptions(options);
        
        if (target && portal) {
            // Synchronize the database with the property list on disk
            iSCSIPLSynchronize();
            
            // If portal and target both exist then do nothing, otherwise
            // add target and or portal with user-specified options
            CFStringRef targetIQN = iSCSITargetGetIQN(target);
            
            if(!iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {

                // Setup optional session or connection configuration from switches
                iSCSIAuthRef auth = iSCSIAuthCreateNone();
                iSCSIMutableSessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
                iSCSIMutableConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();

                iSCSIPLSetTarget(target);
                iSCSIPLSetPortalForTarget(targetIQN,portal);
                iSCSIPLSetAuthenticationForTarget(targetIQN,auth);
                iSCSIPLSetSessionConfig(targetIQN,sessCfg);
                iSCSIPLSetConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal),connCfg);
                    
                iSCSIPLSynchronize();

                iSCSIAuthRelease(auth);
                iSCSISessionConfigRelease(sessCfg);
                iSCSIConnectionConfigRelease(connCfg);

            } else {
                iSCSICtlDisplayError("The specified target and portal already exist.");
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
        iSCSICtlDisplayError("The specified target does not exist.");
        error = EINVAL;
    }

    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            error = EINVAL;
        }
    }

    // Verify that portal exists in property list
    if(!error && portal && !iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayError("The specified portal does not exist.");
        error = EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    if(!error) {
        if(portal)
            if(iSCSIDaemonIsPortalActive(handle,target,portal))
                iSCSICtlDisplayError("The specified portal is connected and cannot be removed.");
            else {
                iSCSIPLRemovePortalForTarget(targetIQN,iSCSIPortalGetAddress(portal));
                iSCSIPLSynchronize();
            }
        else
            if(iSCSIDaemonIsTargetActive(handle,target))
                iSCSICtlDisplayError("The specified target has an active session and cannot be removed.");
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
    // Check for authentication modifications
    CFStringRef initiatorUser, sharedSecret = NULL;
    if(CFDictionaryGetValueIfPresent(options,kOptCHAPUser,(const void **)&initiatorUser))
    {
        if(CFDictionaryGetValueIfPresent(options,kOptCHAPSecret,(const void **)&sharedSecret))
        {
            iSCSIAuthRef auth = iSCSIAuthCreateCHAP(initiatorUser,sharedSecret);
            iSCSIPLSetAuthenticationForInitiator(auth);
            iSCSIPLSynchronize();
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
        iSCSICtlDisplayError("The specified target does not exist.");
        error = EINVAL;
    }

    if(!error && iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            error = EINVAL;
        }
    }

    // Verify that portal exists in property list
    if(!error && portal && !iSCSIPLContainsPortalForTarget(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayError("The specified portal does not exist.");
        error = EINVAL;
    }

    // Check for active sessions or connections before allowing modification
    if(!error) {
        if(portal) {
            if(iSCSIDaemonIsPortalActive(handle,target,portal))
                iSCSICtlDisplayError("The specified portal is connected and cannot be modified.");
            else {
                iSCSICtlModifyPortalFromOptions(options,portal);
                iSCSIPLSetPortalForTarget(targetIQN,portal);
            }

        }
        // Else we're modifying target-level parameters
        else {
            if(iSCSIDaemonIsTargetActive(handle,target))
                iSCSICtlDisplayError("The specified target has an active session and cannot be modified.");
            else {
                //                iSCSICtlModifyTargetFromOptions(options,target);
                //                iSCSIPLSetTarget(target);
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
    CFStringRef targetStatus = NULL, targetIQN = iSCSITargetGetIQN(target);
    
    if(properties)
        targetStatus = CFSTR(" [  Active  ]\n");
    else
        targetStatus = CFSTR(" [ Inactive ]\n");

    iSCSICtlDisplayString(CFSTR("+-o "));
    iSCSICtlDisplayString(targetIQN);
    iSCSICtlDisplayString(targetStatus);
    
    CFRelease(targetStatus);
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
        iSCSITargetRef target;

        if(!(target = iSCSIPLCopyTarget(targetIQN)))
            continue;

        CFDictionaryRef properties = iSCSIDaemonCreateCFPropertiesForSession(handle,target);

        displayTargetInfo(handle,target,properties);
        
        CFArrayRef portalsList = iSCSIPLCreateArrayOfPortalsForTarget(targetIQN);
        CFIndex portalCount = CFArrayGetCount(portalsList);
        
        for(CFIndex portalIdx = 0; portalIdx < portalCount; portalIdx++)
        {
            iSCSIPortalRef portal = iSCSIPLCopyPortalForTarget(targetIQN,CFArrayGetValueAtIndex(portalsList,portalIdx));
            
            if(portal) {
                CFDictionaryRef properties = iSCSIDaemonCreateCFPropertiesForConnection(handle,target,portal);
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
                                                      CFSTR("  +-o LUN: %@ [ Peripheral Device Type: 0x%02x (%@) ]\n"),
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
    
    while((target = IOIteratorNext(targetIterator)) != IO_OBJECT_NULL)
    {
        CFDictionaryRef targetDict = iSCSIIORegistryCreateCFPropertiesForTarget(target);
        
        CFStringRef targetIQN = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertyiSCSIQualifiedNameKey));
        CFStringRef targetVendor = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSIVendorIdentification));
        CFStringRef targetProduct = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSIProductIdentification));
        CFNumberRef targetId = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSITargetIdentifierKey));
        
        CFStringRef targetStr = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                         CFSTR("\n+-o %@ [ Target Id: %@, Vendor: %@, Model: %@ ]\n"),
                                                         targetIQN,targetId,targetVendor,targetProduct);
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

errno_t iSCSICtlSetInitiatorName(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
// TODO: implement
    return 0;
}

errno_t iSCSICtlSetInitiatorAlias(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
// TODO: implement
    return 0;
}

/*! Displays the contents of a discovery record, including targets, their
 *  associated portals and portal group tags.
 *  @param discoveryRecord the discovery record to display.
 *  @param portal the portal used for discovery.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlDisplayDiscoveryRecord(iSCSIDiscoveryRecRef discoveryRecord)
{
    CFArrayRef targets = iSCSIDiscoveryRecCreateArrayOfTargets(discoveryRecord);
    
    for(CFIndex idxTarget = 0; idxTarget < CFArrayGetCount(targets); idxTarget++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(targets,idxTarget);
        iSCSICtlDisplayString(targetIQN);
        
        if(!targetIQN)
            continue;
        
        CFArrayRef portalGroups = iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(discoveryRecord,targetIQN);
        
        for(CFIndex idxTPG = 0; idxTPG < CFArrayGetCount(portalGroups); idxTPG++)
        {
            CFStringRef TPGT = CFArrayGetValueAtIndex(portalGroups,idxTPG);
            iSCSICtlDisplayString(CFSTR("\n"));
            
            CFArrayRef portals = iSCSIDiscoveryRecGetPortals(discoveryRecord,targetIQN,TPGT);
            
            for(CFIndex idxPortal = 0; idxPortal < CFArrayGetCount(portals); idxPortal++)
            {
                iSCSIPortalRef portal = CFArrayGetValueAtIndex(portals,idxPortal);
                CFStringRef portalStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                                     CFSTR("\t%@ [ Port: %@, PortalGroup: %@ ]\n"),
                                                     iSCSIPortalGetAddress(portal),
                                                     iSCSIPortalGetPort(portal),TPGT);
                iSCSICtlDisplayString(portalStr);
                CFRelease(portalStr);
            }
        }
        CFRelease(portalGroups);
    }
    CFRelease(targets);
    return 0;
}

errno_t iSCSICtlDiscoverTargets(iSCSIDaemonHandle handle,CFDictionaryRef options)
{

    errno_t result = EINVAL; // default result
    if (handle && options) {
        iSCSIPortalRef portal = iSCSICtlCreatePortalFromOptions(options);
        iSCSIPLSynchronize();

        if (portal) {
            iSCSIAuthRef auth = iSCSICtlCreateAuthFromOptions(options);
            
            enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
            iSCSIMutableDiscoveryRecRef discoveryRecord;
            result = iSCSIDaemonQueryPortalForTargets(handle,portal,auth,&discoveryRecord,&statusCode);
            if (result) {
                if(statusCode != kiSCSILoginInvalidStatusCode)
                    iSCSICtlDisplayDiscoveryLoginStatus(statusCode,portal);
                else
                    iSCSICtlDisplayError(strerror(result));
                
            } else {
                iSCSICtlDisplayDiscoveryRecord(discoveryRecord);
                iSCSIPLAddDiscoveryRecord(discoveryRecord);
                iSCSIPLSynchronize();

            }
            iSCSIDiscoveryRecRelease(discoveryRecord);
            iSCSIAuthRelease(auth);
        } else {
            iSCSIDiscoveryRecRef discoveryRec = iSCSIPLCopyDiscoveryRecord();
            if (!discoveryRec) {
                result = 0;
            } else {
                iSCSICtlDisplayDiscoveryRecord(discoveryRec);
                iSCSIDiscoveryRecRelease(discoveryRec);
            }
        }
        iSCSIPortalRelease(portal);
    }
    return result;
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
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,CFSTR("/dev/stdout"),kCFURLPOSIXPathStyle,false);
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
            {}
            break;

        case kiSCSICtlCmdModify:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlModifyTarget(handle,optDictionary);
            if(subCmd == kiSCSICtlSubCmdInitiator)
                error = iSCSICtlModifyInitiator(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscovery)
            {}

            break;
        case kiSCSICtlCmdRemove:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlRemoveTarget(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdDiscovery)
            {}


        case kiSCSICtlCmdList:
            if(subCmd == kiSCSICtlSubCmdTarget)
                error = iSCSICtlListTargets(handle,optDictionary);
            else if(subCmd == kiSCSICtlSubCmdLUNs)
                error = iSCSICtlListLUNs(handle,optDictionary);
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
    };

    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
}
    
    return error;
}