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
#include "zeroconf_global.h"
#ifdef Q_OS_WIN
// Disable min max macros from windows headers,
// because we want to use the template methods from std.
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#endif // Q_OS_WIN

#include "mdnsderived.h"
#include "servicebrowser_p.h"

#include <algorithm>
#include <errno.h>
#include <limits>
#include <signal.h>
#ifdef Q_OS_UNIX
// for select()
#  include <unistd.h>
#endif

#include <QAtomicPointer>
#include <QCoreApplication>
#include <QDateTime>
#include <QGlobalStatic>
#include <QMetaType>
#include <QMutexLocker>
#include <QNetworkInterface>
#include <QString>
#include <QStringList>
#include <QtEndian>
#include <QHostInfo>

//the timeval struct under windows uses long instead of suseconds_t
#ifdef Q_OS_WIN
typedef long suseconds_t;
#endif

/*!
  \namespace ZeroConf

  \brief namespace for zeroconf (Bonjour/DNS-SD) functionality, currently mostly for browsing services.
*/

namespace { // anonymous namespace for free functions
// ----------------- free functions -----------------

using namespace ZeroConf;
using namespace ZeroConf::Internal;

QString toFullNameC(const char * const service, const char * const regtype, const char * const domain)
{
    char fullName[kDNSServiceMaxDomainName];
    myDNSServiceConstructFullName(fullName, service, regtype, domain);
    fullName[kDNSServiceMaxDomainName - 1] = 0; // just to be sure
    return QString::fromUtf8(fullName);
}

int fromFullNameC(const char * const fullName, QString &service, QString &regtype, QString &domain)
{
    char fullNameDecoded[kDNSServiceMaxDomainName];
    int encodedI = 0;
    int decodedI = 0;
    int oldPos[4];
    int iPos = 0;
    while (fullName[encodedI] != 0 && encodedI <kDNSServiceMaxDomainName){
        char c = fullName[encodedI++];
        if (c == '\\'){
            c = fullName[++encodedI];
            if (c == 0 || encodedI == kDNSServiceMaxDomainName)
                return 1;
            if (c >= '0' && c <= '9'){
                int val = (c - '0') * 100;
                c = fullName[++encodedI];
                if (c < '0' || c > '9' || encodedI == kDNSServiceMaxDomainName)
                    return 2;
                val += (c - '0') * 10;
                c = fullName[++encodedI];
                if (c < '0' || c > '9')
                    return 3;
                val += (c - '0');
                fullNameDecoded[decodedI++] = static_cast<char>(static_cast<unsigned char>(val));
            } else {
                fullNameDecoded[decodedI++] = c;
            }
        } else if (c == '.') {
            if (iPos < 4)
                oldPos[iPos++] = decodedI;
            fullNameDecoded[decodedI++] = c;
        } else {
            fullNameDecoded[decodedI++] = c;
        }
    }
    if (iPos != 4) return 5;
    service = QString::fromUtf8(&fullNameDecoded[0], oldPos[0]);
    regtype = QString::fromUtf8(&fullNameDecoded[oldPos[0] + 1], oldPos[3] - oldPos[0] - 1);
    domain = QString::fromUtf8(&fullNameDecoded[oldPos[3] + 1], decodedI - oldPos[3] - 1);
    return 0;
}

/// singleton for lib setup
class ZeroConfLib
{
public:
    ZeroConfLib();
    ZConfLib::Ptr defaultLib();
    void setDefaultLib(LibUsage usage, const QString &avahiLibName, const QString &avahiVersion,
                       const QString &dnsSdLibName, const QString &dnsSdDaemonPath);
    int nFallbacksTot() const;

private:
    static const char *defaultmDnsSdLibName;
    static const char *defaultmDNSDaemonName;
    int m_nFallbacksTot;
    QMutex m_lock;
    ZConfLib::Ptr m_defaultLib;
};

Q_GLOBAL_STATIC(ZeroConfLib, zeroConfLibInstance)

#ifdef Q_OS_WIN
    const char *ZeroConfLib::defaultmDnsSdLibName  = "dnssd";
    const char *ZeroConfLib::defaultmDNSDaemonName = "mdnssd.exe";
#else
    const char *ZeroConfLib::defaultmDnsSdLibName  = "dns_sd";
    const char *ZeroConfLib::defaultmDNSDaemonName = "mdnssd";
#endif

ZeroConfLib::ZeroConfLib(): m_lock(QMutex::Recursive),
    m_defaultLib(ZConfLib::createDnsSdLib(QLatin1String(defaultmDnsSdLibName),
                 ZConfLib::createEmbeddedLib(QString(),
                 ZConfLib::createAvahiLib(QLatin1String("avahi-client"),QLatin1String("3"),
                 ZConfLib::createEmbeddedLib(QLatin1String(defaultmDNSDaemonName))))))
{
    qRegisterMetaType<ZeroConf::Service::ConstPtr>("ZeroConf::Service::ConstPtr");
    qRegisterMetaType<ZeroConf::ErrorMessage::SeverityLevel>("ZeroConf::ErrorMessage::SeverityLevel");
    qRegisterMetaType<ZeroConf::ErrorMessage>("ZeroConf::ErrorMessage");
    qRegisterMetaType<QList<ZeroConf::ErrorMessage> >("QList<ZeroConf::ErrorMessage>");
    m_nFallbacksTot = (m_defaultLib ? m_defaultLib->nFallbacks() : 0);

}

ZConfLib::Ptr ZeroConfLib::defaultLib(){
    QMutexLocker l(&m_lock);
    return m_defaultLib;
}

void ZeroConfLib::setDefaultLib(LibUsage usage, const QString &avahiLibName,
                                const QString &avahiVersion, const QString &dnsSdLibName,
                                const QString &dnsSdDaemonPath){
    QMutexLocker l(&m_lock);
    switch (usage){
    case (UseDnsSdOnly):
        m_defaultLib = ZConfLib::createDnsSdLib(dnsSdLibName);
        break;
    case (UseEmbeddedOnly):
        m_defaultLib = ZConfLib::createEmbeddedLib(dnsSdDaemonPath);
        break;
    case (UseAvahiOnly):
        m_defaultLib = ZConfLib::createAvahiLib(avahiLibName, avahiVersion);
        break;
    case (UseAvahiOrDnsSd):
        m_defaultLib = ZConfLib::createAvahiLib(avahiLibName, avahiVersion,
                       ZConfLib::createDnsSdLib(dnsSdLibName));
        break;
    case (UseAvahiOrDnsSdOrEmbedded):
        m_defaultLib = ZConfLib::createAvahiLib(avahiLibName, avahiVersion,
                       ZConfLib::createDnsSdLib(dnsSdLibName,
                       ZConfLib::createEmbeddedLib(dnsSdDaemonPath)));
        break;
    default:
        qDebug() << "invalid usage " << usage;
    }
    int newNFallbacks = m_defaultLib->nFallbacks();
    if (m_nFallbacksTot < newNFallbacks)
        m_nFallbacksTot = newNFallbacks;
}

int ZeroConfLib::nFallbacksTot() const
{
    return m_nFallbacksTot;
}

} // end anonymous namespace

namespace ZeroConf {

int gQuickStop = 0;

// ----------------- ErrorMessage impl -----------------
/*!
  \class ZeroConf::ErrorMessage

  \brief class representing an error message

  very simple class representing an error message
 */

/// empty constructor (required by qRegisterMetaType)
ErrorMessage::ErrorMessage(): severity(FailureLevel) {}
/// constructor
ErrorMessage::ErrorMessage(SeverityLevel s, const QString &m): severity(s), msg(m) {}

/// returns a human readable string for each severity level
QString ErrorMessage::severityLevelToString(ErrorMessage::SeverityLevel severity)
{
    const char *ctx = "Zeroconf::SeverityLevel";
    switch (severity) {
    case NoteLevel:
        return QCoreApplication::translate(ctx, "NOTE");
    case WarningLevel:
        return QCoreApplication::translate(ctx, "WARNING");
    case ErrorLevel:
        return QCoreApplication::translate(ctx, "ERROR");
    case FailureLevel:
        return QCoreApplication::translate(ctx, "FATAL_ERROR");
    default:
        return QCoreApplication::translate(ctx, "UNKNOWN_LEVEL_%1").arg(severity);
    }
}

QDebug operator<<(QDebug dbg, const ErrorMessage &eMsg)
{
    dbg << ErrorMessage::severityLevelToString(eMsg.severity) << eMsg.msg;
    return dbg;
}

// ----------------- Service impl -----------------
/*!
  \class ZeroConf::Service

  \brief class representing a zeroconf service

  Instances of this class are basically constant, but can be outdated. They are normally accessed
  through a Shared pointer.
  This design avoids race conditions when used though multiple threads.

  \threadsafe
 */

Service::Service(const Service &o) :
    m_name(o.m_name),
    m_type(o.m_type),
    m_domain(o.m_domain),
    m_fullName(o.m_fullName),
    m_port(o.m_port),
    m_txtRecord(o.m_txtRecord),
    m_host(o.m_host ? new QHostInfo(*o.m_host) : 0),
    m_interfaceNr(o.m_interfaceNr),
    m_outdated(o.m_outdated)
{
}

Service::Service() : m_host(0), m_interfaceNr(0), m_outdated(false)
{ }

Service::~Service()
{
    delete m_host;
}

/*!
  \fn QString Service::networkInterface()

  Returns the interface on which the service is reachable.
 */
QNetworkInterface Service::networkInterface() const
{
    return QNetworkInterface::interfaceFromIndex(m_interfaceNr);
}

QStringList Service::addresses(Service::AddressStyle style) const
{
    QStringList res;
    if (host() == 0)
        return res;
    foreach (const QHostAddress &addr, host()->addresses()){
        QString addrStr;
        if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            // Add the interface to use to the address <address>%<interface>
            //
            // This is required only for link local addresses (like fe80:*)
            // but as we have it we do it (and most likely addresses here are
            // link local).
            //
            // on windows only addresses like fe80::42%22 work
            // on OSX 10.5 only things like fe80::42%en4 work
            // on later OSX versions and linux both <address>%<interface number>
            // and <address>%<interface name> work, we use the interface name as
            // it looks better
#ifdef Q_OS_WIN
            QString interfaceStr = QString::number(networkInterface().index());
#else
            QString interfaceStr = networkInterface().name();
#endif
            addrStr = QString::fromLatin1("%1%%2").arg(addr.toString()).arg(interfaceStr);
            if (style == QuoteIPv6Adresses)
                addrStr = QString::fromLatin1("[%1]").arg(addrStr);
        } else {
            addrStr = addr.toString();
        }
        res.append(addrStr);
    }
    return res;
}

