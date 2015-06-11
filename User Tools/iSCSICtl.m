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

/*! Add command-line mode. */
CFStringRef kModeAdd = CFSTR("-add");

/*! Discovery command-line mode. */
CFStringRef kModeDiscovery = CFSTR("-discovery");

/*! Modify command-line mode. */
CFStringRef kModeModify = CFSTR("-modify");

/*! Remove command-line mode. */
CFStringRef kModeRemove = CFSTR("-remove");

/*! List command-line mode. */
CFStringRef kModeListTargets = CFSTR("-targets");

/*! List command-line mode. */
CFStringRef kModeListLUNs = CFSTR("-luns");

/*! Login command-line mode. */
CFStringRef kModeLogin = CFSTR("-login");

/*! Logout command-line mode. */
CFStringRef kModeLogout = CFSTR("-logout");

/*! Mount command-line mode. */
CFStringRef kModeMount = CFSTR("-mount");

/*! Probe command; probes a target for authentication method. */
CFStringRef kModeProbe = CFSTR("-probe");

/*! Unmount command-line mode. */
CFStringRef kModeUnmount = CFSTR("-unmount");


/*! Sets the initiator name. */
CFStringRef kOptInitiatorName = CFSTR("InitiatorName");

/*! Sets the initiator alias. */
CFStringRef kOptInitiatorAlias = CFSTR("InitiatorAlias");

/*! Sets the maximum number of connections for a session. */
CFStringRef kOptMaxConnection = CFSTR("MaxConnections");

/*! Sets the error recovery level for a session. */
CFStringRef kOptErrorRecoveryLevel = CFSTR("ErrorRecoveryLevel");


/*! Target command-line option. */
CFStringRef kOptTarget = CFSTR("target");

/*! Portal command-line option. */
CFStringRef kOptPortal = CFSTR("portal");

/*! Interface command-line option. */
CFStringRef kOptInterface = CFSTR("interface");

/*! Session identifier command-line option. */
CFStringRef kOptSessionId = CFSTR("session");

/*! Digest command-line options. */
CFStringRef kOptDigest = CFSTR("digest");

/*! User (CHAP) command-line option. */
CFStringRef kOptUser = CFSTR("user");

/*! Secret (CHAP) command-line option. */
CFStringRef kOptSecret = CFSTR("secret");

/*! Mutual-user (CHAP) command-line option. */
CFStringRef kOptMutualUser = CFSTR("mutualUser");

/*! Mutual-secret (CHAP) command-line option. */
CFStringRef kOptMutualSecret = CFSTR("mutualSecret");


// TODO: the "all" flag should be used to remove/login/logout "all" sessions
// it needs to be impelemented in respective functions below
/*! All command-line option. */
CFStringRef kOptAll = CFSTR("all");


/*! Name of this command-line executable. */
const char * executableName;

/*! Command line arguments (used for getopt()). */
//const char * kShortOptions = "AMLRlp:t:i:df:vac:n:us:q:r:";

/*! The standard out stream, used by various functions to write. */
CFWriteStreamRef stdoutStream = NULL;

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
 *  @param target the target
 *  @param portal the portal used to logon to the target. */
