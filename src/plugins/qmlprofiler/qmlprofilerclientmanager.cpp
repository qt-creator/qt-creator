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

#include "qmlprofilerclientmanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatemanager.h"

#include <utils/qtcassert.h>

namespace QmlProfiler {
namespace Internal {

QmlProfilerClientManager::QmlProfilerClientManager(QObject *parent) : QObject(parent)
{
    setObjectName(QLatin1String("QML Profiler Connections"));
}

QmlProfilerClientManager::~QmlProfilerClientManager()
{
    // Don't receive any signals from the dtors of child objects while our own dtor is running.
    // That can lead to invalid reads.
    if (m_connection)
        m_connection->disconnect();
    if (m_qmlclientplugin)
        m_qmlclientplugin->disconnect();
}

void QmlProfilerClientManager::setModelManager(QmlProfilerModelManager *m)
{
    QTC_ASSERT(m_connection.isNull() && m_qmlclientplugin.isNull(), disconnectClient());
    m_modelManager = m;
}

void QmlProfilerClientManager::setFlushInterval(quint32 flushInterval)
{
    m_flushInterval = flushInterval;
}

void QmlProfilerClientManager::setRetryParams(int interval, int maxAttempts)
{
    m_retryInterval = interval;
    m_maximumRetries = maxAttempts;
}

void QmlProfilerClientManager::setTcpConnection(QString host, Utils::Port port)
{
    if (!m_localSocket.isEmpty() || m_tcpHost != host || m_tcpPort != port) {
        m_tcpHost = host;
        m_tcpPort = port;
        m_localSocket.clear();
        disconnectClient();
        stopConnectionTimer();
    }
}

void QmlProfilerClientManager::setLocalSocket(QString file)
{
    if (m_localSocket != file || !m_tcpHost.isEmpty() || m_tcpPort.isValid()) {
        m_localSocket = file;
        m_tcpHost.clear();
        m_tcpPort = Utils::Port();
        disconnectClient();
        stopConnectionTimer();
    }
}

void QmlProfilerClientManager::clearConnection()
{
    m_localSocket.clear();
    m_tcpHost.clear();
    m_tcpPort = Utils::Port();
    disconnectClient();
    stopConnectionTimer();
}

void QmlProfilerClientManager::clearBufferedData()
{
    if (m_qmlclientplugin)
        m_qmlclientplugin->clearData();
}

void QmlProfilerClientManager::connectToTcpServer()
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
                m_connection->connectToHost(m_tcpHost, m_tcpPort.number());
            } else if (m_numRetries < 3
                       && m_connection->socketState() != QAbstractSocket::ConnectedState) {
                // If we don't get connected in the first retry interval, drop the socket and try
                // with a new one. On some operating systems (maxOS) the very first connection to a
                // TCP server takes a very long time to get established and this helps.
                // On other operating systems (windows) every connection takes forever to get
                // established. So, after tearing down and rebuilding the socket twice, just
                // keep trying with the same one.
                m_connection->connectToHost(m_tcpHost, m_tcpPort.number());
            } // Else leave it alone and wait for hello.
        } else {
            // On final timeout, clear the connection.
            stopConnectionTimer();
            if (m_connection)
                disconnectClientSignals();
            m_qmlclientplugin.reset();
            m_connection.reset();
            emit connectionFailed();
        }
    });
    m_connectionTimer.start(m_retryInterval);

    if (m_connection.isNull()) {
        QTC_ASSERT(m_qmlclientplugin.isNull(), disconnectClient());
        createConnection();
        QTC_ASSERT(m_connection, emit connectionFailed(); return);
        m_connection->connectToHost(m_tcpHost, m_tcpPort.number());
    }
}

void QmlProfilerClientManager::startLocalServer()
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
        QTC_ASSERT(m_qmlclientplugin.isNull(), disconnectClient());
        createConnection();
        QTC_ASSERT(m_connection, emit connectionFailed(); return);
        m_connection->startLocalServer(m_localSocket);
    }
}

void QmlProfilerClientManager::stopRecording()
{
    QTC_ASSERT(m_qmlclientplugin, return);
    m_qmlclientplugin->setRecording(false);
}

void QmlProfilerClientManager::retryConnect()
{
    if (!m_localSocket.isEmpty()) {
        startLocalServer();
    } else if (!m_tcpHost.isEmpty() && m_tcpPort.isValid()) {
        disconnectClient();
        connectToTcpServer();
    } else {
        emit connectionFailed();
    }
}

void QmlProfilerClientManager::createConnection()
{
    QTC_ASSERT(m_profilerState, return);
    QTC_ASSERT(m_modelManager, return);
    QTC_ASSERT(m_connection.isNull() && m_qmlclientplugin.isNull(), disconnectClient());

    m_connection.reset(new QmlDebug::QmlDebugConnection);

    // false by default (will be set to true when connected)
    m_profilerState->setServerRecording(false);
    m_profilerState->setRecordedFeatures(0);
    m_qmlclientplugin.reset(new QmlProfilerTraceClient(m_connection.data(), m_modelManager,
                                                       m_profilerState->requestedFeatures()));
    m_qmlclientplugin->setFlushInterval(m_flushInterval);
    connectClientSignals();
}

