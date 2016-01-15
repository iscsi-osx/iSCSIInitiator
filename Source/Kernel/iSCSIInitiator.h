/*!
 * @author		Nareg Sinenian
 * @file		iSCSIInitiator.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_INITIATOR_H__
#define __ISCSI_INITIATOR_H__

#include <IOKit/IOService.h>
#include "iSCSIKernelClasses.h"

/*! This class defines the iSCSI driver entry point. This driver is a virtual
 *	device and does control hardware directly. The class iSCSIInitiator acts
 *  as a nub for a virtual host bus adapter (HBA), called the 
 *	iSCSIInitiatorVirtualHBA.  That virtual HBA matches against this class.
 */
class iSCSIInitiator : public IOService
{
	OSDeclareDefaultStructors(iSCSIInitiator);
	
public:
	virtual bool init(OSDictionary * dictionary = 0);
	virtual void free(void);
	virtual IOService * probe(IOService * provider, SInt32 * score);
	virtual bool start(IOService * provider);
	virtual void stop(IOService * provider);
};

#endif /* (__ISCSI_INITIATOR_H__) */