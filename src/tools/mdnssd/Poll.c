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

#include "Poll.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include "GenLinkedList.h"
#include "DebugServices.h"


typedef struct PollSource_struct
{
	SOCKET socket;
	HANDLE handle;
	void   *context;

	union
	{
		mDNSPollSocketCallback socket;
		mDNSPollEventCallback event;
	} callback;

	struct Worker_struct		*worker;
	struct PollSource_struct	*next;

} PollSource;


typedef struct Worker_struct
{
	HANDLE					thread;		// NULL for main worker
	unsigned				id;			// 0 for main worker

	HANDLE					start;		// NULL for main worker
	HANDLE					stop;		// NULL for main worker
	BOOL					done;		// Not used for main worker

	DWORD					numSources;
	PollSource				*sources[ MAXIMUM_WAIT_OBJECTS ];
	HANDLE					handles[ MAXIMUM_WAIT_OBJECTS ];
	DWORD					result;
	struct Worker_struct	*next;
} Worker;


typedef struct Poll_struct
{
	mDNSBool		setup;
	HANDLE			wakeup;
	GenLinkedList	sources;
	DWORD			numSources;
	Worker			main;
	GenLinkedList	workers;
	HANDLE			workerHandles[ MAXIMUM_WAIT_OBJECTS ];
	DWORD			numWorkers;

} Poll;


/*
 * Poll Methods
 */

mDNSlocal mStatus			PollSetup();
mDNSlocal mStatus			PollRegisterSource( PollSource *source ); 
mDNSlocal void				PollUnregisterSource( PollSource *source );
mDNSlocal mStatus			PollStartWorkers();
mDNSlocal mStatus			PollStopWorkers();
mDNSlocal void				PollRemoveWorker( Worker *worker );


/*
 * Worker Methods
 */

mDNSlocal mStatus			WorkerInit( Worker *worker );
mDNSlocal void				WorkerFree( Worker *worker );
mDNSlocal void				WorkerRegisterSource( Worker *worker, PollSource *source );
mDNSlocal int				WorkerSourceToIndex( Worker *worker, PollSource *source );
mDNSlocal void				WorkerUnregisterSource( Worker *worker, PollSource *source );
mDNSlocal void				WorkerDispatch( Worker *worker);
mDNSlocal void CALLBACK		WorkerWakeupNotification( HANDLE event, void *context );
mDNSlocal unsigned WINAPI	WorkerMain( LPVOID inParam );


static void
ShiftDown( void * arr, size_t arraySize, size_t itemSize, int index )
{
    memmove( ( ( unsigned char* ) arr ) + ( ( index - 1 ) * itemSize ), ( ( unsigned char* ) arr ) + ( index * itemSize ), ( arraySize - index ) * itemSize );
}


#define	DEBUG_NAME	"[mDNSWin32] "
#define gMDNSRecord mDNSStorage
mDNSlocal Poll gPoll = { mDNSfalse, NULL };

#define LogErr( err, FUNC ) LogMsg( "%s:%d - %s failed: %d\n", __FUNCTION__, __LINE__, FUNC, err );


mStatus
mDNSPollRegisterSocket( SOCKET socket, int networkEvents, mDNSPollSocketCallback callback, void *context )
{
	PollSource	*source = NULL;
	HANDLE		event = INVALID_HANDLE_VALUE;
	mStatus		err = mStatus_NoError;

	if ( !gPoll.setup )
	{
		err = PollSetup();
		require_noerr( err, exit );
	}

	source = malloc( sizeof( PollSource ) );
	require_action( source, exit, err = mStatus_NoMemoryErr );

	event = WSACreateEvent();
	require_action( event, exit, err = mStatus_NoMemoryErr );

	err = WSAEventSelect( socket, event, networkEvents );
	require_noerr( err, exit );

	source->socket = socket;
	source->handle = event;
	source->callback.socket = callback;
	source->context = context;

	err = PollRegisterSource( source );
	require_noerr( err, exit );
	
exit:

	if ( err != mStatus_NoError )
	{
		if ( event != INVALID_HANDLE_VALUE )
		{
			WSACloseEvent( event );
		}

		if ( source != NULL )
		{
			free( source );
		}
	}

	return err;
}


