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

#ifdef Q_OS_WIN32
# include <Winsock2.h>
# ifndef SHUT_RDWR
#  ifdef SD_BOTH
#    define SHUT_RDWR SD_BOTH
#  else
#    define SHUT_RDWR 2
#  endif
# endif
#else
# include <sys/socket.h>
#endif

#include "mdnsderived.h"
#include "servicebrowser_p.h"

#include <algorithm>
#include <errno.h>

#include <QtCore/QAtomicPointer>
#include <QtCore/QCoreApplication>
#include <QtCore/QGlobalStatic>
#include <QtCore/QMetaType>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtEndian>
#include <QtNetwork/QHostInfo>

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
            if (iPos < 4) {
                oldPos[iPos++] = decodedI;
            }
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
    ZConfLib *defaultLib();
    void setDefaultLib(LibUsage usage, const QString &libName, const QString &daemonPath);

private:
    QMutex m_lock;
    ZConfLib *m_defaultLib;
};

Q_GLOBAL_STATIC(ZeroConfLib, zeroConfLibInstance)

ZeroConfLib::ZeroConfLib(): m_lock(QMutex::Recursive),
    m_defaultLib(ZConfLib::createAvahiLib(QLatin1String("avahi-client"),
                 ZConfLib::createNativeLib(QLatin1String("dns_sd"),
                 ZConfLib::createEmbeddedLib(QString("mdnssd"), 0))))
{
    qRegisterMetaType<Service::ConstPtr>("ZeroConf::Service::ConstPtr");
}

ZConfLib *ZeroConfLib::defaultLib(){
    QMutexLocker l(&m_lock);
    return m_defaultLib;
}

void ZeroConfLib::setDefaultLib(LibUsage usage, const QString &libName, const QString &daemonPath){ // leaks... should be ok, switch to shared pointers???
    QMutexLocker l(&m_lock);
    switch (usage){
    case (UseNativeOnly):
        m_defaultLib = ZConfLib::createNativeLib(libName, 0);
        break;
    case (UseEmbeddedOnly):
        m_defaultLib = ZConfLib::createEmbeddedLib(daemonPath, 0);
        break;
    case (UseNativeOrEmbedded):
        m_defaultLib = ZConfLib::createNativeLib(libName, ZConfLib::createEmbeddedLib(daemonPath, 0));
        break;
    default:
        qDebug() << "invalid usage " << usage;
    }
}

} // end anonymous namespace

