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

#include "servicebrowser.h"
#include "servicebrowser_p.h"

#include <QtGlobal>
#include <QDebug>
#include <QLibrary>
#include <QString>
#include <QStringList>

#ifdef Q_OS_LINUX
#define AVAHI_LIB
#endif

#ifdef AVAHI_LIB

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <netinet/in.h>
#include <sys/poll.h>

namespace ZeroConf {
namespace Internal {

extern "C" void cAvahiResolveReply(
        AvahiServiceResolver * r, AvahiIfIndex interface, AvahiProtocol /*protocol*/,
        AvahiResolverEvent event, const char *name, const char *type, const char *domain,
        const char *hostName, const AvahiAddress *address, uint16_t port, AvahiStringList *txt,
        AvahiLookupResultFlags /*flags*/, void* context);
extern "C" void cAvahiResolveEmptyReply(
        AvahiServiceResolver * r, AvahiIfIndex, AvahiProtocol, AvahiResolverEvent, const char *,
        const char *, const char *, const char *, const AvahiAddress *, uint16_t,
        AvahiStringList *, AvahiLookupResultFlags, void*);
extern "C" void cAvahiClientReply (AvahiClient * /*s*/, AvahiClientState state, void* context);
extern "C" void cAvahiBrowseReply(
        AvahiServiceBrowser * /*b*/, AvahiIfIndex interface, AvahiProtocol /*protocol*/,
        AvahiBrowserEvent event, const char *name, const char *type, const char *domain,
        AvahiLookupResultFlags /*flags*/, void* context);
extern "C" int cAvahiPollFunction(struct pollfd *ufds, unsigned int nfds, int timeout, void *userdata);

extern "C" {
typedef const AvahiPoll* (*AvahiSimplePollGet)(AvahiSimplePoll *s);
typedef AvahiSimplePoll *(*AvahiSimplePollNewPtr)(void);
typedef int (*AvahiSimplePollIteratePtr)(AvahiSimplePoll *s, int sleep_time);
typedef void (*AvahiSimplePollQuitPtr)(AvahiSimplePoll *s);
typedef void (*AvahiSimplePollFreePtr)(AvahiSimplePoll *s);
typedef void (*AvahiSimplePollSetFuncPtr)(AvahiSimplePoll *s, AvahiPollFunc func, void *userdata);
typedef AvahiClient* (*AvahiClientNewPtr)(
        const AvahiPoll *poll_api, AvahiClientFlags flags, AvahiClientCallback callback,
        void *userdata, int *error);
typedef void (*AvahiClientFreePtr)(AvahiClient *client);
typedef AvahiServiceBrowser* (*AvahiServiceBrowserNewPtr)(
        AvahiClient *client, AvahiIfIndex interface, AvahiProtocol protocol, const char *type,
        const char *domain, AvahiLookupFlags flags, AvahiServiceBrowserCallback callback,
        void *userdata);
typedef int (*AvahiServiceBrowserFreePtr)(AvahiServiceBrowser *);

typedef AvahiServiceResolver * (*AvahiServiceResolverNewPtr)(
        AvahiClient *client, AvahiIfIndex interface, AvahiProtocol protocol, const char *name,
        const char *type, const char *domain, AvahiProtocol aprotocol, AvahiLookupFlags flags,
        AvahiServiceResolverCallback callback, void *userdata);
typedef int (*AvahiServiceResolverFreePtr)(AvahiServiceResolver *r);
}

class AvahiZConfLib;

struct MyAvahiConnection
{
    AvahiClient *client;
    AvahiSimplePoll *simple_poll;
    AvahiZConfLib *lib;
    MyAvahiConnection() : client(0), simple_poll(0), lib(0)
    { }
};

// represents an avahi library
class AvahiZConfLib : public ZConfLib {
private:
    AvahiSimplePollGet m_simplePollGet;
    AvahiSimplePollNewPtr m_simplePollNew;
    AvahiSimplePollIteratePtr m_simplePollIterate;
    AvahiSimplePollQuitPtr m_simplePollQuit;
    AvahiSimplePollFreePtr m_simplePollFree;
    AvahiSimplePollSetFuncPtr m_simplePollSetFunc;
    AvahiClientNewPtr m_clientNew;
    AvahiClientFreePtr m_clientFree;
    AvahiServiceBrowserNewPtr m_serviceBrowserNew;
    AvahiServiceBrowserFreePtr m_serviceBrowserFree;
    AvahiServiceResolverNewPtr m_serviceResolverNew;
    AvahiServiceResolverFreePtr m_serviceResolverFree;
    QLibrary nativeLib;
public:

