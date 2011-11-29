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

#include "qmladapter.h"

#include "debuggerstartparameters.h"
#include "qscriptdebuggerclient.h"
#include "qmlv8debuggerclient.h"
#include "qmljsprivateapi.h"

#include "qmlengine.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

class QmlAdapterPrivate
{
public:
    explicit QmlAdapterPrivate(DebuggerEngine *engine)
        : m_engine(engine)
        , m_qmlClient(0)
        , m_engineDebugClient(0)
        , m_conn(0)
        , m_currentSelectedDebugId(-1)
    {
        m_connectionTimer.setInterval(4000);
        m_connectionTimer.setSingleShot(true);
    }

    QWeakPointer<DebuggerEngine> m_engine;
    QmlDebuggerClient *m_qmlClient;
    QmlJsDebugClient::QDeclarativeEngineDebug *m_engineDebugClient;
    QTimer m_connectionTimer;
    QDeclarativeDebugConnection *m_conn;
    QHash<QString, QmlDebuggerClient*> debugClients;
    int m_currentSelectedDebugId;
    QString m_currentSelectedDebugName;
};

} // namespace Internal

QmlAdapter::QmlAdapter(DebuggerEngine *engine, QObject *parent)
    : QObject(parent), d(new Internal::QmlAdapterPrivate(engine))
{
    connect(&d->m_connectionTimer, SIGNAL(timeout()), SLOT(checkConnectionState()));
    d->m_conn = new QDeclarativeDebugConnection(this);
    connect(d->m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(d->m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionErrorOccurred(QAbstractSocket::SocketError)));

    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(this);

    createDebuggerClients();
}

QmlAdapter::~QmlAdapter()
{
    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();

    if (pluginManager->allObjects().contains(this))
        pluginManager->removeObject(this);
    delete d;
}

void QmlAdapter::beginConnection()
{
    if (d->m_engine.isNull()
            || (d->m_conn && d->m_conn->state() != QAbstractSocket::UnconnectedState))
        return;

    const DebuggerStartParameters &parameters = d->m_engine.data()->startParameters();
    if (parameters.communicationChannel == DebuggerStartParameters::CommunicationChannelUsb) {
        const QString &port = parameters.remoteChannel;
        showConnectionStatusMessage(tr("Connecting to debug server on %1").arg(port));
        d->m_conn->connectToOst(port);
    } else {
        const QString &address = parameters.qmlServerAddress;
        quint16 port = parameters.qmlServerPort;
        showConnectionStatusMessage(tr("Connecting to debug server %1:%2").arg(address).arg(QString::number(port)));
        d->m_conn->connectToHost(address, port);
    }

    //A timeout to check the connection state
    d->m_connectionTimer.start();
}

void QmlAdapter::closeConnection()
{
    if (d->m_connectionTimer.isActive()) {
        d->m_connectionTimer.stop();
    } else {
        if (d->m_conn) {
            d->m_conn->close();
        }
    }
}

void QmlAdapter::connectionErrorOccurred(QAbstractSocket::SocketError socketError)
{
    showConnectionStatusMessage(tr("Error: (%1) %2", "%1=error code, %2=error message")
                                .arg(socketError).arg(d->m_conn->errorString()));

    // this is only an error if we are already connected and something goes wrong.
    if (isConnected()) {
        emit connectionError(socketError);
    } else {
        d->m_connectionTimer.stop();
        emit connectionStartupFailed();
    }
}

void QmlAdapter::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (QDeclarativeDebugClient *client = qobject_cast<QDeclarativeDebugClient*>(sender()))
        serviceName = client->name();

    logServiceStatusChange(serviceName, status);

    if (status == QDeclarativeDebugClient::Enabled) {
        d->m_qmlClient = d->debugClients.value(serviceName);
        d->m_qmlClient->flushSendBuffer();
        d->m_qmlClient->startSession();
    }
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

        d->m_connectionTimer.stop();

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