void iSCSICtlDisplayLoginStatus(enum iSCSILoginStatusCode statusCode,
                        iSCSITargetRef target,
                        iSCSIPortalRef portal)
{
    CFStringRef targetIQN      = iSCSITargetGetIQN(target);
    CFStringRef portalAddress   = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort      = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    CFStringRef loginStr;
    
    if(statusCode == kiSCSILoginSuccess)
    {
        loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
            CFSTR("Login to [ target: %@ portal: %@:%@ if: %@ ] successful.\n"),
            targetIQN,portalAddress,
            portalPort,portalInterface);
        
        iSCSICtlDisplayString(loginStr);
        CFRelease(loginStr);
        return;
    }

    loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
        CFSTR("Login to [ target: %@ portal: %@:%@ if: %@ ] failed: %@.\n"),
        targetIQN,portalAddress,portalPort,
        portalInterface,iSCSICtlGetStringForLoginStatus(statusCode));
    
    iSCSICtlDisplayString(loginStr);
    CFRelease(loginStr);
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
iSCSITargetRef iSCSICtlCreateTargetFromOptions(CFDictionaryRef options)
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
        iSCSICtlDisplayError("The specified iSCSI portal is invalid.");
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    
    // If portal is present, set it
    if(CFArrayGetCount(portalParts) > 1)
        iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
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
    CFStringRef user = NULL, secret = NULL, mutualUser = NULL, mutualSecret = NULL;
    CFDictionaryGetValueIfPresent(options,kOptUser,(const void **)&user);
    CFDictionaryGetValueIfPresent(options,kOptSecret,(const void **)&secret);
    CFDictionaryGetValueIfPresent(options,kOptMutualUser,(const void **)&mutualUser);
    CFDictionaryGetValueIfPresent(options,kOptMutualSecret,(const void **)&mutualSecret);
    
    if(!user && secret) {
        iSCSICtlDisplayMissingOptionError(kOptUser);
        return NULL;
    }
    else if(user && !secret) {
        iSCSICtlDisplayMissingOptionError(kOptSecret);
        return NULL;
    }
    
    if(!mutualUser && mutualSecret) {
        iSCSICtlDisplayMissingOptionError(kOptMutualUser);
        return NULL;
    }
    else if(mutualUser && !mutualSecret) {
        iSCSICtlDisplayMissingOptionError(kOptMutualSecret);
        return NULL;
    }
    
    // At this point we've validated input combinations, if the user is not
    // present then we're dealing with no authentication.  Otherwise we have
    // either CHAP or mutual CHAP
    if(!user)
        return iSCSIAuthCreateNone();
    else
        return iSCSIAuthCreateCHAP(user,secret,mutualUser,mutualSecret);
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
    }
    
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

errno_t iSCSICtlLoginSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    
    // Create target object from user input (may be null if user didn't specify)
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
        iSCSICtlDisplayError("The specified target does not exist.");
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Portal is optional for a login operation (omission logs in all portals)
    if(iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
        {
            iSCSITargetRelease(target);
            return EINVAL;
        }
    }
    
    // Verify that portal exists
    if(portal && !iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayError("The specified portal does not exist.");
        
        iSCSIPortalRelease(portal);
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,targetIQN,&sessionId);
    
    if(sessionId != kiSCSIInvalidSessionId && portal)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    if(error)
        iSCSICtlDisplayError(strerror(error));
    
    // There's an active session and connection for the target/portal
    if(sessionId != kiSCSIInvalidSessionId && connectionId != kiSCSIInvalidConnectionId) {
        iSCSICtlDisplayError("The specified target has an active session over the specified portal.");
    }
    else if(!error)
    {
        // At this point we're doing some kind of login
        iSCSISessionConfigRef sessCfg = NULL;
        
        // If the session needs to be logged in...
        if(sessionId == kiSCSIInvalidSessionId)
        {
            // Grab session configuration from property list
            if(!(sessCfg = iSCSIPLCopySessionConfig(targetIQN)))
               sessCfg = iSCSISessionConfigCreateMutable();
        }

        if(portal) {
            // Check property list for If a valid portal was specified, then check the database for
            // configuration for the portal
            iSCSIConnectionConfigRef connCfg = NULL;
            iSCSIAuthRef auth = NULL;
            // User may have only specified the portal name (address).  We must
            // get the preconfigured port and/or interface from the property list.
            CFStringRef portalAddress = CFStringCreateCopy(kCFAllocatorDefault,iSCSIPortalGetAddress(portal));
            iSCSIPortalRelease(portal);
            portal = iSCSIPLCopyPortal(targetIQN,portalAddress);
            CFRelease(portalAddress);
            
            
            // Grab connection configuration from property list
            if(!(connCfg = iSCSIPLCopyConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal))))
               connCfg = iSCSIConnectionConfigCreateMutable();
            
            // If user didn't specify any authentication options, check the
            // database for options (if the user's input was incorrect, don't
            // revert to defaults or the property list; instead quit)
            if(!error && !auth)
                auth = iSCSIPLCopyAuthentication(targetIQN,iSCSIPortalGetAddress(portal));

            // If it doesn't exist in the database create a new one with defaults
            if(!error && !auth)
                auth = iSCSIAuthCreateNone();
            
            enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
            
            // Do either session login or add a connection...
            if(!error && sessionId == kiSCSIInvalidSessionId)
                error = iSCSIDaemonLoginSession(handle,portal,target,auth,sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
            else if(!error)
                error = iSCSIDaemonLoginConnection(handle,sessionId,portal,auth,connCfg,&connectionId,&statusCode);

            if(!error)
                iSCSICtlDisplayLoginStatus(statusCode,target,portal);
            else
                iSCSICtlDisplayError(strerror(error));

        }
    }
    
    iSCSITargetRelease(target);
   
    return error;
}

