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
const char * kShortOptions = "AMLRDp:t:i:d:h:vacn:";




/*! Displays a list of valid command-line options. */
void displayUsage()
{
    CFStringRef usage = CFSTR(
        "usage: iscisctl -A -p portal -t target [-f interface] [-u user -s secret]\n\
       iscsictl -A -d discovery-host [-u user -s secret]\n\
       iscsictl -A -a\n\
       iscisctl -M -i session-id [-p portal] [-t target]\n\
       iscsictl -L [-v]");
    
    CFShow(usage);

}
CFHashCode hashOption(const void * value)
{
    // Hash is the value since options are unique chars by design
    return  *((int *)value);
}


errno_t iSCSICtlAddSession(CFDictionaryRef options)
{
//    CFStringRef target = CFDictionaryGetValue(options,&kOptTarget);
//    CFStringRef portal = CFDictionaryGetValue(options,&kOptPortal);

    bool addAll = (CFDictionaryGetCountOfKey(options,&kOptAll) == 1);

    iSCSIMutableTargetRef target = iSCSITargetCreate();
    iSCSITargetSetName(target,CFDictionaryGetValue(options,&kOptTarget));
    iSCSITargetSetDataDigest(target,false);
    iSCSITargetSetHeaderDigest(target,false);
    
    // Grab portal and parse into address & port
    CFStringRef portalAddress = CFDictionaryGetValue(options,&kOptPortal);
 //   CFString
    
    
    iSCSIMutablePortalRef portal = iSCSIPortalCreate();
    iSCSIPortalSetAddress(portal,CFDictionaryGetValue(options,&kOptPortal));
    
    
//    iSCSIPortalSetHostInterface(<#iSCSIMutablePortalRef portal#>, <#CFStringRef hostInterface#>)
    
    

    return 0;
}

errno_t iSCSICtlRemoveSession(CFDictionaryRef options)
{
    return 0;
}

errno_t iSCSICtlModifySession(CFDictionaryRef options)
{
    return 0;
}

errno_t iSCSICtlListSession(CFDictionaryRef options)
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
    errno_t (* mode)(CFDictionaryRef) = NULL;
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

            default:
                displayUsage();
                return 0;
        };
        
        if(optArgString != NULL)
            CFRelease(optArgString);
    }
    
    if(mode)
        return mode(options);
    
    displayUsage();
  
    return 0;
}




