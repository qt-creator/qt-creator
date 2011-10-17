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
#include <qglobal.h>
#ifdef Q_OS_LINUX
#define EMBEDDED_LIB
#endif

#ifdef Q_OS_WIN
# include <Winsock2.h>
#endif
#include "servicebrowser_p.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

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
class EmbeddedZConfLib : public ZConfLib
{
public:
    QString daemonPath;

    EmbeddedZConfLib(const QString &daemonPath, ZConfLib *fallBack = 0) : ZConfLib(fallBack), daemonPath(daemonPath)
    {
        if (!daemonPath.isEmpty() && daemonPath.at(0) != '/' && daemonPath.at(0) != '.')
            this->daemonPath = QCoreApplication::applicationDirPath() + QChar('/') + daemonPath;
    }

    ~EmbeddedZConfLib()
    { }

    QString name()
    {
        return QString::fromUtf8("EmbeddedZeroConfLib@%1").arg(size_t(this),0,16);
    }

    bool tryStartDaemon()
    {
        if (!daemonPath.isEmpty()) {
            if (QProcess::startDetached(daemonPath)) {
                QThread::yieldCurrentThread();
                // sleep a bit?
                if (DEBUG_ZEROCONF)
                    qDebug() << name() << " started " << daemonPath;
                return true;
            } else {
                this->setError(true, tr("%1 failed starting embedded daemon at %2").arg(name()).arg(daemonPath));
            }
        }
        return false;
    }

    void refDeallocate(DNSServiceRef sdRef)
    {
        embeddedLib::DNSServiceRefDeallocate(sdRef);
    }

    void browserDeallocate(BrowserRef *bRef)
    {
        if (bRef){
            embeddedLib::DNSServiceRefDeallocate(*reinterpret_cast<DNSServiceRef*>(bRef));
            *bRef=0;
        }
    }

    void stopConnection(ConnectionRef cRef)
    {
        int sock = refSockFD(cRef);
        if (sock>0)
            shutdown(sock,SHUT_RDWR);
    }

    void destroyConnection(ConnectionRef *sdRef)
    {
        if (sdRef) {
            embeddedLib::DNSServiceRefDeallocate(*reinterpret_cast<DNSServiceRef*>(sdRef));
            *sdRef = 0;
        }
    }

    DNSServiceErrorType resolve(ConnectionRef                       cRef,
                                DNSServiceRef                       *sdRef,
                                uint32_t                            interfaceIndex,
                                const char                          *name,
                                const char                          *regtype,
                                const char                          *domain,
                                ServiceGatherer                     *gatherer)
    {
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceResolve(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout,
                                              interfaceIndex, name, regtype, domain, &cServiceResolveReply, gatherer);
    }

    DNSServiceErrorType queryRecord(ConnectionRef                       cRef,
                                    DNSServiceRef                       *sdRef,
                                    uint32_t                            interfaceIndex,
                                    const char                          *fullname,
                                    ServiceGatherer                     *gatherer)
    {
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceQueryRecord(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout,
                                                  interfaceIndex, fullname,
                                                  kDNSServiceType_TXT, kDNSServiceClass_IN, &cTxtRecordReply ,gatherer);
    }

    DNSServiceErrorType getAddrInfo(ConnectionRef                    cRef,
                                    DNSServiceRef                    *sdRef,
                                    uint32_t                         interfaceIndex,
                                    DNSServiceProtocol               protocol,
                                    const char                       *hostname,
                                    ServiceGatherer                  *gatherer)
    {
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceGetAddrInfo(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable | kDNSServiceFlagsTimeout,
                                                  interfaceIndex, protocol,
                                                  hostname, &cAddrReply, gatherer);
    }

    DNSServiceErrorType reconfirmRecord(ConnectionRef /*cRef*/, uint32_t /*interfaceIndex*/,
                                                const char * /*name*/, const char * /*type*/, const char * /*domain*/,
                                                const char * /*fullname*/)
    {
        // reload and force update with in the callback with
        // embeddedLib::DNSServiceReconfirmRecord(flags, interfaceIndex, fullname, rrtype,
        //     rrclass, rdlen, rdata);
        return kDNSServiceErr_Unsupported;
    }

    DNSServiceErrorType browse(ConnectionRef                       cRef,
                               BrowserRef                          *bRef,
                               uint32_t                            interfaceIndex,
                               const char                          *regtype,
                               const char                          *domain,    /* may be NULL */
                               ServiceBrowserPrivate               *browser)
    {
        DNSServiceRef *sdRef = reinterpret_cast<DNSServiceRef *>(bRef);
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceBrowse(sdRef, kDNSServiceFlagsShareConnection | kDNSServiceFlagsSuppressUnusable,
                                             interfaceIndex, regtype, domain, &cBrowseReply, browser);
    }

    DNSServiceErrorType getProperty(const char *property,  /* Requested property (i.e. kDNSServiceProperty_DaemonVersion) */
                                    void       *result,    /* Pointer to place to store result */
                                    uint32_t   *size       /* size of result location */
                                    )
    {
        return embeddedLib::DNSServiceGetProperty(property, result, size);
    }

    ProcessStatus processResult(ConnectionRef sdRef) {
        if (embeddedLib::DNSServiceProcessResult(reinterpret_cast<DNSServiceRef>(sdRef)) != kDNSServiceErr_NoError)
            return ProcessedError;
        return ProcessedOk;
    }

    DNSServiceErrorType createConnection(ConnectionRef *sdRef)
    {
        return embeddedLib::DNSServiceCreateConnection(reinterpret_cast<DNSServiceRef*>(sdRef));
    }

    int refSockFD(ConnectionRef sdRef)
    {
        return embeddedLib::DNSServiceRefSockFD(reinterpret_cast<DNSServiceRef>(sdRef));
    }
};

ZConfLib *ZConfLib::createEmbeddedLib(const QString &daemonPath, ZConfLib *fallback)
{
    return new EmbeddedZConfLib(daemonPath, fallback);
}
} // namespace Internal
} // namespace ZeroConf

#else // no embedded lib

namespace ZeroConf {
namespace Internal {

ZConfLib *ZConfLib::createEmbeddedLib(const QString &, ZConfLib * fallback)
{
    return fallback;
}

} // namespace Internal
} // namespace ZeroConf
#endif
