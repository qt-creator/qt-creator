/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerclientmanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilerplugin.h"

#include <qmldebug/qmldebugclient.h>
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

    enum ConnectMode {
        TcpConnection, OstConnection
    };
    ConnectMode connectMode;
    QString tcpHost;
    quint64 tcpPort;
    QString ostDevice;
    QString sysroot;

    bool qmlDataReady;

    QmlProfilerModelManager *modelManager;
};

QmlProfilerClientManager::QmlProfilerClientManager(QObject *parent) :
    QObject(parent), d(new QmlProfilerClientManagerPrivate)
{
    setObjectName(QLatin1String("QML Profiler Connections"));

    d->profilerState = 0;

    d->connection = 0;
    d->connectionAttempts = 0;
    d->qmlDataReady = false;

    d->modelManager = 0;

    d->connectionTimer.setInterval(200);
    connect(&d->connectionTimer, SIGNAL(timeout()), SLOT(tryToConnect()));
}

QmlProfilerClientManager::~QmlProfilerClientManager()
{
    delete d->connection;
    delete d->qmlclientplugin.data();
    delete d;
}

void QmlProfilerClientManager::setModelManager(QmlProfilerModelManager *m)
{
    if (d->modelManager)
        disconnect(this,SIGNAL(dataReadyForProcessing()), d->modelManager, SLOT(complete()));
    d->modelManager = m;
    if (d->modelManager)
        connect(this,SIGNAL(dataReadyForProcessing()), d->modelManager, SLOT(complete()));
}

void QmlProfilerClientManager::setTcpConnection(QString host, quint64 port)
{
    d->connectMode = QmlProfilerClientManagerPrivate::TcpConnection;
    d->tcpHost = host;
    d->tcpPort = port;
}