errno_t iSCSICtlLogoutSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    // See if there exists an active session for this target
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,iSCSITargetGetIQN(target),&sessionId);
    
    if(!error && sessionId == kiSCSIInvalidSessionId)
    {
        iSCSICtlDisplayError("The specified target has no active session.");
        error = EINVAL;
    }
    
    if(iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
            return EINVAL;
    }
    
    // See if there exists an active connection for this portal
    if(!error && portal)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    // If the portal was specified and a connection doesn't exist for it...
    if(!error && portal && connectionId == kiSCSIInvalidConnectionId)
    {
        iSCSICtlDisplayError("The specified portal has no active connections.");
        error = EINVAL;
    }
    
    // At this point either the we logout the session or just the connection
    // associated with the specified portal, if one was specified
    if(!error)
    {
        enum iSCSILogoutStatusCode statusCode;
        
        if(!portal)
            error = iSCSIDaemonLogoutSession(handle,sessionId,&statusCode);
        else
            error = iSCSIDaemonLogoutConnection(handle,sessionId,connectionId,&statusCode);
        
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
            
            if(!iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal))) {
                // Create an authentication object from user-specified switches
                iSCSIAuthRef auth;
                if(!(auth = iSCSICtlCreateAuthFromOptions(options))) {
                    error = EINVAL;
                } else {
                    // Setup optional session or connection configuration from switches
                    iSCSIMutableSessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
                    iSCSIMutableConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();

                    error = iSCSICtlModifySessionConfigFromOptions(options,sessCfg);

                    if(!error)
                        error = iSCSICtlModifyConnectionConfigFromOptions(options,connCfg);
                    
                    if(!error)
                    {
                        iSCSIPLSetPortal(targetIQN,portal);
                        iSCSIPLSetAuthentication(targetIQN,iSCSIPortalGetAddress(portal),auth);
                        iSCSIPLSetSessionConfig(targetIQN,sessCfg);
                        iSCSIPLSetConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal),connCfg);
                        
                        iSCSIPLSynchronize();
                    }
                    
                    if(auth)
                        iSCSIAuthRelease(auth);
                
                    iSCSISessionConfigRelease(sessCfg);
                    iSCSIConnectionConfigRelease(connCfg);
                }
            } else {
                iSCSICtlDisplayError("The specified target and portal already exist.");
            }
            iSCSIPortalRelease(portal);
        }
        if (target)
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
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
       return EINVAL;
       
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
        iSCSICtlDisplayError("The specified target does not exist.");
        
        iSCSITargetRelease(target);
        return EINVAL;
    }

    if(iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            iSCSITargetRelease(target);
            return EINVAL;
        }
    }

    // Verify that portal exists in property list
    if(portal && !iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayError("The specified portal does not exist.");
        
        iSCSIPortalRelease(portal);
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,targetIQN,&sessionId);
    
    if(sessionId != kiSCSIInvalidSessionId && portal)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    if(error)
        iSCSICtlDisplayError(strerror(error));
    else if(!portal)
    {
        if(sessionId != kiSCSIInvalidSessionId)
            iSCSICtlDisplayError("The specified target has an active session and cannot be removed.");
        else if(sessionId == kiSCSIInvalidSessionId) {
            iSCSIPLRemoveTarget(targetIQN);
            iSCSIPLSynchronize();
        }
    }
    else if(portal)
    {
        if(connectionId != kiSCSIInvalidConnectionId)
            iSCSICtlDisplayError("The specified portal is connected and cannot be removed.");
        else if(connectionId == kiSCSIInvalidConnectionId) {
            iSCSIPLRemovePortal(targetIQN,iSCSIPortalGetAddress(portal));
            iSCSIPLSynchronize();
        }
    }

    if(portal)
        iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);
    
    return error;
}

