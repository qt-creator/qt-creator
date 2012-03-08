/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2002-2004 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _Poll_h
#define _Poll_h

#include	"CommonServices.h"
#include	<mswsock.h>
#include	"mDNSEmbeddedAPI.h"
#include	"uDNS.h"


#if defined(__cplusplus )
extern "C" {
#endif


typedef void ( CALLBACK *mDNSPollSocketCallback )( SOCKET socket, LPWSANETWORKEVENTS event, void *context );
typedef void ( CALLBACK *mDNSPollEventCallback )( HANDLE event, void *context );


extern mStatus
mDNSPollRegisterSocket( SOCKET socket, int networkEvents, mDNSPollSocketCallback callback, void *context );


extern void
mDNSPollUnregisterSocket( SOCKET socket );


extern mStatus
mDNSPollRegisterEvent( HANDLE event, mDNSPollEventCallback callback, void *context );


extern void
mDNSPollUnregisterEvent( HANDLE event );


extern mStatus
mDNSPoll( DWORD msec );


#if defined(__cplusplus)
}
#endif


#endif