bool Service::operator==(const Service &o) const {
    bool eq = m_fullName == o.m_fullName
            && m_name == o.m_name && m_type == o.m_type
            && m_domain == o.m_domain && m_port == o.m_port
            && m_txtRecord == o.m_txtRecord && m_interfaceNr == o.m_interfaceNr
            && m_outdated == m_outdated;
    if (eq) {
        if (m_host != o.m_host) {
            if (m_host == 0 || o.m_host == 0)
                return false;
            return m_host->hostName() == o.m_host->hostName()
                    && m_host->addresses() == o.m_host->addresses();
        }
    }
    return eq;
}

QDebug operator<<(QDebug dbg, const Service &service)
{
    dbg.maybeSpace() << "Service{ name:" << service.name() << ", "
                     << "type:" << service.type() << ", domain:" << service.domain() << ", "
                     << " fullName:" << service.fullName() << ", port:" << service.port()
                     << ", txtRecord:{";
    bool first = true;
    QHashIterator<QString, QString> i(service.txtRecord());
    while (i.hasNext()){
        i.next();
        if (first)
            first = false;
        else
            dbg << ", ";
        dbg << i.key() << ":" << i.value();
    }
    dbg << "}, ";
    if (const QHostInfo *host = service.host()){
        dbg << "host:{" << host->hostName() << ", addresses[";
        first = true;
        foreach (const QHostAddress &addr, host->addresses()){
            if (first)
                first = false;
            else
                dbg << ", ";
            dbg << addr.toString();
        }
        dbg << "], },";
    } else {
        dbg << " host:*null*,";
    }
    dbg << " interfaceNr:" << service.interfaceNr() << ", outdated:" << service.outdated() << " }";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Service::ConstPtr &service){
    if (service.data() == 0)
        dbg << "Service{*NULL*}";
    else
        dbg << *service.data();
    return dbg;
}

// inline methods
/*!
  \fn bool Service::outdated() const

  Returns if the service data is outdated, its value might change even on
  the (otherwise constant) objects returned by a ServiceBrowser.
*/
/*!
  \fn QString Service::name() const

  Returns the name of the service (non escaped).
*/
/*!
  \fn QString Service::type() const

  Returns the name of the service type (non escaped).
*/
/*!
  \fn QString Service::domain() const

  Returns the name of the domain (non escaped).
*/
/*!
  \fn QString Service::fullName() const

  Returns the full name (service.type.domain) with each component correctly escaped.
*/
/*!
  \fn QString Service::port() const

  Return the port of the service (as a string, not as number).
*/
/*!
  \fn const Service::ServiceTxtRecord &txtRecord() const

  Returns the extra information on this service.
*/
/*!
  \fn const Service::QHostInfo *host() const

  Returns the host through which this service is reachable.
*/
/*!
  \fn int Service::interfaceNr() const

  returns the interface on which the service is reachable, 1 based, 0 means to try all interfaces
*/
/*!
  \fn bool Service::invalidate()

  Marks this service as outdated.
*/

// ----------------- ServiceBrowser impl -----------------
/*!
  \class ZeroConf::ServiceBrowser

  \brief class that browses (searches) for a given zeronconf service

  The actual browsing starts only when startBrowsing() is called. If you want to receive all service
  changes connect before starting browsing.

  The current list of services can be gathered with the services() method.

  \threadsafe
 */

/// starts the browsing, return true if successfull
void ServiceBrowser::startBrowsing(qint32 interfaceIndex)
{
    d->startBrowsing(interfaceIndex);
}

/// create a new brower for the given service type
ServiceBrowser::ServiceBrowser(const QString &serviceType, const QString &domain,
        AddressesSetting addressesSetting, QObject *parent)
    : QObject(parent), timer(0),
      d(new ServiceBrowserPrivate(serviceType, domain, addressesSetting == RequireAddresses,
          MainConnectionPtr()))
{
    connect(this,SIGNAL(activateAutoRefresh()),this,SLOT(autoRefresh()));
    d->q = this;
}

ServiceBrowser::ServiceBrowser(const MainConnectionPtr &mainConnection, const QString &serviceType,
        const QString &domain, AddressesSetting addressesSetting, QObject *parent)
    : QObject(parent), timer(0),
      d(new ServiceBrowserPrivate(serviceType, domain, addressesSetting == RequireAddresses,
          mainConnection))
{
    d->q = this;
}

ServiceBrowser::~ServiceBrowser()
{
    delete d;
}

/// returns the main connection used by this ServiceBrowser
MainConnectionPtr ServiceBrowser::mainConnection() const
{
    return d->mainConnection;
}

/// stops browsing, but does not delete all services found
void ServiceBrowser::stopBrowsing()
{
    if (timer) {
        timer->stop();
        delete timer;
        timer = 0;
    }
    d->stopBrowsing();
}

/// starts an explicit browse (important especially with avahi)
void ServiceBrowser::triggerRefresh()
{
    d->triggerRefresh();
}

/// if the service is currently active
bool ServiceBrowser::isBrowsing() const
{
    return d->browsing;
}

/// type of the service browsed (non escaped)
const QString& ServiceBrowser::serviceType() const
{
    return d->serviceType;
}

/// domain that is browser (non escaped)
const QString& ServiceBrowser::domain() const
{
    return d->domain;
}

/// if addresses should be resolved automatically for each service found
bool ServiceBrowser::adressesAutoResolved() const
{
    return d->autoResolveAddresses;
}

/// if addresses are required to add the service to the list of available services
bool ServiceBrowser::addressesRequired() const
{
    return d->requireAddresses;
}

/// list of current services (by copy on purpose)
QList<Service::ConstPtr> ServiceBrowser::services() const
{
    QMutexLocker l(d->mainConnection->lock());
    return d->activeServices;
}

/// forces a full update of a service (call this after a failure to connect to the service)
/// this is an expensive call, use only when needed
void ServiceBrowser::reconfirmService(Service::ConstPtr service)
{
    d->reconfirmService(service);
}

void ServiceBrowser::autoRefresh()
{
    QMutexLocker l(d->mainConnection->lock());
    if (!timer) {
        timer = new QTimer(this);
        connect(timer,SIGNAL(timeout()),this,SLOT(triggerRefresh()));
        timer->setSingleShot(true);
    }
    timer->start(5000);
}

// signals
/*!
  \fn void ServiceBrowser::serviceChanged(
       Service::ConstPtr oldService, Service::ConstPtr newService, ServiceBrowser *browser)

   This signal is called when a service is added removed or changes.
   Both oldService or newService might be null (covers both add and remove).
   The services list might not be synchronized with respect to this signal.
*/
/*!
  \fn void ServiceBrowser::serviceAdded(Service::ConstPtr service, ServiceBrowser *browser)

   This signal is called when a service is added (convenience method)
   the services list might not be synchronized with respect to this signal

   \sa serviceChanged()
*/
/*!
  \fn void ServiceBrowser::serviceRemoved(Service::ConstPtr service, ServiceBrowser *browser)

  This signal is called when a service is removed (convenience method)
  the services list might not be synchronized with respect to this signal

   \sa serviceChanged()
*/
/*!
  \fn void ServiceBrowser::servicesUpdated(ServiceBrowser *browser)

  This signal is called when the list is updated.
  It might collect several serviceChanged signals together, if you use the list returned by
  services(), use this signal, not serviceChanged(), serviceAdded() or serviceRemoved() to know
  about changes to the list.
*/
/*!
  \fn void errorMessage(ZeroConf::ErrorMessage::SeverityLevel severity, const QString &msg, ZeroConf::ServiceBrowser *browser)

  This signal is called every time a warning or error is emitted (for example when a library 
  cannot be used and another one has to be used). Zeroconf will still work if severity < FailureLevel.
*/
/*!
  \fn void hadFailure(const QList<ZeroConf::ErrorMessage> &messages, ZeroConf::ServiceBrowser *browser)

  This signal is emitted only when a full failure has happened, and all the previous errors/attempts to set up zeroconf
  are passed in messages.
*/
/*!
  \fn void startedBrowsing(ZeroConf::ServiceBrowser *browser)

  This signal is emitted when browsing has actually started.
  One can rely on either startedBrowsing or hadFailure to be emitted.
*/

