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

#ifndef _DNS_SD_H
#define _DNS_SD_H 3200500

#include "dns_sd_types.h"
/*********************************************************************************************
 *
 * Version checking
 *
 *********************************************************************************************/

/* DNSServiceGetProperty() Parameters:
 *
 * property:        The requested property.
 *                  Currently the only property defined is kDNSServiceProperty_DaemonVersion.
 *
 * result:          Place to store result.
 *                  For retrieving DaemonVersion, this should be the address of a uint32_t.
 *
 * size:            Pointer to uint32_t containing size of the result location.
 *                  For retrieving DaemonVersion, this should be sizeof(uint32_t).
 *                  On return the uint32_t is updated to the size of the data returned.
 *                  For DaemonVersion, the returned size is always sizeof(uint32_t), but
 *                  future properties could be defined which return variable-sized results.
 *
 * return value:    Returns kDNSServiceErr_NoError on success, or kDNSServiceErr_ServiceNotRunning
 *                  if the daemon (or "system service" on Windows) is not running.
 */

DNSServiceErrorType DNSSD_API DNSServiceGetProperty
    (
    const char *property,  /* Requested property (i.e. kDNSServiceProperty_DaemonVersion) */
    void       *result,    /* Pointer to place to store result */
    uint32_t   *size       /* size of result location */
    );


/*********************************************************************************************
 *
 * Unix Domain Socket access, DNSServiceRef deallocation, and data processing functions
 *
 *********************************************************************************************/

/* DNSServiceRefSockFD()
 *
 * Access underlying Unix domain socket for an initialized DNSServiceRef.
 * The DNS Service Discovery implementation uses this socket to communicate between the client and
 * the mDNSResponder daemon. The application MUST NOT directly read from or write to this socket.
 * Access to the socket is provided so that it can be used as a kqueue event source, a CFRunLoop
 * event source, in a select() loop, etc. When the underlying event management subsystem (kqueue/
 * select/CFRunLoop etc.) indicates to the client that data is available for reading on the
 * socket, the client should call DNSServiceProcessResult(), which will extract the daemon's
 * reply from the socket, and pass it to the appropriate application callback. By using a run
 * loop or select(), results from the daemon can be processed asynchronously. Alternatively,
 * a client can choose to fork a thread and have it loop calling "DNSServiceProcessResult(ref);"
 * If DNSServiceProcessResult() is called when no data is available for reading on the socket, it
 * will block until data does become available, and then process the data and return to the caller.
 * When data arrives on the socket, the client is responsible for calling DNSServiceProcessResult(ref)
 * in a timely fashion -- if the client allows a large backlog of data to build up the daemon
 * may terminate the connection.
 *
 * sdRef:           A DNSServiceRef initialized by any of the DNSService calls.
 *
 * return value:    The DNSServiceRef's underlying socket descriptor, or -1 on
 *                  error.
 */

int DNSSD_API DNSServiceRefSockFD(DNSServiceRef sdRef);


/* DNSServiceProcessResult()
 *
 * Read a reply from the daemon, calling the appropriate application callback. This call will
 * block until the daemon's response is received. Use DNSServiceRefSockFD() in
 * conjunction with a run loop or select() to determine the presence of a response from the
 * server before calling this function to process the reply without blocking. Call this function
 * at any point if it is acceptable to block until the daemon's response arrives. Note that the
 * client is responsible for ensuring that DNSServiceProcessResult() is called whenever there is
 * a reply from the daemon - the daemon may terminate its connection with a client that does not
 * process the daemon's responses.
 *
 * sdRef:           A DNSServiceRef initialized by any of the DNSService calls
 *                  that take a callback parameter.
 *
 * return value:    Returns kDNSServiceErr_NoError on success, otherwise returns
 *                  an error code indicating the specific failure that occurred.
 */

DNSServiceErrorType DNSSD_API DNSServiceProcessResult(DNSServiceRef sdRef);


/* DNSServiceRefDeallocate()
 *
 * Terminate a connection with the daemon and free memory associated with the DNSServiceRef.
 * Any services or records registered with this DNSServiceRef will be deregistered. Any
 * Browse, Resolve, or Query operations called with this reference will be terminated.
 *
 * Note: If the reference's underlying socket is used in a run loop or select() call, it should
 * be removed BEFORE DNSServiceRefDeallocate() is called, as this function closes the reference's
 * socket.
 *
 * Note: If the reference was initialized with DNSServiceCreateConnection(), any DNSRecordRefs
 * created via this reference will be invalidated by this call - the resource records are
 * deregistered, and their DNSRecordRefs may not be used in subsequent functions. Similarly,
 * if the reference was initialized with DNSServiceRegister, and an extra resource record was
 * added to the service via DNSServiceAddRecord(), the DNSRecordRef created by the Add() call
 * is invalidated when this function is called - the DNSRecordRef may not be used in subsequent
 * functions.
 *
 * Note: This call is to be used only with the DNSServiceRef defined by this API. It is
 * not compatible with dns_service_discovery_ref objects defined in the legacy Mach-based
 * DNSServiceDiscovery.h API.
 *
 * sdRef:           A DNSServiceRef initialized by any of the DNSService calls.
 *
 */

void DNSSD_API DNSServiceRefDeallocate(DNSServiceRef sdRef);


/*********************************************************************************************
 *
 * Domain Enumeration
 *
 *********************************************************************************************/


/* DNSServiceEnumerateDomains() Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds
 *                  then it initializes the DNSServiceRef, returns kDNSServiceErr_NoError,
 *                  and the enumeration operation will run indefinitely until the client
 *                  terminates it by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * flags:           Possible values are:
 *                  kDNSServiceFlagsBrowseDomains to enumerate domains recommended for browsing.
 *                  kDNSServiceFlagsRegistrationDomains to enumerate domains recommended
 *                  for registration.
 *
 * interfaceIndex:  If non-zero, specifies the interface on which to look for domains.
 *                  (the index for a given interface is determined via the if_nametoindex()
 *                  family of calls.) Most applications will pass 0 to enumerate domains on
 *                  all interfaces. See "Constants for specifying an interface index" for more details.
 *
 * callBack:        The function to be called when a domain is found or the call asynchronously
 *                  fails.
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred (the callback is not invoked and the DNSServiceRef
 *                  is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceEnumerateDomains
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceDomainEnumReply           callBack,
    void                                *context  /* may be NULL */
    );


