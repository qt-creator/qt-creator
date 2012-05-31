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

	To Do:
	
	- Get unicode name of machine for nice name instead of just the host name.
	- Use the IPv6 Internet Connection Firewall API to allow IPv6 mDNS without manually changing the firewall.
	- Get DNS server address(es) from Windows and provide them to the uDNS layer.
	- Implement TCP support for truncated packets (only stubs now).	

*/

#define _CRT_RAND_S

#include	<stdarg.h>
#include	<stddef.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<crtdbg.h>
#include	<string.h>

#include	"Poll.h"
#include	"CommonServices.h"
#include	"DebugServices.h"
#include	"Firewall.h"
#include	"RegNames.h"
#include	"Secret.h"
#include	"dns_sd.h"

#include	<Iphlpapi.h>
#include	<mswsock.h>
#include	<process.h>
#include	<ntsecapi.h>
#include	<lm.h>
#include	<winioctl.h>
#include	<ntddndis.h>        // This defines the IOCTL constants.

#include	"mDNSEmbeddedAPI.h"
#include	"GenLinkedList.h"
#include	"DNSCommon.h"
#include	"mDNSWin32.h"

#if 0
#pragma mark == Constants ==
#endif

//===========================================================================================================================
//	Constants
//===========================================================================================================================

#define	DEBUG_NAME									"[mDNSWin32] "

#define	MDNS_WINDOWS_USE_IPV6_IF_ADDRS				1
#define	MDNS_WINDOWS_ENABLE_IPV4					1
#define	MDNS_WINDOWS_ENABLE_IPV6					1
#define	MDNS_FIX_IPHLPAPI_PREFIX_BUG				1
#define MDNS_SET_HINFO_STRINGS						0

#define	kMDNSDefaultName							"My Computer"

#define	kWinSockMajorMin							2
#define	kWinSockMinorMin							2

#define kRegistryMaxKeyLength						255
#define kRegistryMaxValueName						16383

static GUID											kWSARecvMsgGUID = WSAID_WSARECVMSG;

#define kIPv6IfIndexBase							(10000000L)
#define SMBPortAsNumber								445
#define DEVICE_PREFIX								"\\\\.\\"

#if 0
#pragma mark == Prototypes ==
#endif

//===========================================================================================================================
//	Prototypes
//===========================================================================================================================

mDNSlocal mStatus			SetupNiceName( mDNS * const inMDNS );
mDNSlocal mStatus			SetupHostName( mDNS * const inMDNS );
mDNSlocal mStatus			SetupName( mDNS * const inMDNS );
mDNSlocal mStatus			SetupInterface( mDNS * const inMDNS, const struct ifaddrs *inIFA, mDNSInterfaceData **outIFD );
mDNSlocal mStatus			TearDownInterface( mDNS * const inMDNS, mDNSInterfaceData *inIFD );
mDNSlocal void CALLBACK		FreeInterface( mDNSInterfaceData *inIFD );
mDNSlocal mStatus			SetupSocket( mDNS * const inMDNS, const struct sockaddr *inAddr, mDNSIPPort port, SocketRef *outSocketRef  );
mDNSlocal mStatus			SockAddrToMDNSAddr( const struct sockaddr * const inSA, mDNSAddr *outIP, mDNSIPPort *outPort );
mDNSlocal OSStatus			GetWindowsVersionString( char *inBuffer, size_t inBufferSize );
mDNSlocal int				getifaddrs( struct ifaddrs **outAddrs );
mDNSlocal void				freeifaddrs( struct ifaddrs *inAddrs );



// Platform Accessors

#ifdef	__cplusplus
	extern "C" {
#endif

typedef struct mDNSPlatformInterfaceInfo	mDNSPlatformInterfaceInfo;
struct	mDNSPlatformInterfaceInfo
{
	const char *		name;
	mDNSAddr			ip;
};


mDNSexport mStatus	mDNSPlatformInterfaceNameToID( mDNS * const inMDNS, const char *inName, mDNSInterfaceID *outID );
mDNSexport mStatus	mDNSPlatformInterfaceIDToInfo( mDNS * const inMDNS, mDNSInterfaceID inID, mDNSPlatformInterfaceInfo *outInfo );


// Wakeup Structs

#define kUnicastWakeupNumTries				( 1 )
#define kUnicastWakeupSleepBetweenTries		( 0 )
#define kMulticastWakeupNumTries			( 18 )
#define kMulticastWakeupSleepBetweenTries	( 100 )

typedef struct MulticastWakeupStruct
{
	mDNS					*inMDNS;
	struct sockaddr_in		addr;
	INT						addrLen;
	unsigned char			data[ 102 ];
	INT						dataLen;
	INT						numTries;
	INT						msecSleep;
} MulticastWakeupStruct;


// Utilities

#if( MDNS_WINDOWS_USE_IPV6_IF_ADDRS )
	mDNSlocal int	getifaddrs_ipv6( struct ifaddrs **outAddrs );
#endif

mDNSlocal int getifaddrs_ipv4( struct ifaddrs **outAddrs );


mDNSlocal DWORD				GetPrimaryInterface();
mDNSlocal mStatus			AddressToIndexAndMask( struct sockaddr * address, uint32_t * index, struct sockaddr * mask );
mDNSlocal mDNSBool			CanReceiveUnicast( void );
mDNSlocal mDNSBool			IsPointToPoint( IP_ADAPTER_UNICAST_ADDRESS * addr );

mDNSlocal mStatus			StringToAddress( mDNSAddr * ip, LPSTR string );
mDNSlocal mStatus			RegQueryString( HKEY key, LPCSTR param, LPSTR * string, DWORD * stringLen, DWORD * enabled );
mDNSlocal struct ifaddrs*	myGetIfAddrs(int refresh);
mDNSlocal OSStatus			TCHARtoUTF8( const TCHAR *inString, char *inBuffer, size_t inBufferSize );
mDNSlocal OSStatus			WindowsLatin1toUTF8( const char *inString, char *inBuffer, size_t inBufferSize );
mDNSlocal void CALLBACK		TCPSocketNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context );
mDNSlocal void				TCPCloseSocket( TCPSocket * socket );
mDNSlocal void CALLBACK		UDPSocketNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context );
mDNSlocal void				UDPCloseSocket( UDPSocket * sock );
mDNSlocal mStatus           SetupAddr(mDNSAddr *ip, const struct sockaddr *const sa);
mDNSlocal void				GetDDNSFQDN( domainname *const fqdn );
#ifdef UNICODE
mDNSlocal void				GetDDNSDomains( DNameListElem ** domains, LPCWSTR lpSubKey );
#else
mDNSlocal void				GetDDNSDomains( DNameListElem ** domains, LPCSTR lpSubKey );
#endif
mDNSlocal void				SetDomainSecrets( mDNS * const inMDNS );
mDNSlocal void				SetDomainSecret( mDNS * const m, const domainname * inDomain );
mDNSlocal VOID CALLBACK		CheckFileSharesProc( LPVOID arg, DWORD dwTimerLowValue, DWORD dwTimerHighValue );
mDNSlocal void				CheckFileShares( mDNS * const inMDNS );
mDNSlocal void				SMBCallback(mDNS *const m, ServiceRecordSet *const srs, mStatus result);
mDNSlocal mDNSu8			IsWOMPEnabledForAdapter( const char * adapterName );
mDNSlocal void				SendWakeupPacket( mDNS * const inMDNS, LPSOCKADDR addr, INT addrlen, const char * buf, INT buflen, INT numTries, INT msecSleep );
mDNSlocal void _cdecl		SendMulticastWakeupPacket( void *arg );

#ifdef	__cplusplus
	}
#endif

#if 0
#pragma mark == Globals ==
#endif

//===========================================================================================================================
//	Globals
//===========================================================================================================================

mDNSlocal mDNS_PlatformSupport	gMDNSPlatformSupport;
mDNSs32							mDNSPlatformOneSecond	= 0;
mDNSlocal UDPSocket		*		gUDPSockets				= NULL;
mDNSlocal int					gUDPNumSockets			= 0;
mDNSlocal BOOL					gEnableIPv6				= TRUE;

#if( MDNS_WINDOWS_USE_IPV6_IF_ADDRS )

	typedef DWORD
		( WINAPI * GetAdaptersAddressesFunctionPtr )( 
			ULONG 					inFamily, 
			DWORD 					inFlags, 
			PVOID 					inReserved, 
			PIP_ADAPTER_ADDRESSES 	inAdapter, 
			PULONG					outBufferSize );

	mDNSlocal HMODULE								gIPHelperLibraryInstance			= NULL;
	mDNSlocal GetAdaptersAddressesFunctionPtr		gGetAdaptersAddressesFunctionPtr	= NULL;

#endif


#ifndef HCRYPTPROV
   typedef ULONG_PTR HCRYPTPROV;    // WinCrypt.h, line 249
#endif


#ifndef CRYPT_MACHINE_KEYSET
#	define CRYPT_MACHINE_KEYSET    0x00000020
#endif

#ifndef CRYPT_NEWKEYSET
#	define CRYPT_NEWKEYSET         0x00000008
#endif

#ifndef PROV_RSA_FULL
#  define PROV_RSA_FULL 1
#endif

typedef BOOL (__stdcall *fnCryptGenRandom)( HCRYPTPROV, DWORD, BYTE* ); 
typedef BOOL (__stdcall *fnCryptAcquireContext)( HCRYPTPROV*, LPCTSTR, LPCTSTR, DWORD, DWORD);
typedef BOOL (__stdcall *fnCryptReleaseContext)(HCRYPTPROV, DWORD);

static fnCryptAcquireContext g_lpCryptAcquireContext 	= NULL;
static fnCryptReleaseContext g_lpCryptReleaseContext 	= NULL;
static fnCryptGenRandom		 g_lpCryptGenRandom 		= NULL;
static HINSTANCE			 g_hAAPI32 					= NULL;
static HCRYPTPROV			 g_hProvider 				= ( ULONG_PTR ) NULL;


typedef DNSServiceErrorType ( DNSSD_API *DNSServiceRegisterFunc )
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,         /* may be NULL */
    const char                          *regtype,
    const char                          *domain,       /* may be NULL */
    const char                          *host,         /* may be NULL */
    uint16_t                            port,
    uint16_t                            txtLen,
    const void                          *txtRecord,    /* may be NULL */
    DNSServiceRegisterReply             callBack,      /* may be NULL */
    void                                *context       /* may be NULL */
    );


typedef void ( DNSSD_API *DNSServiceRefDeallocateFunc )( DNSServiceRef sdRef );

mDNSlocal HMODULE					gDNSSDLibrary				= NULL;
mDNSlocal DNSServiceRegisterFunc	gDNSServiceRegister			= NULL;
mDNSlocal DNSServiceRefDeallocateFunc gDNSServiceRefDeallocate	= NULL;
mDNSlocal HANDLE					gSMBThread					= NULL;
mDNSlocal HANDLE					gSMBThreadRegisterEvent		= NULL;
mDNSlocal HANDLE					gSMBThreadDeregisterEvent	= NULL;
mDNSlocal HANDLE					gSMBThreadStopEvent			= NULL;
mDNSlocal HANDLE					gSMBThreadQuitEvent			= NULL;

#define	kSMBStopEvent				( WAIT_OBJECT_0 + 0 )
#define	kSMBRegisterEvent			( WAIT_OBJECT_0 + 1 )
#define kSMBDeregisterEvent			( WAIT_OBJECT_0 + 2 )


#if 0
#pragma mark -
#pragma mark == Platform Support ==
#endif

//===========================================================================================================================
//	mDNSPlatformInit
//===========================================================================================================================

