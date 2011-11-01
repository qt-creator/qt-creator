/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2003-2004, Apple Computer, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*! @header     DNS Service Discovery
 *
 * @discussion  This section describes the functions, callbacks, and data structures
 *              that make up the DNS Service Discovery API.
 *
 *              The DNS Service Discovery API is part of Bonjour, Apple's implementation
 *              of zero-configuration networking (ZEROCONF).
 *
 *              Bonjour allows you to register a network service, such as a
 *              printer or file server, so that it can be found by name or browsed
 *              for by service type and domain. Using Bonjour, applications can
 *              discover what services are available on the network, along with
 *              all the information -- such as name, IP address, and port --
 *              necessary to access a particular service.
 *
 *              In effect, Bonjour combines the functions of a local DNS server and
 *              AppleTalk. Bonjour allows applications to provide user-friendly printer
 *              and server browsing, among other things, over standard IP networks.
 *              This behavior is a result of combining protocols such as multicast and
 *              DNS to add new functionality to the network (such as multicast DNS).
 *
 *              Bonjour gives applications easy access to services over local IP
 *              networks without requiring the service or the application to support
 *              an AppleTalk or a Netbeui stack, and without requiring a DNS server
 *              for the local network.
 */


/* _DNS_SD_H contains the mDNSResponder version number for this header file, formatted as follows:
 *   Major part of the build number * 10000 +
 *   minor part of the build number *   100
 * For example, Mac OS X 10.4.9 has mDNSResponder-108.4, which would be represented as
 * version 1080400. This allows C code to do simple greater-than and less-than comparisons:
 * e.g. an application that requires the DNSServiceGetProperty() call (new in mDNSResponder-126) can check:
 *
 *   #if _DNS_SD_H+0 >= 1260000
 *   ... some C code that calls DNSServiceGetProperty() ...
 *   #endif
 *
 * The version defined in this header file symbol allows for compile-time
 * checking, so that C code building with earlier versions of the header file
 * can avoid compile errors trying to use functions that aren't even defined
 * in those earlier versions. Similar checks may also be performed at run-time:
 *  => weak linking -- to avoid link failures if run with an earlier
 *     version of the library that's missing some desired symbol, or
 *  => DNSServiceGetProperty(DaemonVersion) -- to verify whether the running daemon
 *     ("system service" on Windows) meets some required minimum functionality level.
 */

#ifndef _DNS_SD_TYPES_H
#define _DNS_SD_TYPES_H 3200500

