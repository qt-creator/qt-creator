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

#ifdef Q_OS_LINUX
#define EMBEDDED_LIB
#endif

#ifdef Q_OS_WIN
# include <Winsock2.h>
#endif
#include "servicebrowser_p.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

#ifdef EMBEDDED_LIB
#include "embed/dnssd_ipc.c"
#include "embed/dnssd_clientlib.c"
#include "embed/dnssd_clientstub.c"
#ifdef Q_OS_WIN
#include "embed/DebugServices.c"
#endif

namespace ZeroConf {
namespace Internal {
// represents a zero conf library exposing the dns-sd interface
class EmbeddedZConfLib : public ZConfLib{
public:
    QString daemonPath;

    EmbeddedZConfLib(const QString &daemonPath, ZConfLib *fallBack = 0) : ZConfLib(fallBack), daemonPath(daemonPath)
    { }

    virtual ~EmbeddedZConfLib() {
    }

    virtual QString name(){
        return QString::fromUtf8("EmbeddedZeroConfLib@%1").arg(size_t(this),0,16);
    }

    // virtual bool tryStartDaemon();

    virtual void refDeallocate(DNSServiceRef sdRef){
        embeddedLib::DNSServiceRefDeallocate(sdRef);
    }

    virtual DNSServiceErrorType resolve(DNSServiceRef                       *sdRef,
                                        DNSServiceFlags                     flags,
                                        uint32_t                            interfaceIndex,
                                        const char                          *name,
                                        const char                          *regtype,
                                        const char                          *domain,
                                        DNSServiceResolveReply              callBack,
                                        void                                *context)
    {
        return embeddedLib::DNSServiceResolve(sdRef, flags, interfaceIndex, name, regtype, domain, callBack, context);
    }

    virtual DNSServiceErrorType queryRecord(DNSServiceRef                       *sdRef,
                                            DNSServiceFlags                     flags,
                                            uint32_t                            interfaceIndex,
                                            const char                          *fullname,
                                            uint16_t                            rrtype,
                                            uint16_t                            rrclass,
                                            DNSServiceQueryRecordReply          callBack,
                                            void                                *context)
    {
        return embeddedLib::DNSServiceQueryRecord(sdRef, flags, interfaceIndex, fullname,
                                                  rrtype, rrclass, callBack, context);
    }

    virtual DNSServiceErrorType getAddrInfo(DNSServiceRef                    *sdRef,
                                            DNSServiceFlags                  flags,
                                            uint32_t                         interfaceIndex,
                                            DNSServiceProtocol               protocol,
                                            const char                       *hostname,
                                            DNSServiceGetAddrInfoReply       callBack,
                                            void                             *context)
    {
        return embeddedLib::DNSServiceGetAddrInfo(sdRef, flags, interfaceIndex, protocol,
                                                  hostname, callBack, context);
    }

    virtual uint16_t txtRecordGetCount(uint16_t         txtLen,
                                       const void       *txtRecord)
    {
        return embeddedLib::TXTRecordGetCount(txtLen, txtRecord);
    }

    virtual DNSServiceErrorType txtRecordGetItemAtIndex(uint16_t         txtLen,
                                                        const void       *txtRecord,
                                                        uint16_t         itemIndex,
                                                        uint16_t         keyBufLen,
                                                        char             *key,
                                                        uint8_t          *valueLen,
                                                        const void       **value)
    {
        return embeddedLib::TXTRecordGetItemAtIndex(txtLen, txtRecord, itemIndex, keyBufLen,
                                                    key, valueLen, value);
    }

    virtual DNSServiceErrorType reconfirmRecord(DNSServiceFlags                    flags,
                                                uint32_t                           interfaceIndex,
                                                const char                         *fullname,
                                                uint16_t                           rrtype,
                                                uint16_t                           rrclass,
                                                uint16_t                           rdlen,
                                                const void                         *rdata)
    {
        return embeddedLib::DNSServiceReconfirmRecord(flags, interfaceIndex, fullname, rrtype,
                                                      rrclass, rdlen, rdata);
    }

    virtual DNSServiceErrorType browse(DNSServiceRef                       *sdRef,
                                       DNSServiceFlags                     flags,
                                       uint32_t                            interfaceIndex,
                                       const char                          *regtype,
                                       const char                          *domain,    /* may be NULL */
                                       DNSServiceBrowseReply               callBack,
                                       void                                *context    /* may be NULL */
                                       )
    {
        return embeddedLib::DNSServiceBrowse(sdRef, flags, interfaceIndex, regtype, domain, callBack, context);
    }

    virtual DNSServiceErrorType getProperty(const char *property,  /* Requested property (i.e. kDNSServiceProperty_DaemonVersion) */
                                            void       *result,    /* Pointer to place to store result */
                                            uint32_t   *size       /* size of result location */
                                            )
    {
        return embeddedLib::DNSServiceGetProperty(property, result, size);
    }

    virtual DNSServiceErrorType processResult(DNSServiceRef sdRef){
        return embeddedLib::DNSServiceProcessResult(sdRef);
    }

    virtual DNSServiceErrorType createConnection(DNSServiceRef *sdRef){
        return embeddedLib::DNSServiceCreateConnection(sdRef);
    }

    virtual int refSockFD(DNSServiceRef sdRef){
        return embeddedLib::DNSServiceRefSockFD(sdRef);
    }
};

ZConfLib *ZConfLib::createEmbeddedLib(const QString &daemonPath, ZConfLib *fallback){
    return new EmbeddedZConfLib(daemonPath, fallback);
}
} // namespace Internal
} // namespace ZeroConf

#else // no embedded lib

namespace ZeroConf {
namespace Internal {

ZConfLib *ZConfLib::createEmbeddedLib(const QString &, ZConfLib * fallback){
    return fallback;
}

} // namespace Internal
} // namespace ZeroConf
#endif