/*********************************************************************************************
 *
 *  Service Registration
 *
 *********************************************************************************************/

/* DNSServiceRegister() Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds
 *                  then it initializes the DNSServiceRef, returns kDNSServiceErr_NoError,
 *                  and the registration will remain active indefinitely until the client
 *                  terminates it by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * interfaceIndex:  If non-zero, specifies the interface on which to register the service
 *                  (the index for a given interface is determined via the if_nametoindex()
 *                  family of calls.) Most applications will pass 0 to register on all
 *                  available interfaces. See "Constants for specifying an interface index" for more details.
 *
 * flags:           Indicates the renaming behavior on name conflict (most applications
 *                  will pass 0). See flag definitions above for details.
 *
 * name:            If non-NULL, specifies the service name to be registered.
 *                  Most applications will not specify a name, in which case the computer
 *                  name is used (this name is communicated to the client via the callback).
 *                  If a name is specified, it must be 1-63 bytes of UTF-8 text.
 *                  If the name is longer than 63 bytes it will be automatically truncated
 *                  to a legal length, unless the NoAutoRename flag is set,
 *                  in which case kDNSServiceErr_BadParam will be returned.
 *
 * regtype:         The service type followed by the protocol, separated by a dot
 *                  (e.g. "_ftp._tcp"). The service type must be an underscore, followed
 *                  by 1-15 characters, which may be letters, digits, or hyphens.
 *                  The transport protocol must be "_tcp" or "_udp". New service types
 *                  should be registered at <http://www.dns-sd.org/ServiceTypes.html>.
 *
 *                  Additional subtypes of the primary service type (where a service
 *                  type has defined subtypes) follow the primary service type in a
 *                  comma-separated list, with no additional spaces, e.g.
 *                      "_primarytype._tcp,_subtype1,_subtype2,_subtype3"
 *                  Subtypes provide a mechanism for filtered browsing: A client browsing
 *                  for "_primarytype._tcp" will discover all instances of this type;
 *                  a client browsing for "_primarytype._tcp,_subtype2" will discover only
 *                  those instances that were registered with "_subtype2" in their list of
 *                  registered subtypes.
 *
 *                  The subtype mechanism can be illustrated with some examples using the
 *                  dns-sd command-line tool:
 *
 *                  % dns-sd -R Simple _test._tcp "" 1001 &
 *                  % dns-sd -R Better _test._tcp,HasFeatureA "" 1002 &
 *                  % dns-sd -R Best   _test._tcp,HasFeatureA,HasFeatureB "" 1003 &
 *
 *                  Now:
 *                  % dns-sd -B _test._tcp             # will find all three services
 *                  % dns-sd -B _test._tcp,HasFeatureA # finds "Better" and "Best"
 *                  % dns-sd -B _test._tcp,HasFeatureB # finds only "Best"
 *
 *                  Subtype labels may be up to 63 bytes long, and may contain any eight-
 *                  bit byte values, including zero bytes. However, due to the nature of
 *                  using a C-string-based API, conventional DNS escaping must be used for
 *                  dots ('.'), commas (','), backslashes ('\') and zero bytes, as shown below:
 *
 *                  % dns-sd -R Test '_test._tcp,s\.one,s\,two,s\\three,s\000four' local 123
 *
 * domain:          If non-NULL, specifies the domain on which to advertise the service.
 *                  Most applications will not specify a domain, instead automatically
 *                  registering in the default domain(s).
 *
 * host:            If non-NULL, specifies the SRV target host name. Most applications
 *                  will not specify a host, instead automatically using the machine's
 *                  default host name(s). Note that specifying a non-NULL host does NOT
 *                  create an address record for that host - the application is responsible
 *                  for ensuring that the appropriate address record exists, or creating it
 *                  via DNSServiceRegisterRecord().
 *
 * port:            The port, in network byte order, on which the service accepts connections.
 *                  Pass 0 for a "placeholder" service (i.e. a service that will not be discovered
 *                  by browsing, but will cause a name conflict if another client tries to
 *                  register that same name). Most clients will not use placeholder services.
 *
 * txtLen:          The length of the txtRecord, in bytes. Must be zero if the txtRecord is NULL.
 *
 * txtRecord:       The TXT record rdata. A non-NULL txtRecord MUST be a properly formatted DNS
 *                  TXT record, i.e. <length byte> <data> <length byte> <data> ...
 *                  Passing NULL for the txtRecord is allowed as a synonym for txtLen=1, txtRecord="",
 *                  i.e. it creates a TXT record of length one containing a single empty string.
 *                  RFC 1035 doesn't allow a TXT record to contain *zero* strings, so a single empty
 *                  string is the smallest legal DNS TXT record.
 *                  As with the other parameters, the DNSServiceRegister call copies the txtRecord
 *                  data; e.g. if you allocated the storage for the txtRecord parameter with malloc()
 *                  then you can safely free that memory right after the DNSServiceRegister call returns.
 *
 * callBack:        The function to be called when the registration completes or asynchronously
 *                  fails. The client MAY pass NULL for the callback -  The client will NOT be notified
 *                  of the default values picked on its behalf, and the client will NOT be notified of any
 *                  asynchronous errors (e.g. out of memory errors, etc.) that may prevent the registration
 *                  of the service. The client may NOT pass the NoAutoRename flag if the callback is NULL.
 *                  The client may still deregister the service at any time via DNSServiceRefDeallocate().
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred (the callback is never invoked and the DNSServiceRef
 *                  is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceRegister
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,         /* may be NULL */
    const char                          *regtype,
    const char                          *domain,       /* may be NULL */
    const char                          *host,         /* may be NULL */
    uint16_t                            port,          /* In network byte order */
    uint16_t                            txtLen,
    const void                          *txtRecord,    /* may be NULL */
    DNSServiceRegisterReply             callBack,      /* may be NULL */
    void                                *context       /* may be NULL */
    );


