/*!
 * @author		Nareg Sinenian
 * @file		main.h
 * @date		June 29, 2014
 * @version		1.0
 * @copyright	(c) 2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI management utility.
 */

#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>

#include <regex.h>

#include "iSCSITypes.h"
#include "iSCSIPropertyList.h"
#include "iSCSIDaemonInterface.h"
#include "iSCSIDA.h"


/*! Add command-line option. */
CFStringRef kSwitchAdd = CFSTR("-add");

/*! Modify command-line option. */
CFStringRef kSwitchModify = CFSTR("-modify");

/*! Remove command-line option. */
CFStringRef kSwitchRemove = CFSTR("-remove");

/*! List command-line option. */
CFStringRef kSwitchList = CFSTR("-list");

/*! Login command-line option. */
CFStringRef kSwitchLogin = CFSTR("-login");

/*! Logout command-line option. */
CFStringRef kSwitchLogout = CFSTR("-logout");

/*! Probe command; probes a target for authentication method. */
CFStringRef kSwitchProbe = CFSTR("-probe");

/*! Target command-line option. */
CFStringRef kOptTarget = CFSTR("target");

/*! Portal command-line option. */
CFStringRef kOptPortal = CFSTR("portal");

/*! Discovery command-line option. */
CFStringRef kOptDiscovery = CFSTR("discovery");

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


/*! Verbose command-line option. */
CFStringRef kOptVerbose = CFSTR("verbose");

/*! All command-line option. */
CFStringRef kOptAll = CFSTR("all");

/*! Nickname command-line option. */  ////// TODO
CFStringRef kOptNickname = CFSTR("nickname");



/*! Name of this command-line executable. */
const char * executableName;

/*! Command line arguments (used for getopt()). */
//const char * kShortOptions = "AMLRlp:t:i:df:vac:n:us:q:r:";

/*! The standard out stream, used by various functions to write. */
CFWriteStreamRef stdoutStream = NULL;

/*! Helper function.  Returns true if the targetName is a valid IQN/EUI name. */
Boolean validateTargetName(CFStringRef targetName)
{
    // IEEE regular expression for matching IQN name
    const char pattern[] =  "^iqn\.[0-9]\{4\}-[0-9]\{2\}\.[[:alnum:]]\{3\}\."
                            "[A-Za-z0-9.]\{1,255\}:[A-Za-z0-9.]\{1,255\}"
                            "|^eui\.[[:xdigit:]]\{16\}$";
    
    Boolean validName = false;
    regex_t preg;
    regcomp(&preg,pattern,REG_EXTENDED | REG_NOSUB);
    
    if(regexec(&preg,CFStringGetCStringPtr(targetName,kCFStringEncodingASCII),0,NULL,0) == 0)
        validName = true;

    regfree(&preg);
    return validName;
}

/*! Helper function.  Creates an array where the first element is the host
 *  IP address (either IPv4 or IPv6) or host name and the second part 
 *  contains the port to use, if present. */
CFArrayRef CreateArrayBySeparatingPortalParts(CFStringRef portal)
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

/*! Writes a string to stdout. */
void displayString(CFStringRef string)
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
void displayUsage()
{
    CFStringRef usage = CFSTR(
        "usage: iscisctl -A -t target -p portal [-f interface] [-u user -s secret]\n"
        "       iscsictl -A -d discovery-host [-u user -s secret]\n"
        "       iscsictl -A -a\n"
        "       iscisctl -M -i session-id [-p portal] [-t target]\n"
        "       iscsictl -L [-v]\n");
    
    displayString(usage);
    CFRelease(usage);
}

/*! Displays error for a missing option.
 *  @param option the missing option. */
void displayMissingOptionError(CFStringRef option)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,
        NULL,
        CFSTR("%s: required option -%s\n"),
        executableName,
        CFStringGetCStringPtr(option,kCFStringEncodingASCII));
    
    displayString(error);
    CFRelease(error);
}

/*! Displays a generic error message.
 *  @param errorString the error message to display. */
void displayError(const char * errorString)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %s\n"),executableName,errorString);
    
    displayString(error);
    CFRelease(error);
}

/*! Helper function used to display login status.
 *  @param statusCode the status code indicating the login result.
 *  @param target the target
 *  @param portal the portal used to logon to the target. */