// ----------------- library initialization impl -----------------
/*!
  Sets the library used by future Service Browsers to preform the mdns queries.
  This changes the default library used by the next MainConnection, it does not change the already
  instantiated connections.
  \a usage can decide which libraries are tried,
  \a libName should be the name (or path) to the libdns library,
  \a daemonPath is the path to the daemon executable which should be started by the embedded library
                if no daemon is found.

  \threadsafe
*/
void setDefaultZConfLib(LibUsage usage, const QString &avahiLibName, const QString &version,
                        const QString &dnsSdLibName,const QString &dnsSdDaemonPath)
{
    zeroConfLibInstance()->setDefaultLib(usage, avahiLibName, version, dnsSdLibName, dnsSdDaemonPath);
}

namespace Internal {
// ----------------- dns-sd C callbacks -----------------

extern "C" void DNSSD_API cServiceResolveReply(DNSServiceRef                       sdRef,
                                               DNSServiceFlags                     flags,
                                               uint32_t                            interfaceIndex,
                                               DNSServiceErrorType                 errorCode,
                                               const char                          *fullname,
                                               const char                          *hosttarget,
                                               uint16_t                            port, /* In network byte order */
                                               uint16_t                            txtLen,
                                               const unsigned char                 *txtRecord,
                                               void                                *context)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "cServiceResolveReply(" << ((size_t)sdRef) << ", " << ((quint32)flags)
                 << ", " << interfaceIndex << ", " << ((int)errorCode) << ", " << fullname
                 << ", " << hosttarget << ", " << qFromBigEndian(port) << ", " << txtLen << ", '"
                 << QString::fromUtf8((const char *)txtRecord, txtLen) << "', "
                 << ((size_t)context);
    ServiceGatherer *ctxGatherer = reinterpret_cast<ServiceGatherer *>(context);
    if (ctxGatherer){
        if (ctxGatherer->currentService->fullName() != QString::fromLocal8Bit(fullname)) {
            qDebug() << "ServiceBrowser " << ctxGatherer->serviceBrowser->serviceType
                     << " for service " << ctxGatherer->currentService->name()
                     << " ignoring resolve reply for " << fullname << " vs. "
                     << ctxGatherer->currentService->fullName();
            return;
        }
        ctxGatherer->serviceResolveReply(flags, interfaceIndex, errorCode, hosttarget,
                                         QString::number(qFromBigEndian(port)), txtLen, txtRecord);
    }
}

extern "C" void DNSSD_API cTxtRecordReply(DNSServiceRef                       sdRef,
                                          DNSServiceFlags                     flags,
                                          uint32_t                            interfaceIndex,
                                          DNSServiceErrorType                 errorCode,
                                          const char                          *fullname,
                                          uint16_t                            rrtype,
                                          uint16_t                            rrclass,
                                          uint16_t                            rdlen,
                                          const void                          *rdata,
                                          uint32_t                            ttl,
                                          void                                *context)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "cTxtRecordReply(" << ((size_t)sdRef) << ", " << ((int)flags) << ", "
                 << interfaceIndex << ", " << ((int)errorCode) << ", " << fullname << ", "
                 << rrtype << ", " << rrclass << ", " << ", " << rdlen
                 << QString::fromUtf8((const char *)rdata, rdlen) << "', " << ttl << ", "
                 << ((size_t)context);
    ServiceGatherer *ctxGatherer = reinterpret_cast<ServiceGatherer *>(context);
    if (ctxGatherer){
        if (rrtype != kDNSServiceType_TXT || rrclass != kDNSServiceClass_IN) {
            qDebug() << "ServiceBrowser " << ctxGatherer->serviceBrowser->serviceType
                     << " for service " << ctxGatherer->currentService->fullName()
                     << " received an unexpected rrtype/class:" << rrtype << "/" << rrclass;
        }
        ctxGatherer->txtRecordReply(flags, errorCode, rdlen, rdata, ttl);
    }
}

extern "C" void DNSSD_API cAddrReply(DNSServiceRef                    sdRef,
                                     DNSServiceFlags                  flags,
                                     uint32_t                         interfaceIndex,
                                     DNSServiceErrorType              errorCode,
                                     const char                       *hostname,
                                     const struct sockaddr            *address,
                                     uint32_t                         ttl,
                                     void                             *context)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "cAddrReply(" << ((size_t)sdRef) << ", " << ((int)flags) << ", "
                 << interfaceIndex << ", " << ((int)errorCode) << ", " << hostname << ", "
                 << QHostAddress(address).toString() << ", " << ttl << ", " << ((size_t)context);
    ServiceGatherer *ctxGatherer = reinterpret_cast<ServiceGatherer *>(context);
    if (ctxGatherer)
        ctxGatherer->addrReply(flags, errorCode, hostname, address, ttl);
}

/// callback for service browsing
extern "C" void DNSSD_API cBrowseReply(DNSServiceRef       sdRef,
                                       DNSServiceFlags     flags,
                                       uint32_t            interfaceIndex,
                                       DNSServiceErrorType errorCode,
                                       const char          *serviceName,
                                       const char          *regtype,
                                       const char          *replyDomain,
                                       void                *context)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "cBrowseReply(" << ((size_t)sdRef) << ", " << flags << ", " << interfaceIndex
                 << ", " << ((int)errorCode) << ", " << serviceName << ", " << regtype << ", "
                 << replyDomain << ", " << ((size_t)context);
    ServiceBrowserPrivate *sb = (ServiceBrowserPrivate *)(context);
    if (sb == 0){
        qDebug() << "ServiceBrowser ignoring reply because context was null ";
        return;
    }
    sb->browseReply(flags, interfaceIndex, ZK_PROTO_IPv4_OR_IPv6,
                    errorCode, serviceName, regtype, replyDomain);
}

// ----------------- ConnectionThread impl -----------------

void ConnectionThread::run()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    struct sigaction act;
    // ignore SIGPIPE
    act.sa_handler=SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGPIPE, &act, NULL);
#endif
    connection.handleEvents();
}

ConnectionThread::ConnectionThread(MainConnection &mc, QObject *parent):
    QThread(parent), connection(mc)
{ }

// ----------------- ServiceGatherer impl -----------------

ZConfLib::Ptr ServiceGatherer::lib()
{
    return serviceBrowser->mainConnection->lib;
}

QString ServiceGatherer::fullName(){
    return currentService->fullName();
}

bool ServiceGatherer::enactServiceChange()
{
    if (DEBUG_ZEROCONF)
        qDebug() << "ServiceGatherer::enactServiceChange() for service "
                 << currentService->fullName();
    if (currentServiceCanBePublished()) {
        if ((publishedService.data() == 0 && currentService == 0)
                || (publishedService.data() != 0 && currentService != 0
                    && *publishedService == *currentService))
            return false;
        Service::Ptr nService = Service::Ptr(currentService);
        if (publishedService) {
            publishedService->invalidate();
            serviceBrowser->nextActiveServices.removeOne(publishedService);
            serviceBrowser->serviceRemoved(publishedService, serviceBrowser->q);
        }
        serviceBrowser->serviceChanged(publishedService, nService, serviceBrowser->q);
        publishedService = nService;
        if (nService) {
            serviceBrowser->nextActiveServices.append(nService);
            serviceBrowser->serviceAdded(nService, serviceBrowser->q);
            currentService = new Service(*currentService);
        }
        return true;
    }
    return false;
}

void ServiceGatherer::retireService()
{
    if (publishedService) {
        if (DEBUG_ZEROCONF)
            qDebug() << "ServiceGatherer::retireService() for service "
                     << currentService->fullName();
        Service::Ptr nService;
        serviceBrowser->nextActiveServices.removeOne(publishedService);
        publishedService->invalidate();
        serviceBrowser->serviceRemoved(publishedService, serviceBrowser->q);
        serviceBrowser->serviceChanged(publishedService, nService, serviceBrowser->q);
        publishedService = nService;
    } else if (DEBUG_ZEROCONF){
        qDebug() << "ServiceGatherer::retireService() for non published service "
                 << currentService->fullName();
    }
}