    AvahiZConfLib(const QString &libName = QLatin1String("avahi-client"),
                  const QString &version = QLatin1String("3"),
                  const ZConfLib::Ptr &fallBack = ZConfLib::Ptr(0)) :
        ZConfLib(fallBack), nativeLib(libName, version)
    {
#ifndef ZCONF_AVAHI_STATIC_LINKING
        // dynamic linking
        if (!nativeLib.load()) {
            setError(true,
                     ZConfLib::tr("AvahiZConfLib could not load the native library '%1': %2").arg(libName, nativeLib.errorString()));
        }
        m_simplePollGet = reinterpret_cast<AvahiSimplePollGet>(nativeLib.resolve("avahi_simple_poll_get"));
        m_simplePollNew = reinterpret_cast<AvahiSimplePollNewPtr>(nativeLib.resolve("avahi_simple_poll_new"));
        m_simplePollIterate = reinterpret_cast<AvahiSimplePollIteratePtr>(nativeLib.resolve("avahi_simple_poll_iterate"));
        m_simplePollQuit = reinterpret_cast<AvahiSimplePollQuitPtr>(nativeLib.resolve("avahi_simple_poll_quit"));
        m_simplePollFree = reinterpret_cast<AvahiSimplePollFreePtr>(nativeLib.resolve("avahi_simple_poll_free"));
        m_simplePollSetFunc = reinterpret_cast<AvahiSimplePollSetFuncPtr>(nativeLib.resolve("avahi_simple_poll_set_func"));
        m_clientNew = reinterpret_cast<AvahiClientNewPtr>(nativeLib.resolve("avahi_client_new"));
        m_clientFree = reinterpret_cast<AvahiClientFreePtr>(nativeLib.resolve("avahi_client_free"));
        m_serviceBrowserNew = reinterpret_cast<AvahiServiceBrowserNewPtr>(nativeLib.resolve("avahi_service_browser_new"));
        m_serviceBrowserFree = reinterpret_cast<AvahiServiceBrowserFreePtr>(nativeLib.resolve("avahi_service_browser_free"));
        m_serviceResolverNew = reinterpret_cast<AvahiServiceResolverNewPtr>(nativeLib.resolve("avahi_service_resolver_new"));
        m_serviceResolverFree = reinterpret_cast<AvahiServiceResolverFreePtr>(nativeLib.resolve("avahi_service_resolver_free"));
#else
        m_simplePollGet = reinterpret_cast<AvahiSimplePollGet>(&avahi_simple_poll_get);
        m_simplePollNew = reinterpret_cast<AvahiSimplePollNewPtr>(&avahi_simple_poll_new);
        m_simplePollIterate = reinterpret_cast<AvahiSimplePollIteratePtr>(&avahi_simple_poll_iterate);
        m_simplePollQuit = reinterpret_cast<AvahiSimplePollQuitPtr>(&avahi_simple_poll_quit);
        m_simplePollFree = reinterpret_cast<AvahiSimplePollFreePtr>(&avahi_simple_poll_free);
        m_simplePollSetFunc = reinterpret_cast<AvahiSimplePollSetFuncPtr>(&avahi_simple_poll_set_func);
        m_clientNew = reinterpret_cast<AvahiClientNewPtr>(&avahi_client_new);
        m_clientFree = reinterpret_cast<AvahiClientFreePtr>(&avahi_client_free);
        m_serviceBrowserNew = reinterpret_cast<AvahiServiceBrowserNewPtr>(&avahi_service_browser_new);
        m_serviceBrowserFree = reinterpret_cast<AvahiServiceBrowserFreePtr>(&avahi_service_browser_free);
        m_serviceResolverNew = reinterpret_cast<AvahiServiceResolverNewPtr>(&avahi_service_resolver_new);
        m_serviceResolverFree = reinterpret_cast<AvahiServiceResolverFreePtr>(&avahi_service_resolver_free);
#endif
        if (DEBUG_ZEROCONF) {
            if (!m_simplePollGet) qDebug() << name() << " has null m_simplePollGet";
            if (!m_simplePollNew) qDebug() << name() << " has null m_simplePollNew";
            if (!m_simplePollIterate) qDebug() << name() << " has null m_simplePollIterate";
            if (!m_simplePollQuit) qDebug() << name() << " has null m_simplePollQuit";
            if (!m_simplePollFree) qDebug() << name() << " has null m_simplePollFree";
            if (!m_simplePollSetFunc) qDebug() << name() << " has null m_simplePollSetFunc";
            if (!m_clientNew) qDebug() << name() << " has null m_clientNew";
            if (!m_clientFree) qDebug() << name() << " has null m_clientFree";
            if (!m_serviceBrowserNew) qDebug() << name() << " has null m_serviceBrowserNew";
            if (!m_serviceBrowserFree) qDebug() << name() << " has null m_serviceBrowserFree";
            if (!m_serviceResolverNew) qDebug() << name() << " has null m_serviceResolverNew";
            if (!m_serviceResolverFree) qDebug() << name() << " has null m_serviceResolverFree";
        }
    }

