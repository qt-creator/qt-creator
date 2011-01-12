/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmladapter.h"

#include "debuggerstartparameters.h"
#include "qmldebuggerclient.h"
#include "qmljsprivateapi.h"

#include "debuggerengine.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

class QmlAdapterPrivate
{
public:
    explicit QmlAdapterPrivate(DebuggerEngine *engine)
        : m_engine(engine)
        , m_qmlClient(0)
        , m_mainClient(0)
        , m_connectionAttempts(0)
        , m_conn(0)
    {
        m_connectionTimer.setInterval(200);
    }

    QWeakPointer<DebuggerEngine> m_engine;
    Internal::QmlDebuggerClient *m_qmlClient;
    QDeclarativeEngineDebug *m_mainClient;

    QTimer m_connectionTimer;
    int m_connectionAttempts;
    int m_maxConnectionAttempts;
    QDeclarativeDebugConnection *m_conn;
    QList<QByteArray> sendBuffer;
};

} // namespace Internal

QmlAdapter::QmlAdapter(DebuggerEngine *engine, QObject *parent)
    : QObject(parent), d(new Internal::QmlAdapterPrivate(engine))
{
    connect(&d->m_connectionTimer, SIGNAL(timeout()), SLOT(pollInferior()));
}

QmlAdapter::~QmlAdapter()
{
}

void QmlAdapter::beginConnection()
{
    d->m_connectionAttempts = 0;
    d->m_connectionTimer.start();
}

void QmlAdapter::pauseConnection()
{
    d->m_connectionTimer.stop();
}

void QmlAdapter::closeConnection()
{
    if (d->m_connectionTimer.isActive()) {
        d->m_connectionTimer.stop();
    } else {
        if (d->m_conn) {
            d->m_conn->disconnectFromHost();
        }
    }
}

void QmlAdapter::pollInferior()
{
    ++d->m_connectionAttempts;

    if (connectToViewer()) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
    } else if (d->m_connectionAttempts == d->m_maxConnectionAttempts) {
        emit connectionStartupFailed();
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
    }
}

bool QmlAdapter::connectToViewer()
{
    if (d->m_engine.isNull() || (d->m_conn && d->m_conn->state() != QAbstractSocket::UnconnectedState))
        return false;

    d->m_conn = new QDeclarativeDebugConnection(this);
    connect(d->m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(d->m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionErrorOccurred(QAbstractSocket::SocketError)));

    QString address = d->m_engine.data()->startParameters().qmlServerAddress;
    QString port = QString::number(d->m_engine.data()->startParameters().qmlServerPort);
    showConnectionStatusMessage(tr("Connect to debug server %1:%2").arg(address).arg(port));
    d->m_conn->connectToHost(d->m_engine.data()->startParameters().qmlServerAddress,
                          d->m_engine.data()->startParameters().qmlServerPort);


    // blocks until connected; if no connection is available, will fail immediately
    if (!d->m_conn->waitForConnected())
        return false;

    return true;
}

void QmlAdapter::sendMessage(const QByteArray &msg)
{
    if (d->m_qmlClient->status() == QDeclarativeDebugClient::Enabled) {
        flushSendBuffer();
        d->m_qmlClient->sendMessage(msg);
    } else {
        d->sendBuffer.append(msg);
    }
}

void QmlAdapter::connectionErrorOccurred(QAbstractSocket::SocketError socketError)
{
    showConnectionStatusMessage(tr("Error: (%1) %2", "%1=error code, %2=error message")
                                .arg(d->m_conn->error()).arg(d->m_conn->errorString()));

    // this is only an error if we are already connected and something goes wrong.
    if (isConnected())
        emit connectionError(socketError);
}

void QmlAdapter::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (QDeclarativeDebugClient *client = qobject_cast<QDeclarativeDebugClient*>(sender())) {
        serviceName = client->name();
    }

    logServiceStatusChange(serviceName, status);

    if (status == QDeclarativeDebugClient::Enabled)
        flushSendBuffer();
}

