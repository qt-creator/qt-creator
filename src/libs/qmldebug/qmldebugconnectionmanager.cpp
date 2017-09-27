/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qmldebugconnectionmanager.h"
#include "qmldebugconnection.h"

#include <utils/qtcassert.h>
#include <utils/url.h>

namespace QmlDebug {

QmlDebugConnectionManager::QmlDebugConnectionManager(QObject *parent) : QObject(parent)
{
}

QmlDebugConnectionManager::~QmlDebugConnectionManager()
{
    // Don't receive any signals from the dtors of child objects while our own dtor is running.
    // That can lead to invalid reads.
    if (m_connection)
        disconnectConnectionSignals();
}

void QmlDebugConnectionManager::setRetryParams(int interval, int maxAttempts)
{
    m_retryInterval = interval;
    m_maximumRetries = maxAttempts;
}

void QmlDebugConnectionManager::connectToServer(const QUrl &server)
{
    if (m_server != server) {
        m_server = server;
        destroyConnection();
        stopConnectionTimer();
    }
    if (server.scheme() == Utils::urlTcpScheme())
        connectToTcpServer();
    else if (server.scheme() == Utils::urlSocketScheme())
        startLocalServer();
    else
        QTC_ASSERT(false, emit connectionFailed());
}

void QmlDebugConnectionManager::disconnectFromServer()
{
    m_server.clear();
    destroyConnection();
    stopConnectionTimer();
}

static quint16 port16(const QUrl &url)
{
    const int port32 = url.port();
    QTC_ASSERT(port32 > 0 && port32 <= std::numeric_limits<quint16>::max(), return 0);
    return static_cast<quint16>(port32);
}

void QmlDebugConnectionManager::connectToTcpServer()
{
    // Calling this again when we're already trying means "reset the retry timer". This is
    // useful in cases where we have to parse the port from the output. We might waste retries
    // on an initial guess for the port.
    stopConnectionTimer();
    connect(&m_connectionTimer, &QTimer::timeout, this, [this]{
        QTC_ASSERT(!isConnected(), return);

        if (++(m_numRetries) < m_maximumRetries) {
            if (m_connection.isNull()) {
                // If the previous connection failed, recreate it.
                createConnection();
                m_connection->connectToHost(m_server.host(), port16(m_server));
            } else if (m_numRetries < 3
                       && m_connection->socketState() != QAbstractSocket::ConnectedState) {
                // If we don't get connected in the first retry interval, drop the socket and try
                // with a new one. On some operating systems (maxOS) the very first connection to a
                // TCP server takes a very long time to get established and this helps.
                // On other operating systems (windows) every connection takes forever to get
                // established. So, after tearing down and rebuilding the socket twice, just
                // keep trying with the same one.
                m_connection->connectToHost(m_server.host(), port16(m_server));
            } // Else leave it alone and wait for hello.
        } else {
            // On final timeout, clear the connection.
            stopConnectionTimer();
            destroyConnection();
            emit connectionFailed();
        }
    });
    m_connectionTimer.start(m_retryInterval);

    if (m_connection.isNull()) {
        createConnection();
        QTC_ASSERT(m_connection, emit connectionFailed(); return);
        m_connection->connectToHost(m_server.host(), port16(m_server));
    }
}

void QmlDebugConnectionManager::startLocalServer()
{
    stopConnectionTimer();
    connect(&m_connectionTimer, &QTimer::timeout, this, [this]() {
        QTC_ASSERT(!isConnected(), return);

        // We leave the server running as some application might currently be trying to
        // connect. Don't cut this off, or the application might hang on the hello mutex.
        // qmlConnectionFailed() might drop the connection, which is fatal. We detect this
        // here and signal it accordingly.

        if (!m_connection || ++(m_numRetries) >= m_maximumRetries) {
            stopConnectionTimer();
            emit connectionFailed();
        }
    });
    m_connectionTimer.start(m_retryInterval);

    if (m_connection.isNull()) {
        // Otherwise, reuse the same one
        createConnection();
        QTC_ASSERT(m_connection, emit connectionFailed(); return);
        m_connection->startLocalServer(m_server.path());
    }
}

void QmlDebugConnectionManager::retryConnect()
{
    if (m_server.scheme() == Utils::urlSocketScheme()) {
        startLocalServer();
    } else if (m_server.scheme() == Utils::urlTcpScheme()) {
        destroyConnection();
        connectToTcpServer();
    } else {
        emit connectionFailed();
    }
}

void QmlDebugConnectionManager::logState(const QString &message)
{
    Q_UNUSED(message);
}

QmlDebugConnection *QmlDebugConnectionManager::connection() const
{
    return m_connection.data();
}

void QmlDebugConnectionManager::createConnection()
{
    QTC_ASSERT(m_connection.isNull(), destroyConnection());

    m_connection.reset(new QmlDebug::QmlDebugConnection);

    createClients();
    connectConnectionSignals();
}

void QmlDebugConnectionManager::connectConnectionSignals()
{
    QTC_ASSERT(m_connection, return);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::connected,
                     this, &QmlDebugConnectionManager::qmlDebugConnectionOpened);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::disconnected,
                     this, &QmlDebugConnectionManager::qmlDebugConnectionClosed);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::connectionFailed,
                     this, &QmlDebugConnectionManager::qmlDebugConnectionFailed);

    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::logStateChange,
                     this, &QmlDebugConnectionManager::logState);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::logError,
                     this, &QmlDebugConnectionManager::logState);
}

void QmlDebugConnectionManager::disconnectConnectionSignals()
{
    QTC_ASSERT(m_connection, return);
    m_connection->disconnect();
}

bool QmlDebugConnectionManager::isConnected() const
{
    return m_connection && m_connection->isConnected();
}

void QmlDebugConnectionManager::destroyConnection()
{
    // This might be called indirectly by QDebugConnectionPrivate::readyRead().
    // Therefore, allow the function to complete before deleting the object.
    if (m_connection) {
        // Don't receive any more signals from the connection or the client
        disconnectConnectionSignals();
        destroyClients();
        m_connection.take()->deleteLater();
    }
}

void QmlDebugConnectionManager::qmlDebugConnectionOpened()
{
    logState(tr("Debug connection opened"));
    QTC_ASSERT(m_connection, return);
    QTC_ASSERT(m_connection->isConnected(), return);
    stopConnectionTimer();
    emit connectionOpened();
}

void QmlDebugConnectionManager::qmlDebugConnectionClosed()
{
    logState(tr("Debug connection closed"));
    QTC_ASSERT(m_connection, return);
    QTC_ASSERT(!m_connection->isConnected(), return);
    destroyConnection();
    emit connectionClosed();
}

void QmlDebugConnectionManager::qmlDebugConnectionFailed()
{
    logState(tr("Debug connection failed"));
    QTC_ASSERT(m_connection, return);
    QTC_ASSERT(!m_connection->isConnected(), /**/);

    destroyConnection();
    // The retry handler, driven by m_connectionTimer should decide to retry or signal a failure.

    QTC_ASSERT(m_connectionTimer.isActive(), emit connectionFailed());
}

void QmlDebugConnectionManager::stopConnectionTimer()
{
    m_connectionTimer.stop();
    m_connectionTimer.disconnect();
    m_numRetries = 0;
}

} // namespace QmlDebug