    ~AvahiZConfLib() {
    }

    QString name(){
        return QString::fromLatin1("Avahi Library");
    }

    // bool tryStartDaemon();
    void refDeallocate(DNSServiceRef /*sdRef*/) {
    }

    void browserDeallocate(BrowserRef *sdRef) {
        if (sdRef && (*sdRef) && m_serviceBrowserFree) {
            m_serviceBrowserFree(*reinterpret_cast<AvahiServiceBrowser **>(sdRef));
            *sdRef = 0;
        }
    }

    void destroyConnection(ConnectionRef *sdRef) {
        MyAvahiConnection **connection = reinterpret_cast<MyAvahiConnection **>(sdRef);
        if (connection && (*connection)) {
            if ((*connection)->client && m_clientFree)
                m_clientFree((*connection)->client);

            if ((*connection)->simple_poll && m_simplePollFree)
                m_simplePollFree((*connection)->simple_poll);
            delete(*connection);
            *connection = 0;
        }
    }

    DNSServiceErrorType resolve(ConnectionRef cRef, DNSServiceRef *sdRef, uint32_t interfaceIndex,
                                ZK_IP_Protocol protocol, const char *name, const char *regtype,
                                const char *domain, ServiceGatherer *gatherer)
    {
        if (!sdRef) {
            qDebug() << "Error: sdRef is null in resolve";
            return kDNSServiceErr_Unknown;
        }
        MyAvahiConnection *connection = reinterpret_cast<MyAvahiConnection *>(cRef);
        if (!m_serviceResolverNew)
            return kDNSServiceErr_Unknown;
        AvahiProtocol avahiProtocol = AVAHI_PROTO_UNSPEC;
        switch (protocol) {
        case ZK_PROTO_IPv4:
            avahiProtocol = AVAHI_PROTO_INET;
            break;
        case ZK_PROTO_IPv6:
            avahiProtocol = AVAHI_PROTO_INET6;
            break;
        case ZK_PROTO_IPv4_OR_IPv6:
            avahiProtocol = AVAHI_PROTO_UNSPEC;
            break;
        default:
            qDebug() << "Zeroconf: unexpected value " << static_cast<int>(protocol) <<
                        " for protocol in avahiLib.resolve";
        }
        AvahiServiceResolver *resolver = m_serviceResolverNew(
                    connection->client, interfaceIndex, avahiProtocol, name, regtype, domain,
                    avahiProtocol, AvahiLookupFlags(0), &cAvahiResolveReply, gatherer);
        //*sdRef = reinterpret_cast<DNSServiceRef>(resolver); // add for restart?
        if (!resolver)
            return kDNSServiceErr_Unknown; // avahi_strerror(avahi_client_errno(connection->client));
        return kDNSServiceErr_NoError;
    }

    DNSServiceErrorType queryRecord(ConnectionRef, DNSServiceRef *, uint32_t,
                                    const char *, ServiceGatherer *)
    {
        return kDNSServiceErr_NoError;
    }

    DNSServiceErrorType getAddrInfo(ConnectionRef, DNSServiceRef *, uint32_t,
                                    DNSServiceProtocol, const char *, ServiceGatherer *)
    {
        return kDNSServiceErr_NoError;
    }

