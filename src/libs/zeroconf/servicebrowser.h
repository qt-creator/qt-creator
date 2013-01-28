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

#ifndef SERVICEBROWSER_H
#define SERVICEBROWSER_H

#include "zeroconf_global.h"

#include <QHash>
#include <QObject>
#include <QSharedPointer>
#include <QStringList>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QHostInfo;
class QNetworkInterface;
QT_END_NAMESPACE

namespace ZeroConf {

namespace Internal {
class ServiceGatherer;
class MainConnection;
class ServiceBrowserPrivate;
}


class ZEROCONFSHARED_EXPORT ErrorMessage
{
public:
    enum SeverityLevel
    {
        NoteLevel,
        WarningLevel,
        ErrorLevel,
        FailureLevel
    };

    static QString severityLevelToString(SeverityLevel);

    ErrorMessage();
    ErrorMessage(SeverityLevel s, const QString &m);

    SeverityLevel severity;
    QString msg;

    class ErrorLogger{
    public:
        virtual void appendError(ErrorMessage::SeverityLevel severity, const QString& msg) = 0;
    };
};

ZEROCONFSHARED_EXPORT QDebug operator<<(QDebug dbg, const ErrorMessage &eMsg);

typedef QSharedPointer<Internal::MainConnection> MainConnectionPtr;

typedef QHash<QString, QString> ServiceTxtRecord;

class ZEROCONFSHARED_EXPORT Service
{
    friend class Internal::ServiceGatherer;

public:
    typedef QSharedPointer<const Service> ConstPtr;
    typedef QSharedPointer<Service> Ptr;

    enum AddressStyle
    {
        PlainAddresses,
        QuoteIPv6Adresses
    };

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
    QNetworkInterface networkInterface() const;
    QStringList addresses(AddressStyle style = PlainAddresses) const;
    bool operator==(const Service &o) const;

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

ZEROCONFSHARED_EXPORT QDebug operator<<(QDebug dbg, const Service &service);
ZEROCONFSHARED_EXPORT QDebug operator<<(QDebug dbg, const Service::ConstPtr &service);

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

    void startBrowsing(qint32 interfaceIndex = 0);
    void stopBrowsing();
    bool isBrowsing() const;
    bool didFail() const;
    int maxProgress() const;

    const QString& serviceType() const;
    const QString& domain() const;

    bool adressesAutoResolved() const;
    bool addressesRequired() const;

    QList<Service::ConstPtr> services() const;
    void reconfirmService(Service::ConstPtr service);
public slots:
    void triggerRefresh();
    void autoRefresh();
signals:
    void activateAutoRefresh();
    void serviceChanged(const ZeroConf::Service::ConstPtr &oldService,
        const ZeroConf::Service::ConstPtr &newService, ZeroConf::ServiceBrowser *browser);
    void serviceAdded(const ZeroConf::Service::ConstPtr &service,
        ZeroConf::ServiceBrowser *browser);
    void serviceRemoved(const ZeroConf::Service::ConstPtr &service,
        ZeroConf::ServiceBrowser *browser);
    void servicesUpdated(ZeroConf::ServiceBrowser *browser);
    void startupPhase(int progress, const QString &description, ZeroConf::ServiceBrowser *browser);
    void errorMessage(ZeroConf::ErrorMessage::SeverityLevel severity, const QString &msg, ZeroConf::ServiceBrowser *browser);
    void hadFailure(const QList<ZeroConf::ErrorMessage> &messages, ZeroConf::ServiceBrowser *browser);
    void startedBrowsing(ZeroConf::ServiceBrowser *browser);
private:
    QTimer *timer;
    Internal::ServiceBrowserPrivate *d;
};

enum LibUsage {
    UseDnsSdOnly = 1,
    UseEmbeddedOnly,
    UseAvahiOnly,
    UseAvahiOrDnsSd,
    UseAvahiOrDnsSdOrEmbedded
};

void setDefaultZConfLib(LibUsage usage, const QString &avahiLibName, const QString &dnsSdLibName,
                        const QString & dnsSdDaemonPath);

}

#endif // SERVICEBROWSER_H