#ifdef  __cplusplus
    extern "C" {
#endif

/* Set to 1 if libdispatch is supported
 * Note: May also be set by project and/or Makefile
 */
#ifndef _DNS_SD_LIBDISPATCH
#define _DNS_SD_LIBDISPATCH 0
#endif /* ndef _DNS_SD_LIBDISPATCH */

/* standard calling convention under Win32 is __stdcall */
/* Note: When compiling Intel EFI (Extensible Firmware Interface) under MS Visual Studio, the */
/* _WIN32 symbol is defined by the compiler even though it's NOT compiling code for Windows32 */
#if defined(_WIN32) && !defined(EFI32) && !defined(EFI64)
#define DNSSD_API __stdcall
#else
#define DNSSD_API
#endif

/* stdint.h does not exist on FreeBSD 4.x; its types are defined in sys/types.h instead */
#if defined(__FreeBSD__) && (__FreeBSD__ < 5)
#include <sys/types.h>

/* Likewise, on Sun, standard integer types are in sys/types.h */
#elif defined(__sun__)
#include <sys/types.h>

/* EFI does not have stdint.h, or anything else equivalent */
#elif defined(EFI32) || defined(EFI64) || defined(EFIX64)
#include "Tiano.h"
#if !defined(_STDINT_H_)
typedef UINT8       uint8_t;
typedef INT8        int8_t;
typedef UINT16      uint16_t;
typedef INT16       int16_t;
typedef UINT32      uint32_t;
typedef INT32       int32_t;
#endif
/* Windows has its own differences */
#elif defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#define _UNUSED
#ifndef _MSL_STDINT_H
typedef UINT8       uint8_t;
typedef INT8        int8_t;
typedef UINT16      uint16_t;
typedef INT16       int16_t;
typedef UINT32      uint32_t;
typedef INT32       int32_t;
#endif

/* All other Posix platforms use stdint.h */
#else
#include <stdint.h>
#endif

#if _DNS_SD_LIBDISPATCH
#include <dispatch/dispatch.h>
#endif

/* DNSServiceRef, DNSRecordRef
 *
 * Opaque internal data types.
 * Note: client is responsible for serializing access to these structures if
 * they are shared between concurrent threads.
 */

typedef struct _DNSServiceRef_t *DNSServiceRef;
typedef struct _DNSRecordRef_t *DNSRecordRef;

struct sockaddr;

/*! @enum General flags
 * Most DNS-SD API functions and callbacks include a DNSServiceFlags parameter.
 * As a general rule, any given bit in the 32-bit flags field has a specific fixed meaning,
 * regardless of the function or callback being used. For any given function or callback,
 * typically only a subset of the possible flags are meaningful, and all others should be zero.
 * The discussion section for each API call describes which flags are valid for that call
 * and callback. In some cases, for a particular call, it may be that no flags are currently
 * defined, in which case the DNSServiceFlags parameter exists purely to allow future expansion.
 * In all cases, developers should expect that in future releases, it is possible that new flag
 * values will be defined, and write code with this in mind. For example, code that tests
 *     if (flags == kDNSServiceFlagsAdd) ...
 * will fail if, in a future release, another bit in the 32-bit flags field is also set.
 * The reliable way to test whether a particular bit is set is not with an equality test,
 * but with a bitwise mask:
 *     if (flags & kDNSServiceFlagsAdd) ...
 */
enum
    {
    kDNSServiceFlagsMoreComing          = 0x1,
    /* MoreComing indicates to a callback that at least one more result is
     * queued and will be delivered following immediately after this one.
     * When the MoreComing flag is set, applications should not immediately
     * update their UI, because this can result in a great deal of ugly flickering
     * on the screen, and can waste a great deal of CPU time repeatedly updating
     * the screen with content that is then immediately erased, over and over.
     * Applications should wait until until MoreComing is not set, and then
     * update their UI when no more changes are imminent.
     * When MoreComing is not set, that doesn't mean there will be no more
     * answers EVER, just that there are no more answers immediately
     * available right now at this instant. If more answers become available
     * in the future they will be delivered as usual.
     */

    kDNSServiceFlagsAdd                 = 0x2,
    kDNSServiceFlagsDefault             = 0x4,
    /* Flags for domain enumeration and browse/query reply callbacks.
     * "Default" applies only to enumeration and is only valid in
     * conjunction with "Add". An enumeration callback with the "Add"
     * flag NOT set indicates a "Remove", i.e. the domain is no longer
     * valid.
     */

    kDNSServiceFlagsNoAutoRename        = 0x8,
    /* Flag for specifying renaming behavior on name conflict when registering
     * non-shared records. By default, name conflicts are automatically handled
     * by renaming the service. NoAutoRename overrides this behavior - with this
     * flag set, name conflicts will result in a callback. The NoAutorename flag
     * is only valid if a name is explicitly specified when registering a service
     * (i.e. the default name is not used.)
     */

    kDNSServiceFlagsShared              = 0x10,
    kDNSServiceFlagsUnique              = 0x20,
    /* Flag for registering individual records on a connected
     * DNSServiceRef. Shared indicates that there may be multiple records
     * with this name on the network (e.g. PTR records). Unique indicates that the
     * record's name is to be unique on the network (e.g. SRV records).
     */

    kDNSServiceFlagsBrowseDomains       = 0x40,
    kDNSServiceFlagsRegistrationDomains = 0x80,
    /* Flags for specifying domain enumeration type in DNSServiceEnumerateDomains.
     * BrowseDomains enumerates domains recommended for browsing, RegistrationDomains
     * enumerates domains recommended for registration.
     */

    kDNSServiceFlagsLongLivedQuery      = 0x100,
    /* Flag for creating a long-lived unicast query for the DNSServiceQueryRecord call. */

    kDNSServiceFlagsAllowRemoteQuery    = 0x200,
    /* Flag for creating a record for which we will answer remote queries
     * (queries from hosts more than one hop away; hosts not directly connected to the local link).
     */

    kDNSServiceFlagsForceMulticast      = 0x400,
    /* Flag for signifying that a query or registration should be performed exclusively via multicast
     * DNS, even for a name in a domain (e.g. foo.apple.com.) that would normally imply unicast DNS.
     */

    kDNSServiceFlagsForce               = 0x800,
    /* Flag for signifying a "stronger" variant of an operation.
     * Currently defined only for DNSServiceReconfirmRecord(), where it forces a record to
     * be removed from the cache immediately, instead of querying for a few seconds before
     * concluding that the record is no longer valid and then removing it. This flag should
     * be used with caution because if a service browsing PTR record is indeed still valid
     * on the network, forcing its removal will result in a user-interface flap -- the
     * discovered service instance will disappear, and then re-appear moments later.
     */

    kDNSServiceFlagsReturnIntermediates = 0x1000,
    /* Flag for returning intermediate results.
     * For example, if a query results in an authoritative NXDomain (name does not exist)
     * then that result is returned to the client. However the query is not implicitly
     * cancelled -- it remains active and if the answer subsequently changes
     * (e.g. because a VPN tunnel is subsequently established) then that positive
     * result will still be returned to the client.
     * Similarly, if a query results in a CNAME record, then in addition to following
     * the CNAME referral, the intermediate CNAME result is also returned to the client.
     * When this flag is not set, NXDomain errors are not returned, and CNAME records
     * are followed silently without informing the client of the intermediate steps.
     * (In earlier builds this flag was briefly calledkDNSServiceFlagsReturnCNAME)
     */

    kDNSServiceFlagsNonBrowsable        = 0x2000,
    /* A service registered with the NonBrowsable flag set can be resolved using
     * DNSServiceResolve(), but will not be discoverable using DNSServiceBrowse().
     * This is for cases where the name is actually a GUID; it is found by other means;
     * there is no end-user benefit to browsing to find a long list of opaque GUIDs.
     * Using the NonBrowsable flag creates SRV+TXT without the cost of also advertising
     * an associated PTR record.
     */

    kDNSServiceFlagsShareConnection     = 0x4000,
    /* For efficiency, clients that perform many concurrent operations may want to use a
     * single Unix Domain Socket connection with the background daemon, instead of having a
     * separate connection for each independent operation. To use this mode, clients first
     * call DNSServiceCreateConnection(&MainRef) to initialize the main DNSServiceRef.
     * For each subsequent operation that is to share that same connection, the client copies
     * the MainRef, and then passes the address of that copy, setting the ShareConnection flag
     * to tell the library that this DNSServiceRef is not a typical uninitialized DNSServiceRef;
     * it's a copy of an existing DNSServiceRef whose connection information should be reused.
     *
     * For example:
     *
     * DNSServiceErrorType error;
     * DNSServiceRef MainRef;
     * error = DNSServiceCreateConnection(&MainRef);
     * if (error) ...
     * DNSServiceRef BrowseRef = MainRef;  // Important: COPY the primary DNSServiceRef first...
     * error = DNSServiceBrowse(&BrowseRef, kDNSServiceFlagsShareConnection, ...); // then use the copy
     * if (error) ...
     * ...
     * DNSServiceRefDeallocate(BrowseRef); // Terminate the browse operation
     * DNSServiceRefDeallocate(MainRef);   // Terminate the shared connection
     *
     * Notes:
     *
     * 1. Collective kDNSServiceFlagsMoreComing flag
     * When callbacks are invoked using a shared DNSServiceRef, the
     * kDNSServiceFlagsMoreComing flag applies collectively to *all* active
     * operations sharing the same parent DNSServiceRef. If the MoreComing flag is
     * set it means that there are more results queued on this parent DNSServiceRef,
     * but not necessarily more results for this particular callback function.
     * The implication of this for client programmers is that when a callback
     * is invoked with the MoreComing flag set, the code should update its
     * internal data structures with the new result, and set a variable indicating
     * that its UI needs to be updated. Then, later when a callback is eventually
     * invoked with the MoreComing flag not set, the code should update *all*
     * stale UI elements related to that shared parent DNSServiceRef that need
     * updating, not just the UI elements related to the particular callback
     * that happened to be the last one to be invoked.
     *
     * 2. Canceling operations and kDNSServiceFlagsMoreComing
     * Whenever you cancel any operation for which you had deferred UI updates
     * waiting because of a kDNSServiceFlagsMoreComing flag, you should perform
     * those deferred UI updates. This is because, after cancelling the operation,
     * you can no longer wait for a callback *without* MoreComing set, to tell
     * you do perform your deferred UI updates (the operation has been canceled,
     * so there will be no more callbacks). An implication of the collective
     * kDNSServiceFlagsMoreComing flag for shared connections is that this
     * guideline applies more broadly -- any time you cancel an operation on
     * a shared connection, you should perform all deferred UI updates for all
     * operations sharing that connection. This is because the MoreComing flag
     * might have been referring to events coming for the operation you canceled,
     * which will now not be coming because the operation has been canceled.
     *
     * 3. Only share DNSServiceRef's created with DNSServiceCreateConnection
     * Calling DNSServiceCreateConnection(&ref) creates a special shareable DNSServiceRef.
     * DNSServiceRef's created by other calls like DNSServiceBrowse() or DNSServiceResolve()
     * cannot be shared by copying them and using kDNSServiceFlagsShareConnection.
     *
     * 4. Don't Double-Deallocate
     * Calling DNSServiceRefDeallocate(ref) for a particular operation's DNSServiceRef terminates
     * just that operation. Calling DNSServiceRefDeallocate(ref) for the main shared DNSServiceRef
     * (the parent DNSServiceRef, originally created by DNSServiceCreateConnection(&ref))
     * automatically terminates the shared connection and all operations that were still using it.
     * After doing this, DO NOT then attempt to deallocate any remaining subordinate DNSServiceRef's.
     * The memory used by those subordinate DNSServiceRef's has already been freed, so any attempt
     * to do a DNSServiceRefDeallocate (or any other operation) on them will result in accesses
     * to freed memory, leading to crashes or other equally undesirable results.
     *
     * 5. Thread Safety
     * The dns_sd.h API does not presuppose any particular threading model, and consequently
     * does no locking of its own (which would require linking some specific threading library).
     * If client code calls API routines on the same DNSServiceRef concurrently
     * from multiple threads, it is the client's responsibility to use a mutext
     * lock or take similar appropriate precautions to serialize those calls.
     */

    kDNSServiceFlagsSuppressUnusable    = 0x8000,
        /*
         * This flag is meaningful only in DNSServiceQueryRecord which suppresses unusable queries on the
         * wire. If "hostname" is a wide-area unicast DNS hostname (i.e. not a ".local." name)
         * but this host has no routable IPv6 address, then the call will not try to look up IPv6 addresses
         * for "hostname", since any addresses it found would be unlikely to be of any use anyway. Similarly,
         * if this host has no routable IPv4 address, the call will not try to look up IPv4 addresses for
         * "hostname".
         */

    kDNSServiceFlagsTimeout            = 0x10000,
        /*
         * When kDNServiceFlagsTimeout is passed to DNSServiceQueryRecord or DNSServiceGetAddrInfo, the query is
         * stopped after a certain number of seconds have elapsed. The time at which the query will be stopped
         * is determined by the system and cannot be configured by the user. The query will be stopped irrespective
         * of whether a response was given earlier or not. When the query is stopped, the callback will be called
         * with an error code of kDNSServiceErr_Timeout and a NULL sockaddr will be returned for DNSServiceGetAddrInfo
         * and zero length rdata will be returned for DNSServiceQueryRecord.
         */

    kDNSServiceFlagsIncludeP2P          = 0x20000,
        /*
         * Include P2P interfaces when kDNSServiceInterfaceIndexAny is specified.
         * By default, specifying kDNSServiceInterfaceIndexAny does not include P2P interfaces.
         */
        kDNSServiceFlagsWakeOnResolve      = 0x40000
        /*
         * This flag is meaningful only in DNSServiceResolve. When set, it tries to send a magic packet
         * to wake up the client.
         */
    };

/* Possible protocols for DNSServiceNATPortMappingCreate(). */
enum
    {
    kDNSServiceProtocol_IPv4 = 0x01,
    kDNSServiceProtocol_IPv6 = 0x02,
    /* 0x04 and 0x08 reserved for future internetwork protocols */

    kDNSServiceProtocol_UDP  = 0x10,
    kDNSServiceProtocol_TCP  = 0x20
    /* 0x40 and 0x80 reserved for future transport protocols, e.g. SCTP [RFC 2960]
     * or DCCP [RFC 4340]. If future NAT gateways are created that support port
     * mappings for these protocols, new constants will be defined here.
     */
    };

/*
 * The values for DNS Classes and Types are listed in RFC 1035, and are available
 * on every OS in its DNS header file. Unfortunately every OS does not have the
 * same header file containing DNS Class and Type constants, and the names of
 * the constants are not consistent. For example, BIND 8 uses "T_A",
 * BIND 9 uses "ns_t_a", Windows uses "DNS_TYPE_A", etc.
 * For this reason, these constants are also listed here, so that code using
 * the DNS-SD programming APIs can use these constants, so that the same code
 * can compile on all our supported platforms.
 */

enum
    {
    kDNSServiceClass_IN       = 1       /* Internet */
    };

enum
    {
    kDNSServiceType_A          = 1,      /* Host address. */
    kDNSServiceType_NS         = 2,      /* Authoritative server. */
    kDNSServiceType_MD         = 3,      /* Mail destination. */
    kDNSServiceType_MF         = 4,      /* Mail forwarder. */
    kDNSServiceType_CNAME      = 5,      /* Canonical name. */
    kDNSServiceType_SOA        = 6,      /* Start of authority zone. */
    kDNSServiceType_MB         = 7,      /* Mailbox domain name. */
    kDNSServiceType_MG         = 8,      /* Mail group member. */
    kDNSServiceType_MR         = 9,      /* Mail rename name. */
    kDNSServiceType_NULL       = 10,     /* Null resource record. */
    kDNSServiceType_WKS        = 11,     /* Well known service. */
    kDNSServiceType_PTR        = 12,     /* Domain name pointer. */
    kDNSServiceType_HINFO      = 13,     /* Host information. */
    kDNSServiceType_MINFO      = 14,     /* Mailbox information. */
    kDNSServiceType_MX         = 15,     /* Mail routing information. */
    kDNSServiceType_TXT        = 16,     /* One or more text strings (NOT "zero or more..."). */
    kDNSServiceType_RP         = 17,     /* Responsible person. */
    kDNSServiceType_AFSDB      = 18,     /* AFS cell database. */
    kDNSServiceType_X25        = 19,     /* X_25 calling address. */
    kDNSServiceType_ISDN       = 20,     /* ISDN calling address. */
    kDNSServiceType_RT         = 21,     /* Router. */
    kDNSServiceType_NSAP       = 22,     /* NSAP address. */
    kDNSServiceType_NSAP_PTR   = 23,     /* Reverse NSAP lookup (deprecated). */
    kDNSServiceType_SIG        = 24,     /* Security signature. */
    kDNSServiceType_KEY        = 25,     /* Security key. */
    kDNSServiceType_PX         = 26,     /* X.400 mail mapping. */
    kDNSServiceType_GPOS       = 27,     /* Geographical position (withdrawn). */
    kDNSServiceType_AAAA       = 28,     /* IPv6 Address. */
    kDNSServiceType_LOC        = 29,     /* Location Information. */
    kDNSServiceType_NXT        = 30,     /* Next domain (security). */
    kDNSServiceType_EID        = 31,     /* Endpoint identifier. */
    kDNSServiceType_NIMLOC     = 32,     /* Nimrod Locator. */
    kDNSServiceType_SRV        = 33,     /* Server Selection. */
    kDNSServiceType_ATMA       = 34,     /* ATM Address */
    kDNSServiceType_NAPTR      = 35,     /* Naming Authority PoinTeR */
    kDNSServiceType_KX         = 36,     /* Key Exchange */
    kDNSServiceType_CERT       = 37,     /* Certification record */
    kDNSServiceType_A6         = 38,     /* IPv6 Address (deprecated) */
    kDNSServiceType_DNAME      = 39,     /* Non-terminal DNAME (for IPv6) */
    kDNSServiceType_SINK       = 40,     /* Kitchen sink (experimental) */
    kDNSServiceType_OPT        = 41,     /* EDNS0 option (meta-RR) */
    kDNSServiceType_APL        = 42,     /* Address Prefix List */
    kDNSServiceType_DS         = 43,     /* Delegation Signer */
    kDNSServiceType_SSHFP      = 44,     /* SSH Key Fingerprint */
    kDNSServiceType_IPSECKEY   = 45,     /* IPSECKEY */
    kDNSServiceType_RRSIG      = 46,     /* RRSIG */
    kDNSServiceType_NSEC       = 47,     /* Denial of Existence */
    kDNSServiceType_DNSKEY     = 48,     /* DNSKEY */
    kDNSServiceType_DHCID      = 49,     /* DHCP Client Identifier */
    kDNSServiceType_NSEC3      = 50,     /* Hashed Authenticated Denial of Existence */
    kDNSServiceType_NSEC3PARAM = 51,     /* Hashed Authenticated Denial of Existence */

    kDNSServiceType_HIP        = 55,     /* Host Identity Protocol */

    kDNSServiceType_SPF        = 99,     /* Sender Policy Framework for E-Mail */
    kDNSServiceType_UINFO      = 100,    /* IANA-Reserved */
    kDNSServiceType_UID        = 101,    /* IANA-Reserved */
    kDNSServiceType_GID        = 102,    /* IANA-Reserved */
    kDNSServiceType_UNSPEC     = 103,    /* IANA-Reserved */

    kDNSServiceType_TKEY       = 249,    /* Transaction key */
    kDNSServiceType_TSIG       = 250,    /* Transaction signature. */
    kDNSServiceType_IXFR       = 251,    /* Incremental zone transfer. */
    kDNSServiceType_AXFR       = 252,    /* Transfer zone of authority. */
    kDNSServiceType_MAILB      = 253,    /* Transfer mailbox records. */
    kDNSServiceType_MAILA      = 254,    /* Transfer mail agent records. */
    kDNSServiceType_ANY        = 255     /* Wildcard match. */
    };

/* possible error code values */
enum
    {
    kDNSServiceErr_NoError                   = 0,
    kDNSServiceErr_Unknown                   = -65537,  /* 0xFFFE FFFF */
    kDNSServiceErr_NoSuchName                = -65538,
    kDNSServiceErr_NoMemory                  = -65539,
    kDNSServiceErr_BadParam                  = -65540,
    kDNSServiceErr_BadReference              = -65541,
    kDNSServiceErr_BadState                  = -65542,
    kDNSServiceErr_BadFlags                  = -65543,
    kDNSServiceErr_Unsupported               = -65544,
    kDNSServiceErr_NotInitialized            = -65545,
    kDNSServiceErr_AlreadyRegistered         = -65547,
    kDNSServiceErr_NameConflict              = -65548,
    kDNSServiceErr_Invalid                   = -65549,
    kDNSServiceErr_Firewall                  = -65550,
    kDNSServiceErr_Incompatible              = -65551,  /* client library incompatible with daemon */
    kDNSServiceErr_BadInterfaceIndex         = -65552,
    kDNSServiceErr_Refused                   = -65553,
    kDNSServiceErr_NoSuchRecord              = -65554,
    kDNSServiceErr_NoAuth                    = -65555,
    kDNSServiceErr_NoSuchKey                 = -65556,
    kDNSServiceErr_NATTraversal              = -65557,
    kDNSServiceErr_DoubleNAT                 = -65558,
    kDNSServiceErr_BadTime                   = -65559,  /* Codes up to here existed in Tiger */
    kDNSServiceErr_BadSig                    = -65560,
    kDNSServiceErr_BadKey                    = -65561,
    kDNSServiceErr_Transient                 = -65562,
    kDNSServiceErr_ServiceNotRunning         = -65563,  /* Background daemon not running */
    kDNSServiceErr_NATPortMappingUnsupported = -65564,  /* NAT doesn't support NAT-PMP or UPnP */
    kDNSServiceErr_NATPortMappingDisabled    = -65565,  /* NAT supports NAT-PMP or UPnP but it's disabled by the administrator */
    kDNSServiceErr_NoRouter                  = -65566,  /* No router currently configured (probably no network connectivity) */
    kDNSServiceErr_PollingMode               = -65567,
    kDNSServiceErr_Timeout                   = -65568

    /* mDNS Error codes are in the range
     * FFFE FF00 (-65792) to FFFE FFFF (-65537) */
    };

/* Maximum length, in bytes, of a service name represented as a */
/* literal C-String, including the terminating NULL at the end. */

#define kDNSServiceMaxServiceName 64

/* Maximum length, in bytes, of a domain name represented as an *escaped* C-String */
/* including the final trailing dot, and the C-String terminating NULL at the end. */

#define kDNSServiceMaxDomainName 1009

/*
 * Notes on DNS Name Escaping
 *   -- or --
 * "Why is kDNSServiceMaxDomainName 1009, when the maximum legal domain name is 256 bytes?"
 *
 * All strings used in the DNS-SD APIs are UTF-8 strings. Apart from the exceptions noted below,
 * the APIs expect the strings to be properly escaped, using the conventional DNS escaping rules:
 *
 *   '\\' represents a single literal '\' in the name
 *   '\.' represents a single literal '.' in the name
 *   '\ddd', where ddd is a three-digit decimal value from 000 to 255,
 *        represents a single literal byte with that value.
 *   A bare unescaped '.' is a label separator, marking a boundary between domain and subdomain.
 *
 * The exceptions, that do not use escaping, are the routines where the full
 * DNS name of a resource is broken, for convenience, into servicename/regtype/domain.
 * In these routines, the "servicename" is NOT escaped. It does not need to be, since
 * it is, by definition, just a single literal string. Any characters in that string
 * represent exactly what they are. The "regtype" portion is, technically speaking,
 * escaped, but since legal regtypes are only allowed to contain letters, digits,
 * and hyphens, there is nothing to escape, so the issue is moot. The "domain"
 * portion is also escaped, though most domains in use on the public Internet
 * today, like regtypes, don't contain any characters that need to be escaped.
 * As DNS-SD becomes more popular, rich-text domains for service discovery will
 * become common, so software should be written to cope with domains with escaping.
 *
 * The servicename may be up to 63 bytes of UTF-8 text (not counting the C-String
 * terminating NULL at the end). The regtype is of the form _service._tcp or
 * _service._udp, where the "service" part is 1-15 characters, which may be
 * letters, digits, or hyphens. The domain part of the three-part name may be
 * any legal domain, providing that the resulting servicename+regtype+domain
 * name does not exceed 256 bytes.
 *
 * For most software, these issues are transparent. When browsing, the discovered
 * servicenames should simply be displayed as-is. When resolving, the discovered
 * servicename/regtype/domain are simply passed unchanged to DNSServiceResolve().
 * When a DNSServiceResolve() succeeds, the returned fullname is already in
 * the correct format to pass to standard system DNS APIs such as res_query().
 * For converting from servicename/regtype/domain to a single properly-escaped
 * full DNS name, the helper function DNSServiceConstructFullName() is provided.
 *
 * The following (highly contrived) example illustrates the escaping process.
 * Suppose you have an service called "Dr. Smith\Dr. Johnson", of type "_ftp._tcp"
 * in subdomain "4th. Floor" of subdomain "Building 2" of domain "apple.com."
 * The full (escaped) DNS name of this service's SRV record would be:
 * Dr\.\032Smith\\Dr\.\032Johnson._ftp._tcp.4th\.\032Floor.Building\0322.apple.com.
 */


/*
 * Constants for specifying an interface index
 *
 * Specific interface indexes are identified via a 32-bit unsigned integer returned
 * by the if_nametoindex() family of calls.
 *
 * If the client passes 0 for interface index, that means "do the right thing",
 * which (at present) means, "if the name is in an mDNS local multicast domain
 * (e.g. 'local.', '254.169.in-addr.arpa.', '{8,9,A,B}.E.F.ip6.arpa.') then multicast
 * on all applicable interfaces, otherwise send via unicast to the appropriate
 * DNS server." Normally, most clients will use 0 for interface index to
 * automatically get the default sensible behaviour.
 *
 * If the client passes a positive interface index, then for multicast names that
 * indicates to do the operation only on that one interface. For unicast names the
 * interface index is ignored unless kDNSServiceFlagsForceMulticast is also set.
 *
 * If the client passes kDNSServiceInterfaceIndexLocalOnly when registering
 * a service, then that service will be found *only* by other local clients
 * on the same machine that are browsing using kDNSServiceInterfaceIndexLocalOnly
 * or kDNSServiceInterfaceIndexAny.
 * If a client has a 'private' service, accessible only to other processes
 * running on the same machine, this allows the client to advertise that service
 * in a way such that it does not inadvertently appear in service lists on
 * all the other machines on the network.
 *
 * If the client passes kDNSServiceInterfaceIndexLocalOnly when browsing
 * then it will find *all* records registered on that same local machine.
 * Clients explicitly wishing to discover *only* LocalOnly services can
 * accomplish this by inspecting the interfaceIndex of each service reported
 * to their DNSServiceBrowseReply() callback function, and discarding those
 * where the interface index is not kDNSServiceInterfaceIndexLocalOnly.
 *
 * kDNSServiceInterfaceIndexP2P is meaningful only in Browse, QueryRecord,
 * and Resolve operations. It should not be used in other DNSService APIs.
 *
 * - If kDNSServiceInterfaceIndexP2P is passed to DNSServiceBrowse or
 *   DNSServiceQueryRecord, it restricts the operation to P2P.
 *
 * - If kDNSServiceInterfaceIndexP2P is passed to DNSServiceResolve, it is
 *   mapped internally to kDNSServiceInterfaceIndexAny, because resolving
 *   a P2P service may create and/or enable an interface whose index is not
 *   known a priori. The resolve callback will indicate the index of the
 *   interface via which the service can be accessed.
 *
 * If applications pass kDNSServiceInterfaceIndexAny to DNSServiceBrowse
 * or DNSServiceQueryRecord, they must set the kDNSServiceFlagsIncludeP2P flag
 * to include P2P. In this case, if a service instance or the record being queried
 * is found over P2P, the resulting ADD event will indicate kDNSServiceInterfaceIndexP2P
 * as the interface index.
 */

#define kDNSServiceInterfaceIndexAny 0
#define kDNSServiceInterfaceIndexLocalOnly ((uint32_t)-1)
#define kDNSServiceInterfaceIndexUnicast   ((uint32_t)-2)
#define kDNSServiceInterfaceIndexP2P       ((uint32_t)-3)

typedef uint32_t DNSServiceFlags;
typedef uint32_t DNSServiceProtocol;
typedef int32_t  DNSServiceErrorType;

/*
 * When requesting kDNSServiceProperty_DaemonVersion, the result pointer must point
 * to a 32-bit unsigned integer, and the size parameter must be set to sizeof(uint32_t).
 *
 * On return, the 32-bit unsigned integer contains the version number, formatted as follows:
 *   Major part of the build number * 10000 +
 *   minor part of the build number *   100
 *
 * For example, Mac OS X 10.4.9 has mDNSResponder-108.4, which would be represented as
 * version 1080400. This allows applications to do simple greater-than and less-than comparisons:
 * e.g. an application that requires at least mDNSResponder-108.4 can check:
 *
 *   if (version >= 1080400) ...
 *
 * Example usage:
 *
 * uint32_t version;
 * uint32_t size = sizeof(version);
 * DNSServiceErrorType err = DNSServiceGetProperty(kDNSServiceProperty_DaemonVersion, &version, &size);
 * if (!err) printf("Bonjour version is %d.%d\n", version / 10000, version / 100 % 100);
 */

#define kDNSServiceProperty_DaemonVersion "DaemonVersion"


/* DNSServiceEnumerateDomains()
 *
 * Asynchronously enumerate domains available for browsing and registration.
 *
 * The enumeration MUST be cancelled via DNSServiceRefDeallocate() when no more domains
 * are to be found.
 *
 * Note that the names returned are (like all of DNS-SD) UTF-8 strings,
 * and are escaped using standard DNS escaping rules.
 * (See "Notes on DNS Name Escaping" earlier in this file for more details.)
 * A graphical browser displaying a hierarchical tree-structured view should cut
 * the names at the bare dots to yield individual labels, then de-escape each
 * label according to the escaping rules, and then display the resulting UTF-8 text.
 *
 * DNSServiceDomainEnumReply Callback Parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceEnumerateDomains().
 *
 * flags:           Possible values are:
 *                  kDNSServiceFlagsMoreComing
 *                  kDNSServiceFlagsAdd
 *                  kDNSServiceFlagsDefault
 *
 * interfaceIndex:  Specifies the interface on which the domain exists. (The index for a given
 *                  interface is determined via the if_nametoindex() family of calls.)
 *
 * errorCode:       Will be kDNSServiceErr_NoError (0) on success, otherwise indicates
 *                  the failure that occurred (other parameters are undefined if errorCode is nonzero).
 *
 * replyDomain:     The name of the domain.
 *
 * context:         The context pointer passed to DNSServiceEnumerateDomains.
 *
 */

typedef void (DNSSD_API *DNSServiceDomainEnumReply)
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *replyDomain,
    void                                *context
    );


