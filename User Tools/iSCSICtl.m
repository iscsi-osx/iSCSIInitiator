/*!
 * @author		Nareg Sinenian
 * @file		main.h
 * @version		1.0
 * @copyright	(c) 2014-2015 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI management utility.
 */

#include <CoreFoundation/CoreFoundation.h>
#include <Cocoa/Cocoa.h>

#include <regex.h>

#include "iSCSITypes.h"
#include "iSCSIPropertyList.h"
#include "iSCSIDaemonInterface.h"
#include "iSCSIDA.h"
#include "iSCSIIORegistry.h"

/*! Add command-line option. */
CFStringRef kSwitchAdd = CFSTR("-add");

/*! Discovery command-line option. */
CFStringRef kSwitchDiscovery = CFSTR("-discovery");

/*! Modify command-line option. */
CFStringRef kSwitchModify = CFSTR("-modify");

/*! Remove command-line option. */
CFStringRef kSwitchRemove = CFSTR("-remove");

/*! List command-line option. */
CFStringRef kSwitchListTargets = CFSTR("-listtargets");

/*! List command-line option. */
CFStringRef kSwitchListLUNs = CFSTR("-listluns");

/*! Login command-line option. */
CFStringRef kSwitchLogin = CFSTR("-login");

/*! Logout command-line option. */
CFStringRef kSwitchLogout = CFSTR("-logout");

/*! Mount command-line option. */
CFStringRef kSwitchMount = CFSTR("-mount");

/*! Probe command; probes a target for authentication method. */
CFStringRef kSwitchProbe = CFSTR("-probe");

/*! Rescans command; tells the kernel to rescan the SCSI bus for new LUNs. */
CFStringRef kSwitchRescan = CFSTR("-rescan");

/*! Unmount command-line option. */
CFStringRef kSwitchUnmount = CFSTR("-unmount");

/*! Sets the initiator name (IQN). */
CFStringRef kSwitchInitiatorIQN = CFSTR("-setinitiatorIQN");

/*! Sets the initiator alias. */
CFStringRef kSwitchInitiatorAlias = CFSTR("-setinitiatoralias");



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

/*! Helper function.  Returns true if the targetIQN is a valid IQN/EUI name. */
Boolean validateTargetIQN(CFStringRef targetIQN)
{
    // IEEE regular expression for matching IQN name
    const char pattern[] =  "^iqn\.[0-9]\{4\}-[0-9]\{2\}\.[[:alnum:]]\{3\}\."
                            "[A-Za-z0-9.]\{1,255\}:[A-Za-z0-9.]\{1,255\}"
                            "|^eui\.[[:xdigit:]]\{16\}$";
    
    Boolean validName = false;
    regex_t preg;
    regcomp(&preg,pattern,REG_EXTENDED | REG_NOSUB);
    
    if(regexec(&preg,CFStringGetCStringPtr(targetIQN,kCFStringEncodingASCII),0,NULL,0) == 0)
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
    CFStringRef targetIQN      = iSCSITargetGetIQN(target);
    CFStringRef portalAddress   = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort      = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    CFStringRef loginStr;
    
    if(statusCode == kiSCSILoginSuccess)
    {
        loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                            CFSTR("Login to [target: %@ portal: %@:%@ if: %@] successful.\n"),
                                            targetIQN,portalAddress,
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
                                        targetIQN,portalAddress,portalPort,
                                        portalInterface,statusStr);
    displayString(loginStr);
    CFRelease(loginStr);
}