void ServiceGatherer::stopResolve(ZK_IP_Protocol protocol)
{
    if ((protocol == ZK_PROTO_IPv4_OR_IPv6 || protocol == ZK_PROTO_IPv4)
            && (status & ResolveConnectionActive) != 0) {
        QMutexLocker l(serviceBrowser->mainConnection->lock());
        if (serviceBrowser->mainConnection->status() < MainConnection::Stopping)
            lib()->refDeallocate(resolveConnection);
        status &= ~ResolveConnectionActive;
        serviceBrowser->updateFlowStatusForCancel();
    }
    if ((protocol == ZK_PROTO_IPv4_OR_IPv6 || protocol == ZK_PROTO_IPv6)
            && (status & ResolveConnectionV6Active) != 0) {
        QMutexLocker l(serviceBrowser->mainConnection->lock());
        if (serviceBrowser->mainConnection->status() < MainConnection::Stopping)
            lib()->refDeallocate(resolveConnectionV6);
        status &= ~ResolveConnectionV6Active;
        serviceBrowser->updateFlowStatusForCancel();
    }
}

void ServiceGatherer::restartResolve(ZK_IP_Protocol protocol)
{
    stopResolve(protocol);
    if (protocol == ZK_PROTO_IPv6) {
        if (currentService->host()) {
            QList<QHostAddress> addrNow = currentService->host()->addresses();
            QMutableListIterator<QHostAddress> addr(addrNow);
            bool changed = false;
            while (addr.hasNext()) {
                if (addr.next().protocol() == QAbstractSocket::IPv6Protocol) {
                    addr.remove();
                    changed = true;
                }
            }
            if (changed)
                currentService->m_host->setAddresses(addrNow);
        }
        DNSServiceErrorType err = lib()->resolve(
                    serviceBrowser->mainRef(),
                    &resolveConnectionV6,
                    currentService->interfaceNr(), protocol,
                    currentService->name().toUtf8().constData(),
                    currentService->type().toUtf8().constData(),
                    currentService->domain().toUtf8().constData(), this);
        if (err != kDNSServiceErr_NoError) {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                     << " failed IPv6 discovery of service " << currentService->fullName()
                     << " due to error " << err;
            status = status | ResolveConnectionV6Failed;
        } else {
            status = ((status & ~ResolveConnectionV6Failed) | ResolveConnectionV6Active);
        }
    } else {
        if (currentService->host()) {
            if (protocol == ZK_PROTO_IPv4_OR_IPv6) {
                currentService->m_host->setAddresses(QList<QHostAddress>());
            } else {
                QList<QHostAddress> addrNow = currentService->host()->addresses();
                QMutableListIterator<QHostAddress> addr(addrNow);
                bool changed = false;
                while (addr.hasNext()) {
                    if (addr.next().protocol() == QAbstractSocket::IPv4Protocol) {
                        addr.remove();
                        changed = true;
                    }
                }
                if (changed)
                    currentService->m_host->setAddresses(addrNow);
            }
        }
        DNSServiceErrorType err = lib()->resolve(
                    serviceBrowser->mainRef(),
                    &resolveConnection,
                    currentService->interfaceNr(), protocol,
                    currentService->name().toUtf8().constData(),
                    currentService->type().toUtf8().constData(),
                    currentService->domain().toUtf8().constData(), this);
        if (err != kDNSServiceErr_NoError) {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                     << " failed discovery of service " << currentService->fullName()
                     << " due to error " << err;
            status = status | ResolveConnectionFailed;
        } else {
            status = ((status & ~ResolveConnectionFailed) | ResolveConnectionActive);
        }
    }
}

void ServiceGatherer::stopTxt()
{
    if ((status & TxtConnectionActive) == 0) return;
    QMutexLocker l(serviceBrowser->mainConnection->lock());
    if (serviceBrowser->mainConnection->status() < MainConnection::Stopping)
        lib()->refDeallocate(txtConnection);
    status &= ~TxtConnectionActive;
    serviceBrowser->updateFlowStatusForCancel();
}

void ServiceGatherer::restartTxt()
{
    stopTxt();
    DNSServiceErrorType err = lib()->queryRecord(serviceBrowser->mainRef(), &txtConnection,
                                                 currentService->interfaceNr(),
                                                 currentService->fullName().toUtf8().constData(),
                                                 this);

    if (err != kDNSServiceErr_NoError) {
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                 << " failed query of TXT record of service " << currentService->fullName()
                 << " due to error " << err;
        status = status | TxtConnectionFailed;
    } else {
        status = ((status & ~TxtConnectionFailed) | TxtConnectionActive);
    }
}

void ServiceGatherer::stopHostResolution()
{
    if ((status & AddrConnectionActive) == 0) return;
    QMutexLocker l(serviceBrowser->mainConnection->lock());
    if (serviceBrowser->mainConnection->status() < MainConnection::Stopping)
        lib()->refDeallocate(addrConnection);
    status &= ~AddrConnectionActive;
    serviceBrowser->updateFlowStatusForCancel();
}

void ServiceGatherer::restartHostResolution()
{
    stopHostResolution();
    if (DEBUG_ZEROCONF)
        qDebug() << "ServiceGatherer::restartHostResolution for host " << hostName << " service "
                 << currentService->fullName();
    if (hostName.isEmpty()){
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                 << " cannot start host resolution without hostname for service "
                 << currentService->fullName();
    }
    DNSServiceErrorType err = lib()->getAddrInfo(serviceBrowser->mainRef(), &addrConnection,
                                                 currentService->interfaceNr(),
                                                 0 /* kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6 */,
                                                 hostName.toUtf8().constData(), this);

    if (err != kDNSServiceErr_NoError) {
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                 << " failed starting resolution of host " << hostName << " for service "
                 << currentService->fullName() << " due to error " << err;
        status = status | AddrConnectionFailed;
    } else {
        status = ((status & ~AddrConnectionFailed) | AddrConnectionActive);
    }
}

/// if the current service can be added
bool ServiceGatherer::currentServiceCanBePublished()
{
    return (currentService->host() && !currentService->host()->addresses().isEmpty())
            || !serviceBrowser->requireAddresses;
}

ServiceGatherer::ServiceGatherer(const QString &newServiceName, const QString &newType,
                                 const QString &newDomain, const QString &fullName,
                                 uint32_t interfaceIndex, ZK_IP_Protocol protocol,
                                 ServiceBrowserPrivate *serviceBrowser):
    serviceBrowser(serviceBrowser), publishedService(0), currentService(new Service()), status(0)
{
    if (DEBUG_ZEROCONF)
        qDebug() << " creating ServiceGatherer(" << newServiceName << ", " << newType << ", "
                 << newDomain << ", " << fullName << ", " << interfaceIndex << ", "
                 << ((size_t) serviceBrowser);
    currentService->m_name = newServiceName;
    currentService->m_type = newType;
    currentService->m_domain = newDomain;
    currentService->m_fullName = fullName;
    currentService->m_interfaceNr = interfaceIndex;
    if (fullName.isEmpty())
        currentService->m_fullName = toFullNameC(currentService->name().toUtf8().data(),
                                                 currentService->type().toUtf8().data(),
                                                 currentService->domain().toUtf8().data());
    restartResolve(protocol);
    restartTxt();
}

ServiceGatherer::~ServiceGatherer()
{
    stopHostResolution();
    stopResolve();
    stopTxt();
    delete currentService;
}

ServiceGatherer::Ptr ServiceGatherer::createGatherer(
        const QString &newServiceName, const QString &newType, const QString &newDomain,
        const QString &fullName, uint32_t interfaceIndex, ZK_IP_Protocol protocol,
        ServiceBrowserPrivate *serviceBrowser)
{
    Ptr res(new ServiceGatherer(newServiceName, newType, newDomain, fullName, interfaceIndex,
                                protocol, serviceBrowser));
    res->self = res.toWeakRef();
    return res;
}

ServiceGatherer::Ptr ServiceGatherer::gatherer() {
    return self.toStrongRef();
}

void ServiceGatherer::serviceResolveReply(DNSServiceFlags                     flags,
                                          uint32_t                            interfaceIndex,
                                          DNSServiceErrorType                 errorCode,
                                          const char                          *hosttarget,
                                          const QString                       &port,
                                          uint16_t                            txtLen,
                                          const unsigned char                 *rawTxtRecord)
{
    if (errorCode != kDNSServiceErr_NoError){
        if (errorCode == kDNSServiceErr_Timeout){
            if ((status & ResolveConnectionSuccess) == 0){
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                         << " failed service resolution for service "
                         << currentService->fullName() << " as it did timeout";
                status |= ResolveConnectionFailed;
            }
        } else {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                     << " failed service resolution for service " << currentService->fullName()
                     << " with error " << errorCode;
            status |= ResolveConnectionFailed;
        }
        if (status & ResolveConnectionActive) {
            status &= ~ResolveConnectionActive;
            lib()->refDeallocate(resolveConnection);
            serviceBrowser->updateFlowStatusForCancel();
        }
        return;
    }
    serviceBrowser->updateFlowStatusForFlags(flags);
    uint16_t nKeys = txtRecordGetCount(txtLen, rawTxtRecord);
    for (uint16_t i = 0; i < nKeys; ++i){
        enum { maxTxtLen = 256 };
        char keyBuf[maxTxtLen];
        uint8_t valLen;
        const char *valueCStr;
        DNSServiceErrorType txtErr = txtRecordGetItemAtIndex(
                    txtLen, rawTxtRecord, i, maxTxtLen, keyBuf, &valLen, (const void **)&valueCStr);
        if (txtErr != kDNSServiceErr_NoError){
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " error " << txtErr
                     << " decoding txt record of service " << currentService->fullName();
            break;
        }
        keyBuf[maxTxtLen-1] = 0; // just to be sure
        currentService->m_txtRecord.insert(QString::fromUtf8(keyBuf),
                                           QString::fromUtf8(valueCStr, valLen));
    }
    currentService->m_interfaceNr = interfaceIndex;
    currentService->m_port = port;
    if (hostName != QString::fromUtf8(hosttarget)) {
        hostName = QString::fromUtf8(hosttarget);
        if (!currentService->host())
            currentService->m_host = new QHostInfo();
        else
            currentService->m_host->setAddresses(QList<QHostAddress>());
        currentService->m_host->setHostName(hostName);
        if (serviceBrowser->autoResolveAddresses)
            restartHostResolution();
    }
    if (currentServiceCanBePublished())
        serviceBrowser->pendingGathererAdd(gatherer());
}

