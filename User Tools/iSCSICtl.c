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

/*! Database command-line option. */
static const int kOptDatabase = 'D';

/*! Target command-line option. */
static const int kOptTarget = 't';

/*! Portal command-line option. */
static const int kOptPortal = 'p';

/*! Discovery command-line option. */
static const int kOptDiscovery = 'd';

/*! Interface command-line option. */
static const int kOptInterface = 'h';

/*! Session identifier command-line option. */
static const int kOptSessionId = 'i';

/*! User (CHAP) command-line option. */
static const int kOptUser = 'u';

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

/*! Configuraiton file command-line option. */  ////// TODO
static const int kOptCfgFile = 'c';

/*! Name of this command-line executable. */
const char * executableName;

/*! Command line arguments (used for getopt()). */
const char * kShortOptions = "AMLRDp:t:i:d:h:vacn:u:s:q:r:";

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
    // IEEE regular expression for matching IPv4/IPv6 address (incl. optional port)
    const char pattern[] =  "(((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|([0-9])?"       // IPv4 pattern
                            "[0-9])\.)\{3\}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]"    // IPv4 pattern
                            "|([0-9])?[0-9])(:[0-9]\{1,5\})?)$|"                // IPv4 pattern
    
                            "^(\[)?(((([[:xdigit:]]{0,4}:){1,7}|:)"             // IPv6 pattern
                            "(([[:xdigit:]]){0,4})?))(]:[0-9]{1,5})?$|"         // IPv6 pattern
    
                            "([A-Za-z0-9.]\{1,253\})(:[0-9]\{1,5\})?$";         // DNS  pattern

    regex_t preg;
    regcomp(&preg,pattern,REG_EXTENDED | REG_NOSUB);
    
    int error = regexec(&preg,CFStringGetCStringPtr(portal,kCFStringEncodingASCII),0,NULL,0);
    regfree(&preg);
    
    if(error)
        return NULL;
    
    // Next check the valid IP/DNS to see if a port was specified
    const char patternPortal[] = "([A-Za-z0-9.:]\{1,253\}):([0-9]\{1,5\})?$";
    
    regcomp(&preg,patternPortal,REG_EXTENDED);

    // Two matches at most: one for entire string, one for hostname one for port
    const int maxMatches = 3;
    
    regmatch_t matches[maxMatches];
    
    // Array indices of hostname and port in matches array
    const int hostnameIdx = 1;
    const int portIdx = 2;

    error = regexec(&preg,CFStringGetCStringPtr(portal,kCFStringEncodingASCII),maxMatches,matches,0);
    
    // Determine number of matches to see if port was provided
    int numMatches;
    for(numMatches = 0; numMatches < maxMatches; numMatches++)
        if(matches[numMatches].rm_so == -1)
            break;

    // Extract host name and port if port was provided
    const int numPortalParts = 2;
    CFStringRef portalParts[numPortalParts];

    CFRange rangeHost = CFRangeMake(matches[hostnameIdx].rm_so, matches[hostnameIdx].rm_eo - matches[hostnameIdx].rm_so);
    portalParts[0] = CFStringCreateWithSubstring(kCFAllocatorDefault,portal,rangeHost);
    
    if(numMatches == maxMatches)
    {
        CFRange rangePort = CFRangeMake(matches[portIdx].rm_so, matches[portIdx].rm_eo - matches[portIdx].rm_so);
        portalParts[1] = CFStringCreateWithSubstring(kCFAllocatorDefault,portal,rangePort);
    }

    regfree(&preg);
    return CFArrayCreate(kCFAllocatorDefault,(void **)portalParts,numPortalParts,&kCFTypeArrayCallBacks);
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

CFHashCode hashOption(const void * value)
{
    // Hash is the value since options are unique chars by design
    return  *((int *)value);
}

iSCSITargetRef iSCSICtlCreateTargetFromOptions(CFDictionaryRef options)
{
    CFStringRef targetName;
    if(!CFDictionaryGetValueIfPresent(options,&kOptTarget,(const void **)&targetName))
    {
        displayMissingOptionError(kOptTarget);
        return NULL;
    }
    
    if(!validateTargetName(targetName))
    {
        displayString(CFSTR("The specified iSCSI target is invalid.\n"));
        return NULL;
    }
    
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,CFDictionaryGetValue(options,&kOptTarget));
    
    return target;
}

iSCSIPortalRef iSCSICtlCreatePortalFromOptions(CFDictionaryRef options)
{
    CFStringRef portalAddress, hostInterface;
    
    if(!CFDictionaryGetValueIfPresent(options,&kOptPortal,(const void **)&portalAddress))
    {
        displayMissingOptionError(kOptPortal);
        return NULL;
    }
    
    iSCSIMutablePortalRef portal = iSCSIMutablePortalCreate();
    CFArrayRef portalParts = CreateArrayBySeparatingPortalParts(portalAddress);
    
    if(!portalParts)
    {
        displayString(CFSTR("The specified iSCSI portal is invalid.\n"));
        return NULL;
    }
    
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    CFRelease(portalParts);
    
    if(!CFDictionaryGetValueIfPresent(options,&kOptInterface,(const void **)&hostInterface))
        iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    else
        iSCSIPortalSetHostInterface(portal,hostInterface);

    return portal;
}

