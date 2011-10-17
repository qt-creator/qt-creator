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

#ifndef SERVICEBROWSER_H
#define SERVICEBROWSER_H

#include "zeroconf_global.h"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>

QT_FORWARD_DECLARE_CLASS(QHostInfo)

namespace ZeroConf {

namespace Internal {
class ServiceGatherer;
class MainConnection;
class ServiceBrowserPrivate;
}

typedef QSharedPointer<Internal::MainConnection> MainConnectionPtr;

typedef QHash<QString, QString> ServiceTxtRecord;

class ZEROCONFSHARED_EXPORT Service
{
    friend class Internal::ServiceGatherer;

public:
    typedef QSharedPointer<const Service> ConstPtr;
    typedef QSharedPointer<Service> Ptr;

    Service(const Service &o);
    Service();
    ~Service();

    bool outdated() const { return m_outdated; }

    QString name() const { return m_name; }
    QString type() const { return m_type; }
    QString domain() const { return m_domain; }
    QString fullName() const { return m_fullName; }
    QString port() const { return m_port; }
    const ServiceTxtRecord &txtRecord() const { return m_txtRecord; }
    const QHostInfo *host() const { return m_host; }
    int interfaceNr() const { return m_interfaceNr; }

    bool invalidate() { bool res = m_outdated; m_outdated = true; return res; }
private:
    QString m_name;
    QString m_type;
    QString m_domain;
    QString m_fullName;
    QString m_port;
    ServiceTxtRecord m_txtRecord;
    QHostInfo *m_host;
    int m_interfaceNr;
    bool m_outdated;
};

QDebug operator<<(QDebug dbg, const Service &service);

class ZEROCONFSHARED_EXPORT ServiceBrowser : public QObject
{
    Q_OBJECT
    friend class Internal::ServiceBrowserPrivate;
public:
    enum AddressesSetting { RequireAddresses, DoNotRequireAddresses };

    ServiceBrowser(const QString &serviceType, const QString &domain = QLatin1String("local."),
        AddressesSetting addressesSetting = RequireAddresses, QObject *parent = 0);
    ServiceBrowser(const MainConnectionPtr &mainConnection, const QString &serviceType,
        const QString &domain = QLatin1String("local."),
        AddressesSetting addressesSetting = RequireAddresses, QObject *parent = 0);
    ~ServiceBrowser();

    MainConnectionPtr mainConnection() const;

    bool startBrowsing(qint32 interfaceIndex = 0);
    void stopBrowsing();
    bool isBrowsing() const;
    bool didFail() const;

    const QString& serviceType() const;
    const QString& domain() const;

    bool adressesAutoResolved() const;
    bool addressesRequired() const;

    QList<Service::ConstPtr> services() const;
    void reconfirmService(Service::ConstPtr service);
signals:
    void serviceChanged(const ZeroConf::Service::ConstPtr &oldService,
        const ZeroConf::Service::ConstPtr &newService, ZeroConf::ServiceBrowser *browser);
    void serviceAdded(const ZeroConf::Service::ConstPtr &service,
        ZeroConf::ServiceBrowser *browser);
    void serviceRemoved(const ZeroConf::Service::ConstPtr &service,
        ZeroConf::ServiceBrowser *browser);
    void servicesUpdated(ZeroConf::ServiceBrowser *browser);
    void hadError(QStringList errorMsgs, bool completeFailure);
private:
    Internal::ServiceBrowserPrivate *d;
};

enum LibUsage {
    UseNativeOnly = 1,
    UseEmbeddedOnly,
    UseNativeOrEmbedded
};

void initLib(LibUsage usage, const QString &libName, const QString & daemonPaths);

}

#endif // SERVICEBROWSER_H
