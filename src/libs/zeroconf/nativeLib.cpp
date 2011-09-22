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

#include <QtCore/QDebug>
#include <QtCore/QLibrary>
#include <QtCore/QString>
#include <QtCore/QStringList>

#ifndef NO_NATIVE_LIB

#ifdef Q_OS_MACX
#define ZCONF_STATIC_LINKING
#endif

#ifdef ZCONF_STATIC_LINKING
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
typedef uint16_t (DNSSD_API *TxtRecordGetCountPtr)(uint16_t txtLen, const void *txtRecord);
typedef DNSServiceErrorType (DNSSD_API *TxtRecordGetItemAtIndexPtr)(uint16_t txtLen, const void *txtRecord,
                                                                    uint16_t itemIndex, uint16_t keyBufLen,
                                                                    char *key, uint8_t *valueLen, const void **value);
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
private:
    RefDeallocatePtr m_refDeallocate;
    ResolvePtr m_resolve;
    QueryRecordPtr m_queryRecord;
    GetAddrInfoPtr m_getAddrInfo;
    TxtRecordGetCountPtr m_txtRecordGetCount;
    TxtRecordGetItemAtIndexPtr m_txtRecordGetItemAtIndex;
    ReconfirmRecordPtr m_reconfirmRecord;
    BrowsePtr m_browse;
    GetPropertyPtr m_getProperty;
    ProcessResultPtr m_processResult;
    CreateConnectionPtr m_createConnection;
    RefSockFDPtr m_refSockFD;
    QLibrary nativeLib;
public:

    NativeZConfLib(QString libName = QLatin1String("dns_sd"), ZConfLib *fallBack = 0) : ZConfLib(fallBack), nativeLib(libName)
    {
#ifndef ZCONF_STATIC_LINKING
        // dynamic linking
        if (!nativeLib.load()) {
            qDebug() << "NativeZConfLib could not load native library";
        }
        m_refDeallocate = reinterpret_cast<RefDeallocatePtr>(nativeLib.resolve("DNSServiceRefDeallocate"));
        m_resolve = reinterpret_cast<ResolvePtr>(nativeLib.resolve("DNSServiceResolve"));
        m_queryRecord = reinterpret_cast<QueryRecordPtr>(nativeLib.resolve("DNSServiceQueryRecord"));
        m_getAddrInfo = reinterpret_cast<GetAddrInfoPtr>(nativeLib.resolve("DNSServiceGetAddrInfo"));
        m_txtRecordGetCount = reinterpret_cast<TxtRecordGetCountPtr>(nativeLib.resolve("TXTRecordGetCount"));
        m_txtRecordGetItemAtIndex = reinterpret_cast<TxtRecordGetItemAtIndexPtr>(nativeLib.resolve("TXTRecordGetItemAtIndex")) ;
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
        m_txtRecordGetCount = reinterpret_cast<TxtRecordGetCountPtr>(&TXTRecordGetCount);
        m_txtRecordGetItemAtIndex = reinterpret_cast<TxtRecordGetItemAtIndexPtr>(&TXTRecordGetItemAtIndex) ;
        m_reconfirmRecord = reinterpret_cast<ReconfirmRecordPtr>(&DNSServiceReconfirmRecord);
        m_browse = reinterpret_cast<BrowsePtr>(&DNSServiceBrowse);
        m_getProperty = reinterpret_cast<GetPropertyPtr>(&DNSServiceGetProperty);
        m_processResult = reinterpret_cast<ProcessResultPtr>(&DNSServiceProcessResult) ;
        m_createConnection = reinterpret_cast<CreateConnectionPtr>(&DNSServiceCreateConnection);
        m_refSockFD = reinterpret_cast<RefSockFDPtr>(&DNSServiceRefSockFD);
#endif
        if (m_refDeallocate == 0) qDebug() << QLatin1String("NativeZConfLib.m_refDeallocate == 0");
        if (m_resolve == 0) qDebug() << QLatin1String("NativeZConfLib.m_resolve == 0");
        if (m_queryRecord == 0) qDebug() << QLatin1String("NativeZConfLib.m_queryRecord == 0");
        if (m_getAddrInfo == 0) qDebug() << QLatin1String("NativeZConfLib.m_getAddrInfo == 0");
        if (m_txtRecordGetCount == 0) qDebug() << QLatin1String("NativeZConfLib.m_txtRecordGetCount == 0");
        if (m_txtRecordGetItemAtIndex == 0) qDebug() << QLatin1String("NativeZConfLib.m_txtRecordGetItemAtIndex == 0");
        if (m_reconfirmRecord == 0) qDebug() << QLatin1String("NativeZConfLib.m_reconfirmRecord == 0");
        if (m_browse == 0) qDebug() << QLatin1String("NativeZConfLib.m_browse == 0");
        if (m_getProperty == 0) qDebug() << QLatin1String("NativeZConfLib.m_getProperty == 0");
        if (m_processResult == 0) qDebug() << QLatin1String("NativeZConfLib.m_processResult == 0");
        if (m_createConnection == 0) qDebug() << QLatin1String("NativeZConfLib.m_createConnection == 0");
        if (m_refSockFD == 0) qDebug() << QLatin1String("NativeZConfLib.m_refSockFD == 0");
    }

    virtual ~NativeZConfLib() {
    }

    virtual QString name(){
        return QString::fromUtf8("NativeZeroConfLib@%1").arg(size_t(this),0,16);
    }

    // virtual bool tryStartDaemon();
    virtual void refDeallocate(DNSServiceRef sdRef) {
        if (m_refDeallocate == 0) return;
        m_refDeallocate(sdRef);
    }

    virtual DNSServiceErrorType resolve(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                        uint32_t interfaceIndex, const char *name,
                                        const char *regtype, const char *domain,
                                        DNSServiceResolveReply callBack, void *context)
    {
        if (m_resolve == 0) return kDNSServiceErr_Unsupported;
        return m_resolve(sdRef, flags, interfaceIndex, name, regtype, domain, callBack, context);
    }

    virtual DNSServiceErrorType queryRecord(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                            uint32_t interfaceIndex, const char *fullname,
                                            uint16_t rrtype, uint16_t rrclass,
                                            DNSServiceQueryRecordReply callBack, void *context)
    {
        if (m_queryRecord == 0) return kDNSServiceErr_Unsupported;
        return m_queryRecord(sdRef, flags, interfaceIndex, fullname,
                             rrtype, rrclass, callBack, context);
    }

    virtual DNSServiceErrorType getAddrInfo(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                            uint32_t interfaceIndex, DNSServiceProtocol protocol,
                                            const char *hostname, DNSServiceGetAddrInfoReply callBack,
                                            void *context)
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
                callBack(*sdRef, kDNSServiceFlagsAdd, interfaceIndex, kDNSServiceErr_NoError,
                         hostname, ansAtt->ai_addr, longTTL, context);
            }
            freeaddrinfo(ans);
            return kDNSServiceErr_NoError;