/* Register a service that is discovered via Browse() and Resolve() calls.
 *
 * DNSServiceRegisterReply() Callback Parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceRegister().
 *
 * flags:           When a name is successfully registered, the callback will be
 *                  invoked with the kDNSServiceFlagsAdd flag set. When Wide-Area
 *                  DNS-SD is in use, it is possible for a single service to get
 *                  more than one success callback (e.g. one in the "local" multicast
 *                  DNS domain, and another in a wide-area unicast DNS domain).
 *                  If a successfully-registered name later suffers a name conflict
 *                  or similar problem and has to be deregistered, the callback will
 *                  be invoked with the kDNSServiceFlagsAdd flag not set. The callback
 *                  is *not* invoked in the case where the caller explicitly terminates
 *                  the service registration by calling DNSServiceRefDeallocate(ref);
 *
 * errorCode:       Will be kDNSServiceErr_NoError on success, otherwise will
 *                  indicate the failure that occurred (including name conflicts,
 *                  if the kDNSServiceFlagsNoAutoRename flag was used when registering.)
 *                  Other parameters are undefined if errorCode is nonzero.
 *
 * name:            The service name registered (if the application did not specify a name in
 *                  DNSServiceRegister(), this indicates what name was automatically chosen).
 *
 * regtype:         The type of service registered, as it was passed to the callout.
 *
 * domain:          The domain on which the service was registered (if the application did not
 *                  specify a domain in DNSServiceRegister(), this indicates the default domain
 *                  on which the service was registered).
 *
 * context:         The context pointer that was passed to the callout.
 *
 */

