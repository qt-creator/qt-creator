/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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

    QSharedPointer<SshConnection> acquireConnection(const SshConnectionParameters &sshParams)
    {
        QMutexLocker locker(&m_listMutex);

        // Check in-use connections:
        foreach (SshConnection::Ptr connection, m_acquiredConnections) {
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
        foreach (SshConnection::Ptr connection, m_unacquiredConnections) {
            if (connection->state() != SshConnection::Connected
                    || connection->connectionParameters() != sshParams)
                continue;

            if (connection->thread() != QThread::currentThread()) {
                QMetaObject::invokeMethod(this, "switchToCallerThread",
                    Qt::BlockingQueuedConnection,
                    Q_ARG(SshConnection *, connection.data()),
                    Q_ARG(QObject *, QThread::currentThread()));
            }

            m_unacquiredConnections.removeOne(connection);
            m_acquiredConnections.append(connection);
            return connection;
        }

        // create a new connection:
        SshConnection::Ptr connection = SshConnection::create(sshParams);
        connect(connection.data(), SIGNAL(disconnected()), this, SLOT(cleanup()));
        m_acquiredConnections.append(connection);

        return connection;
    }

    void releaseConnection(const SshConnection::Ptr &connection)
    {
        QMutexLocker locker(&m_listMutex);

        m_acquiredConnections.removeOne(connection);
        if (!m_acquiredConnections.contains(connection)) {
            // no longer in use:
            connection->moveToThread(QCoreApplication::instance()->thread());
            if (m_deprecatedConnections.contains(connection))
                m_deprecatedConnections.removeAll(connection);
            else if (connection->state() == SshConnection::Connected) {
                // Make sure to only keep one connection open
                bool haveConnection = false;
                foreach (SshConnection::Ptr conn, m_unacquiredConnections) {
                    if (conn->connectionParameters() == connection->connectionParameters()) {
                        haveConnection = true;
                        break;
                    }
                }
                if (!haveConnection)
                    m_unacquiredConnections.append(connection);
            }
        }
    }

    void forceNewConnection(const SshConnectionParameters &sshParams)
    {
        QMutexLocker locker(&m_listMutex);

        SshConnection::Ptr toReset;
        foreach (SshConnection::Ptr connection, m_unacquiredConnections) {
            if (connection->connectionParameters() == sshParams) {
                toReset = connection;
                break;
            }
        }

        if (toReset.isNull()) {
            foreach (SshConnection::Ptr connection, m_acquiredConnections) {
                if (connection->connectionParameters() == sshParams) {
                    toReset = connection;
                    break;
                }
            }
        }

        if (!toReset.isNull() && !m_deprecatedConnections.contains(toReset))
            m_deprecatedConnections.append(toReset);
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

        for (int i = m_unacquiredConnections.count() - 1; i >= 0; --i) {
            if (m_unacquiredConnections.at(i) == currentConnection)
                m_unacquiredConnections.removeAt(i);
        }
    }

private:
    // We expect the number of concurrently open connections to be small.
    // If that turns out to not be the case, we can still use a data
    // structure with faster access.
    QList<SshConnection::Ptr> m_unacquiredConnections;
    QList<SshConnection::Ptr> m_acquiredConnections;
    QList<SshConnection::Ptr> m_deprecatedConnections;
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

SshConnection::Ptr SshConnectionManager::acquireConnection(const SshConnectionParameters &sshParams)
{
    return d->acquireConnection(sshParams);
}

void SshConnectionManager::releaseConnection(const SshConnection::Ptr &connection)
{
    d->releaseConnection(connection);
}

void SshConnectionManager::forceNewConnection(const SshConnectionParameters &sshParams)
{
    d->forceNewConnection(sshParams);
}

} // namespace QSsh

#include "sshconnectionmanager.moc"
