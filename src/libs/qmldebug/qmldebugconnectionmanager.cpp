// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldebugconnectionmanager.h"

#include "qmldebugconnection.h"
#include "qmldebugtr.h"

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

void QmlDebugConnectionManager::setServer(const QUrl &server)
{
    m_server = server;
}

void QmlDebugConnectionManager::connectToServer()
{
    destroyConnection();
    stopConnectionTimer();
    if (m_server.scheme() == Utils::urlTcpScheme())
        connectToTcpServer();
    else if (m_server.scheme() == Utils::urlSocketScheme())
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

bool QmlDebugConnectionManager::isConnecting() const
{
    return m_connectionTimer.isActive();
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
            if (!m_connection) {
                // If the previous connection failed, recreate it.
                createConnection();
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

    if (!m_connection) {
        createConnection();
        QTC_ASSERT(m_connection, emit connectionFailed(); return);
        m_connection->connectToHost(m_server.host(), port16(m_server));
    }
}

void QmlDebugConnectionManager::startLocalServer()
{
    stopConnectionTimer();
    connect(&m_connectionTimer, &QTimer::timeout, this, [this] {
        QTC_ASSERT(!isConnected(), return);

        // We leave the server running as some application might currently be trying to
        // connect. Don't cut this off, or the application might hang on the hello mutex.
        // qmlConnectionFailed() might drop the connection, which is fatal. We detect this
        // here and signal it accordingly.

        if (!m_connection || ++(m_numRetries) >= m_maximumRetries) {
            stopConnectionTimer();
            destroyConnection();
            emit connectionFailed();
        }
    });
    m_connectionTimer.start(m_retryInterval);

    if (!m_connection) {
        // Otherwise, reuse the same one
        createConnection();
        QTC_ASSERT(m_connection, emit connectionFailed(); return);
        m_connection->startLocalServer(m_server.path());
    }
}

void QmlDebugConnectionManager::retryConnect()
{
    destroyConnection();
    if (m_server.scheme() == Utils::urlSocketScheme()) {
        startLocalServer();
    } else if (m_server.scheme() == Utils::urlTcpScheme()) {
        connectToTcpServer();
    } else {
        emit connectionFailed();
    }
}

void QmlDebugConnectionManager::logState(const QString &message)
{
    Q_UNUSED(message)
}

QmlDebugConnection *QmlDebugConnectionManager::connection() const
{
    return m_connection.get();
}

void QmlDebugConnectionManager::createConnection()
{
    QTC_ASSERT(!m_connection, destroyConnection());

    m_connection.reset(new QmlDebug::QmlDebugConnection);

    createClients();
    connectConnectionSignals();
}

void QmlDebugConnectionManager::connectConnectionSignals()
{
    QTC_ASSERT(m_connection, return);
    QObject::connect(m_connection.get(), &QmlDebug::QmlDebugConnection::connected,
                     this, &QmlDebugConnectionManager::qmlDebugConnectionOpened);
    QObject::connect(m_connection.get(), &QmlDebug::QmlDebugConnection::disconnected,
                     this, &QmlDebugConnectionManager::qmlDebugConnectionClosed);
    QObject::connect(m_connection.get(), &QmlDebug::QmlDebugConnection::connectionFailed,
                     this, &QmlDebugConnectionManager::qmlDebugConnectionFailed);

    QObject::connect(m_connection.get(), &QmlDebug::QmlDebugConnection::logStateChange,
                     this, &QmlDebugConnectionManager::logState);
    QObject::connect(m_connection.get(), &QmlDebug::QmlDebugConnection::logError,
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
        m_connection.release()->deleteLater();
    }
}

void QmlDebugConnectionManager::qmlDebugConnectionOpened()
{
    logState(Tr::tr("Debug connection opened."));
    QTC_ASSERT(m_connection, return);
    QTC_ASSERT(m_connection->isConnected(), return);
    stopConnectionTimer();
    emit connectionOpened();
}

void QmlDebugConnectionManager::qmlDebugConnectionClosed()
{
    logState(Tr::tr("Debug connection closed."));
    QTC_ASSERT(m_connection, return);
    QTC_ASSERT(!m_connection->isConnected(), return);
    destroyConnection();
    emit connectionClosed();
}

void QmlDebugConnectionManager::qmlDebugConnectionFailed()
{
    logState(Tr::tr("Debug connection failed."));
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