mDNSexport mStatus	mDNSPlatformInit( mDNS * const inMDNS )
{
	mStatus		err;
	OSVERSIONINFO osInfo;
	BOOL ok;
	WSADATA		wsaData;
	int			supported;
	struct sockaddr_in	sa4;
	struct sockaddr_in6 sa6;
	int					sa4len;
	int					sa6len;
	DWORD				size;
	
	dlog( kDebugLevelTrace, DEBUG_NAME "platform init\n" );
	
	// Initialize variables. If the PlatformSupport pointer is not null then just assume that a non-Apple client is 
	// calling mDNS_Init and wants to provide its own storage for the platform-specific data so do not overwrite it.
	
	mDNSPlatformMemZero( &gMDNSPlatformSupport, sizeof( gMDNSPlatformSupport ) );
	if( !inMDNS->p ) inMDNS->p				= &gMDNSPlatformSupport;
	inMDNS->p->mainThread					= OpenThread( THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId() );
	require_action( inMDNS->p->mainThread, exit, err = mStatus_UnknownErr );
	inMDNS->p->checkFileSharesTimer = CreateWaitableTimer( NULL, FALSE, NULL );
	require_action( inMDNS->p->checkFileSharesTimer, exit, err = mStatus_UnknownErr );
	inMDNS->p->checkFileSharesTimeout		= 10;		// Retry time for CheckFileShares() in seconds
	mDNSPlatformOneSecond 					= 1000;		// Use milliseconds as the quantum of time
	
	// Get OS version info
	
	osInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	ok = GetVersionEx( &osInfo );
	err = translate_errno( ok, (OSStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	inMDNS->p->osMajorVersion = osInfo.dwMajorVersion;
	inMDNS->p->osMinorVersion = osInfo.dwMinorVersion;
	
	// Don't enable IPv6 on anything less recent than Windows Vista

	if ( inMDNS->p->osMajorVersion < 6 )
	{
		gEnableIPv6 = FALSE;
	}

	// Startup WinSock 2.2 or later.
	
	err = WSAStartup( MAKEWORD( kWinSockMajorMin, kWinSockMinorMin ), &wsaData );
	require_noerr( err, exit );
	
	supported = ( ( LOBYTE( wsaData.wVersion ) == kWinSockMajorMin ) && ( HIBYTE( wsaData.wVersion ) == kWinSockMinorMin ) );
	require_action( supported, exit, err = mStatus_UnsupportedErr );
	
	inMDNS->CanReceiveUnicastOn5353 = CanReceiveUnicast();
	
	// Setup the HINFO HW strings.
	//<rdar://problem/7245119> device-info should have model=Windows

	strcpy_s( ( char* ) &inMDNS->HIHardware.c[ 1 ], sizeof( inMDNS->HIHardware.c ) - 2, "Windows" );
	inMDNS->HIHardware.c[ 0 ] = ( mDNSu8 ) mDNSPlatformStrLen( &inMDNS->HIHardware.c[ 1 ] );
	dlog( kDebugLevelInfo, DEBUG_NAME "HIHardware: %#s\n", inMDNS->HIHardware.c );

	// Setup the HINFO SW strings.
#if ( MDNS_SET_HINFO_STRINGS )
	mDNS_snprintf( (char *) &inMDNS->HISoftware.c[ 1 ], sizeof( inMDNS->HISoftware.c ) - 2, 
		"mDNSResponder (%s %s)", __DATE__, __TIME__ );
	inMDNS->HISoftware.c[ 0 ] = (mDNSu8) mDNSPlatformStrLen( &inMDNS->HISoftware.c[ 1 ] );
	dlog( kDebugLevelInfo, DEBUG_NAME "HISoftware: %#s\n", inMDNS->HISoftware.c );
#endif

	// Set up the IPv4 unicast socket

	inMDNS->p->unicastSock4.fd			= INVALID_SOCKET;
	inMDNS->p->unicastSock4.recvMsgPtr	= NULL;
	inMDNS->p->unicastSock4.ifd			= NULL;
	inMDNS->p->unicastSock4.next		= NULL;
	inMDNS->p->unicastSock4.m			= inMDNS;

#if ( MDNS_WINDOWS_ENABLE_IPV4 )

	sa4.sin_family		= AF_INET;
	sa4.sin_addr.s_addr = INADDR_ANY;
	err = SetupSocket( inMDNS, (const struct sockaddr*) &sa4, zeroIPPort, &inMDNS->p->unicastSock4.fd );
	check_noerr( err );
	sa4len = sizeof( sa4 );
	err = getsockname( inMDNS->p->unicastSock4.fd, (struct sockaddr*) &sa4, &sa4len );
	require_noerr( err, exit );
	inMDNS->p->unicastSock4.port.NotAnInteger = sa4.sin_port;
	inMDNS->UnicastPort4 = inMDNS->p->unicastSock4.port;
	err = WSAIoctl( inMDNS->p->unicastSock4.fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &kWSARecvMsgGUID, sizeof( kWSARecvMsgGUID ), &inMDNS->p->unicastSock4.recvMsgPtr, sizeof( inMDNS->p->unicastSock4.recvMsgPtr ), &size, NULL, NULL );
		
	if ( err )
	{
		inMDNS->p->unicastSock4.recvMsgPtr = NULL;
	}

	err = mDNSPollRegisterSocket( inMDNS->p->unicastSock4.fd, FD_READ, UDPSocketNotification, &inMDNS->p->unicastSock4 );
	require_noerr( err, exit ); 

#endif

	// Set up the IPv6 unicast socket

	inMDNS->p->unicastSock6.fd			= INVALID_SOCKET;
	inMDNS->p->unicastSock6.recvMsgPtr	= NULL;
	inMDNS->p->unicastSock6.ifd			= NULL;
	inMDNS->p->unicastSock6.next		= NULL;
	inMDNS->p->unicastSock6.m			= inMDNS;

#if ( MDNS_WINDOWS_ENABLE_IPV6 )

	if ( gEnableIPv6 )
	{
		sa6.sin6_family		= AF_INET6;
		sa6.sin6_addr		= in6addr_any;
		sa6.sin6_scope_id	= 0;

		// This call will fail if the machine hasn't installed IPv6.  In that case,
		// the error will be WSAEAFNOSUPPORT.

		err = SetupSocket( inMDNS, (const struct sockaddr*) &sa6, zeroIPPort, &inMDNS->p->unicastSock6.fd );
		require_action( !err || ( err == WSAEAFNOSUPPORT ), exit, err = (mStatus) WSAGetLastError() );
		err = kNoErr;
		
		// If we weren't able to create the socket (because IPv6 hasn't been installed) don't do this

		if ( inMDNS->p->unicastSock6.fd != INVALID_SOCKET )
		{
			sa6len = sizeof( sa6 );
			err = getsockname( inMDNS->p->unicastSock6.fd, (struct sockaddr*) &sa6, &sa6len );
			require_noerr( err, exit );
			inMDNS->p->unicastSock6.port.NotAnInteger = sa6.sin6_port;
			inMDNS->UnicastPort6 = inMDNS->p->unicastSock6.port;

			err = WSAIoctl( inMDNS->p->unicastSock6.fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &kWSARecvMsgGUID, sizeof( kWSARecvMsgGUID ), &inMDNS->p->unicastSock6.recvMsgPtr, sizeof( inMDNS->p->unicastSock6.recvMsgPtr ), &size, NULL, NULL );
			
			if ( err != 0 )
			{
				inMDNS->p->unicastSock6.recvMsgPtr = NULL;
			}

			err = mDNSPollRegisterSocket( inMDNS->p->unicastSock6.fd, FD_READ, UDPSocketNotification, &inMDNS->p->unicastSock6 );
			require_noerr( err, exit );
		}
	}

#endif

	// Notify core of domain secret keys

	SetDomainSecrets( inMDNS );
	
	// Success!

	mDNSCoreInitComplete( inMDNS, err );

	
exit:

	if ( err )
	{
		mDNSPlatformClose( inMDNS );
	}

	dlog( kDebugLevelTrace, DEBUG_NAME "platform init done (err=%d %m)\n", err, err );
	return( err );
}

//===========================================================================================================================
//	mDNSPlatformClose
//===========================================================================================================================

mDNSexport void	mDNSPlatformClose( mDNS * const inMDNS )
{
	mStatus		err;
	
	dlog( kDebugLevelTrace, DEBUG_NAME "platform close\n" );
	check( inMDNS );

	if ( gSMBThread != NULL )
	{
		dlog( kDebugLevelTrace, DEBUG_NAME "tearing down smb registration thread\n" );
		SetEvent( gSMBThreadStopEvent );
		
		if ( WaitForSingleObject( gSMBThreadQuitEvent, 5 * 1000 ) == WAIT_OBJECT_0 )
		{
			if ( gSMBThreadQuitEvent )
			{
				CloseHandle( gSMBThreadQuitEvent );
				gSMBThreadQuitEvent = NULL;
			}

			if ( gSMBThreadStopEvent )
			{
				CloseHandle( gSMBThreadStopEvent );
				gSMBThreadStopEvent = NULL;
			}

			if ( gSMBThreadDeregisterEvent )
			{
				CloseHandle( gSMBThreadDeregisterEvent );
				gSMBThreadDeregisterEvent = NULL;
			}

			if ( gSMBThreadRegisterEvent )
			{
				CloseHandle( gSMBThreadRegisterEvent );
				gSMBThreadRegisterEvent = NULL;
			}

			if ( gDNSSDLibrary )
			{
				FreeLibrary( gDNSSDLibrary );
				gDNSSDLibrary = NULL;
			}	
		}
		else
		{
			LogMsg( "Unable to stop SMBThread" );
		}

		inMDNS->p->smbFileSharing = mDNSfalse;
		inMDNS->p->smbPrintSharing = mDNSfalse;
	}

	// Tear everything down in reverse order to how it was set up.
	
	err = TearDownInterfaceList( inMDNS );
	check_noerr( err );
	check( !inMDNS->p->inactiveInterfaceList );

#if ( MDNS_WINDOWS_ENABLE_IPV4 )

	UDPCloseSocket( &inMDNS->p->unicastSock4 );

#endif
	
#if ( MDNS_WINDOWS_ENABLE_IPV6 )

	if ( gEnableIPv6 )
	{
		UDPCloseSocket( &inMDNS->p->unicastSock6 );
	}

#endif

	// Free the DLL needed for IPv6 support.
	
#if( MDNS_WINDOWS_USE_IPV6_IF_ADDRS )
	if( gIPHelperLibraryInstance )
	{
		gGetAdaptersAddressesFunctionPtr = NULL;
		
		FreeLibrary( gIPHelperLibraryInstance );
		gIPHelperLibraryInstance = NULL;
	}
#endif

	if ( g_hAAPI32 )
	{
		// Release any resources

		if ( g_hProvider && g_lpCryptReleaseContext )
		{
			( g_lpCryptReleaseContext )( g_hProvider, 0 );
		}

		// Free the AdvApi32.dll

		FreeLibrary( g_hAAPI32 );

		// And reset all the data

		g_lpCryptAcquireContext = NULL;
		g_lpCryptReleaseContext = NULL;
		g_lpCryptGenRandom 		= NULL;
		g_hProvider 			= ( ULONG_PTR ) NULL;
		g_hAAPI32				= NULL;
	}

	WSACleanup();
	
	dlog( kDebugLevelTrace, DEBUG_NAME "platform close done\n" );
}


//===========================================================================================================================
//	mDNSPlatformLock
//===========================================================================================================================

mDNSexport void	mDNSPlatformLock( const mDNS * const inMDNS )
{
	( void ) inMDNS;
}

//===========================================================================================================================
//	mDNSPlatformUnlock
//===========================================================================================================================

mDNSexport void	mDNSPlatformUnlock( const mDNS * const inMDNS )
{
	( void ) inMDNS;
}

//===========================================================================================================================
//	mDNSPlatformStrCopy
//===========================================================================================================================

mDNSexport void	mDNSPlatformStrCopy( void *inDst, const void *inSrc )
{
	check( inSrc );
	check( inDst );
	
	strcpy( (char *) inDst, (const char*) inSrc );
}

//===========================================================================================================================
//	mDNSPlatformStrLen
//===========================================================================================================================

mDNSexport mDNSu32	mDNSPlatformStrLen( const void *inSrc )
{
	check( inSrc );
	
	return( (mDNSu32) strlen( (const char *) inSrc ) );
}

//===========================================================================================================================
//	mDNSPlatformMemCopy
//===========================================================================================================================

mDNSexport void	mDNSPlatformMemCopy( void *inDst, const void *inSrc, mDNSu32 inSize )
{
	check( inSrc );
	check( inDst );
	
	memcpy( inDst, inSrc, inSize );
}

//===========================================================================================================================
//	mDNSPlatformMemSame
//===========================================================================================================================

mDNSexport mDNSBool	mDNSPlatformMemSame( const void *inDst, const void *inSrc, mDNSu32 inSize )
{
	check( inSrc );
	check( inDst );
	
	return( (mDNSBool)( memcmp( inSrc, inDst, inSize ) == 0 ) );
}

//===========================================================================================================================
//	mDNSPlatformMemZero
//===========================================================================================================================

mDNSexport void	mDNSPlatformMemZero( void *inDst, mDNSu32 inSize )
{
	check( inDst );
	
	memset( inDst, 0, inSize );
}

//===========================================================================================================================
//	mDNSPlatformMemAllocate
//===========================================================================================================================

mDNSexport void *	mDNSPlatformMemAllocate( mDNSu32 inSize )
{
	void *		mem;
	
	check( inSize > 0 );
	
	mem = malloc( inSize );
	check( mem );
	
	return( mem );
}

//===========================================================================================================================
//	mDNSPlatformMemFree
//===========================================================================================================================

mDNSexport void	mDNSPlatformMemFree( void *inMem )
{
	check( inMem );
	
	free( inMem );
}

//===========================================================================================================================
//	mDNSPlatformRandomNumber
//===========================================================================================================================

mDNSexport mDNSu32 mDNSPlatformRandomNumber(void)
{
	unsigned int	randomNumber;
	errno_t			err;

	err = rand_s( &randomNumber );
	require_noerr( err, exit );

exit:

	if ( err )
	{
		randomNumber = rand();
	}

	return ( mDNSu32 ) randomNumber;
}

//===========================================================================================================================
//	mDNSPlatformTimeInit
//===========================================================================================================================

mDNSexport mStatus	mDNSPlatformTimeInit( void )
{
	// No special setup is required on Windows -- we just use GetTickCount().
	return( mStatus_NoError );
}

//===========================================================================================================================
//	mDNSPlatformRawTime
//===========================================================================================================================

mDNSexport mDNSs32	mDNSPlatformRawTime( void )
{
	return( (mDNSs32) GetTickCount() );
}

//===========================================================================================================================
//	mDNSPlatformUTC
//===========================================================================================================================

mDNSexport mDNSs32	mDNSPlatformUTC( void )
{
	return ( mDNSs32 ) time( NULL );
}

//===========================================================================================================================
//	mDNSPlatformInterfaceNameToID
//===========================================================================================================================

mDNSexport mStatus	mDNSPlatformInterfaceNameToID( mDNS * const inMDNS, const char *inName, mDNSInterfaceID *outID )
{
	mStatus					err;
	mDNSInterfaceData *		ifd;
	
	check( inMDNS );
	check( inMDNS->p );
	check( inName );
	
	// Search for an interface with the specified name,
	
	for( ifd = inMDNS->p->interfaceList; ifd; ifd = ifd->next )
	{
		if( strcmp( ifd->name, inName ) == 0 )
		{
			break;
		}
	}
	require_action_quiet( ifd, exit, err = mStatus_NoSuchNameErr );
	
	// Success!
	
	if( outID )
	{
		*outID = (mDNSInterfaceID) ifd;
	}
	err = mStatus_NoError;
	
exit:
	return( err );
}

//===========================================================================================================================
//	mDNSPlatformInterfaceIDToInfo
//===========================================================================================================================

mDNSexport mStatus	mDNSPlatformInterfaceIDToInfo( mDNS * const inMDNS, mDNSInterfaceID inID, mDNSPlatformInterfaceInfo *outInfo )
{
	mStatus					err;
	mDNSInterfaceData *		ifd;
	
	check( inMDNS );
	check( inID );
	check( outInfo );
	
	// Search for an interface with the specified ID,
	
	for( ifd = inMDNS->p->interfaceList; ifd; ifd = ifd->next )
	{
		if( ifd == (mDNSInterfaceData *) inID )
		{
			break;
		}
	}
	require_action_quiet( ifd, exit, err = mStatus_NoSuchNameErr );
	
	// Success!
	
	outInfo->name 	= ifd->name;
	outInfo->ip 	= ifd->interfaceInfo.ip;
	err 			= mStatus_NoError;
	
exit:
	return( err );
}

//===========================================================================================================================
//	mDNSPlatformInterfaceIDfromInterfaceIndex
//===========================================================================================================================

mDNSexport mDNSInterfaceID	mDNSPlatformInterfaceIDfromInterfaceIndex( mDNS * const inMDNS, mDNSu32 inIndex )
{
	mDNSInterfaceID		id;
	
	id = mDNSNULL;
	if( inIndex == kDNSServiceInterfaceIndexLocalOnly )
	{
		id = mDNSInterface_LocalOnly;
	}
	else if( inIndex != 0 )
	{
		mDNSInterfaceData *		ifd;
		
		for( ifd = inMDNS->p->interfaceList; ifd; ifd = ifd->next )
		{
			if( ( ifd->scopeID == inIndex ) && ifd->interfaceInfo.InterfaceActive )
			{
				id = ifd->interfaceInfo.InterfaceID;
				break;
			}
		}
		check( ifd );
	}
	return( id );
}

//===========================================================================================================================
//	mDNSPlatformInterfaceIndexfromInterfaceID
//===========================================================================================================================
	
mDNSexport mDNSu32	mDNSPlatformInterfaceIndexfromInterfaceID( mDNS * const inMDNS, mDNSInterfaceID inID, mDNSBool suppressNetworkChange )
{
	mDNSu32		index;
	
	(void) suppressNetworkChange;

	index = 0;
	if( inID == mDNSInterface_LocalOnly )
	{
		index = (mDNSu32) kDNSServiceInterfaceIndexLocalOnly;
	}
	else if( inID )
	{
		mDNSInterfaceData *		ifd;
		
		// Search active interfaces.
		for( ifd = inMDNS->p->interfaceList; ifd; ifd = ifd->next )
		{
			if( (mDNSInterfaceID) ifd == inID )
			{
				index = ifd->scopeID;
				break;
			}
		}
		
		// Search inactive interfaces too so remove events for inactive interfaces report the old interface index.
		
		if( !ifd )
		{
			for( ifd = inMDNS->p->inactiveInterfaceList; ifd; ifd = ifd->next )
			{
				if( (mDNSInterfaceID) ifd == inID )
				{
					index = ifd->scopeID;
					break;
				}
			}
		}
		check( ifd );
	}
	return( index );
}


//===========================================================================================================================
//	mDNSPlatformTCPSocket
//===========================================================================================================================

TCPSocket *
mDNSPlatformTCPSocket
	(
	mDNS			* const m,
	TCPSocketFlags		flags,
	mDNSIPPort			*	port 
	)
{
	TCPSocket *		sock    = NULL;
	u_long				on		= 1;  // "on" for setsockopt
	struct sockaddr_in	saddr;
	int					len;
	mStatus				err		= mStatus_NoError;

	DEBUG_UNUSED( m );

	require_action( flags == 0, exit, err = mStatus_UnsupportedErr );

	// Setup connection data object

	sock = (TCPSocket *) malloc( sizeof( TCPSocket ) );
	require_action( sock, exit, err = mStatus_NoMemoryErr );
	mDNSPlatformMemZero( sock, sizeof( TCPSocket ) );
	sock->fd		= INVALID_SOCKET;
	sock->flags		= flags;
	sock->m			= m;

	mDNSPlatformMemZero(&saddr, sizeof(saddr));
	saddr.sin_family		= AF_INET;
	saddr.sin_addr.s_addr	= htonl( INADDR_ANY );
	saddr.sin_port			= port->NotAnInteger;
	
	// Create the socket

	sock->fd = socket(AF_INET, SOCK_STREAM, 0);
	err = translate_errno( sock->fd != INVALID_SOCKET, WSAGetLastError(), mStatus_UnknownErr );
	require_noerr( err, exit );

	// bind

	err = bind( sock->fd, ( struct sockaddr* ) &saddr, sizeof( saddr )  );
	err = translate_errno( err == 0, WSAGetLastError(), mStatus_UnknownErr );
	require_noerr( err, exit );

	// Set it to be non-blocking

	err = ioctlsocket( sock->fd, FIONBIO, &on );
	err = translate_errno( err == 0, WSAGetLastError(), mStatus_UnknownErr );
	require_noerr( err, exit );

	// Get port number

	mDNSPlatformMemZero( &saddr, sizeof( saddr ) );
	len = sizeof( saddr );

	err = getsockname( sock->fd, ( struct sockaddr* ) &saddr, &len );
	err = translate_errno( err == 0, WSAGetLastError(), mStatus_UnknownErr );
	require_noerr( err, exit );

	port->NotAnInteger = saddr.sin_port;

exit:

	if ( err && sock )
	{
		TCPCloseSocket( sock );
		free( sock );
		sock = mDNSNULL;
	}

	return sock;
}

//===========================================================================================================================
//	mDNSPlatformTCPConnect
//===========================================================================================================================

mStatus
mDNSPlatformTCPConnect
	(
	TCPSocket			*	sock,
	const mDNSAddr		*	inDstIP, 
	mDNSOpaque16 			inDstPort, 
	domainname			*	hostname,
	mDNSInterfaceID			inInterfaceID,
	TCPConnectionCallback	inCallback, 
	void *					inContext
	)
{
	struct sockaddr_in	saddr;
	mStatus				err		= mStatus_NoError;

	DEBUG_UNUSED( hostname );
	DEBUG_UNUSED( inInterfaceID );

	if ( inDstIP->type != mDNSAddrType_IPv4 )
	{
		LogMsg("ERROR: mDNSPlatformTCPConnect - attempt to connect to an IPv6 address: operation not supported");
		return mStatus_UnknownErr;
	}

	// Setup connection data object

	sock->userCallback	= inCallback;
	sock->userContext	= inContext;

	mDNSPlatformMemZero(&saddr, sizeof(saddr));
	saddr.sin_family	= AF_INET;
	saddr.sin_port		= inDstPort.NotAnInteger;
	memcpy(&saddr.sin_addr, &inDstIP->ip.v4.NotAnInteger, sizeof(saddr.sin_addr));

	// Try and do connect

	err = connect( sock->fd, ( struct sockaddr* ) &saddr, sizeof( saddr ) );
	require_action( !err || ( WSAGetLastError() == WSAEWOULDBLOCK ), exit, err = mStatus_ConnFailed );
	sock->connected	= !err ? TRUE : FALSE;

	err = mDNSPollRegisterSocket( sock->fd, FD_CONNECT | FD_READ | FD_CLOSE, TCPSocketNotification, sock );
	require_noerr( err, exit );

exit:

	if ( !err )
	{
		err = sock->connected ? mStatus_ConnEstablished : mStatus_ConnPending;
	}

	return err;
}


//===========================================================================================================================
//	mDNSPlatformTCPAccept
//===========================================================================================================================

mDNSexport 
mDNSexport TCPSocket *mDNSPlatformTCPAccept( TCPSocketFlags flags, int fd )
	{
	TCPSocket	*	sock = NULL;
	mStatus							err = mStatus_NoError;

	require_action( !flags, exit, err = mStatus_UnsupportedErr );

	sock = malloc( sizeof( TCPSocket ) );
	require_action( sock, exit, err = mStatus_NoMemoryErr );
	
	mDNSPlatformMemZero( sock, sizeof( *sock ) );

	sock->fd	= fd;
	sock->flags = flags;

exit:

	if ( err && sock )
	{
		free( sock );
		sock = NULL;
	}

	return sock;
	}


//===========================================================================================================================
//	mDNSPlatformTCPCloseConnection
//===========================================================================================================================

mDNSexport void	mDNSPlatformTCPCloseConnection( TCPSocket *sock )
{
	check( sock );

	if ( sock )
	{
		dlog( kDebugLevelChatty, DEBUG_NAME "mDNSPlatformTCPCloseConnection 0x%x:%d\n", sock, sock->fd );

		if ( sock->fd != INVALID_SOCKET )
		{
			mDNSPollUnregisterSocket( sock->fd );
			closesocket( sock->fd );
			sock->fd = INVALID_SOCKET;
		}

		free( sock );
	}
}


//===========================================================================================================================
//	mDNSPlatformReadTCP
//===========================================================================================================================

mDNSexport long	mDNSPlatformReadTCP( TCPSocket *sock, void *inBuffer, unsigned long inBufferSize, mDNSBool * closed )
{
	int			nread;
    OSStatus    err;

	*closed = mDNSfalse;
    nread = recv( sock->fd, inBuffer, inBufferSize, 0 );
    err = translate_errno( ( nread >= 0 ), WSAGetLastError(), mStatus_UnknownErr );
	
	if ( nread > 0 )
	{
		dlog( kDebugLevelChatty, DEBUG_NAME "mDNSPlatformReadTCP: 0x%x:%d read %d bytes\n", sock, sock->fd, nread );
	}
	else if ( !nread )
	{
		*closed = mDNStrue;
	}
	else if ( err == WSAECONNRESET )
	{
		*closed = mDNStrue;
		nread = 0;
	}
	else if ( err == WSAEWOULDBLOCK )
	{
		nread = 0;
	}
	else
	{
		LogMsg( "ERROR: mDNSPlatformReadTCP - recv: %d\n", err );
		nread = -1;
	}

    return nread;
}


//===========================================================================================================================
//	mDNSPlatformWriteTCP
//===========================================================================================================================

mDNSexport long	mDNSPlatformWriteTCP( TCPSocket *sock, const char *inMsg, unsigned long inMsgSize )
{
	int			nsent;
	OSStatus	err;

	nsent = send( sock->fd, inMsg, inMsgSize, 0 );

	err = translate_errno( ( nsent >= 0 ) || ( WSAGetLastError() == WSAEWOULDBLOCK ), WSAGetLastError(), mStatus_UnknownErr );
	require_noerr( err, exit );

	if ( nsent < 0)
	{
		nsent = 0;
	}
		
exit:

	return nsent;
}

//===========================================================================================================================
//	mDNSPlatformTCPGetFD
//===========================================================================================================================

mDNSexport int mDNSPlatformTCPGetFD(TCPSocket *sock )
{
	return ( int ) sock->fd;
}



//===========================================================================================================================
//	TCPSocketNotification
//===========================================================================================================================

mDNSlocal void CALLBACK
TCPSocketNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context )
{
	TCPSocket				*tcpSock = ( TCPSocket* ) context;
	TCPConnectionCallback	callback;
	int						err;

	DEBUG_UNUSED( sock );

	require_action( tcpSock, exit, err = mStatus_BadParamErr );
	callback = ( TCPConnectionCallback ) tcpSock->userCallback;
	require_action( callback, exit, err = mStatus_BadParamErr );

	if ( event && ( event->lNetworkEvents & FD_CONNECT ) )
	{
		if ( event->iErrorCode[ FD_CONNECT_BIT ] == 0 )
		{
			callback( tcpSock, tcpSock->userContext, mDNStrue, 0 );
			tcpSock->connected = mDNStrue;
		}
		else
		{
			callback( tcpSock, tcpSock->userContext, mDNSfalse, event->iErrorCode[ FD_CONNECT_BIT ] );
		}
	}
	else
	{
		callback( tcpSock, tcpSock->userContext, mDNSfalse, 0 );
	}

exit:

	return;
}



//===========================================================================================================================
//	mDNSPlatformUDPSocket
//===========================================================================================================================

mDNSexport UDPSocket* mDNSPlatformUDPSocket(mDNS *const m, const mDNSIPPort requestedport)
{
	UDPSocket*	sock	= NULL;
	mDNSIPPort	port	= requestedport;
	mStatus		err		= mStatus_NoError;
	unsigned	i;

	// Setup connection data object

	sock = ( UDPSocket* ) malloc(sizeof( UDPSocket ) );
	require_action( sock, exit, err = mStatus_NoMemoryErr );
	memset( sock, 0, sizeof( UDPSocket ) );

	// Create the socket

	sock->fd			= INVALID_SOCKET;
	sock->recvMsgPtr	= m->p->unicastSock4.recvMsgPtr;
	sock->addr			= m->p->unicastSock4.addr;
	sock->ifd			= NULL;
	sock->m				= m;

	// Try at most 10000 times to get a unique random port

	for (i=0; i<10000; i++)
	{
		struct sockaddr_in saddr;

		saddr.sin_family		= AF_INET;
		saddr.sin_addr.s_addr	= 0;

		// The kernel doesn't do cryptographically strong random port
		// allocation, so we do it ourselves here

        if (mDNSIPPortIsZero(requestedport))
		{
			port = mDNSOpaque16fromIntVal( ( mDNSu16 ) ( 0xC000 + mDNSRandom(0x3FFF) ) );
		}

		saddr.sin_port = port.NotAnInteger;

        err = SetupSocket(m, ( struct sockaddr* ) &saddr, port, &sock->fd );
        if (!err) break;
	}

	require_noerr( err, exit );

	// Set the port

	sock->port = port;

	// Arm the completion routine

	err = mDNSPollRegisterSocket( sock->fd, FD_READ, UDPSocketNotification, sock );
	require_noerr( err, exit ); 

	// Bookkeeping

	sock->next		= gUDPSockets;
	gUDPSockets		= sock;
	gUDPNumSockets++;

exit:

	if ( err && sock )
	{
		UDPCloseSocket( sock );
		free( sock );
		sock = NULL;
	}

	return sock;
}
	
//===========================================================================================================================
//	mDNSPlatformUDPClose
//===========================================================================================================================
	
mDNSexport void mDNSPlatformUDPClose( UDPSocket *sock )
{
	UDPSocket	*	current  = gUDPSockets;
	UDPSocket	*	last = NULL;

	while ( current )
	{
		if ( current == sock )
		{
			if ( last == NULL )
			{
				gUDPSockets = sock->next;
			}
			else
			{
				last->next = sock->next;
			}

			UDPCloseSocket( sock );
			free( sock );

			gUDPNumSockets--;

			break;
		}

		last	= current;
		current	= current->next;
	}
}


//===========================================================================================================================
//	mDNSPlatformSendUDP
//===========================================================================================================================

mDNSexport mStatus
	mDNSPlatformSendUDP( 
		const mDNS * const			inMDNS, 
		const void * const	        inMsg, 
		const mDNSu8 * const		inMsgEnd, 
		mDNSInterfaceID 			inInterfaceID, 
		UDPSocket *					inSrcSocket,
		const mDNSAddr *			inDstIP, 
		mDNSIPPort 					inDstPort )
{
	SOCKET						sendingsocket = INVALID_SOCKET;
	mStatus						err = mStatus_NoError;
	mDNSInterfaceData *			ifd = (mDNSInterfaceData*) inInterfaceID;
	struct sockaddr_storage		addr;
	int							n;
	
	DEBUG_USE_ONLY( inMDNS );
	
	n = (int)( inMsgEnd - ( (const mDNSu8 * const) inMsg ) );
	check( inMDNS );
	check( inMsg );
	check( inMsgEnd );
	check( inDstIP );
	
	dlog( kDebugLevelChatty, DEBUG_NAME "platform send %d bytes to %#a:%u\n", n, inDstIP, ntohs( inDstPort.NotAnInteger ) );
	
	if( inDstIP->type == mDNSAddrType_IPv4 )
	{
		struct sockaddr_in *		sa4;
		
		sa4						= (struct sockaddr_in *) &addr;
		sa4->sin_family			= AF_INET;
		sa4->sin_port			= inDstPort.NotAnInteger;
		sa4->sin_addr.s_addr	= inDstIP->ip.v4.NotAnInteger;
		sendingsocket           = ifd ? ifd->sock.fd : inMDNS->p->unicastSock4.fd;

		if (inSrcSocket) { sendingsocket = inSrcSocket->fd; debugf("mDNSPlatformSendUDP using port %d, static port %d, sock %d", mDNSVal16(inSrcSocket->port), inMDNS->p->unicastSock4.fd, sendingsocket); }
	}
	else if( inDstIP->type == mDNSAddrType_IPv6 )
	{
		struct sockaddr_in6 *		sa6;
		
		sa6					= (struct sockaddr_in6 *) &addr;
		sa6->sin6_family	= AF_INET6;
		sa6->sin6_port		= inDstPort.NotAnInteger;
		sa6->sin6_flowinfo	= 0;
		sa6->sin6_addr		= *( (struct in6_addr *) &inDstIP->ip.v6 );
		sa6->sin6_scope_id	= 0;	// Windows requires the scope ID to be zero. IPV6_MULTICAST_IF specifies interface.
		sendingsocket		= ifd ? ifd->sock.fd : inMDNS->p->unicastSock6.fd;
	}
	else
	{
		dlog( kDebugLevelError, DEBUG_NAME "%s: dst is not an IPv4 or IPv6 address (type=%d)\n", __ROUTINE__, inDstIP->type );
		err = mStatus_BadParamErr;
		goto exit;
	}
	
	if (IsValidSocket(sendingsocket))
	{
		n = sendto( sendingsocket, (char *) inMsg, n, 0, (struct sockaddr *) &addr, sizeof( addr ) );
		err = translate_errno( n > 0, errno_compat(), kWriteErr );

		if ( err )
		{
			// Don't report EHOSTDOWN (i.e. ARP failure), ENETDOWN, or no route to host for unicast destinations

			if ( !mDNSAddressIsAllDNSLinkGroup( inDstIP ) && ( WSAGetLastError() == WSAEHOSTDOWN || WSAGetLastError() == WSAENETDOWN || WSAGetLastError() == WSAEHOSTUNREACH || WSAGetLastError() == WSAENETUNREACH ) )
			{
				err = mStatus_TransientErr;
			}
			else
			{
				//require_noerr( err, exit );
			}
		}
	}
	
exit:
	return( err );
}


mDNSexport void mDNSPlatformUpdateProxyList(mDNS *const m, const mDNSInterfaceID InterfaceID)
	{
	DEBUG_UNUSED( m );
	DEBUG_UNUSED( InterfaceID );
	}


mDNSexport void mDNSPlatformSetAllowSleep(mDNS *const m, mDNSBool allowSleep, const char *reason)
	{
	DEBUG_UNUSED( m );
	DEBUG_UNUSED( allowSleep );
	DEBUG_UNUSED( reason );
	}

//===========================================================================================================================
//	mDNSPlatformSendRawPacket
//===========================================================================================================================

mDNSexport void mDNSPlatformSendWakeupPacket(mDNS *const m, mDNSInterfaceID InterfaceID, char *ethaddr, char *ipaddr, int iteration)
{
	unsigned char			mac[ 6 ];
	unsigned char			buf[ 102 ];
	char					hex[ 3 ] = { 0 };
	unsigned char			*bufPtr = buf;
	struct sockaddr_storage	saddr;
	INT						len = sizeof( saddr );
	mDNSBool				unicast = mDNSfalse;
	MulticastWakeupStruct	*info;
	int						i;
	mStatus					err;

	(void) InterfaceID;

	require_action( ethaddr, exit, err = mStatus_BadParamErr );

	for ( i = 0; i < 6; i++ )
	{
		memcpy( hex, ethaddr + ( i * 3 ), 2 );
		mac[ i ] = ( unsigned char ) strtoul( hex, NULL, 16 );
	}

	memset( buf, 0, sizeof( buf ) );

	for ( i = 0; i < 6; i++ )
	{
		*bufPtr++ = 0xff;
	}
	
	for ( i = 0; i < 16; i++ )
	{
		memcpy( bufPtr, mac, sizeof( mac ) );
		bufPtr += sizeof( mac );
	}

	if ( ipaddr )
	{
		if ( WSAStringToAddressA( ipaddr, AF_INET, NULL, ( LPSOCKADDR ) &saddr, &len ) == 0 )
		{
			struct sockaddr_in * saddr4 = ( struct sockaddr_in* ) &saddr;
			saddr4->sin_port = htons( 9 );
			len = sizeof( *saddr4 );

			if ( saddr4->sin_addr.s_addr != htonl( INADDR_ANY ) )
			{
				unicast = mDNStrue;
			}
		}
		else if ( WSAStringToAddressA( ipaddr, AF_INET6, NULL, ( LPSOCKADDR ) &saddr, &len ) == 0 )
		{
			mDNSInterfaceData *ifd = ( mDNSInterfaceData* ) InterfaceID;
			struct sockaddr_in6 * saddr6 = ( struct sockaddr_in6* ) &saddr;
			saddr6->sin6_port = htons( 9 );

			if ( ifd != NULL )
			{
				saddr6->sin6_scope_id = ifd->scopeID;
			}

			len = sizeof( *saddr6 );

			if ( memcmp( &saddr6->sin6_addr, &in6addr_any, sizeof( IN6_ADDR ) ) != 0 )
			{
				unicast = mDNStrue;
			}
		}
	}

	if ( ( iteration < 2 ) && ( unicast ) )
	{
		SendWakeupPacket( m, ( LPSOCKADDR ) &saddr, len, ( const char* ) buf, sizeof( buf ), kUnicastWakeupNumTries, kUnicastWakeupSleepBetweenTries );
	}		

	info = ( MulticastWakeupStruct* ) malloc( sizeof( MulticastWakeupStruct ) );
	require_action( info, exit, err = mStatus_NoMemoryErr );
	info->inMDNS = m;
	memset( &info->addr, 0, sizeof( info->addr ) );
	info->addr.sin_family = AF_INET;
	info->addr.sin_addr.s_addr = AllDNSLinkGroup_v4.ip.v4.NotAnInteger;
	info->addr.sin_port = htons( 9 );
	info->addrLen = sizeof( info->addr );
	memcpy( info->data, buf, sizeof( buf ) );
	info->dataLen = sizeof( buf );
	info->numTries  = kMulticastWakeupNumTries;
	info->msecSleep = kMulticastWakeupSleepBetweenTries;

	_beginthread( SendMulticastWakeupPacket, 0, ( void* ) info );

exit:

	return;
}


mDNSexport mDNSBool mDNSPlatformValidRecordForInterface(AuthRecord *rr, const NetworkInterfaceInfo *intf)
{
	DEBUG_UNUSED( rr );
	DEBUG_UNUSED( intf );

	return mDNStrue;
}
 

mDNSexport void mDNSPlatformSendRawPacket(const void *const msg, const mDNSu8 *const end, mDNSInterfaceID InterfaceID)
	{
	DEBUG_UNUSED( msg );
	DEBUG_UNUSED( end );
	DEBUG_UNUSED( InterfaceID );
	}


mDNSexport void mDNSPlatformSetLocalAddressCacheEntry(mDNS *const m, const mDNSAddr *const tpa, const mDNSEthAddr *const tha, mDNSInterfaceID InterfaceID)
	{
	DEBUG_UNUSED( m );
	DEBUG_UNUSED( tpa );
	DEBUG_UNUSED( tha );
	DEBUG_UNUSED( InterfaceID );
	}


mDNSexport void mDNSPlatformReceiveRawPacket(const void *const msg, const mDNSu8 *const end, mDNSInterfaceID InterfaceID)
	{
	DEBUG_UNUSED( msg );
	DEBUG_UNUSED( end );
	DEBUG_UNUSED( InterfaceID );
	}

mDNSexport void mDNSPlatformSetLocalARP( const mDNSv4Addr * const tpa, const mDNSEthAddr * const tha, mDNSInterfaceID InterfaceID )
	{
	DEBUG_UNUSED( tpa );
	DEBUG_UNUSED( tha );
	DEBUG_UNUSED( InterfaceID );
	}

mDNSexport void mDNSPlatformWriteDebugMsg(const char *msg)
	{
	dlog( kDebugLevelInfo, "%s\n", msg );
	}

mDNSexport void mDNSPlatformWriteLogMsg( const char * ident, const char * msg, mDNSLogLevel_t loglevel )
	{
	extern mDNS mDNSStorage;
	int type;
	
	DEBUG_UNUSED( ident );

	type = EVENTLOG_ERROR_TYPE;

	switch (loglevel) 
	{
		case MDNS_LOG_MSG:       type = EVENTLOG_ERROR_TYPE;		break;
		case MDNS_LOG_OPERATION: type = EVENTLOG_WARNING_TYPE;		break;
		case MDNS_LOG_SPS:       type = EVENTLOG_INFORMATION_TYPE;  break;
		case MDNS_LOG_INFO:      type = EVENTLOG_INFORMATION_TYPE;	break;
		case MDNS_LOG_DEBUG:     type = EVENTLOG_INFORMATION_TYPE;	break;
		default:
			fprintf(stderr, "Unknown loglevel %d, assuming LOG_ERR\n", loglevel);
			fflush(stderr);
			}

	mDNSStorage.p->reportStatusFunc( type, msg );
	dlog( kDebugLevelInfo, "%s\n", msg );
	}

mDNSexport void mDNSPlatformSourceAddrForDest( mDNSAddr * const src, const mDNSAddr * const dst )
	{
	DEBUG_UNUSED( src );
	DEBUG_UNUSED( dst );
	}

//===========================================================================================================================
//	mDNSPlatformTLSSetupCerts
//===========================================================================================================================

mDNSexport mStatus
mDNSPlatformTLSSetupCerts(void)
{
	return mStatus_UnsupportedErr;
}

//===========================================================================================================================
//	mDNSPlatformTLSTearDownCerts
//===========================================================================================================================

mDNSexport void
mDNSPlatformTLSTearDownCerts(void)
{
}

//===========================================================================================================================
//	mDNSPlatformSetDNSConfig
//===========================================================================================================================

mDNSlocal void SetDNSServers( mDNS *const m );
mDNSlocal void SetSearchDomainList( void );

mDNSexport void mDNSPlatformSetDNSConfig(mDNS *const m, mDNSBool setservers, mDNSBool setsearch, domainname *const fqdn, DNameListElem **regDomains, DNameListElem **browseDomains)
{
	if (setservers) SetDNSServers(m);
	if (setsearch) SetSearchDomainList();
	
	if ( fqdn )
	{
		GetDDNSFQDN( fqdn );
	}

	if ( browseDomains )
	{
		GetDDNSDomains( browseDomains, kServiceParametersNode TEXT("\\DynDNS\\Setup\\") kServiceDynDNSBrowseDomains );
	}

	if ( regDomains )
	{
		GetDDNSDomains( regDomains, kServiceParametersNode TEXT("\\DynDNS\\Setup\\") kServiceDynDNSRegistrationDomains );
	}
}


//===========================================================================================================================
//	mDNSPlatformDynDNSHostNameStatusChanged
//===========================================================================================================================

mDNSexport void
mDNSPlatformDynDNSHostNameStatusChanged(const domainname *const dname, const mStatus status)
{
	char		uname[MAX_ESCAPED_DOMAIN_NAME];
	BYTE		bStatus;
	LPCTSTR		name;
	HKEY		key = NULL;
	mStatus		err;
	char	*	p;
	
	ConvertDomainNameToCString(dname, uname);
	
	p = uname;

	while (*p)
	{
		*p = (char) tolower(*p);
		if (!(*(p+1)) && *p == '.') *p = 0; // if last character, strip trailing dot
		p++;
	}

	check( strlen( p ) <= MAX_ESCAPED_DOMAIN_NAME );
	name = kServiceParametersNode TEXT("\\DynDNS\\State\\HostNames");
	err = RegCreateKey( HKEY_CURRENT_USER, name, &key );
	require_noerr( err, exit );

	bStatus = ( status ) ? 0 : 1;
	err = RegSetValueEx( key, kServiceDynDNSStatus, 0, REG_DWORD, (const LPBYTE) &bStatus, sizeof(DWORD) );
	require_noerr( err, exit );

exit:

	if ( key )
	{
		RegCloseKey( key );
	}

	return;
}


//===========================================================================================================================
//	SetDomainSecrets
//===========================================================================================================================

// This routine needs to be called whenever the system secrets database changes.
// We call it from DynDNSConfigDidChange and mDNSPlatformInit

void
SetDomainSecrets( mDNS * const m )
{
	DomainAuthInfo *ptr;
	domainname		fqdn;
	DNameListElem * regDomains = NULL;

	// Rather than immediately deleting all keys now, we mark them for deletion in ten seconds.
	// In the case where the user simultaneously removes their DDNS host name and the key
	// for it, this gives mDNSResponder ten seconds to gracefully delete the name from the
	// server before it loses access to the necessary key. Otherwise, we'd leave orphaned
	// address records behind that we no longer have permission to delete.
	
	for (ptr = m->AuthInfoList; ptr; ptr = ptr->next)
		ptr->deltime = NonZeroTime(m->timenow + mDNSPlatformOneSecond*10);

	GetDDNSFQDN( &fqdn );

	if ( fqdn.c[ 0 ] )
	{
		SetDomainSecret( m, &fqdn );
	}

	GetDDNSDomains( &regDomains, kServiceParametersNode TEXT("\\DynDNS\\Setup\\") kServiceDynDNSRegistrationDomains );

	while ( regDomains )
	{
		DNameListElem * current = regDomains;
		SetDomainSecret( m, &current->name );
		regDomains = regDomains->next;
		free( current );
	}
}


//===========================================================================================================================
//	SetSearchDomainList
//===========================================================================================================================

mDNSlocal void SetDomainFromDHCP( void );
mDNSlocal void SetReverseMapSearchDomainList( void );

mDNSlocal void
SetSearchDomainList( void )
{
	char			*	searchList	= NULL;
	DWORD				searchListLen;
	//DNameListElem	*	head = NULL;
	//DNameListElem	*	current = NULL;
	char			*	tok;
	HKEY				key;
	mStatus				err;

	err = RegCreateKey( HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), &key );
	require_noerr( err, exit );

	err = RegQueryString( key, "SearchList", &searchList, &searchListLen, NULL );
	require_noerr( err, exit );

	// Windows separates the search domains with ','

	tok = strtok( searchList, "," );
	while ( tok )
	{
		if ( ( strcmp( tok, "" ) != 0 ) && ( strcmp( tok, "." ) != 0 ) )
			mDNS_AddSearchDomain_CString(tok, mDNSNULL);
		tok = strtok( NULL, "," );
	}

exit:

	if ( searchList ) 
	{
		free( searchList );
	}

	if ( key )
	{
		RegCloseKey( key );
	}

	SetDomainFromDHCP();
	SetReverseMapSearchDomainList();
}