/* DNSServiceAddRecord()
 *
 * Add a record to a registered service. The name of the record will be the same as the
 * registered service's name.
 * The record can later be updated or deregistered by passing the RecordRef initialized
 * by this function to DNSServiceUpdateRecord() or DNSServiceRemoveRecord().
 *
 * Note that the DNSServiceAddRecord/UpdateRecord/RemoveRecord are *NOT* thread-safe
 * with respect to a single DNSServiceRef. If you plan to have multiple threads
 * in your program simultaneously add, update, or remove records from the same
 * DNSServiceRef, then it's the caller's responsibility to use a mutext lock
 * or take similar appropriate precautions to serialize those calls.
 *
 * Parameters;
 *
 * sdRef:           A DNSServiceRef initialized by DNSServiceRegister().
 *
 * RecordRef:       A pointer to an uninitialized DNSRecordRef. Upon succesfull completion of this
 *                  call, this ref may be passed to DNSServiceUpdateRecord() or DNSServiceRemoveRecord().
 *                  If the above DNSServiceRef is passed to DNSServiceRefDeallocate(), RecordRef is also
 *                  invalidated and may not be used further.
 *
 * flags:           Currently ignored, reserved for future use.
 *
 * rrtype:          The type of the record (e.g. kDNSServiceType_TXT, kDNSServiceType_SRV, etc)
 *
 * rdlen:           The length, in bytes, of the rdata.
 *
 * rdata:           The raw rdata to be contained in the added resource record.
 *
 * ttl:             The time to live of the resource record, in seconds.
 *                  Most clients should pass 0 to indicate that the system should
 *                  select a sensible default value.
 *
 * return value:    Returns kDNSServiceErr_NoError on success, otherwise returns an
 *                  error code indicating the error that occurred (the RecordRef is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceAddRecord
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        *RecordRef,
    DNSServiceFlags                     flags,
    uint16_t                            rrtype,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl
    );


/* DNSServiceUpdateRecord
 *
 * Update a registered resource record. The record must either be:
 *   - The primary txt record of a service registered via DNSServiceRegister()
 *   - A record added to a registered service via DNSServiceAddRecord()
 *   - An individual record registered by DNSServiceRegisterRecord()
 *
 * Parameters:
 *
 * sdRef:           A DNSServiceRef that was initialized by DNSServiceRegister()
 *                  or DNSServiceCreateConnection().
 *
 * RecordRef:       A DNSRecordRef initialized by DNSServiceAddRecord, or NULL to update the
 *                  service's primary txt record.
 *
 * flags:           Currently ignored, reserved for future use.
 *
 * rdlen:           The length, in bytes, of the new rdata.
 *
 * rdata:           The new rdata to be contained in the updated resource record.
 *
 * ttl:             The time to live of the updated resource record, in seconds.
 *                  Most clients should pass 0 to indicate that the system should
 *                  select a sensible default value.
 *
 * return value:    Returns kDNSServiceErr_NoError on success, otherwise returns an
 *                  error code indicating the error that occurred.
 */

DNSServiceErrorType DNSSD_API DNSServiceUpdateRecord
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        RecordRef,     /* may be NULL */
    DNSServiceFlags                     flags,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl
    );


/* DNSServiceRemoveRecord
 *
 * Remove a record previously added to a service record set via DNSServiceAddRecord(), or deregister
 * an record registered individually via DNSServiceRegisterRecord().
 *
 * Parameters:
 *
 * sdRef:           A DNSServiceRef initialized by DNSServiceRegister() (if the
 *                  record being removed was registered via DNSServiceAddRecord()) or by
 *                  DNSServiceCreateConnection() (if the record being removed was registered via
 *                  DNSServiceRegisterRecord()).
 *
 * recordRef:       A DNSRecordRef initialized by a successful call to DNSServiceAddRecord()
 *                  or DNSServiceRegisterRecord().
 *
 * flags:           Currently ignored, reserved for future use.
 *
 * return value:    Returns kDNSServiceErr_NoError on success, otherwise returns an
 *                  error code indicating the error that occurred.
 */

DNSServiceErrorType DNSSD_API DNSServiceRemoveRecord
    (
    DNSServiceRef                 sdRef,
    DNSRecordRef                  RecordRef,
    DNSServiceFlags               flags
    );


/*********************************************************************************************
 *
 *  Service Discovery
 *
 *********************************************************************************************/

/* DNSServiceBrowse() Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds
 *                  then it initializes the DNSServiceRef, returns kDNSServiceErr_NoError,
 *                  and the browse operation will run indefinitely until the client
 *                  terminates it by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * flags:           Currently ignored, reserved for future use.
 *
 * interfaceIndex:  If non-zero, specifies the interface on which to browse for services
 *                  (the index for a given interface is determined via the if_nametoindex()
 *                  family of calls.) Most applications will pass 0 to browse on all available
 *                  interfaces. See "Constants for specifying an interface index" for more details.
 *
 * regtype:         The service type being browsed for followed by the protocol, separated by a
 *                  dot (e.g. "_ftp._tcp"). The transport protocol must be "_tcp" or "_udp".
 *                  A client may optionally specify a single subtype to perform filtered browsing:
 *                  e.g. browsing for "_primarytype._tcp,_subtype" will discover only those
 *                  instances of "_primarytype._tcp" that were registered specifying "_subtype"
 *                  in their list of registered subtypes.
 *
 * domain:          If non-NULL, specifies the domain on which to browse for services.
 *                  Most applications will not specify a domain, instead browsing on the
 *                  default domain(s).
 *
 * callBack:        The function to be called when an instance of the service being browsed for
 *                  is found, or if the call asynchronously fails.
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred (the callback is not invoked and the DNSServiceRef
 *                  is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceBrowse
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *regtype,
    const char                          *domain,    /* may be NULL */
    DNSServiceBrowseReply               callBack,
    void                                *context    /* may be NULL */
    );

