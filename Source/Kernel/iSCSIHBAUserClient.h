/*
 * Copyright (c) 2016, Nareg Sinenian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ISCSI_USER_CLIENT_H__
#define __ISCSI_USER_CLIENT_H__

#include <IOKit/IOUserClient.h>

#include "iSCSIKernelClasses.h"
#include "iSCSIHBATypes.h"
#include "iSCSIPDUShared.h"
#include "iSCSIVirtualHBA.h"
#include "iSCSITypesShared.h"

class iSCSIHBAUserClient : public IOUserClient
{
	OSDeclareDefaultStructors(iSCSIHBAUserClient);

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
	static IOReturn OpenInitiator(iSCSIHBAUserClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);

	/*! Dispatched function called from the device interface to this user
	 *	client .*/
	static IOReturn	CloseInitiator(iSCSIHBAUserClient * target,
                                   void * reference,
                                   IOExternalMethodArguments * args);
	
    /*! Dispatched function invoked from user-space to create new session. */
    static IOReturn CreateSession(iSCSIHBAUserClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to release session. */
    static IOReturn ReleaseSession(iSCSIHBAUserClient * target,
                                   void * reference,
                                   IOExternalMethodArguments * args);
        
    static IOReturn SetSessionParameter(iSCSIHBAUserClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);
    
    static IOReturn GetSessionParameter(iSCSIHBAUserClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to create new connection. */
    static IOReturn CreateConnection(iSCSIHBAUserClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to release connection. */
    static IOReturn ReleaseConnection(iSCSIHBAUserClient * target,
                                      void * reference,
                                      IOExternalMethodArguments * args);

    static IOReturn ActivateConnection(iSCSIHBAUserClient * target,
                                       void * reference,
                                       IOExternalMethodArguments * args);

    static IOReturn ActivateAllConnections(iSCSIHBAUserClient * target,
                                           void * reference,
                                           IOExternalMethodArguments * args);

    static IOReturn DeactivateConnection(iSCSIHBAUserClient * target,
                                         void * reference,
                                         IOExternalMethodArguments * args);

    static IOReturn DeactivateAllConnections(iSCSIHBAUserClient * target,
                                             void * reference,
                                             IOExternalMethodArguments * args);

    static IOReturn GetConnection(iSCSIHBAUserClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);
    
    static IOReturn GetNumConnections(iSCSIHBAUserClient * target,
                                      void * reference,
                                      IOExternalMethodArguments * args);
    
    static IOReturn GetSessionIdForTargetIQN(iSCSIHBAUserClient * target,
                                               void * reference,
                                               IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionIdForPortalAddress(iSCSIHBAUserClient * target,
                                            void * reference,
                                            IOExternalMethodArguments * args);
    
    static IOReturn GetSessionIds(iSCSIHBAUserClient * target,
                                  void * reference,
                                  IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionIds(iSCSIHBAUserClient * target,
                                     void * reference,
                                     IOExternalMethodArguments * args);
    
    static IOReturn GetTargetIQNForSessionId(iSCSIHBAUserClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args);
    

    static IOReturn GetPortalAddressForConnectionId(iSCSIHBAUserClient * target,
                                              void * reference,
                                              IOExternalMethodArguments * args);

    
    static IOReturn GetPortalPortForConnectionId(iSCSIHBAUserClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args);
    
    static IOReturn GetHostInterfaceForConnectionId(iSCSIHBAUserClient * target,
                                                    void * reference,
                                                    IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to send data
     *  over an existing, active connection. */
    static IOReturn SendBHS(iSCSIHBAUserClient * target,
                            void * reference,
                            IOExternalMethodArguments * args);

    /*! Dispatched function invoked from user-space to send data
     *  over an existing, active connection. */
    static IOReturn SendData(iSCSIHBAUserClient * target,
                             void * reference,
                             IOExternalMethodArguments * args);
    
    /*! Dispatched function invoked from user-space to receive data
     *  over an existing, active connection, and to retrieve the size of
     *  a user-space buffer that is required to hold the data. */
    static IOReturn RecvBHS(iSCSIHBAUserClient * target,
                            void * reference,
                            IOExternalMethodArguments * args);
    
    /*! Dispatched function invoked from user-space to receive data
     *  over an existing, active connection, and to retrieve the size of
     *  a user-space buffer that is required to hold the data. */
    static IOReturn RecvData(iSCSIHBAUserClient * target,
                             void * reference,
                             IOExternalMethodArguments * args);
    
    static IOReturn SetConnectionParameter(iSCSIHBAUserClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args);
    
    static IOReturn GetConnectionParameter(iSCSIHBAUserClient * target,
                                        void * reference,
                                        IOExternalMethodArguments * args);
    
    static IOReturn GetActiveConnection(iSCSIHBAUserClient * target,
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
    IOReturn sendNotification(iSCSIHBANotificationMessage * message);

    /*! Sends a notification message to the user indicating that an
     *  iSCSI asynchronous event has occured.
     *  @param sessionId the session identifier.
     *  @param connectionId the connection identifier.
     *  @param event the asynchronous event.
     *  @return an error code indicating the result of the operation. */
    IOReturn sendAsyncMessageNotification(SessionIdentifier sessionId,
                                          ConnectionIdentifier connectionId,
                                          enum iSCSIPDUAsyncMsgEvent event);
    
    /*! Notifies clients that a network connnectivity issue has
     *  caused the specified connection and session to be dropped.
     *  @param sessionId session identifier.
     *  @param connectionId conncetion identifier. 
     *  @return an error code indicating the result of the operation. */
    IOReturn sendTimeoutMessageNotification(SessionIdentifier sessionId,ConnectionIdentifier connectionId);
    
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
    
    /*! Access lock for kernel functions. */
    IOLock * accessLock;
};

#endif /* defined(__ISCSI_USER_CLIENT_H__) */