errno_t iSCSICtlModifyTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // First check if the target exists in the propery list.  Then check to
    // see if it has an active session.  If it does, target properties cannot
    // be modified.  Do the same for portal (to change connection properties,
    // but this is optional and a portal may not have been specified by the
    // user).
    
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
        iSCSICtlDisplayError("The specified target does not exist.");
        
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    if(iSCSICtlIsPortalSpecified(options)) {
        if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
            iSCSITargetRelease(target);
            return EINVAL;
        }
    }
    
    // Verify that portal exists in property list
    if(portal && !iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal))) {
        iSCSICtlDisplayError("The specified portal does not exist.");
        
        iSCSIPortalRelease(portal);
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,targetIQN,&sessionId);
    
    if(sessionId != kiSCSIInvalidSessionId && portal)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    if(error)
        iSCSICtlDisplayError(strerror(error));
    else
    {
        if(sessionId != kiSCSIInvalidSessionId)
            iSCSICtlDisplayError("The specified target has an active session and cannot be modified.");
        else if(sessionId == kiSCSIInvalidSessionId) {
            iSCSISessionConfigRef sessCfg = iSCSIPLCopySessionConfig(targetIQN);
            iSCSIMutableSessionConfigRef config = iSCSISessionConfigCreateMutableWithExisting(sessCfg);
            
            if(!(error = iSCSICtlModifySessionConfigFromOptions(options,config)))
                iSCSIPLSetSessionConfig(targetIQN,config);
            
            iSCSISessionConfigRelease(config);
            iSCSISessionConfigRelease(sessCfg);
        }

        // If the portal was specified, make connection parameter changes
        if(portal)
        {
            if(connectionId != kiSCSIInvalidConnectionId)
                iSCSICtlDisplayError("The specified portal is connected and cannot be modified.");
            else if(connectionId == kiSCSIInvalidConnectionId) {
                iSCSIConnectionConfigRef connCfg = iSCSIPLCopyConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal));
                iSCSIMutableConnectionConfigRef config = iSCSIConnectionConfigCreateMutableWithExisting(connCfg);
 
                if(!(error = iSCSICtlModifyConnectionConfigFromOptions(options,config)))
                    iSCSIPLSetConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal),config);
            }
        }
    }
    
   if(!error)
       iSCSIPLSynchronize();
    
    if(portal)
        iSCSIPortalRelease(portal);
    iSCSITargetRelease(target);

    return error;
}

/*! Helper function. Displays information about a target/session. */
void displayTargetInfo(iSCSIDaemonHandle handle,
                       iSCSITargetRef target,
                       SID sessionId)
{
    CFStringRef targetStatus = NULL, targetIQN = iSCSITargetGetIQN(target);
    
    if(sessionId != kiSCSIInvalidSessionId)
    {
        targetStatus = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                CFSTR(" [ SID: %d ]\n"),sessionId);
    }
    else
        targetStatus = CFSTR(" [ Not connected ]\n");
    
    iSCSICtlDisplayString(targetIQN);
    iSCSICtlDisplayString(targetStatus);
    
    CFRelease(targetStatus);
}