void
mDNSPollUnregisterSocket( SOCKET socket )
{
	PollSource	*source;

	for ( source = gPoll.sources.Head; source; source = source->next )
	{
		if ( source->socket == socket )
		{
			break;
		}
	}

	if ( source )
	{
		WSACloseEvent( source->handle );
		PollUnregisterSource( source );
		free( source );
	}
}


mStatus
mDNSPollRegisterEvent( HANDLE event, mDNSPollEventCallback callback, void *context )
{
	PollSource	*source = NULL;
	mStatus		err = mStatus_NoError;

	if ( !gPoll.setup )
	{
		err = PollSetup();
		require_noerr( err, exit );
	}

	source = malloc( sizeof( PollSource ) );
	require_action( source, exit, err = mStatus_NoMemoryErr );

	source->socket = INVALID_SOCKET;
	source->handle = event;
	source->callback.event = callback;
	source->context = context;

	err = PollRegisterSource( source ); 
	require_noerr( err, exit );
	
exit:

	if ( err != mStatus_NoError )
	{
		if ( source != NULL )
		{
			free( source );
		}
	}

	return err;
}


void
mDNSPollUnregisterEvent( HANDLE event )
{
	PollSource	*source;

	for ( source = gPoll.sources.Head; source; source = source->next )
	{
		if ( source->handle == event )
		{
			break;
		}
	}

	if ( source )
	{
		PollUnregisterSource( source );
		free( source );
	}
}


mStatus
mDNSPoll( DWORD msec )
{
	mStatus err = mStatus_NoError;

	if ( gPoll.numWorkers > 0 )
	{	
		err = PollStartWorkers();
		require_noerr( err, exit );
	}

	gPoll.main.result = WaitForMultipleObjects( gPoll.main.numSources, gPoll.main.handles, FALSE, msec );
	err = translate_errno( ( gPoll.main.result != WAIT_FAILED ), ( mStatus ) GetLastError(), kUnknownErr );
	if ( err ) LogErr( err, "WaitForMultipleObjects()" );
	require_action( gPoll.main.result != WAIT_FAILED, exit, err = ( mStatus ) GetLastError() );

	if ( gPoll.numWorkers > 0 )
	{
		err = PollStopWorkers();
		require_noerr( err, exit );
	}

	WorkerDispatch( &gPoll.main );

exit:

	return ( err );
}


mDNSlocal mStatus
PollSetup()
{
	mStatus err = mStatus_NoError;

	if ( !gPoll.setup )
	{
		memset( &gPoll, 0, sizeof( gPoll ) );

		InitLinkedList( &gPoll.sources, offsetof( PollSource, next ) );
		InitLinkedList( &gPoll.workers, offsetof( Worker, next ) );

		gPoll.wakeup = CreateEvent( NULL, TRUE, FALSE, NULL );
		require_action( gPoll.wakeup, exit, err = mStatus_NoMemoryErr );

		err = WorkerInit( &gPoll.main );
		require_noerr( err, exit );
		
		gPoll.setup = mDNStrue;
	}

exit:

	return err;
}


