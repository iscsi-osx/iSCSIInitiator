/*!
 * @author		Nareg Sinenian
 * @file		iSCSITypes.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI data types used in user space.  All of the data types
 *              defined here are based on Core Foundation (CF) data types.
 */

#include "iSCSITypes.h"

/*! iSCSI portal records are dictionaries with three keys with string values
 *  that specify the address (DNS name or IP address), the port, and the
 *  host interface to use when connecting to the portal. */
CFStringRef kiSCSIPortalAddresssKey = CFSTR("Address");
CFStringRef kiSCSIPortalPortKey = CFSTR("Port");
CFStringRef kiSCSIPortalHostInterfaceKey = CFSTR("HostInterface");

/*! Maximum number of discovery record entries (targets). */
const int kMaxDiscoveryRecEntries = 50;

/*! Maximum number of portal groups per target. */
const int kMaxPortalGroupsPerTarget = 10;

/*! Maximum number of portals per portal group. */
const int kMaxPortalsPerGroup = 10;

/*! Creates a new portal object from byte representation. */
iSCSIPortalRef iSCSIPortalCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    iSCSIPortalRef portal = CFPropertyListCreateWithData(kCFAllocatorDefault,data,kCFPropertyListImmutable,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return portal;

    return NULL;
}

/*! Convenience function.  Creates a new iSCSIPortalRef with the above keys. */
iSCSIMutablePortalRef iSCSIMutablePortalCreate()
{
    iSCSIMutablePortalRef portal = CFDictionaryCreateMutable(kCFAllocatorDefault,3,NULL,NULL);
    CFDictionaryAddValue(portal,kiSCSIPortalAddresssKey,CFSTR(""));
    CFDictionaryAddValue(portal,kiSCSIPortalPortKey,CFSTR(""));
    CFDictionaryAddValue(portal,kiSCSIPortalHostInterfaceKey,CFSTR(""));
    
    return portal;
}

CFStringRef iSCSIPortalGetAddress(iSCSIPortalRef portal)
{
    return CFDictionaryGetValue(portal,kiSCSIPortalAddresssKey);
}

void iSCSIPortalSetAddress(iSCSIMutablePortalRef portal,CFStringRef address)
{
    // Ignore blanks
    if(CFStringCompare(address,CFSTR(""),0) == kCFCompareEqualTo)
        return;

    CFDictionarySetValue(portal,kiSCSIPortalAddresssKey,address);
}

CFStringRef iSCSIPortalGetPort(iSCSIPortalRef portal)
{
    return CFDictionaryGetValue(portal,kiSCSIPortalPortKey);
}

void iSCSIPortalSetPort(iSCSIMutablePortalRef portal,CFStringRef port)
{
    // Ignore blanks
    if(CFStringCompare(port,CFSTR(""),0) == kCFCompareEqualTo)
        return;

    CFDictionarySetValue(portal,kiSCSIPortalPortKey,port);
}

CFStringRef iSCSIPortalGetHostInterface(iSCSIPortalRef portal)
{
    return CFDictionaryGetValue(portal,kiSCSIPortalHostInterfaceKey);
}

void iSCSIPortalSetHostInterface(iSCSIMutablePortalRef portal,CFStringRef hostInterface)
{
    CFDictionarySetValue(portal,kiSCSIPortalHostInterfaceKey,hostInterface);
}

/*! Releases memory associated with iSCSI portals. */
void iSCSIPortalRelease(iSCSIPortalRef portal)
{
    CFRelease(portal);
}

/*! Retains an iSCSI portal object. */
void iSCSIPortalRetain(iSCSIPortalRef portal)
{
    CFRetain(portal);
}

/*! Creates a new portal object from a dictionary representation. */
iSCSIPortalRef iSCSIPortalCreateWithDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies a target object to a dictionary representation. */
CFDictionaryRef iSCSIPortalCreateDictionary(iSCSIPortalRef portal)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,portal);
}

/*! Copies the portal object to a byte array representation. */
CFDataRef iSCSIPortalCreateData(iSCSIPortalRef portal)
{
    CFPropertyListRef plist = portal;
    return CFPropertyListCreateData(kCFAllocatorDefault,plist,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}


/*! Creates a new target object from byte representation. */
iSCSISessionConfigRef iSCSITargetCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    
    iSCSITargetRef target = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);

    if(format == kCFPropertyListBinaryFormat_v1_0)
        return target;

    return NULL;
}

