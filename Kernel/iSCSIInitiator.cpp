/*!
 * @author		Nareg Sinenian
 * @file		iSCSIInitiator.cpp
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#include <IOKit/IOLib.h>
#include "iSCSIInitiator.h"
#include "iSCSIKernelInterfaceShared.h"

/*! Required IOKit macro that defines the constructors, destructors, etc. */
OSDefineMetaClassAndStructors(iSCSIInitiator,IOService);

/*! The superclass is defined as a macro to follow IOKit conventions. */
#define super IOService

bool iSCSIInitiator::init(OSDictionary * dictionary)
{
	if(super::init(dictionary))
        return true;
    
    return false;
}

void iSCSIInitiator::free(void)
{
	super::free();
}

IOService * iSCSIInitiator::probe(IOService * provider, SInt32 * score)
{
	return super::probe(provider,score);
}

bool iSCSIInitiator::start(IOService * provider)
{
	if(super::start(provider))
    {
        registerService();
        return true;
    }
	return false;
}

void iSCSIInitiator::stop(IOService * provider)
{
	super::stop(provider);
}