typedef void (DNSSD_API *DNSServiceRegisterReply)
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    void                                *context
    );


/* Browse for instances of a service.
 *
 * DNSServiceBrowseReply() Parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceBrowse().
 *
 * flags:           Possible values are kDNSServiceFlagsMoreComing and kDNSServiceFlagsAdd.
 *                  See flag definitions for details.
 *
 * interfaceIndex:  The interface on which the service is advertised. This index should
 *                  be passed to DNSServiceResolve() when resolving the service.
 *
 * errorCode:       Will be kDNSServiceErr_NoError (0) on success, otherwise will
 *                  indicate the failure that occurred. Other parameters are undefined if
 *                  the errorCode is nonzero.
 *
 * serviceName:     The discovered service name. This name should be displayed to the user,
 *                  and stored for subsequent use in the DNSServiceResolve() call.
 *
 * regtype:         The service type, which is usually (but not always) the same as was passed
 *                  to DNSServiceBrowse(). One case where the discovered service type may
 *                  not be the same as the requested service type is when using subtypes:
 *                  The client may want to browse for only those ftp servers that allow
 *                  anonymous connections. The client will pass the string "_ftp._tcp,_anon"
 *                  to DNSServiceBrowse(), but the type of the service that's discovered
 *                  is simply "_ftp._tcp". The regtype for each discovered service instance
 *                  should be stored along with the name, so that it can be passed to
 *                  DNSServiceResolve() when the service is later resolved.
 *
 * domain:          The domain of the discovered service instance. This may or may not be the
 *                  same as the domain that was passed to DNSServiceBrowse(). The domain for each
 *                  discovered service instance should be stored along with the name, so that
 *                  it can be passed to DNSServiceResolve() when the service is later resolved.
 *
 * context:         The context pointer that was passed to the callout.
 *
 */