void QmlProfilerClientManager::setOstConnection(QString ostDevice)
{
    d->connectMode = QmlProfilerClientManagerPrivate::OstConnection;
    d->ostDevice = ostDevice;
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

void QmlProfilerClientManager::connectClient(quint16 port)
{
    if (d->connection)
        delete d->connection;
    d->connection = new QmlDebugConnection;
    enableServices();
    connect(d->connection, SIGNAL(stateMessage(QString)), this, SLOT(logState(QString)));
    connect(d->connection, SIGNAL(errorMessage(QString)), this, SLOT(logState(QString)));
    connect(d->connection, SIGNAL(opened()), this, SLOT(qmlDebugConnectionOpened()));
    connect(d->connection, SIGNAL(closed()), this, SLOT(qmlDebugConnectionClosed()));
    d->connectionTimer.start();
    d->tcpPort = port;
}

void QmlProfilerClientManager::enableServices()
{
    QTC_ASSERT(d->profilerState, return);

    disconnectClientSignals();
    d->profilerState->setServerRecording(false); // false by default (will be set to true when connected)
    delete d->qmlclientplugin.data();
    d->profilerState->setRecordedFeatures(0);
    d->qmlclientplugin = new QmlProfilerTraceClient(d->connection,
                                                    d->profilerState->requestedFeatures());
    connectClientSignals();
}

void QmlProfilerClientManager::connectClientSignals()
{
    QTC_ASSERT(d->profilerState, return);
    if (d->qmlclientplugin) {
        connect(d->qmlclientplugin.data(), SIGNAL(complete(qint64)),
                this, SLOT(qmlComplete(qint64)));
        connect(d->qmlclientplugin.data(),
                SIGNAL(rangedEvent(QmlDebug::Message,QmlDebug::RangeType,int,qint64,qint64,
                                   QString,QmlDebug::QmlEventLocation,qint64,qint64,qint64,
                                   qint64,qint64)),
                d->modelManager,
                SLOT(addQmlEvent(QmlDebug::Message,QmlDebug::RangeType,int,qint64,qint64,
                                 QString,QmlDebug::QmlEventLocation,qint64,qint64,qint64,qint64,
                                 qint64)));
        connect(d->qmlclientplugin.data(), SIGNAL(traceFinished(qint64,QList<int>)),
                d->modelManager->traceTime(), SLOT(increaseEndTime(qint64)));
        connect(d->qmlclientplugin.data(), SIGNAL(traceStarted(qint64,QList<int>)),
                d->modelManager->traceTime(), SLOT(decreaseStartTime(qint64)));
        connect(d->qmlclientplugin.data(), SIGNAL(enabledChanged()),
                d->qmlclientplugin.data(), SLOT(sendRecordingStatus()));
        connect(d->qmlclientplugin.data(), SIGNAL(recordingChanged(bool)),
                d->profilerState, SLOT(setServerRecording(bool)));
        connect(d->profilerState, &QmlProfilerStateManager::requestedFeaturesChanged,
                d->qmlclientplugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
        connect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                d->profilerState, &QmlProfilerStateManager::setRecordedFeatures);
    }
}

void QmlProfilerClientManager::disconnectClientSignals()
{
    if (d->qmlclientplugin) {
        disconnect(d->qmlclientplugin.data(), SIGNAL(complete(qint64)),
                   this, SLOT(qmlComplete(qint64)));
        disconnect(d->qmlclientplugin.data(),
                   SIGNAL(rangedEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation,
                                qint64,qint64,qint64,qint64,qint64)),
                   d->modelManager,
                   SLOT(addQmlEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation,
                                    qint64,qint64,qint64,qint64,qint64)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(traceFinished(qint64)),
                   d->modelManager->traceTime(), SLOT(increaseEndTime(qint64)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(traceStarted(qint64)),
                   d->modelManager->traceTime(), SLOT(decreaseStartTime(qint64)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(enabledChanged()),
                   d->qmlclientplugin.data(), SLOT(sendRecordingStatus()));
        // fixme: this should be unified for both clients
        disconnect(d->qmlclientplugin.data(), SIGNAL(recordingChanged(bool)),
                   d->profilerState, SLOT(setServerRecording(bool)));
        disconnect(d->profilerState, &QmlProfilerStateManager::requestedFeaturesChanged,
                   d->qmlclientplugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
        disconnect(d->qmlclientplugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                   d->profilerState, &QmlProfilerStateManager::setRecordedFeatures);
    }
}

void QmlProfilerClientManager::connectToClient()
{
    if (!d->connection || d->connection->isOpen() || d->connection->isConnecting())
        return;

    d->connection->connectToHost(d->tcpHost, d->tcpPort);
}

bool QmlProfilerClientManager::isConnected() const
{
    return d->connection && d->connection->isOpen();
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

    if (d->connection && d->connection->isOpen()) {
        d->connectionTimer.stop();
        d->connectionAttempts = 0;
    } else if (d->connectionAttempts == 50) {
        d->connectionTimer.stop();
        d->connectionAttempts = 0;

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

        connect(infoBox, SIGNAL(finished(int)),
                this, SLOT(retryMessageBoxFinished(int)));

        infoBox->show();
    } else {
        connectToClient();
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

void QmlProfilerClientManager::logState(const QString &msg)
{
    QString state = QLatin1String("QML Profiler: ") + msg;
    if (QmlProfilerPlugin::debugOutput)
        qWarning() << state;
    QmlProfilerTool::logState(state);
}

void QmlProfilerClientManager::retryMessageBoxFinished(int result)
{
    if (d->connection) {
        QTC_ASSERT(!d->connection->isOpen(), return);
        if (d->connection->isConnecting())
            d->connection->disconnect();
    }

    switch (result) {
    case QMessageBox::Retry: {
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
    d->qmlDataReady = true;
    emit dataReadyForProcessing();
    // once complete is sent, reset the flags
    d->qmlDataReady = false;
}

void QmlProfilerClientManager::stopClientsRecording()
{
    if (d->qmlclientplugin)
        d->qmlclientplugin.data()->setRecording(false);
}

void QmlProfilerClientManager::registerProfilerStateManager( QmlProfilerStateManager *profilerState )
{
    if (d->profilerState) {
        disconnect(d->profilerState, SIGNAL(stateChanged()),
                   this, SLOT(profilerStateChanged()));
        disconnect(d->profilerState, SIGNAL(clientRecordingChanged()),
                   this, SLOT(clientRecordingChanged()));
        disconnect(d->profilerState, SIGNAL(serverRecordingChanged()),
                   this, SLOT(serverRecordingChanged()));
    }

    d->profilerState = profilerState;

    // connect
    if (d->profilerState) {
        connect(d->profilerState, SIGNAL(stateChanged()),
                this, SLOT(profilerStateChanged()));
        connect(d->profilerState, SIGNAL(clientRecordingChanged()),
                this, SLOT(clientRecordingChanged()));
        connect(d->profilerState, SIGNAL(serverRecordingChanged()),
                this, SLOT(serverRecordingChanged()));
    }
}

void QmlProfilerClientManager::profilerStateChanged()
{
    QTC_ASSERT(d->profilerState, return);
    switch (d->profilerState->currentState()) {
    case QmlProfilerStateManager::AppStopRequested :
        if (d->profilerState->serverRecording())
            stopClientsRecording();
        else
            d->profilerState->setCurrentState(QmlProfilerStateManager::AppReadyToStop);
        break;
    default:
        break;
    }
}

void QmlProfilerClientManager::clientRecordingChanged()
{
    QTC_ASSERT(d->profilerState, return);
    if (d->profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        if (d->qmlclientplugin)
            d->qmlclientplugin.data()->setRecording(d->profilerState->clientRecording());
    }
}

void QmlProfilerClientManager::serverRecordingChanged()
{
    if (d->profilerState->serverRecording())
        d->qmlDataReady = false;
}

} // namespace Internal
} // namespace QmlProfiler