namespace ZeroConf {

// ----------------- Service impl -----------------
/*!
  \class ZeroConf::Service

  \brief class representing a zeroconf service

  Instances of this class are basically constant, but can be outdated. They are normally accessed through a Shared pointer.
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

QDebug operator<<(QDebug dbg, const Service &service)
{
    dbg.maybeSpace() << "Service{ name:" << service.name() << ", "
                     << "type:" << service.type() << ", domain:" << service.domain() << ", "
                     << " fullName:" << service.fullName() << ", port:" << service.port()
                     << ", txtRecord:{";
    bool first=true;
    const ServiceTxtRecord &txtRecord = service.txtRecord();
    foreach (const QString &k, txtRecord){
        if (first)
            first = false;
        else
            dbg << ", ";
        dbg << k << ":" << txtRecord.value(k);
    }
    dbg << "}, ";
    if (const QHostInfo *host = service.host()){
        dbg << "host:{" << host->hostName() << ", addresses[";
        first=true;
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
bool ServiceBrowser::startBrowsing(qint32 interfaceIndex)
{
    return d->startBrowsing(interfaceIndex);
}

/// create a new brower for the given service type
ServiceBrowser::ServiceBrowser(const QString &serviceType, const QString &domain,
        AddressesSetting addressesSetting, QObject *parent)
    : QObject(parent),
      d(new ServiceBrowserPrivate(serviceType, domain, addressesSetting == RequireAddresses,
          MainConnectionPtr()))
{
    d->q = this;
}

ServiceBrowser::ServiceBrowser(const MainConnectionPtr &mainConnection, const QString &serviceType,
        const QString &domain, AddressesSetting addressesSetting, QObject *parent)
    : QObject(parent),
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
    d->stopBrowsing();
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

// signals
/*!
  \fn void ServiceBrowser::serviceChanged(Service::ConstPtr oldService, Service::ConstPtr newService, ServiceBrowser *browser)

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
  It might collect several serviceChanged signals together, if you use the list returned by services(),
  use this signal, not serviceChanged(), serviceAdded() or serviceRemoved() to know about changes to the list.
*/

// ----------------- library initialization impl -----------------
/*!
  Intializes the library used for the mdns queries.
  This changes the default library used by the next MainConnection, it does not change the already instantiated
  connections.
  \a usage can decide which libraries are tried,
  \a libName should be the name (or path) to the libdns library,
  \a daemonPath is the path to the daemon executable which should be started by the embedded library if no daemon
  is found.

  \threadsafe
*/
void initLib(LibUsage usage, const QString &libName, const QString &daemonPath)
{
    zeroConfLibInstance()->setDefaultLib(usage, libName, daemonPath);
}

namespace Internal {
// ----------------- dns-sd C callbacks -----------------

extern "C" void cServiceResolveReply(DNSServiceRef                       sdRef,
                                     DNSServiceFlags                     flags,
                                     uint32_t                            interfaceIndex,
                                     DNSServiceErrorType                 errorCode,
                                     const char                          *fullname,
                                     const char                          *hosttarget,
                                     uint16_t                            port,        /* In network byte order */
                                     uint16_t                            txtLen,
                                     const unsigned char                 *txtRecord,
                                     void                                *context)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "cServiceResolveReply(" << ((size_t)sdRef) << ", " << ((quint32)flags) << ", " << interfaceIndex
                 << ", " << ((int)errorCode) << ", " << fullname << ", " << hosttarget << ", " << qFromBigEndian(port) << ", "
                 << txtLen << ", '" << QString::fromUtf8((const char *)txtRecord, txtLen) << "', " << ((size_t)context);
    ServiceGatherer *ctxGatherer = reinterpret_cast<ServiceGatherer *>(context);
    if (ctxGatherer){
        if (ctxGatherer->currentService->fullName() != fullname){
            qDebug() << "ServiceBrowser " << ctxGatherer->serviceBrowser->serviceType << " for service "
                     << ctxGatherer->currentService->name() << " ignoring resolve reply for " << fullname << " vs. " << ctxGatherer->currentService->fullName();
            return;
        }
        ctxGatherer->serviceResolveReply(flags, interfaceIndex, errorCode, hosttarget, QString::number(qFromBigEndian(port)),
                                         txtLen, txtRecord);
    }
}

extern "C" void cTxtRecordReply(DNSServiceRef                       sdRef,
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
        qDebug() << "cTxtRecordReply(" << ((size_t)sdRef) << ", " << ((int)flags) << ", " << interfaceIndex
                 << ", " << ((int)errorCode) << ", " << fullname << ", " << rrtype << ", " << rrclass << ", "
                 << ", " << rdlen << QString::fromUtf8((const char *)rdata, rdlen) << "', " << ttl << ", " << ((size_t)context);
    ServiceGatherer *ctxGatherer = reinterpret_cast<ServiceGatherer *>(context);
    if (ctxGatherer){
        if (rrtype != kDNSServiceType_TXT || rrclass != kDNSServiceClass_IN) {
            qDebug() << "ServiceBrowser " << ctxGatherer->serviceBrowser->serviceType << " for service "
                     << ctxGatherer->currentService->fullName() << " received an unexpected rrtype/class:" << rrtype << "/" << rrclass;
        }
        ctxGatherer->txtRecordReply(flags, errorCode, rdlen, rdata, ttl);
    }
}

extern "C" void cAddrReply(DNSServiceRef                    sdRef,
                           DNSServiceFlags                  flags,
                           uint32_t                         interfaceIndex,
                           DNSServiceErrorType              errorCode,
                           const char                       *hostname,
                           const struct sockaddr            *address,
                           uint32_t                         ttl,
                           void                             *context)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "cAddrReply(" << ((size_t)sdRef) << ", " << ((int)flags) << ", " << interfaceIndex
                 << ", " << ((int)errorCode) << ", " << hostname << ", " << QHostAddress(address).toString() << ", " << ttl << ", "
                 << ((size_t)context);
    ServiceGatherer *ctxGatherer = reinterpret_cast<ServiceGatherer *>(context);
    if (ctxGatherer){
        ctxGatherer->addrReply(flags, errorCode, hostname, address, ttl);
    }
}

/// callback for service browsing
extern "C" void cBrowseReply(DNSServiceRef       sdRef,
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
                 << ", " << ((int)errorCode) << ", " << serviceName << ", " << regtype << ", " << replyDomain << ", "
                 << ((size_t)context);
    ServiceBrowserPrivate *sb = (ServiceBrowserPrivate *)(context);
    if (sb == 0){
        qDebug() << "ServiceBrowser ignoring reply because context was null ";
        return;
    }
    sb->browseReply(flags, interfaceIndex, errorCode, serviceName, regtype, replyDomain);
}

