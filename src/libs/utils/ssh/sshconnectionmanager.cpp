/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>

namespace Utils {
namespace Internal {

struct ConnectionInfo
{
    typedef QSharedPointer<ConnectionInfo> Ptr;
    static ConnectionInfo::Ptr create(const SshConnection::Ptr &conn)
    {
        return Ptr(new ConnectionInfo(conn));
    }

    SshConnection::Ptr connection;
    int refCount;
    bool isConnecting;

private:
    ConnectionInfo(const SshConnection::Ptr &conn)
        : connection(conn), refCount(1), isConnecting(false) {}
};

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
        connect(&m_cleanupTimer, SIGNAL(timeout()), SLOT(cleanup()));
        m_cleanupTimer.start(5*60*1000);
    }

    ConnectionInfo::Ptr findConnection(const SshConnection::Ptr &connection)
    {
        foreach (const ConnectionInfo::Ptr &connInfo, m_connections) {
            if (connInfo->connection == connection)
                return connInfo;
        }
        return ConnectionInfo::Ptr();
    }


    QSharedPointer<SshConnection> acquireConnection(const SshConnectionParameters &sshParams)
    {
        QMutexLocker locker(&m_listMutex);
        foreach (const ConnectionInfo::Ptr &connInfo, m_connections) {
            const SshConnection::Ptr connection = connInfo->connection;
            bool connectionUsable = false;
            if (connection->state() == SshConnection::Connected
                    && connection->connectionParameters() == sshParams) {
                if (connInfo->refCount == 0) {
                    if (connection->thread() != QThread::currentThread()) {
                        QMetaObject::invokeMethod(this, "switchToCallerThread",
                            Qt::BlockingQueuedConnection,
                            Q_ARG(SshConnection *, connection.data()),
                            Q_ARG(QObject *, QThread::currentThread()));
                    }
                    connectionUsable = true;
                } else if (connection->thread() == QThread::currentThread()) {
                    connectionUsable = true;
                }
                if (connectionUsable) {
                    ++connInfo->refCount;
                    return connection;
                }
            }
        }

        ConnectionInfo::Ptr connInfo
            = ConnectionInfo::create(SshConnection::create(sshParams));
        m_connections << connInfo;
        return connInfo->connection;
    }

    void releaseConnection(const SshConnection::Ptr &connection)
    {
        QMutexLocker locker(&m_listMutex);
        ConnectionInfo::Ptr connInfo = findConnection(connection);
        Q_ASSERT_X(connInfo, Q_FUNC_INFO, "Fatal: Unowned SSH Connection released.");
        if (--connInfo->refCount == 0) {
            connection->moveToThread(QCoreApplication::instance()->thread());
            if (connection->state() != SshConnection::Connected)
                m_connections.removeOne(connInfo);
        }
    }

private:
    Q_INVOKABLE void switchToCallerThread(SshConnection *connection, QObject *threadObj)
    {
        connection->moveToThread(qobject_cast<QThread *>(threadObj));
    }

    Q_SLOT void cleanup()
    {
        QMutexLocker locker(&m_listMutex);
        foreach (const ConnectionInfo::Ptr &connInfo, m_connections) {
            if (connInfo->refCount == 0 &&
                    connInfo->connection->state() != SshConnection::Connected) {
                m_connections.removeOne(connInfo);
            }
        }
    }

    // We expect the number of concurrently open connections to be small.
    // If that turns out to not be the case, we can still use a data
    // structure with faster access.
    QList<ConnectionInfo::Ptr> m_connections;

    QMutex m_listMutex;
    QTimer m_cleanupTimer;
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

} // namespace Utils

#include "sshconnectionmanager.moc"
