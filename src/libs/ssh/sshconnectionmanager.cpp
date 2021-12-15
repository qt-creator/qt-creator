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
#include <QHash>
#include <QObject>
#include <QThread>
#include <QTimer>

namespace QSsh {
namespace Internal {

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
        for (auto it = m_connections.cbegin(); it != m_connections.cend(); ++it) {
            SshConnection * const connection = it.key();
            const SshConnectionState &state = it.value();
            QTC_CHECK(state.refCount() == 0);
            QTC_CHECK(!state.isStale());
            disconnect(connection, nullptr, this, nullptr);
            delete connection;
        }
    }

    SshConnection *acquireConnection(const SshConnectionParameters &sshParams)
    {
        if (SshSettings::connectionSharingEnabled()) {
            for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
                SshConnection * const connection = it.key();
                if (connection->connectionParameters() != sshParams)
                    continue;

                SshConnectionState &state = it.value();
                if (state.isStale())
                    continue;

                if (state.refCount() == 0 && connection->state() != SshConnection::Connected)
                    continue;

                state.ref();
                return connection;
            }
        }

        SshConnection * const connection = new SshConnection(sshParams);
        if (SshSettings::connectionSharingEnabled()) {
            connect(connection, &SshConnection::disconnected,
                    this, [this, connection] { cleanup(connection); });
            m_connections.insert(connection, {});
        }

        return connection;
    }

    void releaseConnection(SshConnection *connection)
    {
        auto it = m_connections.find(connection);
        bool doDelete = false;
        if (it == m_connections.end()) {
            QTC_ASSERT(!connection->sharingEnabled(), return);
            doDelete = true;
        } else {
            SshConnectionState &state = it.value();
            if (state.deref())
                return;

            if (state.isStale() || connection->state() != SshConnection::Connected) {
                doDelete = true;
                m_connections.erase(it);
            }
        }

        if (doDelete) {
            disconnect(connection, nullptr, this, nullptr);
            connection->deleteLater();
        }
    }

    void forceNewConnection(const SshConnectionParameters &sshParams)
    {
        auto it = m_connections.begin();
        while (it != m_connections.end()) {
            SshConnection * const connection = it.key();
            if (connection->connectionParameters() != sshParams) {
                ++it;
                continue;
            }

            SshConnectionState &state = it.value();
            if (state.refCount()) {
                state.makeStale();
                ++it;
                continue;
            }

            disconnect(connection, nullptr, this, nullptr);
            delete connection;
            it = m_connections.erase(it);
        }
    }

private:
    void cleanup(SshConnection *connection)
    {
        auto it = m_connections.find(connection);
        if (it == m_connections.end())
            return;

        SshConnectionState &state = it.value();
        if (state.refCount())
            return;

        disconnect(connection, nullptr, this, nullptr);
        connection->deleteLater();
    }

    void removeInactiveConnections()
    {
        auto it = m_connections.begin();
        while (it != m_connections.end()) {
            SshConnection * const connection = it.key();
            SshConnectionState &state = it.value();
            if (state.refCount() == 0 && state.scheduleForRemoval()) {
                disconnect(connection, nullptr, this, nullptr);
                connection->deleteLater();
                it = m_connections.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    struct SshConnectionState {
        void ref() { ++m_ref; m_scheduledForRemoval = false; }
        bool deref() { QTC_ASSERT(m_ref, return false); return --m_ref; }
        int refCount() const { return m_ref; }

        void makeStale() { m_isStale = true; }
        bool isStale() const { return m_isStale; }

        bool scheduleForRemoval()
        {
            const bool ret = m_scheduledForRemoval;
            m_scheduledForRemoval = true;
            return ret;
        }
    private:
        int m_ref = 1; // 0 means unacquired connection
        bool m_isStale = false;
        bool m_scheduledForRemoval = false;
    };

    QHash<SshConnection *, SshConnectionState> m_connections;
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