//===========================================================================================================================
//	SetReverseMapSearchDomainList
//===========================================================================================================================

mDNSlocal void
SetReverseMapSearchDomainList( void )
{
	struct ifaddrs	*	ifa;

	ifa = myGetIfAddrs( 1 );
	while (ifa)
	{
		mDNSAddr addr;
		
		if (ifa->ifa_addr->sa_family == AF_INET && !SetupAddr(&addr, ifa->ifa_addr) && !(ifa->ifa_flags & IFF_LOOPBACK) && ifa->ifa_netmask)
		{
			mDNSAddr	netmask;
			char		buffer[256];
			
			if (!SetupAddr(&netmask, ifa->ifa_netmask))
			{
				sprintf(buffer, "%d.%d.%d.%d.in-addr.arpa.", addr.ip.v4.b[3] & netmask.ip.v4.b[3],
                                                             addr.ip.v4.b[2] & netmask.ip.v4.b[2],
                                                             addr.ip.v4.b[1] & netmask.ip.v4.b[1],
                                                             addr.ip.v4.b[0] & netmask.ip.v4.b[0]);
				mDNS_AddSearchDomain_CString(buffer, mDNSNULL);
			}
		}
	
		ifa = ifa->ifa_next;
	}

	return;
}


//===========================================================================================================================
//	SetDNSServers
//===========================================================================================================================

mDNSlocal void
SetDNSServers( mDNS *const m )
{
	PIP_PER_ADAPTER_INFO	pAdapterInfo	=	NULL;
	FIXED_INFO			*	fixedInfo	= NULL;
	ULONG					bufLen		= 0;	
	IP_ADDR_STRING		*	dnsServerList;
	IP_ADDR_STRING		*	ipAddr;
	DWORD					index;
	int						i			= 0;
	mStatus					err			= kUnknownErr;

	// Get the primary interface.

	index = GetPrimaryInterface();

	// This should have the interface index of the primary index.  Fall back in cases where
	// it can't be determined.

	if ( index )
	{
		bufLen = 0;

		for ( i = 0; i < 100; i++ )
		{
			err = GetPerAdapterInfo( index, pAdapterInfo, &bufLen );

			if ( err != ERROR_BUFFER_OVERFLOW )
			{
				break;
			}

			pAdapterInfo = (PIP_PER_ADAPTER_INFO) realloc( pAdapterInfo, bufLen );
			require_action( pAdapterInfo, exit, err = mStatus_NoMemoryErr );
		}

		require_noerr( err, exit );

		dnsServerList = &pAdapterInfo->DnsServerList;
	}
	else
	{
		bufLen = sizeof( FIXED_INFO );

		for ( i = 0; i < 100; i++ )
		{
			if ( fixedInfo )
			{
				GlobalFree( fixedInfo );
				fixedInfo = NULL;
			}

			fixedInfo = (FIXED_INFO*) GlobalAlloc( GPTR, bufLen );
			require_action( fixedInfo, exit, err = mStatus_NoMemoryErr );
	   
			err = GetNetworkParams( fixedInfo, &bufLen );

			if ( err != ERROR_BUFFER_OVERFLOW )
			{
				break;
			}
		}

		require_noerr( err, exit );

		dnsServerList = &fixedInfo->DnsServerList;
	}

	for ( ipAddr = dnsServerList; ipAddr; ipAddr = ipAddr->Next )
	{
		mDNSAddr addr;
		err = StringToAddress( &addr, ipAddr->IpAddress.String );
		if ( !err ) mDNS_AddDNSServer(m, mDNSNULL, mDNSInterface_Any, &addr, UnicastDNSPort, mDNSfalse, DEFAULT_UDNS_TIMEOUT, mDNSfalse);
	}

exit:

	if ( pAdapterInfo )
	{
		free( pAdapterInfo );
	}

	if ( fixedInfo )
	{
		GlobalFree( fixedInfo );
	}
}


//===========================================================================================================================
//	SetDomainFromDHCP
//===========================================================================================================================

