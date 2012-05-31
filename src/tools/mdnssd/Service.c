/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2003-2004 Apple Computer, Inc. All rights reserved.
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

#include	<stdio.h>
#include	<stdlib.h>
#include	<crtdbg.h>
#include	<stdarg.h>
#include	<stddef.h>

#include	"Poll.h"
#include	"CommonServices.h"
#include	"DebugServices.h"
#include	"RegNames.h"

#include	"uds_daemon.h"
#include	"GenLinkedList.h"
#include	"Service.h"
#include	"EventLog.h"

#include	"Resource.h"

#include	"mDNSEmbeddedAPI.h"
#include	"uDNS.h"
#include	"mDNSWin32.h"
#include	"mDNSDebug.h"

#include	"Firewall.h"

#if( !TARGET_OS_WINDOWS_CE )
	#include	<mswsock.h>
	#include	<process.h>
	#include	<ipExport.h>
	#include	<ws2def.h>
	#include	<ws2ipdef.h>
	#include	<iphlpapi.h>
	#include	<netioapi.h>
	#include	<iptypes.h>
	#include	<powrprof.h>
#endif

#ifndef HeapEnableTerminationOnCorruption
#	define HeapEnableTerminationOnCorruption (HEAP_INFORMATION_CLASS)1
#endif

#if 0
#pragma mark == Constants ==
#endif

//===========================================================================================================================
//	Constants
//===========================================================================================================================

#define	DEBUG_NAME							"[mDNSWin32] "
#define kServiceFirewallName				L"Bonjour"
#define	kServiceDependencies				TEXT("Tcpip\0\0")
#define	kDNSServiceCacheEntryCountDefault	512
#define kRetryFirewallPeriod				30 * 1000
#define kDefValueSize						MAX_PATH + 1
#define kZeroIndex							0
#define kDefaultRouteMetric					399
#define kSecondsTo100NSUnits				( 10 * 1000 * 1000 )
#define kSPSMaintenanceWakePeriod			-30
#define kWaitToRetry						(60 * 5)

#define RR_CACHE_SIZE 500
static CacheEntity gRRCache[RR_CACHE_SIZE];
#if 0
#pragma mark == Structures ==
#endif

#if 0
#pragma mark == Prototypes ==
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================
static void				Usage( void );
static BOOL WINAPI		ConsoleControlHandler( DWORD inControlEvent );
static OSStatus			InstallService( LPCTSTR inName, LPCTSTR inDisplayName, LPCTSTR inDescription, LPCTSTR inPath );
static OSStatus			RemoveService( LPCTSTR inName );
static OSStatus			SetServiceParameters();
static OSStatus			GetServiceParameters();
static OSStatus			CheckFirewall();
static OSStatus			SetServiceInfo( SC_HANDLE inSCM, LPCTSTR inServiceName, LPCTSTR inDescription );
static void				ReportStatus( int inType, const char *inFormat, ... );

static void WINAPI		ServiceMain( DWORD argc, LPTSTR argv[] );
static OSStatus			ServiceSetupEventLogging( void );
static DWORD WINAPI		ServiceControlHandler( DWORD inControl, DWORD inEventType, LPVOID inEventData, LPVOID inContext );

static OSStatus			ServiceRun( int argc, LPTSTR argv[] );
static void				ServiceStop( void );

static OSStatus			ServiceSpecificInitialize( int argc, LPTSTR  argv[] );
static OSStatus			ServiceSpecificRun( int argc, LPTSTR argv[] );
static OSStatus			ServiceSpecificStop( void );
static void				ServiceSpecificFinalize( int argc, LPTSTR argv[] );
static mStatus			SetupServiceEvents();
static mStatus			TearDownServiceEvents();
static mStatus			SetupNotifications();
static mStatus			TearDownNotifications();
static void CALLBACK	StopNotification( HANDLE event, void * context );
static void CALLBACK	PowerSuspendNotification( HANDLE event, void * context );
static void	CALLBACK	PowerResumeNotification( HANDLE event, void * context );
static void CALLBACK	InterfaceListNotification( SOCKET socket, LPWSANETWORKEVENTS event, void *context );
static void CALLBACK	ComputerDescriptionNotification( HANDLE event, void *context );
static void CALLBACK	TCPChangedNotification( HANDLE event, void *context );
static void CALLBACK	TCPChangedNotification2( HANDLE event, void *context );
static void CALLBACK	DDNSChangedNotification( HANDLE event, void *context );
static void CALLBACK	FileSharingChangedNotification( HANDLE event, void *context );
static void CALLBACK	FirewallChangedNotification( HANDLE event, void *context );
static void CALLBACK	AdvertisedServicesChangedNotification( HANDLE event, void *context );
static void CALLBACK	SPSWakeupNotification( HANDLE event, void *context );
static void	CALLBACK	SPSSleepNotification( HANDLE event, void *context );
static void CALLBACK	UDSAcceptNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context );
static void CALLBACK	UDSReadNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context );
static void				CoreCallback(mDNS * const inMDNS, mStatus result);
static mDNSu8			SystemWakeForNetworkAccess( LARGE_INTEGER * timeout );
static OSStatus			GetRouteDestination(DWORD * ifIndex, DWORD * address);
static OSStatus			SetLLRoute( mDNS * const inMDNS );
static bool				HaveRoute( PMIB_IPFORWARDROW rowExtant, unsigned long addr, unsigned long metric );
static bool				IsValidAddress( const char * addr );
static bool				IsNortelVPN( IP_ADAPTER_INFO * pAdapter );
static bool				IsJuniperVPN( IP_ADAPTER_INFO * pAdapter );
static bool				IsCiscoVPN( IP_ADAPTER_INFO * pAdapter );
static const char *		strnistr( const char * string, const char * subString, size_t max );

#if defined(UNICODE)
#	define StrLen(X)	wcslen(X)
#	define StrCmp(X,Y)	wcscmp(X,Y)
#else
#	define StrLen(X)	strlen(X)
#	define StrCmp(X,Y)	strcmp(X,Y)
#endif


#define kLLNetworkAddr      "169.254.0.0"
#define kLLNetworkAddrMask  "255.255.0.0"


#include	"mDNSEmbeddedAPI.h"

#if 0
#pragma mark == Globals ==
#endif

//===========================================================================================================================
//	Globals
//===========================================================================================================================
#define gMDNSRecord mDNSStorage
DEBUG_LOCAL	mDNS_PlatformSupport		gPlatformStorage;
DEBUG_LOCAL BOOL						gServiceQuietMode		= FALSE;
DEBUG_LOCAL SERVICE_TABLE_ENTRY			gServiceDispatchTable[] = 
{
	{ kServiceName,	ServiceMain }, 
	{ NULL, 		NULL }
};
DEBUG_LOCAL HANDLE						gStopEvent					= NULL;
DEBUG_LOCAL HANDLE						gPowerSuspendEvent			= NULL;
DEBUG_LOCAL HANDLE						gPowerSuspendAckEvent		= NULL;
DEBUG_LOCAL HANDLE						gPowerResumeEvent			= NULL;
DEBUG_LOCAL SOCKET						gInterfaceListChangedSocket	= INVALID_SOCKET;
DEBUG_LOCAL HKEY						gDescKey					= NULL;
DEBUG_LOCAL HANDLE						gDescChangedEvent			= NULL;	// Computer description changed event
DEBUG_LOCAL HKEY						gTcpipKey					= NULL;
DEBUG_LOCAL HANDLE						gTcpipChangedEvent			= NULL;	// TCP/IP config changed
DEBUG_LOCAL HANDLE						gTcpipChangedEvent2Handle	= NULL;	// handle for overlapped comm of gTcpipChangedEvent2
DEBUG_LOCAL OVERLAPPED					gTcpipChangedEvent2;                // TCP/IP config changed (using NotiyAddrChanges)
DEBUG_LOCAL HKEY						gDdnsKey					= NULL;
DEBUG_LOCAL HANDLE						gDdnsChangedEvent			= NULL;	// DynDNS config changed
DEBUG_LOCAL HKEY						gFileSharingKey				= NULL;
DEBUG_LOCAL HANDLE						gFileSharingChangedEvent	= NULL;	// File Sharing changed
DEBUG_LOCAL HKEY						gFirewallKey				= NULL;
DEBUG_LOCAL HANDLE						gFirewallChangedEvent		= NULL;	// Firewall changed
DEBUG_LOCAL HKEY						gAdvertisedServicesKey		= NULL;
DEBUG_LOCAL HANDLE						gAdvertisedServicesChangedEvent	= NULL; // Advertised services changed
DEBUG_LOCAL SERVICE_STATUS				gServiceStatus;
DEBUG_LOCAL SERVICE_STATUS_HANDLE		gServiceStatusHandle 	= NULL;
DEBUG_LOCAL HANDLE						gServiceEventSource		= NULL;
DEBUG_LOCAL bool						gServiceAllowRemote		= false;
DEBUG_LOCAL int							gServiceCacheEntryCount	= 0;	// 0 means to use the DNS-SD default.
DEBUG_LOCAL bool						gServiceManageLLRouting = true;
DEBUG_LOCAL HANDLE						gSPSWakeupEvent			= NULL;
DEBUG_LOCAL HANDLE						gSPSSleepEvent			= NULL;
DEBUG_LOCAL SocketRef					gUDSSocket				= 0;
DEBUG_LOCAL udsEventCallback			gUDSCallback			= NULL;
DEBUG_LOCAL BOOL						gRetryFirewall			= FALSE;
DEBUG_LOCAL BOOL						gJustCheckFirewall  	= FALSE;