mDNSlocal mStatus
PollRegisterSource( PollSource *source )
{
	Worker	*worker = NULL;
	mStatus err = mStatus_NoError;

	AddToTail( &gPoll.sources, source );
	gPoll.numSources++;

	// First check our main worker. In most cases, we won't have to worry about threads

	if ( gPoll.main.numSources < MAXIMUM_WAIT_OBJECTS )
	{
		WorkerRegisterSource( &gPoll.main, source );
	}
	else
	{
		// Try to find a thread to use that we've already created

		for ( worker = gPoll.workers.Head; worker; worker = worker->next )
		{
			if ( worker->numSources < MAXIMUM_WAIT_OBJECTS )
			{
				WorkerRegisterSource( worker, source );
				break;
			}
		}

		// If not, then create a worker and make a thread to run it in
		
		if ( !worker )
		{
			worker = ( Worker* ) malloc( sizeof( Worker ) );			
			require_action( worker, exit, err = mStatus_NoMemoryErr );

			memset( worker, 0, sizeof( Worker ) );

			worker->start = CreateEvent( NULL, FALSE, FALSE, NULL );
			require_action( worker->start, exit, err = mStatus_NoMemoryErr );

			worker->stop = CreateEvent( NULL, FALSE, FALSE, NULL );
			require_action( worker->stop, exit, err = mStatus_NoMemoryErr );

			err = WorkerInit( worker );
			require_noerr( err, exit );

			// Create thread with _beginthreadex() instead of CreateThread() to avoid
			// memory leaks when using static run-time libraries.
			// See <http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/createthread.asp>.

			worker->thread = ( HANDLE ) _beginthreadex_compat( NULL, 0, WorkerMain, worker, 0, &worker->id );
			err = translate_errno( worker->thread, ( mStatus ) GetLastError(), kUnknownErr );
			require_noerr( err, exit );

			AddToTail( &gPoll.workers, worker );
			gPoll.workerHandles[ gPoll.numWorkers++ ] = worker->stop;
			
			WorkerRegisterSource( worker, source );
		}
	}

exit:

	if ( err && worker )
	{
		WorkerFree( worker );
	}

	return err;
}


mDNSlocal void
PollUnregisterSource( PollSource *source )
{
	RemoveFromList( &gPoll.sources, source );
	gPoll.numSources--;

	WorkerUnregisterSource( source->worker, source );
}


mDNSlocal mStatus
PollStartWorkers()
{
	Worker	*worker;
	mStatus	err = mStatus_NoError;
	BOOL	ok;

	dlog( kDebugLevelChatty, DEBUG_NAME "starting workers\n" );
	
	worker = gPoll.workers.Head;

	while ( worker )
	{
		Worker *next = worker->next;

		if ( worker->numSources == 1 )
		{
			PollRemoveWorker( worker );
		}
		else
		{
			dlog( kDebugLevelChatty, DEBUG_NAME "waking up worker\n" );

			ok = SetEvent( worker->start );
			err = translate_errno( ok, ( mStatus ) GetLastError(), kUnknownErr );
			if ( err ) LogErr( err, "SetEvent()" );

			if ( err )
			{
				PollRemoveWorker( worker );
			}
		}

		worker = next;
	}

	err = mStatus_NoError;

	return err;
}


mDNSlocal mStatus
PollStopWorkers()
{
	DWORD	result;
	Worker	*worker;
	BOOL	ok;
	mStatus	err = mStatus_NoError;

	dlog( kDebugLevelChatty, DEBUG_NAME "stopping workers\n" );
	
	ok = SetEvent( gPoll.wakeup );
	err = translate_errno( ok, ( mStatus ) GetLastError(), kUnknownErr );
	if ( err ) LogErr( err, "SetEvent()" );

	// Wait For 5 seconds for all the workers to wake up

	result = WaitForMultipleObjects( gPoll.numWorkers, gPoll.workerHandles, TRUE, 5000 );
	err = translate_errno( ( result != WAIT_FAILED ), ( mStatus ) GetLastError(), kUnknownErr );
	if ( err ) LogErr( err, "WaitForMultipleObjects()" );

	ok = ResetEvent( gPoll.wakeup );
	err = translate_errno( ok, ( mStatus ) GetLastError(), kUnknownErr );
	if ( err ) LogErr( err, "ResetEvent()" );

	for ( worker = gPoll.workers.Head; worker; worker = worker->next )
	{
		WorkerDispatch( worker );
	}

	err = mStatus_NoError;

	return err;
}


