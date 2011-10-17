/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "servicebrowser.h"
#include "servicebrowser_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QLibrary>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <errno.h>

#ifndef NO_NATIVE_LIB

#ifdef Q_OS_MACX
#define ZCONF_MDNS_STATIC_LINKING
#endif

#ifdef ZCONF_MDNS_STATIC_LINKING
extern "C" {
#include "dns_sd_funct.h"
}
#endif

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace ZeroConf {
namespace Internal {

extern "C" {
typedef void (DNSSD_API *RefDeallocatePtr)(DNSServiceRef sdRef);
typedef DNSServiceErrorType (DNSSD_API *ResolvePtr)(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                                    uint32_t interfaceIndex, const char *name,
                                                    const char *regtype, const char *domain,
                                                    DNSServiceResolveReply callBack, void *context);
typedef DNSServiceErrorType (DNSSD_API *QueryRecordPtr)(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                                        uint32_t interfaceIndex, const char *fullname,
                                                        uint16_t rrtype, uint16_t rrclass,
                                                        DNSServiceQueryRecordReply callBack, void *context);
typedef DNSServiceErrorType (DNSSD_API *GetAddrInfoPtr)(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                                        uint32_t interfaceIndex, DNSServiceProtocol protocol,
                                                        const char *hostname, DNSServiceGetAddrInfoReply callBack,
                                                        void *context);
typedef DNSServiceErrorType (DNSSD_API *ReconfirmRecordPtr)(DNSServiceFlags flags, uint32_t interfaceIndex,
                                                            const char *fullname, uint16_t rrtype,
                                                            uint16_t rrclass, uint16_t rdlen, const void *rdata);
typedef DNSServiceErrorType (DNSSD_API *BrowsePtr)(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                                   uint32_t interfaceIndex, const char *regtype, const char *domain,
                                                   DNSServiceBrowseReply callBack, void *context);
typedef DNSServiceErrorType (DNSSD_API *GetPropertyPtr)(const char *property, void *result, uint32_t *size);
typedef DNSServiceErrorType (DNSSD_API *ProcessResultPtr)(DNSServiceRef sdRef);
typedef DNSServiceErrorType (DNSSD_API *CreateConnectionPtr)(DNSServiceRef *sdRef);
typedef int (DNSSD_API *RefSockFDPtr)(DNSServiceRef sdRef);
}

// represents a zero conf library exposing the dns-sd interface
class NativeZConfLib : public ZConfLib{
    Q_DECLARE_TR_FUNCTIONS(ZeroConf)
private:
    RefDeallocatePtr m_refDeallocate;
    ResolvePtr m_resolve;
    QueryRecordPtr m_queryRecord;
    GetAddrInfoPtr m_getAddrInfo;
    ReconfirmRecordPtr m_reconfirmRecord;
    BrowsePtr m_browse;
    GetPropertyPtr m_getProperty;
    ProcessResultPtr m_processResult;
    CreateConnectionPtr m_createConnection;
    RefSockFDPtr m_refSockFD;
    QLibrary nativeLib;
public:
    enum {
        // Note: the select() implementation on Windows (Winsock2) fails with any timeout much larger than this
        LONG_TIME = 100000000
    };

    NativeZConfLib(QString libName = QLatin1String("dns_sd"), ZConfLib *fallBack = 0) : ZConfLib(fallBack), nativeLib(libName)
    {
#ifndef ZCONF_MDNS_STATIC_LINKING
        // dynamic linking
        if (!nativeLib.load()) {
            m_isOk = false;
            m_errorMsg=tr("NativeZConfLib could not load native library");
        }
        m_refDeallocate = reinterpret_cast<RefDeallocatePtr>(nativeLib.resolve("DNSServiceRefDeallocate"));
        m_resolve = reinterpret_cast<ResolvePtr>(nativeLib.resolve("DNSServiceResolve"));
        m_queryRecord = reinterpret_cast<QueryRecordPtr>(nativeLib.resolve("DNSServiceQueryRecord"));
        m_getAddrInfo = reinterpret_cast<GetAddrInfoPtr>(nativeLib.resolve("DNSServiceGetAddrInfo"));
        m_reconfirmRecord = reinterpret_cast<ReconfirmRecordPtr>(nativeLib.resolve("DNSServiceReconfirmRecord"));
        m_browse = reinterpret_cast<BrowsePtr>(nativeLib.resolve("DNSServiceBrowse"));
        m_getProperty = reinterpret_cast<GetPropertyPtr>(nativeLib.resolve("DNSServiceGetProperty"));
        m_processResult = reinterpret_cast<ProcessResultPtr>(nativeLib.resolve("DNSServiceProcessResult")) ;
        m_createConnection = reinterpret_cast<CreateConnectionPtr>(nativeLib.resolve("DNSServiceCreateConnection"));
        m_refSockFD = reinterpret_cast<RefSockFDPtr>(nativeLib.resolve("DNSServiceRefSockFD"));
#else
        // static linking
        m_refDeallocate = reinterpret_cast<RefDeallocatePtr>(&DNSServiceRefDeallocate);
        m_resolve = reinterpret_cast<ResolvePtr>(&DNSServiceResolve);
        m_queryRecord = reinterpret_cast<QueryRecordPtr>(DNSServiceQueryRecord);
        m_getAddrInfo = reinterpret_cast<GetAddrInfoPtr>(&DNSServiceGetAddrInfo);
        m_reconfirmRecord = reinterpret_cast<ReconfirmRecordPtr>(&DNSServiceReconfirmRecord);
        m_browse = reinterpret_cast<BrowsePtr>(&DNSServiceBrowse);
        m_getProperty = reinterpret_cast<GetPropertyPtr>(&DNSServiceGetProperty);
        m_processResult = reinterpret_cast<ProcessResultPtr>(&DNSServiceProcessResult) ;
        m_createConnection = reinterpret_cast<CreateConnectionPtr>(&DNSServiceCreateConnection);
        m_refSockFD = reinterpret_cast<RefSockFDPtr>(&DNSServiceRefSockFD);
#endif
        if (DEBUG_ZEROCONF){
            if (m_refDeallocate == 0) qDebug() << QLatin1String("NativeZConfLib.m_refDeallocate == 0");
            if (m_resolve == 0) qDebug() << QLatin1String("NativeZConfLib.m_resolve == 0");
            if (m_queryRecord == 0) qDebug() << QLatin1String("NativeZConfLib.m_queryRecord == 0");
            if (m_getAddrInfo == 0) qDebug() << QLatin1String("NativeZConfLib.m_getAddrInfo == 0");
            if (m_reconfirmRecord == 0) qDebug() << QLatin1String("NativeZConfLib.m_reconfirmRecord == 0");
            if (m_browse == 0) qDebug() << QLatin1String("NativeZConfLib.m_browse == 0");
            if (m_getProperty == 0) qDebug() << QLatin1String("NativeZConfLib.m_getProperty == 0");
            if (m_processResult == 0) qDebug() << QLatin1String("NativeZConfLib.m_processResult == 0");
            if (m_createConnection == 0) qDebug() << QLatin1String("NativeZConfLib.m_createConnection == 0");
            if (m_refSockFD == 0) qDebug() << QLatin1String("NativeZConfLib.m_refSockFD == 0");
        }
    }

    ~NativeZConfLib() {
    }

    QString name(){
        return QString::fromUtf8("NativeZeroConfLib@%1").arg(size_t(this),0,16);
    }

    // bool tryStartDaemon();
    void refDeallocate(DNSServiceRef sdRef) {
        if (m_refDeallocate == 0) return;
        m_refDeallocate(sdRef);
    }

    void browserDeallocate(BrowserRef *bRef) {
        if (m_refDeallocate == 0) return;
        if (bRef) {
            m_refDeallocate(*reinterpret_cast<DNSServiceRef *>(bRef));
            *bRef=0;
        }
    }

    void stopConnection(ConnectionRef cRef)
    {
        int sock = refSockFD(cRef);
        if (sock>0)
            shutdown(sock,SHUT_RDWR);
    }

    void destroyConnection(ConnectionRef *sdRef) {
        if (m_refDeallocate == 0) return;
        if (sdRef) {
            m_refDeallocate(*reinterpret_cast<DNSServiceRef *>(sdRef));
            *sdRef=0;
        }
    }

    DNSServiceErrorType resolve(ConnectionRef cRef, DNSServiceRef *sdRef,
                                uint32_t interfaceIndex, const char *name,
                                const char *regtype, const char *domain,
                                ServiceGatherer *gatherer)
    {
        if (m_resolve == 0) return kDNSServiceErr_Unsupported;
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return m_resolve(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout,
                         interfaceIndex, name, regtype, domain, &cServiceResolveReply, gatherer
                         );
    }

    DNSServiceErrorType queryRecord(ConnectionRef cRef, DNSServiceRef *sdRef,
                                    uint32_t interfaceIndex, const char *fullname,
                                    ServiceGatherer *gatherer)
    {
        if (m_queryRecord == 0) return kDNSServiceErr_Unsupported;
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return m_queryRecord(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout,
                             interfaceIndex, fullname,
                             kDNSServiceType_TXT, kDNSServiceClass_IN, &cTxtRecordReply, gatherer);
    }

    DNSServiceErrorType getAddrInfo(ConnectionRef cRef, DNSServiceRef *sdRef,
                                    uint32_t interfaceIndex, DNSServiceProtocol protocol,
                                    const char *hostname, ServiceGatherer *gatherer)
    {
        enum { longTTL = 100 };
        if (m_getAddrInfo == 0) {
#ifdef Q_OS_UNIX
            // try to use getaddrinfo (for example on linux with avahi)
            struct addrinfo req, *ans; int err;
            memset(&req,0,sizeof(req));
            req.ai_flags = 0;
            req.ai_family = AF_UNSPEC;
            req.ai_socktype = SOCK_STREAM;
            req.ai_protocol = 0;
            if ((err = getaddrinfo(hostname, 0, &req, &ans)) != 0) {
                qDebug() << "getaddrinfo for " << hostname << " failed with " << gai_strerror(err);
                return kDNSServiceErr_Unsupported; // use another error here???
            }
            for (struct addrinfo *ansAtt = ans; ansAtt != 0; ansAtt = ansAtt->ai_next){
                gatherer->addrReply(kDNSServiceFlagsAdd, kDNSServiceErr_NoError,
                                    hostname, ansAtt->ai_addr, longTTL);
            }
            freeaddrinfo(ans);
            return kDNSServiceErr_NoError;
#else
            return kDNSServiceErr_Unsupported;
#endif
        }
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return m_getAddrInfo(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout,
                             interfaceIndex, protocol, hostname, &cAddrReply, gatherer);
    }

    DNSServiceErrorType reconfirmRecord(ConnectionRef /*cRef*/, uint32_t /*interfaceIndex*/,
                                                const char * /*name*/, const char * /*type*/, const char * /*domain*/,
                                                const char * /*fullname*/)
    {
        if (m_reconfirmRecord == 0) return kDNSServiceErr_Unsupported;
        // reload and force update with in the callback with
        // m_reconfirmRecord(flags, interfaceIndex, fullname, rrtype,
        //                         rrclass, rdlen, rdata);
        return kDNSServiceErr_Unsupported;
    }

    DNSServiceErrorType browse(ConnectionRef cRef, BrowserRef *bRef,
                               uint32_t interfaceIndex, const char *regtype,
                               const char *domain, ServiceBrowserPrivate *browser)
    {
        if (m_browse == 0) return kDNSServiceErr_Unsupported;
        DNSServiceRef *sdRef = reinterpret_cast<DNSServiceRef *>(bRef);
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return m_browse(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable,
                        interfaceIndex, regtype, domain, &cBrowseReply, browser);
    }

    DNSServiceErrorType getProperty(const char *property,  // Requested property (i.e. kDNSServiceProperty_DaemonVersion)
                                    void       *result,    // Pointer to place to store result
                                    uint32_t   *size       // size of result location
                                    )
    {
        if (m_getProperty == 0)
            return kDNSServiceErr_Unsupported;
        return m_getProperty(property, result, size);
    }

    ProcessStatus processResult(ConnectionRef sdRef) {
        if (m_processResult == 0)
            return ProcessedFailure;
        if (m_processResult(reinterpret_cast<DNSServiceRef>(sdRef)) != kDNSServiceErr_NoError)
            return ProcessedError;
        return ProcessedOk;
    }

    // alternative processResult implementation using select
    DNSServiceErrorType  processResultSelect(ConnectionRef cRef)
    {
#if _DNS_SD_LIBDISPATCH
        {
            main_queue = dispatch_get_main_queue();
            if (mainRef)  DNSServiceSetDispatchQueue(mainRef, main_queue);
            dispatch_main();
        }
#else
        {
            if (m_processResult == 0) return kDNSServiceErr_Unsupported;
            int dns_sd_fd  = (cRef?refSockFD(cRef):-1);
            int nfds = dns_sd_fd + 1;
            fd_set readfds;
            struct timeval tv;
            int result;
            while (true) {
                dns_sd_fd  = (cRef?refSockFD(cRef):-1);
                if (dns_sd_fd < 0)
                    return false;
                nfds = dns_sd_fd + 1;
                FD_ZERO(&readfds);
                FD_SET(dns_sd_fd , &readfds);

                tv.tv_sec  = LONG_TIME;
                tv.tv_usec = 0;

                result = select(nfds, &readfds, (fd_set *)NULL, (fd_set *)NULL, &tv);
                if (result > 0) {
                    if (FD_ISSET(dns_sd_fd , &readfds)) {
                        m_processResult(reinterpret_cast<DNSServiceRef>(cRef));
                        return true;
                    }
                } else if (result == 0) {
                    // we are idle... could do something productive... :)
                } else if (errno != EINTR) {
                    qDebug() << "select() returned " << result << " errno " << errno << strerror(errno);
                    return false;
                }
            }
        }
#endif
    }

    DNSServiceErrorType createConnection(ConnectionRef *sdRef) {
        if (m_createConnection == 0) return kDNSServiceErr_Unsupported;
        return m_createConnection(reinterpret_cast<DNSServiceRef *>(sdRef));
    }

    int refSockFD(ConnectionRef sdRef) {
        if (m_refSockFD == 0) return kDNSServiceErr_Unsupported;
        return m_refSockFD(reinterpret_cast<DNSServiceRef>(sdRef));
    }
};

ZConfLib *ZConfLib::createNativeLib(const QString &libName, ZConfLib *fallback) {
    return new NativeZConfLib(libName, fallback);
    return fallback;
}
} // namespace Internal
} // namespace ZeroConf

#else // no native lib

namespace ZeroConf {
namespace Internal {

ZConfLib *ZConfLib::createNativeLib(const QString &/*extraPaths*/, ZConfLib * fallback) {
    return fallback;
}

} // namespace Internal
} // namespace ZeroConf
#endif