mDNSlocal void
SetDomainFromDHCP( void )
{
	int					i			= 0;
	IP_ADAPTER_INFO *	pAdapterInfo;
	IP_ADAPTER_INFO *	pAdapter;
	DWORD				bufLen;
	DWORD				index;
	HKEY				key = NULL;
	LPSTR				domain = NULL;
	DWORD				dwSize;
	mStatus				err = mStatus_NoError;

	pAdapterInfo	= NULL;
	
	for ( i = 0; i < 100; i++ )
	{
		err = GetAdaptersInfo( pAdapterInfo, &bufLen);

		if ( err != ERROR_BUFFER_OVERFLOW )
		{
			break;
		}

		pAdapterInfo = (IP_ADAPTER_INFO*) realloc( pAdapterInfo, bufLen );
		require_action( pAdapterInfo, exit, err = kNoMemoryErr );
	}

	require_noerr( err, exit );

	index = GetPrimaryInterface();

	for ( pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next )
	{
		if ( pAdapter->IpAddressList.IpAddress.String &&
		     pAdapter->IpAddressList.IpAddress.String[0] &&
		     pAdapter->GatewayList.IpAddress.String &&
		     pAdapter->GatewayList.IpAddress.String[0] &&
		     ( !index || ( pAdapter->Index == index ) ) )
		{
			// Found one that will work

			char keyName[1024];

			_snprintf( keyName, 1024, "%s%s", "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\", pAdapter->AdapterName );

			err = RegCreateKeyA( HKEY_LOCAL_MACHINE, keyName, &key );
			require_noerr( err, exit );

			err = RegQueryString( key, "Domain", &domain, &dwSize, NULL );
			check_noerr( err );

			if ( !domain || !domain[0] )
			{
				if ( domain )
				{
					free( domain );
					domain = NULL;
				}

				err = RegQueryString( key, "DhcpDomain", &domain, &dwSize, NULL );
				check_noerr( err );
			}

			if ( domain && domain[0] ) mDNS_AddSearchDomain_CString(domain, mDNSNULL);

			break;
		}
	}

exit:

	if ( pAdapterInfo )
	{
		free( pAdapterInfo );
	}

	if ( domain )
	{
		free( domain );
	}

	if ( key )
	{
		RegCloseKey( key );
	}
}


//===========================================================================================================================
//	mDNSPlatformGetPrimaryInterface
//===========================================================================================================================

mDNSexport mStatus
mDNSPlatformGetPrimaryInterface( mDNS * const m, mDNSAddr * v4, mDNSAddr * v6, mDNSAddr * router )
{
	IP_ADAPTER_INFO *	pAdapterInfo;
	IP_ADAPTER_INFO *	pAdapter;
	DWORD				bufLen;
	int					i;
	BOOL				found;
	DWORD				index;
	mStatus				err = mStatus_NoError;

	DEBUG_UNUSED( m );

	*v6 = zeroAddr;

	pAdapterInfo	= NULL;
	bufLen			= 0;
	found			= FALSE;

	for ( i = 0; i < 100; i++ )
	{
		err = GetAdaptersInfo( pAdapterInfo, &bufLen);

		if ( err != ERROR_BUFFER_OVERFLOW )
		{
			break;
		}

		pAdapterInfo = (IP_ADAPTER_INFO*) realloc( pAdapterInfo, bufLen );
		require_action( pAdapterInfo, exit, err = kNoMemoryErr );
	}

	require_noerr( err, exit );

	index = GetPrimaryInterface();

	for ( pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next )
	{
		if ( pAdapter->IpAddressList.IpAddress.String &&
		     pAdapter->IpAddressList.IpAddress.String[0] &&
		     pAdapter->GatewayList.IpAddress.String &&
		     pAdapter->GatewayList.IpAddress.String[0] &&
		     ( StringToAddress( v4, pAdapter->IpAddressList.IpAddress.String ) == mStatus_NoError ) &&
		     ( StringToAddress( router, pAdapter->GatewayList.IpAddress.String ) == mStatus_NoError ) &&
		     ( !index || ( pAdapter->Index == index ) ) )
		{
			// Found one that will work

			if ( pAdapter->AddressLength == sizeof( m->PrimaryMAC ) )
			{
				memcpy( &m->PrimaryMAC, pAdapter->Address, pAdapter->AddressLength );
			}

			found = TRUE;
			break;
		}
	}

exit:

	if ( pAdapterInfo )
	{
		free( pAdapterInfo );
	}

	return err;
}


#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	debugf_
//===========================================================================================================================
#if( MDNS_DEBUGMSGS )
mDNSexport void	debugf_( const char *inFormat, ... )
{
	char		buffer[ 512 ];
    va_list		args;
    mDNSu32		length;
	
	va_start( args, inFormat );
	length = mDNS_vsnprintf( buffer, sizeof( buffer ), inFormat, args );
	va_end( args );
	
	dlog( kDebugLevelInfo, "%s\n", buffer );
}
#endif

//===========================================================================================================================
//	verbosedebugf_
//===========================================================================================================================

#if( MDNS_DEBUGMSGS > 1 )
mDNSexport void	verbosedebugf_( const char *inFormat, ... )
{
	char		buffer[ 512 ];
    va_list		args;
    mDNSu32		length;
	
	va_start( args, inFormat );
	length = mDNS_vsnprintf( buffer, sizeof( buffer ), inFormat, args );
	va_end( args );
	
	dlog( kDebugLevelVerbose, "%s\n", buffer );
}
#endif


#if 0
#pragma mark -
#pragma mark == Platform Internals  ==
#endif


//===========================================================================================================================
//	SetupNiceName
//===========================================================================================================================

mStatus	SetupNiceName( mDNS * const inMDNS )
{
	HKEY		descKey = NULL;
	char		utf8[ 256 ];
	LPCTSTR		s;
	LPWSTR		joinName;
	NETSETUP_JOIN_STATUS joinStatus;
	mStatus		err = 0;
	DWORD		namelen;
	BOOL		ok;
	
	check( inMDNS );
	
	// Set up the nice name.
	utf8[0] = '\0';

	// First try and open the registry key that contains the computer description value
	s = TEXT("SYSTEM\\CurrentControlSet\\Services\\lanmanserver\\parameters");
	err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s, 0, KEY_READ, &descKey);
	check_translated_errno( err == 0, errno_compat(), kNameErr );

	if ( !err )
	{
		TCHAR	desc[256];
		DWORD	descSize = sizeof( desc );

		// look for the computer description
		err = RegQueryValueEx( descKey, TEXT("srvcomment"), 0, NULL, (LPBYTE) &desc, &descSize);
		
		if ( !err )
		{
			err = TCHARtoUTF8( desc, utf8, sizeof( utf8 ) );
		}

		if ( err )
		{
			utf8[ 0 ] = '\0';
		}
	}

	// if we can't find it in the registry, then use the hostname of the machine
	if ( err || ( utf8[ 0 ] == '\0' ) )
	{
		TCHAR hostname[256];
		
		namelen = sizeof( hostname ) / sizeof( TCHAR );

		ok = GetComputerNameExW( ComputerNamePhysicalDnsHostname, hostname, &namelen );
		err = translate_errno( ok, (mStatus) GetLastError(), kNameErr );
		check_noerr( err );
		
		if( !err )
		{
			err = TCHARtoUTF8( hostname, utf8, sizeof( utf8 ) );
		}

		if ( err )
		{
			utf8[ 0 ] = '\0';
		}
	}

	// if we can't get the hostname
	if ( err || ( utf8[ 0 ] == '\0' ) )
	{
		// Invalidate name so fall back to a default name.
		
		strcpy( utf8, kMDNSDefaultName );
	}

	utf8[ sizeof( utf8 ) - 1 ]	= '\0';	
	inMDNS->nicelabel.c[ 0 ]	= (mDNSu8) (strlen( utf8 ) < MAX_DOMAIN_LABEL ? strlen( utf8 ) : MAX_DOMAIN_LABEL);
	memcpy( &inMDNS->nicelabel.c[ 1 ], utf8, inMDNS->nicelabel.c[ 0 ] );
	
	dlog( kDebugLevelInfo, DEBUG_NAME "nice name \"%.*s\"\n", inMDNS->nicelabel.c[ 0 ], &inMDNS->nicelabel.c[ 1 ] );
	
	if ( descKey )
	{
		RegCloseKey( descKey );
	}

	ZeroMemory( inMDNS->p->nbname, sizeof( inMDNS->p->nbname ) );
	ZeroMemory( inMDNS->p->nbdomain, sizeof( inMDNS->p->nbdomain ) );

	namelen = sizeof( inMDNS->p->nbname );
	ok = GetComputerNameExA( ComputerNamePhysicalNetBIOS, inMDNS->p->nbname, &namelen );
	check( ok );
	if ( ok ) dlog( kDebugLevelInfo, DEBUG_NAME "netbios name \"%s\"\n", inMDNS->p->nbname );

	err = NetGetJoinInformation( NULL, &joinName, &joinStatus );
	check ( err == NERR_Success );
	if ( err == NERR_Success )
	{
		if ( ( joinStatus == NetSetupWorkgroupName ) || ( joinStatus == NetSetupDomainName ) )
		{
			err = TCHARtoUTF8( joinName, inMDNS->p->nbdomain, sizeof( inMDNS->p->nbdomain ) );
			check( !err );
			if ( !err ) dlog( kDebugLevelInfo, DEBUG_NAME "netbios domain/workgroup \"%s\"\n", inMDNS->p->nbdomain );
		}

		NetApiBufferFree( joinName );
		joinName = NULL;
	}

	err = 0;

	return( err );
}

//===========================================================================================================================
//	SetupHostName
//===========================================================================================================================

mDNSlocal mStatus	SetupHostName( mDNS * const inMDNS )
{
	mStatus		err = 0;
	char		tempString[ 256 ];
	DWORD		tempStringLen;
	domainlabel tempLabel;
	BOOL		ok;
	
	check( inMDNS );

	// Set up the nice name.
	tempString[ 0 ] = '\0';

	// use the hostname of the machine
	tempStringLen = sizeof( tempString );
	ok = GetComputerNameExA( ComputerNamePhysicalDnsHostname, tempString, &tempStringLen );
	err = translate_errno( ok, (mStatus) GetLastError(), kNameErr );
	check_noerr( err );

	// if we can't get the hostname
	if( err || ( tempString[ 0 ] == '\0' ) )
	{
		// Invalidate name so fall back to a default name.
		
		strcpy( tempString, kMDNSDefaultName );
	}

	tempString[ sizeof( tempString ) - 1 ] = '\0';
	tempLabel.c[ 0 ] = (mDNSu8) (strlen( tempString ) < MAX_DOMAIN_LABEL ? strlen( tempString ) : MAX_DOMAIN_LABEL );
	memcpy( &tempLabel.c[ 1 ], tempString, tempLabel.c[ 0 ] );
	
	// Set up the host name.
	
	ConvertUTF8PstringToRFC1034HostLabel( tempLabel.c, &inMDNS->hostlabel );
	if( inMDNS->hostlabel.c[ 0 ] == 0 )
	{
		// Nice name has no characters that are representable as an RFC1034 name (e.g. Japanese) so use the default.
		
		MakeDomainLabelFromLiteralString( &inMDNS->hostlabel, kMDNSDefaultName );
	}

	check( inMDNS->hostlabel.c[ 0 ] != 0 );
	
	mDNS_SetFQDN( inMDNS );
	
	dlog( kDebugLevelInfo, DEBUG_NAME "host name \"%.*s\"\n", inMDNS->hostlabel.c[ 0 ], &inMDNS->hostlabel.c[ 1 ] );
	
	return( err );
}

//===========================================================================================================================
//	SetupName
//===========================================================================================================================

mDNSlocal mStatus	SetupName( mDNS * const inMDNS )
{
	mStatus		err = 0;
	
	check( inMDNS );
	
	err = SetupNiceName( inMDNS );
	check_noerr( err );

	err = SetupHostName( inMDNS );
	check_noerr( err );

	return err;
}


//===========================================================================================================================
//	SetupInterfaceList
//===========================================================================================================================

mStatus	SetupInterfaceList( mDNS * const inMDNS )
{
	mStatus						err;
	mDNSInterfaceData **		next;
	mDNSInterfaceData *			ifd;
	struct ifaddrs *			addrs;
	struct ifaddrs *			p;
	struct ifaddrs *			loopbackv4;
	struct ifaddrs *			loopbackv6;
	u_int						flagMask;
	u_int						flagTest;
	mDNSBool					foundv4;
	mDNSBool					foundv6;
	mDNSBool					foundUnicastSock4DestAddr;
	mDNSBool					foundUnicastSock6DestAddr;
	
	dlog( kDebugLevelTrace, DEBUG_NAME "setting up interface list\n" );
	check( inMDNS );
	check( inMDNS->p );
	
	inMDNS->p->registeredLoopback4	= mDNSfalse;
	inMDNS->p->nextDHCPLeaseExpires = 0x7FFFFFFF;
	addrs							= NULL;
	foundv4							= mDNSfalse;
	foundv6							= mDNSfalse;
	foundUnicastSock4DestAddr		= mDNSfalse;
	foundUnicastSock6DestAddr		= mDNSfalse;
	
	// Tear down any existing interfaces that may be set up.
	
	TearDownInterfaceList( inMDNS );

	// Set up the name of this machine.
	
	err = SetupName( inMDNS );
	check_noerr( err );

	// Set up IPv4 interface(s). We have to set up IPv4 first so any IPv6 interface with an IPv4-routable address
	// can refer to the IPv4 interface when it registers to allow DNS AAAA records over the IPv4 interface.
	
	err = getifaddrs( &addrs );
	require_noerr( err, exit );
	
	loopbackv4	= NULL;
	loopbackv6	= NULL;
	next		= &inMDNS->p->interfaceList;

	flagMask = IFF_UP | IFF_MULTICAST;
	flagTest = IFF_UP | IFF_MULTICAST;
	
#if( MDNS_WINDOWS_ENABLE_IPV4 )
	for( p = addrs; p; p = p->ifa_next )
	{
		if( !p->ifa_addr || ( p->ifa_addr->sa_family != AF_INET ) || ( ( p->ifa_flags & flagMask ) != flagTest ) )
		{
			continue;
		}
		if( p->ifa_flags & IFF_LOOPBACK )
		{
			if( !loopbackv4 )
			{
				loopbackv4 = p;
			}
			continue;
		}
		dlog( kDebugLevelVerbose, DEBUG_NAME "Interface %40s (0x%08X) %##a\n", 
			p->ifa_name ? p->ifa_name : "<null>", p->ifa_extra.index, p->ifa_addr );
		
		err = SetupInterface( inMDNS, p, &ifd );
		require_noerr( err, exit );

		// If this guy is point-to-point (ifd->interfaceInfo.McastTxRx == 0 ) we still want to
		// register him, but we also want to note that we haven't found a v4 interface
		// so that we register loopback so same host operations work
 		
		if ( ifd->interfaceInfo.McastTxRx == mDNStrue )
		{
			foundv4 = mDNStrue;
		}

		if ( p->ifa_dhcpEnabled && ( p->ifa_dhcpLeaseExpires < inMDNS->p->nextDHCPLeaseExpires ) )
		{
			inMDNS->p->nextDHCPLeaseExpires = p->ifa_dhcpLeaseExpires;
		}

		// If we're on a platform that doesn't have WSARecvMsg(), there's no way
		// of determing the destination address of a packet that is sent to us.
		// For multicast packets, that's easy to determine.  But for the unicast
		// sockets, we'll fake it by taking the address of the first interface
		// that is successfully setup.

		if ( !foundUnicastSock4DestAddr )
		{
			inMDNS->p->unicastSock4.addr = ifd->interfaceInfo.ip;
			foundUnicastSock4DestAddr = TRUE;
		}
			
		*next = ifd;
		next  = &ifd->next;
		++inMDNS->p->interfaceCount;
	}
#endif
	
	// Set up IPv6 interface(s) after IPv4 is set up (see IPv4 notes above for reasoning).
	
#if( MDNS_WINDOWS_ENABLE_IPV6 )

	if ( gEnableIPv6 )
	{
		for( p = addrs; p; p = p->ifa_next )
		{
			if( !p->ifa_addr || ( p->ifa_addr->sa_family != AF_INET6 ) || ( ( p->ifa_flags & flagMask ) != flagTest ) )
			{
				continue;
			}
			if( p->ifa_flags & IFF_LOOPBACK )
			{
				if( !loopbackv6 )
				{
					loopbackv6 = p;
				}
				continue;
			}
			dlog( kDebugLevelVerbose, DEBUG_NAME "Interface %40s (0x%08X) %##a\n", 
				p->ifa_name ? p->ifa_name : "<null>", p->ifa_extra.index, p->ifa_addr );
			
			err = SetupInterface( inMDNS, p, &ifd );
			require_noerr( err, exit );
					
			// If this guy is point-to-point (ifd->interfaceInfo.McastTxRx == 0 ) we still want to
			// register him, but we also want to note that we haven't found a v4 interface
			// so that we register loopback so same host operations work
	 		
			if ( ifd->interfaceInfo.McastTxRx == mDNStrue )
			{
				foundv6 = mDNStrue;
			}

			// If we're on a platform that doesn't have WSARecvMsg(), there's no way
			// of determing the destination address of a packet that is sent to us.
			// For multicast packets, that's easy to determine.  But for the unicast
			// sockets, we'll fake it by taking the address of the first interface
			// that is successfully setup.

			if ( !foundUnicastSock6DestAddr )
			{
				inMDNS->p->unicastSock6.addr = ifd->interfaceInfo.ip;
				foundUnicastSock6DestAddr = TRUE;
			}

			*next = ifd;
			next  = &ifd->next;
			++inMDNS->p->interfaceCount;
		}
	}

#endif

	// If there are no real interfaces, but there is a loopback interface, use that so same-machine operations work.

#if( !MDNS_WINDOWS_ENABLE_IPV4 && !MDNS_WINDOWS_ENABLE_IPV6 )
	
	flagMask |= IFF_LOOPBACK;
	flagTest |= IFF_LOOPBACK;
	
	for( p = addrs; p; p = p->ifa_next )
	{
		if( !p->ifa_addr || ( ( p->ifa_flags & flagMask ) != flagTest ) )
		{
			continue;
		}
		if( ( p->ifa_addr->sa_family != AF_INET ) && ( p->ifa_addr->sa_family != AF_INET6 ) )
		{
			continue;
		}
		
		v4loopback = p;
		break;
	}
	
#endif
	
	if ( !foundv4 && loopbackv4 )
	{
		dlog( kDebugLevelInfo, DEBUG_NAME "Interface %40s (0x%08X) %##a\n", 
			loopbackv4->ifa_name ? loopbackv4->ifa_name : "<null>", loopbackv4->ifa_extra.index, loopbackv4->ifa_addr );
		
		err = SetupInterface( inMDNS, loopbackv4, &ifd );
		require_noerr( err, exit );

		inMDNS->p->registeredLoopback4 = mDNStrue;
		
#if( MDNS_WINDOWS_ENABLE_IPV4 )

		// If we're on a platform that doesn't have WSARecvMsg(), there's no way
		// of determing the destination address of a packet that is sent to us.
		// For multicast packets, that's easy to determine.  But for the unicast
		// sockets, we'll fake it by taking the address of the first interface
		// that is successfully setup.

		if ( !foundUnicastSock4DestAddr )
		{
			inMDNS->p->unicastSock4.addr = ifd->sock.addr;
			foundUnicastSock4DestAddr = TRUE;
		}
#endif

		*next = ifd;
		next  = &ifd->next;
		++inMDNS->p->interfaceCount;
	}

	if ( !foundv6 && loopbackv6 )
	{
		dlog( kDebugLevelInfo, DEBUG_NAME "Interface %40s (0x%08X) %##a\n", 
			loopbackv6->ifa_name ? loopbackv6->ifa_name : "<null>", loopbackv6->ifa_extra.index, loopbackv6->ifa_addr );
		
		err = SetupInterface( inMDNS, loopbackv6, &ifd );
		require_noerr( err, exit );
		
#if( MDNS_WINDOWS_ENABLE_IPV6 )

		if ( gEnableIPv6 )
		{
			// If we're on a platform that doesn't have WSARecvMsg(), there's no way
			// of determing the destination address of a packet that is sent to us.
			// For multicast packets, that's easy to determine.  But for the unicast
			// sockets, we'll fake it by taking the address of the first interface
			// that is successfully setup.

			if ( !foundUnicastSock6DestAddr )
			{
				inMDNS->p->unicastSock6.addr = ifd->sock.addr;
				foundUnicastSock6DestAddr = TRUE;
			}
		}

#endif

		*next = ifd;
		next  = &ifd->next;
		++inMDNS->p->interfaceCount;
	}

	CheckFileShares( inMDNS );

exit:
	if( err )
	{
		TearDownInterfaceList( inMDNS );
	}
	if( addrs )
	{
		freeifaddrs( addrs );
	}
	dlog( kDebugLevelTrace, DEBUG_NAME "setting up interface list done (err=%d %m)\n", err, err );
	return( err );
}

//===========================================================================================================================
//	TearDownInterfaceList
//===========================================================================================================================

mStatus	TearDownInterfaceList( mDNS * const inMDNS )
{
	mDNSInterfaceData **		p;
	mDNSInterfaceData *		ifd;
	
	dlog( kDebugLevelTrace, DEBUG_NAME "tearing down interface list\n" );
	check( inMDNS );
	check( inMDNS->p );

	// Free any interfaces that were previously marked inactive and are no longer referenced by the mDNS cache.
	// Interfaces are marked inactive, but not deleted immediately if they were still referenced by the mDNS cache
	// so that remove events that occur after an interface goes away can still report the correct interface.

	p = &inMDNS->p->inactiveInterfaceList;
	while( *p )
	{
		ifd = *p;
		if( NumCacheRecordsForInterfaceID( inMDNS, (mDNSInterfaceID) ifd ) > 0 )
		{
			p = &ifd->next;
			continue;
		}
		
		dlog( kDebugLevelInfo, DEBUG_NAME "freeing unreferenced, inactive interface %#p %#a\n", ifd, &ifd->interfaceInfo.ip );
		*p = ifd->next;

		QueueUserAPC( ( PAPCFUNC ) FreeInterface, inMDNS->p->mainThread, ( ULONG_PTR ) ifd );
	}

	// Tear down all the interfaces.
	
	while( inMDNS->p->interfaceList )
	{
		ifd = inMDNS->p->interfaceList;
		inMDNS->p->interfaceList = ifd->next;
		
		TearDownInterface( inMDNS, ifd );
	}
	inMDNS->p->interfaceCount = 0;
	
	dlog( kDebugLevelTrace, DEBUG_NAME "tearing down interface list done\n" );
	return( mStatus_NoError );
}

