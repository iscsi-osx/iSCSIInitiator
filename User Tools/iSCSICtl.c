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

/*
static struct option kLongOptions[] = {
    {"add",no_argument,NULL,'A'},
    {"modify",no_argument,NULL,'M'},
    {"remove",no_argument,NULL,'R'},
    {"list",no_argument,NULL,'L'},
    {"portal",required_argument,NULL,'p'},
    {"target",required_argument,NULL,'t'},
    {"interface",required_argument,NULL,'f'},
    {"session-id",required_argument,NULL,'i'},
    {"user",required_argument,NULL,'u'},
    {"secret",required_argument,NULL,'s'},
    {"mutual-user",required_argument,NULL,'u'},
    {"mutual-secret",required_argument,NULL,'s'},
    {"database-only",no_argument,NULL,'b'},
};*/

/*! Command line arguments (used for getopt()). */
const char * kShortOptions = "AMLRDp:t:i:d:h:vacn:u:s:q:r:";

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
        "usage: iscisctl -A -p portal -t target [-f interface] [-u user -s secret]\n"
        "       iscsictl -A -d discovery-host [-u user -s secret]\n"
        "       iscsictl -A -a\n"
        "       iscisctl -M -i session-id [-p portal] [-t target]\n"
        "       iscsictl -L [-v]");
    
    CFShow(usage);
    CFRelease(usage);
}

/*! Displays error for a missing option. */
void displayMissingOptionError(int option)
{
    CFStringRef error = CFStringCreateWithFormat(
        kCFAllocatorDefault,
        NULL,
        CFSTR("%s: required option -- %c"),
        executableName,
        option);
    
    CFShow(error);
    CFRelease(error);
}

CFHashCode hashOption(const void * value)
{
    // Hash is the value since options are unique chars by design
    return  *((int *)value);
}

errno_t iSCSICtlAddSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    CFStringRef targetName, portalAddress, hostInterface;
    if(!CFDictionaryGetValueIfPresent(options,&kOptTarget,(const void **)&targetName))
    {
        displayMissingOptionError(kOptTarget);
        return EINVAL;
    }

    if(!validateTargetName(targetName))
    {
        CFShow(CFSTR("The specified iSCSI target is invalid."));
        return EINVAL;
    }
    
    if(!CFDictionaryGetValueIfPresent(options,&kOptPortal,(const void **)&portalAddress))
    {
        displayMissingOptionError(kOptPortal);
        return EINVAL;
    }
    
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,CFDictionaryGetValue(options,&kOptTarget));
    iSCSITargetSetDataDigest(target,false);
    iSCSITargetSetHeaderDigest(target,false);

    iSCSIMutablePortalRef portal = iSCSIMutablePortalCreate();
    CFArrayRef portalParts = CreateArrayBySeparatingPortalParts(portalAddress);
    
    if(!portalParts)
    {
        CFShow(CFSTR("The specified iSCSI portal is invalid."));
        return EINVAL;
    }
    
    iSCSIPortalSetAddress(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    iSCSIPortalSetPort(portal,(CFStringRef)CFArrayGetValueAtIndex(portalParts,1));

    CFShow((CFStringRef)CFArrayGetValueAtIndex(portalParts,0));
    CFShow((CFStringRef)CFArrayGetValueAtIndex(portalParts,1));
    
    CFRelease(portalParts);
    
    if(!CFDictionaryGetValueIfPresent(options,&kOptInterface,(const void **)&hostInterface))
        iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    else
        iSCSIPortalSetHostInterface(portal,hostInterface);
    
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
    
    iSCSIAuthRef auth = NULL;
    if(!user)
        auth = iSCSIAuthCreateNone();
    else
        auth = iSCSIAuthCreateCHAP(user,secret,mutualUser,mutualSecret);

    CID connectionId;
    SID sessionId;
    enum iSCSILoginStatusCode statusCode;
    
    iSCSIDaemonLoginSession(handle,portal,target,auth,&sessionId,&connectionId,&statusCode);
    return 0;
}