void ServiceGatherer::txtRecordReply(DNSServiceFlags                     flags,
                                     DNSServiceErrorType                 errorCode,
                                     uint16_t                            txtLen,
                                     const void                          *rawTxtRecord,
                                     uint32_t                            /*ttl*/)
{
    if (errorCode != kDNSServiceErr_NoError){
        if (errorCode == kDNSServiceErr_Timeout){
            if ((status & TxtConnectionSuccess) == 0){
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                         << " failed txt gathering for service " << currentService->fullName()
                         << " as it did timeout";
                status |= TxtConnectionFailed;
            }
        } else {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                     << " failed txt gathering for service " << currentService->fullName()
                     << " with error " << errorCode;
            status |= TxtConnectionFailed;
        }
        if (status & TxtConnectionActive) {
            status &= ~TxtConnectionActive;
            lib()->refDeallocate(txtConnection);
            serviceBrowser->updateFlowStatusForCancel();
        }
        return;
    }
    serviceBrowser->updateFlowStatusForFlags(flags);

    uint16_t nKeys = txtRecordGetCount(txtLen, rawTxtRecord);
    for (uint16_t i = 0; i < nKeys; ++i){
        char keyBuf[256];
        uint8_t valLen;
        const char *valueCStr;
        DNSServiceErrorType txtErr = txtRecordGetItemAtIndex(txtLen, rawTxtRecord, i, 256, keyBuf,
                                                             &valLen, (const void **)&valueCStr);
        if (txtErr != kDNSServiceErr_NoError){
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " error " << txtErr
                     << " decoding txt record of service " << currentService->fullName();
            if ((flags & kDNSServiceFlagsAdd) == 0)
                currentService->m_txtRecord.clear();
            break;
        }
        keyBuf[255] = 0; // just to be sure
        if (flags & kDNSServiceFlagsAdd) {
            currentService->m_txtRecord.insert(QString::fromUtf8(keyBuf),
                                               QString::fromUtf8(valueCStr, valLen));
        } else {
            currentService->m_txtRecord.remove(QString::fromUtf8(keyBuf)); // check value???
        }
    }
    if ((flags & kDNSServiceFlagsAdd) != 0)
        status |= TxtConnectionSuccess;
    if (currentService->m_txtRecord.count() != 0 && currentServiceCanBePublished())
        serviceBrowser->pendingGathererAdd(gatherer());
}

void ServiceGatherer::txtFieldReply(DNSServiceFlags                     flags,
                                    DNSServiceErrorType                 errorCode,
                                    uint16_t                            txtLen,
                                    const void                          *rawTxtRecord,
                                    uint32_t                            /*ttl*/){
    if (errorCode != kDNSServiceErr_NoError){
        if (errorCode == kDNSServiceErr_Timeout){
            if ((status & TxtConnectionSuccess) == 0){
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                         << " failed txt gathering for service " << currentService->fullName()
                         << " as it did timeout";
                status |= TxtConnectionFailed;
            }
        } else {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                     << " failed txt gathering for service " << currentService->fullName()
                     << " with error " << errorCode;
            status |= TxtConnectionFailed;
        }
        if (status & TxtConnectionActive) {
            status &= ~TxtConnectionActive;
            lib()->refDeallocate(txtConnection);
            serviceBrowser->updateFlowStatusForCancel();
        }
        return;
    }
    serviceBrowser->updateFlowStatusForFlags(flags);
    uint16_t keyLen = 0;
    const char *txt = reinterpret_cast<const char *>(rawTxtRecord);
    while (keyLen < txtLen) {
        if (txt[keyLen]=='=')
            break;
        ++keyLen;
    }
    if (flags & kDNSServiceFlagsAdd) {
        currentService->m_txtRecord.insert(
                    QString::fromUtf8(txt, keyLen),
                    QString::fromUtf8(txt + keyLen + 1, ((txtLen > keyLen)?txtLen - keyLen - 1:0)));
    } else {
        currentService->m_txtRecord.remove(QString::fromUtf8(txt, keyLen)); // check value???
    }

}

void ServiceGatherer::addrReply(DNSServiceFlags                  flags,
                                DNSServiceErrorType              errorCode,
                                const char                       *hostname,
                                const struct sockaddr            *address,
                                uint32_t                         /*ttl*/) // should we use this???
{
    if (errorCode != kDNSServiceErr_NoError) {
        if (errorCode == kDNSServiceErr_Timeout){
            if ((status & AddrConnectionSuccess) == 0){
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                         << " failed address resolve for service " << currentService->fullName()
                         << " as it did timeout";
                status |= AddrConnectionFailed;
            }
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType
                     << " failed addr resolve for service " << currentService->fullName()
                     << " with error " << errorCode;
            status |= AddrConnectionFailed;
        }
        if (status & AddrConnectionActive){
            status &= ~AddrConnectionActive;
            lib()->refDeallocate(addrConnection);
            serviceBrowser->updateFlowStatusForCancel();
        }
        return;
    }
    serviceBrowser->updateFlowStatusForFlags(flags);
    if (!currentService->host())
        currentService->m_host = new QHostInfo();
    if (currentService->host()->hostName() != QString::fromUtf8(hostname)) {
        if ((flags & kDNSServiceFlagsAdd) == 1)
            currentService->m_host->setHostName(QString::fromUtf8(hostname));
        if (currentService->host()->addresses().isEmpty()) {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " for service "
                     << currentService->fullName() << " add with name " << hostname
                     << " while old name " << currentService->host()->hostName()
                     << " has still adresses, removing them";
            currentService->m_host->setAddresses(QList<QHostAddress>());
        } else {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " for service "
                     << currentService->fullName() << " ignoring remove for " << hostname
                     << " as current hostname is " << currentService->host()->hostName();
            return;
        }
    }
    QHostAddress newAddr(address);
    QList<QHostAddress> addrNow = currentService->host()->addresses();
    if ((flags & kDNSServiceFlagsAdd) == 0) {
        if (addrNow.removeOne(newAddr))
            currentService->m_host->setAddresses(addrNow);
    } else {
        if (!addrNow.contains(newAddr)){
            switch (newAddr.protocol()){
#ifdef Q_OS_WIN
            case QAbstractSocket::IPv4Protocol: // prefers IPv4 addresses
#else
            case QAbstractSocket::IPv6Protocol: // prefers IPv6 addresses
#endif
                addrNow.insert(0, newAddr);
                break;
            default:
                addrNow.append(newAddr);
                break;
            }
            currentService->m_host->setAddresses(addrNow);
        }
    }
    serviceBrowser->pendingGathererAdd(gatherer());
}

void ServiceGatherer::maybeRemove()
{
    // could trigger an update, but for now we just ignore it (less chatty)
}

void ServiceGatherer::stop(ZK_IP_Protocol protocol)
{
    if ((protocol == ZK_PROTO_IPv4_OR_IPv6 || protocol == ZK_PROTO_IPv4)
            && (status & ResolveConnectionActive) != 0){
        status &= ~ResolveConnectionActive;
        lib()->refDeallocate(resolveConnection);
        serviceBrowser->updateFlowStatusForCancel();
    }
    if ((protocol == ZK_PROTO_IPv4_OR_IPv6 || protocol == ZK_PROTO_IPv6)
            && (status & ResolveConnectionV6Active) != 0){
        status &= ~ResolveConnectionV6Active;
        lib()->refDeallocate(resolveConnectionV6);
        serviceBrowser->updateFlowStatusForCancel();
    }
    if (status & TxtConnectionActive){
        status &= ~TxtConnectionActive;
        lib()->refDeallocate(txtConnection);
        serviceBrowser->updateFlowStatusForCancel();
    }
    if (status & AddrConnectionActive) {
        status &= ~AddrConnectionActive;
        lib()->refDeallocate(addrConnection);
        serviceBrowser->updateFlowStatusForCancel();
    }
}

void ServiceGatherer::reload(qint32 interfaceIndex, ZK_IP_Protocol protocol)
{
    this->interfaceIndex = interfaceIndex;
    stop(protocol);
    restartResolve(protocol);
    restartTxt();
    if (currentServiceCanBePublished()) // avoid???
        restartHostResolution();
}