//===========================================================================================================================
//	SetupInterface
//===========================================================================================================================

mDNSlocal mStatus	SetupInterface( mDNS * const inMDNS, const struct ifaddrs *inIFA, mDNSInterfaceData **outIFD )
{
	mDNSInterfaceData	*	ifd;
	mDNSInterfaceData	*	p;
	mStatus					err;
	
	ifd = NULL;
	dlog( kDebugLevelTrace, DEBUG_NAME "setting up interface\n" );
	check( inMDNS );
	check( inMDNS->p );
	check( inIFA );
	check( inIFA->ifa_addr );
	check( outIFD );
	
	// Allocate memory for the interface and initialize it.
	
	ifd = (mDNSInterfaceData *) calloc( 1, sizeof( *ifd ) );
	require_action( ifd, exit, err = mStatus_NoMemoryErr );
	ifd->sock.fd	= kInvalidSocketRef;
	ifd->sock.ifd	= ifd;
	ifd->sock.next	= NULL;
	ifd->sock.m		= inMDNS;
	ifd->index		= inIFA->ifa_extra.index;
	ifd->scopeID	= inIFA->ifa_extra.index;
	check( strlen( inIFA->ifa_name ) < sizeof( ifd->name ) );
	strncpy( ifd->name, inIFA->ifa_name, sizeof( ifd->name ) - 1 );
	ifd->name[ sizeof( ifd->name ) - 1 ] = '\0';
	
	strncpy(ifd->interfaceInfo.ifname, inIFA->ifa_name, sizeof(ifd->interfaceInfo.ifname));
	ifd->interfaceInfo.ifname[sizeof(ifd->interfaceInfo.ifname)-1] = 0;
	
	// We always send and receive using IPv4, but to reduce traffic, we send and receive using IPv6 only on interfaces 
	// that have no routable IPv4 address. Having a routable IPv4 address assigned is a reasonable indicator of being 
	// on a large configured network, which means there's a good chance that most or all the other devices on that 
	// network should also have v4. By doing this we lose the ability to talk to true v6-only devices on that link, 
	// but we cut the packet rate in half. At this time, reducing the packet rate is more important than v6-only 
	// devices on a large configured network, so we are willing to make that sacrifice.
	
	ifd->interfaceInfo.McastTxRx   = ( ( inIFA->ifa_flags & IFF_MULTICAST ) && !( inIFA->ifa_flags & IFF_POINTTOPOINT ) ) ? mDNStrue : mDNSfalse;
	ifd->interfaceInfo.InterfaceID = NULL;

	for( p = inMDNS->p->interfaceList; p; p = p->next )
	{
		if ( strcmp( p->name, ifd->name ) == 0 )
		{
			if (!ifd->interfaceInfo.InterfaceID)
			{
				ifd->interfaceInfo.InterfaceID	= (mDNSInterfaceID) p;
			}

			if ( ( inIFA->ifa_addr->sa_family != AF_INET ) &&
			     ( p->interfaceInfo.ip.type == mDNSAddrType_IPv4 ) &&
			     ( p->interfaceInfo.ip.ip.v4.b[ 0 ] != 169 || p->interfaceInfo.ip.ip.v4.b[ 1 ] != 254 ) )
			{
				ifd->interfaceInfo.McastTxRx = mDNSfalse;
			}

			break;
		}
	}

	if ( !ifd->interfaceInfo.InterfaceID )
	{
		ifd->interfaceInfo.InterfaceID = (mDNSInterfaceID) ifd;
	}

	// Set up a socket for this interface (if needed).
	
	if( ifd->interfaceInfo.McastTxRx )
	{
		DWORD size;
			
		err = SetupSocket( inMDNS, inIFA->ifa_addr, MulticastDNSPort, &ifd->sock.fd );
		require_noerr( err, exit );
		ifd->sock.addr = ( inIFA->ifa_addr->sa_family == AF_INET6 ) ? AllDNSLinkGroup_v6 : AllDNSLinkGroup_v4;
		ifd->sock.port = MulticastDNSPort;
		
		// Get a ptr to the WSARecvMsg function, if supported. Otherwise, we'll fallback to recvfrom.

		err = WSAIoctl( ifd->sock.fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &kWSARecvMsgGUID, sizeof( kWSARecvMsgGUID ), &ifd->sock.recvMsgPtr, sizeof( ifd->sock.recvMsgPtr ), &size, NULL, NULL );

		if ( err )
		{
			ifd->sock.recvMsgPtr = NULL;
		}
	}

	if ( inIFA->ifa_dhcpEnabled && ( inIFA->ifa_dhcpLeaseExpires < inMDNS->p->nextDHCPLeaseExpires ) )
	{
		inMDNS->p->nextDHCPLeaseExpires = inIFA->ifa_dhcpLeaseExpires;
	}

	ifd->interfaceInfo.NetWake = inIFA->ifa_womp;

	// Register this interface with mDNS.
	
	err = SockAddrToMDNSAddr( inIFA->ifa_addr, &ifd->interfaceInfo.ip, NULL );
	require_noerr( err, exit );
	
	err = SockAddrToMDNSAddr( inIFA->ifa_netmask, &ifd->interfaceInfo.mask, NULL );
	require_noerr( err, exit );

	memcpy( ifd->interfaceInfo.MAC.b, inIFA->ifa_physaddr, sizeof( ifd->interfaceInfo.MAC.b ) );
	
	ifd->interfaceInfo.Advertise = ( mDNSu8 ) inMDNS->AdvertiseLocalAddresses;

	if ( ifd->sock.fd != kInvalidSocketRef )
	{
		err = mDNSPollRegisterSocket( ifd->sock.fd, FD_READ, UDPSocketNotification, &ifd->sock );
		require_noerr( err, exit );
	}

	err = mDNS_RegisterInterface( inMDNS, &ifd->interfaceInfo, mDNSfalse );
	require_noerr( err, exit );
	ifd->hostRegistered = mDNStrue;
	
	dlog( kDebugLevelInfo, DEBUG_NAME "Registered interface %##a with mDNS\n", inIFA->ifa_addr );
	
	// Success!
	
	*outIFD = ifd;
	ifd = NULL;
	
exit:

	if( ifd )
	{
		TearDownInterface( inMDNS, ifd );
	}
	dlog( kDebugLevelTrace, DEBUG_NAME "setting up interface done (err=%d %m)\n", err, err );
	return( err );
}

//===========================================================================================================================
//	TearDownInterface
//===========================================================================================================================

mDNSlocal mStatus	TearDownInterface( mDNS * const inMDNS, mDNSInterfaceData *inIFD )
{	
	check( inMDNS );
	check( inIFD );
	
	// Deregister this interface with mDNS.
	
	dlog( kDebugLevelInfo, DEBUG_NAME "Deregistering interface %#a with mDNS\n", &inIFD->interfaceInfo.ip );
	
	if( inIFD->hostRegistered )
	{
		inIFD->hostRegistered = mDNSfalse;
		mDNS_DeregisterInterface( inMDNS, &inIFD->interfaceInfo, mDNSfalse );
	}
	
	// Tear down the multicast socket.
	
	UDPCloseSocket( &inIFD->sock );

	// If the interface is still referenced by items in the mDNS cache then put it on the inactive list. This keeps 
	// the InterfaceID valid so remove events report the correct interface. If it is no longer referenced, free it.

	if( NumCacheRecordsForInterfaceID( inMDNS, (mDNSInterfaceID) inIFD ) > 0 )
	{
		inIFD->next = inMDNS->p->inactiveInterfaceList;
		inMDNS->p->inactiveInterfaceList = inIFD;
		dlog( kDebugLevelInfo, DEBUG_NAME "deferring free of interface %#p %#a\n", inIFD, &inIFD->interfaceInfo.ip );
	}
	else
	{
		dlog( kDebugLevelInfo, DEBUG_NAME "freeing interface %#p %#a immediately\n", inIFD, &inIFD->interfaceInfo.ip );
		QueueUserAPC( ( PAPCFUNC ) FreeInterface, inMDNS->p->mainThread, ( ULONG_PTR ) inIFD );
	}

	return( mStatus_NoError );
}

mDNSlocal void CALLBACK FreeInterface( mDNSInterfaceData *inIFD )
{
	free( inIFD );
}

//===========================================================================================================================
//	SetupSocket
//===========================================================================================================================

mDNSlocal mStatus	SetupSocket( mDNS * const inMDNS, const struct sockaddr *inAddr, mDNSIPPort port, SocketRef *outSocketRef  )
{
	mStatus			err;
	SocketRef		sock;
	int				option;
	DWORD			bytesReturned = 0;
	BOOL			behavior = FALSE;
	
	DEBUG_UNUSED( inMDNS );
	
	dlog( kDebugLevelTrace, DEBUG_NAME "setting up socket %##a\n", inAddr );
	check( inMDNS );
	check( outSocketRef );
	
	// Set up an IPv4 or IPv6 UDP socket.
	
	sock = socket( inAddr->sa_family, SOCK_DGRAM, IPPROTO_UDP );
	err = translate_errno( IsValidSocket( sock ), errno_compat(), kUnknownErr );
	require_noerr( err, exit );
		
	// Turn on reuse address option so multiple servers can listen for Multicast DNS packets,
	// if we're creating a multicast socket
	
	if ( !mDNSIPPortIsZero( port ) )
	{
		option = 1;
		err = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );
	}

	// <rdar://problem/7894393> Bonjour for Windows broken on Windows XP
	//
	// Not sure why, but the default behavior for sockets is to behave incorrectly
	// when using them in Overlapped I/O mode on XP. According to MSDN:
	//
	// SIO_UDP_CONNRESET (opcode setting: I, T==3)
	//     Windows XP:  Controls whether UDP PORT_UNREACHABLE messages are reported. Set to TRUE to enable reporting.
	//     Set to FALSE to disable reporting.
	//
	// Packet traces from misbehaving Bonjour installations showed that ICMP port unreachable
	// messages were being sent to us after we sent out packets to a multicast address. This is clearly
	// incorrect behavior, but should be harmless. However, after receiving a port unreachable error, WinSock
	// will no longer receive any packets from that socket, which is not harmless. This behavior is only
	// seen on XP.
	//
	// So we turn off port unreachable reporting to make sure our sockets that are reading
	// multicast packets function correctly under all circumstances.

	err = WSAIoctl( sock, SIO_UDP_CONNRESET, &behavior, sizeof(behavior), NULL, 0, &bytesReturned, NULL, NULL );
	check_translated_errno( err == 0, errno_compat(), kOptionErr );

	if( inAddr->sa_family == AF_INET )
	{
		mDNSv4Addr				ipv4;
		struct sockaddr_in		sa4;
		struct ip_mreq			mreqv4;
		
		// Bind the socket to the desired port
		
		ipv4.NotAnInteger 	= ( (const struct sockaddr_in *) inAddr )->sin_addr.s_addr;
		mDNSPlatformMemZero( &sa4, sizeof( sa4 ) );
		sa4.sin_family 		= AF_INET;
		sa4.sin_port 		= port.NotAnInteger;
		sa4.sin_addr.s_addr	= ipv4.NotAnInteger;
		
		err = bind( sock, (struct sockaddr *) &sa4, sizeof( sa4 ) );
		check_translated_errno( err == 0, errno_compat(), kUnknownErr );
		
		// Turn on option to receive destination addresses and receiving interface.
		
		option = 1;
		err = setsockopt( sock, IPPROTO_IP, IP_PKTINFO, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );
		
		if ( !mDNSIPPortIsZero( port ) )
		{
			// Join the all-DNS multicast group so we receive Multicast DNS packets

			mreqv4.imr_multiaddr.s_addr = AllDNSLinkGroup_v4.ip.v4.NotAnInteger;
			mreqv4.imr_interface.s_addr = ipv4.NotAnInteger;
			err = setsockopt( sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreqv4, sizeof( mreqv4 ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );
		
			// Specify the interface to send multicast packets on this socket.
		
			sa4.sin_addr.s_addr = ipv4.NotAnInteger;
			err = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_IF, (char *) &sa4.sin_addr, sizeof( sa4.sin_addr ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );
		
			// Enable multicast loopback so we receive multicast packets we send (for same-machine operations).
		
			option = 1;
			err = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &option, sizeof( option ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );
		}

		// Send unicast packets with TTL 255 (helps against spoofing).
		
		option = 255;
		err = setsockopt( sock, IPPROTO_IP, IP_TTL, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );

		// Send multicast packets with TTL 255 (helps against spoofing).
		
		option = 255;
		err = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );

	}
	else if( inAddr->sa_family == AF_INET6 )
	{
		struct sockaddr_in6 *		sa6p;
		struct sockaddr_in6			sa6;
		struct ipv6_mreq			mreqv6;
		
		sa6p = (struct sockaddr_in6 *) inAddr;
		
		// Bind the socket to the desired port
		
		mDNSPlatformMemZero( &sa6, sizeof( sa6 ) );
		sa6.sin6_family		= AF_INET6;
		sa6.sin6_port		= port.NotAnInteger;
		sa6.sin6_flowinfo	= 0;
		sa6.sin6_addr		= sa6p->sin6_addr;
		sa6.sin6_scope_id	= sa6p->sin6_scope_id;
		
		err = bind( sock, (struct sockaddr *) &sa6, sizeof( sa6 ) );
		check_translated_errno( err == 0, errno_compat(), kUnknownErr );
		
		// Turn on option to receive destination addresses and receiving interface.
		
		option = 1;
		err = setsockopt( sock, IPPROTO_IPV6, IPV6_PKTINFO, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );
		
		// We only want to receive IPv6 packets (not IPv4-mapped IPv6 addresses) because we have a separate socket 
		// for IPv4, but the IPv6 stack in Windows currently doesn't support IPv4-mapped IPv6 addresses and doesn't
		// support the IPV6_V6ONLY socket option so the following code would typically not be executed (or needed).

		#if( defined( IPV6_V6ONLY ) && ! defined( WIN32 ) )
			option = 1;
			err = setsockopt( sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &option, sizeof( option ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );		
		#endif
		
		if ( !mDNSIPPortIsZero( port ) )
		{
			// Join the all-DNS multicast group so we receive Multicast DNS packets.
		
			mreqv6.ipv6mr_multiaddr = *( (struct in6_addr *) &AllDNSLinkGroup_v6.ip.v6 );
			mreqv6.ipv6mr_interface = sa6p->sin6_scope_id;
			err = setsockopt( sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char *) &mreqv6, sizeof( mreqv6 ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );
		
			// Specify the interface to send multicast packets on this socket.
		
			option = (int) sa6p->sin6_scope_id;
			err = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *) &option, sizeof( option ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );
		
			// Enable multicast loopback so we receive multicast packets we send (for same-machine operations).
			
			option = 1;
			err = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *) &option, sizeof( option ) );
			check_translated_errno( err == 0, errno_compat(), kOptionErr );
		}

		// Send unicast packets with TTL 255 (helps against spoofing).
		
		option = 255;
		err = setsockopt( sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );

		// Send multicast packets with TTL 255 (helps against spoofing).
			
		option = 255;
		err = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *) &option, sizeof( option ) );
		check_translated_errno( err == 0, errno_compat(), kOptionErr );
	}
	else
	{
		dlog( kDebugLevelError, DEBUG_NAME "%s: unsupport socket family (%d)\n", __ROUTINE__, inAddr->sa_family );
		err = kUnsupportedErr;
		goto exit;
	}
	
	// Success!
	
	*outSocketRef = sock;
	sock = kInvalidSocketRef;
	err = mStatus_NoError;
	
exit:
	if( IsValidSocket( sock ) )
	{
		close_compat( sock );
	}
	return( err );
}

//===========================================================================================================================
//	SetupSocket
//===========================================================================================================================

mDNSlocal mStatus	SockAddrToMDNSAddr( const struct sockaddr * const inSA, mDNSAddr *outIP, mDNSIPPort *outPort )
{
	mStatus		err;
	
	check( inSA );
	check( outIP );
	
	if( inSA->sa_family == AF_INET )
	{
		struct sockaddr_in *		sa4;
		
		sa4 						= (struct sockaddr_in *) inSA;
		outIP->type 				= mDNSAddrType_IPv4;
		outIP->ip.v4.NotAnInteger	= sa4->sin_addr.s_addr;
		if( outPort )
		{
			outPort->NotAnInteger	= sa4->sin_port;
		}
		err = mStatus_NoError;
	}
	else if( inSA->sa_family == AF_INET6 )
	{
		struct sockaddr_in6 *		sa6;
		
		sa6 			= (struct sockaddr_in6 *) inSA;
		outIP->type 	= mDNSAddrType_IPv6;
		outIP->ip.v6 	= *( (mDNSv6Addr *) &sa6->sin6_addr );
		if( IN6_IS_ADDR_LINKLOCAL( &sa6->sin6_addr ) )
		{
			outIP->ip.v6.w[ 1 ] = 0;
		}
		if( outPort )
		{
			outPort->NotAnInteger = sa6->sin6_port;
		}
		err = mStatus_NoError;
	}
	else
	{
		dlog( kDebugLevelError, DEBUG_NAME "%s: invalid sa_family %d", __ROUTINE__, inSA->sa_family );
		err = mStatus_BadParamErr;
	}
	return( err );
}


#if 0
#pragma mark -
#endif

//===========================================================================================================================
//	UDPSocketNotification
//===========================================================================================================================