iSCSIAuthRef iSCSICtlCreateAuthFromOptions(CFDictionaryRef options)
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
        return NULL;
    }
    else if(user && !secret)
    {
        displayMissingOptionError(kOptSecret);
        return NULL;
    }
    
    if(!mutualUser && mutualSecret)
    {
        displayMissingOptionError(kOptMutualUser);
        return NULL;
    }
    else if(mutualUser && !mutualSecret)
    {
        displayMissingOptionError(kOptMutualSecret);
        return NULL;
    }
    
    iSCSIAuthRef auth = NULL;
    if(!user)
        auth = iSCSIAuthCreateNone();
    else
        auth = iSCSIAuthCreateCHAP(user,secret,mutualUser,mutualSecret);
    
    return auth;
}

iSCSISessionConfigRef iSCSICtlCreateSessionConfigFromOptions(CFDictionaryRef options)
{
    return iSCSIMutableSessionConfigCreate();
}

iSCSIConnectionConfigRef iSCSICtlCreateConnectionConfigFromOptions(CFDictionaryRef options)
{
    return iSCSIMutableConnectionConfigCreate();
}

errno_t iSCSICtlLoginSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    return 0;
}

errno_t iSCSICtlLogoutSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // Create a target object so we can determine whether the target has an
    // associated session that may be active
    iSCSITargetRef target = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    // If a session identifier was found for this target, remove that session
    SID sessionId = kiSCSIInvalidSessionId;
    if(!iSCSIDaemonGetSessionIdForTarget(handle,iSCSITargetGetName(target),&sessionId))
    {
        enum iSCSILogoutStatusCode statusCode;
        
        if(iSCSIDaemonLogoutSession(handle,sessionId,&statusCode) || statusCode != kiSCSILogoutSuccess)
        {
            displayString(CFSTR("Failed to logout the session.\n"));
            
            iSCSITargetRelease(target);
            return EAGAIN;
        }
    }
    
    return 0;
}

/*! Adds a target to the property list.  First builds a target object using
 *  the command-line options passed in by the user.
 *  @param handle a handle to the iSCSI daemon.
 *  @param options the command-line options.
 *  @return an error code indicating the result of the operation. */
errno_t iSCSICtlAddTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // Create a target & portal object from user-supplied command-line input
    iSCSITargetRef target = NULL;
    iSCSIPortalRef portal = NULL;
    
    if(!(target = iSCSICtlCreateTargetFromOptions(options)))
        return EINVAL;
    
    
    if(!(portal = iSCSICtlCreatePortalFromOptions(options)))
    {
        iSCSITargetRelease(target);
        return EINVAL;
    }

    // These functions are guaranteed to return valid objects (if they aren't
    // specified as command line options, defaults are used - see the
    // documentation for these functions).
    iSCSIAuthRef auth = iSCSICtlCreateAuthFromOptions(options);
    iSCSISessionConfigRef sessCfg = iSCSICtlCreateSessionConfigFromOptions(options);
    iSCSIConnectionConfigRef connCfg = iSCSICtlCreateConnectionConfigFromOptions(options);
    
    // Add the target to the property list (if target, portal or any other
    // attributes already exist, they will be updated with the new values
    CFStringRef targetName = iSCSITargetGetName(target);
    iSCSIPLSetPortal(targetName,portal);
    iSCSIPLSetSessionConfig(targetName,sessCfg);
    iSCSIPLSetConnectionConfig(targetName,iSCSIPortalGetAddress(portal),connCfg);
    
    iSCSITargetRelease(target);
    iSCSIPortalRelease(portal);
    iSCSIAuthRelease(auth);
    iSCSISessionConfigRelease(sessCfg);
    iSCSIConnectionConfigRelease(connCfg);
    
    iSCSIPLSynchronize();
    
    return 0;
}

/*! Removes a target from the property list.  First checks to see if the target
 *  has any associated sessions that are active and logs out of those sessions.
 *  @param handle handle to the iSCSI daemon.
 *  @param options the command-line options dictionary. */
errno_t iSCSICtlRemoveTarget(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    // Get either session identifier if supplied, or the session identifier
    // from the target name if the latter was provided instead
    CFStringRef sessionIdString = CFDictionaryGetValue(options,&kOptSessionId);
    SID sessionId = kiSCSIInvalidSessionId;
    
    if(sessionIdString) {
        sessionId = CFStringGetIntValue(sessionIdString);
    }
    else
    {
        // Get identifier from target name
        CFStringRef targetName;
        if(!CFDictionaryGetValueIfPresent(options,&kOptTarget,(const void **)&targetName))
        {
            displayString(CFSTR("Specify a session identifier or a target to remove.\n"));
            return EINVAL;
        }
    }

    enum iSCSILogoutStatusCode statusCode = kiSCSILogoutInvalidStatusCode;
    iSCSIDaemonLogoutSession(handle,sessionId,&statusCode);
    
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
    CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,CFSTR("/dev/stdout"), kCFURLPOSIXPathStyle,false);
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
            case kOptList:   mode = &iSCSICtlListTargets;     break;
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