/**
 * @author		Nareg Sinenian
 * @file		iSCSIIOEventSource.h
 * @date		October 13, 2013
 * @version		1.0
 * @copyright	(c) 2013 Nareg Sinenian. All rights reserved.
 */

#ifndef __ISCSI_IO_EVENT_SOURCE_H__
#define __ISCSI_IO_EVENT_SOURCE_H__

#include <IOKit/IOService.h>
#include <IOKit/IOEventSource.h>

#include <sys/kpi_socket.h>

#include "iSCSIVirtualHBA.h"

#define iSCSIIOEventSource	com_NSinenian_iSCSIIOEventSource

struct iSCSISession;
struct iSCSIConnection;

/** This event source wraps around a network socket and provides a software
 *	interrupt when data becomes available at a the socket. It is used to wake
 *	up the driver's workloop and to process incoming data by using a callback
 *	function (see Action private member). The callback function is executed
 *	within the driver's workloop when the workloop calls the function
 *	checkForWork(). The action member and a socket is specified when this class
 *	is initialized (by calling init()). For the signaling mechanism to work,
 *	the static member socketCallback must be used as the callback function when
 *	your socket is created.  Users of this class must first create a socket
 *	with the static member as the callback, and then instantiate this class
 *	and call init(), passing in the socket. */
class iSCSIIOEventSource : public IOEventSource
{
    OSDeclareDefaultStructors(iSCSIIOEventSource);
    
public:
    
	/** Pointer to the method that is called (within the driver's workloop)
	 *	when data becomes available at a network socket. */
    typedef void (*Action) (iSCSIVirtualHBA::iSCSISession * session,
                            iSCSIVirtualHBA::iSCSIConnection * connection);
	
	/** Initializes the event source with an owner and an action.
	 *	@param owner the owner that this event source will be attached to.
	 *	@param action pointer to a function to call when processing
	 *	interrupts.  This function is called by checkForWork() and executes in
	 *	the owner's workloop.
	 *	@param session the session object.
     *  @param connection the connection object.
	 *	@return true if the event source was successfully initialized. */
	virtual bool init(iSCSIVirtualHBA * owner,
                      iSCSIIOEventSource::Action action,
                      iSCSIVirtualHBA::iSCSISession * session,
                      iSCSIVirtualHBA::iSCSIConnection * connection);

	/** Callback function for BSD sockets. Assign this function as the
	 *	call back when opening a socket using sock_socket(). Note that the
	 *	cookie (see sock_socket() documentation) must be an instance of
	 *	an event source. */
	static void socketCallback(socket_t so,
							   iSCSIIOEventSource * eventSource,
							   int waitf);
	
    
protected:
	
	/** Called by the attached work loop to check if there is any processing
	 *	to be completed.  This fu	nction will call the action method pointed
	 *	to by this object.
	 *	@return true if there was work, false otherwise. */
	virtual bool checkForWork();
		
private:
					  	
    iSCSIVirtualHBA::iSCSISession * session;
    
    iSCSIVirtualHBA::iSCSIConnection * connection;
};

#endif /* defined(__ISCSI_EVENT_SOURCE_H__) */