void displayLogoutStatus(enum iSCSILogoutStatusCode statusCode,
                         iSCSITargetRef target,
                         iSCSIPortalRef portal)
{
    CFStringRef targetIQN      = iSCSITargetGetIQN(target);
    CFStringRef portalAddress   = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort      = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    CFStringRef loginStr;
    
    if(statusCode == kiSCSILogoutSuccess)
    {
        loginStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                            CFSTR("Logout of [target: %@ portal: %@:%@ if: %@] successful.\n"),
                                            targetIQN,portalAddress,portalPort,portalInterface);
        
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
                                        targetIQN,portalAddress,portalPort,
                                        portalInterface,statusStr);
    
    displayString(loginStr);
    CFRelease(loginStr);
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
        displayMissingOptionError(kOptTarget);
        return NULL;
    }
    
    if(!validateTargetIQN(targetIQN)) {
        displayError("the specified iSCSI target is invalid.");
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
        displayMissingOptionError(kOptPortal);
        return NULL;
    }
    
    // Returns an array of hostname, port strings (port optional)
    CFArrayRef portalParts = CreateArrayBySeparatingPortalParts(portalAddress);
    
    if(!portalParts) {
        displayError("the specified iSCSI portal is invalid.");
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreateMutable();
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    
    // If portal is present, set it
    if(CFArrayGetCount(portalParts) > 1)
        iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    else
        iSCSIPortalSetPort(portal,CFSTR("3260"));
    
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
    
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
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
    if(portal && !iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal))) {
        displayError("the specified portal does not exist.");
        
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
            if(!(sessCfg = iSCSIPLCopySessionConfig(targetIQN)))
               sessCfg = iSCSISessionConfigCreateMutable();
            
            // If user specified command-line session options, use those
            iSCSICtlModifySessionConfigFromOptions(options,&sessCfg);
        }
        
        // Check property list for If a valid portal was specified, then check the database for
        // configuration for the portal
        iSCSIConnectionConfigRef connCfg = NULL;
        iSCSIAuthRef auth = NULL;
        
        if(portal) {
            
            // Grab connection configuration from property list
            if(!(connCfg = iSCSIPLCopyConnectionConfig(targetIQN,iSCSIPortalGetAddress(portal))))
               connCfg = iSCSIConnectionConfigCreateMutable();
            
            // Override the options with command-line switches, if any
            iSCSICtlModifyConnectionConfigFromOptions(options,&connCfg);
            
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
    errno_t error = iSCSIDaemonGetSessionIdForTarget(handle,iSCSITargetGetIQN(target),&sessionId);
    
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
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    if(!iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal)))
    {
        // Create an authentication object from user-specified switches
        iSCSIAuthRef auth;
        if(!(auth = iSCSICtlCreateAuthFromOptions(options)))
            error = EINVAL;
        
        // Setup optional session or connection configuration from switches
        iSCSISessionConfigRef sessCfg = iSCSISessionConfigCreateMutable();
        iSCSIConnectionConfigRef connCfg = iSCSIConnectionConfigCreateMutable();

        if(!error)
            error = iSCSICtlModifySessionConfigFromOptions(options,&sessCfg);

        if(!error)
            error = iSCSICtlModifyConnectionConfigFromOptions(options,&connCfg);
        
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
       
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Verify that the target exists in the property list
    if(!iSCSIPLContainsTarget(targetIQN)) {
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
    if(portal && !iSCSIPLContainsPortal(targetIQN,iSCSIPortalGetAddress(portal))) {
        displayError("the specified portal does not exist.");
        
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
        displayError("I/O error occurred while communicating with the iscsid.");
    else if(!portal)
    {
        if(sessionId != kiSCSIInvalidSessionId)
            displayError("the specified target has an active session and cannot be removed.");
        else if(sessionId == kiSCSIInvalidSessionId) {
            iSCSIPLRemoveTarget(targetIQN);
            iSCSIPLSynchronize();
        }
    }
    else if(portal)
    {
        if(connectionId != kiSCSIInvalidConnectionId)
            displayError("the specified portal is connected and cannot be removed.");
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
    return 0;
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
    
    displayString(targetIQN);
    displayString(targetStatus);
    
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
        portalStatus = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                CFSTR("\t%@ [ CID: -, Port: %@, Auth: %@, Digest: %@ ]\n"),
                                                portalAddress,iSCSIPortalGetPort(portal),authStr,digestStr);
    }
    else {
        portalStatus = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                CFSTR("\t%@ [ CID: %d, Port: %@, Auth: %@, Digest: %@ ]\n"),
                                                portalAddress,connectionId,iSCSIPortalGetPort(portal),authStr,digestStr);
    }
    
    displayString(portalStatus);
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
        displayString(CFSTR("No persistent targets are defined.\n"));
        return 0;
    }
    
    displayString(CFSTR("\n"));
 
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
            
            CID connectionId = kiSCSIInvalidConnectionId;
            iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
            
            displayPortalInfo(handle,target,portal,sessionId,connectionId);
            
            iSCSIPortalRelease(portal);
        }
        
        displayString(CFSTR("\n"));
        
        iSCSITargetRelease(target);
        CFRelease(portalsList);
    }
    
   
    CFRelease(targetsList);
 
    return 0;
}