void QmlAdapter::checkConnectionState()
{
    if (!isConnected()) {
        closeConnection();
        emit connectionStartupFailed();
    }
}

void QmlAdapter::createDebuggerClients()
{

    Internal::QScriptDebuggerClient *client1 = new Internal::QScriptDebuggerClient(d->m_conn);
    connect(client1, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));

    Internal::QmlV8DebuggerClient *client2 = new Internal::QmlV8DebuggerClient(d->m_conn);
    connect(client2, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));

    d->debugClients.insert(client1->name(),client1);
    d->debugClients.insert(client2->name(),client2);


    client1->setEngine((Internal::QmlEngine*)(d->m_engine.data()));
    client2->setEngine((Internal::QmlEngine*)(d->m_engine.data()));

    //engine->startSuccessful();  // FIXME: AAA: port to new debugger states
}

bool QmlAdapter::isConnected() const
{
    return d->m_conn && d->m_qmlClient && d->m_conn->state() == QAbstractSocket::ConnectedState;
}

QDeclarativeDebugConnection *QmlAdapter::connection() const
{
    return d->m_conn;
}

DebuggerEngine *QmlAdapter::debuggerEngine() const
{
    return d->m_engine.data();
}

void QmlAdapter::showConnectionStatusMessage(const QString &message)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QLatin1String("QML Debugger: ") + message, LogStatus);
}

void QmlAdapter::showConnectionErrorMessage(const QString &message)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QLatin1String("QML Debugger: ") + message, LogError);
}

bool QmlAdapter::disableJsDebugging(bool block)
{
    if (d->m_engine.isNull())
        return block;

    bool isBlocked = d->m_engine.data()->state() == InferiorRunOk;

    if (isBlocked == block)
        return block;

    if (block) {
        d->m_engine.data()->continueInferior();
    } else {
        d->m_engine.data()->requestInterruptInferior();
    }

    return isBlocked;
}

Internal::QmlDebuggerClient *QmlAdapter::activeDebuggerClient()
{
    return d->m_qmlClient;
}

QHash<QString, Internal::QmlDebuggerClient*> QmlAdapter::debuggerClients()
{
    return d->debugClients;
}

QmlJsDebugClient::QDeclarativeEngineDebug *QmlAdapter::engineDebugClient() const
{
    return d->m_engineDebugClient;
}

void QmlAdapter::setEngineDebugClient(QmlJsDebugClient::QDeclarativeEngineDebug *client)
{
    d->m_engineDebugClient = client;
}

int QmlAdapter::currentSelectedDebugId() const
{
    return d->m_currentSelectedDebugId;
}

QString QmlAdapter::currentSelectedDisplayName() const
{
    return d->m_currentSelectedDebugName;
}

void QmlAdapter::setCurrentSelectedDebugInfo(int currentDebugId, const QString &displayName)
{
    d->m_currentSelectedDebugId = currentDebugId;
    d->m_currentSelectedDebugName = displayName;
    emit selectionChanged();
}

void QmlAdapter::logServiceStatusChange(const QString &service,
                                        QDeclarativeDebugClient::Status newStatus)
{
    switch (newStatus) {
    case QDeclarativeDebugClient::Unavailable: {
        showConnectionStatusMessage(tr("Status of '%1' changed to 'unavailable'.").arg(service));
        break;
    }
    case QDeclarativeDebugClient::Enabled: {
        showConnectionStatusMessage(tr("Status of '%1' changed to 'enabled'.").arg(service));
        break;
    }

    case QDeclarativeDebugClient::NotConnected: {
        showConnectionStatusMessage(tr("Status of '%1' changed to 'not connected'.").arg(service));
        break;
    }
    }
}

void QmlAdapter::logServiceActivity(const QString &service, const QString &logMessage)
{
    if (!d->m_engine.isNull())
        d->m_engine.data()->showMessage(QString("%1 %2").arg(service, logMessage), LogDebug);
}

} // namespace Debugger
