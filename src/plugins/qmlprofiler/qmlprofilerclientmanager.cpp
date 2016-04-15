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
#include "qmlprofilerplugin.h"

#include <qmldebug/qmlprofilertraceclient.h>

#include <utils/qtcassert.h>
#include <QPointer>
#include <QTimer>
#include <QMessageBox>

#include "qmlprofilermodelmanager.h"

using namespace QmlDebug;
using namespace Core;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerClientManager::QmlProfilerClientManagerPrivate
{
public:
    QmlProfilerStateManager *profilerState;

    QmlDebugConnection *connection;
    QPointer<QmlProfilerTraceClient> qmlclientplugin;

    QTimer connectionTimer;
    int connectionAttempts;

    QString localSocket;
    QString tcpHost;
    quint64 tcpPort;
    QString sysroot;
    quint32 flushInterval;
    bool aggregateTraces;

    QmlProfilerModelManager *modelManager;
};

QmlProfilerClientManager::QmlProfilerClientManager(QObject *parent) :
    QObject(parent), d(new QmlProfilerClientManagerPrivate)
{
    setObjectName(QLatin1String("QML Profiler Connections"));

    d->profilerState = 0;

    d->connection = 0;
    d->connectionAttempts = 0;
    d->flushInterval = 0;
    d->aggregateTraces = true;

    d->modelManager = 0;

    d->connectionTimer.setInterval(200);
    connect(&d->connectionTimer, &QTimer::timeout, this, &QmlProfilerClientManager::tryToConnect);
}

QmlProfilerClientManager::~QmlProfilerClientManager()
{
    delete d->connection;
    delete d->qmlclientplugin.data();
    delete d;
}

void QmlProfilerClientManager::setModelManager(QmlProfilerModelManager *m)
{
    d->modelManager = m;
}

void QmlProfilerClientManager::setFlushInterval(quint32 flushInterval)
{
    d->flushInterval = flushInterval;
}

bool QmlProfilerClientManager::aggregateTraces() const
{
    return d->aggregateTraces;
}

void QmlProfilerClientManager::setAggregateTraces(bool aggregateTraces)
{
    d->aggregateTraces = aggregateTraces;
}

void QmlProfilerClientManager::setTcpConnection(QString host, quint64 port)
{
    d->tcpHost = host;
    d->tcpPort = port;
    d->localSocket.clear();
    disconnectClient();
    // Wait for the application to announce the port before connecting.
}

void QmlProfilerClientManager::setLocalSocket(QString file)
{
    d->localSocket = file;
    d->tcpHost.clear();
    d->tcpPort = 0;
    disconnectClient();
    // We open the server and the application connects to it, so let's do that right away.
    connectLocalClient(file);
}

void QmlProfilerClientManager::clearBufferedData()
{
    if (d->qmlclientplugin)
        d->qmlclientplugin.data()->clearData();
}

void QmlProfilerClientManager::discardPendingData()
{
    clearBufferedData();
}

void QmlProfilerClientManager::connectTcpClient(quint16 port)
{
    if (d->connection) {
        if (port == d->tcpPort) {
            tryToConnect();
            return;
        } else {
            delete d->connection;
        }
    }

    createConnection();
    d->connectionTimer.start();
    d->tcpPort = port;
    d->connection->connectToHost(d->tcpHost, d->tcpPort);
}

void QmlProfilerClientManager::connectLocalClient(const QString &file)
{
    if (d->connection) {
        if (file == d->localSocket)
            return;
        else
            delete d->connection;
    }

    createConnection();
    d->localSocket = file;
    d->connection->startLocalServer(file);
}

void QmlProfilerClientManager::createConnection()
{
    d->connection = new QmlDebugConnection;
    QTC_ASSERT(d->profilerState, return);

    disconnectClientSignals();
    d->profilerState->setServerRecording(false); // false by default (will be set to true when connected)
    delete d->qmlclientplugin.data();
    d->profilerState->setRecordedFeatures(0);
    d->qmlclientplugin = new QmlProfilerTraceClient(d->connection,
                                                    d->profilerState->requestedFeatures());
    d->qmlclientplugin->setFlushInterval(d->flushInterval);
    connectClientSignals();
    connect(d->connection, &QmlDebugConnection::connected,
            this, &QmlProfilerClientManager::qmlDebugConnectionOpened);
    connect(d->connection, &QmlDebugConnection::disconnected,
            this, &QmlProfilerClientManager::qmlDebugConnectionClosed);
    connect(d->connection, &QmlDebugConnection::socketError,
            this, &QmlProfilerClientManager::qmlDebugConnectionError);
    connect(d->connection, &QmlDebugConnection::socketStateChanged,
            this, &QmlProfilerClientManager::qmlDebugConnectionStateChanged);
}