typedef void (DNSSD_API *DNSServiceBrowseReply)
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *serviceName,
    const char                          *regtype,
    const char                          *replyDomain,
    void                                *context
    );



/* DNSServiceResolve()
 *
 * Resolve a service name discovered via DNSServiceBrowse() to a target host name, port number, and
 * txt record.
 *
 * Note: Applications should NOT use DNSServiceResolve() solely for txt record monitoring - use
 * DNSServiceQueryRecord() instead, as it is more efficient for this task.
 *
 * Note: When the desired results have been returned, the client MUST terminate the resolve by calling
 * DNSServiceRefDeallocate().
 *
 * Note: DNSServiceResolve() behaves correctly for typical services that have a single SRV record
 * and a single TXT record. To resolve non-standard services with multiple SRV or TXT records,
 * DNSServiceQueryRecord() should be used.
 *
 * DNSServiceResolveReply Callback Parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceResolve().
 *
 * flags:           Possible values: kDNSServiceFlagsMoreComing
 *
 * interfaceIndex:  The interface on which the service was resolved.
 *
 * errorCode:       Will be kDNSServiceErr_NoError (0) on success, otherwise will
 *                  indicate the failure that occurred. Other parameters are undefined if
 *                  the errorCode is nonzero.
 *
 * fullname:        The full service domain name, in the form <servicename>.<protocol>.<domain>.
 *                  (This name is escaped following standard DNS rules, making it suitable for
 *                  passing to standard system DNS APIs such as res_query(), or to the
 *                  special-purpose functions included in this API that take fullname parameters.
 *                  See "Notes on DNS Name Escaping" earlier in this file for more details.)
 *
 * hosttarget:      The target hostname of the machine providing the service. This name can
 *                  be passed to functions like gethostbyname() to identify the host's IP address.
 *
 * port:            The port, in network byte order, on which connections are accepted for this service.
 *
 * txtLen:          The length of the txt record, in bytes.
 *
 * txtRecord:       The service's primary txt record, in standard txt record format.
 *
 * context:         The context pointer that was passed to the callout.
 *
 * NOTE: In earlier versions of this header file, the txtRecord parameter was declared "const char *"
 * This is incorrect, since it contains length bytes which are values in the range 0 to 255, not -128 to +127.
 * Depending on your compiler settings, this change may cause signed/unsigned mismatch warnings.
 * These should be fixed by updating your own callback function definition to match the corrected
 * function signature using "const unsigned char *txtRecord". Making this change may also fix inadvertent
 * bugs in your callback function, where it could have incorrectly interpreted a length byte with value 250
 * as being -6 instead, with various bad consequences ranging from incorrect operation to software crashes.
 * If you need to maintain portable code that will compile cleanly with both the old and new versions of
 * this header file, you should update your callback function definition to use the correct unsigned value,
 * and then in the place where you pass your callback function to DNSServiceResolve(), use a cast to eliminate
 * the compiler warning, e.g.:
 *   DNSServiceResolve(sd, flags, index, name, regtype, domain, (DNSServiceResolveReply)MyCallback, context);
 * This will ensure that your code compiles cleanly without warnings (and more importantly, works correctly)
 * with both the old header and with the new corrected version.
 *
 */

