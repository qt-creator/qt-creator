/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmladapter.h"
#include "qmldebuggerclient.h"
#include "qmljsprivateapi.h"

#include "debuggerengine.h"

#include <QtCore/QTimer>
#include <QtCore/QDebug>

namespace Debugger {

struct QmlAdapterPrivate {
    explicit QmlAdapterPrivate(DebuggerEngine *engine, QmlAdapter *q);

    QWeakPointer<DebuggerEngine> m_engine;
    Internal::QmlDebuggerClient *m_qmlClient;
    QDeclarativeEngineDebug *m_mainClient;

    QTimer *m_connectionTimer;
    int m_connectionAttempts;
    int m_maxConnectionAttempts;
    QDeclarativeDebugConnection *m_conn;
};

QmlAdapterPrivate::QmlAdapterPrivate(DebuggerEngine *engine, QmlAdapter *q) :
  m_engine(engine)
, m_qmlClient(0)
, m_mainClient(0)
, m_connectionTimer(new QTimer(q))
, m_connectionAttempts(0)
, m_conn(0)
{
}

QmlAdapter::QmlAdapter(DebuggerEngine *engine, QObject *parent)
    : QObject(parent), d(new QmlAdapterPrivate(engine, this))
{
    d->m_connectionTimer->setInterval(200);
    connect(d->m_connectionTimer, SIGNAL(timeout()), SLOT(pollInferior()));
}

QmlAdapter::~QmlAdapter()
{
}

void QmlAdapter::beginConnection()
{
    d->m_connectionAttempts = 0;
    d->m_connectionTimer->start();
}

void QmlAdapter::pauseConnection()
{
    d->m_connectionTimer->stop();
}

void QmlAdapter::closeConnection()
{
    if (d->m_connectionTimer->isActive()) {
        d->m_connectionTimer->stop();
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
        d->m_connectionTimer->stop();
        d->m_connectionAttempts = 0;
    } else if (d->m_connectionAttempts == d->m_maxConnectionAttempts) {
        emit connectionStartupFailed();
        d->m_connectionTimer->stop();
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

void QmlAdapter::connectionErrorOccurred(QAbstractSocket::SocketError socketError)
{
    showConnectionErrorMessage(tr("Error: (%1) %2", "%1=error code, %2=error message")
                                .arg(d->m_conn->error()).arg(d->m_conn->errorString()));

    // this is only an error if we are already connected and something goes wrong.
    if (isConnected())
        emit connectionError(socketError);
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

    connect(d->m_engine.data(), SIGNAL(sendMessage(QByteArray)),
            d->m_qmlClient, SLOT(slotSendMessage(QByteArray)));
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
    d->m_connectionTimer->setInterval(interval);
}

} // namespace Debugger