/* DNSServiceResolve() Parameters
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds
 *                  then it initializes the DNSServiceRef, returns kDNSServiceErr_NoError,
 *                  and the resolve operation will run indefinitely until the client
 *                  terminates it by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * flags:           Specifying kDNSServiceFlagsForceMulticast will cause query to be
 *                  performed with a link-local mDNS query, even if the name is an
 *                  apparently non-local name (i.e. a name not ending in ".local.")
 *
 * interfaceIndex:  The interface on which to resolve the service. If this resolve call is
 *                  as a result of a currently active DNSServiceBrowse() operation, then the
 *                  interfaceIndex should be the index reported in the DNSServiceBrowseReply
 *                  callback. If this resolve call is using information previously saved
 *                  (e.g. in a preference file) for later use, then use interfaceIndex 0, because
 *                  the desired service may now be reachable via a different physical interface.
 *                  See "Constants for specifying an interface index" for more details.
 *
 * name:            The name of the service instance to be resolved, as reported to the
 *                  DNSServiceBrowseReply() callback.
 *
 * regtype:         The type of the service instance to be resolved, as reported to the
 *                  DNSServiceBrowseReply() callback.
 *
 * domain:          The domain of the service instance to be resolved, as reported to the
 *                  DNSServiceBrowseReply() callback.
 *
 * callBack:        The function to be called when a result is found, or if the call
 *                  asynchronously fails.
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred (the callback is never invoked and the DNSServiceRef
 *                  is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceResolve
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    DNSServiceResolveReply              callBack,
    void                                *context  /* may be NULL */
    );


/*********************************************************************************************
 *
 *  Querying Individual Specific Records
 *
 *********************************************************************************************/

/* DNSServiceQueryRecord() Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds
 *                  then it initializes the DNSServiceRef, returns kDNSServiceErr_NoError,
 *                  and the query operation will run indefinitely until the client
 *                  terminates it by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * flags:           kDNSServiceFlagsForceMulticast or kDNSServiceFlagsLongLivedQuery.
 *                  Pass kDNSServiceFlagsLongLivedQuery to create a "long-lived" unicast
 *                  query in a non-local domain. Without setting this flag, unicast queries
 *                  will be one-shot - that is, only answers available at the time of the call
 *                  will be returned. By setting this flag, answers (including Add and Remove
 *                  events) that become available after the initial call is made will generate
 *                  callbacks. This flag has no effect on link-local multicast queries.
 *
 * interfaceIndex:  If non-zero, specifies the interface on which to issue the query
 *                  (the index for a given interface is determined via the if_nametoindex()
 *                  family of calls.) Passing 0 causes the name to be queried for on all
 *                  interfaces. See "Constants for specifying an interface index" for more details.
 *
 * fullname:        The full domain name of the resource record to be queried for.
 *
 * rrtype:          The numerical type of the resource record to be queried for
 *                  (e.g. kDNSServiceType_PTR, kDNSServiceType_SRV, etc)
 *
 * rrclass:         The class of the resource record (usually kDNSServiceClass_IN).
 *
 * callBack:        The function to be called when a result is found, or if the call
 *                  asynchronously fails.
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred (the callback is never invoked and the DNSServiceRef
 *                  is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceQueryRecord
    (
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    DNSServiceQueryRecordReply          callBack,
    void                                *context  /* may be NULL */
    );


/*********************************************************************************************
 *
 *  Unified lookup of both IPv4 and IPv6 addresses for a fully qualified hostname
 *
 *********************************************************************************************/

/* DNSServiceGetAddrInfo() Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds then it
 *                  initializes the DNSServiceRef, returns kDNSServiceErr_NoError, and the query
 *                  begins and will last indefinitely until the client terminates the query
 *                  by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * flags:           kDNSServiceFlagsForceMulticast or kDNSServiceFlagsLongLivedQuery.
 *                  Pass kDNSServiceFlagsLongLivedQuery to create a "long-lived" unicast
 *                  query in a non-local domain. Without setting this flag, unicast queries
 *                  will be one-shot - that is, only answers available at the time of the call
 *                  will be returned. By setting this flag, answers (including Add and Remove
 *                  events) that become available after the initial call is made will generate
 *                  callbacks. This flag has no effect on link-local multicast queries.
 *
 * interfaceIndex:  The interface on which to issue the query.  Passing 0 causes the query to be
 *                  sent on all active interfaces via Multicast or the primary interface via Unicast.
 *
 * protocol:        Pass in kDNSServiceProtocol_IPv4 to look up IPv4 addresses, or kDNSServiceProtocol_IPv6
 *                  to look up IPv6 addresses, or both to look up both kinds. If neither flag is
 *                  set, the system will apply an intelligent heuristic, which is (currently)
 *                  that it will attempt to look up both, except:
 *
 *                   * If "hostname" is a wide-area unicast DNS hostname (i.e. not a ".local." name)
 *                     but this host has no routable IPv6 address, then the call will not try to
 *                     look up IPv6 addresses for "hostname", since any addresses it found would be
 *                     unlikely to be of any use anyway. Similarly, if this host has no routable
 *                     IPv4 address, the call will not try to look up IPv4 addresses for "hostname".
 *
 * hostname:        The fully qualified domain name of the host to be queried for.
 *
 * callBack:        The function to be called when the query succeeds or fails asynchronously.
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred.
 */

DNSServiceErrorType DNSSD_API DNSServiceGetAddrInfo
    (
    DNSServiceRef                    *sdRef,
    DNSServiceFlags                  flags,
    uint32_t                         interfaceIndex,
    DNSServiceProtocol               protocol,
    const char                       *hostname,
    DNSServiceGetAddrInfoReply       callBack,
    void                             *context          /* may be NULL */
    );


/*********************************************************************************************
 *
 *  Special Purpose Calls:
 *  DNSServiceCreateConnection(), DNSServiceRegisterRecord(), DNSServiceReconfirmRecord()
 *  (most applications will not use these)
 *
 *********************************************************************************************/