typedef DWORD ( WINAPI * GetIpInterfaceEntryFunctionPtr )( PMIB_IPINTERFACE_ROW );
mDNSlocal HMODULE								gIPHelperLibraryInstance		= NULL;
mDNSlocal GetIpInterfaceEntryFunctionPtr		gGetIpInterfaceEntryFunctionPtr	= NULL;

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	Main
//===========================================================================================================================
int	Main( int argc, LPTSTR argv[] )
{
	OSStatus		err;
//	BOOL			ok;
	BOOL			start;
	int				i;

	HeapSetInformation( NULL, HeapEnableTerminationOnCorruption, NULL, 0 );

	debug_initialize( kDebugOutputTypeMetaConsole );
	debug_set_property( kDebugPropertyTagPrintLevel, kDebugLevelVerbose );

	// Default to automatically starting the service dispatcher if no extra arguments are specified.
	
	start = ( argc <= 1 );
	
	// Parse arguments.
	
	for( i = 1; i < argc; ++i )
	{
        if( StrCmp( argv[ i ], TEXT("-check-firewall") ) == 0 )			// Check firewall
        {
            gJustCheckFirewall = TRUE;
            start = TRUE;
            break;
        } else
        if( StrCmp( argv[ i ], TEXT("-start") ) == 0 )		// Start
		{
			start = TRUE;
		}
        else if( StrCmp( argv[ i ], TEXT("-server") ) == 0 )		// Server
		{
			err = RunDirect( argc, argv );
			if( err )
			{
				ReportStatus( EVENTLOG_ERROR_TYPE, "run service directly failed (%d)\n", err );
			}
			goto exit;
        }
        else if( StrCmp( argv[ i ], TEXT("-q") ) == 0 )			// Quiet Mode (toggle)
		{
			gServiceQuietMode = !gServiceQuietMode;
		}
		else if( ( StrCmp( argv[ i ], TEXT("-help") ) == 0 ) || 	// Help
				 ( StrCmp( argv[ i ], TEXT("-h") ) == 0 ) )
		{
			Usage();
			err = 0;
			break;
		}
		else
		{
			Usage();
			err = kParamErr;
			break;
		}
	}
	
	// Start the service dispatcher if requested. This does not return until all services have terminated. If any 
	// global initialization is needed, it should be done before starting the service dispatcher, but only if it 
	// will take less than 30 seconds. Otherwise, use a separate thread for it and start the dispatcher immediately.
	
	if( start )
	{
		ServiceMain( argc, argv );
		//ok = StartServiceCtrlDispatcher( gServiceDispatchTable );
		//err = translate_errno( ok, (OSStatus) GetLastError(), kInUseErr );
		//if( err != kNoErr )
		//{
		//	ReportStatus( EVENTLOG_ERROR_TYPE, "start service dispatcher failed (%d)\n", err );
		//	goto exit;
		//}
	}
	err = 0;
	
exit:
	dlog( kDebugLevelTrace, DEBUG_NAME "exited (%d %m)\n", err, err );
	_CrtDumpMemoryLeaks();
	return( (int) err );
}

//===========================================================================================================================
//	Usage
//===========================================================================================================================

static void	Usage( void )
{
	fprintf( stderr, "\n" );
    fprintf( stderr, "mdnsd 1.0d1\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "    <no args>    Runs the service normally\n" );
	fprintf( stderr, "    -start       Starts the service dispatcher after processing all other arguments\n" );
	fprintf( stderr, "    -server      Runs the service directly as a server (for debugging)\n" );
	fprintf( stderr, "    -q           Toggles Quiet Mode (no events or output)\n" );
	fprintf( stderr, "    -h[elp]      Display Help/Usage\n" );
	fprintf( stderr, "\n" );
}

//===========================================================================================================================
//	ConsoleControlHandler
//===========================================================================================================================

static BOOL WINAPI	ConsoleControlHandler( DWORD inControlEvent )
{
	BOOL			handled;
	OSStatus		err;
	
	handled = FALSE;
	switch( inControlEvent )
	{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			err = ServiceSpecificStop();
			require_noerr( err, exit );
			
			handled = TRUE;
			break;
		
		default:
			break;
	}
	
exit:
	return( handled );
}

//===========================================================================================================================
//	InstallService
//===========================================================================================================================

static OSStatus	InstallService( LPCTSTR inName, LPCTSTR inDisplayName, LPCTSTR inDescription, LPCTSTR inPath )
{
	OSStatus		err;
	SC_HANDLE		scm;
	SC_HANDLE		service;
	BOOL			ok;
	TCHAR			fullPath[ MAX_PATH ];
	TCHAR *			namePtr;
	DWORD			size;
	
	scm		= NULL;
	service = NULL;
	
	// Get a full path to the executable since a relative path may have been specified.
	
	size = GetFullPathName( inPath, MAX_PATH, fullPath, &namePtr );
	err = translate_errno( size > 0, (OSStatus) GetLastError(), kPathErr );
	require_noerr( err, exit );
	
	// Create the service and start it.
	
	scm = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	err = translate_errno( scm, (OSStatus) GetLastError(), kOpenErr );
	require_noerr( err, exit );
	
	service = CreateService( scm, inName, inDisplayName, SERVICE_ALL_ACCESS, SERVICE_WIN32_SHARE_PROCESS, 
							 SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, fullPath, NULL, NULL, kServiceDependencies, 
							 NULL, NULL );
	err = translate_errno( service, (OSStatus) GetLastError(), kDuplicateErr );
	require_noerr( err, exit );

	err = SetServiceParameters();
	check_noerr( err );
	
	if( inDescription )
	{
		err = SetServiceInfo( scm, inName, inDescription );
		check_noerr( err );
	}

	ok = StartService( service, 0, NULL );
	err = translate_errno( ok, (OSStatus) GetLastError(), kInUseErr );
	require_noerr( err, exit );
	
	ReportStatus( EVENTLOG_SUCCESS, "installed service\n" );
	err = kNoErr;
	
exit:
	if( service )
	{
		CloseServiceHandle( service );
	}
	if( scm )
	{
		CloseServiceHandle( scm );
	}
	return( err );
}

//===========================================================================================================================
//	RemoveService
//===========================================================================================================================

static OSStatus	RemoveService( LPCTSTR inName )
{
	OSStatus			err;
	SC_HANDLE			scm;
	SC_HANDLE			service;
	BOOL				ok;
	SERVICE_STATUS		status;
	
	scm		= NULL;
	service = NULL;
	
	// Open a connection to the service.
	
	scm = OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );
	err = translate_errno( scm, (OSStatus) GetLastError(), kOpenErr );
	require_noerr( err, exit );
	
	service = OpenService( scm, inName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE );
	err = translate_errno( service, (OSStatus) GetLastError(), kNotFoundErr );
	require_noerr( err, exit );
	
	// Stop the service, if it is not already stopped, then delete it.
	
	ok = QueryServiceStatus( service, &status );
	err = translate_errno( ok, (OSStatus) GetLastError(), kAuthenticationErr );
	require_noerr( err, exit );
	
	if( status.dwCurrentState != SERVICE_STOPPED )
	{
		ok = ControlService( service, SERVICE_CONTROL_STOP, &status );
		check_translated_errno( ok, (OSStatus) GetLastError(), kAuthenticationErr );
	}
	
	ok = DeleteService( service );
	err = translate_errno( ok, (OSStatus) GetLastError(), kDeletedErr );
	require_noerr( err, exit );
		
	ReportStatus( EVENTLOG_SUCCESS, "Removed service\n" );
	err = ERROR_SUCCESS;
	
exit:
	if( service )
	{
		CloseServiceHandle( service );
	}
	if( scm )
	{
		CloseServiceHandle( scm );
	}
	return( err );
}



//===========================================================================================================================
//	SetServiceParameters
//===========================================================================================================================

static OSStatus SetServiceParameters()
{
	DWORD 			value;
	DWORD			valueLen = sizeof(DWORD);
	DWORD			type;
	OSStatus		err;
	HKEY			key;

	key = NULL;

	//
	// Add/Open Parameters section under service entry in registry
	//
    err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode, &key );
	require_noerr( err, exit );
	
	//
	// If the value isn't already there, then we create it
	//
	err = RegQueryValueEx(key, kServiceManageLLRouting, 0, &type, (LPBYTE) &value, &valueLen);

	if (err != ERROR_SUCCESS)
	{
		value = 1;

		err = RegSetValueEx( key, kServiceManageLLRouting, 0, REG_DWORD, (const LPBYTE) &value, sizeof(DWORD) );
		require_noerr( err, exit );
	}

exit:

	if ( key )
	{
		RegCloseKey( key );
	}

	return( err );
}



//===========================================================================================================================
//	GetServiceParameters
//===========================================================================================================================

static OSStatus GetServiceParameters()
{
	DWORD 			value;
	DWORD			valueLen;
	DWORD			type;
	OSStatus		err;
	HKEY			key;

	key = NULL;

	//
	// Add/Open Parameters section under service entry in registry
	//
    err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode, &key );
	require_noerr( err, exit );
	
	valueLen = sizeof(DWORD);
	err = RegQueryValueEx(key, kServiceManageLLRouting, 0, &type, (LPBYTE) &value, &valueLen);
	if (err == ERROR_SUCCESS)
	{
		gServiceManageLLRouting = (value) ? true : false;
	}

	valueLen = sizeof(DWORD);
	err = RegQueryValueEx(key, kServiceCacheEntryCount, 0, &type, (LPBYTE) &value, &valueLen);
	if (err == ERROR_SUCCESS)
	{
		gServiceCacheEntryCount = value;
	}

exit:

	if ( key )
	{
		RegCloseKey( key );
	}

	return( err );
}


//===========================================================================================================================
//	CheckFirewall
//===========================================================================================================================