mDNSlocal void
PollRemoveWorker( Worker *worker )
{
	DWORD	result;
	mStatus	err;
	BOOL	ok;
	DWORD	i;

	dlog( kDebugLevelChatty, DEBUG_NAME "removing worker %d\n", worker->id );
	
	RemoveFromList( &gPoll.workers, worker );

	// Remove handle from gPoll.workerHandles

	for ( i = 0; i < gPoll.numWorkers; i++ )
	{
		if ( gPoll.workerHandles[ i ] == worker->stop )
		{
			ShiftDown( gPoll.workerHandles, gPoll.numWorkers, sizeof( gPoll.workerHandles[ 0 ] ), i + 1 );
			break;
		}
	}

	worker->done = TRUE;
	gPoll.numWorkers--;

	// Cause the thread to exit.

	ok = SetEvent( worker->start );
	err = translate_errno( ok, ( OSStatus ) GetLastError(), kUnknownErr );
	if ( err ) LogErr( err, "SetEvent()" );
			
	result = WaitForSingleObject( worker->thread, 5000 );
	err = translate_errno( result != WAIT_FAILED, ( OSStatus ) GetLastError(), kUnknownErr );
	if ( err ) LogErr( err, "WaitForSingleObject()" );
			
	if ( ( result == WAIT_FAILED ) || ( result == WAIT_TIMEOUT ) )
	{
		ok = TerminateThread( worker->thread, 0 );
		err = translate_errno( ok, ( OSStatus ) GetLastError(), kUnknownErr );
		if ( err ) LogErr( err, "TerminateThread()" );
	}
	
	CloseHandle( worker->thread );
	worker->thread = NULL;

	WorkerFree( worker );
}


mDNSlocal void
WorkerRegisterSource( Worker *worker, PollSource *source )
{
	source->worker = worker;
	worker->sources[ worker->numSources ] = source;
	worker->handles[ worker->numSources ] = source->handle;
	worker->numSources++;
}


mDNSlocal int
WorkerSourceToIndex( Worker *worker, PollSource *source )
{
	int index;

	for ( index = 0; index < ( int ) worker->numSources; index++ )
	{
		if ( worker->sources[ index ] == source )
		{
			break;
		}
	}

	if ( index == ( int ) worker->numSources )
	{
		index = -1;
	}

	return index;
}


mDNSlocal void
WorkerUnregisterSource( Worker *worker, PollSource *source )
{
	int sourceIndex = WorkerSourceToIndex( worker, source );
	DWORD delta;

	if ( sourceIndex == -1 )
	{
		LogMsg( "WorkerUnregisterSource: source not found in list" );
		goto exit;
	}

	delta = ( worker->numSources - sourceIndex - 1 );

	// If this source is not at the end of the list, then move memory

	if ( delta > 0 )
	{
		ShiftDown( worker->sources, worker->numSources, sizeof( worker->sources[ 0 ] ), sourceIndex + 1 );
		ShiftDown( worker->handles, worker->numSources, sizeof( worker->handles[ 0 ] ), sourceIndex + 1 );
	}
		         
	worker->numSources--;

exit:

	return;
}


mDNSlocal void CALLBACK
WorkerWakeupNotification( HANDLE event, void *context )
{
	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	dlog( kDebugLevelChatty, DEBUG_NAME "Worker thread wakeup\n" );
}