// ----------------- ConnectionThread impl -----------------

void ConnectionThread::run()
{
    connection.handleEvents();
}

ConnectionThread::ConnectionThread(MainConnection &mc, QObject *parent):
    QThread(parent), connection(mc)
{ }

// ----------------- ServiceGatherer impl -----------------

ZConfLib *ServiceGatherer::lib()
{
    return serviceBrowser->mainConnection->lib;
}

QString ServiceGatherer::fullName(){
    return currentService->fullName();
}

void ServiceGatherer::enactServiceChange()
{
    if (DEBUG_ZEROCONF)
        qDebug() << "ServiceGatherer::enactServiceChange() for service " << currentService->fullName();
    if (currentServiceCanBePublished()) {
        Service::Ptr nService = Service::Ptr(currentService);
        serviceBrowser->serviceChanged(publishedService, nService, serviceBrowser->q);
        if (publishedService) {
            serviceBrowser->nextActiveServices.removeOne(publishedService);
            serviceBrowser->serviceRemoved(publishedService, serviceBrowser->q);
        }
        publishedService = nService;
        if (nService) {
            serviceBrowser->nextActiveServices.append(nService);
            serviceBrowser->serviceAdded(nService, serviceBrowser->q);
            currentService = new Service(*currentService);
        }
    }
}

void ServiceGatherer::retireService()
{
    if (publishedService) {
        Service::Ptr nService;
        serviceBrowser->nextActiveServices.removeOne(publishedService);
        serviceBrowser->serviceChanged(publishedService, nService, serviceBrowser->q);
        serviceBrowser->serviceRemoved(publishedService, serviceBrowser->q);
        publishedService = nService;
    }
}

void ServiceGatherer::stopResolve()
{
    if ((status & ResolveConnectionActive) == 0) return;
    lib()->refDeallocate(resolveConnection);
    status &= ~ResolveConnectionActive;
    serviceBrowser->updateFlowStatusForCancel();
}

void ServiceGatherer::restartResolve()
{
    stopResolve();
    DNSServiceErrorType err = lib()->resolve(
                serviceBrowser->mainRef(),
                &resolveConnection,
                currentService->interfaceNr(), currentService->name().toUtf8().constData(),
                currentService->type().toUtf8().constData(), currentService->domain().toUtf8().constData(), this);
    if (err != kDNSServiceErr_NoError) {
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed discovery of service " << currentService->fullName()
                 << " due to error " << err;
        status = status | ResolveConnectionFailed;
    } else {
        status = ((status & ~ResolveConnectionFailed) | ResolveConnectionActive);
    }
}

void ServiceGatherer::stopTxt()
{
    if ((status & TxtConnectionActive) == 0) return;
    lib()->refDeallocate(txtConnection);
    status &= ~TxtConnectionActive;
    serviceBrowser->updateFlowStatusForCancel();
}

void ServiceGatherer::restartTxt()
{
    stopTxt();
    DNSServiceErrorType err = lib()->queryRecord(serviceBrowser->mainRef(), &txtConnection,
                                                 currentService->interfaceNr(), currentService->fullName().toUtf8().constData(), this);

    if (err != kDNSServiceErr_NoError) {
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed query of TXT record of service " << currentService->fullName()
                 << " due to error " << err;
        status = status | TxtConnectionFailed;
    } else {
        status = ((status & ~TxtConnectionFailed) | TxtConnectionActive);
    }
}

void ServiceGatherer::stopHostResolution()
{
    if ((status & AddrConnectionActive) == 0) return;
    lib()->refDeallocate(addrConnection);
    status &= ~AddrConnectionActive;
    serviceBrowser->updateFlowStatusForCancel();
}

void ServiceGatherer::restartHostResolution()
{
    stopHostResolution();
    if (DEBUG_ZEROCONF)
        qDebug() << "ServiceGatherer::restartHostResolution for host " << hostName << " service " << currentService->fullName();
    if (hostName.isEmpty()){
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " cannot start host resolution without hostname for service "
                 << currentService->fullName();
    }
    DNSServiceErrorType err = lib()->getAddrInfo(serviceBrowser->mainRef(), &addrConnection,
                                                 currentService->interfaceNr(), 0 /* kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6 */,
                                                 hostName.toUtf8().constData(), this);

    if (err != kDNSServiceErr_NoError) {
        qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed starting resolution of host "
                 << hostName << " for service " << currentService->fullName() << " due to error " << err;
        status = status | AddrConnectionFailed;
    } else {
        status = ((status & ~AddrConnectionFailed) | AddrConnectionActive);
    }
}