void displayLoginStatus(enum iSCSILoginStatusCode statusCode,
                        iSCSITargetRef target,
                        iSCSIPortalRef portal)
{
    CFStringRef targetName      = iSCSITargetGetName(target);
    CFStringRef portalAddress   = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort      = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    CFStringRef loginStr;
    
    if(statusCode == kiSCSILoginSuccess)
    {
        loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                            CFSTR("Login to [target: %@ portal: %@:%@ if: %@] successful.\n"),
                                            targetName,portalAddress,
                                            portalPort,portalInterface);
        
        displayString(loginStr);
        CFRelease(loginStr);
        return;
    }
    
    CFStringRef statusStr = CFSTR("");
    switch(statusCode)
    {
        case kiSCSILoginSuccess:
            break;
        case kiSCSILoginAccessDenied:
            statusStr = CFSTR("the target has denied access");
            break;
        case kiSCSILoginAuthFail:
            statusStr = CFSTR("authentication failure");
            break;
        case kiSCSILoginCantIncludeInSeession:
            statusStr = CFSTR("can't include the portal in the session");
            break;
        case kiSCSILoginInitiatorError:
            statusStr = CFSTR("an initiator error has occurred");
            break;
        case kiSCSILoginInvalidReqDuringLogin:
            statusStr = CFSTR("the initiator made an invalid request");
            break;
        case kiSCSILoginMissingParam:
            statusStr = CFSTR("missing login parameters");
            break;
        case kiSCSILoginNotFound:
            statusStr = CFSTR("target was not found");
            break;
        case kiSCSILoginOutOfResources:
            statusStr = CFSTR("target is out of resources");
            break;
        case kiSCSILoginServiceUnavailable:
            statusStr = CFSTR("target services unavailable");
            break;
        case kiSCSILoginSessionDoesntExist:
            statusStr = CFSTR("session doesn't exist");
            break;
        case kiSCSILoginSessionTypeUnsupported:
            statusStr = CFSTR("target doesn't support login");
            break;
        case kiSCSILoginTargetHWorSWError:
            statusStr = CFSTR("target software or hardware error has occured");
            break;
        case kiSCSILoginTargetMovedPerm:
            statusStr = CFSTR("target has permanently moved");
            break;
        case kiSCSILoginTargetMovedTemp:
            statusStr = CFSTR("target has temporarily moved");
            break;
        case kiSCSILoginTargetRemoved:
            statusStr = CFSTR("target has been removed");
            break;
        case kiSCSILoginTooManyConnections:
            statusStr = CFSTR(".\n");
            break;
        case kiSCSILoginUnsupportedVer:
            statusStr = CFSTR("target is incompatible with the initiator");
            break;
        case kiSCSILoginInvalidStatusCode:
        default:
            statusStr = CFSTR("unknown error occurred");
            break;
            
    };
    
    loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                        CFSTR("Login to [target: %@ portal: %@:%@ if: %@] failed: %@.\n"),
                                        targetName,portalAddress,portalPort,
                                        portalInterface,statusStr);
    displayString(loginStr);
    CFRelease(loginStr);
}

void displayLogoutStatus(enum iSCSILogoutStatusCode statusCode,
                         iSCSITargetRef target,
                         iSCSIPortalRef portal)
{
    CFStringRef targetName      = iSCSITargetGetName(target);
    CFStringRef portalAddress   = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort      = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    CFStringRef loginStr;
    
    if(statusCode == kiSCSILogoutSuccess)
    {
        loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                            CFSTR("Logout of [target: %@ portal: %@:%@ if: %@] successful.\n"),
                                            targetName,portalAddress,portalPort,portalInterface);
        
        displayString(loginStr);
        CFRelease(loginStr);
        return;
    }

    CFStringRef statusStr = CFSTR("");
    switch(statusCode)
    {
        case kiSCSILogoutSuccess:
            break;
            
        case kiSCSILogoutCIDNotFound:
            statusStr = CFSTR("the connection was not found");
            break;
        case kiSCSILogoutCleanupFailed:
            statusStr = CFSTR("target cleanup of connection failed");
            break;
        case kiSCSILogoutRecoveryNotSupported:
            statusStr = CFSTR("could not recover the connection");
            break;
    };
    
    loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                        CFSTR("Logout of [target: %@ portal: %@:%@ if: %@] failed: %@.\n"),
                                        targetName,portalAddress,portalPort,
                                        portalInterface,statusStr);
    
    displayString(loginStr);
    CFRelease(loginStr);
}