/*! iSCSI target records are dictionaries with keys with string values
 *  that specify the target name and other parameters. */
CFStringRef kiSCSITargetNameKey = CFSTR("TargetName");

/*! Convenience function.  Creates a new iSCSITargetRef with the above keys. */
iSCSIMutableTargetRef iSCSIMutableTargetCreate()
{
    iSCSIMutableTargetRef target = CFDictionaryCreateMutable(kCFAllocatorDefault,5,NULL,NULL);
    return target;
}

/*! Gets the name associated with the iSCSI target. */
CFStringRef iSCSITargetGetName(iSCSITargetRef target)
{
    return CFDictionaryGetValue(target,kiSCSITargetNameKey);
}

/*! Sets the name associated with the iSCSI target. */
void iSCSITargetSetName(iSCSIMutableTargetRef target,CFStringRef name)
{
    // Ignore blanks
    if(CFStringCompare(name,CFSTR(""),0) == kCFCompareEqualTo)
        return;
    
    CFDictionarySetValue(target,kiSCSITargetNameKey,name);
}

/*! Releases memory associated with iSCSI targets. */
void iSCSITargetRelease(iSCSITargetRef target)
{
    CFRelease(target);
}

/*! Retains an iSCSI target object. */
void iSCSITargetRetain(iSCSITargetRef target)
{
    CFRetain(target);
}

/*! Creates a new target object from a dictionary representation. */
iSCSITargetRef iSCSITargetCreateWithDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies a target object to a dictionary representation. */
CFDictionaryRef iSCSITargetCreateDictionary(iSCSITargetRef target)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,target);
}

/*! Copies the target object to a byte array representation. */
CFDataRef iSCSITargetCreateData(iSCSITargetRef target)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,target,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}




/*! Creates a new authentication object from byte representation. */
iSCSIAuthRef iSCSIAuthCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    
    iSCSIAuthRef auth = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);

    if(format == kCFPropertyListBinaryFormat_v1_0)
        return auth;
    
    return NULL;
}

/*! Creates a new iSCSIAuth object with empty authentication parameters
 *  (defaults to no authentication). */
iSCSIAuthRef iSCSIAuthCreateNone()
{
    UInt8 authMethod = kiSCSIAuthMethodNone;
    CFNumberRef authNum = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&authMethod);
    
    const void * keys[] = {CFSTR("AuthMethod")};
    const void * values[] = {authNum};

    return CFDictionaryCreate(kCFAllocatorDefault,keys,values,1,NULL,NULL);
}

/*! Creates a new iSCSIAuth object with empty authentication parameters
 *  (defaults to no authentication). */
iSCSIAuthRef iSCSIAuthCreateCHAP(CFStringRef initiatorUser,
                                 CFStringRef initiatorSecret,
                                 CFStringRef targetUser,
                                 CFStringRef targetSecret)
{
    // Required parameters
    if(!initiatorUser || !initiatorSecret)
        return NULL;
    
    UInt8 authMethod = kiSCSIAuthMethodCHAP;
    CFNumberRef authNum = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&authMethod);
    
    const void * keys[] = {
        CFSTR("AuthMethod"),
        CFSTR("InitiatorUser"),
        CFSTR("InitiatorSecret"),
        CFSTR("TargetUser"),
        CFSTR("TargetSecret")
    };
    
    const void * values[] = {
        authNum,
        initiatorUser,
        initiatorSecret,
        targetUser,
        targetSecret
    };

    // Check for mutual CHAP before including the targetUser and targetSecret
    // (include the first there terms of the arrays above when mutual CHAP
    // is not used).
    if(!targetUser || !targetSecret)
        return CFDictionaryCreate(kCFAllocatorDefault,keys,values,3,NULL,NULL);
    
    // Else we include all parameters
    return CFDictionaryCreate(kCFAllocatorDefault,keys,values,5,NULL,NULL);
}

/*! Returns the CHAP authentication parameter values if the authentication
 *  method is actually CHAP. */