/// if the current service can be added
bool ServiceGatherer::currentServiceCanBePublished()
{
    return (currentService->host() && !currentService->host()->addresses().isEmpty()) || !serviceBrowser->requireAddresses;
}

ServiceGatherer::ServiceGatherer(const QString &newServiceName, const QString &newType, const QString &newDomain,
                                 const QString &fullName, uint32_t interfaceIndex, ServiceBrowserPrivate *serviceBrowser):
    serviceBrowser(serviceBrowser), publishedService(0), currentService(new Service()), status(0)
{
    if (DEBUG_ZEROCONF)
        qDebug() << " creating ServiceGatherer(" << newServiceName << ", " << newType << ", " << newDomain << ", "
                 << fullName << ", " << interfaceIndex << ", " << ((size_t) serviceBrowser);
    currentService->m_name = newServiceName;
    currentService->m_type = newType;
    currentService->m_domain = newDomain;
    currentService->m_fullName = fullName;
    currentService->m_interfaceNr = interfaceIndex;
    if (fullName.isEmpty())
        currentService->m_fullName = toFullNameC(currentService->name().toUtf8().data(), currentService->type().toUtf8().data(), currentService->domain().toUtf8().data());
    restartResolve();
    restartTxt();
}

ServiceGatherer::~ServiceGatherer()
{
    stopHostResolution();
    stopResolve();
    stopTxt();
    delete currentService;
}

ServiceGatherer::Ptr ServiceGatherer::createGatherer(const QString &newServiceName, const QString &newType,
                                                     const QString &newDomain, const QString &fullName,
                                                     uint32_t interfaceIndex, ServiceBrowserPrivate *serviceBrowser)
{
    Ptr res(new ServiceGatherer(newServiceName, newType, newDomain, fullName, interfaceIndex, serviceBrowser));
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
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed service resolution for service "
                         << currentService->fullName() << " as it did timeout";
                status |= ResolveConnectionFailed;
            }
        } else {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed service resolution for service "
                     << currentService->fullName() << " with error " << errorCode;
            status |= ResolveConnectionFailed;
        }
        if (status & ResolveConnectionActive) {
            status &= ~ResolveConnectionActive;
            lib()->refDeallocate(resolveConnection);
            serviceBrowser->updateFlowStatusForCancel();
        }
        return;
    }
    if (publishedService) publishedService->invalidate(); // delay this to enactServiceChange?
    serviceBrowser->updateFlowStatusForFlags(flags);

    uint16_t nKeys = txtRecordGetCount(txtLen, rawTxtRecord);
    for (uint16_t i = 0; i < nKeys; ++i){
        enum { maxTxtLen= 256 };
        char keyBuf[maxTxtLen];
        uint8_t valLen;
        const char *valueCStr;
        DNSServiceErrorType txtErr = txtRecordGetItemAtIndex(txtLen, rawTxtRecord, i, maxTxtLen,
                                                                    keyBuf, &valLen, (const void **)&valueCStr);
        if (txtErr != kDNSServiceErr_NoError){
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " error " << txtErr
                     << " decoding txt record of service " << currentService->fullName();
            break;
        }
        keyBuf[maxTxtLen-1] = 0; // just to be sure
        if (flags & kDNSServiceFlagsAdd) {
            txtRecord[QString::fromUtf8(keyBuf)] = QString::fromUtf8(valueCStr, valLen);
        } else {
            txtRecord.remove(QString::fromUtf8(keyBuf)); // check value???
        }
    }
    currentService->m_interfaceNr = interfaceIndex;
    currentService->m_port = port;
    if (hostName != hosttarget) {
        hostName = QString::fromUtf8(hosttarget);
        if (!currentService->host())
            currentService->m_host = new QHostInfo();
        else
            currentService->m_host->setAddresses(QList<QHostAddress>());
        currentService->m_host->setHostName(hostName);
        if (serviceBrowser->autoResolveAddresses){
            restartHostResolution();
        }
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
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed txt gathering for service "
                         << currentService->fullName() << " as it did timeout";
                status |= TxtConnectionFailed;
            }
        } else {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed txt gathering for service "
                     << currentService->fullName() << " with error " << errorCode;
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
        DNSServiceErrorType txtErr = txtRecordGetItemAtIndex(txtLen, rawTxtRecord, i, 256, keyBuf, &valLen, (const void **)&valueCStr);
        if (txtErr != kDNSServiceErr_NoError){
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " error " << txtErr
                     << " decoding txt record of service " << currentService->fullName();
            if ((flags & kDNSServiceFlagsAdd) == 0)
                txtRecord.clear();
            break;
        }
        keyBuf[255] = 0; // just to be sure
        if (flags & kDNSServiceFlagsAdd) {
            txtRecord[QString::fromUtf8(keyBuf)] = QString::fromUtf8(valueCStr, valLen);
        } else {
            txtRecord.remove(QString::fromUtf8(keyBuf)); // check value???
        }
    }
    if ((flags & kDNSServiceFlagsAdd) != 0) {
        status |= TxtConnectionSuccess;
    }
    if (txtRecord.count() != 0 && currentServiceCanBePublished())
        serviceBrowser->pendingGathererAdd(gatherer());
}