mDNSlocal void CALLBACK
UDPSocketNotification( SOCKET sock, LPWSANETWORKEVENTS event, void *context )
{
	UDPSocket				*udpSock = ( UDPSocket* ) context;
	WSAMSG					wmsg;
	WSABUF					wbuf;
	struct sockaddr_storage	sockSrcAddr;		// This is filled in by the WSARecv* function
	INT						sockSrcAddrLen;		// See above
	mDNSAddr				srcAddr;
	mDNSInterfaceID			iid;
	mDNSIPPort				srcPort;
	mDNSAddr				dstAddr;
	mDNSIPPort				dstPort;
	uint8_t					controlBuffer[ 128 ];
	mDNSu8				*	end;
	int						num;
	DWORD					numTries;
	mStatus					err;

	DEBUG_UNUSED( sock );
	DEBUG_UNUSED( event );

	require_action( udpSock != NULL, exit, err = mStatus_BadStateErr );

	dlog( kDebugLevelChatty, DEBUG_NAME "%s: sock = %d\n", __ROUTINE__, udpSock->fd );
	
	// Initialize the buffer structure

	wbuf.buf		= (char *) &udpSock->packet;
	wbuf.len		= (u_long) sizeof( udpSock->packet );
	sockSrcAddrLen	= sizeof( sockSrcAddr );

	numTries = 0;

	do
	{
		if ( udpSock->recvMsgPtr )
		{
			DWORD size;

			wmsg.name			= ( LPSOCKADDR ) &sockSrcAddr;
			wmsg.namelen		= sockSrcAddrLen;
			wmsg.lpBuffers		= &wbuf;
			wmsg.dwBufferCount	= 1;
			wmsg.Control.buf	= ( CHAR* ) controlBuffer;
			wmsg.Control.len	= sizeof( controlBuffer );
			wmsg.dwFlags		= 0;

			err = udpSock->recvMsgPtr( udpSock->fd, &wmsg, &size, NULL, NULL );
			err = translate_errno( ( err == 0 ), (OSStatus) WSAGetLastError(), kUnknownErr ); 
			num = ( int ) size;

			// <rdar://problem/7824093> iTunes 9.1 fails to install with Bonjour service on Windows 7 Ultimate
			//
			// There seems to be a bug in some network device drivers that involves calling WSARecvMsg().
			// Although all the parameters to WSARecvMsg() are correct, it returns a
			// WSAEFAULT error code when there is no actual error. We have found experientially that falling
			// back to using WSARecvFrom() when this happens will work correctly.

			if ( err == WSAEFAULT ) udpSock->recvMsgPtr = NULL;
		}
		else
		{
			DWORD flags = 0;

			num = WSARecvFrom( udpSock->fd, &wbuf, 1, NULL, &flags, ( LPSOCKADDR ) &sockSrcAddr, &sockSrcAddrLen, NULL, NULL );
			err = translate_errno( ( num >= 0 ), ( OSStatus ) WSAGetLastError(), kUnknownErr );
		}

		// According to MSDN <http://msdn.microsoft.com/en-us/library/ms741687(VS.85).aspx>:
		//
		// "WSAECONNRESET: For a UDP datagram socket, this error would indicate that a previous
		//                 send operation resulted in an ICMP "Port Unreachable" message."
		//
		// Because this is the case, we want to ignore this error and try again.  Just in case
		// this is some kind of pathological condition, we'll break out of the retry loop 
		// after 100 iterations

		require_action( !err || ( err == WSAECONNRESET ) || ( err == WSAEFAULT ), exit, err = WSAGetLastError() );
	}
	while ( ( ( err == WSAECONNRESET ) || ( err == WSAEFAULT ) ) && ( numTries++ < 100 ) );
	
	require_noerr( err, exit );
	
	// Translate the source of this packet into mDNS data types

	SockAddrToMDNSAddr( (struct sockaddr* ) &sockSrcAddr, &srcAddr, &srcPort );
	
	// Initialize the destination of this packet. Just in case
	// we can't determine this info because we couldn't call
	// WSARecvMsg (recvMsgPtr)

	dstAddr = udpSock->addr;
	dstPort = udpSock->port;

	if ( udpSock->recvMsgPtr )
	{
		LPWSACMSGHDR	header;
		LPWSACMSGHDR	last = NULL;
		int				count = 0;
		
		// Parse the control information. Reject packets received on the wrong interface.
		
		// <rdar://problem/7832196> INSTALL: Bonjour 2.0 on Windows can not start / stop
		// 
		// There seems to be an interaction between Bullguard and this next bit of code.
		// When a user's machine is running Bullguard, the control information that is
		// returned is corrupted, and the code would go into an infinite loop. We'll add
		// two bits of defensive coding here. The first will check that each pointer to
		// the LPWSACMSGHDR that is returned in the for loop is different than the last.
		// This fixes the problem with Bullguard. The second will break out of this loop
		// after 100 iterations, just in case the corruption isn't caught by the first
		// check.

		for ( header = WSA_CMSG_FIRSTHDR( &wmsg ); header; header = WSA_CMSG_NXTHDR( &wmsg, header ) )
		{
			if ( ( header != last ) && ( ++count < 100 ) )
			{
				last = header;
					
				if ( ( header->cmsg_level == IPPROTO_IP ) && ( header->cmsg_type == IP_PKTINFO ) )
				{
					IN_PKTINFO * ipv4PacketInfo;
					
					ipv4PacketInfo = (IN_PKTINFO *) WSA_CMSG_DATA( header );

					if ( udpSock->ifd != NULL )
					{
						require_action( ipv4PacketInfo->ipi_ifindex == udpSock->ifd->index, exit, err = ( DWORD ) kMismatchErr );
					}

					dstAddr.type 				= mDNSAddrType_IPv4;
					dstAddr.ip.v4.NotAnInteger	= ipv4PacketInfo->ipi_addr.s_addr;
				}
				else if( ( header->cmsg_level == IPPROTO_IPV6 ) && ( header->cmsg_type == IPV6_PKTINFO ) )
				{
					IN6_PKTINFO * ipv6PacketInfo;
						
					ipv6PacketInfo = (IN6_PKTINFO *) WSA_CMSG_DATA( header );
		
					if ( udpSock->ifd != NULL )
					{
						require_action( ipv6PacketInfo->ipi6_ifindex == ( udpSock->ifd->index - kIPv6IfIndexBase ), exit, err = ( DWORD ) kMismatchErr );
					}

					dstAddr.type	= mDNSAddrType_IPv6;
					dstAddr.ip.v6	= *( (mDNSv6Addr *) &ipv6PacketInfo->ipi6_addr );
				}
			}
			else
			{
				static BOOL loggedMessage = FALSE;

				if ( !loggedMessage )
				{
					LogMsg( "UDPEndRecv: WSARecvMsg control information error." );
					loggedMessage = TRUE;
				}

				break;
			}
		}
	}

	dlog( kDebugLevelChatty, DEBUG_NAME "packet received\n" );
	dlog( kDebugLevelChatty, DEBUG_NAME "    size      = %d\n", num );
	dlog( kDebugLevelChatty, DEBUG_NAME "    src       = %#a:%u\n", &srcAddr, ntohs( srcPort.NotAnInteger ) );
	dlog( kDebugLevelChatty, DEBUG_NAME "    dst       = %#a:%u\n", &dstAddr, ntohs( dstPort.NotAnInteger ) );
	
	if ( udpSock->ifd != NULL )
	{
		dlog( kDebugLevelChatty, DEBUG_NAME "    interface = %#a (index=0x%08X)\n", &udpSock->ifd->interfaceInfo.ip, udpSock->ifd->index );
	}

	dlog( kDebugLevelChatty, DEBUG_NAME "\n" );

	iid = udpSock->ifd ? udpSock->ifd->interfaceInfo.InterfaceID : NULL;
	end = ( (mDNSu8 *) &udpSock->packet ) + num;

	mDNSCoreReceive( udpSock->m, &udpSock->packet, end, &srcAddr, srcPort, &dstAddr, dstPort, iid );

exit:

	return;
}


//===========================================================================================================================
//	InterfaceListDidChange
//===========================================================================================================================
void InterfaceListDidChange( mDNS * const inMDNS )
{
	mStatus err;
	
	dlog( kDebugLevelInfo, DEBUG_NAME "interface list changed\n" );
	check( inMDNS );
	
	// Tear down the existing interfaces and set up new ones using the new IP info.
	
	err = TearDownInterfaceList( inMDNS );
	check_noerr( err );
	
	err = SetupInterfaceList( inMDNS );
	check_noerr( err );
		
	err = uDNS_SetupDNSConfig( inMDNS );
	check_noerr( err );
	
	// Inform clients of the change.
	
	mDNS_ConfigChanged(inMDNS);
	
	// Force mDNS to update.
	
	mDNSCoreMachineSleep( inMDNS, mDNSfalse ); // What is this for? Mac OS X does not do this
}


//===========================================================================================================================
//	ComputerDescriptionDidChange
//===========================================================================================================================
void ComputerDescriptionDidChange( mDNS * const inMDNS )
{	
	dlog( kDebugLevelInfo, DEBUG_NAME "computer description has changed\n" );
	check( inMDNS );

	// redo the names
	SetupNiceName( inMDNS );
}


//===========================================================================================================================
//	TCPIPConfigDidChange
//===========================================================================================================================
void TCPIPConfigDidChange( mDNS * const inMDNS )
{
	mStatus		err;
	
	dlog( kDebugLevelInfo, DEBUG_NAME "TCP/IP config has changed\n" );
	check( inMDNS );

	err = uDNS_SetupDNSConfig( inMDNS );
	check_noerr( err );
}


//===========================================================================================================================
//	DynDNSConfigDidChange
//===========================================================================================================================
void DynDNSConfigDidChange( mDNS * const inMDNS )
{
	mStatus		err;
	
	dlog( kDebugLevelInfo, DEBUG_NAME "DynDNS config has changed\n" );
	check( inMDNS );

	SetDomainSecrets( inMDNS );

	err = uDNS_SetupDNSConfig( inMDNS );
	check_noerr( err );
}


//===========================================================================================================================
//	FileSharingDidChange
//===========================================================================================================================
void FileSharingDidChange( mDNS * const inMDNS )
{	
	dlog( kDebugLevelInfo, DEBUG_NAME "File shares has changed\n" );
	check( inMDNS );

	CheckFileShares( inMDNS );
}


//===========================================================================================================================
//	FilewallDidChange
//===========================================================================================================================
void FirewallDidChange( mDNS * const inMDNS )
{	
	dlog( kDebugLevelInfo, DEBUG_NAME "Firewall has changed\n" );
	check( inMDNS );

	CheckFileShares( inMDNS );
}


#if 0
#pragma mark -
#pragma mark == Utilities ==
#endif

//===========================================================================================================================
//	getifaddrs
//===========================================================================================================================

mDNSlocal int	getifaddrs( struct ifaddrs **outAddrs )
{
	int		err;
	
#if( MDNS_WINDOWS_USE_IPV6_IF_ADDRS )
	
	// Try to the load the GetAdaptersAddresses function from the IP Helpers DLL. This API is only available on Windows
	// XP or later. Looking up the symbol at runtime allows the code to still work on older systems without that API.
	
	if( !gIPHelperLibraryInstance )
	{
		gIPHelperLibraryInstance = LoadLibrary( TEXT( "Iphlpapi" ) );
		if( gIPHelperLibraryInstance )
		{
			gGetAdaptersAddressesFunctionPtr = 
				(GetAdaptersAddressesFunctionPtr) GetProcAddress( gIPHelperLibraryInstance, "GetAdaptersAddresses" );
			if( !gGetAdaptersAddressesFunctionPtr )
			{
				BOOL		ok;
				
				ok = FreeLibrary( gIPHelperLibraryInstance );
				check_translated_errno( ok, GetLastError(), kUnknownErr );
				gIPHelperLibraryInstance = NULL;
			}
		}
	}
	
	// Use the new IPv6-capable routine if supported. Otherwise, fall back to the old and compatible IPv4-only code.
	// <rdar://problem/4278934>  Fall back to using getifaddrs_ipv4 if getifaddrs_ipv6 fails
	// <rdar://problem/6145913>  Fall back to using getifaddrs_ipv4 if getifaddrs_ipv6 returns no addrs

	if( !gGetAdaptersAddressesFunctionPtr || ( ( ( err = getifaddrs_ipv6( outAddrs ) ) != mStatus_NoError ) || ( ( outAddrs != NULL ) && ( *outAddrs == NULL ) ) ) )
	{
		err = getifaddrs_ipv4( outAddrs );
		require_noerr( err, exit );
	}
	
#else

	err = getifaddrs_ipv4( outAddrs );
	require_noerr( err, exit );

#endif

exit:
	return( err );
}

#if( MDNS_WINDOWS_USE_IPV6_IF_ADDRS )
//===========================================================================================================================
//	getifaddrs_ipv6
//===========================================================================================================================

mDNSlocal int	getifaddrs_ipv6( struct ifaddrs **outAddrs )
{
	DWORD						err;
	int							i;
	DWORD						flags;
	struct ifaddrs *			head;
	struct ifaddrs **			next;
	IP_ADAPTER_ADDRESSES *		iaaList;
	ULONG						iaaListSize;
	IP_ADAPTER_ADDRESSES *		iaa;
	size_t						size;
	struct ifaddrs *			ifa;
	
	check( gGetAdaptersAddressesFunctionPtr );
	
	head	= NULL;
	next	= &head;
	iaaList	= NULL;
	
	// Get the list of interfaces. The first call gets the size and the second call gets the actual data.
	// This loops to handle the case where the interface changes in the window after getting the size, but before the
	// second call completes. A limit of 100 retries is enforced to prevent infinite loops if something else is wrong.
	
	flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
	i = 0;
	for( ;; )
	{
		iaaListSize = 0;
		err = gGetAdaptersAddressesFunctionPtr( AF_UNSPEC, flags, NULL, NULL, &iaaListSize );
		check( err == ERROR_BUFFER_OVERFLOW );
		check( iaaListSize >= sizeof( IP_ADAPTER_ADDRESSES ) );
		
		iaaList = (IP_ADAPTER_ADDRESSES *) malloc( iaaListSize );
		require_action( iaaList, exit, err = ERROR_NOT_ENOUGH_MEMORY );
		
		err = gGetAdaptersAddressesFunctionPtr( AF_UNSPEC, flags, NULL, iaaList, &iaaListSize );
		if( err == ERROR_SUCCESS ) break;
		
		free( iaaList );
		iaaList = NULL;
		++i;
		require( i < 100, exit );
		dlog( kDebugLevelWarning, "%s: retrying GetAdaptersAddresses after %d failure(s) (%d %m)\n", __ROUTINE__, i, err, err );
	}
	
	for( iaa = iaaList; iaa; iaa = iaa->Next )
	{
		int								addrIndex;
		IP_ADAPTER_UNICAST_ADDRESS	*	addr;
		DWORD							ipv6IfIndex;
		IP_ADAPTER_PREFIX			*	firstPrefix;

		if( iaa->IfIndex > 0xFFFFFF )
		{
			dlog( kDebugLevelAlert, DEBUG_NAME "%s: IPv4 ifindex out-of-range (0x%08X)\n", __ROUTINE__, iaa->IfIndex );
		}
		if( iaa->Ipv6IfIndex > 0xFF )
		{
			dlog( kDebugLevelAlert, DEBUG_NAME "%s: IPv6 ifindex out-of-range (0x%08X)\n", __ROUTINE__, iaa->Ipv6IfIndex );
		}

		// For IPv4 interfaces, there seems to be a bug in iphlpapi.dll that causes the 
		// following code to crash when iterating through the prefix list.  This seems
		// to occur when iaa->Ipv6IfIndex != 0 when IPv6 is not installed on the host.
		// This shouldn't happen according to Microsoft docs which states:
		//
		//     "Ipv6IfIndex contains 0 if IPv6 is not available on the interface."
		//
		// So the data structure seems to be corrupted when we return from
		// GetAdaptersAddresses(). The bug seems to occur when iaa->Length <
		// sizeof(IP_ADAPTER_ADDRESSES), so when that happens, we'll manually
		// modify iaa to have the correct values.

		if ( iaa->Length >= sizeof( IP_ADAPTER_ADDRESSES ) )
		{
			ipv6IfIndex = iaa->Ipv6IfIndex;
			firstPrefix = iaa->FirstPrefix;
		}
		else
		{
			ipv6IfIndex	= 0;
			firstPrefix = NULL;
		}

		// Skip pseudo and tunnel interfaces.
		
		if( ( ( ipv6IfIndex == 1 ) && ( iaa->IfType != IF_TYPE_SOFTWARE_LOOPBACK ) ) || ( iaa->IfType == IF_TYPE_TUNNEL ) )
		{
			continue;
		}
		
		// Add each address as a separate interface to emulate the way getifaddrs works.
		
		for( addrIndex = 0, addr = iaa->FirstUnicastAddress; addr; ++addrIndex, addr = addr->Next )
		{			
			int						family;
			IP_ADAPTER_PREFIX *		prefix;
			uint32_t				ipv4Index;
			struct sockaddr_in		ipv4Netmask;

			family = addr->Address.lpSockaddr->sa_family;
			if( ( family != AF_INET ) && ( family != AF_INET6 ) ) continue;
			
			// <rdar://problem/6220642> iTunes 8: Bonjour doesn't work after upgrading iTunes 8
			// Seems as if the problem here is a buggy implementation of some network interface
			// driver. It is reporting that is has a link-local address when it is actually
			// disconnected. This was causing a problem in AddressToIndexAndMask.
			// The solution is to call AddressToIndexAndMask first, and if unable to lookup
			// the address, to ignore that address.

			ipv4Index = 0;
			memset( &ipv4Netmask, 0, sizeof( ipv4Netmask ) );
			
			if ( family == AF_INET )
			{
				err = AddressToIndexAndMask( addr->Address.lpSockaddr, &ipv4Index, ( struct sockaddr* ) &ipv4Netmask );
				
				if ( err )
				{
					err = 0;
					continue;
				}
			}

			ifa = (struct ifaddrs *) calloc( 1, sizeof( struct ifaddrs ) );
			require_action( ifa, exit, err = WSAENOBUFS );
			
			*next = ifa;
			next  = &ifa->ifa_next;
			
			// Get the name.
			
			size = strlen( iaa->AdapterName ) + 1;
			ifa->ifa_name = (char *) malloc( size );
			require_action( ifa->ifa_name, exit, err = WSAENOBUFS );
			memcpy( ifa->ifa_name, iaa->AdapterName, size );
			
			// Get interface flags.
			
			ifa->ifa_flags = 0;
			if( iaa->OperStatus == IfOperStatusUp ) 		ifa->ifa_flags |= IFF_UP;
			if( iaa->IfType == IF_TYPE_SOFTWARE_LOOPBACK )	ifa->ifa_flags |= IFF_LOOPBACK;
			else if ( IsPointToPoint( addr ) )				ifa->ifa_flags |= IFF_POINTTOPOINT;
			if( !( iaa->Flags & IP_ADAPTER_NO_MULTICAST ) )	ifa->ifa_flags |= IFF_MULTICAST;

			
			// <rdar://problem/4045657> Interface index being returned is 512
			//
			// Windows does not have a uniform scheme for IPv4 and IPv6 interface indexes.
			// This code used to shift the IPv4 index up to ensure uniqueness between
			// it and IPv6 indexes.  Although this worked, it was somewhat confusing to developers, who
			// then see interface indexes passed back that don't correspond to anything
			// that is seen in Win32 APIs or command line tools like "route".  As a relatively
			// small percentage of developers are actively using IPv6, it seems to 
			// make sense to make our use of IPv4 as confusion free as possible.
			// So now, IPv6 interface indexes will be shifted up by a
			// constant value which will serve to uniquely identify them, and we will
			// leave IPv4 interface indexes unmodified.
			
			switch( family )
			{
				case AF_INET:  ifa->ifa_extra.index = iaa->IfIndex; break;
				case AF_INET6: ifa->ifa_extra.index = ipv6IfIndex + kIPv6IfIndexBase;	 break;
				default: break;
			}

			// Get lease lifetime

			if ( ( iaa->IfType != IF_TYPE_SOFTWARE_LOOPBACK ) && ( addr->LeaseLifetime != 0 ) && ( addr->ValidLifetime != 0xFFFFFFFF ) )
			{
				ifa->ifa_dhcpEnabled		= TRUE;
				ifa->ifa_dhcpLeaseExpires	= time( NULL ) + addr->ValidLifetime;
			}
			else
			{
				ifa->ifa_dhcpEnabled		= FALSE;
				ifa->ifa_dhcpLeaseExpires	= 0;
			}

			if ( iaa->PhysicalAddressLength == sizeof( ifa->ifa_physaddr ) )
			{
				memcpy( ifa->ifa_physaddr, iaa->PhysicalAddress, iaa->PhysicalAddressLength );
			}

			// Because we don't get notified of womp changes, we're going to just assume
			// that all wired interfaces have it enabled. Before we go to sleep, we'll check
			// if the interface actually supports it, and update mDNS->SystemWakeOnLANEnabled
			// accordingly

			ifa->ifa_womp = ( iaa->IfType == IF_TYPE_ETHERNET_CSMACD ) ? mDNStrue : mDNSfalse;
			
			// Get address.
			
			switch( family )
			{
				case AF_INET:
				case AF_INET6:
					ifa->ifa_addr = (struct sockaddr *) calloc( 1, (size_t) addr->Address.iSockaddrLength );
					require_action( ifa->ifa_addr, exit, err = WSAENOBUFS );
					memcpy( ifa->ifa_addr, addr->Address.lpSockaddr, (size_t) addr->Address.iSockaddrLength );
					break;
				
				default:
					break;
			}
			check( ifa->ifa_addr );
			
			// Get subnet mask (IPv4)/link prefix (IPv6). It is specified as a bit length (e.g. 24 for 255.255.255.0).

			switch ( family )
			{
				case AF_INET:
				{
					struct sockaddr_in * sa4;
					
					sa4 = (struct sockaddr_in *) calloc( 1, sizeof( *sa4 ) );
					require_action( sa4, exit, err = WSAENOBUFS );
					sa4->sin_family = AF_INET;
					sa4->sin_addr.s_addr = ipv4Netmask.sin_addr.s_addr;

					dlog( kDebugLevelInfo, DEBUG_NAME "%s: IPv4 mask = %s\n", __ROUTINE__, inet_ntoa( sa4->sin_addr ) );
					ifa->ifa_netmask = (struct sockaddr *) sa4;
					break;
				}

				case AF_INET6:
				{
					struct sockaddr_in6 *sa6;
					char buf[ 256 ] = { 0 };
					DWORD buflen = sizeof( buf );

					sa6 = (struct sockaddr_in6 *) calloc( 1, sizeof( *sa6 ) );
					require_action( sa6, exit, err = WSAENOBUFS );
					sa6->sin6_family = AF_INET6;
					memset( sa6->sin6_addr.s6_addr, 0xFF, sizeof( sa6->sin6_addr.s6_addr ) );
					ifa->ifa_netmask = (struct sockaddr *) sa6;

					for ( prefix = firstPrefix; prefix; prefix = prefix->Next )
					{
						IN6_ADDR	mask;
						IN6_ADDR	maskedAddr;
						int			maskIndex;
						DWORD		len;

						// According to MSDN:
						// "On Windows Vista and later, the linked IP_ADAPTER_PREFIX structures pointed to by the FirstPrefix member
						// include three IP adapter prefixes for each IP address assigned to the adapter. These include the host IP address prefix,
						// the subnet IP address prefix, and the subnet broadcast IP address prefix.
						// In addition, for each adapter there is a multicast address prefix and a broadcast address prefix.
						// On Windows XP with SP1 and later prior to Windows Vista, the linked IP_ADAPTER_PREFIX structures pointed to by the FirstPrefix member
						// include only a single IP adapter prefix for each IP address assigned to the adapter."
						
						// We're only interested in the subnet IP address prefix.  We'll determine if the prefix is the
						// subnet prefix by masking our address with a mask (computed from the prefix length) and see if that is the same
						// as the prefix address.

						if ( ( prefix->PrefixLength == 0 ) ||
						     ( prefix->PrefixLength > 128 ) ||
						     ( addr->Address.iSockaddrLength != prefix->Address.iSockaddrLength ) ||
							 ( memcmp( addr->Address.lpSockaddr, prefix->Address.lpSockaddr, addr->Address.iSockaddrLength ) == 0 ) )
						{
							continue;
						}

						// Compute the mask

						memset( mask.s6_addr, 0, sizeof( mask.s6_addr ) );

						for ( len = (int) prefix->PrefixLength, maskIndex = 0; len > 0; len -= 8 )
						{
							uint8_t maskByte = ( len >= 8 ) ? 0xFF : (uint8_t)( ( 0xFFU << ( 8 - len ) ) & 0xFFU );
							mask.s6_addr[ maskIndex++ ] = maskByte;
						}

						// Apply the mask

						for ( i = 0; i < 16; i++ )
						{
							maskedAddr.s6_addr[ i ] = ( ( struct sockaddr_in6* ) addr->Address.lpSockaddr )->sin6_addr.s6_addr[ i ] & mask.s6_addr[ i ];
						}

						// Compare

						if ( memcmp( ( ( struct sockaddr_in6* ) prefix->Address.lpSockaddr )->sin6_addr.s6_addr, maskedAddr.s6_addr, sizeof( maskedAddr.s6_addr ) ) == 0 )
						{
							memcpy( sa6->sin6_addr.s6_addr, mask.s6_addr, sizeof( mask.s6_addr ) );
							break;
						}
					}

					WSAAddressToStringA( ( LPSOCKADDR ) sa6, sizeof( struct sockaddr_in6 ), NULL, buf, &buflen );
					dlog( kDebugLevelInfo, DEBUG_NAME "%s: IPv6 mask = %s\n", __ROUTINE__, buf );				

					break;
				}
				
				default:
					break;
			}
		}
	}
	
	// Success!
	
	if( outAddrs )
	{
		*outAddrs = head;
		head = NULL;
	}
	err = ERROR_SUCCESS;
	
exit:
	if( head )
	{
		freeifaddrs( head );
	}
	if( iaaList )
	{
		free( iaaList );
	}
	return( (int) err );
}