typedef void (DNSSD_API *DNSServiceResolveReply)
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *fullname,
    const char                          *hosttarget,
    uint16_t                            port,        /* In network byte order */
    uint16_t                            txtLen,
    const unsigned char                 *txtRecord,
    void                                *context
    );


/* DNSServiceQueryRecord
 *
 * Query for an arbitrary DNS record.
 *
 * DNSServiceQueryRecordReply() Callback Parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceQueryRecord().
 *
 * flags:           Possible values are kDNSServiceFlagsMoreComing and
 *                  kDNSServiceFlagsAdd. The Add flag is NOT set for PTR records
 *                  with a ttl of 0, i.e. "Remove" events.
 *
 * interfaceIndex:  The interface on which the query was resolved (the index for a given
 *                  interface is determined via the if_nametoindex() family of calls).
 *                  See "Constants for specifying an interface index" for more details.
 *
 * errorCode:       Will be kDNSServiceErr_NoError on success, otherwise will
 *                  indicate the failure that occurred. Other parameters are undefined if
 *                  errorCode is nonzero.
 *
 * fullname:        The resource record's full domain name.
 *
 * rrtype:          The resource record's type (e.g. kDNSServiceType_PTR, kDNSServiceType_SRV, etc)
 *
 * rrclass:         The class of the resource record (usually kDNSServiceClass_IN).
 *
 * rdlen:           The length, in bytes, of the resource record rdata.
 *
 * rdata:           The raw rdata of the resource record.
 *
 * ttl:             If the client wishes to cache the result for performance reasons,
 *                  the TTL indicates how long the client may legitimately hold onto
 *                  this result, in seconds. After the TTL expires, the client should
 *                  consider the result no longer valid, and if it requires this data
 *                  again, it should be re-fetched with a new query. Of course, this
 *                  only applies to clients that cancel the asynchronous operation when
 *                  they get a result. Clients that leave the asynchronous operation
 *                  running can safely assume that the data remains valid until they
 *                  get another callback telling them otherwise.
 *
 * context:         The context pointer that was passed to the callout.
 *
 */