/* DNSServiceCreateConnection()
 *
 * Create a connection to the daemon allowing efficient registration of
 * multiple individual records.
 *
 * Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. Deallocating
 *                  the reference (via DNSServiceRefDeallocate()) severs the
 *                  connection and deregisters all records registered on this connection.
 *
 * return value:    Returns kDNSServiceErr_NoError on success, otherwise returns
 *                  an error code indicating the specific failure that occurred (in which
 *                  case the DNSServiceRef is not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceCreateConnection(DNSServiceRef *sdRef);

/* DNSServiceRegisterRecord() Parameters:
 *
 * sdRef:           A DNSServiceRef initialized by DNSServiceCreateConnection().
 *
 * RecordRef:       A pointer to an uninitialized DNSRecordRef. Upon succesfull completion of this
 *                  call, this ref may be passed to DNSServiceUpdateRecord() or DNSServiceRemoveRecord().
 *                  (To deregister ALL records registered on a single connected DNSServiceRef
 *                  and deallocate each of their corresponding DNSServiceRecordRefs, call
 *                  DNSServiceRefDeallocate()).
 *
 * flags:           Possible values are kDNSServiceFlagsShared or kDNSServiceFlagsUnique
 *                  (see flag type definitions for details).
 *
 * interfaceIndex:  If non-zero, specifies the interface on which to register the record
 *                  (the index for a given interface is determined via the if_nametoindex()
 *                  family of calls.) Passing 0 causes the record to be registered on all interfaces.
 *                  See "Constants for specifying an interface index" for more details.
 *
 * fullname:        The full domain name of the resource record.
 *
 * rrtype:          The numerical type of the resource record (e.g. kDNSServiceType_PTR, kDNSServiceType_SRV, etc)
 *
 * rrclass:         The class of the resource record (usually kDNSServiceClass_IN)
 *
 * rdlen:           Length, in bytes, of the rdata.
 *
 * rdata:           A pointer to the raw rdata, as it is to appear in the DNS record.
 *
 * ttl:             The time to live of the resource record, in seconds.
 *                  Most clients should pass 0 to indicate that the system should
 *                  select a sensible default value.
 *
 * callBack:        The function to be called when a result is found, or if the call
 *                  asynchronously fails (e.g. because of a name conflict.)
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred (the callback is never invoked and the DNSRecordRef is
 *                  not initialized).
 */

DNSServiceErrorType DNSSD_API DNSServiceRegisterRecord
    (
    DNSServiceRef                       sdRef,
    DNSRecordRef                        *RecordRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl,
    DNSServiceRegisterRecordReply       callBack,
    void                                *context    /* may be NULL */
    );


/* DNSServiceReconfirmRecord
 *
 * Instruct the daemon to verify the validity of a resource record that appears
 * to be out of date (e.g. because TCP connection to a service's target failed.)
 * Causes the record to be flushed from the daemon's cache (as well as all other
 * daemons' caches on the network) if the record is determined to be invalid.
 * Use this routine conservatively. Reconfirming a record necessarily consumes
 * network bandwidth, so this should not be done indiscriminately.
 *
 * Parameters:
 *
 * flags:           Pass kDNSServiceFlagsForce to force immediate deletion of record,
 *                  instead of after some number of reconfirmation queries have gone unanswered.
 *
 * interfaceIndex:  Specifies the interface of the record in question.
 *                  The caller must specify the interface.
 *                  This API (by design) causes increased network traffic, so it requires
 *                  the caller to be precise about which record should be reconfirmed.
 *                  It is not possible to pass zero for the interface index to perform
 *                  a "wildcard" reconfirmation, where *all* matching records are reconfirmed.
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
 */

DNSServiceErrorType DNSSD_API DNSServiceReconfirmRecord
    (
    DNSServiceFlags                    flags,
    uint32_t                           interfaceIndex,
    const char                         *fullname,
    uint16_t                           rrtype,
    uint16_t                           rrclass,
    uint16_t                           rdlen,
    const void                         *rdata
    );


/*********************************************************************************************
 *
 *  NAT Port Mapping
 *
 *********************************************************************************************/

/* DNSServiceNATPortMappingCreate() Parameters:
 *
 * sdRef:           A pointer to an uninitialized DNSServiceRef. If the call succeeds then it
 *                  initializes the DNSServiceRef, returns kDNSServiceErr_NoError, and the nat
 *                  port mapping will last indefinitely until the client terminates the port
 *                  mapping request by passing this DNSServiceRef to DNSServiceRefDeallocate().
 *
 * flags:           Currently ignored, reserved for future use.
 *
 * interfaceIndex:  The interface on which to create port mappings in a NAT gateway. Passing 0 causes
 *                  the port mapping request to be sent on the primary interface.
 *
 * protocol:        To request a port mapping, pass in kDNSServiceProtocol_UDP, or kDNSServiceProtocol_TCP,
 *                  or (kDNSServiceProtocol_UDP | kDNSServiceProtocol_TCP) to map both.
 *                  The local listening port number must also be specified in the internalPort parameter.
 *                  To just discover the NAT gateway's external IP address, pass zero for protocol,
 *                  internalPort, externalPort and ttl.
 *
 * internalPort:    The port number in network byte order on the local machine which is listening for packets.
 *
 * externalPort:    The requested external port in network byte order in the NAT gateway that you would
 *                  like to map to the internal port. Pass 0 if you don't care which external port is chosen for you.
 *
 * ttl:             The requested renewal period of the NAT port mapping, in seconds.
 *                  If the client machine crashes, suffers a power failure, is disconnected from
 *                  the network, or suffers some other unfortunate demise which causes it to vanish
 *                  unexpectedly without explicitly removing its NAT port mappings, then the NAT gateway
 *                  will garbage-collect old stale NAT port mappings when their lifetime expires.
 *                  Requesting a short TTL causes such orphaned mappings to be garbage-collected
 *                  more promptly, but consumes system resources and network bandwidth with
 *                  frequent renewal packets to keep the mapping from expiring.
 *                  Requesting a long TTL is more efficient on the network, but in the event of the
 *                  client vanishing, stale NAT port mappings will not be garbage-collected as quickly.
 *                  Most clients should pass 0 to use a system-wide default value.
 *
 * callBack:        The function to be called when the port mapping request succeeds or fails asynchronously.
 *
 * context:         An application context pointer which is passed to the callback function
 *                  (may be NULL).
 *
 * return value:    Returns kDNSServiceErr_NoError on success (any subsequent, asynchronous
 *                  errors are delivered to the callback), otherwise returns an error code indicating
 *                  the error that occurred.
 *
 *                  If you don't actually want a port mapped, and are just calling the API
 *                  because you want to find out the NAT's external IP address (e.g. for UI
 *                  display) then pass zero for protocol, internalPort, externalPort and ttl.
 */

