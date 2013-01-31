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

#include "qmlprofilerclientmanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilerplugin.h"

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmlprofilertraceclient.h>
#include <qmldebug/qv8profilerclient.h>

#include <utils/qtcassert.h>
#include <QPointer>
#include <QTimer>
#include <QMessageBox>

using namespace QmlDebug;
using namespace Core;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerClientManager::QmlProfilerClientManagerPrivate {
public:
    QmlProfilerClientManagerPrivate(QmlProfilerClientManager *qq) { Q_UNUSED(qq); }

    QmlProfilerStateManager* profilerState;

    QmlDebugConnection *connection;
    QPointer<QmlProfilerTraceClient> qmlclientplugin;
    QPointer<QV8ProfilerClient> v8clientplugin;

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

    bool v8DataReady;
    bool qmlDataReady;
};

QmlProfilerClientManager::QmlProfilerClientManager(QObject *parent) :
    QObject(parent), d(new QmlProfilerClientManagerPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler Connections"));

    d->profilerState = 0;

    d->connection = 0;
    d->connectionAttempts = 0;
    d->v8DataReady = false;
    d->qmlDataReady = false;

    d->connectionTimer.setInterval(200);
    connect(&d->connectionTimer, SIGNAL(timeout()), SLOT(tryToConnect()));
}

QmlProfilerClientManager::~QmlProfilerClientManager()
{
    disconnectClientSignals();
    delete d->connection;
    delete d->qmlclientplugin.data();
    delete d->v8clientplugin.data();

    delete d;
}
////////////////////////////////////////////////////////////////
// Interface
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
    if (d->v8clientplugin)
        d->v8clientplugin.data()->clearData();
}

void QmlProfilerClientManager::discardPendingData()
{
    if (d->connection)
        d->connection->flush();
    clearBufferedData();
}

////////////////////////////////////////////////////////////////
// Internal
void QmlProfilerClientManager::connectClient(quint16 port)
{
    if (d->connection)
        delete d->connection;
    d->connection = new QmlDebugConnection;
    enableServices();
    connect(d->connection, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(connectionStateChanged()));
    d->connectionTimer.start();
    d->tcpPort = port;
}

void QmlProfilerClientManager::enableServices()
{
    QTC_ASSERT(d->profilerState, return);

    disconnectClientSignals();
    d->profilerState->setServerRecording(false); // false by default (will be set to true when connected)
    delete d->qmlclientplugin.data();
    d->qmlclientplugin = new QmlProfilerTraceClient(d->connection);
    delete d->v8clientplugin.data();
    d->v8clientplugin = new QV8ProfilerClient(d->connection);
    connectClientSignals();
}

void QmlProfilerClientManager::connectClientSignals()
{
    QTC_ASSERT(d->profilerState, return);
    if (d->qmlclientplugin) {
        connect(d->qmlclientplugin.data(), SIGNAL(complete()),
                this, SLOT(qmlComplete()));
        connect(d->qmlclientplugin.data(),
                SIGNAL(range(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation)),
                this,
                SIGNAL(addRangedEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation)));
        connect(d->qmlclientplugin.data(), SIGNAL(traceFinished(qint64)),
                this, SIGNAL(traceFinished(qint64)));
        connect(d->qmlclientplugin.data(), SIGNAL(traceStarted(qint64)),
                this, SIGNAL(traceStarted(qint64)));
        connect(d->qmlclientplugin.data(), SIGNAL(frame(qint64,int,int)),
                this, SIGNAL(addFrameEvent(qint64,int,int)));
        connect(d->qmlclientplugin.data(), SIGNAL(enabledChanged()),
                d->qmlclientplugin.data(), SLOT(sendRecordingStatus()));
        // fixme: this should be unified for both clients
        connect(d->qmlclientplugin.data(), SIGNAL(recordingChanged(bool)),
                d->profilerState, SLOT(setServerRecording(bool)));
    }
    if (d->v8clientplugin) {
        connect(d->v8clientplugin.data(), SIGNAL(complete()), this, SLOT(v8Complete()));
        connect(d->v8clientplugin.data(),
                SIGNAL(v8range(int,QString,QString,int,double,double)),
                this,
                SIGNAL(addV8Event(int,QString,QString,int,double,double)));
        connect(d->v8clientplugin.data(), SIGNAL(enabledChanged()),
                d->v8clientplugin.data(), SLOT(sendRecordingStatus()));
    }
}