void iSCSIAuthGetCHAPValues(iSCSIAuthRef auth,
                            CFStringRef * initiatorUser,
                            CFStringRef * initiatorSecret,
                            CFStringRef * targetUser,
                            CFStringRef * targetSecret)
{
    if(iSCSIAuthGetMethod(auth) != kiSCSIAuthMethodCHAP)
        return;
    
    if(!auth || !initiatorUser || !initiatorSecret || !targetUser || !targetSecret)
        return;
    
    *initiatorUser = CFDictionaryGetValue(auth,CFSTR("InitiatorUser"));
    *initiatorUser = CFDictionaryGetValue(auth,CFSTR("InitiatorSecret"));
    *targetUser = CFDictionaryGetValue(auth,CFSTR("TargetUser"));
    *targetSecret = CFDictionaryGetValue(auth,CFSTR("TargetSecret"));
}

/*! Gets the authentication method used. */
enum iSCSIAuthMethods iSCSIAuthGetMethod(iSCSIAuthRef auth)
{
    UInt8 authMethod = 0;
    CFNumberRef authNum = CFDictionaryGetValue(auth,CFSTR("AuthMethod"));
    CFNumberGetValue(authNum,kCFNumberIntType,&authMethod);
    return (enum iSCSIAuthMethods)authMethod;
}

/*! Releases memory associated with an iSCSI auth object. */
void iSCSIAuthRelease(iSCSIAuthRef auth)
{
    CFRelease(auth);
}

/*! Retains an iSCSI auth object. */
void iSCSIAuthRetain(iSCSIAuthRef auth)
{
    CFRetain(auth);
}

/*! Creates a new authentication object from a dictionary representation. */
iSCSIAuthRef iSCSIAuthCreateWithDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies an authentication object to a dictionary representation. */
CFDictionaryRef iSCSIAuthCreateDictionary(iSCSIAuthRef auth)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,auth);
}

/*! Copies the authentication object to a byte array representation. */
CFDataRef iSCSIAuthCreateData(iSCSIAuthRef auth)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,auth,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}






/*! Creates a discovery record object. */
iSCSIMutableDiscoveryRecRef iSCSIMutableDiscoveryRecCreate()
{
    return CFDictionaryCreateMutable(kCFAllocatorDefault,
                                     kMaxDiscoveryRecEntries,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
}

/*! Creates a discovery record from an external data representation.
 * @param data data used to construct an iSCSI discovery object.
 * @return an iSCSI discovery object or NULL if object creation failed */
iSCSIMutableDiscoveryRecRef iSCSIMutableDiscoveryRecCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    iSCSIMutableDiscoveryRecRef discoveryRec = (iSCSIMutableDiscoveryRecRef)
        CFPropertyListCreateWithData(kCFAllocatorDefault,data,kCFPropertyListImmutable,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return discoveryRec;
    
    return NULL;
}


/*! Add a portal to a specified portal group tag for a given target.
 *  @param discoveryRec the discovery record.
 *  @param targetName the name of the target to add.
 *  @param portalGroupTag the target portal group tag to add.
 *  @param portal the iSCSI portal to add. */
void iSCSIDiscoveryRecAddPortal(iSCSIMutableDiscoveryRecRef discoveryRec,
                                CFStringRef targetName,
                                CFStringRef portalGroupTag,
                                iSCSIPortalRef portal)
{
    // Validate inputs
    if(!discoveryRec || !targetName || !portalGroupTag)
        return;
    
    CFMutableDictionaryRef targetDict;
    
    // If target doesn't exist add it
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetName,(void *)&targetDict))
    {
        targetDict = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                               kMaxPortalGroupsPerTarget,
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);
        
        CFDictionaryAddValue(discoveryRec,targetName,targetDict);
    }
    
    // If the group tag doesn't exist add it
    CFMutableArrayRef portalsArray;
    
    if(!CFDictionaryGetValueIfPresent(targetDict,portalGroupTag,(void *)&portalsArray))
    {
        portalsArray = CFArrayCreateMutable(kCFAllocatorDefault,
                                            kMaxPortalsPerGroup,
                                            &kCFTypeArrayCallBacks);
        
        CFDictionaryAddValue(targetDict,portalGroupTag,portalsArray);
    }
    
    // If we've exceeded the max number of portals do nothing
    if(CFArrayGetCount(portalsArray) == kMaxPortalsPerGroup)
        return;
    
    // Add a new portal
    CFArrayAppendValue(portalsArray,portal);
}