DNSServiceErrorType DNSSD_API DNSServiceNATPortMappingCreate
    (
    DNSServiceRef                    *sdRef,
    DNSServiceFlags                  flags,
    uint32_t                         interfaceIndex,
    DNSServiceProtocol               protocol,          /* TCP and/or UDP          */
    uint16_t                         internalPort,      /* network byte order      */
    uint16_t                         externalPort,      /* network byte order      */
    uint32_t                         ttl,               /* time to live in seconds */
    DNSServiceNATPortMappingReply    callBack,
    void                             *context           /* may be NULL             */
    );


/*********************************************************************************************
 *
 *  General Utility Functions
 *
 *********************************************************************************************/

/* DNSServiceConstructFullName()
 *
 * Concatenate a three-part domain name (as returned by the above callbacks) into a
 * properly-escaped full domain name. Note that callbacks in the above functions ALREADY ESCAPE
 * strings where necessary.
 *
 * Parameters:
 *
 * fullName:        A pointer to a buffer that where the resulting full domain name is to be written.
 *                  The buffer must be kDNSServiceMaxDomainName (1009) bytes in length to
 *                  accommodate the longest legal domain name without buffer overrun.
 *
 * service:         The service name - any dots or backslashes must NOT be escaped.
 *                  May be NULL (to construct a PTR record name, e.g.
 *                  "_ftp._tcp.apple.com.").
 *
 * regtype:         The service type followed by the protocol, separated by a dot
 *                  (e.g. "_ftp._tcp").
 *
 * domain:          The domain name, e.g. "apple.com.". Literal dots or backslashes,
 *                  if any, must be escaped, e.g. "1st\. Floor.apple.com."
 *
 * return value:    Returns kDNSServiceErr_NoError (0) on success, kDNSServiceErr_BadParam on error.
 *
 */

DNSServiceErrorType DNSSD_API DNSServiceConstructFullName
    (
    char                            * const fullName,
    const char                      * const service,      /* may be NULL */
    const char                      * const regtype,
    const char                      * const domain
    );


/*********************************************************************************************
 *
 *   TXT Record Construction Functions
 *
 *********************************************************************************************/

/*
 * A typical calling sequence for TXT record construction is something like:
 *
 * Client allocates storage for TXTRecord data (e.g. declare buffer on the stack)
 * TXTRecordCreate();
 * TXTRecordSetValue();
 * TXTRecordSetValue();
 * TXTRecordSetValue();
 * ...
 * DNSServiceRegister( ... TXTRecordGetLength(), TXTRecordGetBytesPtr() ... );
 * TXTRecordDeallocate();
 * Explicitly deallocate storage for TXTRecord data (if not allocated on the stack)
 */


/* TXTRecordCreate()
 *
 * Creates a new empty TXTRecordRef referencing the specified storage.
 *
 * If the buffer parameter is NULL, or the specified storage size is not
 * large enough to hold a key subsequently added using TXTRecordSetValue(),
 * then additional memory will be added as needed using malloc().
 *
 * On some platforms, when memory is low, malloc() may fail. In this
 * case, TXTRecordSetValue() will return kDNSServiceErr_NoMemory, and this
 * error condition will need to be handled as appropriate by the caller.
 *
 * You can avoid the need to handle this error condition if you ensure
 * that the storage you initially provide is large enough to hold all
 * the key/value pairs that are to be added to the record.
 * The caller can precompute the exact length required for all of the
 * key/value pairs to be added, or simply provide a fixed-sized buffer
 * known in advance to be large enough.
 * A no-value (key-only) key requires  (1 + key length) bytes.
 * A key with empty value requires     (1 + key length + 1) bytes.
 * A key with non-empty value requires (1 + key length + 1 + value length).
 * For most applications, DNS-SD TXT records are generally
 * less than 100 bytes, so in most cases a simple fixed-sized
 * 256-byte buffer will be more than sufficient.
 * Recommended size limits for DNS-SD TXT Records are discussed in
 * <http://files.dns-sd.org/draft-cheshire-dnsext-dns-sd.txt>
 *
 * Note: When passing parameters to and from these TXT record APIs,
 * the key name does not include the '=' character. The '=' character
 * is the separator between the key and value in the on-the-wire
 * packet format; it is not part of either the key or the value.
 *
 * txtRecord:       A pointer to an uninitialized TXTRecordRef.
 *
 * bufferLen:       The size of the storage provided in the "buffer" parameter.
 *
 * buffer:          Optional caller-supplied storage used to hold the TXTRecord data.
 *                  This storage must remain valid for as long as
 *                  the TXTRecordRef.
 */

void DNSSD_API TXTRecordCreate
    (
    TXTRecordRef     *txtRecord,
    uint16_t         bufferLen,
    void             *buffer
    );


/* TXTRecordDeallocate()
 *
 * Releases any resources allocated in the course of preparing a TXT Record
 * using TXTRecordCreate()/TXTRecordSetValue()/TXTRecordRemoveValue().
 * Ownership of the buffer provided in TXTRecordCreate() returns to the client.
 *
 * txtRecord:           A TXTRecordRef initialized by calling TXTRecordCreate().
 *
 */

void DNSSD_API TXTRecordDeallocate
    (
    TXTRecordRef     *txtRecord
    );


