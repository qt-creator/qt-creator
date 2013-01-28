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

#ifndef SERVICEBROWSER_P_H
#define SERVICEBROWSER_P_H

#include "dns_sd_types.h"
#include "servicebrowser.h"

#include <QCoreApplication>
#include <QAtomicInt>
#include <QHash>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QSharedPointer>
#include <QStringList>
#include <QThread>

namespace ZeroConf {

class ServiceBrowser;

namespace Internal {

class ServiceGatherer;
class ServiceBrowserPrivate;

enum ZK_IP_Protocol {
    ZK_PROTO_IPv4_OR_IPv6,
    ZK_PROTO_IPv4,
    ZK_PROTO_IPv6
};

// represents a zero conf library exposing the dns-sd interface
class ZConfLib {
    Q_DECLARE_TR_FUNCTIONS(ZeroConf::Internal::ZConfLib)
public:
    typedef QSharedPointer<ZConfLib> Ptr;
    typedef void *ConnectionRef;
    typedef void *BrowserRef;
    enum RunLoopStatus {
        ProcessedIdle,
        ProcessedOk,
        ProcessedQuit,
        ProcessedError,
        ProcessedFailure
    };
    enum {
        // Note: the select() implementation on Windows (Winsock2) fails with any timeout much larger than this
        MAX_SEC_FOR_READ = 100000000
    };
    Ptr fallbackLib;

    ZConfLib(Ptr fallBack);
    virtual ~ZConfLib();

    virtual QString name();

    virtual bool tryStartDaemon(ErrorMessage::ErrorLogger *logger = 0);

    virtual void refDeallocate(DNSServiceRef sdRef) = 0;
    virtual void browserDeallocate(BrowserRef *sdRef) = 0;
    virtual DNSServiceErrorType resolve(ConnectionRef cRef, DNSServiceRef *sdRef,
                                        uint32_t interfaceIndex, ZK_IP_Protocol protocol,
                                        const char *name, const char *regtype,
                                        const char *domain, ServiceGatherer *gatherer) = 0;
    virtual DNSServiceErrorType queryRecord(ConnectionRef cRef, DNSServiceRef *sdRef,
                                            uint32_t interfaceIndex, const char *fullname,
                                            ServiceGatherer *gatherer) = 0;
    virtual DNSServiceErrorType getAddrInfo(ConnectionRef cRef, DNSServiceRef *sdRef,
                                            uint32_t interfaceIndex, DNSServiceProtocol protocol,
                                            const char *hostname, ServiceGatherer *gatherer) = 0;

    virtual DNSServiceErrorType reconfirmRecord(ConnectionRef cRef, uint32_t interfaceIndex,
                                                const char *name, const char *type,
                                                const char *domain, const char *fullname) = 0;
    virtual DNSServiceErrorType browse(ConnectionRef cRef, BrowserRef *sdRef,
                                       uint32_t interfaceIndex,
                                       const char *regtype, const char *domain,
                                       ServiceBrowserPrivate *browser) = 0;
    virtual DNSServiceErrorType getProperty(const char *property, void *result, uint32_t *size) = 0;
    virtual RunLoopStatus processOneEventBlock(ConnectionRef sdRef) = 0;
    virtual RunLoopStatus processOneEvent(MainConnection *mainConnection,
                                          ConnectionRef sdRef, qint64 maxMsBlock);
    virtual DNSServiceErrorType createConnection(MainConnection *mainConnection,
                                                 ConnectionRef *sdRef) = 0;
    virtual void stopConnection(ConnectionRef cRef) = 0;
    virtual void destroyConnection(ConnectionRef *sdRef) = 0;
    virtual int refSockFD(ConnectionRef sdRef) = 0;
    bool isOk();
    QString errorMsg();
    void setError(bool failure, const QString &eMsg);
    int nFallbacks() const;
    int maxErrors() const;

    static Ptr createEmbeddedLib(const QString &daemonPath, const Ptr &fallback = Ptr(0));
    static Ptr createDnsSdLib(const QString &libName, const Ptr &fallback = Ptr(0));
    static Ptr createAvahiLib(const QString &libName, const QString &version, const Ptr &fallback = Ptr(0));
protected:
    bool m_isOk;
    QString m_errorMsg;
    int m_maxErrors;
};

/// class that gathers all needed info on a service, all its methods (creation included) are
/// supposed to be called by the listener/reaction thread
class ServiceGatherer {
public:
    QString                 hostName;
    ServiceBrowserPrivate   *serviceBrowser;
    QHostInfo               *host;
    Service::Ptr             publishedService;
    Service                 *currentService;