mDNSlocal void
WorkerDispatch( Worker *worker )
{
	if ( worker->result == WAIT_FAILED )
	{
		/* What should we do here? */
	}
	else if ( worker->result == WAIT_TIMEOUT )
	{
		dlog( kDebugLevelChatty, DEBUG_NAME "timeout\n" );
	}
	else
	{
		DWORD		waitItemIndex = ( DWORD )( ( ( int ) worker->result ) - WAIT_OBJECT_0 );
		PollSource	*source = NULL;

		// Sanity check

		if ( waitItemIndex >= worker->numSources )
		{
			LogMsg( "WorkerDispatch: waitItemIndex (%d) is >= numSources (%d)", waitItemIndex, worker->numSources );
			goto exit;
		}

		source = worker->sources[ waitItemIndex ];

		if ( source->socket != INVALID_SOCKET )
		{
			WSANETWORKEVENTS event;
	
			if ( WSAEnumNetworkEvents( source->socket, source->handle, &event ) == 0 )
			{
				source->callback.socket( source->socket, &event, source->context );
			}
			else
			{
				source->callback.socket( source->socket, NULL, source->context );
			}
		}
		else
		{
			source->callback.event( source->handle, source->context );
		}
	}

exit:

	return;
}


mDNSlocal mStatus
WorkerInit( Worker *worker )
{
	PollSource *source = NULL;
	mStatus err = mStatus_NoError;
	
	require_action( worker, exit, err = mStatus_BadParamErr );

	source = malloc( sizeof( PollSource ) );
	require_action( source, exit, err = mStatus_NoMemoryErr );

	source->socket = INVALID_SOCKET;
	source->handle = gPoll.wakeup;
	source->callback.event = WorkerWakeupNotification;
	source->context = NULL;
	
	WorkerRegisterSource( worker, source );

exit:

	return err;
}
	

mDNSlocal void
WorkerFree( Worker *worker )
{
	if ( worker->start )
	{
		CloseHandle( worker->start );
		worker->start = NULL;
	}

	if ( worker->stop )
	{
		CloseHandle( worker->stop );
		worker->stop = NULL;
	}

	free( worker );
}


mDNSlocal unsigned WINAPI
WorkerMain( LPVOID inParam )
{
	Worker *worker = ( Worker* ) inParam;
	mStatus err = mStatus_NoError;

	require_action( worker, exit, err = mStatus_BadParamErr );

	dlog( kDebugLevelVerbose, DEBUG_NAME, "entering WorkerMain()\n" );

	while ( TRUE )
	{
		DWORD	result;
		BOOL	ok;

		dlog( kDebugLevelChatty, DEBUG_NAME, "worker thread %d will wait on main loop\n", worker->id );
		
		result = WaitForSingleObject( worker->start, INFINITE );	
		err = translate_errno( ( result != WAIT_FAILED ), ( mStatus ) GetLastError(), kUnknownErr );
		if ( err ) { LogErr( err, "WaitForSingleObject()" ); break; }
		if ( worker->done ) break;

		dlog( kDebugLevelChatty, DEBUG_NAME "worker thread %d will wait on sockets\n", worker->id );

		worker->result = WaitForMultipleObjects( worker->numSources, worker->handles, FALSE, INFINITE );
		err = translate_errno( ( worker->result != WAIT_FAILED ), ( mStatus ) GetLastError(), kUnknownErr );
		if ( err ) { LogErr( err, "WaitForMultipleObjects()" ); break; }

		dlog( kDebugLevelChatty, DEBUG_NAME "worker thread %d did wait on sockets: %d\n", worker->id, worker->result );

		ok = SetEvent( gPoll.wakeup );
		err = translate_errno( ok, ( mStatus ) GetLastError(), kUnknownErr );
		if ( err ) { LogErr( err, "SetEvent()" ); break; }

		dlog( kDebugLevelChatty, DEBUG_NAME, "worker thread %d preparing to sleep\n", worker->id );

		ok = SetEvent( worker->stop );
		err = translate_errno( ok, ( mStatus ) GetLastError(), kUnknownErr );
		if ( err ) { LogErr( err, "SetEvent()" ); break; }
	}

	dlog( kDebugLevelVerbose, DEBUG_NAME "exiting WorkerMain()\n" );

exit:

	return 0;
}