/*! Creates a CFArray object containing CFString objects with names of
 *  all of the targets in the discovery record.
 *  @param discoveryRec the discovery record.
 *  @return an array of strings with names of the targets in the record. */
CFArrayRef iSCSIDiscoveryRecCreateArrayOfTargets(iSCSIMutableDiscoveryRecRef discoveryRec)
{
    // Validate input
    if(!discoveryRec)
        return NULL;
    
    // Get all keys, which correspond to the targets
    const void * keys;
    CFDictionaryGetKeysAndValues(discoveryRec,&keys,NULL);
    
    CFArrayRef targets = CFArrayCreate(kCFAllocatorDefault,
                                       &keys,
                                       CFDictionaryGetCount(discoveryRec),
                                       &kCFTypeArrayCallBacks);
    
    return targets;
}

/*! Creates a CFArray object containing CFString objects with portal group
 *  tags for a particular target.
 *  @param discoveryRec the discovery record.
 *  @param targetName the name of the target.
 *  @return an array of strings with portal group tags for the specified target. */
CFArrayRef iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(iSCSIMutableDiscoveryRecRef discoveryRec,
                                                         CFStringRef targetName)
{
    // Validate inputs
    if(!discoveryRec || !targetName)
        return NULL;
    
    // If target doesn't exist return NULL
    CFMutableDictionaryRef targetDict;
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetName,(void *)&targetDict))
        return NULL;
    
    // Otherwise get all keys, which correspond to the portal group tags
    const void * keys;
    CFDictionaryGetKeysAndValues(targetDict,&keys,NULL);
    
    CFArrayRef portalGroups = CFArrayCreate(kCFAllocatorDefault,
                                            &keys,
                                            CFDictionaryGetCount(targetDict),
                                            &kCFTypeArrayCallBacks);
    
    return portalGroups;
}

/*! Gets all of the portals associated with a partiular target and portal
 *  group tag.
 *  @param discoveryRec the discovery record.
 *  @param targetName the name of the target.
 *  @param portalGroupTag the portal group tag associated with the target.
 *  @return an array of iSCSIPortal objects associated with the specified
 *  group tag for the specified target. */
CFArrayRef iSCSIDiscoveryRecGetPortals(iSCSIMutableDiscoveryRecRef discoveryRec,
                                       CFStringRef targetName,
                                       CFStringRef portalGroupTag)
{
    // Validate inputs
    if(!discoveryRec || !targetName || !portalGroupTag)
        return NULL;
    
    // If target doesn't exist return NULL
    CFMutableDictionaryRef targetDict;
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetName,(void *)&targetDict))
        return NULL;

    // Grab requested portal group
    CFArrayRef portalGroup = NULL;
    CFDictionaryGetValueIfPresent(targetDict,portalGroupTag,(void *)&portalGroup);
    
    return portalGroup;
}

/*! Releases memory associated with an iSCSI discovery record object.
 * @param target the iSCSI discovery record object. */
void iSCSIDiscoveryRecRelease(iSCSIMutableDiscoveryRecRef discoveryRec)
{
    CFRelease(discoveryRec);
}

/*! Retains an iSCSI discovery record object.
 * @param target the iSCSI discovery record object. */
void iSCSIDiscoveryRecRetain(iSCSIMutableDiscoveryRecRef discoveryRec)
{
    CFRetain(discoveryRec);
}

/*! Copies an iSCSI discovery record object to a dictionary representation.
 *  @param auth an iSCSI discovery record object.
 *  @return a dictionary representation of the discovery record object or
 *  NULL if the discovery record object is invalid. */
CFDictionaryRef iSCSIDiscoveryRecCreateDictionary(iSCSIMutableDiscoveryRecRef discoveryRec)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,discoveryRec);
}

/*! Copies the discovery record object to a byte array representation.
 *  @param auth an iSCSI discovery record object.
 *  @return data representing the discovery record object
 *  or NULL if the discovery record object is invalid. */
CFDataRef iSCSIDiscoveryRecCreateData(iSCSIMutableDiscoveryRecRef discoveryRec)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,discoveryRec,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}





CFStringRef kiSCSISessionConfigErrorRecoveryKey = CFSTR("ErrorRecoveryLevel");
CFStringRef kiSCSISessionConfigPortalGroupTagKey = CFSTR("TargetPortalGroupTag");
CFStringRef kiSCSISessionConfigMaxConnectionsKey = CFSTR("MaxConnections");