void ServiceGatherer::remove()
{
    stop();
    if (serviceBrowser->gatherers.contains(currentService->fullName())
            && serviceBrowser->gatherers[currentService->fullName()] == this)
    {
        serviceBrowser->gatherers.remove(currentService->fullName());
    }
}
/// forces a full reload of the record
void ServiceGatherer::reconfirm()
{
    stop();
    /*        DNSServiceErrorType err = DNSServiceReconfirmRecord(kDNSServiceFlagsShareConnection,
                                                        interfaceIndex, fullName.toUtf8().constData(),
                                                        kDNSServiceType_PTR, // kDNSServiceType_SRV could be another possibility
                                                        kDNSServiceClass_IN //,
                                                        // uint16_t                           rdlen, // not cached... what is the best solution???
                                                        // const void                         *rdata
                                                        ); */
}

// ----------------- ServiceBrowserPrivate impl -----------------

ZConfLib::ConnectionRef ServiceBrowserPrivate::mainRef()
{
    return mainConnection->mainRef();
}

void ServiceBrowserPrivate::updateFlowStatusForCancel()
{
    mainConnection->updateFlowStatusForCancel();
}

void ServiceBrowserPrivate::updateFlowStatusForFlags(DNSServiceFlags flags)
{
    mainConnection->updateFlowStatusForFlags(flags);
}

void ServiceBrowserPrivate::pendingGathererAdd(ServiceGatherer::Ptr gatherer)
{
    int ng = pendingGatherers.count();
    for (int i = 0; i < ng; ++i){
        const ServiceGatherer::Ptr &g = pendingGatherers.at(i);
        if (g->fullName() == gatherer->fullName()){
            if (g != gatherer){
                gatherer->publishedService = g->publishedService;
                pendingGatherers[i] = gatherer;
            }
            return;
        }
    }
    pendingGatherers.append(gatherer);
}

ServiceBrowserPrivate::ServiceBrowserPrivate(const QString &serviceType, const QString &domain,
                                             bool requireAddresses, MainConnectionPtr mconn):
    q(0), serviceType(serviceType), domain(domain), mainConnection(mconn), serviceConnection(0), flags(0), interfaceIndex(0),
    delayDeletesUntil(std::numeric_limits<qint64>::min()), failed(false), browsing(false),
    autoResolveAddresses(requireAddresses), requireAddresses(requireAddresses), shouldRefresh(false)
{
}

ServiceBrowserPrivate::~ServiceBrowserPrivate()
{
    if (DEBUG_ZEROCONF)
        qDebug() << "destroying ServiceBrowserPrivate " << serviceType;
    if (browsing)
        stopBrowsing();
    if (mainConnection)
        mainConnection->removeBrowser(this);
}

void ServiceBrowserPrivate::insertGatherer(const QString &fullName)
{
    if (!gatherers.contains(fullName)){
        QString newServiceName, newType, newDomain;
        if (fromFullNameC(fullName.toUtf8().data(), *&newServiceName, *&newType, *&newDomain)){
            qDebug() << "Error unescaping fullname " << fullName;
        } else {
            ServiceGatherer::Ptr serviceGatherer = ServiceGatherer::createGatherer(
                        newServiceName, newType, newDomain, fullName, 0, ZK_PROTO_IPv4_OR_IPv6, this);
            gatherers[fullName] = serviceGatherer;
        }
    }
}

void ServiceBrowserPrivate::maybeUpdateLists()
{
    if (mainConnection->flowStatus != MainConnection::MoreComingRFS
            || pendingGatherers.count() > 50 || shouldRefresh)
    {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        QList<QString>::iterator i = knownServices.begin(), endi = knownServices.end();
        QMap<QString, ServiceGatherer::Ptr>::iterator j = gatherers.begin();
        bool hasServicesChanges = false;
        while (i != endi && j != gatherers.end()) {
            const QString vi = *i;
            QString vj = j.value()->fullName();
            if (vi == vj){
                ++i;
                ++j;
            } else if (vi < vj) {
                qDebug() << "ServiceBrowser " << serviceType << ", missing gatherer for " << vi;
                insertGatherer(vi);
                ++i;
            } else if (delayDeletesUntil <= now) {
                pendingGatherers.removeAll(j.value());
                j.value()->retireService();
                j = gatherers.erase(j);
                hasServicesChanges = true;
            } else {
                ++j;
            }
        }
        while (i != endi) {
            qDebug() << "ServiceBrowser " << serviceType << ", missing gatherer for " << *i;
            insertGatherer(*i);
        }
        while (j != gatherers.end()) {
            if (delayDeletesUntil <= now) {
                pendingGatherers.removeAll(j.value());
                j.value()->retireService();
                j = gatherers.erase(j);
                hasServicesChanges = true;
            } else {
                ++j;
            }
        }
        foreach (const ServiceGatherer::Ptr &g, pendingGatherers)
            if (delayDeletesUntil <= now || ! g->publishedService)
                hasServicesChanges = g->enactServiceChange() || hasServicesChanges;
        if (hasServicesChanges) {
            {
                QMutexLocker l(mainConnection->lock());
                activeServices = nextActiveServices;
            }
            emit q->servicesUpdated(q);
        }
    }
    {
        QMutexLocker l(mainConnection->lock());
        if (shouldRefresh)
            refresh();
    }
}

/// callback announcing
void ServiceBrowserPrivate::browseReply(DNSServiceFlags                     flags,
                                        uint32_t                            interfaceIndex,
                                        ZK_IP_Protocol                             protocol,
                                        DNSServiceErrorType                 errorCode,
                                        const char                          *serviceName,
                                        const char                          *regtype,
                                        const char                          *replyDomain)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "browseReply(" << ((int)flags) << ", " << interfaceIndex << ", "
                 << ((int)errorCode) << ", " << serviceName << ", " << regtype << ", "
                 << replyDomain << ")";
    if (errorCode != kDNSServiceErr_NoError){
        qDebug() << "ServiceBrowser " << serviceType << " ignoring reply due to error " << errorCode;
        return;
    }
    QString newServiceName = QString::fromUtf8(serviceName);
    QString newType = serviceType;
    QString newDomain = domain;
    if (serviceType != QString::fromUtf8(regtype)) // discard? should not happen...
        newType = QString::fromUtf8(regtype);
    if (domain != QString::fromUtf8(replyDomain))
        domain = QString::fromUtf8(replyDomain);
    QString fullName = toFullNameC(serviceName, regtype, replyDomain);
    updateFlowStatusForFlags(flags);
    if (flags & kDNSServiceFlagsAdd){
        ServiceGatherer::Ptr serviceGatherer;
        if (!gatherers.contains(fullName)){
            serviceGatherer = ServiceGatherer::createGatherer(newServiceName, newType, newDomain,
                                                              fullName, interfaceIndex, protocol,
                                                              this);
            gatherers[fullName] = serviceGatherer;
        } else {
            serviceGatherer = gatherers[fullName];
            serviceGatherer->reload(interfaceIndex, protocol);
        }
        QList<QString>::iterator pos = std::lower_bound(knownServices.begin(), knownServices.end(),
                                                        fullName);
        // could order later (more efficient, but then we have to handle eventual duplicates)
        if (pos == knownServices.end() || *pos != fullName)
            knownServices.insert(pos, fullName);
    } else {
        if (gatherers.contains(fullName))
            gatherers[fullName]->maybeRemove();
        knownServices.removeOne(fullName);
    }
    maybeUpdateLists(); // avoid?
}

void ServiceBrowserPrivate::activateAutoRefresh()
{
    emit q->activateAutoRefresh();
}

void ServiceBrowserPrivate::startBrowsing(quint32 interfaceIndex)
{
    this->interfaceIndex = interfaceIndex;
    if (failed || browsing)
        return;
    if (mainConnection.isNull()) {
        startupPhase(1, ServiceBrowser::tr("Starting Zeroconf Browsing"));
        mainConnection = MainConnectionPtr(new MainConnection(this));
    } else {
        mainConnection->addBrowser(this);
    }
}

bool ServiceBrowserPrivate::internalStartBrowsing()
{
    mainConnection->updateFlowStatusForFlags(0);
    if (failed || browsing)
        return false;
    DNSServiceErrorType err;
    err = mainConnection->lib->browse(mainRef(), &serviceConnection, interfaceIndex,
                                      serviceType.toUtf8().constData(),
                                      ((domain.isEmpty()) ? 0 : (domain.toUtf8().constData())), this);
    if (err != kDNSServiceErr_NoError){
        qDebug() << "ServiceBrowser " << serviceType << " failed initializing serviceConnection";
        return false;
    }
    browsing = true;
    if (DEBUG_ZEROCONF)
        qDebug() << "startBrowsing(" << interfaceIndex << ") for serviceType:" << serviceType
                 << " domain:" << domain;
    return true;
}

void ServiceBrowserPrivate::triggerRefresh()
{
    {
        QMutexLocker l(mainConnection->lock());
        const qint64 msecDelay = 5100;
        delayDeletesUntil = QDateTime::currentMSecsSinceEpoch() + msecDelay;
        stopBrowsing();
        shouldRefresh = true;
    }
    {
        QMutexLocker l(mainConnection->mainThreadLock());
        if (!browsing)
            refresh();
    }
}