void displayLUNProperties(CFDictionaryRef lunDict)
{
    if(!lunDict)
        return;
    
    CFStringRef BSDName = CFDictionaryGetValue(lunDict,CFSTR(kIOBSDNameKey));
    CFNumberRef SCSILUNIdentifier = CFDictionaryGetValue(lunDict,CFSTR(kIOPropertySCSILogicalUnitNumberKey));
    CFNumberRef size = CFDictionaryGetValue(lunDict,CFSTR(kIOMediaSizeKey));
    
    // Obtain capacity as a double and format in gigabytes
    double capacity = 0;
    CFNumberGetValue(size,kCFNumberDoubleType,&capacity);
    capacity /= 1e9;
        
    
    CFStringRef LUNStr = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                  CFSTR("\tLUN: %@ [ BSD Name: %@, Capacity: %0.2f GB ]\n"),
                                                  SCSILUNIdentifier,BSDName,capacity);
    
    displayString(LUNStr);
    CFRelease(LUNStr);
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
    
    io_object_t lun = IO_OBJECT_NULL;
    io_iterator_t lunIterator = IO_OBJECT_NULL;
    
    iSCSIIORegistryGetTargets(&targetIterator);
    while((target = IOIteratorNext(targetIterator)) != IO_OBJECT_NULL)
    {
        CFDictionaryRef targetDict = iSCSIIORegistryCreateCFPropertiesForTarget(target);
        
        CFStringRef targetIQN = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertyiSCSIQualifiedNameKey));
        CFStringRef targetVendor = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSIVendorIdentification));
        CFStringRef targetProduct = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSIProductIdentification));
        CFNumberRef targetId = CFDictionaryGetValue(targetDict,CFSTR(kIOPropertySCSITargetIdentifierKey));
        
        CFStringRef targetStr = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                         CFSTR("\n %@ [ Id: %@, Vendor: %@, Model: %@ ]\n"),
                                                         targetIQN,targetId,targetVendor,targetProduct);
        displayString(targetStr);

        // Get a dictionary of properties for each LUN and display those
        iSCSIIORegistryGetLUNs(targetIQN,&lunIterator);
        while((lun = IOIteratorNext(lunIterator)) != IO_OBJECT_NULL)
        {
            CFDictionaryRef lunDict = iSCSIIORegistryCreateCFPropertiesForLUN(lun);
            
            displayLUNProperties(lunDict);
            
            IOObjectRelease(lun);
            CFRelease(lunDict);
        }
        
        CFRelease(targetStr);
        CFRelease(targetDict);
        
        IOObjectRelease(target);
        IOObjectRelease(lunIterator);
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
    
    if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
        return EINVAL;
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    enum iSCSILoginStatusCode statusCode;
    enum iSCSIAuthMethods authMethod;
    
    error = iSCSIDaemonQueryTargetForAuthMethod(handle,portal,
                                                iSCSITargetGetIQN(target),
                                                &authMethod,
                                                &statusCode);
    
    CFStringRef authStr = NULL;
    
    if(!error && statusCode == kiSCSILoginSuccess)
    {
        switch(authMethod)
        {
            case kiSCSIAuthMethodCHAP: authStr = CFSTR("CHAP required"); break;
            case kiSCSIAuthMethodNone: authStr = CFSTR("no authentication required"); break;
            case kiSCSIAuthMethodInvalid:
            default:
                authStr = CFSTR("target requires an invalid authentication method"); break;
        };
    }
    else
        authStr = CFSTR("error occured in communicating with target");
    
    CFStringRef targetIQN = iSCSITargetGetIQN(target);
    CFStringRef portalAddress = iSCSIPortalGetAddress(portal);
    CFStringRef portalPort = iSCSIPortalGetPort(portal);
    CFStringRef portalInterface = iSCSIPortalGetHostInterface(portal);
    
    authStr = CFStringCreateWithFormat(kCFAllocatorDefault,0,
                                       CFSTR("Probing [target: %@ portal: %@:%@ if: %@]: %@.\n"),
                                       targetIQN,portalAddress,portalPort,portalInterface,authStr);
    
    displayString(authStr);
    CFRelease(authStr);
    return error;
}