void QmlProfilerClientManager::disconnectClientSignals()
{
    if (d->qmlclientplugin) {
        disconnect(d->qmlclientplugin.data(), SIGNAL(complete()),
                   this, SLOT(qmlComplete()));
        disconnect(d->qmlclientplugin.data(),
                   SIGNAL(range(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation)),
                   this,
                   SIGNAL(addRangedEvent(int,int,qint64,qint64,QStringList,QmlDebug::QmlEventLocation)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(traceFinished(qint64)),
                   this, SIGNAL(traceFinished(qint64)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(traceStarted(qint64)),
                   this, SIGNAL(traceStarted(qint64)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(frame(qint64,int,int)),
                   this, SIGNAL(addFrameEvent(qint64,int,int)));
        disconnect(d->qmlclientplugin.data(), SIGNAL(enabledChanged()),
                   d->qmlclientplugin.data(), SLOT(sendRecordingStatus()));
        // fixme: this should be unified for both clients
        disconnect(d->qmlclientplugin.data(), SIGNAL(recordingChanged(bool)),
                   d->profilerState, SLOT(setServerRecording(bool)));
    }
    if (d->v8clientplugin) {
        disconnect(d->v8clientplugin.data(), SIGNAL(complete()), this, SLOT(v8Complete()));
        disconnect(d->v8clientplugin.data(),
                   SIGNAL(v8range(int,QString,QString,int,double,double)),
                   this,
                   SIGNAL(addV8Event(int,QString,QString,int,double,double)));
        disconnect(d->v8clientplugin.data(), SIGNAL(enabledChanged()),
                   d->v8clientplugin.data(), SLOT(sendRecordingStatus()));
    }
}

void QmlProfilerClientManager::connectToClient()
{
    if (!d->connection || d->connection->state() != QAbstractSocket::UnconnectedState)
        return;

    QmlProfilerTool::logStatus(QString::fromLatin1("QML Profiler: Connecting to %1:%2 ...")
                               .arg(d->tcpHost, QString::number(d->tcpPort)));
    d->connection->connectToHost(d->tcpHost, d->tcpPort);
}

bool QmlProfilerClientManager::isConnected() const
{
    return d->connection && d->connection->isConnected();
}

void QmlProfilerClientManager::disconnectClient()
{
    // this might be actually be called indirectly by QDDConnectionPrivate::readyRead(), therefore allow
    // method to complete before deleting object
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

void QmlProfilerClientManager::connectionStateChanged()
{
    if (!d->connection)
        return;
    switch (d->connection->state()) {
    case QAbstractSocket::UnconnectedState:
    {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: disconnected");
        disconnectClient();
        emit connectionClosed();
        break;
    }
    case QAbstractSocket::HostLookupState:
        break;
    case QAbstractSocket::ConnectingState: {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: Connecting to debug server ...");
        break;
    }
    case QAbstractSocket::ConnectedState:
    {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: connected and running");
            // notify the client recording status
            clientRecordingChanged();
        break;
    }
    case QAbstractSocket::ClosingState:
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QML Profiler: closing ...");
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
}

void QmlProfilerClientManager::retryMessageBoxFinished(int result)
{
    switch (result) {
    case QMessageBox::Retry: {
        d->connectionAttempts = 0;
        d->connectionTimer.start();
        break;
    }
    case QMessageBox::Help: {
        QmlProfilerTool::handleHelpRequest(QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html"));
        // fall through
    }
    default: {
        if (d->connection)
            QmlProfilerTool::logStatus(QLatin1String("QML Profiler: Failed to connect! ") + d->connection->errorString());
        else
            QmlProfilerTool::logStatus(QLatin1String("QML Profiler: Failed to connect!"));

        emit connectionFailed();
        break;
    }
    }
}

void QmlProfilerClientManager::qmlComplete()
{
    d->qmlDataReady = true;
    if (!d->v8clientplugin || d->v8clientplugin.data()->status() != QmlDebug::Enabled || d->v8DataReady) {
        emit dataReadyForProcessing();
        // once complete is sent, reset the flags
        d->qmlDataReady = false;
        d->v8DataReady = false;
    }
}

void QmlProfilerClientManager::v8Complete()
{
    d->v8DataReady = true;
    if (!d->qmlclientplugin || d->qmlclientplugin.data()->status() != QmlDebug::Enabled || d->qmlDataReady) {
        emit dataReadyForProcessing();
        // once complete is sent, reset the flags
        d->v8DataReady = false;
        d->qmlDataReady = false;
    }
}

void QmlProfilerClientManager::stopClientsRecording()
{
    if (d->qmlclientplugin)
        d->qmlclientplugin.data()->setRecording(false);
    if (d->v8clientplugin)
        d->v8clientplugin.data()->setRecording(false);
}

////////////////////////////////////////////////////////////////
// Profiler State
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
        if (d->v8clientplugin)
            d->v8clientplugin.data()->setRecording(d->profilerState->clientRecording());
    }
}

void QmlProfilerClientManager::serverRecordingChanged()
{
    if (d->profilerState->serverRecording()) {
        d->v8DataReady = false;
        d->qmlDataReady = false;
    }
}

} // namespace Internal
} // namespace QmlProfiler
