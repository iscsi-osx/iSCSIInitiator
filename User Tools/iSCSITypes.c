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
CFStringRef kiSCSIPortalHostInterfaceKey = CFSTR("Host Interface");

/*! Creates a new portal object from byte representation. */
iSCSIPortalRef iSCSIPortalCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    iSCSIPortalRef portal = CFPropertyListCreateWithData(
            kCFAllocatorDefault,data,kCFPropertyListImmutable,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return portal;

    CFRelease(portal);
    return NULL;
}

/*! Convenience function.  Creates a new iSCSIPortalRef with the above keys. */
iSCSIMutablePortalRef iSCSIPortalCreateMutable()
{
    iSCSIMutablePortalRef portal = CFDictionaryCreateMutable(kCFAllocatorDefault,3,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(portal,kiSCSIPortalAddresssKey,CFSTR(""));
    CFDictionaryAddValue(portal,kiSCSIPortalPortKey,CFSTR("3260"));
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

    CFRelease(target);
    return NULL;
}

/*! iSCSI target records are dictionaries with keys with string values
 *  that specify the target name and other parameters. */
CFStringRef kiSCSITargetIQNKey = CFSTR("Target Name");

/*! Convenience function.  Creates a new iSCSITargetRef with the above keys. */
iSCSIMutableTargetRef iSCSITargetCreateMutable()
{
    iSCSIMutableTargetRef target = CFDictionaryCreateMutable(kCFAllocatorDefault,5,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    return target;
}

/*! Gets the name associated with the iSCSI target. */
CFStringRef iSCSITargetGetIQN(iSCSITargetRef target)
{
    return CFDictionaryGetValue(target,kiSCSITargetIQNKey);
}

/*! Sets the name associated with the iSCSI target. */
void iSCSITargetSetName(iSCSIMutableTargetRef target,CFStringRef name)
{
    // Ignore blanks
    if(CFStringCompare(name,CFSTR(""),0) == kCFCompareEqualTo)
        return;
    
    CFDictionarySetValue(target,kiSCSITargetIQNKey,name);
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
    const void * keys[] = {CFSTR("Authentication Method")};
    const void * values[] = {CFSTR("None")};
    
    return CFDictionaryCreate(kCFAllocatorDefault,keys,values,1,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
}

/*! Creates a new iSCSIAuth object with empty authentication parameters
 *  (defaults to no authentication). */
iSCSIAuthRef iSCSIAuthCreateCHAP(CFStringRef targetUser,
                                 CFStringRef targetSecret,
                                 CFStringRef initiatorUser,
                                 CFStringRef initiatorSecret)
{
    // Required parameters
    if(!targetUser || !targetSecret)
        return NULL;
    
    const void * keys[] = {
        CFSTR("Authentication Method"),
        CFSTR("Target User"),
        CFSTR("Target Secret"),
        CFSTR("Initiator User"),
        CFSTR("Initiator Secret")
    };
    
    const void * values[] = {
        CFSTR("CHAP"),
        targetUser,
        targetSecret,
        initiatorUser,
        initiatorSecret
    };

    // Check for mutual CHAP before including the initiatorUser and initiatorSecret
    // (include the first there terms of the arrays above when mutual CHAP
    // is not used).
    if(!initiatorUser || !initiatorSecret)
        return CFDictionaryCreate(kCFAllocatorDefault,keys,values,3,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    
    // Else we include all parameters
    return CFDictionaryCreate(kCFAllocatorDefault,keys,values,5,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
}

/*! Returns the CHAP authentication parameter values if the authentication
 *  method is actually CHAP. */
void iSCSIAuthGetCHAPValues(iSCSIAuthRef auth,
                            CFStringRef * targetUser,
                            CFStringRef * targetSecret,
                            CFStringRef * initiatorUser,
                            CFStringRef * initiatorSecret)
{
    if(!auth || !targetUser || !targetSecret || !initiatorUser || !initiatorUser || (iSCSIAuthGetMethod(auth) != kiSCSIAuthMethodCHAP))
        return;

    *initiatorUser = CFDictionaryGetValue(auth,CFSTR("Initiator User"));
    *initiatorUser = CFDictionaryGetValue(auth,CFSTR("Initiator Secret"));
    *targetUser = CFDictionaryGetValue(auth,CFSTR("Target User"));
    *targetSecret = CFDictionaryGetValue(auth,CFSTR("Target Secret"));
}

/*! Gets the authentication method used. */
enum iSCSIAuthMethods iSCSIAuthGetMethod(iSCSIAuthRef auth)
{
    CFStringRef authMethod = CFDictionaryGetValue(auth,CFSTR("Authentication Method"));
    
    if(!authMethod)
        return kiSCSIAuthMethodInvalid;
    
    if(CFStringCompare(authMethod,CFSTR("CHAP"),0) == kCFCompareEqualTo)
        return kiSCSIAuthMethodCHAP;
    
    else if(CFStringCompare(authMethod,CFSTR("None"),0) == kCFCompareEqualTo)
        return kiSCSIAuthMethodNone;
    
    return kiSCSIAuthMethodInvalid;
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
iSCSIMutableDiscoveryRecRef iSCSIDiscoveryRecCreateMutable()
{
    return CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
}

/*! Creates a new discovery record object from a dictionary representation.
 * @return an iSCSI discovery object or NULL if object creation failed. */
iSCSIDiscoveryRecRef iSCSIDiscoveryRecCreateWithDictionary(CFDictionaryRef dict)
{
    return CFPropertyListCreateDeepCopy(kCFAllocatorDefault,dict,kCFPropertyListImmutable);
}

/*! Creates a discovery record from an external data representation.
 * @param data data used to construct an iSCSI discovery object.
 * @return an iSCSI discovery object or NULL if object creation failed */
iSCSIMutableDiscoveryRecRef iSCSIDiscoveryRecCreateMutableWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    iSCSIMutableDiscoveryRecRef discoveryRec = (iSCSIMutableDiscoveryRecRef)
        CFPropertyListCreateWithData(kCFAllocatorDefault,data,kCFPropertyListImmutable,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return discoveryRec;
    
    if(discoveryRec)
        CFRelease(discoveryRec);
    
    return NULL;
}

/*! Add a portal to a specified portal group tag for a given target.  If the
 *  target does not exist, it is added to the discovery record.
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target to add.
 *  @param portalGroupTag the target portal group tag to add.
 *  @param portal the iSCSI portal to add. */
void iSCSIDiscoveryRecAddPortal(iSCSIMutableDiscoveryRecRef discoveryRec,
                                CFStringRef targetIQN,
                                CFStringRef portalGroupTag,
                                iSCSIPortalRef portal)
{
    // Validate inputs
    if(!discoveryRec || !targetIQN || !portalGroupTag || !portal)
        return;
    
    CFMutableDictionaryRef targetDict;
    
    // If target doesn't exist add it
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetIQN,(void *)&targetDict))
    {
        targetDict = CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);
    }
    
    // If the group tag doesn't exist add it
    CFMutableArrayRef portalsArray;
    
    if(!CFDictionaryGetValueIfPresent(targetDict,portalGroupTag,(void *)&portalsArray))
    {
        portalsArray = CFArrayCreateMutable(kCFAllocatorDefault,0,
                                            &kCFTypeArrayCallBacks);
    
    }
    
    CFDictionaryRef portalDict = iSCSIPortalCreateDictionary(portal);
    
    // Add a new portal
    CFArrayAppendValue(portalsArray,portalDict);
    CFDictionarySetValue(targetDict,portalGroupTag,portalsArray);
    CFDictionarySetValue(discoveryRec,targetIQN,targetDict);
    
    CFRelease(portalDict);
}


/*! Add a target to the discovery record (without any portals).
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target to add. */
void iSCSIDiscoveryRecAddTarget(iSCSIMutableDiscoveryRecRef discoveryRec,
                                CFStringRef targetIQN)
{
    // Validate inputs
    if(!discoveryRec || !targetIQN)
        return;

    CFMutableDictionaryRef targetDict;

    // If target doesn't exist add it
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetIQN,(void *)&targetDict))
    {
        targetDict = CFDictionaryCreateMutable(kCFAllocatorDefault,0,
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);
    }

    // Add target to discovery record
    CFDictionarySetValue(discoveryRec,targetIQN,targetDict);
}

/*! Creates a CFArray object containing CFString objects with names of
 *  all of the targets in the discovery record.
 *  @param discoveryRec the discovery record.
 *  @return an array of strings with names of the targets in the record. */
CFArrayRef iSCSIDiscoveryRecCreateArrayOfTargets(iSCSIDiscoveryRecRef discoveryRec)
{
    // Validate input
    if(!discoveryRec)
        return NULL;
    
    // Get all keys, which correspond to the targets
    const CFIndex count = CFDictionaryGetCount(discoveryRec);
    const void * keys[count];
    CFDictionaryGetKeysAndValues(discoveryRec,keys,NULL);
    
    CFArrayRef targets = CFArrayCreate(kCFAllocatorDefault,
                                       keys,
                                       count,
                                       &kCFTypeArrayCallBacks);
    return targets;
}

/*! Creates a CFArray object containing CFString objects with portal group
 *  tags for a particular target.
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target.
 *  @return an array of strings with portal group tags for the specified target. */
CFArrayRef iSCSIDiscoveryRecCreateArrayOfPortalGroupTags(iSCSIDiscoveryRecRef discoveryRec,
                                                         CFStringRef targetIQN)
{
    // Validate inputs
    if(!discoveryRec || !targetIQN)
        return NULL;
    
    // If target doesn't exist return NULL
    CFMutableDictionaryRef targetDict;
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetIQN,(void *)&targetDict))
        return NULL;
    
    // Otherwise get all keys, which correspond to the portal group tags
    const CFIndex count = CFDictionaryGetCount(targetDict);
    
    const void * keys[count];
    CFDictionaryGetKeysAndValues(targetDict,keys,NULL);
    
    CFArrayRef portalGroups = CFArrayCreate(kCFAllocatorDefault,
                                            keys,
                                            count,
                                            &kCFTypeArrayCallBacks);
    return portalGroups;
}