/*! Convenience function.  Creates a new iSCSISessionConfigRef with the above keys. */
iSCSIMutableSessionConfigRef iSCSIMutableSessionConfigCreate()
{
    iSCSIMutableSessionConfigRef config = CFDictionaryCreateMutable(kCFAllocatorDefault,5,NULL,NULL);
    iSCSISessionConfigSetErrorRecoveryLevel(config,kiSCSIErrorRecoverySession);
    iSCSISessionConfigSetMaxConnections(config,0);
    iSCSISessionConfigSetTargetPortalGroupTag(config,0);
    return config;
}

/*! Gets the error recovery level associated with a target (session). */
enum iSCSIErrorRecoveryLevels iSCSISessionConfigGetErrorRecoveryLevel(iSCSISessionConfigRef target)
{
    UInt8 errorRecoveryLevel = 0;
    CFNumberRef errorRecLevelNum = CFDictionaryGetValue(target,kiSCSISessionConfigErrorRecoveryKey);
    CFNumberGetValue(errorRecLevelNum,kCFNumberIntType,&errorRecoveryLevel);
    return (enum iSCSIErrorRecoveryLevels)errorRecoveryLevel;
}

/*! Sets the desired recovery level associated with a target (session). */
void iSCSISessionConfigSetErrorRecoveryLevel(iSCSIMutableSessionConfigRef target,
                                             enum iSCSIErrorRecoveryLevels errorRecoveryLevel)
{
    CFNumberRef errorRecoveryLevelNum = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&errorRecoveryLevel);
    CFDictionarySetValue(target,kiSCSISessionConfigErrorRecoveryKey,&errorRecoveryLevelNum);
}

/*! Gets the target portal group tag. */
TPGT iSCSISessionConfigGetTargetPortalGroupTag(iSCSISessionConfigRef target)
{
    TPGT targetPortalGroupTag = 0;
    CFNumberRef targetPortalGroupTagNum = CFDictionaryGetValue(target,kiSCSISessionConfigPortalGroupTagKey);
    CFNumberGetValue(targetPortalGroupTagNum,kCFNumberIntType,&targetPortalGroupTag);
    return (TPGT)targetPortalGroupTag;
}

/*! Sets the target portal group tag. */
void iSCSISessionConfigSetTargetPortalGroupTag(iSCSIMutableSessionConfigRef target,
                                               TPGT targetPortalGroupTag)
{
    CFNumberRef targetPortalGroupTagNum = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&targetPortalGroupTag);
    CFDictionarySetValue(target,kiSCSISessionConfigPortalGroupTagKey,&targetPortalGroupTagNum);
}

/*! Gets the maximum number of connections. */
UInt32 iSCSISessionConfigGetMaxConnections(iSCSISessionConfigRef target)
{
    UInt32 maxConnections = 0;
    CFNumberRef maxConnectionsNum = CFDictionaryGetValue(target,kiSCSISessionConfigMaxConnectionsKey);
    CFNumberGetValue(maxConnectionsNum,kCFNumberIntType,&maxConnections);
    return (UInt32)maxConnections;
}

/*! Sets the maximum number of connections. */
void iSCSISessionConfigSetMaxConnections(iSCSIMutableSessionConfigRef target,
                                         UInt32 maxConnections)
{
    CFNumberRef maxConnectionsNum = CFNumberCreate(kCFAllocatorDefault,kCFNumberIntType,&maxConnections);
    CFDictionarySetValue(target,kiSCSISessionConfigMaxConnectionsKey,&maxConnectionsNum);
}

/*! Releases memory associated with an iSCSI session configuration object.
 *  @param config an iSCSI session configuration object. */
void iSCSISessionConfigRelease(iSCSISessionConfigRef config)
{
    CFRelease(config);
}

/*! Retains memory associated with an iSCSI session configuration object.
 *  @param config an iSCSI session configuration object. */
void iSCSISessionConfigRetain(iSCSISessionConfigRef config)
{
    CFRetain(config);
}

/*! Creates a new configuration object object from a dictionary representation.
 *  @return an iSCSI session configuration object or
 *  NULL if object creation failed. */
iSCSISessionConfigRef iSCSISessionConfigCreateWithDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies an configuration object to a dictionary representation.
 *  @param config an iSCSI configuration object.
 *  @return a dictionary representation of the configuration object or
 *  NULL if configuration object is invalid. */
