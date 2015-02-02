/*!
 * @author		Nareg Sinenian
 * @file		iSCSITypes.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		iSCSI data types used in user space.
 */

#include "iSCSITypes.h"

/*! iSCSI portal records are dictionaries with three keys with string values
 *  that specify the address (DNS name or IP address), the port, and the
 *  host interface to use when connecting to the portal. */
CFStringRef kiSCSIPortalAddresssKey = CFSTR("Address");
CFStringRef kiSCSIPortalPortKey = CFSTR("Port");
CFStringRef kiSCSIPortalHostInterfaceKey = CFSTR("HostInterface");

/*! Creates a new portal object from byte representation. */
iSCSIPortalRef iSCSIPoralCreateFromBytes(CFDataRef bytes)
{
    CFPropertyListFormat format;
    
    iSCSIPortalRef portal = CFPropertyListCreateWithData(kCFAllocatorDefault,bytes,0,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return portal;
    
    iSCSIPortalRelease(portal);
    return NULL;
}

/*! Convenience function.  Creates a new iSCSIPortalRef with the above keys. */
iSCSIMutablePortalRef iSCSIPortalCreate()
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
    CFDictionarySetValue(portal,kiSCSIPortalAddresssKey,address);
}

CFStringRef iSCSIPortalGetPort(iSCSIPortalRef portal)
{
    return CFDictionaryGetValue(portal,kiSCSIPortalPortKey);
}

void iSCSIPortalSetPort(iSCSIMutablePortalRef portal,CFStringRef port)
{
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
void iSCSIPortalRelease(iSCSITargetRef portal)
{
    CFRelease(portal);
}

/*! Retains an iSCSI portal object. */
void iSCSIPortalRetain(iSCSITargetRef portal)
{
    CFRetain(portal);
}

/*! Creates a new portal object from a dictionary representation. */
iSCSIPortalRef iSCSIPortalCreateFromDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies a target object to a dictionary representation. */
CFDictionaryRef iSCSIPortalCopyToDictionary(iSCSIPortalRef portal)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,portal);
}

/*! Copies the portal object to a byte array representation. */
CFDataRef iSCSIPortalCopyToBytes(iSCSIPortalRef portal)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,portal,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}


/*! Creates a new target object from byte representation. */
iSCSITargetRef iSCSITargetCreateFromBytes(CFDataRef bytes)
{
    CFPropertyListFormat format;
    
    iSCSITargetRef target = CFPropertyListCreateWithData(kCFAllocatorDefault,bytes,0,&format,NULL);
    
    if(format == kCFPropertyListBinaryFormat_v1_0)
        return target;
    
    iSCSITargetRelease(target);
    return NULL;
}

/*! iSCSI portal records are dictionaries with three keys with string values
 *  that specify the address (DNS name or IP address), the port, and the
 *  host interface to use when connecting to the portal. */
CFStringRef kiSCSITargetNameKey = CFSTR("TargetName");
CFStringRef kiSCSITargetHeaderDigestKey = CFSTR("HeaderDigest");
CFStringRef kiSCSITargetDataDigestKey = CFSTR("DataDigest");

/*! Convenience function.  Creates a new iSCSITargetRef with the above keys. */
iSCSIMutableTargetRef iSCSITargetCreate()
{
    iSCSIMutableTargetRef target = CFDictionaryCreateMutable(kCFAllocatorDefault,5,NULL,NULL);
    CFDictionaryAddValue(target,kiSCSITargetNameKey,CFSTR(""));
    CFDictionaryAddValue(target,kiSCSITargetHeaderDigestKey,CFSTR(""));
    CFDictionaryAddValue(target,kiSCSITargetDataDigestKey,CFSTR(""));
    
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
    CFDictionarySetValue(target,kiSCSITargetNameKey,name);
}