#else
            return kDNSServiceErr_Unsupported;
#endif
        }
        return m_getAddrInfo(sdRef, flags, interfaceIndex, protocol,
                             hostname, callBack, context);
    }

    virtual uint16_t txtRecordGetCount(uint16_t txtLen, const void *txtRecord)
    {
        if (m_txtRecordGetCount == 0) return 0;
        return m_txtRecordGetCount(txtLen, txtRecord);
    }

    virtual DNSServiceErrorType txtRecordGetItemAtIndex(uint16_t txtLen, const void *txtRecord,
                                                        uint16_t itemIndex, uint16_t keyBufLen,
                                                        char *key, uint8_t *valueLen, const void **value)
    {
        if (m_txtRecordGetItemAtIndex == 0) return kDNSServiceErr_Unsupported;
        return m_txtRecordGetItemAtIndex(txtLen, txtRecord, itemIndex, keyBufLen,
                                         key, valueLen, value);
    }

    virtual DNSServiceErrorType reconfirmRecord(DNSServiceFlags flags, uint32_t interfaceIndex,
                                                const char *fullname, uint16_t rrtype,
                                                uint16_t rrclass, uint16_t rdlen, const void *rdata)
    {
        if (m_reconfirmRecord == 0) return kDNSServiceErr_Unsupported;
        return m_reconfirmRecord(flags, interfaceIndex, fullname, rrtype,
                                 rrclass, rdlen, rdata);
    }

    virtual DNSServiceErrorType browse(DNSServiceRef *sdRef, DNSServiceFlags flags,
                                       uint32_t interfaceIndex, const char *regtype,
                                       const char *domain, DNSServiceBrowseReply callBack,
                                       void *context)
    {
        if (m_browse == 0) return kDNSServiceErr_Unsupported;
        return m_browse(sdRef, flags, interfaceIndex, regtype, domain, callBack, context);
    }

    virtual DNSServiceErrorType getProperty(const char *property,  // Requested property (i.e. kDNSServiceProperty_DaemonVersion)
                                            void       *result,    // Pointer to place to store result
                                            uint32_t   *size       // size of result location
                                            )
    {
        if (m_getProperty == 0)
            return kDNSServiceErr_Unsupported;
        return m_getProperty(property, result, size);
    }

    virtual DNSServiceErrorType processResult(DNSServiceRef sdRef) {
        if (m_processResult == 0) return kDNSServiceErr_Unsupported;
        return m_processResult(sdRef);
    }

    virtual DNSServiceErrorType createConnection(DNSServiceRef *sdRef) {
        if (m_createConnection == 0) return kDNSServiceErr_Unsupported;
        return m_createConnection(sdRef);
    }

    virtual int refSockFD(DNSServiceRef sdRef) {
        if (m_refSockFD == 0) return kDNSServiceErr_Unsupported;
        return m_refSockFD(sdRef);
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