CFDictionaryRef iSCSISessionConfigCreateDictionary(iSCSISessionConfigRef config)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,config);
}

/*! Copies the configuration object to a byte array representation.
 *  @param config an iSCSI configuration object.
 *  @return data representing the configuration object
 *  or NULL if the configuration object is invalid. */
CFDataRef iSCSISessionConfigCreateData(iSCSISessionConfigRef config)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,config,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}


CFStringRef kiSCSIConnectionConfigHeaderDigestKey = CFSTR("HeaderDigest");
CFStringRef kiSCSIConnectionConfigDataDigestKey = CFSTR("DataDigest");


/*! Convenience function.  Creates a new iSCSIConnectionConfigRef with the above keys. */
iSCSIMutableConnectionConfigRef iSCSIMutableConnectionConfigCreate()
{
    iSCSIMutableConnectionConfigRef portal = CFDictionaryCreateMutable(kCFAllocatorDefault,3,NULL,NULL);
    CFDictionaryAddValue(portal,kiSCSIConnectionConfigHeaderDigestKey,kCFBooleanFalse);
    CFDictionaryAddValue(portal,kiSCSIConnectionConfigDataDigestKey,kCFBooleanFalse);
    
    return portal;
}

/*! Gets whether a header digest is enabled in the portal object. */
bool iSCSIConnectionConfigGetHeaderDigest(iSCSIConnectionConfigRef portal)
{
    CFBooleanRef headerDigest = CFDictionaryGetValue(portal,kiSCSIConnectionConfigHeaderDigestKey);
    return CFBooleanGetValue(headerDigest);
}

/*! Sets whether a header digest is enabled in the portal object. */
void iSCSIConnectionConfigSetHeaderDigest(iSCSIMutableConnectionConfigRef portal,bool enable)
{
    CFBooleanRef headerDigest = kCFBooleanFalse;
    if(enable)
        headerDigest = kCFBooleanTrue;
    
    CFDictionarySetValue(portal,kiSCSIConnectionConfigDataDigestKey,headerDigest);
}

/*! Gets whether a data digest is enabled in the portal object. */
bool iSCSIConnectionConfigGetDataDigest(iSCSIConnectionConfigRef portal)
{
    CFBooleanRef dataDigest = CFDictionaryGetValue(portal,kiSCSIConnectionConfigDataDigestKey);
    return CFBooleanGetValue(dataDigest);
}

/*! Sets whether a data digest is enabled in the portal object. */
void iSCSIConnectionConfigSetDataDigest(iSCSIMutableConnectionConfigRef portal,bool enable)
{
    CFBooleanRef dataDigest = kCFBooleanFalse;
    if(enable)
        dataDigest = kCFBooleanTrue;
    
    CFDictionarySetValue(portal,kiSCSIConnectionConfigDataDigestKey,dataDigest);
}

/*! Releases memory associated with an iSCSI connection configuration object.
 *  @param config an iSCSI connection configuration object. */
void iSCSIConnectionConfigRelease(iSCSIConnectionConfigRef config)
{
    CFRelease(config);
}

/*! Retains memory associated with an iSCSI connection configuration object.
 *  @param config an iSCSI connection configuration object. */
void iSCSIConnectionConfigRetain(iSCSIConnectionConfigRef config)
{
    CFRetain(config);
}

/*! Creates a new configuration object object from a dictionary representation.
 *  @return an iSCSI connection configuration object or
 *  NULL if object creation failed. */
iSCSIConnectionConfigRef iSCSIConnectionConfigCreateWithDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies an configuration object to a dictionary representation.
 *  @param config an iSCSI configuration object.
 *  @return a dictionary representation of the configuration object or
 *  NULL if configuration object is invalid. */
CFDictionaryRef iSCSIConnectionConfigCreateDictionary(iSCSIConnectionConfigRef config)
{
     return CFDictionaryCreateCopy(kCFAllocatorDefault,config);
}

/*! Copies the configuration object to a byte array representation.
 *  @param config an iSCSI configuration object.
 *  @return data representing the configuration object
 *  or NULL if the configuration object is invalid. */
CFDataRef iSCSIConnectionConfigCreateData(iSCSIConnectionConfigRef config)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,config,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}