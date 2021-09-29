/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sshconnectionmanager.h"

#include "sshconnection.h"
#include "sshsettings.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QThread>
#include <QTimer>

namespace QSsh {
namespace Internal {

class UnacquiredConnection {
public:
    UnacquiredConnection(SshConnection *conn) : connection(conn), scheduledForRemoval(false) {}

    SshConnection *connection;
    bool scheduledForRemoval;
};
bool operator==(const UnacquiredConnection &c1, const UnacquiredConnection &c2) {
    return c1.connection == c2.connection;
}
bool operator!=(const UnacquiredConnection &c1, const UnacquiredConnection &c2) {
    return !(c1 == c2);
}

class SshConnectionManagerPrivate : public QObject
{
public:
    SshConnectionManagerPrivate()
    {
        connect(&m_removalTimer, &QTimer::timeout,
                this, &SshConnectionManagerPrivate::removeInactiveConnections);
        m_removalTimer.start(SshSettings::connectionSharingTimeout() * 1000 * 60 / 2);
    }

    ~SshConnectionManagerPrivate() override
    {
        for (const UnacquiredConnection &connection : qAsConst(m_unacquiredConnections)) {
            disconnect(connection.connection, nullptr, this, nullptr);
            delete connection.connection;
        }

        QTC_CHECK(m_acquiredConnections.isEmpty());
        QTC_CHECK(m_deprecatedConnections.isEmpty());
    }

    SshConnection *acquireConnection(const SshConnectionParameters &sshParams)
    {
        // Check in-use connections:
        for (SshConnection * const connection : qAsConst(m_acquiredConnections)) {
            if (connection->connectionParameters() != sshParams)
                continue;

            if (connection->sharingEnabled() != SshSettings::connectionSharingEnabled())
                continue;

            if (m_deprecatedConnections.contains(connection)) // we were asked to no longer use this one...
                continue;

            m_acquiredConnections.append(connection);
            return connection;
        }

        // Check cached open connections:
        for (int i = 0; i < m_unacquiredConnections.count(); ++i) {
            SshConnection * const connection = m_unacquiredConnections.at(i).connection;
            if (connection->state() != SshConnection::Connected
                    || connection->connectionParameters() != sshParams)
                continue;

            m_unacquiredConnections.removeAt(i);
            m_acquiredConnections.append(connection);
            return connection;
        }

        // create a new connection:
        SshConnection * const connection = new SshConnection(sshParams);
        connect(connection, &SshConnection::disconnected,
                this, &SshConnectionManagerPrivate::cleanup);
        if (SshSettings::connectionSharingEnabled())
            m_acquiredConnections.append(connection);

        return connection;
    }

    void releaseConnection(SshConnection *connection)
    {
        const bool wasAcquired = m_acquiredConnections.removeOne(connection);
        QTC_ASSERT(wasAcquired == connection->sharingEnabled(), return);
        if (m_acquiredConnections.contains(connection))
            return;

        bool doDelete = false;
        if (!connection->sharingEnabled()) {
            doDelete = true;
        } else if (m_deprecatedConnections.removeOne(connection)
                || connection->state() != SshConnection::Connected) {
            doDelete = true;
        } else {
            UnacquiredConnection uc(connection);
            QTC_ASSERT(!m_unacquiredConnections.contains(uc), return);
            m_unacquiredConnections.append(uc);
        }

        if (doDelete) {
            disconnect(connection, nullptr, this, nullptr);
            m_deprecatedConnections.removeAll(connection);
            connection->deleteLater();
        }
    }

    void forceNewConnection(const SshConnectionParameters &sshParams)
    {
        for (int i = 0; i < m_unacquiredConnections.count(); ++i) {
            SshConnection * const connection = m_unacquiredConnections.at(i).connection;
            if (connection->connectionParameters() == sshParams) {
                disconnect(connection, nullptr, this, nullptr);
                delete connection;
                m_unacquiredConnections.removeAt(i);
                break;
            }
        }

        for (SshConnection * const connection : qAsConst(m_acquiredConnections)) {
            if (connection->connectionParameters() == sshParams) {
                if (!m_deprecatedConnections.contains(connection))
                    m_deprecatedConnections.append(connection);
            }
        }
    }

private:
    void cleanup()
    {
        SshConnection *currentConnection = qobject_cast<SshConnection *>(sender());
        if (!currentConnection)
            return;

        if (m_unacquiredConnections.removeOne(UnacquiredConnection(currentConnection))) {
            disconnect(currentConnection, nullptr, this, nullptr);
            currentConnection->deleteLater();
        }
    }

    void removeInactiveConnections()
    {
        for (int i = m_unacquiredConnections.count() - 1; i >= 0; --i) {
            UnacquiredConnection &c = m_unacquiredConnections[i];
            if (c.scheduledForRemoval) {
                disconnect(c.connection, nullptr, this, nullptr);
                c.connection->deleteLater();
                m_unacquiredConnections.removeAt(i);
            } else {
                c.scheduledForRemoval = true;
            }
        }
    }

private:
    // We expect the number of concurrently open connections to be small.
    // If that turns out to not be the case, we can still use a data
    // structure with faster access.
    QList<UnacquiredConnection> m_unacquiredConnections;

    // Can contain the same connection more than once; this acts as a reference count.
    QList<SshConnection *> m_acquiredConnections;

    QList<SshConnection *> m_deprecatedConnections;
    QTimer m_removalTimer;
};

} // namespace Internal

SshConnectionManager::SshConnectionManager()
    : d(new Internal::SshConnectionManagerPrivate())
{
    QTC_CHECK(QThread::currentThread() == qApp->thread());
}

SshConnectionManager::~SshConnectionManager()
{
    delete d;
}

SshConnection *SshConnectionManager::acquireConnection(const SshConnectionParameters &sshParams)
{
    return instance()->d->acquireConnection(sshParams);
}

void SshConnectionManager::releaseConnection(SshConnection *connection)
{
    instance()->d->releaseConnection(connection);
}

void SshConnectionManager::forceNewConnection(const SshConnectionParameters &sshParams)
{
    instance()->d->forceNewConnection(sshParams);
}

} // namespace QSsh