void QmlProfilerClientManager::connectClientSignals()
{
    QTC_ASSERT(d->profilerState, return);
    if (d->qmlclientplugin) {
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::complete,
                this, &QmlProfilerClientManager::qmlComplete);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::newEngine,
                this, &QmlProfilerClientManager::qmlNewEngine);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::rangedEvent,
                d->modelManager, &QmlProfilerModelManager::addQmlEvent);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::debugMessage,
                d->modelManager, &QmlProfilerModelManager::addDebugMessage);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::traceFinished,
                d->modelManager->traceTime(), &QmlProfilerTraceTime::increaseEndTime);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::traceStarted,
                d->modelManager->traceTime(), &QmlProfilerTraceTime::decreaseStartTime);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::recordingChanged,
                d->profilerState, &QmlProfilerStateManager::setServerRecording);
        connect(d->profilerState, &QmlProfilerStateManager::requestedFeaturesChanged,
                d->qmlclientplugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                d->profilerState, &QmlProfilerStateManager::setRecordedFeatures);
    }
}

void QmlProfilerClientManager::disconnectClientSignals()
{
    if (d->qmlclientplugin) {
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::complete,
                   this, &QmlProfilerClientManager::qmlComplete);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::newEngine,
                   this, &QmlProfilerClientManager::qmlNewEngine);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::rangedEvent,
                   d->modelManager, &QmlProfilerModelManager::addQmlEvent);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::debugMessage,
                   d->modelManager, &QmlProfilerModelManager::addDebugMessage);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::traceFinished,
                   d->modelManager->traceTime(), &QmlProfilerTraceTime::increaseEndTime);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::traceStarted,
                   d->modelManager->traceTime(), &QmlProfilerTraceTime::decreaseStartTime);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::recordingChanged,
                   d->profilerState, &QmlProfilerStateManager::setServerRecording);
        disconnect(d->profilerState, &QmlProfilerStateManager::requestedFeaturesChanged,
                   d->qmlclientplugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                   d->profilerState, &QmlProfilerStateManager::setRecordedFeatures);
    }
}

bool QmlProfilerClientManager::isConnected() const
{
    return d->connection && d->connection->isConnected();
}

void QmlProfilerClientManager::disconnectClient()
{
    // this might be actually be called indirectly by QDDConnectionPrivate::readyRead(), therefore allow
    // function to complete before deleting object
    if (d->connection) {
        d->connection->deleteLater();
        d->connection = 0;
    }
}

void QmlProfilerClientManager::tryToConnect()
{
    ++d->connectionAttempts;

    if (d->connection && d->connection->isConnected()) {
        d->connectionTimer.stop();
        d->connectionAttempts = 0;
    } else if (d->connection && d->connection->socketState() != QAbstractSocket::ConnectedState) {
        if (d->connectionAttempts < 3) {
            // Replace the connection after trying for some time. On some operating systems (OSX) the
            // very first connection to a TCP server takes a very long time to get established.

            // delete directly here, so that any pending events aren't delivered. We don't want the
            // connection first to be established and then torn down again.
            delete d->connection;
            d->connection = 0;
            connectTcpClient(d->tcpPort);
        } else if (!d->connection->isConnecting()) {
            d->connection->connectToHost(d->tcpHost, d->tcpPort);
        }
    } else if (d->connectionAttempts == 50) {
        d->connectionTimer.stop();
        d->connectionAttempts = 0;
        delete d->connection; // delete directly.
        d->connection = 0;

        QMessageBox *infoBox = QmlProfilerTool::requestMessageBox();
        infoBox->setIcon(QMessageBox::Critical);
        infoBox->setWindowTitle(tr("Qt Creator"));
        infoBox->setText(tr("Could not connect to the in-process QML profiler.\n"
                            "Do you want to retry?"));
        infoBox->setStandardButtons(QMessageBox::Retry |
                                    QMessageBox::Cancel |
                                    QMessageBox::Help);
        infoBox->setDefaultButton(QMessageBox::Retry);
        infoBox->setModal(true);

        connect(infoBox, &QDialog::finished,
                this, &QmlProfilerClientManager::retryMessageBoxFinished);

        infoBox->show();
    }
}