#endif	// MDNS_WINDOWS_USE_IPV6_IF_ADDRS

//===========================================================================================================================
//	getifaddrs_ipv4
//===========================================================================================================================

mDNSlocal int	getifaddrs_ipv4( struct ifaddrs **outAddrs )
{
	int						err;
	SOCKET					sock;
	DWORD					size;
	DWORD					actualSize;
	INTERFACE_INFO *		buffer;
	INTERFACE_INFO *		tempBuffer;
	INTERFACE_INFO *		ifInfo;
	int						n;
	int						i;
	struct ifaddrs *		head;
	struct ifaddrs **		next;
	struct ifaddrs *		ifa;
	
	sock	= INVALID_SOCKET;
	buffer	= NULL;
	head	= NULL;
	next	= &head;
	
	// Get the interface list. WSAIoctl is called with SIO_GET_INTERFACE_LIST, but since this does not provide a 
	// way to determine the size of the interface list beforehand, we have to start with an initial size guess and
	// call WSAIoctl repeatedly with increasing buffer sizes until it succeeds. Limit this to 100 tries for safety.
	
	sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	err = translate_errno( IsValidSocket( sock ), errno_compat(), kUnknownErr );
	require_noerr( err, exit );
		
	n = 0;
	size = 16 * sizeof( INTERFACE_INFO );
	for( ;; )
	{
		tempBuffer = (INTERFACE_INFO *) realloc( buffer, size );
		require_action( tempBuffer, exit, err = WSAENOBUFS );
		buffer = tempBuffer;
		
		err = WSAIoctl( sock, SIO_GET_INTERFACE_LIST, NULL, 0, buffer, size, &actualSize, NULL, NULL );
		if( err == 0 )
		{
			break;
		}
		
		++n;
		require_action( n < 100, exit, err = WSAEADDRNOTAVAIL );
		
		size += ( 16 * sizeof( INTERFACE_INFO ) );
	}
	check( actualSize <= size );
	check( ( actualSize % sizeof( INTERFACE_INFO ) ) == 0 );
	n = (int)( actualSize / sizeof( INTERFACE_INFO ) );
	
	// Process the raw interface list and build a linked list of IPv4 interfaces.
	
	for( i = 0; i < n; ++i )
	{
		uint32_t ifIndex;
		struct sockaddr_in netmask;
		
		ifInfo = &buffer[ i ];
		if( ifInfo->iiAddress.Address.sa_family != AF_INET )
		{
			continue;
		}
		
		// <rdar://problem/6220642> iTunes 8: Bonjour doesn't work after upgrading iTunes 8
		// See comment in getifaddrs_ipv6

		ifIndex = 0;
		memset( &netmask, 0, sizeof( netmask ) );
		err = AddressToIndexAndMask( ( struct sockaddr* ) &ifInfo->iiAddress.AddressIn, &ifIndex, ( struct sockaddr* ) &netmask );

		if ( err )
		{
			continue;
		}

		ifa = (struct ifaddrs *) calloc( 1, sizeof( struct ifaddrs ) );
		require_action( ifa, exit, err = WSAENOBUFS );
		
		*next = ifa;
		next  = &ifa->ifa_next;
		
		// Get the name.
		
		ifa->ifa_name = (char *) malloc( 16 );
		require_action( ifa->ifa_name, exit, err = WSAENOBUFS );
		sprintf( ifa->ifa_name, "%d", i + 1 );
		
		// Get interface flags.
		
		ifa->ifa_flags = (u_int) ifInfo->iiFlags;
		
		// Get addresses.
		
		if ( ifInfo->iiAddress.Address.sa_family == AF_INET )
		{
			struct sockaddr_in *		sa4;
			
			sa4 = &ifInfo->iiAddress.AddressIn;
			ifa->ifa_addr = (struct sockaddr *) calloc( 1, sizeof( *sa4 ) );
			require_action( ifa->ifa_addr, exit, err = WSAENOBUFS );
			memcpy( ifa->ifa_addr, sa4, sizeof( *sa4 ) );

			ifa->ifa_netmask = (struct sockaddr*) calloc(1, sizeof( *sa4 ) );
			require_action( ifa->ifa_netmask, exit, err = WSAENOBUFS );

			// <rdar://problem/4076478> Service won't start on Win2K. The address
			// family field was not being initialized.

			ifa->ifa_netmask->sa_family = AF_INET;
			( ( struct sockaddr_in* ) ifa->ifa_netmask )->sin_addr = netmask.sin_addr;
			ifa->ifa_extra.index = ifIndex;
		}
		else
		{
			// Emulate an interface index.
		
			ifa->ifa_extra.index = (uint32_t)( i + 1 );
		}
	}
	
	// Success!
	
	if( outAddrs )
	{
		*outAddrs = head;
		head = NULL;
	}
	err = 0;
	
exit:

	if( head )
	{
		freeifaddrs( head );
	}
	if( buffer )
	{
		free( buffer );
	}
	if( sock != INVALID_SOCKET )
	{
		closesocket( sock );
	}
	return( err );
}

//===========================================================================================================================
//	freeifaddrs
//===========================================================================================================================

mDNSlocal void	freeifaddrs( struct ifaddrs *inIFAs )
{
	struct ifaddrs *		p;
	struct ifaddrs *		q;
	
	// Free each piece of the structure. Set to null after freeing to handle macro-aliased fields.
	
	for( p = inIFAs; p; p = q )
	{
		q = p->ifa_next;
		
		if( p->ifa_name )
		{
			free( p->ifa_name );
			p->ifa_name = NULL;
		}
		if( p->ifa_addr )
		{
			free( p->ifa_addr );
			p->ifa_addr = NULL;
		}
		if( p->ifa_netmask )
		{
			free( p->ifa_netmask );
			p->ifa_netmask = NULL;
		}
		if( p->ifa_broadaddr )
		{
			free( p->ifa_broadaddr );
			p->ifa_broadaddr = NULL;
		}
		if( p->ifa_dstaddr )
		{
			free( p->ifa_dstaddr );
			p->ifa_dstaddr = NULL;
		}
		if( p->ifa_data )
		{
			free( p->ifa_data );
			p->ifa_data = NULL;
		}
		free( p );
	}
}


//===========================================================================================================================
//	GetPrimaryInterface
//===========================================================================================================================

mDNSlocal DWORD
GetPrimaryInterface()
{
	PMIB_IPFORWARDTABLE	pIpForwardTable	= NULL;
	DWORD				dwSize			= 0;
	BOOL				bOrder			= FALSE;
	OSStatus			err;
	DWORD				index			= 0;
	DWORD				metric			= 0;
	unsigned long int	i;

	// Find out how big our buffer needs to be.

	err = GetIpForwardTable(NULL, &dwSize, bOrder);
	require_action( err == ERROR_INSUFFICIENT_BUFFER, exit, err = kUnknownErr );

	// Allocate the memory for the table

	pIpForwardTable = (PMIB_IPFORWARDTABLE) malloc( dwSize );
	require_action( pIpForwardTable, exit, err = kNoMemoryErr );
  
	// Now get the table.

	err = GetIpForwardTable(pIpForwardTable, &dwSize, bOrder);
	require_noerr( err, exit );


	// Search for the row in the table we want.

	for ( i = 0; i < pIpForwardTable->dwNumEntries; i++)
	{
		// Look for a default route

		if ( pIpForwardTable->table[i].dwForwardDest == 0 )
		{
			if ( index && ( pIpForwardTable->table[i].dwForwardMetric1 >= metric ) )
			{
				continue;
			}

			index	= pIpForwardTable->table[i].dwForwardIfIndex;
			metric	= pIpForwardTable->table[i].dwForwardMetric1;
		}
	}

exit:

	if ( pIpForwardTable != NULL )
	{
		free( pIpForwardTable );
	}

	return index;
}


//===========================================================================================================================
//	AddressToIndexAndMask
//===========================================================================================================================

mDNSlocal mStatus
AddressToIndexAndMask( struct sockaddr * addr, uint32_t * ifIndex, struct sockaddr * mask  )
{
	// Before calling AddIPAddress we use GetIpAddrTable to get
	// an adapter to which we can add the IP.
	
	PMIB_IPADDRTABLE	pIPAddrTable	= NULL;
	DWORD				dwSize			= 0;
	mStatus				err				= mStatus_UnknownErr;
	DWORD				i;

	// For now, this is only for IPv4 addresses.  That is why we can safely cast
	// addr's to sockaddr_in.

	require_action( addr->sa_family == AF_INET, exit, err = mStatus_UnknownErr );

	// Make an initial call to GetIpAddrTable to get the
	// necessary size into the dwSize variable

	for ( i = 0; i < 100; i++ )
	{
		err = GetIpAddrTable( pIPAddrTable, &dwSize, 0 );

		if ( err != ERROR_INSUFFICIENT_BUFFER )
		{
			break;
		}

		pIPAddrTable = (MIB_IPADDRTABLE *) realloc( pIPAddrTable, dwSize );
		require_action( pIPAddrTable, exit, err = WSAENOBUFS );
	}

	require_noerr( err, exit );
	err = mStatus_UnknownErr;

	for ( i = 0; i < pIPAddrTable->dwNumEntries; i++ )
	{
		if ( ( ( struct sockaddr_in* ) addr )->sin_addr.s_addr == pIPAddrTable->table[i].dwAddr )
		{
			*ifIndex											= pIPAddrTable->table[i].dwIndex;
			( ( struct sockaddr_in*) mask )->sin_addr.s_addr	= pIPAddrTable->table[i].dwMask;
			err													= mStatus_NoError;
			break;
		}
	}

exit:

	if ( pIPAddrTable )
	{
		free( pIPAddrTable );
	}

	return err;
}


//===========================================================================================================================
//	CanReceiveUnicast
//===========================================================================================================================

mDNSlocal mDNSBool	CanReceiveUnicast( void )
{
	mDNSBool				ok;
	SocketRef				sock;
	struct sockaddr_in		addr;
	
	// Try to bind to the port without the SO_REUSEADDR option to test if someone else has already bound to it.
	
	sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	check_translated_errno( IsValidSocket( sock ), errno_compat(), kUnknownErr );
	ok = IsValidSocket( sock );
	if( ok )
	{
		mDNSPlatformMemZero( &addr, sizeof( addr ) );
		addr.sin_family			= AF_INET;
		addr.sin_port			= MulticastDNSPort.NotAnInteger;
		addr.sin_addr.s_addr	= htonl( INADDR_ANY );
		
		ok = ( bind( sock, (struct sockaddr *) &addr, sizeof( addr ) ) == 0 );
		close_compat( sock );
	}
	
	dlog( kDebugLevelInfo, DEBUG_NAME "Unicast UDP responses %s\n", ok ? "okay" : "*not allowed*" );
	return( ok );
}


//===========================================================================================================================
//	IsPointToPoint
//===========================================================================================================================

mDNSlocal mDNSBool IsPointToPoint( IP_ADAPTER_UNICAST_ADDRESS * addr )
{
	struct ifaddrs	*	addrs	=	NULL;
	struct ifaddrs	*	p		=	NULL;
	OSStatus			err;
	mDNSBool			ret		=	mDNSfalse;

	// For now, only works for IPv4 interfaces

	if ( addr->Address.lpSockaddr->sa_family == AF_INET )
	{
		// The getifaddrs_ipv4 call will give us correct information regarding IFF_POINTTOPOINT flags.

		err = getifaddrs_ipv4( &addrs );
		require_noerr( err, exit );

		for ( p = addrs; p; p = p->ifa_next )
		{
			if ( ( addr->Address.lpSockaddr->sa_family == p->ifa_addr->sa_family ) &&
			     ( ( ( struct sockaddr_in* ) addr->Address.lpSockaddr )->sin_addr.s_addr == ( ( struct sockaddr_in* ) p->ifa_addr )->sin_addr.s_addr ) )
			{
				ret = ( p->ifa_flags & IFF_POINTTOPOINT ) ? mDNStrue : mDNSfalse;
				break;
			}
		}
	}

exit:

	if ( addrs )
	{
		freeifaddrs( addrs );
	}

	return ret;
}


//===========================================================================================================================
//	GetWindowsVersionString
//===========================================================================================================================

mDNSlocal OSStatus	GetWindowsVersionString( char *inBuffer, size_t inBufferSize )
{
#if( !defined( VER_PLATFORM_WIN32_CE ) )
	#define VER_PLATFORM_WIN32_CE		3
#endif

	OSStatus				err;
	OSVERSIONINFO			osInfo;
	BOOL					ok;
	const char *			versionString;
	DWORD					platformID;
	DWORD					majorVersion;
	DWORD					minorVersion;
	DWORD					buildNumber;
	
	versionString = "unknown Windows version";
	
	osInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	ok = GetVersionEx( &osInfo );
	err = translate_errno( ok, (OSStatus) GetLastError(), kUnknownErr );
	require_noerr( err, exit );
	
	platformID		= osInfo.dwPlatformId;
	majorVersion	= osInfo.dwMajorVersion;
	minorVersion	= osInfo.dwMinorVersion;
	buildNumber		= osInfo.dwBuildNumber & 0xFFFF;
	
	if( ( platformID == VER_PLATFORM_WIN32_WINDOWS ) && ( majorVersion == 4 ) )
	{
		if( ( minorVersion < 10 ) && ( buildNumber == 950 ) )
		{
			versionString	= "Windows 95";
		}
		else if( ( minorVersion < 10 ) && ( ( buildNumber > 950 ) && ( buildNumber <= 1080 ) ) )
		{
			versionString	= "Windows 95 SP1";
		}
		else if( ( minorVersion < 10 ) && ( buildNumber > 1080 ) )
		{
			versionString	= "Windows 95 OSR2";
		}
		else if( ( minorVersion == 10 ) && ( buildNumber == 1998 ) )
		{
			versionString	= "Windows 98";
		}
		else if( ( minorVersion == 10 ) && ( ( buildNumber > 1998 ) && ( buildNumber < 2183 ) ) )
		{
			versionString	= "Windows 98 SP1";
		}
		else if( ( minorVersion == 10 ) && ( buildNumber >= 2183 ) )
		{
			versionString	= "Windows 98 SE";
		}
		else if( minorVersion == 90 )
		{
			versionString	= "Windows ME";
		}
	}
	else if( platformID == VER_PLATFORM_WIN32_NT )
	{
		if( ( majorVersion == 3 ) && ( minorVersion == 51 ) )
		{
			versionString	= "Windows NT 3.51";
		}
		else if( ( majorVersion == 4 ) && ( minorVersion == 0 ) )
		{
			versionString	= "Windows NT 4";
		}
		else if( ( majorVersion == 5 ) && ( minorVersion == 0 ) )
		{
			versionString	= "Windows 2000";
		}
		else if( ( majorVersion == 5 ) && ( minorVersion == 1 ) )
		{
			versionString	= "Windows XP";
		}
		else if( ( majorVersion == 5 ) && ( minorVersion == 2 ) )
		{
			versionString	= "Windows Server 2003";
		}
	}
	else if( platformID == VER_PLATFORM_WIN32_CE )
	{
		versionString		= "Windows CE";
	}
	
exit:
	if( inBuffer && ( inBufferSize > 0 ) )
	{
		inBufferSize -= 1;
		strncpy( inBuffer, versionString, inBufferSize );
		inBuffer[ inBufferSize ] = '\0';
	}
	return( err );
}


//===========================================================================================================================
//	RegQueryString
//===========================================================================================================================

mDNSlocal mStatus
RegQueryString( HKEY key, LPCSTR valueName, LPSTR * string, DWORD * stringLen, DWORD * enabled )
{
	DWORD	type;
	int		i;
	mStatus err;

	*stringLen	= MAX_ESCAPED_DOMAIN_NAME;
	*string		= NULL;
	i			= 0;

	do
	{
		if ( *string )
		{
			free( *string );
		}

		*string = (char*) malloc( *stringLen );
		require_action( *string, exit, err = mStatus_NoMemoryErr );

		err = RegQueryValueExA( key, valueName, 0, &type, (LPBYTE) *string, stringLen );

		i++;
	}
	while ( ( err == ERROR_MORE_DATA ) && ( i < 100 ) );

	require_noerr_quiet( err, exit );

	if ( enabled )
	{
		DWORD dwSize = sizeof( DWORD );

		err = RegQueryValueEx( key, TEXT("Enabled"), NULL, NULL, (LPBYTE) enabled, &dwSize );
		check_noerr( err );

		err = kNoErr;
	}

exit:

	return err;
}


//===========================================================================================================================
//	StringToAddress
//===========================================================================================================================

mDNSlocal mStatus StringToAddress( mDNSAddr * ip, LPSTR string )
{
	struct sockaddr_in6 sa6;
	struct sockaddr_in	sa4;
	INT					dwSize;
	mStatus				err;

	sa6.sin6_family	= AF_INET6;
	dwSize			= sizeof( sa6 );

	err = WSAStringToAddressA( string, AF_INET6, NULL, (struct sockaddr*) &sa6, &dwSize );

	if ( err == mStatus_NoError )
	{
		err = SetupAddr( ip, (struct sockaddr*) &sa6 );
		require_noerr( err, exit );
	}
	else
	{
		sa4.sin_family = AF_INET;
		dwSize = sizeof( sa4 );

		err = WSAStringToAddressA( string, AF_INET, NULL, (struct sockaddr*) &sa4, &dwSize );
		err = translate_errno( err == 0, WSAGetLastError(), kUnknownErr );
		require_noerr( err, exit );
			
		err = SetupAddr( ip, (struct sockaddr*) &sa4 );
		require_noerr( err, exit );
	}

exit:

	return err;
}


//===========================================================================================================================
//	myGetIfAddrs
//===========================================================================================================================

mDNSlocal struct ifaddrs*
myGetIfAddrs(int refresh)
{
	static struct ifaddrs *ifa = NULL;
	
	if (refresh && ifa)
	{
		freeifaddrs(ifa);
		ifa = NULL;
	}
	
	if (ifa == NULL)
	{
		getifaddrs(&ifa);
	}
	
	return ifa;
}


//===========================================================================================================================
//	TCHARtoUTF8
//===========================================================================================================================

mDNSlocal OSStatus
TCHARtoUTF8( const TCHAR *inString, char *inBuffer, size_t inBufferSize )
{
#if( defined( UNICODE ) || defined( _UNICODE ) )
	OSStatus		err;
	int				len;
	
	len = WideCharToMultiByte( CP_UTF8, 0, inString, -1, inBuffer, (int) inBufferSize, NULL, NULL );
	err = translate_errno( len > 0, errno_compat(), kUnknownErr );
	require_noerr( err, exit );
	
exit:
	return( err );
#else
	return( WindowsLatin1toUTF8( inString, inBuffer, inBufferSize ) );
#endif
}


//===========================================================================================================================
//	WindowsLatin1toUTF8
//===========================================================================================================================