/*! Gets all of the portals associated with a partiular target and portal
 *  group tag.
 *  @param discoveryRec the discovery record.
 *  @param targetIQN the name of the target.
 *  @param portalGroupTag the portal group tag associated with the target.
 *  @return an array of iSCSIPortal objects associated with the specified
 *  group tag for the specified target. */
CFArrayRef iSCSIDiscoveryRecGetPortals(iSCSIDiscoveryRecRef discoveryRec,
                                       CFStringRef targetIQN,
                                       CFStringRef portalGroupTag)
{
    // Validate inputs
    if(!discoveryRec || !targetIQN || !portalGroupTag)
        return NULL;
    
    // If target doesn't exist return NULL
    CFMutableDictionaryRef targetDict;
    if(!CFDictionaryGetValueIfPresent(discoveryRec,targetIQN,(void *)&targetDict))
        return NULL;

    // Grab requested portal group
    CFArrayRef portalGroup = NULL;
    CFDictionaryGetValueIfPresent(targetDict,portalGroupTag,(void *)&portalGroup);
    
    return portalGroup;
}

/*! Releases memory associated with an iSCSI discovery record object.
 * @param target the iSCSI discovery record object. */
void iSCSIDiscoveryRecRelease(iSCSIDiscoveryRecRef discoveryRec)
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
CFDictionaryRef iSCSIDiscoveryRecCreateDictionary(iSCSIDiscoveryRecRef discoveryRec)
{
    return (CFDictionaryRef)CFPropertyListCreateDeepCopy(kCFAllocatorDefault,
                                        (CFDictionaryRef)discoveryRec,
                                        kCFPropertyListImmutable);

}

