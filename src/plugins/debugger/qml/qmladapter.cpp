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

#include <QAbstractSocket>
#include <QTimer>
#include <QDebug>

namespace Debugger {
namespace Internal {

QmlAdapter::QmlAdapter(DebuggerEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_qmlClient(0)
    , m_mainClient(0)
    , m_connectionTimer(new QTimer(this))
    , m_connectionAttempts(0)
    , m_conn(0)

{
    m_connectionTimer->setInterval(200);
    connect(m_connectionTimer, SIGNAL(timeout()), SLOT(pollInferior()));
}

void QmlAdapter::beginConnection()
{
    m_connectionAttempts = 0;
    m_connectionTimer->start();
}

void QmlAdapter::pauseConnection()
{
    m_connectionTimer->stop();
}

void QmlAdapter::closeConnection()
{
    if (m_connectionTimer->isActive()) {
        m_connectionTimer->stop();
    } else {
        if (m_conn) {
            m_conn->disconnectFromHost();
        }
    }
}

void QmlAdapter::pollInferior()
{
    ++m_connectionAttempts;

    if (connectToViewer()) {
        m_connectionTimer->stop();
        m_connectionAttempts = 0;
    } else if (m_connectionAttempts == m_maxConnectionAttempts) {
        emit connectionStartupFailed();
        m_connectionTimer->stop();
        m_connectionAttempts = 0;
    }
}

bool QmlAdapter::connectToViewer()
{
    if (m_engine.isNull() || (m_conn && m_conn->state() != QAbstractSocket::UnconnectedState))
        return false;

    m_conn = new QDeclarativeDebugConnection(this);
    connect(m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionErrorOccurred()));

    QString address = m_engine.data()->startParameters().qmlServerAddress;
    QString port = QString::number(m_engine.data()->startParameters().qmlServerPort);
    showConnectionStatusMessage(tr("Connect to debug server %1:%2").arg(address).arg(port));
    m_conn->connectToHost(m_engine.data()->startParameters().qmlServerAddress,
                          m_engine.data()->startParameters().qmlServerPort);

    // blocks until connected; if no connection is available, will fail immediately
    if (!m_conn->waitForConnected())
        return false;

    return true;
}

void QmlAdapter::connectionErrorOccurred()
{
    showConnectionErrorMessage(tr("Error: (%1) %2", "%1=error code, %2=error message")
                                .arg(m_conn->error()).arg(m_conn->errorString()));

    // this is only an error if we are already connected and something goes wrong.
    if (isConnected())
        emit connectionError();
}

void QmlAdapter::connectionStateChanged()
{
    switch (m_conn->state()) {
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

            if (!m_mainClient) {
                m_mainClient = new QDeclarativeEngineDebug(m_conn, this);
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
    m_qmlClient = new QmlDebuggerClient(m_conn);

    connect(m_engine.data(), SIGNAL(sendMessage(QByteArray)),
            m_qmlClient, SLOT(slotSendMessage(QByteArray)));
    connect(m_qmlClient, SIGNAL(messageWasReceived(QByteArray)),
            m_engine.data(), SLOT(messageReceived(QByteArray)));

    //engine->startSuccessful();  // FIXME: AAA: port to new debugger states
}

bool QmlAdapter::isConnected() const
{
    return m_conn && m_qmlClient && m_conn->state() == QAbstractSocket::ConnectedState;
}

bool QmlAdapter::isUnconnected() const
{
    return !m_conn || m_conn->state() == QAbstractSocket::UnconnectedState;
}

QDeclarativeEngineDebug *QmlAdapter::client() const
{
    return m_mainClient;
}

QDeclarativeDebugConnection *QmlAdapter::connection() const
{
    if (!isConnected())
        return 0;

    return m_conn;
}

void QmlAdapter::showConnectionStatusMessage(const QString &message)
{
    if (!m_engine.isNull())
        m_engine.data()->showMessage(QLatin1String("QmlJSDebugger: ") + message, LogStatus);
}

void QmlAdapter::showConnectionErrorMessage(const QString &message)
{
    if (!m_engine.isNull())
        m_engine.data()->showMessage(QLatin1String("QmlJSDebugger: ") + message, LogError);
}

void QmlAdapter::setMaxConnectionAttempts(int maxAttempts)
{
    m_maxConnectionAttempts = maxAttempts;
}
void QmlAdapter::setConnectionAttemptInterval(int interval)
{
    m_connectionTimer->setInterval(interval);
}

} // namespace Internal
} // namespace Debugger