typedef void (DNSSD_API *DNSServiceQueryRecordReply)
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl,
    void                                *context
    );

/* DNSServiceGetAddrInfo
 *
 * Queries for the IP address of a hostname by using either Multicast or Unicast DNS.
 *
 * DNSServiceGetAddrInfoReply() parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceGetAddrInfo().
 *
 * flags:           Possible values are kDNSServiceFlagsMoreComing and
 *                  kDNSServiceFlagsAdd.
 *
 * interfaceIndex:  The interface to which the answers pertain.
 *
 * errorCode:       Will be kDNSServiceErr_NoError on success, otherwise will
 *                  indicate the failure that occurred.  Other parameters are
 *                  undefined if errorCode is nonzero.
 *
 * hostname:        The fully qualified domain name of the host to be queried for.
 *
 * address:         IPv4 or IPv6 address.
 *
 * ttl:             If the client wishes to cache the result for performance reasons,
 *                  the TTL indicates how long the client may legitimately hold onto
 *                  this result, in seconds. After the TTL expires, the client should
 *                  consider the result no longer valid, and if it requires this data
 *                  again, it should be re-fetched with a new query. Of course, this
 *                  only applies to clients that cancel the asynchronous operation when
 *                  they get a result. Clients that leave the asynchronous operation
 *                  running can safely assume that the data remains valid until they
 *                  get another callback telling them otherwise.
 *
 * context:         The context pointer that was passed to the callout.
 *
 */

typedef void (DNSSD_API *DNSServiceGetAddrInfoReply)
    (
    DNSServiceRef                    sdRef,
    DNSServiceFlags                  flags,
    uint32_t                         interfaceIndex,
    DNSServiceErrorType              errorCode,
    const char                       *hostname,
    const struct sockaddr            *address,
    uint32_t                         ttl,
    void                             *context
    );



/* DNSServiceRegisterRecord
 *
 * Register an individual resource record on a connected DNSServiceRef.
 *
 * Note that name conflicts occurring for records registered via this call must be handled
 * by the client in the callback.
 *
 * DNSServiceRegisterRecordReply() parameters:
 *
 * sdRef:           The connected DNSServiceRef initialized by
 *                  DNSServiceCreateConnection().
 *
 * RecordRef:       The DNSRecordRef initialized by DNSServiceRegisterRecord(). If the above
 *                  DNSServiceRef is passed to DNSServiceRefDeallocate(), this DNSRecordRef is
 *                  invalidated, and may not be used further.
 *
 * flags:           Currently unused, reserved for future use.
 *
 * errorCode:       Will be kDNSServiceErr_NoError on success, otherwise will
 *                  indicate the failure that occurred (including name conflicts.)
 *                  Other parameters are undefined if errorCode is nonzero.
 *
 * context:         The context pointer that was passed to the callout.
 *
 */

 typedef void (DNSSD_API *DNSServiceRegisterRecordReply)
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        RecordRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    void                                *context
    );