/* TXTRecordSetValue()
 *
 * Adds a key (optionally with value) to a TXTRecordRef. If the "key" already
 * exists in the TXTRecordRef, then the current value will be replaced with
 * the new value.
 * Keys may exist in four states with respect to a given TXT record:
 *  - Absent (key does not appear at all)
 *  - Present with no value ("key" appears alone)
 *  - Present with empty value ("key=" appears in TXT record)
 *  - Present with non-empty value ("key=value" appears in TXT record)
 * For more details refer to "Data Syntax for DNS-SD TXT Records" in
 * <http://files.dns-sd.org/draft-cheshire-dnsext-dns-sd.txt>
 *
 * txtRecord:       A TXTRecordRef initialized by calling TXTRecordCreate().
 *
 * key:             A null-terminated string which only contains printable ASCII
 *                  values (0x20-0x7E), excluding '=' (0x3D). Keys should be
 *                  9 characters or fewer (not counting the terminating null).
 *
 * valueSize:       The size of the value.
 *
 * value:           Any binary value. For values that represent
 *                  textual data, UTF-8 is STRONGLY recommended.
 *                  For values that represent textual data, valueSize
 *                  should NOT include the terminating null (if any)
 *                  at the end of the string.
 *                  If NULL, then "key" will be added with no value.
 *                  If non-NULL but valueSize is zero, then "key=" will be
 *                  added with empty value.
 *
 * return value:    Returns kDNSServiceErr_NoError on success.
 *                  Returns kDNSServiceErr_Invalid if the "key" string contains
 *                  illegal characters.
 *                  Returns kDNSServiceErr_NoMemory if adding this key would
 *                  exceed the available storage.
 */

DNSServiceErrorType DNSSD_API TXTRecordSetValue
    (
    TXTRecordRef     *txtRecord,
    const char       *key,
    uint8_t          valueSize,        /* may be zero */
    const void       *value            /* may be NULL */
    );


/* TXTRecordRemoveValue()
 *
 * Removes a key from a TXTRecordRef. The "key" must be an
 * ASCII string which exists in the TXTRecordRef.
 *
 * txtRecord:       A TXTRecordRef initialized by calling TXTRecordCreate().
 *
 * key:             A key name which exists in the TXTRecordRef.
 *
 * return value:    Returns kDNSServiceErr_NoError on success.
 *                  Returns kDNSServiceErr_NoSuchKey if the "key" does not
 *                  exist in the TXTRecordRef.
 */

DNSServiceErrorType DNSSD_API TXTRecordRemoveValue
    (
    TXTRecordRef     *txtRecord,
    const char       *key
    );


/* TXTRecordGetLength()
 *
 * Allows you to determine the length of the raw bytes within a TXTRecordRef.
 *
 * txtRecord:       A TXTRecordRef initialized by calling TXTRecordCreate().
 *
 * return value:    Returns the size of the raw bytes inside a TXTRecordRef
 *                  which you can pass directly to DNSServiceRegister() or
 *                  to DNSServiceUpdateRecord().
 *                  Returns 0 if the TXTRecordRef is empty.
 */

uint16_t DNSSD_API TXTRecordGetLength
    (
    const TXTRecordRef *txtRecord
    );


/* TXTRecordGetBytesPtr()
 *
 * Allows you to retrieve a pointer to the raw bytes within a TXTRecordRef.
 *
 * txtRecord:       A TXTRecordRef initialized by calling TXTRecordCreate().
 *
 * return value:    Returns a pointer to the raw bytes inside the TXTRecordRef
 *                  which you can pass directly to DNSServiceRegister() or
 *                  to DNSServiceUpdateRecord().
 */

const void * DNSSD_API TXTRecordGetBytesPtr
    (
    const TXTRecordRef *txtRecord
    );


/*********************************************************************************************
 *
 *   TXT Record Parsing Functions
 *
 *********************************************************************************************/

/*
 * A typical calling sequence for TXT record parsing is something like:
 *
 * Receive TXT record data in DNSServiceResolve() callback
 * if (TXTRecordContainsKey(txtLen, txtRecord, "key")) then do something
 * val1ptr = TXTRecordGetValuePtr(txtLen, txtRecord, "key1", &len1);
 * val2ptr = TXTRecordGetValuePtr(txtLen, txtRecord, "key2", &len2);
 * ...
 * memcpy(myval1, val1ptr, len1);
 * memcpy(myval2, val2ptr, len2);
 * ...
 * return;
 *
 * If you wish to retain the values after return from the DNSServiceResolve()
 * callback, then you need to copy the data to your own storage using memcpy()
 * or similar, as shown in the example above.
 *
 * If for some reason you need to parse a TXT record you built yourself
 * using the TXT record construction functions above, then you can do
 * that using TXTRecordGetLength and TXTRecordGetBytesPtr calls:
 * TXTRecordGetValue(TXTRecordGetLength(x), TXTRecordGetBytesPtr(x), key, &len);
 *
 * Most applications only fetch keys they know about from a TXT record and
 * ignore the rest.
 * However, some debugging tools wish to fetch and display all keys.
 * To do that, use the TXTRecordGetCount() and TXTRecordGetItemAtIndex() calls.
 */

/* TXTRecordContainsKey()
 *
 * Allows you to determine if a given TXT Record contains a specified key.
 *
 * txtLen:          The size of the received TXT Record.
 *
 * txtRecord:       Pointer to the received TXT Record bytes.
 *
 * key:             A null-terminated ASCII string containing the key name.
 *
 * return value:    Returns 1 if the TXT Record contains the specified key.
 *                  Otherwise, it returns 0.
 */

int DNSSD_API TXTRecordContainsKey
    (
    uint16_t         txtLen,
    const void       *txtRecord,
    const char       *key
    );


/* TXTRecordGetValuePtr()
 *
 * Allows you to retrieve the value for a given key from a TXT Record.
 *
 * txtLen:          The size of the received TXT Record
 *
 * txtRecord:       Pointer to the received TXT Record bytes.
 *
 * key:             A null-terminated ASCII string containing the key name.
 *
 * valueLen:        On output, will be set to the size of the "value" data.
 *
 * return value:    Returns NULL if the key does not exist in this TXT record,
 *                  or exists with no value (to differentiate between
 *                  these two cases use TXTRecordContainsKey()).
 *                  Returns pointer to location within TXT Record bytes
 *                  if the key exists with empty or non-empty value.
 *                  For empty value, valueLen will be zero.
 *                  For non-empty value, valueLen will be length of value data.
 */