mDNSlocal OSStatus
WindowsLatin1toUTF8( const char *inString, char *inBuffer, size_t inBufferSize )
{
	OSStatus		err;
	WCHAR *			utf16;
	int				len;
	
	utf16 = NULL;
	
	// Windows doesn't support going directly from Latin-1 to UTF-8 so we have to go from Latin-1 to UTF-16 first.
	
	len = MultiByteToWideChar( CP_ACP, 0, inString, -1, NULL, 0 );
	err = translate_errno( len > 0, errno_compat(), kUnknownErr );
	require_noerr( err, exit );
	
	utf16 = (WCHAR *) malloc( len * sizeof( *utf16 ) );
	require_action( utf16, exit, err = kNoMemoryErr );
	
	len = MultiByteToWideChar( CP_ACP, 0, inString, -1, utf16, len );
	err = translate_errno( len > 0, errno_compat(), kUnknownErr );
	require_noerr( err, exit );
	
	// Now convert the temporary UTF-16 to UTF-8.
	
	len = WideCharToMultiByte( CP_UTF8, 0, utf16, -1, inBuffer, (int) inBufferSize, NULL, NULL );
	err = translate_errno( len > 0, errno_compat(), kUnknownErr );
	require_noerr( err, exit );

exit:
	if( utf16 ) free( utf16 );
	return( err );
}


//===========================================================================================================================
//	TCPCloseSocket
//===========================================================================================================================

mDNSlocal void
TCPCloseSocket( TCPSocket * sock )
{
	dlog( kDebugLevelChatty, DEBUG_NAME "closing TCPSocket 0x%x:%d\n", sock, sock->fd );

	if ( sock->fd != INVALID_SOCKET )
	{
		closesocket( sock->fd );
		sock->fd = INVALID_SOCKET;
	}
}


//===========================================================================================================================
//  UDPCloseSocket
//===========================================================================================================================

mDNSlocal void
UDPCloseSocket( UDPSocket * sock )
{
	dlog( kDebugLevelChatty, DEBUG_NAME "closing UDPSocket %d\n", sock->fd );

	if ( sock->fd != INVALID_SOCKET )
	{
		mDNSPollUnregisterSocket( sock->fd );
		closesocket( sock->fd );
		sock->fd = INVALID_SOCKET;
	}
}


//===========================================================================================================================
//	SetupAddr
//===========================================================================================================================

mDNSlocal mStatus SetupAddr(mDNSAddr *ip, const struct sockaddr *const sa)
	{
	if (!sa) { LogMsg("SetupAddr ERROR: NULL sockaddr"); return(mStatus_Invalid); }

	if (sa->sa_family == AF_INET)
		{
		struct sockaddr_in *ifa_addr = (struct sockaddr_in *)sa;
		ip->type = mDNSAddrType_IPv4;
		ip->ip.v4.NotAnInteger = ifa_addr->sin_addr.s_addr;
		return(mStatus_NoError);
		}

	if (sa->sa_family == AF_INET6)
		{
		struct sockaddr_in6 *ifa_addr = (struct sockaddr_in6 *)sa;
		ip->type = mDNSAddrType_IPv6;
		if (IN6_IS_ADDR_LINKLOCAL(&ifa_addr->sin6_addr)) ifa_addr->sin6_addr.u.Word[1] = 0;
		ip->ip.v6 = *(mDNSv6Addr*)&ifa_addr->sin6_addr;
		return(mStatus_NoError);
		}

	LogMsg("SetupAddr invalid sa_family %d", sa->sa_family);
	return(mStatus_Invalid);
	}


mDNSlocal void GetDDNSFQDN( domainname *const fqdn )
{
	LPSTR		name = NULL;
	DWORD		dwSize;
	DWORD		enabled;
	HKEY		key = NULL;
	OSStatus	err;

	check( fqdn );

	// Initialize

	fqdn->c[0] = '\0';

	// Get info from Bonjour registry key

	err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode TEXT("\\DynDNS\\Setup\\") kServiceDynDNSHostNames, &key );
	require_noerr( err, exit );

	err = RegQueryString( key, "", &name, &dwSize, &enabled );
	if ( !err && ( name[0] != '\0' ) && enabled )
	{
		if ( !MakeDomainNameFromDNSNameString( fqdn, name ) || !fqdn->c[0] )
		{
			dlog( kDebugLevelError, "bad DDNS host name in registry: %s", name[0] ? name : "(unknown)");
		}
	}

exit:

	if ( key )
	{
		RegCloseKey( key );
		key = NULL;
	}

	if ( name )
	{
		free( name );
		name = NULL;
	}
}


#ifdef UNICODE
mDNSlocal void GetDDNSDomains( DNameListElem ** domains, LPCWSTR lpSubKey )
#else
mDNSlocal void GetDDNSConfig( DNameListElem ** domains, LPCSTR lpSubKey )
#endif
{
	char		subKeyName[kRegistryMaxKeyLength + 1];
	DWORD		cSubKeys = 0;
	DWORD		cbMaxSubKey;
	DWORD		cchMaxClass;
	DWORD		dwSize;
	HKEY		key = NULL;
	HKEY		subKey = NULL;
	domainname	dname;
	DWORD		i;
	OSStatus	err;

	check( domains );

	// Initialize

	*domains = NULL;

	err = RegCreateKey( HKEY_CURRENT_USER, lpSubKey, &key );
	require_noerr( err, exit );

	// Get information about this node

	err = RegQueryInfoKey( key, NULL, NULL, NULL, &cSubKeys, &cbMaxSubKey, &cchMaxClass, NULL, NULL, NULL, NULL, NULL );       
	require_noerr( err, exit );

	for ( i = 0; i < cSubKeys; i++)
	{
		DWORD enabled;

		dwSize = kRegistryMaxKeyLength;
        
		err = RegEnumKeyExA( key, i, subKeyName, &dwSize, NULL, NULL, NULL, NULL );

		if ( !err )
		{
			err = RegOpenKeyExA( key, subKeyName, 0, KEY_READ, &subKey );
			require_noerr( err, exit );

			dwSize = sizeof( DWORD );
			err = RegQueryValueExA( subKey, "Enabled", NULL, NULL, (LPBYTE) &enabled, &dwSize );

			if ( !err && ( subKeyName[0] != '\0' ) && enabled )
			{
				if ( !MakeDomainNameFromDNSNameString( &dname, subKeyName ) || !dname.c[0] )
				{
					dlog( kDebugLevelError, "bad DDNS domain in registry: %s", subKeyName[0] ? subKeyName : "(unknown)");
				}
				else
				{
					DNameListElem * domain = (DNameListElem*) malloc( sizeof( DNameListElem ) );
					require_action( domain, exit, err = mStatus_NoMemoryErr );
					
					AssignDomainName(&domain->name, &dname);
					domain->next = *domains;

					*domains = domain;
				}
			}

			RegCloseKey( subKey );
			subKey = NULL;
		}
	}

exit:

	if ( subKey )
	{
		RegCloseKey( subKey );
	}

	if ( key )
	{
		RegCloseKey( key );
	}
}


mDNSlocal void SetDomainSecret( mDNS * const m, const domainname * inDomain )
{
	char					domainUTF8[ 256 ];
	DomainAuthInfo			*foundInList;
	DomainAuthInfo			*ptr;
	char					outDomain[ 256 ];
	char					outKey[ 256 ];
	char					outSecret[ 256 ];
	OSStatus				err;
	
	ConvertDomainNameToCString( inDomain, domainUTF8 );
	
	// If we're able to find a secret for this domain

	if ( LsaGetSecret( domainUTF8, outDomain, sizeof( outDomain ), outKey, sizeof( outKey ), outSecret, sizeof( outSecret ) ) )
	{
		domainname domain;
		domainname key;

		// Tell the core about this secret

		MakeDomainNameFromDNSNameString( &domain, outDomain );
		MakeDomainNameFromDNSNameString( &key, outKey );

		for (foundInList = m->AuthInfoList; foundInList; foundInList = foundInList->next)
			if (SameDomainName(&foundInList->domain, &domain ) ) break;

		ptr = foundInList;
	
		if (!ptr)
		{
			ptr = (DomainAuthInfo*)malloc(sizeof(DomainAuthInfo));
			require_action( ptr, exit, err = mStatus_NoMemoryErr );
		}

		err = mDNS_SetSecretForDomain(m, ptr, &domain, &key, outSecret, NULL, NULL, FALSE );
		require_action( err != mStatus_BadParamErr, exit, if (!foundInList ) mDNSPlatformMemFree( ptr ) );

		debugf("Setting shared secret for zone %s with key %##s", outDomain, key.c);
	}

exit:

	return;
}


mDNSlocal VOID CALLBACK
CheckFileSharesProc( LPVOID arg, DWORD dwTimerLowValue, DWORD dwTimerHighValue )
{
	mDNS * const m = ( mDNS * const ) arg;

	( void ) dwTimerLowValue;
	( void ) dwTimerHighValue;

	CheckFileShares( m );
}


mDNSlocal unsigned __stdcall 
SMBRegistrationThread( void * arg )
{
	mDNS * const m = ( mDNS * const ) arg;
	DNSServiceRef sref = NULL;
	HANDLE		handles[ 3 ];
	mDNSu8		txtBuf[ 256 ];
	mDNSu8	*	txtPtr;
	size_t		keyLen;
	size_t		valLen;
	mDNSIPPort	port = { { SMBPortAsNumber >> 8, SMBPortAsNumber & 0xFF } };
	DNSServiceErrorType err;

	DEBUG_UNUSED( arg );

	handles[ 0 ] = gSMBThreadStopEvent;
	handles[ 1 ] = gSMBThreadRegisterEvent;
	handles[ 2 ] = gSMBThreadDeregisterEvent;

	memset( txtBuf, 0, sizeof( txtBuf )  );
	txtPtr = txtBuf;
	keyLen = strlen( "netbios=" );
	valLen = strlen( m->p->nbname );
	require_action( valLen < 32, exit, err = kUnknownErr );	// This should never happen, but check to avoid further memory corruption
	*txtPtr++ = ( mDNSu8 ) ( keyLen + valLen );
	memcpy( txtPtr, "netbios=", keyLen );
	txtPtr += keyLen;
	if ( valLen ) { memcpy( txtPtr, m->p->nbname, valLen ); txtPtr += ( mDNSu8 ) valLen; }
	keyLen = strlen( "domain=" );
	valLen = strlen( m->p->nbdomain );
	require_action( valLen < 32, exit, err = kUnknownErr );	// This should never happen, but check to avoid further memory corruption
	*txtPtr++ = ( mDNSu8 )( keyLen + valLen );
	memcpy( txtPtr, "domain=", keyLen );
	txtPtr += keyLen;
	if ( valLen ) { memcpy( txtPtr, m->p->nbdomain, valLen ); txtPtr += valLen; }
	
	for ( ;; )
	{
		DWORD ret;

		ret = WaitForMultipleObjects( 3, handles, FALSE, INFINITE );

		if ( ret != WAIT_FAILED )
		{
			if ( ret == kSMBStopEvent )
			{
				break;
			}
			else if ( ret == kSMBRegisterEvent )
			{
				err = gDNSServiceRegister( &sref, 0, 0, NULL, "_smb._tcp,_file", NULL, NULL, ( uint16_t ) port.NotAnInteger, ( mDNSu16 )( txtPtr - txtBuf ), txtBuf, NULL, NULL );

				if ( err )
				{
					LogMsg( "SMBRegistrationThread: DNSServiceRegister returned %d\n", err );
					sref = NULL;
					break;
				}
			}
			else if ( ret == kSMBDeregisterEvent )
			{
				if ( sref )
				{
					gDNSServiceRefDeallocate( sref );
					sref = NULL;
				}
			}
		}
		else
		{
			LogMsg( "SMBRegistrationThread:  WaitForMultipleObjects returned %d\n", GetLastError() );
			break;
		}
	}

exit:

	if ( sref != NULL )
	{
		gDNSServiceRefDeallocate( sref );
		sref = NULL;
	}

	SetEvent( gSMBThreadQuitEvent );
	_endthreadex( 0 );
	return 0;
}


mDNSlocal void
CheckFileShares( mDNS * const m )
{
	PSHARE_INFO_1	bufPtr = ( PSHARE_INFO_1 ) NULL;
	DWORD			entriesRead = 0;
	DWORD			totalEntries = 0;
	DWORD			resume = 0;
	mDNSBool		advertise = mDNSfalse;
	mDNSBool		fileSharing = mDNSfalse;
	mDNSBool		printSharing = mDNSfalse;
	HKEY			key = NULL;
	BOOL			retry = FALSE;
	NET_API_STATUS  res;
	mStatus			err;

	check( m );

	// Only do this if we're not shutting down

	require_action_quiet( m->AdvertiseLocalAddresses && !m->ShutdownTime, exit, err = kNoErr );

	err = RegCreateKey( HKEY_CURRENT_USER, kServiceParametersNode L"\\Services\\SMB", &key );

	if ( !err )
	{
		DWORD dwSize = sizeof( DWORD );
		RegQueryValueEx( key, L"Advertise", NULL, NULL, (LPBYTE) &advertise, &dwSize );
	}

	if ( advertise && mDNSIsFileAndPrintSharingEnabled( &retry ) )
	{
		dlog( kDebugLevelTrace, DEBUG_NAME "Sharing is enabled\n" );

		res = NetShareEnum( NULL, 1, ( LPBYTE* )&bufPtr, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, &resume );

		if ( ( res == ERROR_SUCCESS ) || ( res == ERROR_MORE_DATA ) )
		{
			PSHARE_INFO_1 p = bufPtr;
			DWORD i;

			for( i = 0; i < entriesRead; i++ ) 
			{
				// We are only interested if the user is sharing anything other 
				// than the built-in "print$" source

				if ( ( p->shi1_type == STYPE_DISKTREE ) && ( wcscmp( p->shi1_netname, TEXT( "print$" ) ) != 0 ) )
				{
					fileSharing = mDNStrue;
				}
				else if ( p->shi1_type == STYPE_PRINTQ )
				{
					printSharing = mDNStrue;
				}

				p++;
			}

			NetApiBufferFree( bufPtr );
			bufPtr = NULL;
			retry = FALSE;
		}
		else if ( res == NERR_ServerNotStarted )
		{
			retry = TRUE;
		}
	}
	
	if ( retry )
	{
		__int64			qwTimeout;
		LARGE_INTEGER   liTimeout;

		qwTimeout = -m->p->checkFileSharesTimeout * 10000000;
		liTimeout.LowPart  = ( DWORD )( qwTimeout & 0xFFFFFFFF );
		liTimeout.HighPart = ( LONG )( qwTimeout >> 32 );

		SetWaitableTimer( m->p->checkFileSharesTimer, &liTimeout, 0, CheckFileSharesProc, m, FALSE );
	}

	if ( !m->p->smbFileSharing && fileSharing )
	{
		if ( !gSMBThread )
		{
			if ( !gDNSSDLibrary )
			{
				gDNSSDLibrary = LoadLibrary( TEXT( "dnssd.dll" ) );
				require_action( gDNSSDLibrary, exit, err = GetLastError() );
			}

			if ( !gDNSServiceRegister )
			{
				gDNSServiceRegister = ( DNSServiceRegisterFunc ) GetProcAddress( gDNSSDLibrary, "DNSServiceRegister" );
				require_action( gDNSServiceRegister, exit, err = GetLastError() );
			}

			if ( !gDNSServiceRefDeallocate )
			{
				gDNSServiceRefDeallocate = ( DNSServiceRefDeallocateFunc ) GetProcAddress( gDNSSDLibrary, "DNSServiceRefDeallocate" );
				require_action( gDNSServiceRefDeallocate, exit, err = GetLastError() );
			}

			if ( !gSMBThreadRegisterEvent )
			{
				gSMBThreadRegisterEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
				require_action( gSMBThreadRegisterEvent != NULL, exit, err = GetLastError() );
			}

			if ( !gSMBThreadDeregisterEvent )
			{
				gSMBThreadDeregisterEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
				require_action( gSMBThreadDeregisterEvent != NULL, exit, err = GetLastError() );
			}

			if ( !gSMBThreadStopEvent )
			{
				gSMBThreadStopEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
				require_action( gSMBThreadStopEvent != NULL, exit, err = GetLastError() );
			}

			if ( !gSMBThreadQuitEvent )
			{
				gSMBThreadQuitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
				require_action( gSMBThreadQuitEvent != NULL, exit, err = GetLastError() );
			}

			gSMBThread = ( HANDLE ) _beginthreadex( NULL, 0, SMBRegistrationThread, m, 0, NULL );
			require_action( gSMBThread != NULL, exit, err = GetLastError() );
		}

		SetEvent( gSMBThreadRegisterEvent );

		m->p->smbFileSharing = mDNStrue;
	}
	else if ( m->p->smbFileSharing && !fileSharing )
	{
		dlog( kDebugLevelTrace, DEBUG_NAME "deregistering smb type\n" );

		if ( gSMBThreadDeregisterEvent != NULL )
		{
			SetEvent( gSMBThreadDeregisterEvent );
		}

		m->p->smbFileSharing = mDNSfalse;
	}

exit:

	if ( key )
	{
		RegCloseKey( key );
	}
}


BOOL
IsWOMPEnabled( mDNS * const m )
{
	BOOL enabled;

	mDNSInterfaceData * ifd;

	enabled = FALSE;

	for( ifd = m->p->interfaceList; ifd; ifd = ifd->next )
	{
		if ( IsWOMPEnabledForAdapter( ifd->name ) )
		{
			enabled = TRUE;
			break;
		}
	}

	return enabled;
}


mDNSlocal mDNSu8
IsWOMPEnabledForAdapter( const char * adapterName )
{
	char						fileName[80];
	NDIS_OID					oid;
    DWORD						count;
    HANDLE						handle	= INVALID_HANDLE_VALUE;
	NDIS_PNP_CAPABILITIES	*	pNPC	= NULL;
	int							err;
	mDNSu8						ok		= TRUE;

	require_action( adapterName != NULL, exit, ok = FALSE );

	dlog( kDebugLevelTrace, DEBUG_NAME "IsWOMPEnabledForAdapter: %s\n", adapterName );
	
    // Construct a device name to pass to CreateFile

	strncpy_s( fileName, sizeof( fileName ), DEVICE_PREFIX, strlen( DEVICE_PREFIX ) );
	strcat_s( fileName, sizeof( fileName ), adapterName );
    handle = CreateFileA( fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, INVALID_HANDLE_VALUE );
	require_action ( handle != INVALID_HANDLE_VALUE, exit, ok = FALSE );

	// We successfully opened the driver, format the IOCTL to pass the driver.
		
	oid = OID_PNP_CAPABILITIES;
	pNPC = ( NDIS_PNP_CAPABILITIES * ) malloc( sizeof( NDIS_PNP_CAPABILITIES ) );
	require_action( pNPC != NULL, exit, ok = FALSE );
	ok = ( mDNSu8 ) DeviceIoControl( handle, IOCTL_NDIS_QUERY_GLOBAL_STATS, &oid, sizeof( oid ), pNPC, sizeof( NDIS_PNP_CAPABILITIES ), &count, NULL );
	err = translate_errno( ok, GetLastError(), kUnknownErr );
	require_action( !err, exit, ok = FALSE );
	ok = ( mDNSu8 ) ( ( count == sizeof( NDIS_PNP_CAPABILITIES ) ) && ( pNPC->Flags & NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE ) );
       
exit:

	if ( pNPC != NULL )
	{
		free( pNPC );
	}

    if ( handle != INVALID_HANDLE_VALUE )
    {
		CloseHandle( handle );
    }

	dlog( kDebugLevelTrace, DEBUG_NAME "IsWOMPEnabledForAdapter returns %s\n", ok ? "true" : "false" );

	return ( mDNSu8 ) ok;
}


mDNSlocal void
SendWakeupPacket( mDNS * const inMDNS, LPSOCKADDR addr, INT addrlen, const char * buf, INT buflen, INT numTries, INT msecSleep )
{
	mDNSBool	repeat = ( numTries == 1 ) ? mDNStrue : mDNSfalse;
	SOCKET		sock;
	int			num;
	mStatus		err;

	( void ) inMDNS;

	sock = socket( addr->sa_family, SOCK_DGRAM, IPPROTO_UDP );
	require_action( sock != INVALID_SOCKET, exit, err = mStatus_UnknownErr );

	while ( numTries-- )
	{
		num = sendto( sock, ( const char* ) buf, buflen, 0, addr, addrlen );

		if ( num != buflen )
		{
			LogMsg( "SendWakeupPacket error: sent %d bytes: %d\n", num, WSAGetLastError() );
		}

		if ( repeat )
		{
			num = sendto( sock, buf, buflen, 0, addr, addrlen );

			if ( num != buflen )
			{
				LogMsg( "SendWakeupPacket error: sent %d bytes: %d\n", num, WSAGetLastError() );
			}
		}

		if ( msecSleep )
		{
			Sleep( msecSleep );
		}
	}

exit:

	if ( sock != INVALID_SOCKET )
	{
		closesocket( sock );
	}
} 


mDNSlocal void _cdecl
SendMulticastWakeupPacket( void *arg )
{
	MulticastWakeupStruct *info = ( MulticastWakeupStruct* ) arg;
	
	if ( info )
	{
		SendWakeupPacket( info->inMDNS, ( LPSOCKADDR ) &info->addr, sizeof( info->addr ), ( const char* ) info->data, sizeof( info->data ), info->numTries, info->msecSleep );
		free( info );
	}

	_endthread();
}


mDNSexport void FreeEtcHosts(mDNS *const m, AuthRecord *const rr, mStatus result)
{
	DEBUG_UNUSED( m );
	DEBUG_UNUSED( rr );
	DEBUG_UNUSED( result );
}
