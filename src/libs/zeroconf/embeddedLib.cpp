/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "syssocket.h" // this should be the first header included

#include "servicebrowser_p.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QProcess>

#ifdef Q_OS_LINUX
#define EMBEDDED_LIB
#endif

#ifdef Q_OS_WIN32
#define EMBEDDED_LIB
#endif

#ifdef EMBEDDED_LIB
#define PID_FILE "/tmp/mdnsd.pid"
#define MDNS_UDS_SERVERPATH "/tmp/mdnsd"

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

    EmbeddedZConfLib(const QString &daemonPath, ZConfLib::Ptr fallBack) : ZConfLib(fallBack),
        daemonPath(daemonPath)
    {
        if (daemonPath.isEmpty())
            m_maxErrors = 0;
        if (!daemonPath.isEmpty() && daemonPath.at(0) != QLatin1Char('/') && daemonPath.at(0) != QLatin1Char('.'))
            this->daemonPath = QCoreApplication::applicationDirPath() + QLatin1Char('/') + daemonPath;
    }

    ~EmbeddedZConfLib()
    { }

    QString name()
    {
        return QString::fromLatin1("Embedded Dns_sd Library");
    }

    bool tryStartDaemon(ErrorMessage::ErrorLogger *logger)
    {
        if (!daemonPath.isEmpty()) {
            QFileInfo dPath(daemonPath);
            QProcess killall;
            bool killAllFailed = false;
#ifdef Q_OS_WIN
            QString cmd = QLatin1String("taskill /im ") + dPath.fileName()
                    + QLatin1String(" /f /t");
#else
            QString cmd = QLatin1String("killall ") + dPath.fileName()
                    + QLatin1String(" 2> /dev/null");
#endif
            killall.start(cmd);
            if (!killall.waitForStarted()) {
                killAllFailed = true;
            } else {
                killall.closeWriteChannel();
                killall.waitForFinished();
            }
            if (killAllFailed) {
                if (logger)
                    logger->appendError(ErrorMessage::WarningLevel,
                                        ZConfLib::tr("%1 failed to kill other daemons with '%2'.")
                                        .arg(name()).arg(cmd));
                if (DEBUG_ZEROCONF)
                    qDebug() << name() << " had an error trying to kill other daemons with " << cmd;
            }
            QString daemonCmd = daemonPath;
            QStringList daemonArgs;
#ifdef Q_OS_LINUX
            if (QFile::exists(QLatin1String("/tmp/mdnsd")) && logger)
                logger->appendError(ErrorMessage::WarningLevel,
                                    ZConfLib::tr("%1 detected a file at /tmp/mdnsd, daemon startup will probably fail.")
                                    .arg(name()));
            QString logFile = QString::fromLatin1("/tmp/mdnssd.log");
            static int didFail = 0;
            QFile oldLog(logFile);
            if (didFail > 1 && oldLog.exists()) {
                oldLog.open(QIODevice::ReadOnly);
                if (logger) {
                    QByteArray logBA = oldLog.readAll();
                    logger->appendError(ErrorMessage::NoteLevel,
                                        ZConfLib::tr("%1: log of previous daemon run is: '%2'.\n")
                                        .arg(name())
                                        .arg(QString::fromLatin1(logBA.constData(), logBA.size())));
                    qDebug()<<logBA.size()<<oldLog.error()<<oldLog.errorString();
                }
                oldLog.close();
            }
            if (++didFail > 1)
                daemonArgs << QString::fromLatin1("-debug");
#endif
            if (QProcess::startDetached(daemonCmd, daemonArgs)) {
                QThread::yieldCurrentThread();
                // sleep a bit?
                if (DEBUG_ZEROCONF)
                    qDebug() << name() << " started " << daemonCmd << daemonArgs;
                return true;
            } else {
                this->setError(true, ZConfLib::tr("%1 failed starting embedded daemon at %2.")
                               .arg(name()).arg(daemonPath));
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
            *bRef = 0;
        }
    }

    void stopConnection(ConnectionRef cRef)
    {
        int sock = refSockFD(cRef);
        if (sock>0)
            shutdown(sock, SHUT_RDWR);
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
                                ZK_IP_Protocol                      /* protocol */,
                                const char                          *name,
                                const char                          *regtype,
                                const char                          *domain,
                                ServiceGatherer                     *gatherer)
    {
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceResolve(sdRef, kDNSServiceFlagsShareConnection
                                              // | kDNSServiceFlagsSuppressUnusable
                                              | kDNSServiceFlagsTimeout,
                                              interfaceIndex, name, regtype, domain,
                                              &cServiceResolveReply, gatherer);
    }

    DNSServiceErrorType queryRecord(ConnectionRef                       cRef,
                                    DNSServiceRef                       *sdRef,
                                    uint32_t                            interfaceIndex,
                                    const char                          *fullname,
                                    ServiceGatherer                     *gatherer)
    {
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceQueryRecord(sdRef, kDNSServiceFlagsShareConnection
                                                  // | kDNSServiceFlagsSuppressUnusable
                                                  | kDNSServiceFlagsTimeout,
                                                  interfaceIndex, fullname,
                                                  kDNSServiceType_TXT, kDNSServiceClass_IN,
                                                  &cTxtRecordReply , gatherer);
    }

    DNSServiceErrorType getAddrInfo(ConnectionRef                    cRef,
                                    DNSServiceRef                    *sdRef,
                                    uint32_t                         interfaceIndex,
                                    DNSServiceProtocol               protocol,
                                    const char                       *hostname,
                                    ServiceGatherer                  *gatherer)
    {
        *sdRef = reinterpret_cast<DNSServiceRef>(cRef);
        return embeddedLib::DNSServiceGetAddrInfo(sdRef, kDNSServiceFlagsShareConnection
                                                  // | kDNSServiceFlagsSuppressUnusable
                                                  | kDNSServiceFlagsTimeout,
                                                  interfaceIndex, protocol,
                                                  hostname, &cAddrReply, gatherer);
    }

    DNSServiceErrorType reconfirmRecord(ConnectionRef /*cRef*/, uint32_t /*interfaceIndex*/,
                                        const char * /*name*/, const char * /*type*/,
                                        const char * /*domain*/, const char * /*fullname*/)
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
        return embeddedLib::DNSServiceBrowse(sdRef, kDNSServiceFlagsShareConnection
                                             /* | kDNSServiceFlagsSuppressUnusable */,
                                             interfaceIndex, regtype, domain, &cBrowseReply,
                                             browser);
    }

    DNSServiceErrorType getProperty(const char *property, void *result, uint32_t *size)
    {
        return embeddedLib::DNSServiceGetProperty(property, result, size);
    }

    RunLoopStatus processOneEventBlock(ConnectionRef cRef)
    {
        if (embeddedLib::DNSServiceProcessResult(reinterpret_cast<DNSServiceRef>(cRef)) != kDNSServiceErr_NoError)
            return ProcessedError;
        return ProcessedOk;
    }

    DNSServiceErrorType createConnection(MainConnection *, ConnectionRef *sdRef)
    {
        return embeddedLib::DNSServiceCreateConnection(reinterpret_cast<DNSServiceRef*>(sdRef));
    }

    int refSockFD(ConnectionRef sdRef)
    {
        return embeddedLib::DNSServiceRefSockFD(reinterpret_cast<DNSServiceRef>(sdRef));
    }
};

ZConfLib::Ptr ZConfLib::createEmbeddedLib(const QString &daemonPath, const ZConfLib::Ptr &fallback)
{
    return ZConfLib::Ptr(new EmbeddedZConfLib(daemonPath, fallback));
}
} // namespace Internal
} // namespace ZeroConf

#else // no embedded lib

namespace ZeroConf {
namespace Internal {

ZConfLib::Ptr ZConfLib::createEmbeddedLib(const QString &, const ZConfLib::Ptr &fallback)
{
    return fallback;
}

} // namespace Internal
} // namespace ZeroConf
#endif