void QmlAdapter::connectionStateChanged()
{
    switch (d->m_conn->state()) {
        case QAbstractSocket::UnconnectedState:
        {
            showConnectionStatusMessage(tr("disconnected.\n\n"));
            emit disconnected();

            break;
        }
        case QAbstractSocket::HostLookupState:
            showConnectionStatusMessage(tr("resolving host..."));
            break;
        case QAbstractSocket::ConnectingState:
            showConnectionStatusMessage(tr("connecting to debug server..."));
            break;
        case QAbstractSocket::ConnectedState:
        {
            showConnectionStatusMessage(tr("connected.\n"));

            if (!d->m_mainClient) {
                d->m_mainClient = new QDeclarativeEngineDebug(d->m_conn, this);
            }

            createDebuggerClient();
            //reloadEngines();
            emit connected();
            break;
        }
        case QAbstractSocket::ClosingState:
            showConnectionStatusMessage(tr("closing..."));
            break;
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ListeningState:
            break;
    }
}

void QmlAdapter::createDebuggerClient()
{
    d->m_qmlClient = new Internal::QmlDebuggerClient(d->m_conn);

    connect(d->m_qmlClient, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(d->m_engine.data(), SIGNAL(sendMessage(QByteArray)),
            this, SLOT(sendMessage(QByteArray)));
    connect(d->m_qmlClient, SIGNAL(messageWasReceived(QByteArray)),
            d->m_engine.data(), SLOT(messageReceived(QByteArray)));

    //engine->startSuccessful();  // FIXME: AAA: port to new debugger states
}

bool QmlAdapter::isConnected() const
{
    return d->m_conn && d->m_qmlClient && d->m_conn->state() == QAbstractSocket::ConnectedState;
}

bool QmlAdapter::isUnconnected() const
{
    return !d->m_conn || d->m_conn->state() == QAbstractSocket::UnconnectedState;
}

QDeclarativeEngineDebug *QmlAdapter::client() const
{
    return d->m_mainClient;
}

QDeclarativeDebugConnection *QmlAdapter::connection() const
{
    if (!isConnected())
        return 0;

    return d->m_conn;
}

void QmlAdapter::showConnectionStatusMessage(const QString &message)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QLatin1String("QmlJSDebugger: ") + message, LogStatus);
}

void QmlAdapter::showConnectionErrorMessage(const QString &message)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QLatin1String("QmlJSDebugger: ") + message, LogError);
}

void QmlAdapter::setMaxConnectionAttempts(int maxAttempts)
{
    d->m_maxConnectionAttempts = maxAttempts;
}
void QmlAdapter::setConnectionAttemptInterval(int interval)
{
    d->m_connectionTimer.setInterval(interval);
}

void QmlAdapter::logServiceStatusChange(const QString &service, QDeclarativeDebugClient::Status newStatus)
{
    switch (newStatus) {
    case QDeclarativeDebugClient::Unavailable: {
        showConnectionErrorMessage(tr("Debug service '%1' became unavailable.").arg(service));
        emit serviceConnectionError(service);
        break;
    }
    case QDeclarativeDebugClient::Enabled: {
        showConnectionStatusMessage(tr("Connected to debug service '%1'.").arg(service));
        break;
    }

    case QDeclarativeDebugClient::NotConnected: {
        showConnectionStatusMessage(tr("Not connected to debug service '%1'.").arg(service));
        break;
    }
    }
}

void QmlAdapter::logServiceActivity(const QString &service, const QString &logMessage)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QString("%1 %2").arg(service, logMessage), LogDebug);
}

void QmlAdapter::flushSendBuffer()
{
    QTC_ASSERT(d->m_qmlClient->status() == QDeclarativeDebugClient::Enabled, return);
    foreach (const QByteArray &msg, d->sendBuffer) {
        d->m_qmlClient->sendMessage(msg);
    }
    d->sendBuffer.clear();
}

} // namespace Debugger