void ServiceBrowserPrivate::refresh()
{
    const qint64 msecDelay = 500;
    delayDeletesUntil = QDateTime::currentMSecsSinceEpoch() + msecDelay;
    shouldRefresh = false;
    internalStartBrowsing();
}


void ServiceBrowserPrivate::stopBrowsing()
{
    QMutexLocker l(mainConnection->lock());
    if (browsing){
        if (serviceConnection) {
            mainConnection->lib->browserDeallocate(&serviceConnection);
            updateFlowStatusForCancel();
            serviceConnection = 0;
        }
        knownServices.clear();
        browsing = false;
    }
}

void ServiceBrowserPrivate::reconfirmService(Service::ConstPtr s)
{
    if (!s->outdated())
        mainConnection->lib->reconfirmRecord(
                    mainRef(), s->interfaceNr(), s->name().toUtf8().data(),
                    s->type().toUtf8().data(), s->domain().toUtf8().data(),
                    s->fullName().toUtf8().data());
}

/// called when a service is added removed or changes. oldService or newService might be null
/// (covers both add and remove)
void ServiceBrowserPrivate::serviceChanged(const Service::ConstPtr &oldService,
                                           const Service::ConstPtr &newService,
                                           ServiceBrowser *browser)
{
    emit q->serviceChanged(oldService, newService, browser);
}

/// called when a service is added (utility method)
void ServiceBrowserPrivate::serviceAdded(const Service::ConstPtr &service, ServiceBrowser *browser)
{
    emit q->serviceAdded(service, browser);
}

/// called when a service is removed (utility method)
void ServiceBrowserPrivate::serviceRemoved(const Service::ConstPtr &service, ServiceBrowser *browser)
{
    emit q->serviceRemoved(service, browser);
}

/// called when the list is updated (this might collect several serviceChanged signals together)
void ServiceBrowserPrivate::servicesUpdated(ServiceBrowser *browser)
{
    emit q->servicesUpdated(browser);
}

/// called to describe the current startup phase
void ServiceBrowserPrivate::startupPhase(int progress, const QString &description)
{
    emit q->startupPhase(progress, description, q);
}


/// called when there is an error message
void ServiceBrowserPrivate::errorMessage(ErrorMessage::SeverityLevel severity, const QString &msg)
{
    if (severity == ErrorMessage::FailureLevel)
        this->failed = true;
    emit q->errorMessage(severity, msg, q);
}

/// called when the browsing fails
void ServiceBrowserPrivate::hadFailure(const QList<ErrorMessage> &messages)
{
    emit q->hadFailure(messages, q);
}

void ServiceBrowserPrivate::startedBrowsing()
{
    emit q->startedBrowsing(q);
}

// ----------------- MainConnection impl -----------------

void MainConnection::stop(bool wait)
{
    {
        if (QThread::currentThread() == m_thread)
            qCritical() << "ERROR ZerocConf::MainConnection::stop called from m_thread";
        // This will most likely lock if called from the connection thread (as mainThreadLock is non recusive)
        // Making it recursive would open a hole during the startup of the thread.
        // As of now this should always be called during the destruction of a browser,
        // so from another thread.
        increaseStatusTo(Stopping);
        QMutexLocker l(mainThreadLock());
        QMutexLocker l2(lock());
    }
    if (m_mainRef) {
        lib->stopConnection(m_mainRef);
        m_mainRef = 0;
    }
    if (!m_thread)
        increaseStatusTo(Stopped);
    else if (wait && QThread::currentThread() != m_thread)
        m_thread->wait();
}

MainConnection::MainConnection(ServiceBrowserPrivate *initialBrowser):
    flowStatus(NormalRFS),
    lib(zeroConfLibInstance()->defaultLib()), m_lock(QMutex::Recursive),
    m_mainThreadLock(QMutex::NonRecursive), m_mainRef(0), m_failed(false), m_status(Starting), m_nErrs(0)
{
    if (initialBrowser)
        addBrowser(initialBrowser);
    if (lib.isNull()){
        appendError(ErrorMessage::FailureLevel, tr("Zeroconf could not load a valid library, failing."));
    } else {
        m_thread = new ConnectionThread(*this);
        m_mainThreadLock.lock();
        m_thread->start(); // delay startup??
    }
}

MainConnection::~MainConnection()
{
    gQuickStop = 1;
    stop(true);
    gQuickStop = 0;
    delete m_thread;
}

bool MainConnection::increaseStatusTo(int s)
{
    int sAtt = status();
    while (sAtt < s){
        if (m_status.testAndSetRelaxed(sAtt, s))
            return true;
        sAtt = status();
    }
    return false;
}

QMutex *MainConnection::lock()
{
    return &m_lock;
}

QMutex *MainConnection::mainThreadLock()
{
    return &m_mainThreadLock;
}

void MainConnection::waitStartup()
{
    int sAtt;
    while (true){
        {
            QMutexLocker l(lock());
            sAtt = status();
            if (sAtt >= Running)
                return;
        }
        QThread::yieldCurrentThread();
    }
}

void MainConnection::addBrowser(ServiceBrowserPrivate *browser)
{
    int actualStatus;
    QList<ErrorMessage> errs;
    {
        QMutexLocker l(lock());
        actualStatus = status();
        m_browsers.append(browser);
        errs = m_errors;
    }
    if (actualStatus == Running && browser->internalStartBrowsing())
        browser->startedBrowsing();
    bool didFail = false;
    foreach (const ErrorMessage &msg, errs) {
        browser->errorMessage(msg.severity, msg.msg);
        didFail = didFail || msg.severity == ErrorMessage::FailureLevel;
    }
    if (didFail)
        browser->hadFailure(errs);
}

void MainConnection::removeBrowser(ServiceBrowserPrivate *browser)
{
    QMutexLocker l(lock());
    m_browsers.removeOne(browser);
}

void MainConnection::updateFlowStatusForCancel(){
    flowStatus = ForceUpdateRFS;
}
void MainConnection::updateFlowStatusForFlags(DNSServiceFlags flags)
{
    if (flags & kDNSServiceFlagsMoreComing) {
        if (flowStatus == NormalRFS)
            flowStatus = MoreComingRFS;
    } else {
        flowStatus = NormalRFS;
    }
}
void MainConnection::maybeUpdateLists()
{
    foreach (ServiceBrowserPrivate *sb, m_browsers) {
        sb->maybeUpdateLists();
    }
}

void MainConnection::gotoValidLib(){
    while (lib){
        if (lib->isOk()) break;
        appendError(ErrorMessage::WarningLevel, tr("Zeroconf giving up on non working %1 (%2).")
                                .arg(lib->name()).arg(lib->errorMsg()));
        lib = lib->fallbackLib;
    }
    if (!lib) {
        appendError(ErrorMessage::FailureLevel, tr("Zeroconf has no valid library, aborting connection."));
        increaseStatusTo(Stopping);
    }
}

void MainConnection::abortLib(){
    if (!lib){
        appendError(ErrorMessage::FailureLevel, tr("Zeroconf has no valid library, aborting connection."));
        increaseStatusTo(Stopping);
    } else if (lib->fallbackLib){
        appendError(ErrorMessage::WarningLevel, tr("Zeroconf giving up on %1, switching to %2.")
                                .arg(lib->name()).arg(lib->fallbackLib->name()));
        lib = lib->fallbackLib;
        m_nErrs = 0;
        gotoValidLib();
    } else {
        appendError(ErrorMessage::FailureLevel, tr("Zeroconf giving up on %1, no fallback provided, aborting connection.")
                                .arg(lib->name()));
        increaseStatusTo(Stopping);
    }
}

void MainConnection::createConnection()
{
    gotoValidLib();
    while (status() <= Running) {
        if (!lib) {
            increaseStatusTo(Stopped);
            break;
        }
        startupPhase(m_nErrs + zeroConfLibInstance()->nFallbacksTot() - lib->nFallbacks() + 2,
                     tr("Trying %1...").arg(lib->name()));
        uint32_t version;
        uint32_t size = (uint32_t)sizeof(uint32_t);
        DNSServiceErrorType err = lib->getProperty(kDNSServiceProperty_DaemonVersion, &version, &size);
        if (err == kDNSServiceErr_NoError){
            DNSServiceErrorType error = lib->createConnection(this, &m_mainRef);
            if (error != kDNSServiceErr_NoError){
                appendError(ErrorMessage::WarningLevel, tr("Zeroconf using %1 failed the initialization of the main library connection with error %2.")
                                        .arg(lib->name()).arg(error));
                ++m_nErrs;
                if (m_nErrs > lib->nFallbacks() || !lib->isOk())
                    abortLib();
            } else {
                QList<ServiceBrowserPrivate *> waitingBrowsers;
                {
                    QMutexLocker l(lock());
                    waitingBrowsers = m_browsers;
                    increaseStatusTo(Running);
                }
                for (int i = waitingBrowsers.count(); i-- != 0; ){
                    ServiceBrowserPrivate *actualBrowser = waitingBrowsers[i];
                    if (actualBrowser && !actualBrowser->browsing
                            && actualBrowser->internalStartBrowsing())
                        actualBrowser->startedBrowsing();
                }
                break;
            }
        } else if (err == kDNSServiceErr_ServiceNotRunning) {
            appendError(ErrorMessage::WarningLevel, tr("Zeroconf using %1 failed because no daemon is running.")
                                    .arg(lib->name()));
            ++m_nErrs;
            if (m_nErrs > lib->maxErrors() || !lib->isOk()) {
                abortLib();
            } else if (lib->tryStartDaemon(this)) {
                appendError(ErrorMessage::WarningLevel, tr("Starting the Zeroconf daemon using %1 seems successful, continuing.")
                                        .arg(lib->name()));
            } else {
                appendError(ErrorMessage::WarningLevel, tr("Zeroconf using %1 failed because no daemon is running.")
                                        .arg(lib->name()));
                abortLib();
            }
        } else {
            appendError(ErrorMessage::WarningLevel, tr("Zeroconf using %1 failed getProperty call with error %2.")
                                    .arg(lib->name()).arg(err));
            abortLib();
        }
    }
    if (status() < Stopping) {
        startupPhase(zeroConfLibInstance()->nFallbacksTot() + 3, tr("Succeeded using %1.").arg(lib->name()));
        appendError(ErrorMessage::NoteLevel,
                    tr("MainConnection could successfully create a connection using %1.")
                    .arg(lib->name()));
    }
}

