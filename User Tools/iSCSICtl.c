/*!
 * @author		Nareg Sinenian
 * @file		main.h
 * @date		June 29, 2014
 * @version		1.0
 * @copyright	(c) 2014 Nareg Sinenian. All rights reserved.
 * @brief		User-space iSCSI management utility.
 */

#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <getopt.h>
#include "iSCSITypes.h"
#include <regex.h>
#include "iSCSIDaemonInterface.h"
#include "iSCSIDA.h"
#include "iSCSIPropertyList.h"

/*! Add command-line option. */
static const int kOptAdd = 'A';

/*! Modify command-line option. */
static const int kOptModify = 'M';

/*! Remove command-line option. */
static const int kOptRemove = 'R';

/*! List command-line option. */
static const int kOptList = 'L';

/*! Login command-line option. */
static const int kOptLogin = 'l';

/*! Logout command-line option. */
static const int kOptLogout = 'u';

/*! Target command-line option. */
static const int kOptTarget = 't';

/*! Portal command-line option. */
static const int kOptPortal = 'p';

/*! Discovery command-line option. */
static const int kOptDiscovery = 'd';

/*! Interface command-line option. */
static const int kOptInterface = 'f';

/*! Session identifier command-line option. */
static const int kOptSessionId = 'i';

/*! User (CHAP) command-line option. */
static const int kOptUser = 'c';

/*! Secret (CHAP) command-line option. */
static const int kOptSecret = 's';

/*! Mutual-user (CHAP) command-line option. */
static const int kOptMutualUser = 'q';

/*! Mutual-secret (CHAP) command-line option. */
static const int kOptMutualSecret = 'r';

/*! Verbose command-line option. */
static const int kOptVerbose = 'v';

/*! All command-line option. */
static const int kOptAll = 'a';

/*! Nickname command-line option. */  ////// TODO
static const int kOptNickname = 'n';



/*! Name of this command-line executable. */
const char * executableName;

/*! Command line arguments (used for getopt()). */
const char * kShortOptions = "AMLRlp:t:i:df:vac:n:us:q:r:";

/*! The standard out stream, used by various functions to write. */
CFWriteStreamRef stdoutStream = NULL;


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
/*! Helper function.  Returns true if the targetName is a valid IQN/EUI name. */
bool validateTargetName(CFStringRef targetName)
{
    // IEEE regular expression for matching IQN name
    const char pattern[] =  "^iqn\.[0-9]\{4\}-[0-9]\{2\}\.[[:alnum:]]\{3\}\."
                            "[A-Za-z0-9.]\{1,255\}:[A-Za-z0-9.]\{1,255\}"
                            "|^eui\.[[:xdigit:]]\{16\}$";
    
    bool validName = false;
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
    const int  maxMatches[] = {10,6,6};
    const int   hostIndex[] = {2,2,2};
    const int   portIndex[] = {9,5,5};
    
    int index = 0;
    
    do
    {
        regex_t preg;
        regcomp(&preg,patterns[index],REG_EXTENDED);
        
        regmatch_t matches[maxMatches[index]];
        memset(matches,0,sizeof(regmatch_t)*maxMatches[index]);
        
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
            CFRange rangeHost = CFRangeMake(matches[hostIndex[index]].rm_so, matches[hostIndex[index]].rm_eo - matches[hostIndex[index]].rm_so);
            CFStringRef host = CFStringCreateWithSubstring(kCFAllocatorDefault,portal,rangeHost);
            
            CFArrayAppendValue(portalParts,host);
            
            CFRelease(host);
        }
        
        // Is the port available?  If so, set it...
        if(matches[portIndex[index]].rm_so != -1)
        {
            CFRange rangePort = CFRangeMake(matches[portIndex[index]].rm_so, matches[portIndex[index]].rm_eo - matches[portIndex[index]].rm_so);
            CFStringRef port = CFStringCreateWithSubstring(kCFAllocatorDefault,portal,rangePort);
            
            CFArrayAppendValue(portalParts,port);
            
            CFRelease(port);
        }
        
        regfree(&preg);
        return portalParts;
        
    } while(index < sizeof(patterns)/sizeof(const char *));
    
    return NULL;
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

/*! Displays error for a missing option. */
void displayMissingOptionError(int option)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,
        NULL,
        CFSTR("%s: required option -- %c\n"),
        executableName,
        option);
    
    displayString(error);
    CFRelease(error);
}

