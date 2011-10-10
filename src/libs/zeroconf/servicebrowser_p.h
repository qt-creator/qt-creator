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

// represents a zero conf library exposing the dns-sd interface
class ZConfLib {
public:
    ZConfLib *fallbackLib;

    ZConfLib(ZConfLib *fallBack);
    virtual ~ZConfLib();

    virtual QString name();

    virtual bool tryStartDaemon();

    virtual void refDeallocate(DNSServiceRef sdRef) = 0;
    virtual DNSServiceErrorType resolve(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                        const char *name, const char *regtype, const char *domain,
                                        DNSServiceResolveReply callBack, void *context) = 0;
    virtual DNSServiceErrorType queryRecord(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                            const char *fullname, uint16_t rrtype, uint16_t rrclass,
                                            DNSServiceQueryRecordReply callBack, void *context) = 0;
    virtual DNSServiceErrorType getAddrInfo(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                            DNSServiceProtocol protocol, const char *hostname,
                                            DNSServiceGetAddrInfoReply callBack, void *context) = 0;

    // remove txt functions from lib and always embed?
    virtual uint16_t txtRecordGetCount(uint16_t txtLen, const void *txtRecord) = 0;
    virtual DNSServiceErrorType txtRecordGetItemAtIndex(uint16_t txtLen, const void *txtRecord,
                                                        uint16_t itemIndex, uint16_t keyBufLen, char *key,
                                                        uint8_t *valueLen, const void **value) = 0;

    virtual DNSServiceErrorType reconfirmRecord(DNSServiceFlags flags, uint32_t interfaceIndex,
                                                const char *fullname, uint16_t rrtype, uint16_t rrclass,
                                                uint16_t rdlen, const void *rdata) = 0; // opt
    virtual DNSServiceErrorType browse(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                       const char *regtype, const char *domain, DNSServiceBrowseReply callBack,
                                       void *context) = 0;
    virtual DNSServiceErrorType getProperty(const char *property, void *result, uint32_t *size) = 0;
    virtual DNSServiceErrorType processResult(DNSServiceRef sdRef) = 0;
    virtual DNSServiceErrorType createConnection(DNSServiceRef *sdRef) = 0;
    virtual int refSockFD(DNSServiceRef sdRef) = 0;
    bool isOk();

    static ZConfLib *createEmbeddedLib(const QString &daemonPath, ZConfLib *fallback=0);
    static ZConfLib *createNativeLib(const QString &libName, ZConfLib *fallback=0);
    static ZConfLib *defaultLib();
private:
    bool m_isOk;
};

/// class that gathers all needed info on a service, all its methods (creation included) are supposed to be called by the listener/reaction thread
class ServiceGatherer {
public:
    QHash<QString, QString> txtRecord;
    short                   port;
    QString                 serviceName;
    QString                 serviceType;
    QString                 domain;
    QString                 fullName;
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

    void serviceResolveReply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                             DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget,
                             uint16_t port /* In network byte order */, uint16_t txtLen,
                             const unsigned char *rawTxtRecord);

    void txtRecordReply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                        DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype,
                        uint16_t rrclass, uint16_t txtLen, const void *rawTxtRecord, uint32_t ttl);

    void addrReply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                   DNSServiceErrorType errorCode, const char *hostname, const struct sockaddr *address,
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
public:
    enum RequestFlowStatus {
        NormalRFS,
        MoreComingRFS,
        ForceUpdateRFS
    };
    RequestFlowStatus flowStatus;
    enum {
        // Note: the select() implementation on Windows (Winsock2) fails with any timeout much larger than this
        LONG_TIME = 100000000
    };
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
    QString tr(const char *s);
    void gotoValidLib();
    void abortLib();
    void createConnection();
    void destroyConnection();
    bool handleEvent();
    bool increaseStatusTo(int s);
    void selectLoop();
    void handleEvents();
    DNSServiceRef mainRef();

    QStringList errors();
    void clearErrors();
    void appendError(const QStringList &msgs,bool fullFailure);
    bool isOk();
private:
    mutable QMutex m_lock;
    QList<ServiceBrowserPrivate *> m_browsers;
    DNSServiceRef   m_mainRef;
    volatile int m_timeOut;
    bool m_failed;
    ConnectionThread *m_thread;
    QAtomicInt m_status;
    int m_nErrs;
    bool m_useSelect;
    QStringList m_errors;
};

class ServiceBrowserPrivate {
    friend class ServiceBrowser;
public:
    ServiceBrowser *q;
    QString         serviceType;
    QString         domain;
    MainConnectionPtr  mainConnection;
    DNSServiceRef   serviceConnection;
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

    DNSServiceRef   mainRef();
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

    void browseReply(DNSServiceRef sdRef, DNSServiceFlags flags,
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
} // namespace Internal
} // namespace Zeroconf

#endif // SERVICEBROWSER_P_H