errno_t iSCSICtlRescanBus(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSITargetRef target = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    iSCSIDARescanBusForTarget(iSCSITargetGetIQN(target));
    
    return 0;
}

errno_t iSCSICtlMountForTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSITargetRef target = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    iSCSIDAMountIOMediaForTarget(iSCSITargetGetIQN(target));
    return 0;
}

errno_t iSCSICtlUnmountForTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    iSCSITargetRef target = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;

    iSCSIDAUnmountIOMediaForTarget(iSCSITargetGetIQN(target));
    return 0;
}

errno_t iSCSICtlSetInitiatorIQN(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    /*
    if(!CFDictionaryGetValueIfPresent(options,kOptPortal,(const void **)&portalAddress))
    {
        displayMissingOptionError(kOptPortal);
        return NULL;
    }
*/
    return 0;
}

errno_t iSCSICtlSetInitiatorAlias(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    /*
    if(!CFDictionaryGetValueIfPresent(options,(const void **)&portalAddress))
    {
        displayMissingOptionError(kOptPortal);
        return NULL;
    }
*/
    return 0;
}

Boolean compareOptions(const void * value1,const void * value2)
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
    
    // Setup a stream for writing to stdout
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,CFSTR("/dev/stdout"),kCFURLPOSIXPathStyle,false);
    stdoutStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault,url);
    CFWriteStreamOpen(stdoutStream);
    
    // Save command line executable name for later use
    executableName = argv[0];
    
    // Connect to the daemon
    iSCSIDaemonHandle handle = iSCSIDaemonConnect();
    
    if(handle < 0) {
        displayError("iscsid is not running.");
        return -1;
    }
    
    // Get the mode of operation (e.g., add, modify, remove, etc).
    CFStringRef mode = NULL;
    if(CFArrayGetCount(arguments) > 0)
        mode = CFArrayGetValueAtIndex(arguments,1);
    
    // Process add, modify, remove or list
    if(CFStringCompare(mode,kSwitchAdd,0) == kCFCompareEqualTo)
        error = iSCSICtlAddTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchModify,0) == kCFCompareEqualTo)
        error = iSCSICtlModifyTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchRemove,0) == kCFCompareEqualTo)
        error = iSCSICtlRemoveTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchListTargets,0) == kCFCompareEqualTo)
        error = iSCSICtlListTargets(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchListLUNs,0) == kCFCompareEqualTo)
        error = iSCSICtlListLUNs(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchProbe,0) == kCFCompareEqualTo)
        error = iSCSICtlProbeTargetForAuthMethod(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchRescan,0) == kCFCompareEqualTo)
        error = iSCSICtlRescanBus(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchMount,0) == kCFCompareEqualTo)
        error = iSCSICtlMountForTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchUnmount,0) == kCFCompareEqualTo)
        error = iSCSICtlUnmountForTarget(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchInitiatorIQN,0) == kCFCompareEqualTo)
        error = iSCSICtlSetInitiatorIQN(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchInitiatorAlias,0) == kCFCompareEqualTo)
        error = iSCSICtlSetInitiatorAlias(handle,optDictionary);
    
    // Then process login or logout in tandem
    if(CFStringCompare(mode,kSwitchLogin,0) == kCFCompareEqualTo)
        error = iSCSICtlLoginSession(handle,optDictionary);
    else if(CFStringCompare(mode,kSwitchLogout,0) == kCFCompareEqualTo)
        error = iSCSICtlLogoutSession(handle,optDictionary);
    
    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
}
    
    return error;
}