static OSStatus CheckFirewall()
{
	DWORD 					value;
	DWORD					valueLen;
	DWORD					type;
	ENUM_SERVICE_STATUS	*	lpService = NULL;
	SC_HANDLE				sc = NULL;
	HKEY					key = NULL;
	BOOL					ok;
	DWORD					bytesNeeded = 0;
	DWORD					srvCount;
	DWORD					resumeHandle = 0;
	DWORD					srvType;
	DWORD					srvState;
	DWORD					dwBytes = 0;
	DWORD					i;
	BOOL					isRunning = FALSE;
	OSStatus				err = kUnknownErr;
	
	// Check to see if the firewall service is running.  If it isn't, then
	// we want to return immediately

	sc = OpenSCManager( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE );
	err = translate_errno( sc, GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	srvType		=	SERVICE_WIN32;
	srvState	=	SERVICE_STATE_ALL;

	for ( ;; )
	{
		// Call EnumServicesStatus using the handle returned by OpenSCManager

		ok = EnumServicesStatus ( sc, srvType, srvState, lpService, dwBytes, &bytesNeeded, &srvCount, &resumeHandle );

		if ( ok || ( GetLastError() != ERROR_MORE_DATA ) )
		{
			break;
		}

		if ( lpService )
		{
			free( lpService );
		}

		dwBytes = bytesNeeded;

		lpService = ( ENUM_SERVICE_STATUS* ) malloc( dwBytes );
		require_action( lpService, exit, err = mStatus_NoMemoryErr );
	}

	err = translate_errno( ok, GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	for ( i = 0; i < srvCount; i++ )
	{
		if ( wcscmp( lpService[i].lpServiceName, L"SharedAccess" ) == 0 )
		{
			if ( lpService[i].ServiceStatus.dwCurrentState == SERVICE_RUNNING )
			{
				isRunning = TRUE;
			}

			break;
		}
	}

	// require_action( isRunning, exit, err = kUnknownErr );

	// Check to see if we've managed the firewall.
	// This package might have been installed, then
	// the OS was upgraded to SP2 or above.  If that's
	// the case, then we need to manipulate the firewall
	// so networking works correctly.

    err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode, &key );
	require_noerr( err, exit );

	valueLen = sizeof(DWORD);
	err = RegQueryValueEx(key, kServiceManageFirewall, 0, &type, (LPBYTE) &value, &valueLen);
	
	if ((err != ERROR_SUCCESS) || (value == 0))
	{
		wchar_t	fullPath[ MAX_PATH ];
		DWORD	size;

		// Get a full path to the executable

		size = GetModuleFileNameW( NULL, fullPath, MAX_PATH );
		err = translate_errno( size > 0, (OSStatus) GetLastError(), kPathErr );
		require_noerr( err, exit );

		err = mDNSAddToFirewall(fullPath, kServiceFirewallName);
		require_noerr( err, exit );

		value = 1;
		err = RegSetValueEx( key, kServiceManageFirewall, 0, REG_DWORD, (const LPBYTE) &value, sizeof( DWORD ) );
		require_noerr( err, exit );
	}
	
exit:

	if ( key )
	{
		RegCloseKey( key );
	}
	
	if ( lpService )
	{
		free( lpService );
	}

	if ( sc )
	{
		CloseServiceHandle ( sc );
	}

	return( err );
}



//===========================================================================================================================
//	SetServiceInfo
//===========================================================================================================================

static OSStatus	SetServiceInfo( SC_HANDLE inSCM, LPCTSTR inServiceName, LPCTSTR inDescription )
{
	OSStatus				err;
	SC_LOCK					lock;
	SC_HANDLE				service;
	SERVICE_DESCRIPTION		description;
	SERVICE_FAILURE_ACTIONS	actions;
	SC_ACTION				action;
	BOOL					ok;
	
	check( inServiceName );
	check( inDescription );
	
	lock 	= NULL;
	service	= NULL;
	
	// Open the database (if not provided) and lock it to prevent other access while re-configuring.
	
	if( !inSCM )
	{
		inSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
		err = translate_errno( inSCM, (OSStatus) GetLastError(), kOpenErr );
		require_noerr( err, exit );
	}
	
	lock = LockServiceDatabase( inSCM );
	err = translate_errno( lock, (OSStatus) GetLastError(), kInUseErr );
	require_noerr( err, exit );
	
	// Open a handle to the service. 

	service = OpenService( inSCM, inServiceName, SERVICE_CHANGE_CONFIG|SERVICE_START );
	err = translate_errno( service, (OSStatus) GetLastError(), kNotFoundErr );
	require_noerr( err, exit );
	
	// Change the description.
	
	description.lpDescription = (LPTSTR) inDescription;
	ok = ChangeServiceConfig2( service, SERVICE_CONFIG_DESCRIPTION, &description );
	err = translate_errno( ok, (OSStatus) GetLastError(), kParamErr );
	require_noerr( err, exit );
	
	actions.dwResetPeriod	=	INFINITE;
	actions.lpRebootMsg		=	NULL;
	actions.lpCommand		=	NULL;
	actions.cActions		=	1;
	actions.lpsaActions		=	&action;
	action.Delay			=	500;
	action.Type				=	SC_ACTION_RESTART;

	ok = ChangeServiceConfig2( service, SERVICE_CONFIG_FAILURE_ACTIONS, &actions );
	err = translate_errno( ok, (OSStatus) GetLastError(), kParamErr );
	require_noerr( err, exit );
	
	err = ERROR_SUCCESS;
	
exit:
	// Close the service and release the lock.
	
	if( service )
	{
		CloseServiceHandle( service );
	}
	if( lock )
	{
		UnlockServiceDatabase( lock ); 
	}
	return( err );
}

//===========================================================================================================================
//	ReportStatus
//===========================================================================================================================

static void	ReportStatus( int inType, const char *inFormat, ... )
{
	if( !gServiceQuietMode )
	{
		va_list		args;
		
		va_start( args, inFormat );
		if( gServiceEventSource )
		{
			char				s[ 1024 ];
			BOOL				ok;
			const char *		array[ 1 ];
			
			vsprintf( s, inFormat, args );
			array[ 0 ] = s;
			ok = ReportEventA( gServiceEventSource, (WORD) inType, 0, MDNSRESPONDER_LOG, NULL, 1, 0, array, NULL );
			check_translated_errno( ok, GetLastError(), kUnknownErr );
		}
		else
		{
			int		n;
			
			n = vfprintf( stderr, inFormat, args );
			check( n >= 0 );
		}
		va_end( args );
	}
}

//===========================================================================================================================
//	RunDirect
//===========================================================================================================================

int	RunDirect( int argc, LPTSTR argv[] )
{
	OSStatus		err;
	BOOL			initialized;
   BOOL        ok;
	
	initialized = FALSE;

	err = SetupServiceEvents();
	require_noerr( err, exit );

	// Install a Console Control Handler to handle things like control-c signals.
	
	ok = SetConsoleCtrlHandler( ConsoleControlHandler, TRUE );
	err = translate_errno( ok, (OSStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	
	err = ServiceSpecificInitialize( argc, argv );
	require_noerr( err, exit );
	initialized = TRUE;
	
	// Run the service. This does not return until the service quits or is stopped.
	
	ReportStatus( EVENTLOG_INFORMATION_TYPE, "Running service directly\n" );
	
	err = ServiceSpecificRun( argc, argv );
	require_noerr( err, exit );
	
	// Clean up.
	
exit:
	if( initialized )
	{
		ServiceSpecificFinalize( argc, argv );
	}

	TearDownServiceEvents();

	return( err );
}

#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	ServiceMain
//===========================================================================================================================

static void WINAPI ServiceMain( DWORD argc, LPTSTR argv[] )
{
	OSStatus		err;
	BOOL			ok;

	err = SetupServiceEvents();
	require_noerr( err, exit );
	
	err = ServiceSetupEventLogging();
	check_noerr( err );

	//err = GetServiceParameters();
	//check_noerr( err );
	
	// Initialize the service status and register the service control handler with the name of the service.
	
	gServiceStatus.dwServiceType 				= SERVICE_WIN32_SHARE_PROCESS;
	gServiceStatus.dwCurrentState 				= 0;
	gServiceStatus.dwControlsAccepted 			= SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_POWEREVENT;
	gServiceStatus.dwWin32ExitCode 				= NO_ERROR;
	gServiceStatus.dwServiceSpecificExitCode 	= NO_ERROR;
	gServiceStatus.dwCheckPoint 				= 0;
	gServiceStatus.dwWaitHint 					= 0;
	
	//gServiceStatusHandle = RegisterServiceCtrlHandlerEx( argv[ 0 ], ServiceControlHandler, NULL );
	//err = translate_errno( gServiceStatusHandle, (OSStatus) GetLastError(), kInUseErr );
	//require_noerr( err, exit );
	
	// Mark the service as starting.

	gServiceStatus.dwCurrentState 	= SERVICE_START_PENDING;
	gServiceStatus.dwCheckPoint	 	= 0;
	gServiceStatus.dwWaitHint 		= 5000;	// 5 seconds
	//ok = SetServiceStatus( gServiceStatusHandle, &gServiceStatus );
	//check_translated_errno( ok, GetLastError(), kParamErr );
	
	// Run the service. This does not return until the service quits or is stopped.
	
	err = ServiceRun( (int) argc, argv );
	if( err != kNoErr )
	{
		gServiceStatus.dwWin32ExitCode				= ERROR_SERVICE_SPECIFIC_ERROR;
		gServiceStatus.dwServiceSpecificExitCode 	= (DWORD) err;
	}
	
	// Service-specific work is done so mark the service as stopped.
	
	gServiceStatus.dwCurrentState = SERVICE_STOPPED;
	//ok = SetServiceStatus( gServiceStatusHandle, &gServiceStatus );
	//check_translated_errno( ok, GetLastError(), kParamErr );
	
	// Note: The service status handle should not be closed according to Microsoft documentation.
	
exit:

	if( gServiceEventSource )
	{
		ok = DeregisterEventSource( gServiceEventSource );
		check_translated_errno( ok, GetLastError(), kUnknownErr );
		gServiceEventSource = NULL;
	}

	TearDownServiceEvents();
}

//===========================================================================================================================
//	ServiceSetupEventLogging
//===========================================================================================================================

static OSStatus	ServiceSetupEventLogging( void )
{
	OSStatus			err;
	HKEY				key;
	LPCTSTR				s;
	DWORD				typesSupported;
	TCHAR				path[ MAX_PATH ];
	DWORD 				n;
	
	key = NULL;
	
	// Add/Open source name as a sub-key under the Application key in the EventLog registry key.

	s = TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\") kServiceName;
    err = RegCreateKey( HKEY_CURRENT_USER, s, &key );
	require_noerr( err, exit );
	
	// Add the name to the EventMessageFile subkey.

	path[ 0 ] = '\0';
	GetModuleFileName( NULL, path, MAX_PATH );
	n = (DWORD) ( ( StrLen( path ) + 1 ) * sizeof( TCHAR ) );
	err = RegSetValueEx( key, TEXT("EventMessageFile"), 0, REG_EXPAND_SZ, (const LPBYTE) path, n );
	require_noerr( err, exit );
	
	// Set the supported event types in the TypesSupported subkey.
	
	typesSupported = 0 
					 | EVENTLOG_SUCCESS
					 | EVENTLOG_ERROR_TYPE
					 | EVENTLOG_WARNING_TYPE
					 | EVENTLOG_INFORMATION_TYPE
					 | EVENTLOG_AUDIT_SUCCESS
					 | EVENTLOG_AUDIT_FAILURE; 
	err = RegSetValueEx( key, TEXT("TypesSupported"), 0, REG_DWORD, (const LPBYTE) &typesSupported, sizeof( DWORD ) );
	require_noerr( err, exit );
	
	// Set up the event source.
	
	gServiceEventSource = RegisterEventSource( NULL, kServiceName );
	err = translate_errno( gServiceEventSource, (OSStatus) GetLastError(), kParamErr );
	require_noerr( err, exit );
		
exit:
	if( key )
	{
		RegCloseKey( key );
	}
	return( err );
}


//===========================================================================================================================
//	ServiceControlHandler
//===========================================================================================================================

static DWORD WINAPI	ServiceControlHandler( DWORD inControl, DWORD inEventType, LPVOID inEventData, LPVOID inContext )
{
	BOOL		setStatus;
	OSStatus	err;
	BOOL		ok;

	DEBUG_UNUSED( inEventData );
	DEBUG_UNUSED( inContext );
	
	setStatus = TRUE;
	switch( inControl )
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			
			dlog( kDebugLevelInfo, DEBUG_NAME "ServiceControlHandler: SERVICE_CONTROL_STOP|SERVICE_CONTROL_SHUTDOWN\n" );
			
			ServiceStop();
			setStatus = FALSE;
			break;
		
		case SERVICE_CONTROL_POWEREVENT:

			if (inEventType == PBT_APMSUSPEND)
			{
				dlog( kDebugLevelInfo, DEBUG_NAME "ServiceControlHandler: PBT_APMSUSPEND\n" );

				if ( gPowerSuspendEvent )
				{
					ok = SetEvent( gPowerSuspendEvent );
					err = translate_errno( ok, (OSStatus) GetLastError(), kUnknownErr );
					check_noerr( err );

					switch ( WaitForSingleObject( gPowerSuspendAckEvent, 5 * 1000 ) )
					{
						case WAIT_OBJECT_0:
						{
							// No error
						}
						break;

						case WAIT_TIMEOUT:
						{
							dlog( kDebugLevelError, DEBUG_NAME "Timed out waiting for acknowledgement of machine sleep\n" );
							ReportStatus( EVENTLOG_ERROR_TYPE, "Timed out waiting for acknowledgement of machine sleep" );
						}
						break;

						default:
						{
							dlog( kDebugLevelError, DEBUG_NAME "Error waiting for acknowledgement of machine sleep: %d", GetLastError() );
							ReportStatus( EVENTLOG_ERROR_TYPE, "Error waiting for acknowledgement of machine sleep: %d", GetLastError() );
						}
						break;
					}
				}
			}
			else if (inEventType == PBT_APMRESUMESUSPEND)
			{
				dlog( kDebugLevelInfo, DEBUG_NAME "ServiceControlHandler: PBT_APMRESUMESUSPEND\n" );

				if ( gPowerResumeEvent )
				{
					ok = SetEvent( gPowerResumeEvent );
					err = translate_errno( ok, (OSStatus) GetLastError(), kUnknownErr );
					check_noerr( err );
				}
			}
		
			break;

		default:
			dlog( kDebugLevelNotice, DEBUG_NAME "ServiceControlHandler: event (0x%08X)\n", inControl );
			break;
	}
	
	if( setStatus && gServiceStatusHandle )
	{
		//ok = SetServiceStatus( gServiceStatusHandle, &gServiceStatus );
		//check_translated_errno( ok, GetLastError(), kUnknownErr );
	}

	return NO_ERROR;
}

//===========================================================================================================================
//	ServiceRun
//===========================================================================================================================

static OSStatus	ServiceRun( int argc, LPTSTR argv[] )
{
	OSStatus		err;
	BOOL			initialized;
	
	DEBUG_UNUSED( argc );
	DEBUG_UNUSED( argv );
	
	initialized = FALSE;
	
	// <rdar://problem/5727548> Make the service as running before we call ServiceSpecificInitialize. We've
	// had reports that some machines with McAfee firewall installed cause a problem with iTunes installation.
	// We think that the firewall product is interferring with code in ServiceSpecificInitialize. So as a
	// simple workaround, we'll mark us as running *before* we call ServiceSpecificInitialize. This will unblock
	// any installers that are waiting for our state to change.

	gServiceStatus.dwCurrentState = SERVICE_RUNNING;
	//ok = SetServiceStatus( gServiceStatusHandle, &gServiceStatus );
	//check_translated_errno( ok, GetLastError(), kParamErr );

	// Initialize the service-specific stuff
	
	while ( 1 )
	{
		DWORD ret;

        //ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service initializing" );

		err = ServiceSpecificInitialize( argc, argv );

		if ( !err )
		{
            //ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service initialized" );
			break;
		}

		ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service initialization failed with err %d. Waiting %d seconds to retry...", err, kWaitToRetry );

		ret = WaitForSingleObject( gStopEvent, 1000 * kWaitToRetry );

		if ( ret == WAIT_OBJECT_0 )
		{
			ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service received a stop event" );
			goto exit;
		}
		else if ( ret == WAIT_OBJECT_0 + 1 )
		{
			ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service received a power suspend event" );
		}
		else if ( ret == WAIT_OBJECT_0 + 2 )
		{
			ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service received a power resume event" );
		}
		else if ( ret != WAIT_TIMEOUT )
		{
			ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service received an error in WaitForSingleObject() : %d, %d", ret, GetLastError() );
			goto exit;
		}
	}

	initialized = TRUE;
	
	err = CheckFirewall();
	check_noerr( err );

	if ( err )
	{
		gRetryFirewall = TRUE;
	}
	
    if (gJustCheckFirewall)
        goto exit;
	// Run the service-specific stuff. This does not return until the service quits or is stopped.

    //ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service started\n" );

	err = ServiceSpecificRun( argc, argv );
	require_noerr( err, exit );

exit:

	// Service stopped. Clean up and we're done.
	
	ReportStatus( EVENTLOG_INFORMATION_TYPE, "Service stopped (%d)\n", err );
	
	if( initialized )
	{
		ServiceSpecificFinalize( argc, argv );
	}
	
	return( err );
}

//===========================================================================================================================
//	ServiceStop
//===========================================================================================================================

static void	ServiceStop( void )
{
	OSStatus		err;
	
	// Signal the event to cause the service to exit.
	
	if( gServiceStatusHandle )
	{
		gServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		//ok = SetServiceStatus( gServiceStatusHandle, &gServiceStatus );
		//check_translated_errno( ok, GetLastError(), kParamErr );
	}
		
	err = ServiceSpecificStop();
	check_noerr( err );
}


#if 0
#pragma mark -
#pragma mark == Service Specific ==
#endif

//===========================================================================================================================
//	ServiceSpecificInitialize
//===========================================================================================================================

static OSStatus	ServiceSpecificInitialize( int argc, LPTSTR argv[] )
{
	OSStatus err;
	
	DEBUG_UNUSED( argc );
	DEBUG_UNUSED( argv );
	
	mDNSPlatformMemZero( &gMDNSRecord, sizeof gMDNSRecord);
	mDNSPlatformMemZero( &gPlatformStorage, sizeof gPlatformStorage);

	gPlatformStorage.reportStatusFunc = ReportStatus;

	err = mDNS_Init( &gMDNSRecord, &gPlatformStorage, gRRCache, RR_CACHE_SIZE, mDNS_Init_AdvertiseLocalAddresses, CoreCallback, mDNS_Init_NoInitCallbackContext); 
	require_noerr( err, exit);

	err = SetupNotifications();
	check_noerr( err );

	err = udsserver_init(mDNSNULL, 0);
	require_noerr( err, exit);

	SetLLRoute( &gMDNSRecord );

exit:
	if( err != kNoErr )
	{
		ServiceSpecificFinalize( argc, argv );
	}
	return( err );
}

//===========================================================================================================================
//	ServiceSpecificRun
//===========================================================================================================================

static OSStatus	ServiceSpecificRun( int argc, LPTSTR argv[] )
{
	mDNSBool done = mDNSfalse;
	mStatus err = mStatus_NoError;
	
	DEBUG_UNUSED( argc );
	DEBUG_UNUSED( argv );

	err = SetupInterfaceList( &gMDNSRecord );
	check( !err );

	err = uDNS_SetupDNSConfig( &gMDNSRecord );
	check( !err );

	while( !done )
	{
		static mDNSs32 RepeatedBusy = 0;	
		mDNSs32 nextTimerEvent;
		mStatus err;

		// Give the mDNS core a chance to do its work and determine next event time.

		nextTimerEvent = udsserver_idle( mDNS_Execute( &gMDNSRecord ) - mDNS_TimeNow( &gMDNSRecord ) );

		if      ( nextTimerEvent < 0)					nextTimerEvent = 0;
		else if ( nextTimerEvent > (0x7FFFFFFF / 1000))	nextTimerEvent = 0x7FFFFFFF / mDNSPlatformOneSecond;
		else											nextTimerEvent = ( nextTimerEvent * 1000) / mDNSPlatformOneSecond;

		// Debugging sanity check, to guard against CPU spins
			
		if ( nextTimerEvent > 0 )
		{
			RepeatedBusy = 0;
		}
		else
		{
			nextTimerEvent = 1;

			if ( ++RepeatedBusy >= mDNSPlatformOneSecond )
			{
				ShowTaskSchedulingError( &gMDNSRecord );
				RepeatedBusy = 0;
			}
		}

		if ( gMDNSRecord.ShutdownTime )
		{
			mDNSs32 now = mDNS_TimeNow( &gMDNSRecord );

			if ( mDNS_ExitNow( &gMDNSRecord, now ) )
			{
				mDNS_FinalExit( &gMDNSRecord );
				done = TRUE;
				break;
			}

			if ( nextTimerEvent - gMDNSRecord.ShutdownTime >= 0 )
			{
				nextTimerEvent = gMDNSRecord.ShutdownTime;
			}
		}

		err = mDNSPoll( nextTimerEvent );

		if ( err )
		{
			Sleep( 3 * 1000 );
				
			err = SetupInterfaceList( &gMDNSRecord );
			check( !err );

			err = uDNS_SetupDNSConfig( &gMDNSRecord );
			check( !err );
				
			break;
		}
	}

	return ( err );
}


//===========================================================================================================================
//	ServiceSpecificStop
//===========================================================================================================================

static OSStatus	ServiceSpecificStop( void )
{
	OSStatus    err;
	BOOL        ok;

	ok = SetEvent(gStopEvent);
	err = translate_errno( ok, (OSStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

exit:

	return( err );
}

//===========================================================================================================================
//	ServiceSpecificFinalize
//===========================================================================================================================

static void	ServiceSpecificFinalize( int argc, LPTSTR argv[] )
{
	DEBUG_UNUSED( argc );
	DEBUG_UNUSED( argv );
	
	//
	// clean up the notifications
	//
	TearDownNotifications();

	//
	// clean up loaded library
	//

	if( gIPHelperLibraryInstance )
	{
		gGetIpInterfaceEntryFunctionPtr = NULL;
		
		FreeLibrary( gIPHelperLibraryInstance );
		gIPHelperLibraryInstance = NULL;
	}
}


//===========================================================================================================================
//	SetupServiceEvents
//===========================================================================================================================

mDNSlocal mStatus	SetupServiceEvents()
{
	mStatus err;
	
	// Stop Event

	gStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	err = translate_errno( gStopEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

exit:

	if ( err )
	{
		TearDownServiceEvents();
	}

	return err;
}


//===========================================================================================================================
//	TearDownServiceNotifications
//===========================================================================================================================

mDNSlocal mStatus	TearDownServiceEvents()
{
	if ( gStopEvent )
	{
		CloseHandle( gStopEvent );
		gStopEvent = NULL;
	}

	return mStatus_NoError;
}


//===========================================================================================================================
//	SetupNotifications
//===========================================================================================================================

mDNSlocal mStatus	SetupNotifications()
{
	mStatus				err;
	SocketRef			sock;
	unsigned long		param;
	int					inBuffer;
	int					outBuffer;
	DWORD				outSize;

	require_action( gStopEvent, exit, err = kUnknownErr );
	err = mDNSPollRegisterEvent( gStopEvent, StopNotification, NULL );
	require_noerr( err, exit );

	// Power Suspend

	gPowerSuspendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	err = translate_errno( gPowerSuspendEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gPowerSuspendEvent, PowerSuspendNotification, NULL );
	require_noerr( err, exit );

	// Power Suspend Ack

	gPowerSuspendAckEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	err = translate_errno( gPowerSuspendAckEvent, ( mStatus ) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	// Power Resume

	gPowerResumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	err = translate_errno( gPowerResumeEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gPowerResumeEvent, PowerResumeNotification, NULL );
	require_noerr( err, exit );

	// Register to listen for address list changes.
	
	sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	err = translate_errno( IsValidSocket( sock ), errno_compat(), kUnknownErr );
	require_noerr( err, exit );
	gInterfaceListChangedSocket = sock;
	
	// Make the socket non-blocking so the WSAIoctl returns immediately with WSAEWOULDBLOCK. It will set the event 
	// when a change to the interface list is detected.
	
	param = 1;
	err = ioctlsocket( sock, FIONBIO, &param );
	err = translate_errno( err == 0, errno_compat(), kUnknownErr );
	require_noerr( err, exit );
	
	inBuffer	= 0;
	outBuffer	= 0;
	err = WSAIoctl( sock, SIO_ADDRESS_LIST_CHANGE, &inBuffer, 0, &outBuffer, 0, &outSize, NULL, NULL );
	if( err < 0 )
	{
		check( errno_compat() == WSAEWOULDBLOCK );
	}

	err = mDNSPollRegisterSocket( sock, FD_ADDRESS_LIST_CHANGE, InterfaceListNotification, NULL );
	require_noerr( err, exit );
	
	gDescChangedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	err = translate_errno( gDescChangedEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\lanmanserver\\parameters"), 0, KEY_READ, &gDescKey);
	check_translated_errno( err == 0, errno_compat(), kNameErr );

	if ( gDescKey != NULL )
	{
		err = RegNotifyChangeKeyValue( gDescKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, gDescChangedEvent, TRUE);
		require_noerr( err, exit );
	}

	err = mDNSPollRegisterEvent( gDescChangedEvent, ComputerDescriptionNotification, NULL );
	require_noerr( err, exit );

#ifndef TCP_CHANGES_NO_REGISTRY
	// This will catch all changes to tcp/ip networking, including changes to the domain search list

	gTcpipChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	err = translate_errno( gTcpipChangedEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = RegCreateKey( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), &gTcpipKey );
	require_noerr( err, exit );
	err = RegNotifyChangeKeyValue( gTcpipKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gTcpipChangedEvent, TRUE);
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gTcpipChangedEvent, TCPChangedNotification, NULL );
	require_noerr( err, exit );
#else
	// This is another way to catch tcp/ip changes, which avoid the registry
	// (better if the registry does not work reliably)
	{
		DWORD				ret;
		gTcpipChangedEvent2.hEvent = WSACreateEvent();
		err = gTcpipChangedEvent2.hEvent == WSA_INVALID_EVENT;
		require_noerr( err, skipTCP2 );
		ret = NotifyAddrChange(&gTcpipChangedEvent2Handle, &gTcpipChangedEvent2);
		if (ret != NO_ERROR)
			err = (WSAGetLastError() != WSA_IO_PENDING );
		require_noerr( err, skipTCP2 );
		err = mDNSPollRegisterEvent( gTcpipChangedEvent2.hEvent, &TCPChangedNotification2, NULL );
		check_noerr( err );
	}
#endif
	// This will catch all changes to ddns configuration

	gDdnsChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	err = translate_errno( gDdnsChangedEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode TEXT("\\DynDNS\\Setup"), &gDdnsKey );
	require_noerr( err, exit );
	err = RegNotifyChangeKeyValue( gDdnsKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gDdnsChangedEvent, TRUE);
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gDdnsChangedEvent, DDNSChangedNotification, NULL );
	require_noerr( err, exit );

	// This will catch all changes to file sharing

	gFileSharingChangedEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	err = translate_errno( gFileSharingChangedEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	err = RegCreateKey( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\lanmanserver\\Shares"), &gFileSharingKey );
	
	// Just to make sure that initialization doesn't fail on some old OS
	// that doesn't have this key, we'll only add the notification if
	// the key exists.

	if ( !err )
	{
		err = RegNotifyChangeKeyValue( gFileSharingKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gFileSharingChangedEvent, TRUE);
		require_noerr( err, exit );
		err = mDNSPollRegisterEvent( gFileSharingChangedEvent, FileSharingChangedNotification, NULL );
		require_noerr( err, exit );
	}
	else
	{
		err = mStatus_NoError;
	}

	// This will catch changes to the Windows firewall

	gFirewallChangedEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	err = translate_errno( gFirewallChangedEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );

	// Just to make sure that initialization doesn't fail on some old OS
	// that doesn't have this key, we'll only add the notification if
	// the key exists.

	err = RegCreateKey( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\FirewallRules"), &gFirewallKey );
	
	if ( !err )
	{
		err = RegNotifyChangeKeyValue( gFirewallKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gFirewallChangedEvent, TRUE);
		require_noerr( err, exit );
		err = mDNSPollRegisterEvent( gFirewallChangedEvent, FirewallChangedNotification, NULL );
		require_noerr( err, exit );
	}
	else
	{
		err = mStatus_NoError;
	}

	// This will catch all changes to advertised services configuration

	gAdvertisedServicesChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	err = translate_errno( gAdvertisedServicesChangedEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode TEXT("\\Services"), &gAdvertisedServicesKey );
	require_noerr( err, exit );
	err = RegNotifyChangeKeyValue( gAdvertisedServicesKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gAdvertisedServicesChangedEvent, TRUE);
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gAdvertisedServicesChangedEvent, AdvertisedServicesChangedNotification, NULL );
	require_noerr( err, exit );

	// SPSWakeup timer

	gSPSWakeupEvent = CreateWaitableTimer( NULL, FALSE, NULL );
	err = translate_errno( gSPSWakeupEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gSPSWakeupEvent, SPSWakeupNotification, NULL );
	require_noerr( err, exit );

	// SPSSleep timer

	gSPSSleepEvent = CreateWaitableTimer( NULL, FALSE, NULL );
	err = translate_errno( gSPSSleepEvent, (mStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	err = mDNSPollRegisterEvent( gSPSSleepEvent, SPSSleepNotification, NULL );
	require_noerr( err, exit );

exit:
	if( err )
	{
		TearDownNotifications();
	}
	return( err );
}

//===========================================================================================================================
//	TearDownNotifications
//===========================================================================================================================

mDNSlocal mStatus	TearDownNotifications()
{
	if( IsValidSocket( gInterfaceListChangedSocket ) )
	{
		mDNSPollUnregisterSocket( gInterfaceListChangedSocket );

		close_compat( gInterfaceListChangedSocket );
		gInterfaceListChangedSocket = kInvalidSocketRef;
	}

	if ( gDescChangedEvent != NULL )
	{
		mDNSPollUnregisterEvent( gDescChangedEvent );
		CloseHandle( gDescChangedEvent );
		gDescChangedEvent = NULL;
	}

	if ( gDescKey != NULL )
	{
		RegCloseKey( gDescKey );
		gDescKey = NULL;
	}

	if ( gTcpipChangedEvent != NULL )
	{
		mDNSPollUnregisterEvent( gTcpipChangedEvent );
		CloseHandle( gTcpipChangedEvent );
		gTcpipChangedEvent = NULL;
	}

	if ( gTcpipChangedEvent2Handle != NULL || gTcpipChangedEvent2.hEvent != NULL  )
	{
		mDNSPollUnregisterEvent( gTcpipChangedEvent2.hEvent );
		CancelIPChangeNotify(&gTcpipChangedEvent2);
		gTcpipChangedEvent2Handle = NULL;
		gTcpipChangedEvent2.hEvent = NULL;
	}

	if ( gDdnsChangedEvent != NULL )
	{
		mDNSPollUnregisterEvent( gDdnsChangedEvent );
		CloseHandle( gDdnsChangedEvent );
		gDdnsChangedEvent = NULL;
	}

	if ( gDdnsKey != NULL )
	{
		RegCloseKey( gDdnsKey );
		gDdnsKey = NULL;
	}

	if ( gFileSharingChangedEvent != NULL )
	{
		mDNSPollUnregisterEvent( gFileSharingChangedEvent );
		CloseHandle( gFileSharingChangedEvent );
		gFileSharingChangedEvent = NULL;
	}

	if ( gFileSharingKey != NULL )
	{
		RegCloseKey( gFileSharingKey );
		gFileSharingKey = NULL;
	}

	if ( gFirewallChangedEvent != NULL )
	{
		mDNSPollUnregisterEvent( gFirewallChangedEvent );
		CloseHandle( gFirewallChangedEvent );
		gFirewallChangedEvent = NULL;
	}

	if ( gFirewallKey != NULL )
	{
		RegCloseKey( gFirewallKey );
		gFirewallKey = NULL;
	}

	if ( gAdvertisedServicesChangedEvent != NULL )
	{
		mDNSPollUnregisterEvent( gAdvertisedServicesChangedEvent );
		CloseHandle( gAdvertisedServicesChangedEvent );
		gAdvertisedServicesChangedEvent = NULL;
	}

	if ( gAdvertisedServicesKey != NULL )
	{
		RegCloseKey( gAdvertisedServicesKey );
		gAdvertisedServicesKey = NULL;
	}

	if ( gSPSWakeupEvent )
	{
		mDNSPollUnregisterEvent( gSPSWakeupEvent );
		CloseHandle( gSPSWakeupEvent );
		gSPSWakeupEvent = NULL;
	}

	if ( gSPSSleepEvent )
	{
		mDNSPollUnregisterEvent( gSPSSleepEvent );
		CloseHandle( gSPSSleepEvent );
		gSPSSleepEvent = NULL;
	}

	if ( gPowerResumeEvent )
	{
		mDNSPollUnregisterEvent( gPowerResumeEvent );
		CloseHandle( gPowerResumeEvent );
		gPowerResumeEvent = NULL;
	}

	if ( gPowerSuspendAckEvent )
	{
		CloseHandle( gPowerSuspendAckEvent );
		gPowerSuspendAckEvent = NULL;
	}

	if ( gPowerSuspendEvent )
	{
		mDNSPollUnregisterEvent( gPowerSuspendEvent );
		CloseHandle( gPowerSuspendEvent );
		gPowerSuspendEvent = NULL;
	}

	if ( gStopEvent )
	{
		mDNSPollUnregisterEvent( gStopEvent );
	}

	return( mStatus_NoError );
}


mDNSlocal void CALLBACK
StopNotification( HANDLE event, void *context )
{
	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	dlog( kDebugLevelVerbose, DEBUG_NAME "stopping...\n" );
	udsserver_exit();
	mDNS_StartExit( &gMDNSRecord );
}


mDNSlocal void CALLBACK
PowerSuspendNotification( HANDLE event, void * context )
{
	LARGE_INTEGER	timeout;
	BOOL			ok;

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	dlog( kDebugLevelInfo, DEBUG_NAME "PowerSuspendNotification\n" );

	gMDNSRecord.SystemWakeOnLANEnabled = SystemWakeForNetworkAccess( &timeout );
				
	if ( gMDNSRecord.SystemWakeOnLANEnabled )
	{
		ok = SetWaitableTimer( gSPSWakeupEvent, &timeout, 0, NULL, NULL, TRUE );
		check( ok );
	}

	mDNSCoreMachineSleep(&gMDNSRecord, TRUE);

	ok = SetEvent( gPowerSuspendAckEvent );

	if ( !ok )
	{
		dlog( kDebugLevelError, DEBUG_NAME "PowerSuspendNotification: error while setting acknowledgement: %d", GetLastError() );
		ReportStatus( EVENTLOG_ERROR_TYPE, "PowerSuspendNotification: error while setting acknowledgement: %d", GetLastError() );
	}
}


mDNSlocal void CALLBACK
PowerResumeNotification( HANDLE event, void * context )
{
	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	dlog( kDebugLevelInfo, DEBUG_NAME "PowerResumeNotification\n" );

	if ( gSPSWakeupEvent )
	{
		CancelWaitableTimer( gSPSWakeupEvent );
	}

	if ( gSPSSleepEvent )
	{
		CancelWaitableTimer( gSPSSleepEvent );
	}

	mDNSCoreMachineSleep(&gMDNSRecord, FALSE);
}



mDNSlocal void CALLBACK
InterfaceListNotification( SOCKET socket, LPWSANETWORKEVENTS event, void *context )
{
	int		inBuffer;
	int		outBuffer;
	DWORD	outSize;
	int		err;

	DEBUG_UNUSED( socket );
	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	// It would be nice to come up with a more elegant solution to this, but it seems that
	// GetAdaptersAddresses doesn't always stay in sync after network changed events.  So as
	// as a simple workaround, we'll pause for a couple of seconds before processing the change.

	// We arrived at 2 secs by trial and error. We could reproduce the problem after sleeping
	// for 500 msec and 750 msec, but couldn't after sleeping for 1 sec.  We added another
	// second on top of that to account for machine load or some other exigency.

	Sleep( 2000 );

	// Interface list changed event. Break out of the inner loop to re-setup the wait list.
					
	InterfaceListDidChange( &gMDNSRecord );

	// reset the event handler
	inBuffer	= 0;
	outBuffer	= 0;
	err = WSAIoctl( gInterfaceListChangedSocket, SIO_ADDRESS_LIST_CHANGE, &inBuffer, 0, &outBuffer, 0, &outSize, NULL, NULL );
	if( err < 0 )
	{
		check( errno_compat() == WSAEWOULDBLOCK );
	}
}


mDNSlocal void CALLBACK
ComputerDescriptionNotification( HANDLE event, void *context )
{
	// The computer description might have changed
					
	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	ComputerDescriptionDidChange( &gMDNSRecord );
	udsserver_handle_configchange( &gMDNSRecord );

	// and reset the event handler
	if ( ( gDescKey != NULL ) && ( gDescChangedEvent != NULL ) )
	{
		int err = RegNotifyChangeKeyValue( gDescKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, gDescChangedEvent, TRUE);
		check_noerr( err );
	}
}


mDNSlocal void CALLBACK
TCPChangedNotification( HANDLE event, void *context )
{
	// The TCP/IP might have changed

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	dlog( kDebugLevelInfo, DEBUG_NAME "TCPChangeNotification\n" );

	TCPIPConfigDidChange( &gMDNSRecord );
	udsserver_handle_configchange( &gMDNSRecord );

	// and reset the event handler

	if ( ( gTcpipKey != NULL ) && ( gTcpipChangedEvent ) )
	{
		int err = RegNotifyChangeKeyValue( gTcpipKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gTcpipChangedEvent, TRUE );
		check_noerr( err );
	}
}


mDNSlocal void CALLBACK
TCPChangedNotification2( HANDLE event, void *context ){
	int err;
	DWORD dwRetVal;

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	dlog( kDebugLevelInfo, DEBUG_NAME "TCPChangeNotification2\n" );

	err = WSAResetEvent(gTcpipChangedEvent2.hEvent);
	check_noerr(err);
	//std::cout << "WSAResetEvent() failed. Error code: " << WSAGetLastError() << std::endl;

	// The TCP/IP might have changed

	TCPIPConfigDidChange( &gMDNSRecord );
	udsserver_handle_configchange( &gMDNSRecord );

	// and reset the event handler

	// call NotifyAddrChange in synchronous mode
	dwRetVal = NotifyAddrChange(&gTcpipChangedEvent2Handle, &gTcpipChangedEvent2);

	check_noerr(dwRetVal != ERROR_IO_PENDING);
}


mDNSlocal void CALLBACK
DDNSChangedNotification( HANDLE event, void *context )
{
	// The DynDNS config might have changed

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	DynDNSConfigDidChange( &gMDNSRecord );
	udsserver_handle_configchange( &gMDNSRecord );

	// and reset the event handler

	if ((gDdnsKey != NULL) && (gDdnsChangedEvent))
	{
		int err = RegNotifyChangeKeyValue(gDdnsKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gDdnsChangedEvent, TRUE);
		check_noerr( err );
	}
}


mDNSlocal void CALLBACK
FileSharingChangedNotification( HANDLE event, void *context )
{
	// File sharing changed

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	FileSharingDidChange( &gMDNSRecord );

	// and reset the event handler

	if ((gFileSharingKey != NULL) && (gFileSharingChangedEvent))
	{
		int err = RegNotifyChangeKeyValue(gFileSharingKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gFileSharingChangedEvent, TRUE);
		check_noerr( err );
	}
}


mDNSlocal void CALLBACK
FirewallChangedNotification( HANDLE event, void *context )
{
	// Firewall configuration changed

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	FirewallDidChange( &gMDNSRecord );

	// and reset the event handler

	if ((gFirewallKey != NULL) && (gFirewallChangedEvent))
	{
		int err = RegNotifyChangeKeyValue(gFirewallKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gFirewallChangedEvent, TRUE);
		check_noerr( err );
	}
}


mDNSlocal void CALLBACK
AdvertisedServicesChangedNotification( HANDLE event, void *context )
{
	// Ultimately we'll want to manage multiple services, but right now the only service
	// we'll be managing is SMB.

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	FileSharingDidChange( &gMDNSRecord );

	// and reset the event handler

	if ( ( gAdvertisedServicesKey != NULL ) && ( gAdvertisedServicesChangedEvent ) )
	{
		int err = RegNotifyChangeKeyValue(gAdvertisedServicesKey, TRUE, REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_LAST_SET, gAdvertisedServicesChangedEvent, TRUE);
		check_noerr( err );
	}
}


mDNSlocal void CALLBACK
SPSWakeupNotification( HANDLE event, void *context )
{
	LARGE_INTEGER timeout;

	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	ReportStatus( EVENTLOG_INFORMATION_TYPE, "Maintenance wake" );

	timeout.QuadPart  = kSPSMaintenanceWakePeriod;
	timeout.QuadPart *= kSecondsTo100NSUnits;

	SetWaitableTimer( gSPSSleepEvent, &timeout, 0, NULL, NULL, TRUE );
}


mDNSlocal void CALLBACK
SPSSleepNotification( HANDLE event, void *context )
{
	DEBUG_UNUSED( event );
	DEBUG_UNUSED( context );

	ReportStatus( EVENTLOG_INFORMATION_TYPE, "Returning to sleep after maintenance wake" );

	// Calling SetSuspendState() doesn't invoke our sleep handlers, so we'll
	// call HandlePowerSuspend() explicity.  This will reset the 
	// maintenance wake timers.

	PowerSuspendNotification( gPowerSuspendEvent, NULL );
	SetSuspendState( FALSE, FALSE, FALSE );
}


//===========================================================================================================================
//	CoreCallback
//===========================================================================================================================

static void
CoreCallback(mDNS * const inMDNS, mStatus status)
{
	if (status == mStatus_ConfigChanged)
	{
		SetLLRoute( inMDNS );
	}
}


//===========================================================================================================================
//	UDSAcceptNotification
//===========================================================================================================================

mDNSlocal void CALLBACK
UDSAcceptNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context )
{
	( void ) sock;
	( void ) event;
	( void ) context;
	
	if ( gUDSCallback )
	{
		gUDSCallback( ( int ) gUDSSocket, 0, context );
	}
}


//===========================================================================================================================
//	UDSReadNotification
//===========================================================================================================================

mDNSlocal void CALLBACK
UDSReadNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context )
{
	TCPSocket *tcpSock = ( TCPSocket* ) context;

	( void ) sock;
	( void ) event;

	if ( tcpSock )
	{
		tcpSock->userCallback( ( int ) tcpSock->fd, 0, tcpSock->userContext );
	}
}


//===========================================================================================================================
//	udsSupportAddFDToEventLoop
//===========================================================================================================================

mStatus
udsSupportAddFDToEventLoop( SocketRef fd, udsEventCallback callback, void *context, void **platform_data)
{
	mStatus err = mStatus_NoError;

	// We are using some knowledge of what is being passed to us here.  If the fd is a listen socket,
	// then the "context" parameter is NULL.  If it is an actual read/write socket, then the "context"
	// parameter is not null.

	if ( context )
	{
		TCPSocket * sock;

		sock = malloc( sizeof( TCPSocket ) );
		require_action( sock, exit, err = mStatus_NoMemoryErr );
		mDNSPlatformMemZero( sock, sizeof( TCPSocket ) );

		sock->fd				= (SOCKET) fd;
		sock->userCallback		= callback;
		sock->userContext		= context;
		sock->m					= &gMDNSRecord;

		*platform_data = sock;

		err = mDNSPollRegisterSocket( sock->fd, FD_READ | FD_CLOSE, UDSReadNotification, sock );
		require_noerr( err, exit );
	}
	else
	{
		gUDSSocket		= fd;
		gUDSCallback	= callback;

		err = mDNSPollRegisterSocket( gUDSSocket, FD_ACCEPT | FD_CLOSE, UDSAcceptNotification, NULL );
		require_noerr( err, exit );
	}

exit:

	return err;
}


int
udsSupportReadFD( SocketRef fd, char *buf, int len, int flags, void *platform_data )
{
	TCPSocket	*	sock;
	mDNSBool		closed;
	int				ret;

	( void ) flags;

	sock = ( TCPSocket* ) platform_data;
	require_action( sock, exit, ret = -1 );
	require_action( sock->fd == fd, exit, ret = -1 );

	ret = mDNSPlatformReadTCP( sock, buf, len, &closed );

	if ( closed )
	{
		ret = 0;
	}
	else if ( !ret && ( WSAGetLastError() == WSAEWOULDBLOCK ) )
	{
		// mDNSPlatformReadTCP will return 0 if it gets WSAEWOULDBLOCK, but
		// that caller of this routine interprets that as close connection.
		// We'll fix that by returning -1 in that case.

		ret = -1;
	}

exit:

	return ret;
}


mStatus
udsSupportRemoveFDFromEventLoop( SocketRef fd, void *platform_data)		// Note: This also CLOSES the socket
{
	mStatus err = kNoErr;

	mDNSPollUnregisterSocket( fd );

	if ( platform_data != NULL )
	{
		TCPSocket * sock;

		dlog( kDebugLevelInfo, DEBUG_NAME "session closed\n" );
		sock = ( TCPSocket* ) platform_data;
		check( sock->fd == fd );
		mDNSPlatformTCPCloseConnection( sock );
	}

	return err;
}


mDNSexport void RecordUpdatedNiceLabel(mDNS *const m, mDNSs32 delay)
	{
	(void)m;
	(void)delay;
	// No-op, for now
	}


//===========================================================================================================================
//	SystemWakeForNetworkAccess
//===========================================================================================================================

mDNSu8
SystemWakeForNetworkAccess( LARGE_INTEGER * timeout )
{
	HKEY					key = NULL;
	DWORD					dwSize;
	DWORD					enabled;
	mDNSu8					ok;
	SYSTEM_POWER_STATUS		powerStatus;
	time_t					startTime;
	time_t					nextWakeupTime;
	int						delta;
	DWORD					err;

	dlog( kDebugLevelInfo, DEBUG_NAME "SystemWakeForNetworkAccess\n" );

	// Make sure we have a timer

	require_action( gSPSWakeupEvent != NULL, exit, ok = FALSE );
	require_action( gSPSSleepEvent != NULL, exit, ok = FALSE );

	// Make sure the user enabled bonjour sleep proxy client 
	
	err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode L"\\Power Management", &key );
	require_action( !err, exit, ok = FALSE );
	dwSize = sizeof( DWORD );
	err = RegQueryValueEx( key, L"Enabled", NULL, NULL, (LPBYTE) &enabled, &dwSize );
	require_action( !err, exit, ok = FALSE );
	require_action( enabled, exit, ok = FALSE );
	
	// Make sure machine is on AC power
	
	ok = ( mDNSu8 ) GetSystemPowerStatus( &powerStatus );
	require_action( ok, exit, ok = FALSE );
	require_action( powerStatus.ACLineStatus == AC_LINE_ONLINE, exit, ok = FALSE );

	// Now make sure we have a network interface that does wake-on-lan

	ok = ( mDNSu8 ) IsWOMPEnabled( &gMDNSRecord );
	require_action( ok, exit, ok = FALSE );

	// Now make sure we have advertised services. Doesn't make sense to
	// enable sleep proxy if we have no multicast services that could
	// potentially wake us up.

	ok = ( mDNSu8 ) mDNSCoreHaveAdvertisedMulticastServices( &gMDNSRecord );
	require_action( ok, exit, ok = FALSE );

	// Calculate next wake up time

	startTime		= time( NULL );					// Seconds since midnight January 1, 1970
	nextWakeupTime	= startTime + ( 120 * 60 );		// 2 hours later
	
	if ( gMDNSRecord.p->nextDHCPLeaseExpires < nextWakeupTime )
	{
		nextWakeupTime = gMDNSRecord.p->nextDHCPLeaseExpires;
	}

	// Finally calculate the next relative wakeup time

	delta = ( int )( ( ( double )( nextWakeupTime - startTime ) ) * 0.9 );
	ReportStatus( EVENTLOG_INFORMATION_TYPE, "enabling sleep proxy client with next maintenance wake in %d seconds", delta );

	// Convert seconds to 100 nanosecond units expected by SetWaitableTimer

	timeout->QuadPart  = -delta;
	timeout->QuadPart *= kSecondsTo100NSUnits;

	ok = TRUE;

exit:

	if ( key )
	{
		RegCloseKey( key );
	}

	return ok;
}


//===========================================================================================================================
//	HaveRoute
//===========================================================================================================================

static bool
HaveRoute( PMIB_IPFORWARDROW rowExtant, unsigned long addr, unsigned long metric )
{
	PMIB_IPFORWARDTABLE	pIpForwardTable	= NULL;
	DWORD				dwSize			= 0;
	BOOL				bOrder			= FALSE;
	OSStatus			err;
	bool				found			= false;
	unsigned long int	i;

	//
	// Find out how big our buffer needs to be.
	//
	err = GetIpForwardTable(NULL, &dwSize, bOrder);
	require_action( err == ERROR_INSUFFICIENT_BUFFER, exit, err = kUnknownErr );

	//
	// Allocate the memory for the table
	//
	pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc( dwSize );
	require_action( pIpForwardTable, exit, err = kNoMemoryErr );
  
	//
	// Now get the table.
	//
	err = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
	require_noerr( err, exit );

	//
	// Search for the row in the table we want.
	//
	for ( i = 0; i < pIpForwardTable->dwNumEntries; i++)
	{
		if ( ( pIpForwardTable->table[i].dwForwardDest == addr ) && ( !metric || ( pIpForwardTable->table[i].dwForwardMetric1 == metric ) ) )
		{
			memcpy( rowExtant, &(pIpForwardTable->table[i]), sizeof(*rowExtant) );
			found = true;
			break;
		}
	}

exit:

	if ( pIpForwardTable != NULL ) 
	{
		free(pIpForwardTable);
	}
    
	return found;
}


//===========================================================================================================================
//	IsValidAddress
//===========================================================================================================================

static bool
IsValidAddress( const char * addr )
{
	return ( addr && ( strcmp( addr, "0.0.0.0" ) != 0 ) ) ? true : false;
}	


//===========================================================================================================================
//	GetAdditionalMetric
//===========================================================================================================================

static ULONG
GetAdditionalMetric( DWORD ifIndex )
{
	ULONG metric = 0;

	if( !gIPHelperLibraryInstance )
	{
		gIPHelperLibraryInstance = LoadLibrary( TEXT( "Iphlpapi" ) );

		gGetIpInterfaceEntryFunctionPtr = 
				(GetIpInterfaceEntryFunctionPtr) GetProcAddress( gIPHelperLibraryInstance, "GetIpInterfaceEntry" );

		if( !gGetIpInterfaceEntryFunctionPtr )
		{		
			BOOL ok;
				
			ok = FreeLibrary( gIPHelperLibraryInstance );
			check_translated_errno( ok, GetLastError(), kUnknownErr );
			gIPHelperLibraryInstance = NULL;
		}
	}

	if ( gGetIpInterfaceEntryFunctionPtr )
	{
		MIB_IPINTERFACE_ROW row;
		DWORD err;

		ZeroMemory( &row, sizeof( MIB_IPINTERFACE_ROW ) );
		row.Family = AF_INET;
		row.InterfaceIndex = ifIndex;
		err = gGetIpInterfaceEntryFunctionPtr( &row );
		require_noerr( err, exit );
		metric = row.Metric + 256;
	}

exit:

	return metric;
}


//===========================================================================================================================
//	SetLLRoute
//===========================================================================================================================

static OSStatus
SetLLRoute( mDNS * const inMDNS )
{
	OSStatus err = kNoErr;

	DEBUG_UNUSED( inMDNS );

	//
	// <rdar://problem/4096464> Don't call SetLLRoute on loopback
	// <rdar://problem/6885843> Default route on Windows 7 breaks network connectivity
	// 
	// Don't mess w/ the routing table on Vista and later OSes, as 
	// they have a permanent route to link-local addresses. Otherwise,
	// set a route to link local addresses (169.254.0.0)
	//
	if ( ( inMDNS->p->osMajorVersion < 6 ) && gServiceManageLLRouting && !gPlatformStorage.registeredLoopback4 )
	{
		DWORD				ifIndex;
		MIB_IPFORWARDROW	rowExtant;
		bool				addRoute;
		MIB_IPFORWARDROW	row;

		ZeroMemory(&row, sizeof(row));

		err = GetRouteDestination(&ifIndex, &row.dwForwardNextHop);
		require_noerr( err, exit );
		row.dwForwardDest		= inet_addr(kLLNetworkAddr);
		row.dwForwardIfIndex	= ifIndex;
		row.dwForwardMask		= inet_addr(kLLNetworkAddrMask);
		row.dwForwardType		= 3;
		row.dwForwardProto		= MIB_IPPROTO_NETMGMT;
		row.dwForwardAge		= 0;
		row.dwForwardPolicy		= 0;
		row.dwForwardMetric1	= 20 + GetAdditionalMetric( ifIndex );
		row.dwForwardMetric2	= (DWORD) - 1;
		row.dwForwardMetric3	= (DWORD) - 1;
		row.dwForwardMetric4	= (DWORD) - 1;
		row.dwForwardMetric5	= (DWORD) - 1;

		addRoute = true;

		//
		// check to make sure we don't already have a route
		//
		if ( HaveRoute( &rowExtant, inet_addr( kLLNetworkAddr ), 0 ) )
		{
			//
			// set the age to 0 so that we can do a memcmp.
			//
			rowExtant.dwForwardAge = 0;

			//
			// check to see if this route is the same as our route
			//
			if (memcmp(&row, &rowExtant, sizeof(row)) != 0)
			{
				//
				// if it isn't then delete this entry
				//
				DeleteIpForwardEntry(&rowExtant);
			}
			else
			{
				//
				// else it is, so we don't want to create another route
				//
				addRoute = false;
			}
		}

		if (addRoute && row.dwForwardNextHop)
		{
			err = CreateIpForwardEntry(&row);
			check_noerr( err );
		}
	}

exit:

	return ( err );
}


//===========================================================================================================================
//	GetRouteDestination
//===========================================================================================================================

static OSStatus
GetRouteDestination(DWORD * ifIndex, DWORD * address)
{
	struct in_addr		ia;
	IP_ADAPTER_INFO	*	pAdapterInfo	=	NULL;
	IP_ADAPTER_INFO	*	pAdapter		=	NULL;
	ULONG				bufLen;
	mDNSBool			done			=	mDNSfalse;
	OSStatus			err;

	//
	// GetBestInterface will fail if there is no default gateway
	// configured.  If that happens, we will just take the first
	// interface in the list. MSDN support says there is no surefire
	// way to manually determine what the best interface might
	// be for a particular network address.
	//
	ia.s_addr	=	inet_addr(kLLNetworkAddr);
	err			=	GetBestInterface(*(IPAddr*) &ia, ifIndex);

	if (err)
	{
		*ifIndex = 0;
	}

	//
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the bufLen variable
	//
	err = GetAdaptersInfo( NULL, &bufLen);
	require_action( err == ERROR_BUFFER_OVERFLOW, exit, err = kUnknownErr );

	pAdapterInfo = (IP_ADAPTER_INFO*) malloc( bufLen );
	require_action( pAdapterInfo, exit, err = kNoMemoryErr );
	
	err = GetAdaptersInfo( pAdapterInfo, &bufLen);
	require_noerr( err, exit );
	
	pAdapter	=	pAdapterInfo;
	err			=	kUnknownErr;
			
	// <rdar://problem/3718122>
	// <rdar://problem/5652098>
	//
	// Look for the Nortel VPN virtual interface, along with Juniper virtual interface.
	//
	// If these interfaces are active (i.e., has a non-zero IP Address),
	// then we want to disable routing table modifications.

	while (pAdapter)
	{
		if ( ( IsNortelVPN( pAdapter ) || IsJuniperVPN( pAdapter ) || IsCiscoVPN( pAdapter ) ) &&
			 ( inet_addr( pAdapter->IpAddressList.IpAddress.String ) != 0 ) )
		{
			dlog( kDebugLevelTrace, DEBUG_NAME "disabling routing table management due to VPN incompatibility" );
			goto exit;
		}

		pAdapter = pAdapter->Next;
	}

	while ( !done )
	{
		pAdapter	=	pAdapterInfo;
		err			=	kUnknownErr;

		while (pAdapter)
		{
			// If we don't have an interface selected, choose the first one that is of type ethernet and
			// has a valid IP Address

			if ((pAdapter->Type == MIB_IF_TYPE_ETHERNET) && ( IsValidAddress( pAdapter->IpAddressList.IpAddress.String ) ) && (!(*ifIndex) || (pAdapter->Index == (*ifIndex))))
			{
				*address =	inet_addr( pAdapter->IpAddressList.IpAddress.String );
				*ifIndex =  pAdapter->Index;
				err		 =	kNoErr;
				break;
			}
		
			pAdapter = pAdapter->Next;
		}

		// If we found the right interface, or we weren't trying to find a specific interface then we're done

		if ( !err || !( *ifIndex) )
		{
			done = mDNStrue;
		}

		// Otherwise, try again by wildcarding the interface

		else
		{
			*ifIndex = 0;
		}
	} 

exit:

	if ( pAdapterInfo != NULL )
	{
		free( pAdapterInfo );
	}

	return( err );
}


static bool
IsNortelVPN( IP_ADAPTER_INFO * pAdapter )
{
	return ((pAdapter->Type == MIB_IF_TYPE_ETHERNET) &&
		    (pAdapter->AddressLength == 6) &&
		    (pAdapter->Address[0] == 0x44) &&
		    (pAdapter->Address[1] == 0x45) &&
		    (pAdapter->Address[2] == 0x53) &&
		    (pAdapter->Address[3] == 0x54) &&
		    (pAdapter->Address[4] == 0x42) &&
			(pAdapter->Address[5] == 0x00)) ? true : false;
}


static bool
IsJuniperVPN( IP_ADAPTER_INFO * pAdapter )
{	
	return ( strnistr( pAdapter->Description, "Juniper", sizeof( pAdapter->Description  ) ) != NULL ) ? true : false;
}


static bool
IsCiscoVPN( IP_ADAPTER_INFO * pAdapter )
{
	return ((pAdapter->Type == MIB_IF_TYPE_ETHERNET) &&
		    (pAdapter->AddressLength == 6) &&
		    (pAdapter->Address[0] == 0x00) &&
		    (pAdapter->Address[1] == 0x05) &&
		    (pAdapter->Address[2] == 0x9a) &&
		    (pAdapter->Address[3] == 0x3c) &&
		    (pAdapter->Address[4] == 0x7a) &&
			(pAdapter->Address[5] == 0x00)) ? true : false;
}


static const char *
strnistr( const char * string, const char * subString, size_t max )
{
	size_t       subStringLen;
	size_t       offset;
	size_t       maxOffset;
	size_t       stringLen;
	const char * pPos;

	if ( ( string == NULL ) || ( subString == NULL ) )
	{
		return string;
	}

	stringLen = ( max > strlen( string ) ) ? strlen( string ) : max;

	if ( stringLen == 0 )
	{
		return NULL;
	}
	
	subStringLen = strlen( subString );

	if ( subStringLen == 0 )
	{
		return string;
	}

	if ( subStringLen > stringLen )
	{
		return NULL;
	}

	maxOffset = stringLen - subStringLen;
	pPos      = string;

	for ( offset = 0; offset <= maxOffset; offset++ )
	{
		if ( _strnicmp( pPos, subString, subStringLen ) == 0 )
		{
			return pPos;
		}

		pPos++;
	}

	return NULL;
}