ZConfLib::RunLoopStatus MainConnection::handleEvent()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 nextEvent = now; // worth keeping a heap to quickly calculate this?
    foreach (ServiceBrowserPrivate *bAtt, m_browsers) {
        if (nextEvent < bAtt->delayDeletesUntil)
            nextEvent = bAtt->delayDeletesUntil;
    }
    if (nextEvent <= now)
        nextEvent = -1;
    else
        nextEvent -= now;
    maybeUpdateLists();
    ZConfLib::RunLoopStatus err = lib->processOneEvent(this, m_mainRef, nextEvent);
    if (err != ZConfLib::ProcessedOk && err != ZConfLib::ProcessedIdle) {
        qDebug() << "processOneEvent returned " << err;
        ++m_nErrs;
    } else {
        m_nErrs = 0;
    }
    return err;
}

void MainConnection::destroyConnection()
{
    // multithreading issues if we support multiple cycles of createConnection/destroyConnection
    for (int i = m_browsers.count(); i-- != 0;){
        ServiceBrowserPrivate *bAtt = m_browsers[i];
        if (bAtt->browsing)
            bAtt->stopBrowsing();
    }
    if (m_mainRef != 0)
        lib->destroyConnection(&m_mainRef);
    m_mainRef = 0;
}

void  MainConnection::handleEvents()
{
    try {
        if (!m_status.testAndSetAcquire(Starting, Started)){
            appendError(ErrorMessage::WarningLevel, tr("Zeroconf, unexpected start status, aborting."));
            increaseStatusTo(Stopped);
            return;
        }
        m_nErrs = 0;
        createConnection();
    } catch(...) {
        mainThreadLock()->unlock();
        throw;
    }
    mainThreadLock()->unlock();
    increaseStatusTo(Running);
    while (status() < Stopping) {
        QMutexLocker l(mainThreadLock());
        if (m_nErrs > 10)
            increaseStatusTo(Stopping);
        switch (handleEvent()) {
        case ZConfLib::ProcessedOk :
        case ZConfLib::ProcessedIdle :
            break;
        case ZConfLib::ProcessedError :
            ++m_nErrs;
            break;
        case ZConfLib::ProcessedFailure :
            increaseStatusTo(Stopping);
            ++m_nErrs;
            break;
        case ZConfLib::ProcessedQuit :
            increaseStatusTo(Stopping);
            break;
        default:
            appendError(ErrorMessage::FailureLevel, tr("Zeroconf detected an unexpected return status of handleEvent."));
            increaseStatusTo(Stopping);
            break;
        }
    }
    destroyConnection();
    if (m_nErrs > 0){
        QString browsersNames = (m_browsers.isEmpty() ? QString() : m_browsers.at(0)->serviceType)
                + ((m_browsers.count() > 1) ? QString::fromLatin1(",...") : QString());
        if (isOk())
            appendError(ErrorMessage::FailureLevel,
                        tr("Zeroconf for [%1] accumulated %n consecutive errors, aborting.", 0, m_nErrs)
                            .arg(browsersNames));
    }
    increaseStatusTo(Stopped);
}

ZConfLib::ConnectionRef MainConnection::mainRef()
{
    while (status() < Running){
        QThread::yieldCurrentThread();
    }
    return m_mainRef;
}

MainConnection::Status MainConnection::status()
{
#if QT_VERSION >= 0x050000
    return static_cast<MainConnection::Status>(m_status.load());
#else
    int val = m_status;
    return static_cast<MainConnection::Status>(val);
#endif
}

QList<ErrorMessage> MainConnection::errors()
{
    QMutexLocker l(lock());
    return m_errors;
}

void MainConnection::appendError(ErrorMessage::SeverityLevel severity, const QString &msg)
{
    QList<ServiceBrowserPrivate *> browsersAtt;
    QList<ErrorMessage> errs;
    {
        QMutexLocker l(lock());
        m_errors.append(ErrorMessage(severity, msg));
        errs = m_errors;
        browsersAtt = m_browsers;
        m_failed = severity == ErrorMessage::FailureLevel || m_failed;
    }
    foreach (ServiceBrowserPrivate *b, browsersAtt) {
        b->errorMessage(severity, msg);
        if (severity == ErrorMessage::FailureLevel)
            b->hadFailure(errs);
    }
}

void MainConnection::startupPhase(int progress, const QString &description)
{
    QList<ServiceBrowserPrivate *> browsersAtt;
    {
        QMutexLocker l(lock());
        browsersAtt = m_browsers;
    }
    foreach (ServiceBrowserPrivate *b, browsersAtt)
        b->startupPhase(progress, description);
}

bool MainConnection::isOk()
{
    return !m_failed;
}

// ----------------- ZConfLib impl -----------------

bool ZConfLib::tryStartDaemon(ErrorMessage::ErrorLogger * /* logger */)
{
    return false;
}

QString ZConfLib::name(){
    return QString::fromLatin1("ZeroConfLib@%1").arg(size_t(this), 0, 16);
}

ZConfLib::ZConfLib(ZConfLib::Ptr f) : fallbackLib(f), m_isOk(true), m_maxErrors(4)
{ }

ZConfLib::~ZConfLib()
{ }

bool ZConfLib::isOk()
{
    return m_isOk;
}

QString ZConfLib::errorMsg()
{
    return m_errorMsg;
}

void ZConfLib::setError(bool failure, const QString &eMsg)
{
    m_errorMsg = eMsg;
    m_isOk = !failure;
}

int ZConfLib::maxErrors() const
{
    return m_maxErrors;
}

int ZConfLib::nFallbacks() const
{
    return (m_isOk ? ((m_maxErrors > 0) ? m_maxErrors : 1) : 0)
            + (fallbackLib ? fallbackLib->nFallbacks() : 0);
}

ZConfLib::RunLoopStatus ZConfLib::processOneEvent(MainConnection *mainConnection,
                                                  ConnectionRef cRef, qint64 maxMsBlock)
{
    if (maxMsBlock < 0) { // just block
        maxMsBlock = MAX_SEC_FOR_READ * static_cast<qint64>(1000);
    }
    // some stuff could be extracted for maximal performance
    int dns_sd_fd  = (cRef ? refSockFD(cRef) : -1);
    int nfds = dns_sd_fd + 1;
    fd_set readfds;
    struct timeval tv;
    int result;
    dns_sd_fd  = (cRef ? refSockFD(cRef) : -1);
    if (dns_sd_fd < 0)
        return ProcessedError;
    nfds = dns_sd_fd + 1;
    FD_ZERO(&readfds);
    FD_SET(dns_sd_fd, &readfds);

    if (maxMsBlock > MAX_SEC_FOR_READ * static_cast<qint64>(1000)) {
        tv.tv_sec = MAX_SEC_FOR_READ;
        tv.tv_usec = 0;
    } else {
        tv.tv_sec = static_cast<time_t>(maxMsBlock / 1000);
        tv.tv_usec = static_cast<suseconds_t>((maxMsBlock % 1000) * 1000);
    }
    QMutex *lock = (mainConnection ? mainConnection->mainThreadLock() : 0);
    if (lock)
        lock->unlock();
    result = select(nfds, &readfds, (fd_set *)NULL, (fd_set *)NULL, &tv);
    if (lock)
        lock->lock();
    if (result > 0) {
        if (FD_ISSET(dns_sd_fd, &readfds))
            return processOneEventBlock(cRef);
    } else if (result == 0) {
        return ProcessedIdle;
    } else if (errno != EINTR) {
        if (DEBUG_ZEROCONF)
            qDebug() << "select() returned " << result << " errno " << errno
                     << strerror(errno);
        return ProcessedError;
    }
    return ProcessedIdle; // change? should never happen anyway
}

}

int ServiceBrowser::maxProgress() const
{
    return zeroConfLibInstance()->nFallbacksTot() + 3;
}

// namespace Internal
} // namespace ZeroConf