void ServiceGatherer::addrReply(DNSServiceFlags                  flags,
                                DNSServiceErrorType              errorCode,
                                const char                       *hostname,
                                const struct sockaddr            *address,
                                uint32_t                         /*ttl*/) // should we use this???
{
    if (errorCode != kDNSServiceErr_NoError){
        if (errorCode == kDNSServiceErr_Timeout){
            if ((status & AddrConnectionSuccess) == 0){
                qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed address resolve for service "
                         << currentService->fullName() << " as it did timeout";
                status |= AddrConnectionFailed;
            }
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " failed addr resolve for service "
                     << currentService->fullName() << " with error " << errorCode;
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
        currentService->m_host=new QHostInfo();
    if (currentService->host()->hostName() != hostname) {
        if ((flags & kDNSServiceFlagsAdd) == 1)
            currentService->m_host->setHostName(QString::fromUtf8(hostname));
        if (currentService->host()->addresses().isEmpty()) {
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " for service "
                     << currentService->fullName() << " add with name " << hostname << " while old name "
                     << currentService->host()->hostName() << " has still adresses, removing them";
            currentService->m_host->setAddresses(QList<QHostAddress>());
        }
        else{
            qDebug() << "ServiceBrowser " << serviceBrowser->serviceType << " for service "
                     << currentService->fullName() << " ignoring remove for " << hostname << " as current hostname is "
                     << currentService->host()->hostName();
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
            addrNow.append(newAddr);
            currentService->m_host->setAddresses(addrNow);
        }
    }
    serviceBrowser->pendingGathererAdd(gatherer());
}

void ServiceGatherer::maybeRemove()
{
    // could trigger an update, but for now we just ignore it (less chatty)
}

void ServiceGatherer::stop()
{
    if (status & ResolveConnectionActive){
        status &= ~ResolveConnectionActive;
        lib()->refDeallocate(resolveConnection);
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

void ServiceGatherer::reload(qint32 interfaceIndex)
{
    this->interfaceIndex = interfaceIndex;
    stop();
    restartResolve();
    restartTxt();
    if (currentServiceCanBePublished()) // avoid???
        restartHostResolution();
}

void ServiceGatherer::remove()
{
    stop();
    if (serviceBrowser->gatherers.contains(currentService->fullName()) && serviceBrowser->gatherers[currentService->fullName()] == this)
        serviceBrowser->gatherers.remove(currentService->fullName());
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
        const ServiceGatherer::Ptr &g=pendingGatherers.at(i);
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

ServiceBrowserPrivate::ServiceBrowserPrivate(const QString &serviceType, const QString &domain, bool requireAddresses, MainConnectionPtr mconn):
    q(0), serviceType(serviceType), domain(domain), mainConnection(mconn), serviceConnection(0), flags(0), interfaceIndex(0),
    failed(false), browsing(false), autoResolveAddresses(requireAddresses), requireAddresses(requireAddresses)
{
}

ServiceBrowserPrivate::~ServiceBrowserPrivate()
{
    qDebug() << "destroying ServiceBrowserPrivate " << serviceType;
    if (browsing){
        stopBrowsing();
    }
    if (mainConnection){
        mainConnection->removeBrowser(this);
    }
}

void ServiceBrowserPrivate::insertGatherer(const QString &fullName)
{
    if (!gatherers.contains(fullName)){
        QString newServiceName, newType, newDomain;
        if (fromFullNameC(fullName.toUtf8().data(), *&newServiceName, *&newType, *&newDomain)){
            qDebug() << "Error unescaping fullname " << fullName;
        } else {
            ServiceGatherer::Ptr serviceGatherer = ServiceGatherer::createGatherer(newServiceName, newType, newDomain, fullName, 0, this);
            gatherers[fullName] = serviceGatherer;
        }
    }
}

void ServiceBrowserPrivate::maybeUpdateLists()
{
    if (mainConnection->flowStatus != MainConnection::MoreComingRFS || pendingGatherers.count() > 50) {
        QList<QString>::iterator i = knownServices.begin(), endi = knownServices.end();
        QMap<QString, ServiceGatherer::Ptr>::iterator j = gatherers.begin();
        while (i != endi && j != gatherers.end()) {
            const QString vi = *i;
            QString vj = (*j)->fullName();
            if (vi == vj){
                ++i;
                ++j;
            } else if (vi < vj) {
                qDebug() << "ServiceBrowser " << serviceType << ", missing gatherer for " << vi;
                insertGatherer(vi);
                ++i;
            } else {
                (*j)->retireService();
                j = gatherers.erase(j);
            }
        }
        while (i != endi) {
            qDebug() << "ServiceBrowser " << serviceType << ", missing gatherer for " << *i;
            insertGatherer(*i);
        }
        while (j != gatherers.end()) {
            (*j)->retireService();
            j = gatherers.erase(j);
        }
        foreach (const ServiceGatherer::Ptr &g, pendingGatherers)
            g->enactServiceChange();
        {
            QMutexLocker l(mainConnection->lock());
            activeServices=nextActiveServices;
        }
        emit q->servicesUpdated(q);
    }
}

/// callback announcing
void ServiceBrowserPrivate::browseReply(DNSServiceFlags                     flags,
                                        uint32_t                            interfaceIndex,
                                        DNSServiceErrorType                 errorCode,
                                        const char                          *serviceName,
                                        const char                          *regtype,
                                        const char                          *replyDomain)
{
    if (DEBUG_ZEROCONF)
        qDebug() << "browseReply(" << ((int)flags) << ", " << interfaceIndex << ", " << ((int)errorCode) << ", " << serviceName << ", "
                 << regtype << ", " << replyDomain << ")";
    if (errorCode != kDNSServiceErr_NoError){
        qDebug() << "ServiceBrowser " << serviceType << " ignoring reply due to error " << errorCode;
        return;
    }
    QString newServiceName = QString::fromUtf8(serviceName);
    QString newType = serviceType;
    QString newDomain = domain;
    if (serviceType != regtype) // discard? should not happen...
        newType = QString::fromUtf8(regtype);
    if (domain != replyDomain)
        domain = QString::fromUtf8(replyDomain);
    QString fullName = toFullNameC(serviceName, regtype, replyDomain);
    updateFlowStatusForFlags(flags);
    if (flags & kDNSServiceFlagsAdd){
        ServiceGatherer::Ptr serviceGatherer;
        if (!gatherers.contains(fullName)){
            serviceGatherer = ServiceGatherer::createGatherer(newServiceName, newType, newDomain, fullName, interfaceIndex, this);
            gatherers[fullName] = serviceGatherer;
        } else {
            serviceGatherer = gatherers[fullName];
            serviceGatherer->reload(interfaceIndex);
        }
        QList<QString>::iterator pos = std::lower_bound(knownServices.begin(), knownServices.end(), fullName);
        // could order later (more efficient, but then we have to handle eventual duplicates)
        if (pos == knownServices.end() || *pos != fullName)
            knownServices.insert(pos, fullName);
    } else {
        if (gatherers.contains(fullName)){
            gatherers[fullName]->maybeRemove();
        }
        knownServices.removeOne(fullName);
    }
    maybeUpdateLists();
}

bool ServiceBrowserPrivate::startBrowsing(quint32 interfaceIndex)
{
    if (failed || browsing) return false;
    if (mainConnection.isNull())
        mainConnection = MainConnectionPtr(new MainConnection());
    mainConnection->addBrowser(this);
    DNSServiceErrorType err;
    err = mainConnection->lib->browse(mainRef(), &serviceConnection, interfaceIndex,
                                      serviceType.toUtf8().constData(), ((domain.isEmpty()) ? 0 : (domain.toUtf8().constData())), this);
    if (err != kDNSServiceErr_NoError){
        qDebug() << "ServiceBrowser " << serviceType << " failed initializing serviceConnection";
        return false;
    }
    browsing = true;
    if (DEBUG_ZEROCONF)
        qDebug() << "startBrowsing(" << interfaceIndex << ") for serviceType:" << serviceType << " domain:" << domain;
    return true;
}

void ServiceBrowserPrivate::stopBrowsing()
{
    QMutexLocker l(mainConnection->lock());
    if (browsing){
        if (serviceConnection) {
            mainConnection->lib->browserDeallocate(&serviceConnection);
            updateFlowStatusForCancel();
            serviceConnection=0;
        }
    }
}

void ServiceBrowserPrivate::reconfirmService(Service::ConstPtr s)
{
    if (!s->outdated())
        mainConnection->lib->reconfirmRecord(mainRef(), s->interfaceNr(), s->name().toUtf8().data(), s->type().toUtf8().data(),
                                             s->domain().toUtf8().data(), s->fullName().toUtf8().data());
}

/// called when a service is added removed or changes. oldService or newService might be null (covers both add and remove)
void ServiceBrowserPrivate::serviceChanged(const Service::ConstPtr &oldService, const Service::ConstPtr &newService, ServiceBrowser *browser)
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

/// called when there is an error
void ServiceBrowserPrivate::hadError(QStringList errorMsgs, bool completeFailure)
{
    if (completeFailure)
        this->failed=true;
    emit q->hadError(errorMsgs,completeFailure);
}

// ----------------- MainConnection impl -----------------

void MainConnection::stop(bool wait)
{
    if (m_status < Stopping)
        increaseStatusTo(Stopping);
    if (m_mainRef)
        lib->stopConnection(m_mainRef);
    if (!m_thread)
        increaseStatusTo(Stopped);
    else if (wait && QThread::currentThread() != m_thread)
        m_thread->wait();
}

MainConnection::MainConnection():
    lib(zeroConfLibInstance()->defaultLib()), m_lock(QMutex::Recursive), m_mainRef(0), m_failed(false), m_status(Starting), m_nErrs(0)
{
    if (lib == 0){
        qDebug() << "could not load a valid library for ZeroConf::MainConnection, failing";
    } else {
        m_thread = new ConnectionThread(*this);
        m_thread->start(); // delay startup??
    }
}

MainConnection::~MainConnection()
{
    stop();
    delete m_thread;
    // to do
}

bool MainConnection::increaseStatusTo(int s)
{
    int sAtt = m_status;
    while (sAtt < s){
        if (m_status.testAndSetRelaxed(sAtt, s))
            return true;
        sAtt = m_status;
    }
    return false;
}

QMutex *MainConnection::lock()
{
    return &m_lock;
}

void MainConnection::waitStartup()
{
    int sAtt;
    while (true){
        {
            QMutexLocker l(lock());
            sAtt = m_status;
            if (sAtt >= Running)
                return;
        }
        QThread::yieldCurrentThread();
    }
}

void MainConnection::addBrowser(ServiceBrowserPrivate *browser)
{
    waitStartup();
    QStringList errs;
    bool didFail;
    {
        QMutexLocker l(lock());
        m_browsers.append(browser);
        errs=m_errors;
        didFail=m_failed;
    }
    if (didFail || !errs.isEmpty())
        browser->hadError(errs, didFail);
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
        appendError(QStringList(tr("MainConnection giving up on non Ok lib %1 (%2)")
                                .arg(lib->name()).arg(lib->errorMsg())), false);
        lib = lib->fallbackLib;
    }
    if (!lib) {
        appendError(QStringList(tr("MainConnection has no valid library, aborting connection")
                                .arg(lib->name())), true);
        increaseStatusTo(Stopping);
    }
}

void MainConnection::abortLib(){
    if (!lib){
        appendError(QStringList(tr("MainConnection has no valid library, aborting connection")
                                .arg(lib->name())), true);
        increaseStatusTo(Stopping);
    } else if (lib->fallbackLib){
        appendError(QStringList(tr("MainConnection giving up on lib %1, switching to lib %2")
                                .arg(lib->name()).arg(lib->fallbackLib->name())), false);
        lib=lib->fallbackLib;
        m_nErrs=0;
        gotoValidLib();
    } else {
        appendError(QStringList(tr("MainConnection giving up on lib %1, no fallback provided, aborting connection")
                                .arg(lib->name())), true);
        increaseStatusTo(Stopping);
    }
}

void MainConnection::createConnection()
{
    gotoValidLib();
    while (m_status <= Running) {
        if (!lib) {
            increaseStatusTo(Stopped);
            break;
        }
        uint32_t version;
        uint32_t size=(uint32_t)sizeof(uint32_t);
        DNSServiceErrorType err=lib->getProperty(kDNSServiceProperty_DaemonVersion, &version, &size);
        if (err == kDNSServiceErr_NoError){
            DNSServiceErrorType error = lib->createConnection(&m_mainRef);
            if (error != kDNSServiceErr_NoError){
                appendError(QStringList(tr("MainConnection using lib %1 failed the initialization of mainRef with error %2")
                                        .arg(lib->name()).arg(error)),false);
                ++m_nErrs;
                if (m_nErrs > 10 || !lib->isOk())
                    abortLib();
            } else {
                increaseStatusTo(Running);
                // multithreading issues if we support multiple cycles of createConnection/destroyConnection
                for (int i = m_browsers.count(); i-- != 0; ){
                    ServiceBrowserPrivate *bAtt=m_browsers[i];
                    if (bAtt && !bAtt->browsing)
                        bAtt->startBrowsing(bAtt->interfaceIndex);
                }
                break;
            }
        } else if (err == kDNSServiceErr_ServiceNotRunning) {
            appendError(QStringList(tr("MainConnection using lib %1 failed because no daemon is running")
                                    .arg(lib->name())),false);
            if (lib->tryStartDaemon()) {
                appendError(QStringList(tr("MainConnection using lib %1 daemon starting seem successful, continuing")
                                        .arg(lib->name())),false);
            } else {
                appendError(QStringList(tr("MainConnection using lib %1 failed because no daemon is running")
                                        .arg(lib->name())),false);
                abortLib();
            }
        } else {
            appendError(QStringList(tr("MainConnection using lib %1 failed getProperty call with error %2")
                                    .arg(lib->name()).arg(err)),false);
            abortLib();
        }
    }
}

ZConfLib::ProcessStatus MainConnection::handleEvent()
{
    ZConfLib::ProcessStatus err = lib->processResult(m_mainRef);
    if (err != ZConfLib::ProcessedOk) {
        qDebug() << "processResult returned " << err;
        ++m_nErrs;
    } else {
        m_nErrs=0;
        maybeUpdateLists();
    }
    return err;
}

void MainConnection::destroyConnection()
{
    // multithreading issues if we support multiple cycles of createConnection/destroyConnection
    for (int i = m_browsers.count(); i-- != 0;){
        ServiceBrowserPrivate *bAtt=m_browsers[i];
        if (bAtt->browsing)
            bAtt->stopBrowsing();
    }
    if (m_mainRef != 0)
        lib->destroyConnection(&m_mainRef);
    m_mainRef=0;
}

void  MainConnection::handleEvents()
{
    if (!m_status.testAndSetAcquire(Starting, Started)){
        appendError(QStringList(tr("MainConnection::handleEvents called with m_status != Starting, aborting")),true);
        increaseStatusTo(Stopped);
        return;
    }
    m_nErrs = 0;
    createConnection();
    increaseStatusTo(Running);
    while (m_status < Stopping) {
        if (m_nErrs > 10)
            increaseStatusTo(Stopping);
        switch (handleEvent()) {
        case ZConfLib::ProcessedOk :
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
            appendError(QStringList(tr("MainConnection::handleEvents unexpected return status of handleEvent")),true);
            increaseStatusTo(Stopping);
            break;
        }
    }
    destroyConnection();
    if (m_nErrs > 0){
        QString browsersNames=(m_browsers.isEmpty()?QString():m_browsers.at(0)->serviceType)+((m_browsers.count()>1)?QString::fromLatin1(",..."):QString());
        appendError(QStringList(tr("MainConnection for [%1] accumulated %2 consecutive errors, aborting").arg(browsersNames).arg(m_nErrs)),true);
    }
    increaseStatusTo(Stopped);
}

ZConfLib::ConnectionRef MainConnection::mainRef()
{
    while (m_status < Running){
        QThread::yieldCurrentThread();
    }
    return m_mainRef;
}

QStringList MainConnection::errors()
{
    QMutexLocker l(lock());
    return m_errors;
}

void MainConnection::clearErrors()
{
    QMutexLocker l(lock());
    m_errors.clear();
}

void MainConnection::appendError(const QStringList &s,bool failure)
{
    QList<ServiceBrowserPrivate *> browsersAtt;
    bool didFail;
    {
        QMutexLocker l(lock());
        m_errors.append(s);
        browsersAtt=m_browsers;
        m_failed= failure || m_failed;
        didFail=m_failed;
    }
    foreach (ServiceBrowserPrivate *b,browsersAtt)
        b->hadError(s,didFail);
}

bool MainConnection::isOk()
{
    return !m_failed;
}

// ----------------- ZConfLib impl -----------------

bool ZConfLib::tryStartDaemon()
{
    return false;
}

QString ZConfLib::name(){
    return QString::fromUtf8("ZeroConfLib@%1").arg(size_t(this),0,16);
}

ZConfLib::ZConfLib(ZConfLib * f) : fallbackLib(f), m_isOk(true)
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

} // namespace Internal
} // namespace ZeroConf