    DNSServiceErrorType reconfirmRecord(ConnectionRef cRef, uint32_t interfaceIndex,
                                        const char *name, const char *type, const char *domain,
                                        const char * /*fullname */)
    {
        MyAvahiConnection *connection = reinterpret_cast<MyAvahiConnection *>(cRef);
        if (!connection)
            return kDNSServiceErr_Unknown;
        AvahiServiceResolver *resolver = m_serviceResolverNew(
                    connection->client, interfaceIndex, AVAHI_PROTO_INET, name, type, domain,
                    AVAHI_PROTO_INET, AvahiLookupFlags(AVAHI_LOOKUP_USE_WIDE_AREA|
                                                       AVAHI_LOOKUP_USE_MULTICAST),
                    &cAvahiResolveEmptyReply, this);
        return ((resolver == 0)?kDNSServiceErr_Unknown:kDNSServiceErr_NoError);
    }

    DNSServiceErrorType browse(ConnectionRef cRef, BrowserRef *sdRef, uint32_t interfaceIndex,
                               const char *regtype, const char *domain,
                               ServiceBrowserPrivate *browser)
    {
        if (!sdRef) {
            qDebug() << "Error: sdRef is null in browse";
            return kDNSServiceErr_Unknown;
        }
        if (!m_serviceBrowserNew)
            return kDNSServiceErr_Unknown;
        MyAvahiConnection *connection = reinterpret_cast<MyAvahiConnection *>(cRef);
        AvahiServiceBrowser *sb = m_serviceBrowserNew(
                    connection->client,
                    ((interfaceIndex == 0)?static_cast<uint32_t>(AVAHI_IF_UNSPEC):
                                         static_cast<uint32_t>(interfaceIndex)),
                    AVAHI_PROTO_UNSPEC, regtype, domain, AvahiLookupFlags(0),
                    &cAvahiBrowseReply, browser);
        *sdRef = reinterpret_cast<BrowserRef>(sb);
        if (!sb) {
            return kDNSServiceErr_Unknown;
            //avahi_strerror(avahi_client_errno(client));
        }
        browser->activateAutoRefresh();
        return kDNSServiceErr_NoError;
    }

    DNSServiceErrorType getProperty(const char * /*property*/,
                                    void       * /*result*/,
                                    uint32_t   * /*size*/
                                    )
    {
        return 0;
    }

    RunLoopStatus processOneEvent(MainConnection *, ConnectionRef cRef, qint64 maxLockMs) {
        if (!m_simplePollIterate)
            return ProcessedFailure;
        MyAvahiConnection *connection = reinterpret_cast<MyAvahiConnection *>(cRef);
        if (!connection)
            return ProcessedError;
        if (m_simplePollIterate(connection->simple_poll, static_cast<int>(maxLockMs)))
            return ProcessedError;
        return ProcessedOk;
    }

    RunLoopStatus processOneEventBlock(ConnectionRef cRef) {
        return processOneEvent(NULL,cRef,-1);
    }

    DNSServiceErrorType createConnection(MainConnection *mainConnection, ConnectionRef *sdRef) {
        if (!m_simplePollNew || !m_clientNew || !m_simplePollSetFunc)
            return kDNSServiceErr_Unknown;
        if (sdRef == 0) {
            qDebug() << "Error: sdRef is null in createConnection";
            return kDNSServiceErr_Unknown;
        }
        MyAvahiConnection *connection = new MyAvahiConnection;
        connection->lib = this;
        /* Allocate main loop object */
        connection->simple_poll = m_simplePollNew();
        if (!connection->simple_poll) {
            delete connection;
            return kDNSServiceErr_Unknown;
        }
        typedef void (*AvahiSimplePollSetFuncPtr)(AvahiSimplePoll *s, AvahiPollFunc func, void *userdata);
        m_simplePollSetFunc(connection->simple_poll, &cAvahiPollFunction, mainConnection);
        /* Allocate a new client */
        int error;
        connection->client = m_clientNew(m_simplePollGet(connection->simple_poll),
                                         AvahiClientFlags(0), &cAvahiClientReply,
                                         connection, &error);
        if (!connection->client) {
            if (m_simplePollFree)
                m_simplePollFree(connection->simple_poll);
            delete connection;
            setError(true, ZConfLib::tr("%1 cannot create a client. The daemon is probably not running.").arg(name()));
            return kDNSServiceErr_Unknown;
        }
        *sdRef = reinterpret_cast<ConnectionRef>(connection);
        return ((isOk())?kDNSServiceErr_NoError:kDNSServiceErr_Unknown);
    }

    void stopConnection(ConnectionRef cRef)
    {
        MyAvahiConnection *connection = reinterpret_cast<MyAvahiConnection *>(cRef);
        if (m_simplePollQuit && connection)
            m_simplePollQuit(connection->simple_poll);
    }