const void * DNSSD_API TXTRecordGetValuePtr
    (
    uint16_t         txtLen,
    const void       *txtRecord,
    const char       *key,
    uint8_t          *valueLen
    );


/* TXTRecordGetCount()
 *
 * Returns the number of keys stored in the TXT Record. The count
 * can be used with TXTRecordGetItemAtIndex() to iterate through the keys.
 *
 * txtLen:          The size of the received TXT Record.
 *
 * txtRecord:       Pointer to the received TXT Record bytes.
 *
 * return value:    Returns the total number of keys in the TXT Record.
 *
 */

uint16_t DNSSD_API TXTRecordGetCount
    (
    uint16_t         txtLen,
    const void       *txtRecord
    );


/* TXTRecordGetItemAtIndex()
 *
 * Allows you to retrieve a key name and value pointer, given an index into
 * a TXT Record. Legal index values range from zero to TXTRecordGetCount()-1.
 * It's also possible to iterate through keys in a TXT record by simply
 * calling TXTRecordGetItemAtIndex() repeatedly, beginning with index zero
 * and increasing until TXTRecordGetItemAtIndex() returns kDNSServiceErr_Invalid.
 *
 * On return:
 * For keys with no value, *value is set to NULL and *valueLen is zero.
 * For keys with empty value, *value is non-NULL and *valueLen is zero.
 * For keys with non-empty value, *value is non-NULL and *valueLen is non-zero.
 *
 * txtLen:          The size of the received TXT Record.
 *
 * txtRecord:       Pointer to the received TXT Record bytes.
 *
 * itemIndex:       An index into the TXT Record.
 *
 * keyBufLen:       The size of the string buffer being supplied.
 *
 * key:             A string buffer used to store the key name.
 *                  On return, the buffer contains a null-terminated C string
 *                  giving the key name. DNS-SD TXT keys are usually
 *                  9 characters or fewer. To hold the maximum possible
 *                  key name, the buffer should be 256 bytes long.
 *
 * valueLen:        On output, will be set to the size of the "value" data.
 *
 * value:           On output, *value is set to point to location within TXT
 *                  Record bytes that holds the value data.
 *
 * return value:    Returns kDNSServiceErr_NoError on success.
 *                  Returns kDNSServiceErr_NoMemory if keyBufLen is too short.
 *                  Returns kDNSServiceErr_Invalid if index is greater than
 *                  TXTRecordGetCount()-1.
 */

DNSServiceErrorType DNSSD_API TXTRecordGetItemAtIndex
    (
    uint16_t         txtLen,
    const void       *txtRecord,
    uint16_t         itemIndex,
    uint16_t         keyBufLen,
    char             *key,
    uint8_t          *valueLen,
    const void       **value
    );

#if _DNS_SD_LIBDISPATCH
/*
* DNSServiceSetDispatchQueue
*
* Allows you to schedule a DNSServiceRef on a serial dispatch queue for receiving asynchronous
* callbacks.  It's the clients responsibility to ensure that the provided dispatch queue is running.
*
* A typical application that uses CFRunLoopRun or dispatch_main on its main thread will
* usually schedule DNSServiceRefs on its main queue (which is always a serial queue)
* using "DNSServiceSetDispatchQueue(sdref, dispatch_get_main_queue());"
*
* If there is any error during the processing of events, the application callback will
* be called with an error code. For shared connections, each subordinate DNSServiceRef
* will get its own error callback. Currently these error callbacks only happen
* if the mDNSResponder daemon is manually terminated or crashes, and the error
* code in this case is kDNSServiceErr_ServiceNotRunning. The application must call
* DNSServiceRefDeallocate to free the DNSServiceRef when it gets such an error code.
* These error callbacks are rare and should not normally happen on customer machines,
* but application code should be written defensively to handle such error callbacks
* gracefully if they occur.
*
* After using DNSServiceSetDispatchQueue on a DNSServiceRef, calling DNSServiceProcessResult
* on the same DNSServiceRef will result in undefined behavior and should be avoided.
*
* Once the application successfully schedules a DNSServiceRef on a serial dispatch queue using
* DNSServiceSetDispatchQueue, it cannot remove the DNSServiceRef from the dispatch queue, or use
* DNSServiceSetDispatchQueue a second time to schedule the DNSServiceRef onto a different serial dispatch
* queue. Once scheduled onto a dispatch queue a DNSServiceRef will deliver events to that queue until
* the application no longer requires that operation and terminates it using DNSServiceRefDeallocate.
*
* service:         DNSServiceRef that was allocated and returned to the application, when the
*                  application calls one of the DNSService API.
*
* queue:           dispatch queue where the application callback will be scheduled
*
* return value:    Returns kDNSServiceErr_NoError on success.
*                  Returns kDNSServiceErr_NoMemory if it cannot create a dispatch source
*                  Returns kDNSServiceErr_BadParam if the service param is invalid or the
*                  queue param is invalid
*/

DNSServiceErrorType DNSSD_API DNSServiceSetDispatchQueue
  (
  DNSServiceRef service,
  dispatch_queue_t queue
  );
#endif //_DNS_SD_LIBDISPATCH

#ifdef __APPLE_API_PRIVATE

#define kDNSServiceCompPrivateDNS   "PrivateDNS"
#define kDNSServiceCompMulticastDNS "MulticastDNS"

#endif //__APPLE_API_PRIVATE

/* Some C compiler cleverness. We can make the compiler check certain things for us,
 * and report errors at compile-time if anything is wrong. The usual way to do this would
 * be to use a run-time "if" statement or the conventional run-time "assert" mechanism, but
 * then you don't find out what's wrong until you run the software. This way, if the assertion
 * condition is false, the array size is negative, and the complier complains immediately.
 */

struct CompileTimeAssertionChecks_DNS_SD
    {
    char assert0[(sizeof(union _TXTRecordRef_t) == 16) ? 1 : -1];
    };

#endif  /* _DNS_SD_H */