CFHashCode hashOption(const void * value)
{
    // Hash is the value since options are unique chars by design
    return  *((int *)value);
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
    CFStringRef targetName;
    if(!CFDictionaryGetValueIfPresent(options,kOptTarget,(const void **)&targetName))
    {
        displayMissingOptionError(kOptTarget);
        return NULL;
    }
    
    if(!validateTargetName(targetName)) {
        displayError("the specified iSCSI target is invalid");
        return NULL;
    }
    
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,targetName);
    
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
        displayMissingOptionError(kOptPortal);
        return NULL;
    }
    
    // Returns an array of hostname, port strings (port optional)
    CFArrayRef portalParts = CreateArrayBySeparatingPortalParts(portalAddress);
    
    if(!portalParts) {
        displayError("the specified iSCSI portal is invalid");
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIMutablePortalCreate();
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    
    // If portal is present, set it
    if(CFArrayGetCount(portalParts) > 1)
        iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    
    CFRelease(portalParts);
//////// TODO: remove "en0" ----> if no interface specified leave blank - this should be handled by the daemon/kernel layer
    if(!CFDictionaryGetValueIfPresent(options,kOptInterface,(const void **)&hostInterface))
        iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
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
        displayMissingOptionError(kOptUser);
        return NULL;
    }
    else if(user && !secret) {
        displayMissingOptionError(kOptSecret);
        return NULL;
    }
    
    if(!mutualUser && mutualSecret) {
        displayMissingOptionError(kOptMutualUser);
        return NULL;
    }
    else if(mutualUser && !mutualSecret) {
        displayMissingOptionError(kOptMutualSecret);
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

errno_t iSCSICtlModifySessionConfigFromOptions(CFDictionaryRef options,iSCSISessionConfigRef * sessCfg)
{
    if(!sessCfg)
        return EINVAL;
    
    return 0;
}

errno_t iSCSICtlModifyConnectionConfigFromOptions(CFDictionaryRef options,iSCSIConnectionConfigRef * connCfg)
{
    if(!connCfg)
        return EINVAL;

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
    
    CFStringRef targetName = iSCSITargetGetName(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetName)) {
        displayError("the specified target does not exist.");
        
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
    if(portal && !iSCSIPLContainsPortal(targetName,iSCSIPortalGetAddress(portal))) {
        displayError("the specified portal does not exist.");
        
        iSCSIPortalRelease(portal);
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,targetName,&sessionId);
    
    if(sessionId != kiSCSIInvalidSessionId && portal)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    if(error)
        displayError("an I/O error occured while communicating with iscsid.");
    
    // There's an active session and connection for the target/portal
    if(sessionId != kiSCSIInvalidSessionId && connectionId != kiSCSIInvalidConnectionId) {
        displayError("the specified target has an active session over the specified portal.");
    }
    else if(!error)
    {
        // At this point we're doing some kind of login
        iSCSISessionConfigRef sessCfg = NULL;
        
        // If the session needs to be logged in...
        if(sessionId == kiSCSIInvalidSessionId)
        {
            // Grab session configuration from property list
            if(!(sessCfg = iSCSIPLCopySessionConfig(targetName)))
               sessCfg = iSCSIMutableSessionConfigCreate();
            
            // If user specified command-line session options, use those
            iSCSICtlModifySessionConfigFromOptions(options,&sessCfg);
        }
        
        // Check property list for If a valid portal was specified, then check the database for
        // configuration for the portal
        iSCSIConnectionConfigRef connCfg = NULL;
        iSCSIAuthRef auth = NULL;
        
        if(portal) {
            
            // Grab connection configuration from property list
            if(!(connCfg = iSCSIPLCopyConnectionConfig(targetName,iSCSIPortalGetAddress(portal))))
               connCfg = iSCSIMutableConnectionConfigCreate();
            
            // Override the options with command-line switches, if any
            iSCSICtlModifyConnectionConfigFromOptions(options,&connCfg);
            
            // If user didn't specify any authentication options, check the
            // database for options (if the user's input was incorrect, don't
            // revert to defaults or the property list; instead quit)
            if(!error && !auth)
                auth = iSCSIPLCopyAuthentication(targetName,iSCSIPortalGetAddress(portal));

            // If it doesn't exist in the database create a new one with defaults
            if(!error && !auth)
                auth = iSCSIAuthCreateNone();
            
            enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
            
            // Do either session login or add a connection...
            if(!error && sessionId == kiSCSIInvalidSessionId)
                error = iSCSIDaemonLoginSession(handle,portal,target,auth,sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
            else if(!error)
                error = iSCSIDaemonLoginConnection(handle,sessionId,portal,auth,connCfg,&connectionId,&statusCode);
            
            if(error)
                displayLoginStatus(statusCode,target,portal);
            
        }
        //else  // At this point the portal was not specified, and the session
        
    }
    
    if(portal)
        iSCSIPortalRelease(portal);
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
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,iSCSITargetGetName(target),&sessionId);
    
    if(!error && sessionId == kiSCSIInvalidSessionId)
    {
        displayError("the specified target has no active session.");
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
        displayError("the specified portal has no active connections.");
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
            displayLogoutStatus(statusCode,target,portal);
        else
            displayError("an I/O error occured while communicating with iscsid.");
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
    if(handle < 0 || !options)
        return EINVAL;
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    errno_t error = 0;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    if(!(portal = iSCSICtlCreatePortalFromOptions(options))) {
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // If portal and target both exist then do nothing, otherwise
    // add target and or portal with user-specified options
    CFStringRef targetName = iSCSITargetGetName(target);
    
    if(!iSCSIPLContainsPortal(targetName,iSCSIPortalGetAddress(portal)))
    {
        // Create an authentication object from user-specified switches
        iSCSIAuthRef auth;
        if(!(auth = iSCSICtlCreateAuthFromOptions(options)))
            error = EINVAL;
        
        // Setup optional session or connection configuration from switches
        iSCSISessionConfigRef sessCfg = iSCSIMutableSessionConfigCreate();
        iSCSIConnectionConfigRef connCfg = iSCSIMutableConnectionConfigCreate();

        if(!error)
            error = iSCSICtlModifySessionConfigFromOptions(options,&sessCfg);

        if(!error)
            error = iSCSICtlModifyConnectionConfigFromOptions(options,&connCfg);
        
        if(!error)
        {
            iSCSIPLSetPortal(targetName,portal);
            iSCSIPLSetAuthentication(targetName,iSCSIPortalGetAddress(portal),auth);
            iSCSIPLSetSessionConfig(targetName,sessCfg);
            iSCSIPLSetConnectionConfig(targetName,iSCSIPortalGetAddress(portal),connCfg);
            
            iSCSIPLSynchronize();
        }
        
        if(auth)
            iSCSIAuthRelease(auth);
    
        iSCSISessionConfigRelease(sessCfg);
        iSCSIConnectionConfigRelease(connCfg);
    }
    else
        displayError("the specified target and portal already exist.");
    
    // Target and portal are necessarily valid at this point
    iSCSITargetRelease(target);
    iSCSIPortalRelease(portal);
    
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
       
    CFStringRef targetName = iSCSITargetGetName(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetName)) {
        displayError("the specified target does not exist.");
        
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
    if(portal && !iSCSIPLContainsPortal(targetName,iSCSIPortalGetAddress(portal))) {
        displayError("the specified portal does not exist.");
        
        iSCSIPortalRelease(portal);
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Check for active sessions or connections before allowing removal
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,targetName,&sessionId);
    
    if(sessionId != kiSCSIInvalidSessionId && portal)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    if(error)
        displayError("I/O error occurred while communicating with the iscsid.");
    else if(!portal)
    {
        if(sessionId != kiSCSIInvalidSessionId)
            displayError("the specified target has an active session and cannot be removed.");
        else if(sessionId == kiSCSIInvalidSessionId) {
            iSCSIPLRemoveTarget(targetName);
            iSCSIPLSynchronize();
        }
    }
    else if(portal)
    {
        if(connectionId != kiSCSIInvalidConnectionId)
            displayError("the specified portal is connected and cannot be removed.");
        else if(connectionId == kiSCSIInvalidConnectionId) {
            iSCSIPLRemovePortal(targetName,iSCSIPortalGetAddress(portal));
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
    return 0;
}

/*! Helper function.  Used by iSCSICtlListTargets to display a single row of
 *  information about a target. */
void displayRow(CFStringRef target,CFStringRef portal,CFStringRef status)
{
    // Setup table header and width information
    CFStringRef strings[] = {target,portal,status};
    CFIndex columnWidth[] = {50,30,10};
    CFIndex length = 3;
    
    // Print the table header
    for(CFIndex idx = 0; idx < length; idx++)
    {
        CFStringRef string = CFStringCreateMutableCopy(kCFAllocatorDefault,0,strings[idx]);

        // Pad strings to align columns
        CFStringPad(string,CFSTR(" "),columnWidth[idx],0);
        
        // Append newline for last column
        if(idx == length - 1)
            CFStringAppend(string,CFSTR("\n"));
        
        displayString(string);
        CFRelease(string);
    }
}

errno_t iSCSICtlListTargets(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // We want to list all defined targets and information about any sessions
    // that may be associated with those targets, including information about
    // various portals and whether they are connected.  Targets from the
    // property list that cannot be matched to an active session are
    // assumed to be disconnected.
    
    // Get a list of all active session identifiers
    CFArrayRef sessionIds = iSCSIDaemonCreateArrayOfSessionIds(handle);
    
    if(!sessionIds || CFArrayGetCount(sessionIds) == 0)
        displayString(CFSTR("No active sessions were found."));
    
    displayRow(CFSTR("Target name"),CFSTR("Target portal"),CFSTR("State"));
    
    // Iterate over all session identifiers, and for each one, retrieve an
    // array of connection identifiers and information for each connection
    CFIndex sessionCount = CFArrayGetCount(sessionIds);
    
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    for(CFIndex sessionIdx = 0; sessionIdx < sessionCount; sessionIdx++)
    {
        // Get connection identifiers for this session
        sessionId = (SID)CFArrayGetValueAtIndex(sessionIds,sessionIdx);
        CFArrayRef connectionIds = iSCSIDaemonCreateArrayOfConnectionsIds(handle,sessionId);
        
        if(!connectionIds)
            continue;
        
        CFIndex connectionCount = CFArrayGetCount(connectionIds);
        
        if(!connectionCount)
        {
            CFRelease(connectionIds);
            continue;
        }
        
        target = iSCSIDaemonCreateTargetForSessionId(handle,sessionId);
        
        // Iterate over each connection and obtain information
        for(CFIndex connectionIdx = 0; connectionIdx < connectionCount; connectionIdx++)
        {
            connectionId = (CID)CFArrayGetValueAtIndex(connectionIds,connectionIdx);
            portal = iSCSIDaemonCreatePortalForConnectionId(handle,sessionId,connectionId);
            
            if(!portal)
                continue;

            // Print target name and connection state for the first connection
            if(connectionIdx == 0) {
                CFArrayRef disks = iSCSIDACreateBSDDiskNamesForSession(sessionId);
                CFStringRef status = CFStringCreateByCombiningStrings(kCFAllocatorDefault,disks,CFSTR(","));
                
                displayRow(iSCSITargetGetName(target),iSCSIPortalGetAddress(portal),status);
            }
            
            iSCSIPortalRelease(portal);
        }
        iSCSITargetRelease(target);
        CFRelease(connectionIds);
    }
    CFRelease(sessionIds);
    
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
    
    if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
        return EINVAL;
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    enum iSCSILoginStatusCode statusCode;
    enum iSCSIAuthMethods authMethod;
    
    error = iSCSIDaemonQueryTargetForAuthMethod(handle,portal,
                                                iSCSITargetGetName(target),
                                                &authMethod,
                                                &statusCode);
    if(!error)
    {
        if(statusCode != kiSCSILoginSuccess)
        {
            
        }
    }
    
    return error;
}

Boolean compareOptions(const void * value1,const void * value2 )
{
    // Compare chars
    return *((int *)value1) == *((int *)value2);
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
    
    // Connect to the daemon
    iSCSIDaemonHandle handle = iSCSIDaemonConnect();
    
    // Setup a stream for writing to stdout
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,CFSTR("/dev/stdout"),kCFURLPOSIXPathStyle,false);
    stdoutStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault,url);
    CFWriteStreamOpen(stdoutStream);
    
    // Save command line executable name for later use
    executableName = argv[0];
    
    // Get the mode of operation (e.g., add, modify, remove, etc).
    CFStringRef mode = NULL;
    if(CFArrayGetCount(arguments) > 0)
        mode = CFArrayGetValueAtIndex(arguments,1);
    
    // Process add, modify, remove or list
    if(CFStringCompare(mode,kSwitchAdd,0) == kCFCompareEqualTo)
        error = iSCSICtlAddTarget(handle,optDictionary);
    if(CFStringCompare(mode,kSwitchModify,0) == kCFCompareEqualTo)
        error = iSCSICtlModifyTarget(handle,optDictionary);
    if(CFStringCompare(mode,kSwitchRemove,0) == kCFCompareEqualTo)
        error = iSCSICtlRemoveTarget(handle,optDictionary);
    if(CFStringCompare(mode,kSwitchList,0) == kCFCompareEqualTo)
        error = iSCSICtlListTargets(handle,optDictionary);
    if(CFStringCompare(mode,kSwitchProbe,0) == kCFCompareEqualTo)
        error = iSCSICtlProbeTargetForAuthMethod(handle,optDictionary);
    
    // Then process login or logout in tandem
    if(CFStringCompare(mode,kSwitchLogin,0) == kCFCompareEqualTo)
        error = iSCSICtlLoginSession(handle,optDictionary);
    if(CFStringCompare(mode,kSwitchLogout,0) == kCFCompareEqualTo)
        error = iSCSICtlLogoutSession(handle,optDictionary);
    
    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
}
    
    return error;
}