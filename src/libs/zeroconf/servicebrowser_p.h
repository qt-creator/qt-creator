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

#ifndef SERVICEBROWSER_P_H
#define SERVICEBROWSER_P_H

#include "dns_sd_types.h"
#include "servicebrowser.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QAtomicInt>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QThread>

class QHostInfo;

namespace ZeroConf {
namespace Internal {

class ServiceGatherer;
class ServiceBrowserPrivate;

// represents a zero conf library exposing the dns-sd interface
class ZConfLib {
    Q_DECLARE_TR_FUNCTIONS(ZeroConf)
public:
    typedef void *ConnectionRef;
    typedef void *BrowserRef;
    enum ProcessStatus {
        ProcessedOk,
        ProcessedQuit,
        ProcessedError,
        ProcessedFailure
    };
    ZConfLib *fallbackLib;

    ZConfLib(ZConfLib *fallBack);
    virtual ~ZConfLib();

    virtual QString name();

    virtual bool tryStartDaemon();

    virtual void refDeallocate(DNSServiceRef sdRef) = 0;
    virtual void browserDeallocate(BrowserRef *sdRef) = 0;
    virtual DNSServiceErrorType resolve(ConnectionRef cRef, DNSServiceRef *sdRef, uint32_t interfaceIndex,
                                        const char *name, const char *regtype, const char *domain,
                                        ServiceGatherer *gatherer) = 0;
    virtual DNSServiceErrorType queryRecord(ConnectionRef cRef, DNSServiceRef *sdRef, uint32_t interfaceIndex,
                                            const char *fullname, ServiceGatherer *gatherer) = 0;
    virtual DNSServiceErrorType getAddrInfo(ConnectionRef cRef, DNSServiceRef *sdRef, uint32_t interfaceIndex,
                                            DNSServiceProtocol protocol, const char *hostname,
                                            ServiceGatherer *gatherer) = 0;

    virtual DNSServiceErrorType reconfirmRecord(ConnectionRef cRef, uint32_t interfaceIndex,
                                                const char *name, const char *type, const char *domain,
                                                const char *fullname) = 0; // opt
    virtual DNSServiceErrorType browse(ConnectionRef cRef, BrowserRef *sdRef, uint32_t interfaceIndex,
                                       const char *regtype, const char *domain, ServiceBrowserPrivate *browser) = 0;
    virtual DNSServiceErrorType getProperty(const char *property, void *result, uint32_t *size) = 0;
    virtual ProcessStatus processResult(ConnectionRef sdRef) = 0;
    virtual DNSServiceErrorType createConnection(ConnectionRef *sdRef) = 0;
    virtual void stopConnection(ConnectionRef cRef) = 0;
    virtual void destroyConnection(ConnectionRef *sdRef) = 0;
    bool isOk();
    QString errorMsg();
    void setError(bool failure, const QString &eMsg);

    static ZConfLib *createEmbeddedLib(const QString &daemonPath, ZConfLib *fallback=0);
    static ZConfLib *createNativeLib(const QString &libName, ZConfLib *fallback=0);
    static ZConfLib *createAvahiLib(const QString &libName, ZConfLib *fallback=0);
    static ZConfLib *defaultLib();
protected:
    bool m_isOk;
    QString m_errorMsg;
};

/// class that gathers all needed info on a service, all its methods (creation included) are supposed to be called by the listener/reaction thread
class ServiceGatherer {
public:
    QHash<QString, QString> txtRecord;
    QString                 hostName;
    ServiceBrowserPrivate   *serviceBrowser;
    QHostInfo               *host;
    Service::Ptr             publishedService;
    Service                 *currentService;

    typedef QSharedPointer<ServiceGatherer> Ptr;

    enum Status{
        ResolveConnectionFailed  = 1 << 0,
        ResolveConnectionActive  = 1 << 1,
        ResolveConnectionSuccess = 1 << 2,
        TxtConnectionFailed      = 1 << 3,
        TxtConnectionActive      = 1 << 4,
        TxtConnectionSuccess     = 1 << 5,
        AddrConnectionFailed     = 1 << 6,
        AddrConnectionActive     = 1 << 7,
        AddrConnectionSuccess    = 1 << 8
    };

    QString fullName();
    void enactServiceChange();
    void retireService();
    Ptr gatherer();

    void stopResolve();
    void stopTxt();
    void stopHostResolution();
    void restartResolve();
    void restartTxt();
    void restartHostResolution();

    bool currentServiceCanBePublished();

    ~ServiceGatherer();
    static Ptr createGatherer(const QString &newService, const QString &newType, const QString &newDomain,
                              const QString &fullName, uint32_t interfaceIndex, ServiceBrowserPrivate *serviceBrowser);

    void serviceResolveReply(DNSServiceFlags flags, uint32_t interfaceIndex,
                             DNSServiceErrorType errorCode, const char *hosttarget,
                             const QString &port, uint16_t txtLen,
                             const unsigned char *rawTxtRecord);

