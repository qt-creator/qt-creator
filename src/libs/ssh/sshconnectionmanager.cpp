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

#include "sshconnectionmanager.h"

#include "sshconnection.h"

#include <QCoreApplication>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QThread>

namespace QSsh {
namespace Internal {

class SshConnectionManagerPrivate : public QObject
{
    Q_OBJECT

public:

    static QMutex instanceMutex;
    static SshConnectionManager &instance()
    {
        static SshConnectionManager manager;
        return manager;
    }

    SshConnectionManagerPrivate()
    {
        moveToThread(QCoreApplication::instance()->thread());
    }

    ~SshConnectionManagerPrivate()
    {
        foreach (SshConnection * const connection, m_unacquiredConnections) {
            disconnect(connection, 0, this, 0);
            delete connection;
        }

        QSSH_ASSERT(m_acquiredConnections.isEmpty());
        QSSH_ASSERT(m_deprecatedConnections.isEmpty());
    }

    SshConnection *acquireConnection(const SshConnectionParameters &sshParams)
    {
        QMutexLocker locker(&m_listMutex);

        // Check in-use connections:
        foreach (SshConnection * const connection, m_acquiredConnections) {
            if (connection->connectionParameters() != sshParams)
                continue;

            if (connection->thread() != QThread::currentThread())
                break;

            if (m_deprecatedConnections.contains(connection)) // we were asked to no longer use this one...
                break;

            m_acquiredConnections.append(connection);
            return connection;
        }

        // Checked cached open connections:
        foreach (SshConnection * const connection, m_unacquiredConnections) {
            if (connection->state() != SshConnection::Connected
                    || connection->connectionParameters() != sshParams)
                continue;

            if (connection->thread() != QThread::currentThread()) {
                if (connection->channelCount() != 0)
                    continue;
                QMetaObject::invokeMethod(this, "switchToCallerThread",
                    Qt::BlockingQueuedConnection,
                    Q_ARG(SshConnection *, connection),
                    Q_ARG(QObject *, QThread::currentThread()));
            }

            m_unacquiredConnections.removeOne(connection);
            m_acquiredConnections.append(connection);
            return connection;
        }

        // create a new connection:
        SshConnection * const connection = new SshConnection(sshParams);
        connect(connection, SIGNAL(disconnected()), this, SLOT(cleanup()));
        m_acquiredConnections.append(connection);

        return connection;
    }

    void releaseConnection(SshConnection *connection)
    {
        QMutexLocker locker(&m_listMutex);

        const bool wasAquired = m_acquiredConnections.removeOne(connection);
        QSSH_ASSERT_AND_RETURN(wasAquired);
        if (m_acquiredConnections.contains(connection))
            return;

        bool doDelete = false;
        connection->moveToThread(QCoreApplication::instance()->thread());
        if (m_deprecatedConnections.removeOne(connection)
                || connection->state() != SshConnection::Connected) {
            doDelete = true;
        } else {
            QSSH_ASSERT_AND_RETURN(!m_unacquiredConnections.contains(connection));

            // It can happen that two or more connections with the same parameters were acquired
            // if the clients were running in different threads. Only keep one of them in
            // such a case.
            bool haveConnection = false;
            foreach (SshConnection * const conn, m_unacquiredConnections) {
                if (conn->connectionParameters() == connection->connectionParameters()) {
                    haveConnection = true;
                    break;
                }
            }
            if (!haveConnection) {
                // Let's nag clients who release connections with open channels.
                const int channelCount = connection->closeAllChannels();
                QSSH_ASSERT(channelCount == 0);

                m_unacquiredConnections.append(connection);
            } else {
                doDelete = true;
            }
        }

        if (doDelete) {
            disconnect(connection, 0, this, 0);
            m_deprecatedConnections.removeAll(connection);
            connection->deleteLater();
        }
    }

    void forceNewConnection(const SshConnectionParameters &sshParams)
    {
        QMutexLocker locker(&m_listMutex);

        for (int i = 0; i < m_unacquiredConnections.count(); ++i) {
            SshConnection * const connection = m_unacquiredConnections.at(i);
            if (connection->connectionParameters() == sshParams) {
                disconnect(connection, 0, this, 0);
                delete connection;
                m_unacquiredConnections.removeAt(i);
                break;
            }
        }

        foreach (SshConnection * const connection, m_acquiredConnections) {
            if (connection->connectionParameters() == sshParams) {
                if (!m_deprecatedConnections.contains(connection))
                    m_deprecatedConnections.append(connection);
            }
        }
    }

private:
    Q_INVOKABLE void switchToCallerThread(SshConnection *connection, QObject *threadObj)
    {
        connection->moveToThread(qobject_cast<QThread *>(threadObj));
    }

private slots:
    void cleanup()
    {
        QMutexLocker locker(&m_listMutex);

        SshConnection *currentConnection = qobject_cast<SshConnection *>(sender());
        if (!currentConnection)
            return;

        if (m_unacquiredConnections.removeOne(currentConnection)) {
            disconnect(currentConnection, 0, this, 0);
            currentConnection->deleteLater();
        }
    }

private:
    // We expect the number of concurrently open connections to be small.
    // If that turns out to not be the case, we can still use a data
    // structure with faster access.
    QList<SshConnection *> m_unacquiredConnections;

    // Can contain the same connection more than once; this acts as a reference count.
    QList<SshConnection *> m_acquiredConnections;

    QList<SshConnection *> m_deprecatedConnections;
    QMutex m_listMutex;
};

QMutex SshConnectionManagerPrivate::instanceMutex;

} // namespace Internal

SshConnectionManager &SshConnectionManager::instance()
{
    QMutexLocker locker(&Internal::SshConnectionManagerPrivate::instanceMutex);
    return Internal::SshConnectionManagerPrivate::instance();
}

SshConnectionManager::SshConnectionManager()
    : d(new Internal::SshConnectionManagerPrivate)
{
}

SshConnectionManager::~SshConnectionManager()
{
}

SshConnection *SshConnectionManager::acquireConnection(const SshConnectionParameters &sshParams)
{
    return d->acquireConnection(sshParams);
}

void SshConnectionManager::releaseConnection(SshConnection *connection)
{
    d->releaseConnection(connection);
}

void SshConnectionManager::forceNewConnection(const SshConnectionParameters &sshParams)
{
    d->forceNewConnection(sshParams);
}

} // namespace QSsh

#include "sshconnectionmanager.moc"
