/*!
 * @author		Nareg Sinenian
 * @file		iSCSIDaemonInterface.h
 * @version		1.0
 * @copyright	(c) 2013-2015 Nareg Sinenian. All rights reserved.
 * @brief		Defines interface used by client applications to access
 *              the iSCSIDaemon
 */


#include "iSCSIDaemonInterface.h"
#include "iSCSIDaemonInterfaceShared.h"

const struct iSCSIDaemonCmdLoginSession iSCSIDCmdLoginSessionInit  = {
    .funcCode = kiSCSIDLoginSession,
    .portalLength = 0,
    .targetLength = 0,
    .authLength   = 0
};

const struct iSCSIDaemonCmdLogoutSession iSCSIDCmdLogoutSessionInit  = {
    .funcCode = kiSCSIDLogoutSession
};

iSCSIDaemonHandle iSCSIDaemonConnect()
{
    // Create a new socket and connect to the daemon
    iSCSIDaemonHandle handle = socket(PF_LOCAL,SOCK_STREAM,0);
    struct sockaddr_un address;
    address.sun_family = AF_LOCAL;
    strcpy(address.sun_path,"/tmp/iscsid_local");

    // Connect to local socket and return handle
    if(connect(handle,(const struct sockaddr *)&address,(socklen_t)SUN_LEN(&address))==0)
        return handle;
    
    close(handle);
    return -1;
}

void iSCSIDaemonDisconnect(iSCSIDaemonHandle handle)
{
    if(handle >= 0)
        close(handle);
}


/*! Creates a normal iSCSI session and returns a handle to the session. Users
 *  must call iSCSISessionClose to close this session and free resources.
 *  @param handle a handle to a daemon connection.
 *  @param portal specifies the portal to use for the new session.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginSession(iSCSIDaemonHandle handle,
                           iSCSIPortalRef portal,
                           iSCSITargetRef target,
                           iSCSIAuthRef auth,
                           SID * sessionId,
                           CID * connectionId,
                           enum iSCSILoginStatusCode * statusCode)
{
    if(handle < 0 || !portal || !target || !auth || !sessionId || !connectionId || !statusCode)
        return EINVAL;
    
    CFDataRef portalBytes = iSCSIPortalCopyToBytes(portal);
    CFDataRef targetBytes = iSCSITargetCopyToBytes(target);
    CFDataRef authBytes   = iSCSIAuthCopyToBytes(auth);
    
    struct iSCSIDaemonCmdLoginSession cmd = iSCSIDCmdLoginSessionInit;
    cmd.portalLength = CFDataGetLength(portalBytes);
    cmd.targetLength = CFDataGetLength(targetBytes);
    cmd.authLength = CFDataGetLength(authBytes);

    // Send data over the UNIX socket
    struct msghdr msg;
    struct iovec  iovec[5];
    memset(&msg,0,sizeof(struct msghdr));
    
    msg.msg_iov = iovec;
    unsigned int iovecCnt = 0;
    
    iovec[iovecCnt].iov_base = (void *)&cmd;
    iovec[iovecCnt].iov_len  = sizeof(cmd);
    iovecCnt++;
    
    iovec[iovecCnt].iov_base = (void *)CFDataGetBytePtr(portalBytes);
    iovec[iovecCnt].iov_len  = cmd.portalLength;
    iovecCnt++;
    
    iovec[iovecCnt].iov_base = (void *)CFDataGetBytePtr(targetBytes);
    iovec[iovecCnt].iov_len  = cmd.targetLength;
    iovecCnt++;
    
    iovec[iovecCnt].iov_base = (void *)CFDataGetBytePtr(authBytes);
    iovec[iovecCnt].iov_len  = cmd.authLength;
    iovecCnt++;
    
    size_t totalLength = sizeof(cmd) + cmd.portalLength + cmd.targetLength + cmd.authLength;
    
    // Update io vector count, send data
    msg.msg_iovlen = iovecCnt;
    size_t bytesSent = sendmsg(handle,&msg,0);
    
    if(bytesSent != totalLength)
        return EAGAIN;
    
    struct iSCSIDaemonRspLoginSession rsp;
    
    if(recv(handle,&(rsp),sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    if(rsp.funcCode != kiSCSIDLoginSession)
        return EAGAIN;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    *sessionId  = rsp.sessionId;
    
    CFRelease(portalBytes);
    CFRelease(targetBytes);
    CFRelease(authBytes);
    
    return 0;
}

/*! Closes the iSCSI connection and frees the session qualifier.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to free.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogoutSession(iSCSIDaemonHandle handle,
                                 SID sessionId,
                                 enum iSCSILogoutStatusCode * statusCode)
{
    if(handle < 0 || sessionId == kiSCSIInvalidSessionId)
        return EINVAL;
    
    struct iSCSIDaemonCmdLogoutSession cmd = iSCSIDCmdLogoutSessionInit;
    cmd.sessionId = sessionId;
    
    if(send(handle,&cmd,sizeof(cmd),0) != sizeof(cmd))
        return EAGAIN;
    
    struct iSCSIDaemonRspLogoutSession rsp;
    
    if(recv(handle,&rsp,sizeof(rsp),0) != sizeof(rsp))
        return EAGAIN;
    
    // At this point we have a valid response, process it
    *statusCode = rsp.statusCode;
    
    return 0;
}

/*! Adds a new connection to an iSCSI session.
 *  @param handle a handle to a daemon connection.
 *  @param portal specifies the portal to use for the connection.
 *  @param target specifies the target and connection parameters to use.
 *  @param auth specifies the authentication parameters to use.
 *  @param sessionId the new session identifier.
 *  @param connectionId the new connection identifier.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLoginConnection(iSCSIDaemonHandle handle,
                                   iSCSIPortalRef portal,
                                   iSCSITargetRef target,
                                   iSCSIAuthRef auth,
                                   SID sessionId,
                                   CID * connectionId,
                                   enum iSCSILoginStatusCode * statusCode)
{
    return 0;
}

/*! Removes a connection from an existing session.
 *  @param handle a handle to a daemon connection.
 *  @param sessionId the session to remove a connection from.
 *  @param connectionId the connection to remove.
 *  @param statusCode iSCSI response code indicating operation status.
 *  @return an error code indicating whether the operation was successful. */
errno_t iSCSIDaemonLogoutConnection(iSCSIDaemonHandle handle,
                                    SID sessionId,
                                    CID connectionId,
                                    enum iSCSILogoutStatusCode * statusCode)
{
    return 0;
}