    typedef QSharedPointer<ServiceGatherer> Ptr;

    enum Status {
        ResolveConnectionFailed  = 1 << 0,
        ResolveConnectionActive  = 1 << 1,
        ResolveConnectionSuccess = 1 << 2,
        ResolveConnectionV6Failed  = 1 << 3,
        ResolveConnectionV6Active  = 1 << 4,
        ResolveConnectionV6Success = 1 << 5,
        TxtConnectionFailed      = 1 << 6,
        TxtConnectionActive      = 1 << 7,
        TxtConnectionSuccess     = 1 << 8,
        AddrConnectionFailed     = 1 << 9,
        AddrConnectionActive     = 1 << 10,
        AddrConnectionSuccess    = 1 << 11
    };

    QString fullName();
    bool enactServiceChange();
    void retireService();
    Ptr gatherer();

    void stopResolve(ZK_IP_Protocol protocol = ZK_PROTO_IPv4_OR_IPv6);
    void stopTxt();
    void stopHostResolution();
    void restartResolve(ZK_IP_Protocol protocol = ZK_PROTO_IPv4_OR_IPv6);
    void restartTxt();
    void restartHostResolution();

    bool currentServiceCanBePublished();

    ~ServiceGatherer();
    static Ptr createGatherer(const QString &newService, const QString &newType,
                              const QString &newDomain, const QString &fullName,
                              uint32_t interfaceIndex, ZK_IP_Protocol protocol,
                              ServiceBrowserPrivate *serviceBrowser);

    void serviceResolveReply(DNSServiceFlags flags, uint32_t interfaceIndex,
                             DNSServiceErrorType errorCode, const char *hosttarget,
                             const QString &port, uint16_t txtLen,
                             const unsigned char *rawTxtRecord);

    void txtRecordReply(DNSServiceFlags flags, DNSServiceErrorType errorCode,
                        uint16_t txtLen, const void *rawTxtRecord, uint32_t ttl);
    void txtFieldReply(DNSServiceFlags flags, DNSServiceErrorType errorCode,
                        uint16_t txtLen, const void *rawTxtRecord, uint32_t ttl);

    void addrReply(DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *hostname,
                   const struct sockaddr *address, uint32_t ttl);
    void maybeRemove();
    void stop(ZK_IP_Protocol protocol = ZK_PROTO_IPv4_OR_IPv6);
    void reload(qint32 interfaceIndex = 0, ZK_IP_Protocol protocol = ZK_PROTO_IPv4_OR_IPv6);
    void remove();
    void reconfirm();
    ZConfLib::Ptr lib();
private:
    ServiceGatherer(const QString &newService, const QString &newType, const QString &newDomain,
                    const QString &fullName, uint32_t interfaceIndex,
                    ZK_IP_Protocol protocol, ServiceBrowserPrivate *serviceBrowser);

    DNSServiceRef           resolveConnection;
    DNSServiceRef           resolveConnectionV6;
    DNSServiceRef           txtConnection;
    DNSServiceRef           addrConnection;
    uint32_t                interfaceIndex;

    quint32 status;
    QWeakPointer<ServiceGatherer> self;
};

//Q_DECLARE_METATYPE(Service::ConstPtr)

class ConnectionThread;

class MainConnection : private ErrorMessage::ErrorLogger {
    Q_DECLARE_TR_FUNCTIONS(ZeroConf)
public:
    enum RequestFlowStatus {
        NormalRFS,
        MoreComingRFS,
        ForceUpdateRFS
    };
    RequestFlowStatus flowStatus;
    /// WARNING order matters in this enum, and status should only be changed with increaseStatusTo
    enum Status {
        Starting,
        Started,
        Running,
        Stopping,
        Stopped
    };
    ZConfLib::Ptr lib;

    MainConnection(ServiceBrowserPrivate *initialBrowser = 0);
    virtual ~MainConnection();
    QMutex *lock();
    QMutex *mainThreadLock();
    void waitStartup();
    void stop(bool wait = true);
    void addBrowser(ServiceBrowserPrivate *browser);
    void removeBrowser(ServiceBrowserPrivate *browser);
    void updateFlowStatusForCancel();
    void updateFlowStatusForFlags(DNSServiceFlags flags);
    void maybeUpdateLists();
    void gotoValidLib();
    void abortLib();
    void createConnection();
    void destroyConnection();
    ZConfLib::RunLoopStatus handleEvent();
    bool increaseStatusTo(int s);
    void handleEvents();
    ZConfLib::ConnectionRef mainRef();
    Status status();

