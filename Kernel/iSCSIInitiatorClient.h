/*!
 * @author		Nareg Sinenian
 * @file		iSCSIInitiatorClient.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_INITIATOR_CLIENT_H__
#define __ISCSI_INITIATOR_CLIENT_H__

#include <IOKit/IOUserClient.h>

#include "iSCSIKernelClasses.h"
#include "iSCSIKernelInterfaceShared.h"
#include "iSCSIPDUShared.h"
#include "iSCSIVirtualHBA.h"
#include "iSCSITypesShared.h"

class iSCSIInitiatorClient : public IOUserClient
{
	OSDeclareDefaultStructors(iSCSIInitiatorClient);

public:
	
	/*! Invoked after initWithTask() as a result of the user-space application
	 *	calling IOServiceOpen(). */
	virtual bool start(IOService * provider);
	
	/*! Called to stop this service. */
	virtual void stop(IOService * provider);
	
	/*! Invoked as a result of the user-space application calling
	 *	IOServiceOpen(). */
	virtual bool initWithTask(task_t owningTask,
							  void * securityToken,
							  UInt32 type,
							  OSDictionary * properties);
    
	/*! Dispatched function called from the device interface to this user
	 *	client .*/
	static IOReturn OpenInitiator(iSCSIInitiatorClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);

	/*! Dispatched function called from the device interface to this user
	 *	client .*/
	static IOReturn	CloseInitiator(iSCSIInitiatorClient * target,
                                   void * reference,
                                   IOExternalMethodArguments * args);
	
    /*! Dispatched function invoked from user-space to create new session. */
    static IOReturn CreateSession(iSCSIInitiatorClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to release session. */
    static IOReturn ReleaseSession(iSCSIInitiatorClient * target,
                                   void * reference,
                                   IOExternalMethodArguments * args);
        
    static IOReturn SetSessionOption(iSCSIInitiatorClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);
    
    static IOReturn GetSessionOption(iSCSIInitiatorClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to create new connection. */
    static IOReturn CreateConnection(iSCSIInitiatorClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to release connection. */
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
    
    static IOReturn GetSessionIdForTargetIQN(iSCSIInitiatorClient * target,
                                               void * reference,
                                               IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionIdForPortalAddress(iSCSIInitiatorClient * target,
                                            void * reference,
                                            IOExternalMethodArguments * args);
    
    static IOReturn GetSessionIds(iSCSIInitiatorClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionIds(iSCSIInitiatorClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);
    
    static IOReturn GetTargetIQNForSessionId(iSCSIInitiatorClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args);
    

    static IOReturn GetPortalAddressForConnectionId(iSCSIInitiatorClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args);

    
    static IOReturn GetPortalPortForConnectionId(iSCSIInitiatorClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args);
    
    static IOReturn GetHostInterfaceForConnectionId(iSCSIInitiatorClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to send data
     *  over an existing, active connection. */
    static IOReturn SendBHS(iSCSIInitiatorClient * target,
                            void * reference,
                            IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to send data
     *  over an existing, active connection. */
    static IOReturn SendData(iSCSIInitiatorClient * target,
                             void * reference,
                             IOExternalMethodArguments * args);
    
    /*! Dispatched function invoked from user-space to receive data
     *  over an existing, active connection, and to retrieve the size of
     *  a user-space buffer that is required to hold the data. */
    static IOReturn RecvBHS(iSCSIInitiatorClient * target,
                            void * reference,
                            IOExternalMethodArguments * args);
    
    /*! Dispatched function invoked from user-space to receive data
     *  over an existing, active connection, and to retrieve the size of
     *  a user-space buffer that is required to hold the data. */
    static IOReturn RecvData(iSCSIInitiatorClient * target,
                             void * reference,
                             IOExternalMethodArguments * args);
    
    static IOReturn SetConnectionOption(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionOption(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args);
    
    static IOReturn GetActiveConnection(iSCSIInitiatorClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args);
    
	/*! Overrides IOUserClient's externalMethod to allow users to call
	 *	dispatched functions defined by this subclass. */
	virtual IOReturn externalMethod(uint32_t selector,
                                    IOExternalMethodArguments * args,
                                    IOExternalMethodDispatch * dispatch,
                                    OSObject * target,
                                    void * ref);
	
	/*! Opens an exclusive connection to the iSCSI initiator device driver. The
	 *	driver can handle multiple iSCSI targets with multiple LUNs. This
	 *	function is remotely invoked by the user-space application. */
	virtual IOReturn open();

	/*! Closes the connection to the iSCSI initiator device driver.  Leaves
	 *	iSCSI target connections intact. This function is remotely invoked
	 *	by the user-space application. */
	virtual IOReturn close();
	
	/*! Invoked when the user-space application calls IOServiceClose. */
	virtual IOReturn clientClose();
	
	/*! Invoked when the user-space application is terminated without calling
	 *	IOServiceClose or remotely invoking close(). */
	virtual IOReturn clientDied();
    
    /*! Invoked when a user-space application registers a notification port
     *  with this user client.
     *  @param port the port associated with the client connection.
     *  @param type the type.
     *  @param refCon a user reference value.
     *  @return an error code indicating the result of the operation. */
    virtual IOReturn registerNotificationPort(mach_port_t port,
                                              UInt32 type,
                                              io_user_reference_t refCon);
    
    /*! Send a notification message to the user-space application.
     *  @param message details regarding the notification message.
     *  @return an error code indicating the result of the operation. */
    IOReturn sendNotification(iSCSIKernelNotificationMessage * message);

    /*! Sends a notification message to the user indicating that an
     *  iSCSI asynchronous event has occured.
     *  @param sessionId the session identifier.
     *  @param connectionId the connection identifier.
     *  @param event the asynchronsou event.
     *  @return an error code indicating the result of the operation. */
    IOReturn sendAsyncMessageNotification(SID sessionId,
                                          CID connectionId,
                                          enum iSCSIPDUAsyncMsgEvent event);
    
    /*! Sends a notification message to the user indicating that the kernel
     *  extension will be terminating. 
     *  @return an error code indicating the result of the operation. */
    IOReturn sendTerminateMessageNotification();

	/*! Array of methods that can be called by user-space. */
	static const IOExternalMethodDispatch methods[kiSCSIInitiatorNumMethods];
	
private:

	/*! Points to the provider object (driver). The pointer is assigned
	 *	when the start() function is called by the I/O Kit. */
	iSCSIVirtualHBA * provider;
    
    /*! Holds a basic header segment (buffer). Used when sending and
     *  receiving PDUs to and from the target. */
    iSCSIPDUInitiatorBHS bhsBuffer;
	
	/*! Identifies the Mach task (user-space) that opened a connection to this
	 *	client. */
	task_t owningTask;
	
	/*! A security token that identifies user privileges. */
	void * securityToken;
	
	/*! A security type that identifies user privileges. */
	UInt32 type;
    
    /*! The notification port associated with a user-space connection. */
    mach_port_t notificationPort;
};

#endif /* defined(__ISCSI_INITIATOR_CLIENT_H__) */