/*! Gets whether a header digest is enabled in the target object. */
bool iSCSITargetGetHeaderDigest(iSCSITargetRef target)
{
    CFBooleanRef headerDigest = CFDictionaryGetValue(target,kiSCSITargetHeaderDigestKey);
    return CFBooleanGetValue(headerDigest);
}

/*! Sets whether a header digest is enabled in the target object. */
void iSCSITargetSetHeaderDigest(iSCSIMutableTargetRef target,bool enable)
{
    CFBooleanRef headerDigest = kCFBooleanFalse;
    if(enable)
        headerDigest = kCFBooleanTrue;
    
    CFDictionarySetValue(target,kiSCSITargetDataDigestKey,headerDigest);
}

/*! Gets whether a data digest is enabled in the target object. */
bool iSCSITargetGetDataDigest(iSCSITargetRef target)
{
    CFBooleanRef dataDigest = CFDictionaryGetValue(target,kiSCSITargetDataDigestKey);
    return CFBooleanGetValue(dataDigest);
}

/*! Sets whether a data digest is enabled in the target object. */
void iSCSITargetSetDataDigest(iSCSIMutableTargetRef target,bool enable)
{
    CFBooleanRef dataDigest = kCFBooleanFalse;
    if(enable)
        dataDigest = kCFBooleanTrue;
    
    CFDictionarySetValue(target,kiSCSITargetDataDigestKey,dataDigest);
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
iSCSITargetRef iSCSITargetCreateFromDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies a target object to a dictionary representation. */
CFDictionaryRef iSCSITargetCopyToDictionary(iSCSITargetRef target)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,target);
}

/*! Copies the target object to a byte array representation. */
CFDataRef iSCSITargetCopyToBytes(iSCSITargetRef target)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,target,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}





/*! Creates a new authentication object from byte representation. */
iSCSIAuthRef iSCSIAuthCreateFromBytes(CFDataRef bytes)
{
    CFPropertyListFormat format;
    
    iSCSIAuthRef auth = CFPropertyListCreateWithData(kCFAllocatorDefault,bytes,0,&format,NULL);

    if(format == kCFPropertyListBinaryFormat_v1_0)
        return auth;
    
    iSCSIAuthRelease(auth);
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
    
    return CFDictionaryCreate(kCFAllocatorDefault,keys,values,5,NULL,NULL);
}

/*! Returns the CHAP authentication parameter values if the authentication
 *  method is actually CHAP. */
errno_t iSCSIAuthGetCHAPValues(iSCSIAuthRef auth,
                               CFStringRef * initiatorUser,
                               CFStringRef * initiatorSecret,
                               CFStringRef * targetUser,
                               CFStringRef * targetSecret)
{
    if(iSCSIAuthGetMethod(auth) != kiSCSIAuthMethodCHAP)
        return EINVAL;
    
    if(!auth || !initiatorUser || !initiatorSecret || !targetUser || !targetSecret)
        return EINVAL;
    
    *initiatorUser = CFDictionaryGetValue(auth,CFSTR("InitiatorUser"));
    *initiatorUser = CFDictionaryGetValue(auth,CFSTR("InitiatorSecret"));
    *targetUser = CFDictionaryGetValue(auth,CFSTR("TargetUser"));
    *targetSecret = CFDictionaryGetValue(auth,CFSTR("TargetSecret"));
    
    return 0;
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
iSCSIAuthRef iSCSIAuthCreateFromDictionary(CFDictionaryRef dict)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,dict);
}

/*! Copies an authentication object to a dictionary representation. */
CFDictionaryRef iSCSIAuthCopyToDictionary(iSCSIAuthRef auth)
{
    return CFDictionaryCreateCopy(kCFAllocatorDefault,auth);
}

/*! Copies the authentication object to a byte array representation. */
CFDataRef iSCSIAuthCopyToBytes(iSCSIAuthRef auth)
{
    return CFPropertyListCreateData(kCFAllocatorDefault,auth,kCFPropertyListBinaryFormat_v1_0,0,NULL);
}

