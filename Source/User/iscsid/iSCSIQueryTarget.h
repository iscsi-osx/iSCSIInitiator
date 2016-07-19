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

#ifndef __ISCSI_QUERY_TARGET_H__
#define __ISCSI_QUERY_TARGET_H__

#include "iSCSITypes.h"
#include "iSCSIPDUUser.h"
#include "iSCSIHBAInterface.h"

/*! Used to perform a login query during the login phase of a connection. */
struct iSCSILoginQueryContext {
    
    // These inputs are required when calling iSCSISessionLoginQuery()
    
    /*! Reference to the HBA interface. */
    iSCSIHBAInterfaceRef interface;
    
    /*! The session identifier. */
    SessionIdentifier sessionId;
    
    /*! The connection identifier. */
    ConnectionIdentifier connectionId;

    /*! The current stage of negotiation process. */
    enum iSCSIPDULoginStages currentStage;
    
    /*! The next stage of the negotiation process. */
    enum iSCSIPDULoginStages nextStage;

    
    // These are returned by iSCSISessionLoginQuery()
    
    /*! The status sequence number. */
    UInt32 statSN;
    
    /*! The expected command sequence number. */
    UInt32 expCmdSN;
    
    /*! The target session identifier. */
    TargetSessionIdentifier targetSessionId;
    
    /*! Whether the target agrees to advance to next stage. */
    bool transitNextStage;
};

/*! Helper function used throughout the login process to query the target.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate login PDU to the target.  It will then receive one or more
 *  login response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  return the C error code.  If communications are successful but the iSCSI
 *  layer experiences errors, it will return an iSCSI error code, either in the
 *  form of a login status code or a PDU rejection code in addition to
 *  a standard C error code. If the nextStage field of the context struct
 *  specifies the full feature phase, this function will return a valid TSIH.
 *  @param context the context to query (session identifier, etc)
 *  @param statusCode the iSCSI status code returned by the target
 *  @param rejectCode the iSCSI reject code, if the command was rejected
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionLoginQuery(struct iSCSILoginQueryContext * context,
                               enum iSCSILoginStatusCode * statusCode,
                               enum iSCSIPDURejectCode * rejectCode,
                               CFDictionaryRef   textCmd,
                               CFMutableDictionaryRef  textRsp);


/*! Helper function used during the full feature phase of a connection to
 *  send and receive text requests and responses.
 *  This function will take a dictionary of key-value pairs and send the
 *  appropriate text request PDU to the target.  It will then receive one or more
 *  text response PDUs from the target, parse them and return the key-value
 *  pairs received as a dictionary.  If an error occurs, this function will
 *  parse the iSCSI error and express it in terms of a system errno_t as
 *  the system would treat any other device.
 *  @param sessionId the session identifier.
 *  @param connectionId a connection identifier.
 *  @param textCmd a dictionary of key-value pairs to send.
 *  @param textRsp a dictionary of key-value pairs to receive.
 *  @return an error code that indicates the result of the operation. */
errno_t iSCSISessionTextQuery(SessionIdentifier sessionId,
                              ConnectionIdentifier connectionId,
                              CFDictionaryRef   textCmd,
                              CFMutableDictionaryRef  textRsp);


#endif /* defined(__ISCSI_QUERY_TARGET_H__) */
