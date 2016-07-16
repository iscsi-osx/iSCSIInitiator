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

#ifndef __ISCSI_KERNEL_CLASSES_H__
#define __ISCSI_KERNEL_CLASSES_H__

#define NAME_CONCAT(PREFIX, SUFFIX) PREFIX##_##SUFFIX
#define NAME_EVAL(PREFIX,SUFFIX)    NAME_CONCAT(PREFIX,SUFFIX)
#define ADD_PREFIX(NAME)            NAME_EVAL(NAME_PREFIX_U,NAME)

#define STRINGIFY(NAME)             #NAME
#define STRINGIFY_EVAL(NAME)        STRINGIFY(NAME)

#define iSCSIVirtualHBA         ADD_PREFIX(iSCSIVirtualHBA)
#define iSCSITaskQueue          ADD_PREFIX(iSCSITaskQueue)
#define iSCSIIOEventSource      ADD_PREFIX(iSCSIIOEventSource)
#define iSCSIHBAUserClient      ADD_PREFIX(iSCSIHBAUserClient)
#define iSCSIInitiator          ADD_PREFIX(iSCSIInitiator)

#define kiSCSIVirtualHBA_IOClassName STRINGIFY_EVAL(iSCSIVirtualHBA)

#endif