void QmlProfilerClientManager::connectClientSignals()
{
    QTC_ASSERT(m_connection, return);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::connected,
                     this, &QmlProfilerClientManager::qmlDebugConnectionOpened);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::disconnected,
                     this, &QmlProfilerClientManager::qmlDebugConnectionClosed);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::connectionFailed,
                     this, &QmlProfilerClientManager::qmlDebugConnectionFailed);

    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::logStateChange,
                     this, &QmlProfilerClientManager::logState);
    QObject::connect(m_connection.data(), &QmlDebug::QmlDebugConnection::logError,
                     this, &QmlProfilerClientManager::logState);


    QTC_ASSERT(m_qmlclientplugin, return);
    QTC_ASSERT(m_modelManager, return);
    QObject::connect(m_qmlclientplugin.data(), &QmlProfilerTraceClient::traceFinished,
                     m_modelManager->traceTime(), &QmlProfilerTraceTime::increaseEndTime);

    QTC_ASSERT(m_profilerState, return);
    QObject::connect(m_profilerState.data(), &QmlProfilerStateManager::requestedFeaturesChanged,
                     m_qmlclientplugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
    QObject::connect(m_qmlclientplugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                     m_profilerState.data(), &QmlProfilerStateManager::setRecordedFeatures);

    QObject::connect(m_qmlclientplugin.data(), &QmlProfilerTraceClient::traceStarted,
                     this, [this](qint64 time) {
        m_profilerState->setServerRecording(true);
        m_modelManager->traceTime()->decreaseStartTime(time);
    });

    QObject::connect(m_qmlclientplugin.data(), &QmlProfilerTraceClient::complete,
                     this, [this](qint64 time) {
        m_modelManager->traceTime()->increaseEndTime(time);
        m_profilerState->setServerRecording(false);
    });

    QObject::connect(m_profilerState.data(), &QmlProfilerStateManager::clientRecordingChanged,
                     m_qmlclientplugin.data(), &QmlProfilerTraceClient::setRecording);

}

void QmlProfilerClientManager::disconnectClientSignals()
{
    QTC_ASSERT(m_connection, return);
    m_connection->disconnect();

    QTC_ASSERT(m_qmlclientplugin, return);
    m_qmlclientplugin->disconnect();

    QTC_ASSERT(m_profilerState, return);
    QObject::disconnect(m_profilerState.data(), &QmlProfilerStateManager::requestedFeaturesChanged,
                        m_qmlclientplugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
    QObject::disconnect(m_profilerState.data(), &QmlProfilerStateManager::clientRecordingChanged,
                        m_qmlclientplugin.data(), &QmlProfilerTraceClient::setRecording);
}

bool QmlProfilerClientManager::isConnected() const
{
    return m_connection && m_connection->isConnected();
}

void QmlProfilerClientManager::disconnectClient()
{
    // This might be called indirectly by QDebugConnectionPrivate::readyRead().
    // Therefore, allow the function to complete before deleting the object.
    if (m_connection) {
        // Don't receive any more signals from the connection or the client
        disconnectClientSignals();

        QTC_ASSERT(m_connection && m_qmlclientplugin, return);
        m_qmlclientplugin.take()->deleteLater();
        m_connection.take()->deleteLater();
    }
}

void QmlProfilerClientManager::qmlDebugConnectionOpened()
{
    logState(tr("Debug connection opened"));
    QTC_ASSERT(m_profilerState, return);
    QTC_ASSERT(m_connection && m_qmlclientplugin, return);
    QTC_ASSERT(m_connection->isConnected(), return);
    stopConnectionTimer();
    m_qmlclientplugin->setRecording(m_profilerState->clientRecording());
    emit connectionOpened();
}

void QmlProfilerClientManager::qmlDebugConnectionClosed()
{
    logState(tr("Debug connection closed"));
    QTC_ASSERT(m_connection && m_qmlclientplugin, return);
    QTC_ASSERT(!m_connection->isConnected(), return);
    disconnectClient();
    emit connectionClosed();
}

void QmlProfilerClientManager::qmlDebugConnectionFailed()
{
    logState(tr("Debug connection failed"));
    QTC_ASSERT(m_connection && m_qmlclientplugin, return);
    QTC_ASSERT(!m_connection->isConnected(), /**/);

    disconnectClient();
    // The retry handler, driven by m_connectionTimer should decide to retry or signal a failure.

    QTC_ASSERT(m_connectionTimer.isActive(), emit connectionFailed());
}

void QmlProfilerClientManager::logState(const QString &msg)
{
    QmlProfilerTool::logState(QLatin1String("QML Profiler: ") + msg);
}

void QmlProfilerClientManager::setProfilerStateManager(QmlProfilerStateManager *profilerState)
{
    // Don't do this while connecting
    QTC_ASSERT(m_connection.isNull() && m_qmlclientplugin.isNull(), disconnectClient());

    m_profilerState = profilerState;
}

void QmlProfilerClientManager::stopConnectionTimer()
{
    m_connectionTimer.stop();
    m_connectionTimer.disconnect();
    m_numRetries = 0;
}

} // namespace Internal
} // namespace QmlProfiler