void QmlProfilerClientManager::qmlDebugConnectionOpened()
{
    logState(tr("Debug connection opened"));
    clientRecordingChanged();
}

void QmlProfilerClientManager::qmlDebugConnectionClosed()
{
    logState(tr("Debug connection closed"));
    disconnectClient();
    emit connectionClosed();
}

void QmlProfilerClientManager::qmlDebugConnectionError(QAbstractSocket::SocketError error)
{
    logState(QmlDebugConnection::socketErrorToString(error));
    if (d->connection->isConnected()) {
        disconnectClient();
        emit connectionClosed();
    } else {
        disconnectClient();
    }
}

void QmlProfilerClientManager::qmlDebugConnectionStateChanged(QAbstractSocket::SocketState state)
{
    logState(QmlDebugConnection::socketStateToString(state));
}

void QmlProfilerClientManager::logState(const QString &msg)
{
    QString state = QLatin1String("QML Profiler: ") + msg;
    if (QmlProfilerPlugin::debugOutput)
        qWarning() << state;
    QmlProfilerTool::logState(state);
}

void QmlProfilerClientManager::retryMessageBoxFinished(int result)
{
    QTC_ASSERT(!d->connection, disconnectClient());

    switch (result) {
    case QMessageBox::Retry: {
        connectTcpClient(d->tcpPort);
        d->connectionAttempts = 0;
        d->connectionTimer.start();
        break;
    }
    case QMessageBox::Help: {
        QmlProfilerTool::handleHelpRequest(QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html"));
        // fall through
    }
    default: {
        // The actual error message has already been logged.
        logState(tr("Failed to connect!"));
        emit connectionFailed();
        break;
    }
    }
}

void QmlProfilerClientManager::qmlComplete(qint64 maximumTime)
{
    d->modelManager->traceTime()->increaseEndTime(maximumTime);
    if (d->modelManager && !d->aggregateTraces)
        d->modelManager->acquiringDone();
}

void QmlProfilerClientManager::qmlNewEngine(int engineId)
{
    if (d->qmlclientplugin->isRecording() != d->profilerState->clientRecording())
        d->qmlclientplugin->setRecording(d->profilerState->clientRecording());
    else
        d->qmlclientplugin->sendRecordingStatus(engineId);
}

void QmlProfilerClientManager::registerProfilerStateManager( QmlProfilerStateManager *profilerState )
{
    if (d->profilerState) {
        disconnect(d->profilerState, &QmlProfilerStateManager::stateChanged,
                   this, &QmlProfilerClientManager::profilerStateChanged);
        disconnect(d->profilerState, &QmlProfilerStateManager::clientRecordingChanged,
                   this, &QmlProfilerClientManager::clientRecordingChanged);
    }

    d->profilerState = profilerState;

    // connect
    if (d->profilerState) {
        connect(d->profilerState, &QmlProfilerStateManager::stateChanged,
                this, &QmlProfilerClientManager::profilerStateChanged);
        connect(d->profilerState, &QmlProfilerStateManager::clientRecordingChanged,
                this, &QmlProfilerClientManager::clientRecordingChanged);
    }
}

void QmlProfilerClientManager::profilerStateChanged()
{
    QTC_ASSERT(d->profilerState, return);
    switch (d->profilerState->currentState()) {
    case QmlProfilerStateManager::AppStopRequested :
        if (d->profilerState->serverRecording()) {
            if (d->qmlclientplugin)
                d->qmlclientplugin.data()->setRecording(false);
        } else {
            d->profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        }
        break;
    default:
        break;
    }
}

void QmlProfilerClientManager::clientRecordingChanged()
{
    QTC_ASSERT(d->profilerState, return);
    if (d->qmlclientplugin)
        d->qmlclientplugin->setRecording(d->profilerState->clientRecording());
}

} // namespace Internal
} // namespace QmlProfiler