/*! Helper function. Displays information about a portal/connection. */
void displayPortalInfo(iSCSIDaemonHandle handle,
                       iSCSITargetRef target,
                       iSCSIPortalRef portal,
                       SID sessionId,
                       CID connectionId)
{
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef portalStatus = NULL;
    CFStringRef portalAddress = iSCSIPortalGetAddress(portal);

    iSCSIConnectionConfigRef connCfg;
    if(connectionId != kiSCSIInvalidConnectionId)
        connCfg = iSCSIDaemonCopyConnectionConfig(handle,sessionId,connectionId);
    else
        connCfg = iSCSIPLCopyConnectionConfig(targetIQN,portalAddress);
    
    Boolean dataDigest = iSCSIConnectionConfigGetDataDigest(connCfg);
    Boolean headerDigest = iSCSIConnectionConfigGetHeaderDigest(connCfg);
    
    CFStringRef digestStr;
    if(dataDigest && headerDigest)
        digestStr = CFSTR("Enabled");
    else if(dataDigest)
        digestStr = CFSTR("Data Only");
    else if(headerDigest)
        digestStr = CFSTR("Header Only");
    else
        digestStr = CFSTR("None");
    
    // Get authentication method in use and generate a status string for it
    iSCSIAuthRef auth = iSCSIPLCopyAuthentication(targetIQN,portalAddress);

    enum iSCSIAuthMethods authMethod = iSCSIAuthGetMethod(auth);
    CFStringRef authStr = CFSTR("-");
    
    if(authMethod == kiSCSIAuthMethodCHAP)
        authStr = CFSTR("CHAP");
    else if(authMethod == kiSCSIAuthMethodNone)
        authStr = CFSTR("None");
    
    
    if(connectionId == kiSCSIInvalidConnectionId) {
        portalStatus = CFStringCreateWithFormat(
            kCFAllocatorDefault,NULL,CFSTR("\t%@ [ CID: -, Port: %@, Iface: %@, Auth: %@, Digest: %@ ]\n"),
            portalAddress,iSCSIPortalGetPort(portal),iSCSIPortalGetHostInterface(portal),authStr,digestStr);
    }
    else {
        portalStatus = CFStringCreateWithFormat(
            kCFAllocatorDefault,NULL,
            CFSTR("\t%@ [ CID: %d, Port: %@, Iface: %@, Auth: %@, Digest: %@ ]\n"),
            portalAddress,connectionId,iSCSIPortalGetPort(portal),iSCSIPortalGetHostInterface(portal),authStr,digestStr);
    }
    
    iSCSICtlDisplayString(portalStatus);
    CFRelease(portalStatus);
    
    iSCSIConnectionConfigRelease(connCfg);
    iSCSIAuthRelease(auth);
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
 
    for(CFIndex targetIdx = 0; targetIdx < CFArrayGetCount(targetsList); targetIdx++)
    {
        CFStringRef targetIQN = CFArrayGetValueAtIndex(targetsList,targetIdx);
        
        SID sessionId = kiSCSIInvalidSessionId;
        iSCSIDaemonGetSessionIdForTarget(handle,targetIQN,&sessionId);
        
        iSCSITargetRef target = iSCSIPLCopyTarget(targetIQN);
        displayTargetInfo(handle,target,sessionId);
        
        CFArrayRef portalsList = iSCSIPLCreateArrayOfPortals(targetIQN);
        
        for(CFIndex portalIdx = 0; portalIdx < CFArrayGetCount(portalsList); portalIdx++)
        {
            iSCSIPortalRef portal = iSCSIPLCopyPortal(targetIQN,CFArrayGetValueAtIndex(portalsList,portalIdx));
            if (portal) {
                CID connectionId = kiSCSIInvalidConnectionId;
                iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
                
                displayPortalInfo(handle,target,portal,sessionId,connectionId);
                
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
                                                      CFSTR("\tLUN: %@ [ Peripheral Device Type: 0x%02x (%@) ]\n"),
                                                      SCSILUNIdentifier,peripheralTypeCode,peripheralDesc);
    iSCSICtlDisplayString(properties);
    CFRelease(properties);
}

void displayIOMediaProperties(CFDictionaryRef propertiesDict)
{
    if(!propertiesDict)
        return;
    
    CFStringRef BSDName = CFDictionaryGetValue(propertiesDict,CFSTR(kIOBSDNameKey));
    CFNumberRef size = CFDictionaryGetValue(propertiesDict,CFSTR(kIOMediaSizeKey));
    
    long long capacity;
    CFNumberGetValue(size,kCFNumberLongLongType,&capacity);
    CFStringRef capacityString = (__bridge CFStringRef)
        ([NSByteCountFormatter stringFromByteCount:capacity countStyle:NSByteCountFormatterCountStyleFile]);
    
    CFStringRef properties = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                      CFSTR("\t\t%@ [ Capacity: %@ ]\n"),
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
                                                         CFSTR("\n%@ [ Target Id: %@, Vendor: %@, Model: %@ ]\n"),
                                                         targetIQN,targetId,targetVendor,targetProduct);
        iSCSICtlDisplayString(targetStr);

        // Get a dictionary of properties for each LUN and display those
        iSCSIIORegistryGetLUNs(targetIQN,&LUNIterator);
        while((LUN = IOIteratorNext(LUNIterator)) != IO_OBJECT_NULL)
        {
            CFDictionaryRef properties = iSCSIIORegistryCreateCFPropertiesForLUN(LUN);
            displayLUNProperties(properties);
            
            io_object_t IOMedia = iSCSIIORegistryFindIOMediaForLUN(LUN);

            if(IOMedia != IO_OBJECT_NULL) {
                CFDictionaryRef properties = iSCSIIORegistryCreateCFPropertiesForIOMedia(IOMedia);
                displayIOMediaProperties(properties);
                CFRelease(properties);
            }
            IOObjectRelease(LUN);
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

errno_t iSCSICtlProbeTargetForAuthMethod(iSCSIDaemonHandle handle,CFDictionaryRef options)
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
    
    NSArray * args = [[NSProcessInfo processInfo] arguments];
    CFArrayRef arguments = (__bridge CFArrayRef)(args);
    
    NSUserDefaults * options = [NSUserDefaults standardUserDefaults];
    CFDictionaryRef optDictionary = (__bridge CFDictionaryRef)([options dictionaryRepresentation]);
    
    // Setup a stream for writing to stdout
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,CFSTR("/dev/stdout"),kCFURLPOSIXPathStyle,false);
    stdoutStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault,url);
    CFRelease(url);
    CFWriteStreamOpen(stdoutStream);
    
    // Save command line executable name for later use
    executableName = argv[0];
    
    // Connect to the daemon
    iSCSIDaemonHandle handle = iSCSIDaemonConnect();
    
    if(handle < 0) {
        iSCSICtlDisplayError("iscsid is not running.");
        CFWriteStreamClose(stdoutStream);
        return -1;
    }
    
    // Get the mode of operation (e.g., add, modify, remove, etc).
    CFStringRef mode = NULL;
    if(CFArrayGetCount(arguments) > 1)
        mode = CFArrayGetValueAtIndex(arguments,1);
    else {
        iSCSIDaemonDisconnect(handle);
        CFWriteStreamClose(stdoutStream);
        return 0;
    }
    
    
    // Process add, modify, remove or list
    if(CFStringCompare(mode,kModeAdd,0) == kCFCompareEqualTo)
        error = iSCSICtlAddTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kModeDiscovery,0) == kCFCompareEqualTo)
        error = iSCSICtlDiscoverTargets(handle,optDictionary);
    else if(CFStringCompare(mode,kModeModify,0) == kCFCompareEqualTo)
        error = iSCSICtlModifyTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kModeRemove,0) == kCFCompareEqualTo)
        error = iSCSICtlRemoveTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kModeListTargets,0) == kCFCompareEqualTo)
        error = iSCSICtlListTargets(handle,optDictionary);
    else if(CFStringCompare(mode,kModeListLUNs,0) == kCFCompareEqualTo)
        error = iSCSICtlListLUNs(handle,optDictionary);
    else if(CFStringCompare(mode,kModeProbe,0) == kCFCompareEqualTo)
        error = iSCSICtlProbeTargetForAuthMethod(handle,optDictionary);
    else if(CFStringCompare(mode,kModeMount,0) == kCFCompareEqualTo)
        error = iSCSICtlMountForTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kModeUnmount,0) == kCFCompareEqualTo)
        error = iSCSICtlUnmountForTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kOptInitiatorName,0) == kCFCompareEqualTo)
        error = iSCSICtlSetInitiatorName(handle,optDictionary);
    else if(CFStringCompare(mode,kOptInitiatorAlias,0) == kCFCompareEqualTo)
        error = iSCSICtlSetInitiatorAlias(handle,optDictionary);
    
    // Then process login or logout in tandem
    if(CFStringCompare(mode,kModeLogin,0) == kCFCompareEqualTo)
        error = iSCSICtlLoginSession(handle,optDictionary);
    else if(CFStringCompare(mode,kModeLogout,0) == kCFCompareEqualTo)
        error = iSCSICtlLogoutSession(handle,optDictionary);
    
    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
}
    
    return error;
}