/*! Displays a generic error message. */
void displayError(const char * errorString)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,NULL,CFSTR("%s: %s\n"),executableName,errorString);
    
    displayString(error);
    CFRelease(error);
}

void displayErrorCode(errno_t error)
{
    CFStringRef errorStr = NULL;
    
    switch(error)
    {
        case 0: return;
        case EPIPE:
        case EIO: errorStr = CFSTR("I/O error occured while communicating with iscsid.\n"); break;
        case EINVAL: errorStr = CFSTR("Invalid argument.\n"); break;
        case EAUTH: errorStr = CFSTR("Access denied.\n"); break;
        default: errorStr = CFSTR("Unknown error.\n"); break;
    };
    
    displayString(errorStr);
}

CFHashCode hashOption(const void * value)
{
    // Hash is the value since options are unique chars by design
    return  *((int *)value);
}

/*! Creates a target object from command-line options.  If the target is not
 *  well-formatted, the function displays an error message and returns an error
 *  code.  If the target is not specified, this function returns success and
 *  a NULL target object.
 *  @param options command-line options.
 *  @param target the target object, or NULL
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlCreateTargetFromOptions(CFDictionaryRef options,iSCSIMutableTargetRef * target)
{
    CFStringRef targetName;
    if(!CFDictionaryGetValueIfPresent(options,&kOptTarget,(const void **)&targetName))
    {
        *target = NULL;
        return 0;
    }
    
    if(!validateTargetName(targetName))
    {
        displayError("the specified iSCSI target is invalid");
        return EINVAL;
    }
    
    *target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(*target,CFDictionaryGetValue(options,&kOptTarget));
    
    return 0;
}

/*! Creates a portal object from command-line options.  If the portal is not
 *  well-formatted, the function displays an error message and returns an error
 *  code.  If the portal is not specified, this function returns success and
 *  a NULL portal object.
 *  @param options command-line options.
 *  @param portal the portal object, or NULL
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlCreatePortalFromOptions(CFDictionaryRef options,iSCSIMutablePortalRef * portal)
{
    CFStringRef portalAddress, hostInterface;
    
    if(!CFDictionaryGetValueIfPresent(options,&kOptPortal,(const void **)&portalAddress))
    {
        *portal = NULL;
        return 0;
    }
    
    *portal = iSCSIMutablePortalCreate();
    
    // Returns an array of hostname, port strings (port optional)
    CFArrayRef portalParts = CreateArrayBySeparatingPortalParts(portalAddress);
    
    if(!portalParts)
    {
        displayError("the specified iSCSI portal is invalid");
        return EINVAL;
    }
    
    iSCSIPortalSetAddress(*portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    
    // If portal is present, set it
    if(CFArrayGetCount(portalParts) > 1)
        iSCSIPortalSetPort(*portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    
    CFRelease(portalParts);
//////// TODO: remove "en0" ----> if no interface specified leave blank - this should be handled by the daemon/kernel layer
    if(!CFDictionaryGetValueIfPresent(options,&kOptInterface,(const void **)&hostInterface))
        iSCSIPortalSetHostInterface(*portal,CFSTR("en0"));
    else
        iSCSIPortalSetHostInterface(*portal,hostInterface);

    return 0;
}

errno_t iSCSICtlCreateAuthFromOptions(CFDictionaryRef options,iSCSIAuthRef * auth)
{
    // Setup authentication object from command-line parameters
    CFStringRef user = NULL, secret = NULL, mutualUser = NULL, mutualSecret = NULL;
    CFDictionaryGetValueIfPresent(options,&kOptUser,(const void **)&user);
    CFDictionaryGetValueIfPresent(options,&kOptSecret,(const void **)&secret);
    CFDictionaryGetValueIfPresent(options,&kOptMutualUser,(const void **)&mutualUser);
    CFDictionaryGetValueIfPresent(options,&kOptMutualSecret,(const void **)&mutualSecret);
    
    if(!user && secret)
    {
        displayMissingOptionError(kOptUser);
        return EINVAL;
    }
    else if(user && !secret)
    {
        displayMissingOptionError(kOptSecret);
        return EINVAL;
    }
    
    if(!mutualUser && mutualSecret)
    {
        displayMissingOptionError(kOptMutualUser);
        return EINVAL;
    }
    else if(mutualUser && !mutualSecret)
    {
        displayMissingOptionError(kOptMutualSecret);
        return EINVAL;
    }
    
    // At this point we've validated input combinations, if the user is not
    // present then we're dealing with no authentication.  Otherwise we have
    // either CHAP or mutual CHAP
    if(!user)
        *auth = iSCSIAuthCreateNone();
    else
        *auth = iSCSIAuthCreateCHAP(user,secret,mutualUser,mutualSecret);
    
    return 0;
}

errno_t iSCSICtlCreateSessionConfigFromOptions(CFDictionaryRef options,iSCSISessionConfigRef * sessCfg)
{
    *sessCfg = NULL;
    return 0;
}

errno_t iSCSICtlCreateConnectionConfigFromOptions(CFDictionaryRef options,iSCSIConnectionConfigRef * connCfg)
{
    *connCfg = NULL;
    return 0;
}

errno_t iSCSICtlLoginSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    if(handle < 0 || !options)
        return EINVAL;
    
    errno_t error = 0;
    iSCSIMutableTargetRef target = NULL;
    iSCSIMutablePortalRef portal = NULL;
    
    // Create target object from user input (may be null if user didn't specify)
    error = iSCSICtlCreateTargetFromOptions(options,&target);
    
    // Quit if there's an error, or no error but the target wasn't specified.
    if(!target)
    {
        if(!error)
            displayMissingOptionError(kOptTarget);
        
        return EINVAL;
    }
    
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    // See if there exists an active session for this target
    error = iSCSIDaemonGetSessionIdForTarget(handle,iSCSITargetGetName(target),&sessionId);
    
    if(!error)
        error = iSCSICtlCreatePortalFromOptions(options,&portal);
    
    // See if there exists an active connection for this portal
    if(!error && portal && sessionId != kiSCSIInvalidSessionId)
        error = iSCSIDaemonGetConnectionIdForPortal(handle,sessionId,portal,&connectionId);
    
    // There's an active session and connection for the target/portal
    if(!error && sessionId != kiSCSIInvalidSessionId && connectionId != kiSCSIInvalidConnectionId) {
        displayError("the specified target has an active session over the specified portal.");
    }
    else if(!error)
    {
        // At this point we're doing some kind of login (either session, adding
        // a connection, or both...)
        
        iSCSISessionConfigRef sessCfg = NULL;
        
        // If the session needs to be logged in...
        if(sessionId == kiSCSIInvalidSessionId)
        {
            // If user specified command-line session options, use that
            iSCSICtlCreateSessionConfigFromOptions(options,&sessCfg);
            
            // If user didn't specify then check the database
            if(!sessCfg)
                sessCfg = iSCSIPLCopySessionConfig(iSCSITargetGetName(target));
            
            // If it doesn't exist in the database create a new one with defaults
            if(!sessCfg)
                sessCfg = iSCSIMutableSessionConfigCreate();
        }
        
        // If a valid portal was specified, then check the database for
        // configuration for the portal
        iSCSIConnectionConfigRef connCfg = NULL;
        iSCSIAuthRef auth = NULL;
        
        if(portal) {
            
            // Get all parameters from command-line options if possible, then
            // resort to the database (if values are provided) and finally
            // use defaults if neither user-specified values nor database
            // entries are available for the portal
            iSCSICtlCreateConnectionConfigFromOptions(options,&connCfg);
            
            if(!connCfg)
                connCfg = iSCSIPLCopyConnectionConfig(iSCSITargetGetName(target),iSCSIPortalGetAddress(portal));
            
            if(!connCfg)
                connCfg = iSCSIMutableConnectionConfigCreate();
            
            iSCSICtlCreateAuthFromOptions(options,&auth);
            
            if(!auth)
                auth = iSCSIPLCopyAuthentication(iSCSITargetGetName(target),iSCSIPortalGetAddress(portal));
            
            if(!auth)
                auth = iSCSIAuthCreateNone();
            
            enum iSCSILoginStatusCode statusCode = kiSCSILoginInvalidStatusCode;
            
            // Do either session login or add a connection...
            if(sessionId == kiSCSIInvalidSessionId)
                error = iSCSIDaemonLoginSession(handle,portal,target,auth,sessCfg,connCfg,&sessionId,&connectionId,&statusCode);
            else
                error = iSCSIDaemonLoginConnection(handle,sessionId,portal,auth,connCfg,&connectionId,&statusCode);
            
            // TODO: Print status of login...
            
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
    
    errno_t error = 0;
    iSCSIMutableTargetRef target = NULL;
    iSCSIMutablePortalRef portal = NULL;
    
    SID sessionId = kiSCSIInvalidSessionId;
    CID connectionId = kiSCSIInvalidConnectionId;
    
    // Create target object from user input (may be null if user didn't specify)
    error = iSCSICtlCreateTargetFromOptions(options,&target);
    
    // Quit if there's an error, or no error but the target wasn't specified.
    if(!target)
    {
        if(!error)
            displayMissingOptionError(kOptTarget);
        
        return EINVAL;
    }
    
    // See if there exists an active session for this target
    error = iSCSIDaemonGetSessionIdForTarget(handle,iSCSITargetGetName(target),&sessionId);
    
    if(!error && sessionId == kiSCSIInvalidSessionId)
    {
        displayError("the specified target has no active session.");
        error = EINVAL;
    }
    
    if(!error)
        error = iSCSICtlCreatePortalFromOptions(options,&portal);
    
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
        {} // TODO: display logout status....
        else
            displayErrorCode(error);
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
    
    iSCSIMutableTargetRef target = NULL;
    iSCSIMutablePortalRef portal = NULL;
    errno_t error = 0;
    
    // Create target object from user input (may be null if user didn't specify)
    error = iSCSICtlCreateTargetFromOptions(options,&target);
    
    // Quit if there's an error, or no error but the target wasn't specified.
    if(!target)
    {
        if(!error)
            displayMissingOptionError(kOptTarget);
        
        return EINVAL;
    }

    error = iSCSICtlCreatePortalFromOptions(options,&portal);
    
    // Ensure that the user specified a valid portal (at least one portal
    // must be specified for a target)
    if(!portal)
    {
        if(!error)
            displayMissingOptionError(kOptPortal);
        
        return EINVAL;
    }
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // If portal and target both exist then do nothing, otherwise
    // add target and or portal with user-specified options
    if(!iSCSIPLContainsPortal(iSCSITargetGetName(target),iSCSIPortalGetAddress(portal)))
    {
        iSCSIAuthRef auth = NULL;
        iSCSISessionConfigRef sessCfg = NULL;
        iSCSIConnectionConfigRef connCfg = NULL;

        // The authenticaiton, session, and configuration objects are
        // guaranteed to be valid if no error was returned (if they
        // are not specified by the user, then defaults are used).
        error = iSCSICtlCreateAuthFromOptions(options,&auth);
        
        if(!error)
            error = iSCSICtlCreateSessionConfigFromOptions(options,&sessCfg);
        
        if(!error)
            error = iSCSICtlCreateConnectionConfigFromOptions(options,&connCfg);
        
        if(!error)
        {
            CFStringRef targetName = iSCSITargetGetName(target);
            iSCSIPLSetPortal(targetName,portal);
            iSCSIPLSetAuthentication(targetName,iSCSIPortalGetAddress(portal),auth);
            iSCSIPLSetSessionConfig(targetName,sessCfg);
            iSCSIPLSetConnectionConfig(targetName,iSCSIPortalGetAddress(portal),connCfg);
            
            iSCSIPLSynchronize();
        }
        
        if(auth)
            iSCSIAuthRelease(auth);
        
        if(sessCfg)
            iSCSISessionConfigRelease(sessCfg);
        
        if(connCfg)
            iSCSIConnectionConfigRelease(connCfg);
    }
    
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
    
    iSCSIMutableTargetRef target = NULL;
    iSCSIMutablePortalRef portal = NULL;
    errno_t error = 0;
    
    // Create target object from user input (may be null if user didn't specify)
    error = iSCSICtlCreateTargetFromOptions(options,&target);
    
    // Quit if there's an error, or no error but the target wasn't specified.
    if(!target)
    {
        if(!error)
            displayMissingOptionError(kOptTarget);
        
        return EINVAL;
    }
    
    error = iSCSICtlCreatePortalFromOptions(options,&portal);
    
    // Portal is optional for a remove operation, but check for error to see
    // if there were any problems parsing a portal if one was supplied.
    if(error)
    {
        iSCSITargetRelease(target);
        return EINVAL;
    }
    
    // Synchronize the database with the property list on disk
    iSCSIPLSynchronize();
    
    // Target must be valid at this point.  Remove the target entirely if no
    // portal was specified.
    CFStringRef targetName = iSCSITargetGetName(target);
    
    if(!portal)
    {
        if(!iSCSIPLContainsTarget(targetName))
            displayError("the specified target does not exist.");
        else
            iSCSIPLRemoveTarget(targetName);
    }
    else
    {
        if(!iSCSIPLContainsPortal(targetName,iSCSIPortalGetAddress(portal)))
            displayError("the specified portal does not exist.");
        else
            iSCSIPLRemovePortal(targetName,iSCSIPortalGetAddress(portal));
    }
    
    iSCSIPLSynchronize();
    
    return 0;
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

Boolean compareOptions(const void * value1,const void * value2 )
{
    // Compare chars
    return *((int *)value1) == *((int *)value2);
}

/*! Entry point.  Parses command line arguments, establishes a connection to the
 *  iSCSI deamon and executes requested iSCSI tasks. */