errno_t iSCSICtlRemoveSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
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
        
        
    }

    enum iSCSILogoutStatusCode statusCode = kiSCSILogoutInvalidStatusCode;
    iSCSIDaemonLogoutSession(handle,sessionId,&statusCode);
    
    return 0;
}

errno_t iSCSICtlModifySession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    return 0;
}

errno_t iSCSICtlListSession(iSCSIDaemonHandle handle,CFDictionaryRef options)
{
    
    
    
    return 0;
}


Boolean compareOptions( const void * value1, const void * value2 )
{
    // Compare chars
    return *((int *)value1) == *((int *)value2);
}

/*! Entry point.  Parses command line arguments, establishes a connection to the
 *  iSCSI deamon and executes requested iSCSI tasks. */
int main(int argc, char * argv[])
{
    iSCSIDaemonHandle handle = iSCSIDaemonConnect();
    

    /*
    iSCSIMutablePortalRef portal = iSCSIMutablePortalCreate();
    iSCSIPortalSetAddress(portal,CFSTR("192.168.1.115"));
    iSCSIPortalSetPort(portal,CFSTR("3260"));
    iSCSIPortalSetHostInterface(portal,CFSTR("en0"));
    
    iSCSIMutableTargetRef target = iSCSIMutableTargetCreate();
    iSCSITargetSetName(target,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55"));
    iSCSITargetSetDataDigest(target,false);
    iSCSITargetSetHeaderDigest(target,false);
    
    iSCSIAuthRef auth = iSCSIAuthCreateNone();
    
    SID sessionId;
    CID connectionId;
    enum iSCSILoginStatusCode statusCode;
    
    iSCSIDaemonLoginSession(handle,portal,target,auth,&sessionId,&connectionId,&statusCode);
*/
/*    CID connIds[kiSCSIMaxConnectionsPerSession];
    UInt32 connCount;
    connIds[0] = 10;
    iSCSIDaemonGetConnectionIds(handle,0,connIds,&connCount);
    int b =10;
 
    SID b = 10;
    b = iSCSIDaemonGetSessionIdForTarget(handle,CFSTR("iqn.1995-05.com.lacie:nas-vault:iscsi55"), &b);
    int c = 10;
 */
    
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
            case kOptAdd:    mode = &iSCSICtlAddSession;      break;
            case kOptRemove: mode = &iSCSICtlRemoveSession;   break;
            case kOptModify: mode = &iSCSICtlModifySession;   break;
            case kOptList:   mode = &iSCSICtlListSession;     break;
            case kOptTarget:
                CFDictionaryAddValue(options,&kOptTarget,optArgString);
                break;
            case kOptPortal:
                CFDictionaryAddValue(options,&kOptPortal,optArgString);
                break;
            case kOptDiscovery:
                CFDictionaryAddValue(options,&kOptDiscovery,optArgString);
                break;
            case kOptInterface:
                CFDictionaryAddValue(options,&kOptInterface,optArgString);
                break;
            case kOptSessionId:
                CFDictionaryAddValue(options,&kOptSessionId,optArgString);
                break;
            case kOptUser:
                CFDictionaryAddValue(options,&kOptUser,optArgString);
                break;
            case kOptSecret:
                CFDictionaryAddValue(options,&kOptSecret,optArgString);
                break;
            case kOptMutualUser:
                CFDictionaryAddValue(options,&kOptMutualUser,optArgString);
                break;
            case kOptMutualSecret:
                CFDictionaryAddValue(options,&kOptMutualSecret,optArgString);
                break;
            case kOptAll:
                CFDictionaryAddValue(options,&kOptAll,optArgString);
                break;
            case '?':
                break;

            default:
                displayUsage();
                return 0;
        };
        
        if(optArgString != NULL)
            CFRelease(optArgString);
    }
    
    if(mode)
        return mode(handle,options);
    
    displayUsage();
  
    return 0;
}