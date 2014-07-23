/**
 * @author		Nareg Sinenian
 * @file		iSCSIInitiatorClient.h
 * @date		October 13, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_INITIATOR_CLIENT_H__
#define __ISCSI_INITIATOR_CLIENT_H__

#include <IOKit/IOUserClient.h>
#include "iSCSIInterfaceShared.h"
#include "iSCSIPDUShared.h"
#include "iSCSIVirtualHBA.h"

#define iSCSIInitiatorClient com_NSinenian_iSCSIInitiatorClient

class iSCSIInitiatorClient : public IOUserClient
{
	OSDeclareDefaultStructors(iSCSIInitiatorClient);

public:
	
	/** Invoked after initWithTask() as a result of the user-space application
	 *	calling IOServiceOpen(). */
	virtual bool start(IOService * provider);
	
	/** Called to stop this service. */
	virtual void stop(IOService * provider);
	
	/** Invoked as a result of the user-space application calling
	 *	IOServiceOpen(). */
	virtual bool initWithTask(task_t owningTask,
							  void * securityToken,
							  UInt32 type,
							  OSDictionary * properties);
    
	/** Dispatched function called from the device interface to this user
	 *	client .*/
	static IOReturn OpenInitiator(iSCSIInitiatorClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);

	/** Dispatched function called from the device interface to this user
	 *	client .*/
	static IOReturn	CloseInitiator(iSCSIInitiatorClient * target,
                                   void * reference,
                                   IOExternalMethodArguments * args);
	
    /** Dispatched function invoked from user-space to create new session. */
    static IOReturn CreateSession(iSCSIInitiatorClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);

    /** Dispatched function invoked from user-space to release session. */
    static IOReturn ReleaseSession(iSCSIInitiatorClient * target,
                                   void * reference,
                                   IOExternalMethodArguments * args);
        
    static IOReturn SetSessionOptions(iSCSIInitiatorClient * target,
                                      void * reference,
                                      IOExternalMethodArguments * args);
    
    static IOReturn GetSessionOptions(iSCSIInitiatorClient * target,
                                      void * reference,
                                      IOExternalMethodArguments * args);

    /** Dispatched function invoked from user-space to create new connection. */
    static IOReturn CreateConnection(iSCSIInitiatorClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);

    /** Dispatched function invoked from user-space to release connection. */
    static IOReturn ReleaseConnection(iSCSIInitiatorClient * target,
                                      void * reference,
                                      IOExternalMethodArguments * args);
    
    static IOReturn ActivateConnection(iSCSIInitiatorClient * target,
                                       void * reference,
                                       IOExternalMethodArguments * args);

    static IOReturn ActivateAllConnections(iSCSIInitiatorClient * target,
                                           void * reference,
                                           IOExternalMethodArguments * args);

    static IOReturn DeactivateConnection(iSCSIInitiatorClient * target,
                                         void * reference,
                                         IOExternalMethodArguments * args);

    static IOReturn DeactivateAllConnections(iSCSIInitiatorClient * target,
                                             void * reference,
                                             IOExternalMethodArguments * args);

    static IOReturn GetConnection(iSCSIInitiatorClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);
    
    static IOReturn GetNumConnections(iSCSIInitiatorClient * target,
                                      void * reference,
                                      IOExternalMethodArguments * args);


    /** Dispatched function invoked from user-space to send data
     *  over an existing, active connection. */
    static IOReturn SendBHS(iSCSIInitiatorClient * target,
                            void * reference,
                            IOExternalMethodArguments * args);

    /** Dispatched function invoked from user-space to send data
     *  over an existing, active connection. */
    static IOReturn SendData(iSCSIInitiatorClient * target,
                             void * reference,
                             IOExternalMethodArguments * args);
    
    /** Dispatched function invoked from user-space to receive data
     *  over an existing, active connection, and to retrieve the size of
     *  a user-space buffer that is required to hold the data. */
    static IOReturn RecvBHS(iSCSIInitiatorClient * target,
                            void * reference,
                            IOExternalMethodArguments * args);
    
    /** Dispatched function invoked from user-space to receive data
     *  over an existing, active connection, and to retrieve the size of
     *  a user-space buffer that is required to hold the data. */
    static IOReturn RecvData(iSCSIInitiatorClient * target,
                             void * reference,
                             IOExternalMethodArguments * args);
    
    static IOReturn SetConnectionOptions(iSCSIInitiatorClient * target,
                                         void * reference,
                                         IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionOptions(iSCSIInitiatorClient * target,
                                         void * reference,
                                         IOExternalMethodArguments * args);
    
    static IOReturn GetActiveConnection(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args);
    
	/** Overrides IOUserClient's externalMethod to allow users to call
	 *	dispatched functions defined by this subclass. */
	virtual IOReturn externalMethod(uint32_t selector,
                                    IOExternalMethodArguments * args,
                                    IOExternalMethodDispatch * dispatch,
                                    OSObject * target,
                                    void * ref);
	
	/** Opens an exclusive connection to the iSCSI initiator device driver. The
	 *	driver can handle multiple iSCSI targets with multiple LUNs. This
	 *	function is remotely invoked by the user-space application. */
	virtual IOReturn open();

	/** Closes the connection to the iSCSI initiator device driver.  Leaves
	 *	iSCSI target connections intact. This function is remotely invoked
	 *	by the user-space application. */
	virtual IOReturn close();
	
	/** Invoked when the user-space application calls IOServiceClose. */
	virtual IOReturn clientClose();
	
	/** Invoked when the user-space application is terminated without calling
	 *	IOServiceClose or remotely invoking close(). */
	virtual IOReturn clientDied();
	
	/** Array of methods that can be called by user-space. */
	static const IOExternalMethodDispatch methods[kiSCSIInitiatorNumMethods];
	
private:
	/** Points to the provider object (driver). The pointer is assigned
	 *	when the start() function is called by the I/O Kit. */
	iSCSIVirtualHBA * provider;
    
    /** Holds a basic header segment (buffer). Used when sending and
     *  receiving PDUs to and from the target. */
    iSCSIPDUInitiatorBHS bhsBuffer;
	
	/** Identifies the Mach task (user-space) that opened a connection to this
	 *	client. */
	task_t owningTask;
	
	/** A security token that identifies user privileges. */
	void * securityToken;
	
	/** A security type that identifies user privileges. */
	UInt32 type;
};

#endif /* defined(__ISCSI_INITIATOR_CLIENT_H__) */