/*! Copies the discovery record object to a byte array representation.
 *  @param auth an iSCSI discovery record object.
 *  @return data representing the discovery record object
 *  or NULL if the discovery record object is invalid. */
CFDataRef iSCSIDiscoveryRecCreateData(iSCSIMutableDiscoveryRecRef discoveryRec)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,discoveryRec,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}


CFStringRef kiSCSISessionConfigErrorRecoveryKey = CFSTR("Error Recovery Level");
CFStringRef kiSCSISessionConfigPortalGroupTagKey = CFSTR("Target Portal Group Tag");
CFStringRef kiSCSISessionConfigMaxConnectionsKey = CFSTR("Maximum Connections");

/*! Convenience function.  Creates a new iSCSISessionConfigRef with the above keys. */
iSCSIMutableSessionConfigRef iSCSISessionConfigCreateMutable()
{
    iSCSIMutableSessionConfigRef config = CFDictionaryCreateMutable(kCFAllocatorDefault,5,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    iSCSISessionConfigSetErrorRecoveryLevel(config,kRFC3720_ErrorRecoveryLevel);
    iSCSISessionConfigSetMaxConnections(config,kRFC3720_MaxConnections);
    iSCSISessionConfigSetTargetPortalGroupTag(config,0);
    return config;
}

/*! Creates a mutable session configuration object from an existing one. */
iSCSIMutableSessionConfigRef iSCSISessionConfigCreateMutableWithExisting(iSCSISessionConfigRef config)
{
    return (iSCSIMutableSessionConfigRef)CFPropertyListCreateDeepCopy(
        kCFAllocatorDefault,config,kCFPropertyListMutableContainersAndLeaves);
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
    CFDictionarySetValue(target,kiSCSISessionConfigErrorRecoveryKey,errorRecoveryLevelNum);
    CFRelease(errorRecoveryLevelNum);
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
    CFDictionarySetValue(target,kiSCSISessionConfigPortalGroupTagKey,targetPortalGroupTagNum);
    CFRelease(targetPortalGroupTagNum);
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
    CFDictionarySetValue(target,kiSCSISessionConfigMaxConnectionsKey,maxConnectionsNum);
    CFRelease(maxConnectionsNum);
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


/*! Creates a new session config object from an external data representation.
 *  @param data data used to construct an iSCSI session config object.
 *  @return an iSCSI session configuration object
 *  or NULL if object creation failed */
iSCSISessionConfigRef iSCSISessionConfigCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    
    iSCSISessionConfigRef sessCfg = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return sessCfg;
    
    return NULL;
}

CFStringRef kiSCSIConnectionConfigHeaderDigestKey = CFSTR("Header Digest");
CFStringRef kiSCSIConnectionConfigDataDigestKey = CFSTR("Data Digest");


/*! Convenience function.  Creates a new iSCSIConnectionConfigRef with the above keys. */
iSCSIMutableConnectionConfigRef iSCSIConnectionConfigCreateMutable()
{
    iSCSIMutableConnectionConfigRef portal = CFDictionaryCreateMutable(kCFAllocatorDefault,3,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(portal,kiSCSIConnectionConfigHeaderDigestKey,kCFBooleanFalse);
    CFDictionaryAddValue(portal,kiSCSIConnectionConfigDataDigestKey,kCFBooleanFalse);
    
    return portal;
}

/*! Creates a mutable connection configuration object from an existing one. */
iSCSIMutableConnectionConfigRef iSCSIConnectionConfigCreateMutableWithExisting(iSCSIConnectionConfigRef config)
{
    return (iSCSIMutableConnectionConfigRef)CFPropertyListCreateDeepCopy(
        kCFAllocatorDefault,config,kCFPropertyListMutableContainersAndLeaves);
}

/*! Gets whether a header digest is enabled in the portal object. */
bool iSCSIConnectionConfigGetHeaderDigest(iSCSIConnectionConfigRef config)
{
    CFBooleanRef headerDigest = CFDictionaryGetValue(config,kiSCSIConnectionConfigHeaderDigestKey);
    return CFBooleanGetValue(headerDigest);
}

/*! Sets whether a header digest is enabled in the portal object. */
void iSCSIConnectionConfigSetHeaderDigest(iSCSIMutableConnectionConfigRef config,bool enable)
{
    CFBooleanRef headerDigest = kCFBooleanFalse;
    if(enable)
        headerDigest = kCFBooleanTrue;
    
    CFDictionarySetValue(config,kiSCSIConnectionConfigHeaderDigestKey,headerDigest);
}

/*! Gets whether a data digest is enabled in the portal object. */
bool iSCSIConnectionConfigGetDataDigest(iSCSIConnectionConfigRef config)
{
    CFBooleanRef dataDigest = (CFBooleanRef)CFDictionaryGetValue(config,kiSCSIConnectionConfigDataDigestKey);
    return CFBooleanGetValue(dataDigest);
}

/*! Sets whether a data digest is enabled in the portal object. */
void iSCSIConnectionConfigSetDataDigest(iSCSIMutableConnectionConfigRef config,bool enable)
{
    CFBooleanRef dataDigest = kCFBooleanFalse;
    if(enable)
        dataDigest = kCFBooleanTrue;
    
    CFDictionarySetValue(config,kiSCSIConnectionConfigDataDigestKey,dataDigest);
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


/*! Creates a new connection config object from an external data representation.
 *  @param data data used to construct an iSCSI connection config object.
 *  @return an iSCSI connection configuration object
 *  or NULL if object creation failed */
iSCSIConnectionConfigRef iSCSIConnectionConfigCreateWithData(CFDataRef data)
{
    CFPropertyListFormat format;
    
    iSCSIConnectionConfigRef connCfg = CFPropertyListCreateWithData(kCFAllocatorDefault,data,0,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return connCfg;
    
    return NULL;
}