    void txtRecordReply(DNSServiceFlags flags, DNSServiceErrorType errorCode,
                        uint16_t txtLen, const void *rawTxtRecord, uint32_t ttl);

    void addrReply(DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address,
                   uint32_t ttl);
    void maybeRemove();
    void stop();
    void reload(qint32 interfaceIndex=0);
    void remove();
    void reconfirm();
    ZConfLib *lib();
private:
    ServiceGatherer(const QString &newService, const QString &newType, const QString &newDomain,
                    const QString &fullName, uint32_t interfaceIndex, ServiceBrowserPrivate *serviceBrowser);

    DNSServiceRef           resolveConnection;
    DNSServiceRef           txtConnection;
    DNSServiceRef           addrConnection;
    uint32_t                interfaceIndex;

    quint32 status;
    QWeakPointer<ServiceGatherer> self;
};

//Q_DECLARE_METATYPE(Service::ConstPtr)

class ConnectionThread;

class MainConnection{
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
    ZConfLib *lib;

    MainConnection();
    ~MainConnection();
    QMutex *lock();
    void waitStartup();
    void stop(bool wait=true);
    void addBrowser(ServiceBrowserPrivate *browser);
    void removeBrowser(ServiceBrowserPrivate *browser);
    void updateFlowStatusForCancel();
    void updateFlowStatusForFlags(DNSServiceFlags flags);
    void maybeUpdateLists();
    void gotoValidLib();
    void abortLib();
    void createConnection();
    void destroyConnection();
    ZConfLib::ProcessStatus handleEvent();
    bool increaseStatusTo(int s);
    void handleEvents();
    ZConfLib::ConnectionRef mainRef();

    QStringList errors();
    void clearErrors();
    void appendError(const QStringList &msgs,bool fullFailure);
    bool isOk();
private:
    mutable QMutex m_lock;
    QList<ServiceBrowserPrivate *> m_browsers;
    ZConfLib::ConnectionRef m_mainRef;
    bool m_failed;
    ConnectionThread *m_thread;
    QAtomicInt m_status;
    int m_nErrs;
    QStringList m_errors;
};

class ServiceBrowserPrivate {
    friend class ServiceBrowser;
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
    bool failed;
    bool browsing;
    bool autoResolveAddresses;
    bool requireAddresses;

    ZConfLib::ConnectionRef mainRef();
    void updateFlowStatusForCancel();
    void updateFlowStatusForFlags(DNSServiceFlags flags);

    void pendingGathererAdd(ServiceGatherer::Ptr gatherer);

    ServiceBrowserPrivate(const QString &serviceType, const QString &domain, bool requireAddresses,
                          MainConnectionPtr conn);
    ~ServiceBrowserPrivate();

    void insertGatherer(const QString &fullName);
    void maybeUpdateLists();

    bool startBrowsing(quint32 interfaceIndex);
    void stopBrowsing();
    void reconfirmService(Service::ConstPtr s);

    void browseReply(DNSServiceFlags flags,
                     uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName,
                     const char *regtype, const char *replyDomain);
    void serviceChanged(const Service::ConstPtr &oldService, const Service::ConstPtr &newService, ServiceBrowser *browser);
    void serviceAdded(const Service::ConstPtr &service, ServiceBrowser *browser);
    void serviceRemoved(const Service::ConstPtr &service, ServiceBrowser *browser);
    void servicesUpdated(ServiceBrowser *browser);
    void hadError(QStringList errorMsgs, bool completeFailure);
};

class ConnectionThread: public QThread{
    MainConnection &connection;

    void run();
public:
    ConnectionThread(MainConnection &mc, QObject *parent=0);
};

extern "C" void DNSSD_API cServiceResolveReply(DNSServiceRef sdRef, DNSServiceFlags flags,
                                     uint32_t interfaceIndex, DNSServiceErrorType errorCode,
                                     const char *fullname, const char *hosttarget, uint16_t port,
                                     uint16_t txtLen, const unsigned char *txtRecord, void *context);
extern "C" void DNSSD_API cTxtRecordReply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                          DNSServiceErrorType errorCode, const char *fullname,
                                          uint16_t rrtype, uint16_t rrclass, uint16_t rdlen,
                                          const void *rdata, uint32_t ttl, void *context);
extern "C" void DNSSD_API cAddrReply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                     DNSServiceErrorType errorCode, const char *hostname,
                                     const struct sockaddr *address, uint32_t ttl, void *context);
extern "C" void DNSSD_API cBrowseReply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                       DNSServiceErrorType errorCode, const char *serviceName,
                                       const char *regtype, const char *replyDomain, void *context);
} // namespace Internal
} // namespace Zeroconf

#endif // SERVICEBROWSER_P_H