    QList<ErrorMessage> errors();
    bool isOk();
    void startupPhase(int progress, const QString &msg);
private:
    void appendError(ErrorMessage::SeverityLevel severity, const QString &msg);

    mutable QMutex m_lock, m_mainThreadLock;
    QList<ServiceBrowserPrivate *> m_browsers;
    ZConfLib::ConnectionRef m_mainRef;
    bool m_failed;
    ConnectionThread *m_thread;
    QAtomicInt m_status;
    int m_nErrs;
    QList<ErrorMessage> m_errors;
};

class ServiceBrowserPrivate {
public:
    ServiceBrowser *q;
    QString         serviceType;
    QString         domain;
    MainConnectionPtr    mainConnection;
    ZConfLib::BrowserRef serviceConnection;
    DNSServiceFlags flags;
    uint32_t        interfaceIndex;
    QList<QString>  knownServices;
    QMap<QString, ServiceGatherer::Ptr> gatherers;
    QList<Service::ConstPtr> activeServices;
    QList<Service::ConstPtr> nextActiveServices;
    QList<ServiceGatherer::Ptr> pendingGatherers;
    qint64 delayDeletesUntil;
    bool failed;
    bool browsing;
    bool autoResolveAddresses;
    bool requireAddresses;
    bool shouldRefresh;

    ZConfLib::ConnectionRef mainRef();
    void updateFlowStatusForCancel();
    void updateFlowStatusForFlags(DNSServiceFlags flags);

    void pendingGathererAdd(ServiceGatherer::Ptr gatherer);

    ServiceBrowserPrivate(const QString &serviceType, const QString &domain, bool requireAddresses,
                          MainConnectionPtr conn);
    ~ServiceBrowserPrivate();

    void insertGatherer(const QString &fullName);
    void maybeUpdateLists();

    void startBrowsing(quint32 interfaceIndex);
    bool internalStartBrowsing();
    void stopBrowsing();
    void triggerRefresh();
    void refresh();
    void reconfirmService(Service::ConstPtr s);

    void browseReply(DNSServiceFlags flags,
                     uint32_t interfaceIndex, ZK_IP_Protocol proto, DNSServiceErrorType errorCode,
                     const char *serviceName, const char *regtype, const char *replyDomain);

    void activateAutoRefresh();
    void serviceChanged(const Service::ConstPtr &oldService, const Service::ConstPtr &newService,
                        ServiceBrowser *browser);
    void serviceAdded(const Service::ConstPtr &service, ServiceBrowser *browser);
    void serviceRemoved(const Service::ConstPtr &service, ServiceBrowser *browser);
    void servicesUpdated(ServiceBrowser *browser);
    void startupPhase(int progress, const QString &description);
    void errorMessage(ErrorMessage::SeverityLevel severity, const QString &msg);
    void hadFailure(const QList<ErrorMessage> &msgs);
    void startedBrowsing();
};

class ConnectionThread: public QThread {
    MainConnection &connection;

    void run();
public:
    ConnectionThread(MainConnection &mc, QObject *parent = 0);
};

extern "C" void DNSSD_API cServiceResolveReply(
        DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
        DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port,
        uint16_t txtLen, const unsigned char *txtRecord, void *context);
extern "C" void DNSSD_API cTxtRecordReply(DNSServiceRef sdRef, DNSServiceFlags flags,
                                          uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                          const char *fullname, uint16_t rrtype, uint16_t rrclass,
                                          uint16_t rdlen, const void *rdata, uint32_t ttl,
                                          void *context);
extern "C" void DNSSD_API cAddrReply(DNSServiceRef sdRef, DNSServiceFlags flags,
                                     uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                     const char *hostname, const struct sockaddr *address,
                                     uint32_t ttl, void *context);
extern "C" void DNSSD_API cBrowseReply(DNSServiceRef sdRef, DNSServiceFlags flags,
                                       uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                       const char *serviceName, const char *regtype,
                                       const char *replyDomain, void *context);
} // namespace Internal
} // namespace Zeroconf

#endif // SERVICEBROWSER_P_H