/* DNSServiceNATPortMappingCreate
 *
 * Request a port mapping in the NAT gateway, which maps a port on the local machine
 * to an external port on the NAT. The NAT should support either the NAT-PMP or the UPnP IGD
 * protocol for this API to create a successful mapping.
 *
 * The port mapping will be renewed indefinitely until the client process exits, or
 * explicitly terminates the port mapping request by calling DNSServiceRefDeallocate().
 * The client callback will be invoked, informing the client of the NAT gateway's
 * external IP address and the external port that has been allocated for this client.
 * The client should then record this external IP address and port using whatever
 * directory service mechanism it is using to enable peers to connect to it.
 * (Clients advertising services using Wide-Area DNS-SD DO NOT need to use this API
 * -- when a client calls DNSServiceRegister() NAT mappings are automatically created
 * and the external IP address and port for the service are recorded in the global DNS.
 * Only clients using some directory mechanism other than Wide-Area DNS-SD need to use
 * this API to explicitly map their own ports.)
 *
 * It's possible that the client callback could be called multiple times, for example
 * if the NAT gateway's IP address changes, or if a configuration change results in a
 * different external port being mapped for this client. Over the lifetime of any long-lived
 * port mapping, the client should be prepared to handle these notifications of changes
 * in the environment, and should update its recorded address and/or port as appropriate.
 *
 * NOTE: There are two unusual aspects of how the DNSServiceNATPortMappingCreate API works,
 * which were intentionally designed to help simplify client code:
 *
 *  1. It's not an error to request a NAT mapping when the machine is not behind a NAT gateway.
 *     In other NAT mapping APIs, if you request a NAT mapping and the machine is not behind a NAT
 *     gateway, then the API returns an error code -- it can't get you a NAT mapping if there's no
 *     NAT gateway. The DNSServiceNATPortMappingCreate API takes a different view. Working out
 *     whether or not you need a NAT mapping can be tricky and non-obvious, particularly on
 *     a machine with multiple active network interfaces. Rather than make every client recreate
 *     this logic for deciding whether a NAT mapping is required, the PortMapping API does that
 *     work for you. If the client calls the PortMapping API when the machine already has a
 *     routable public IP address, then instead of complaining about it and giving an error,
 *     the PortMapping API just invokes your callback, giving the machine's public address
 *     and your own port number. This means you don't need to write code to work out whether
 *     your client needs to call the PortMapping API -- just call it anyway, and if it wasn't
 *     necessary, no harm is done:
 *
 *     - If the machine already has a routable public IP address, then your callback
 *       will just be invoked giving your own address and port.
 *     - If a NAT mapping is required and obtained, then your callback will be invoked
 *       giving you the external address and port.
 *     - If a NAT mapping is required but not obtained from the local NAT gateway,
 *       or the machine has no network connectivity, then your callback will be
 *       invoked giving zero address and port.
 *
 *  2. In other NAT mapping APIs, if a laptop computer is put to sleep and woken up on a new
 *     network, it's the client's job to notice this, and work out whether a NAT mapping
 *     is required on the new network, and make a new NAT mapping request if necessary.
 *     The DNSServiceNATPortMappingCreate API does this for you, automatically.
 *     The client just needs to make one call to the PortMapping API, and its callback will
 *     be invoked any time the mapping state changes. This property complements point (1) above.
 *     If the client didn't make a NAT mapping request just because it determined that one was
 *     not required at that particular moment in time, the client would then have to monitor
 *     for network state changes to determine if a NAT port mapping later became necessary.
 *     By unconditionally making a NAT mapping request, even when a NAT mapping not to be
 *     necessary, the PortMapping API will then begin monitoring network state changes on behalf of
 *     the client, and if a NAT mapping later becomes necessary, it will automatically create a NAT
 *     mapping and inform the client with a new callback giving the new address and port information.
 *
 * DNSServiceNATPortMappingReply() parameters:
 *
 * sdRef:           The DNSServiceRef initialized by DNSServiceNATPortMappingCreate().
 *
 * flags:           Currently unused, reserved for future use.
 *
 * interfaceIndex:  The interface through which the NAT gateway is reached.
 *
 * errorCode:       Will be kDNSServiceErr_NoError on success.
 *                  Will be kDNSServiceErr_DoubleNAT when the NAT gateway is itself behind one or
 *                  more layers of NAT, in which case the other parameters have the defined values.
 *                  For other failures, will indicate the failure that occurred, and the other
 *                  parameters are undefined.
 *
 * externalAddress: Four byte IPv4 address in network byte order.
 *
 * protocol:        Will be kDNSServiceProtocol_UDP or kDNSServiceProtocol_TCP or both.
 *
 * internalPort:    The port on the local machine that was mapped.
 *
 * externalPort:    The actual external port in the NAT gateway that was mapped.
 *                  This is likely to be different than the requested external port.
 *
 * ttl:             The lifetime of the NAT port mapping created on the gateway.
 *                  This controls how quickly stale mappings will be garbage-collected
 *                  if the client machine crashes, suffers a power failure, is disconnected
 *                  from the network, or suffers some other unfortunate demise which
 *                  causes it to vanish without explicitly removing its NAT port mapping.
 *                  It's possible that the ttl value will differ from the requested ttl value.
 *
 * context:         The context pointer that was passed to the callout.
 *
 */

typedef void (DNSSD_API *DNSServiceNATPortMappingReply)
    (
    DNSServiceRef                    sdRef,
    DNSServiceFlags                  flags,
    uint32_t                         interfaceIndex,
    DNSServiceErrorType              errorCode,
    uint32_t                         externalAddress,   /* four byte IPv4 address in network byte order */
    DNSServiceProtocol               protocol,
    uint16_t                         internalPort,      /* In network byte order */
    uint16_t                         externalPort,      /* In network byte order and may be different than the requested port */
    uint32_t                         ttl,               /* may be different than the requested ttl */
    void                             *context
    );



/* TXTRecordRef
 *
 * Opaque internal data type.
 * Note: Represents a DNS-SD TXT record.
 */

typedef union _TXTRecordRef_t { char PrivateData[16]; char *ForceNaturalAlignment; } TXTRecordRef;


#ifdef  __cplusplus
    }
#endif

#endif