    int refSockFD(ConnectionRef) {
        return -1;
    }

    void serviceResolverFree(AvahiServiceResolver *r) {
        if (m_serviceResolverFree)
            m_serviceResolverFree(r);
    }
};

ZConfLib::Ptr ZConfLib::createAvahiLib(const QString &libName, const QString &version,
                                       const ZConfLib::Ptr &fallback)
{
    return ZConfLib::Ptr(new AvahiZConfLib(libName, version, fallback));
}

extern "C" void cAvahiResolveReply(
        AvahiServiceResolver * r, AvahiIfIndex interface, AvahiProtocol /*protocol*/,
        AvahiResolverEvent event, const char * /*name*/, const char * /*type*/,
        const char * /*domain*/, const char *hostName, const AvahiAddress *address, uint16_t port,
        AvahiStringList *txt, AvahiLookupResultFlags /*flags*/, void* context)
{
    enum { defaultTtl = 0 };
    ServiceGatherer *sg = reinterpret_cast<ServiceGatherer *>(context);
    if (!sg){
        qDebug() << "context was null in cAvahiResolveReply";
        return;
    }
    AvahiStringList *txtAtt = 0;
    const unsigned char emptyTxt[1]="";
    switch (event) {
    case AVAHI_RESOLVER_FAILURE:
        sg->serviceResolveReply(0, interface, kDNSServiceErr_Timeout, 0, QString(), 0, 0);
        break;
    case AVAHI_RESOLVER_FOUND:
        sg->serviceResolveReply(
                    kDNSServiceFlagsAdd | ((txtAtt || address)?kDNSServiceFlagsMoreComing:0),
                    interface, kDNSServiceErr_NoError, hostName, QString::number(port), 0, emptyTxt);
        txtAtt = txt;
        while (txtAtt) {
            sg->txtFieldReply(
                        kDNSServiceFlagsAdd | ((txtAtt->next || address)?kDNSServiceFlagsMoreComing:0),
                        kDNSServiceErr_NoError, static_cast<unsigned short>(txtAtt->size),
                        txtAtt->text, ~0);
            txtAtt = txtAtt->next;
        }
        if (address){
            sockaddr_in ipv4;
            sockaddr_in6 ipv6;
//#ifdef HAVE_SA_LEN
            switch (address->proto){
            case (AVAHI_PROTO_INET):
                memset(&ipv4, 0, sizeof(ipv4));
                ipv4.sin_family = AF_INET;
                memcpy(&(ipv4.sin_addr),&(address->data.ipv4.address), sizeof(ipv4.sin_addr));
                sg->addrReply(kDNSServiceFlagsAdd, kDNSServiceErr_NoError, hostName,
                              reinterpret_cast<sockaddr*>(&ipv4), defaultTtl);
                break;
            case (AVAHI_PROTO_INET6):
                memset(&ipv6, 0, sizeof(ipv6));
                ipv6.sin6_family = AF_INET6;
                memcpy(&(ipv6.sin6_addr), &(address->data.ipv6.address), sizeof(ipv6.sin6_addr));
                sg->addrReply(kDNSServiceFlagsAdd, kDNSServiceErr_NoError, hostName,
                              reinterpret_cast<sockaddr*>(&ipv6), defaultTtl);
                break;
            default:
                if (DEBUG_ZEROCONF)
                    qDebug() << "Warning: ignoring address with protocol " << address->proto
                             << " for service " << sg->fullName();
            }
        }
        break;
    default:
        qDebug() << "Error: unexpected avahi event " << event << " in cAvahiResolveReply";
        break;
    }
    ZConfLib::Ptr libBase = sg->serviceBrowser->mainConnection->lib;
    AvahiZConfLib *lib = dynamic_cast<AvahiZConfLib *>(libBase.data());
    if (lib)
        lib->serviceResolverFree(r);
}

extern "C" void cAvahiResolveEmptyReply(
        AvahiServiceResolver * r, AvahiIfIndex, AvahiProtocol, AvahiResolverEvent, const char *,
        const char *, const char *, const char *, const AvahiAddress *, uint16_t, AvahiStringList *,
        AvahiLookupResultFlags, void* context)
{
    AvahiZConfLib *lib = reinterpret_cast<AvahiZConfLib *>(context);
    if (lib)
        lib->serviceResolverFree(r);
}

extern "C" void cAvahiClientReply (AvahiClient * /*s*/, AvahiClientState state, void* context)
{
    MyAvahiConnection *connection =  reinterpret_cast<MyAvahiConnection *>(context);
    ZConfLib *lib = connection->lib;
    if (!lib) {
        qDebug() << "Error: context was null in cAvahiClientReply, ignoring state " << state;
        return;
    }
    if (DEBUG_ZEROCONF)
        qDebug() << "cAvahiClientReply called with state " << state;
    switch (state){
    case (AVAHI_CLIENT_S_REGISTERING):
        /* Server state: REGISTERING */
        break;
    case (AVAHI_CLIENT_S_RUNNING):
        /* Server state: RUNNING */
        lib->setError(false, QString());
        break;
    case (AVAHI_CLIENT_S_COLLISION):
        /* Server state: COLLISION */
        lib->setError(true, ZConfLib::tr("cAvahiClient, server collision."));
        break;
    case (AVAHI_CLIENT_FAILURE):
        lib->setError(true, ZConfLib::tr("cAvahiClient, an error occurred on the client side."));
        break;
    case (AVAHI_CLIENT_CONNECTING):
        lib->setError(false, ZConfLib::tr("cAvahiClient, still connecting, no server available."));
        break;
    default:
        lib->setError(true, ZConfLib::tr("Unexpected state %1 in cAvahiClientReply.")
                      .arg(state));
    }
}

extern "C" void cAvahiBrowseReply(
        AvahiServiceBrowser * /*b*/, AvahiIfIndex interface, AvahiProtocol avahiProtocol,
        AvahiBrowserEvent event, const char *name, const char *type, const char *domain,
        AvahiLookupResultFlags /*flags*/, void* context)
{
    ZK_IP_Protocol protocol = ZK_PROTO_IPv4_OR_IPv6;
    switch (avahiProtocol) {
        case AVAHI_PROTO_INET:
        protocol = ZK_PROTO_IPv4;
        break;
    case AVAHI_PROTO_INET6:
        protocol = ZK_PROTO_IPv6;
        break;
    case AVAHI_PROTO_UNSPEC:
        protocol = ZK_PROTO_IPv4_OR_IPv6;
        break;
    default:
        qDebug() << "Error unexpected protocol value " << avahiProtocol <<  " in cAvahiBrowseReply";
    }

    ServiceBrowserPrivate *browser = reinterpret_cast<ServiceBrowserPrivate *>(context);
    if (browser == 0)
        qDebug() << "Error context is null in cAvahiBrowseReply";
    switch (event) {
        case AVAHI_BROWSER_FAILURE:
            browser->browseReply(kDNSServiceFlagsMoreComing, 0, protocol, kDNSServiceErr_Unknown,
                                 0, 0, 0);
            break;
        case AVAHI_BROWSER_NEW:
            browser->browseReply(kDNSServiceFlagsAdd|kDNSServiceFlagsMoreComing,
                                 static_cast<uint32_t>(interface), protocol, kDNSServiceErr_NoError,
                                 name, type, domain);
            break;
        case AVAHI_BROWSER_REMOVE:
            browser->browseReply(kDNSServiceFlagsMoreComing, static_cast<uint32_t>(interface),
                                 protocol, kDNSServiceErr_NoError, name, type, domain);
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
            browser->updateFlowStatusForFlags(0);
            break;
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            browser->updateFlowStatusForFlags(0);
            break;
        default:
            browser->mainConnection->lib->setError(true, ZConfLib::tr(
                                                       "Unexpected state %1 in cAvahiBrowseReply.")
                                                   .arg(event));
    }
}

extern "C" int cAvahiPollFunction(struct pollfd *ufds, unsigned int nfds, int timeout, void *userdata)
{
    MainConnection *mainConnection = static_cast<MainConnection *>(userdata);
    QMutex *lock = 0;
    if (mainConnection)
        lock = mainConnection->mainThreadLock();
    if (lock)
        lock->unlock();
    int res=poll(ufds,nfds,timeout);
    if (lock)
        lock->lock();
    return res;
}

} // namespace Internal
} // namespace ZeroConf

#else // no native lib

namespace ZeroConf {
namespace Internal {

ZConfLib::Ptr ZConfLib::createAvahiLib(const QString &/*lib*/, const QString &/*version*/,
                                       const ZConfLib::Ptr &fallback)
{
    return fallback;
}

} // namespace Internal
} // namespace ZeroConf
#endif