int main(int argc, char * argv[])
{
    // Connect to the daemon
    iSCSIDaemonHandle handle = iSCSIDaemonConnect();
    
    // Setup a stream for writing to stdout
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,CFSTR("/dev/stdout"),kCFURLPOSIXPathStyle,false);
    stdoutStream = CFWriteStreamCreateWithFile(kCFAllocatorDefault,url);
    CFWriteStreamOpen(stdoutStream);
    
    // Save command line executable name for later use
    executableName = argv[0];
    
    // Setup custom dictionary to process command-line options
    errno_t (* mode)(iSCSIDaemonHandle,CFDictionaryRef) = NULL;
    static int option = -1;
    CFDictionaryKeyCallBacks keyCallbacks;
    keyCallbacks.release = NULL;
    keyCallbacks.retain  = NULL;
    keyCallbacks.equal   = &compareOptions;
    keyCallbacks.hash    = &hashOption;
    CFMutableDictionaryRef options = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                               sizeof(option),&keyCallbacks,
                                                               &kCFTypeDictionaryValueCallBacks);
    CFStringRef optArgString = NULL;
    
    while((option = getopt(argc,argv,kShortOptions)) != -1)
    {
        if(optarg)
            optArgString = CFStringCreateWithCString(kCFAllocatorDefault,optarg,kCFStringEncodingASCII);
        
        switch(option) {
            case kOptAdd:    mode = &iSCSICtlAddTarget;      break;
            case kOptRemove: mode = &iSCSICtlRemoveTarget;   break;
            case kOptModify: mode = &iSCSICtlModifyTarget;   break;
            case kOptList:   mode = &iSCSICtlListTargets;    break;
            case kOptLogin:  mode = &iSCSICtlLoginSession;   break;
            case kOptLogout: mode = &iSCSICtlLogoutSession;  break;
            case kOptTarget:
                CFDictionarySetValue(options,&kOptTarget,optArgString);
                break;
            case kOptPortal:
                CFDictionarySetValue(options,&kOptPortal,optArgString);
                break;
            case kOptDiscovery:
                CFDictionarySetValue(options,&kOptDiscovery,optArgString);
                break;
            case kOptInterface:
                CFDictionarySetValue(options,&kOptInterface,optArgString);
                break;
            case kOptSessionId:
                CFDictionarySetValue(options,&kOptSessionId,optArgString);
                break;
            case kOptUser:
                CFDictionarySetValue(options,&kOptUser,optArgString);
                break;
            case kOptSecret:
                CFDictionarySetValue(options,&kOptSecret,optArgString);
                break;
            case kOptMutualUser:
                CFDictionarySetValue(options,&kOptMutualUser,optArgString);
                break;
            case kOptMutualSecret:
                CFDictionarySetValue(options,&kOptMutualSecret,optArgString);
                break;
            case kOptAll:
                CFDictionarySetValue(options,&kOptAll,optArgString);
                break;
            case '?':
                break;
        };
        
        // Release the string object for this option
        if(optArgString != NULL)
            CFRelease(optArgString);
    }
    
    if(mode)
        mode(handle,options);
    else
        displayUsage();
    
    iSCSIDaemonDisconnect(handle);
    CFWriteStreamClose(stdoutStream);
  
    return 0;
}