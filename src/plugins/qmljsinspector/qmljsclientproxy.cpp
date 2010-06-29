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
#include "qmljsclientproxy.h"
#include "qmljsdebuggerclient.h"
#include "qmljsprivateapi.h"

#include <utils/qtcassert.h>

#include <QUrl>
#include <QAbstractSocket>
#include <QDebug>

using namespace QmlJSInspector::Internal;

ClientProxy *ClientProxy::m_instance = 0;

ClientProxy::ClientProxy(QObject *parent) :
    QObject(parent),
    m_conn(0),
    m_client(0),
    m_engineQuery(0),
    m_contextQuery(0),
    m_objectTreeQuery(0),
    m_debuggerRunControl(0)
{
    m_instance = this;
}

ClientProxy *ClientProxy::instance()
{
    return m_instance;
}

bool ClientProxy::connectToViewer(const QString &host, quint16 port)
{
    if (m_conn && m_conn->state() != QAbstractSocket::UnconnectedState)
        return false;

    qDebug() << Q_FUNC_INFO;
    if (m_client) {
        disconnect(m_client, SIGNAL(propertyDumpReceived(QDeclarativeDebugPropertyDump)),
                   this,     SIGNAL(propertyDumpReceived(QDeclarativeDebugPropertyDump)));
        disconnect(m_client, SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
                   this,     SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)));

        emit aboutToDisconnect();

        delete m_client; m_client = 0;
    }

    if (m_conn) {
        m_conn->disconnectFromHost();
        delete m_conn;
        m_conn = 0;
    }

    m_conn = new QDeclarativeDebugConnection(this);
    connect(m_conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(connectionStateChanged()));
    connect(m_conn, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(connectionError()));

    emit connectionStatusMessage(tr("[Inspector] set to connect to debug server %1:%2").arg(host).arg(port));
    m_conn->connectToHost(host, port);


    // blocks until connected; if no connection is available, will fail immediately
    if (!m_conn->waitForConnected())
        return false;

//  ### commented out as the code resulted in asserts
//    QTC_ASSERT(m_debuggerRunControl, return false);
//    Debugger::Internal::QmlEngine *engine = qobject_cast<Debugger::Internal::QmlEngine *>(m_debuggerRunControl->engine());
//    QTC_ASSERT(engine, return false);
//    (void) new DebuggerClient(m_conn, engine);

    return true;
}

void ClientProxy::disconnectFromViewer()
{
    m_conn->disconnectFromHost();
    emit disconnected();
}

void ClientProxy::connectionError()
{
    emit connectionStatusMessage(tr("[Inspector] error: (%1) %2", "%1=error code, %2=error message")
                                .arg(m_conn->error()).arg(m_conn->errorString()));
}

void ClientProxy::connectionStateChanged()
{
    switch (m_conn->state()) {
        case QAbstractSocket::UnconnectedState:
        {
            emit connectionStatusMessage(tr("[Inspector] disconnected.\n\n"));

            delete m_engineQuery;
            m_engineQuery = 0;
            delete m_contextQuery;
            m_contextQuery = 0;

            if (m_objectTreeQuery) {
                delete m_objectTreeQuery;
                m_objectTreeQuery = 0;
            }

            emit disconnected();

            break;
        }
        case QAbstractSocket::HostLookupState:
            emit connectionStatusMessage(tr("[Inspector] resolving host..."));
            break;
        case QAbstractSocket::ConnectingState:
            emit connectionStatusMessage(tr("[Inspector] connecting to debug server..."));
            break;
        case QAbstractSocket::ConnectedState:
        {
            emit connectionStatusMessage(tr("[Inspector] connected.\n"));

            if (!m_client) {
                qDebug() << "CREATING ENGINE";
                m_client = new QDeclarativeEngineDebug(m_conn, this);
                emit connected(m_client);

                connect(m_client,
                        SIGNAL(propertyDumpReceived(QDeclarativeDebugPropertyDump)),
                        SIGNAL(propertyDumpReceived(QDeclarativeDebugPropertyDump)));
                connect(m_client,
                        SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)),
                        SIGNAL(selectedItemsChanged(QList<QDeclarativeDebugObjectReference>)));
            }

            reloadEngines();

            break;
        }
        case QAbstractSocket::ClosingState:
            emit connectionStatusMessage(tr("[Inspector] closing..."));
            break;
        case QAbstractSocket::BoundState:
        case QAbstractSocket::ListeningState:
            break;
    }
}

bool ClientProxy::isConnected() const
{
    return (m_conn && m_client && m_conn->state() == QAbstractSocket::ConnectedState);
}

bool ClientProxy::isUnconnected() const
{
    return (!m_conn || m_conn->state() == QAbstractSocket::UnconnectedState);
}

void ClientProxy::setSelectedItemByObjectId(int engineId, const QDeclarativeDebugObjectReference &objectRef)
{
#ifdef __GNUC__
#  warning implement ClientProxy::setSelectedItemByObjectId
#endif
    qDebug() << "TODO:" << Q_FUNC_INFO;
#if 0
    if (isConnected())
        m_client->setSelectedItemByObjectId(engineId, objectRef);
#endif
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences(const QUrl &url) const
{
    return objectReferences(url, m_rootObject);
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences(const QUrl &url,
                                                                      const QDeclarativeDebugObjectReference &objectRef) const
{
    QList<QDeclarativeDebugObjectReference> result;
    if (objectRef.source().url() == url)
        result.append(objectRef);

    foreach(const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        result.append(objectReferences(url, child));
    }

    return result;
}

QDeclarativeDebugExpressionQuery *ClientProxy::setBindingForObject(int objectDebugId,
                                                                   const QString &propertyName,
                                                                   const QVariant &value,
                                                                   bool isLiteralValue)
{
#ifdef __GNUC__
#  warning implement ClientProxy::setBindingForObject
#endif
    qDebug() << "TODO:" << Q_FUNC_INFO;
#if 0
    if (propertyName == QLatin1String("id") || objectDebugId == -1)
        return 0;

    qDebug() << "executeBinding():" << objectDebugId << propertyName << value << "isLiteral:" << isLiteralValue;

    return m_client->setBindingForObject(objectDebugId, propertyName, value.toString(), isLiteralValue, 0);
#else
    return 0;
#endif
}

void ClientProxy::queryEngineContext(int id)
{
    if (id < 0)
        return;

    if (m_contextQuery) {
        delete m_contextQuery;
        m_contextQuery = 0;
    }

    m_contextQuery = m_client->queryRootContexts(QDeclarativeDebugEngineReference(id), this);
    if (!m_contextQuery->isWaiting())
        contextChanged();
    else
        QObject::connect(m_contextQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(contextChanged()));
}

void ClientProxy::contextChanged()
{
    if (m_contextQuery) {

        if (m_contextQuery->rootContext().objects().isEmpty()) {
            m_rootObject = QDeclarativeDebugObjectReference();
            emit objectTreeUpdated(m_rootObject);
        } else {
            m_rootObject = m_contextQuery->rootContext().objects().first();
        }

        delete m_contextQuery; m_contextQuery = 0;

        m_objectTreeQuery = m_client->queryObjectRecursive(m_rootObject, this);
        if (!m_objectTreeQuery->isWaiting()) {
            objectTreeFetched();
        } else {
            connect(m_objectTreeQuery,
                    SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                    SLOT(objectTreeFetched(QDeclarativeDebugQuery::State)));
        }
    }

}

void ClientProxy::objectTreeFetched(QDeclarativeDebugQuery::State state)
{
    if (state != QDeclarativeDebugQuery::Completed) {
        m_rootObject = QDeclarativeDebugObjectReference();
        return;
    }

    m_rootObject = m_objectTreeQuery->object();

    delete m_objectTreeQuery;
    m_objectTreeQuery = 0;

    emit objectTreeUpdated(m_rootObject);
}

void ClientProxy::reloadQmlViewer(int engineId)
{
#ifdef __GNUC__
#  warning implement ClientProxy::reloadQmlViewer
#endif
    Q_UNUSED(engineId);
    qDebug() << "TODO:" << Q_FUNC_INFO;
#if 0
    if (m_client && m_conn->isConnected()) {
        m_client->reloadQmlViewer(engineId);
    }
#endif
}

void ClientProxy::reloadEngines()
{
    if (m_engineQuery) {
        emit connectionStatusMessage("[Inspector] Waiting for response to previous engine query");
        return;
    }

    emit aboutToReloadEngines();

    m_engineQuery = m_client->queryAvailableEngines(this);
    if (!m_engineQuery->isWaiting())
        updateEngineList();
    else
        QObject::connect(m_engineQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(updateEngineList()));
}

QList<QDeclarativeDebugEngineReference> ClientProxy::engines() const
{
    return m_engines;
}

void ClientProxy::updateEngineList()
{
    m_engines = m_engineQuery->engines();
    delete m_engineQuery; m_engineQuery = 0;

    emit enginesChanged();
}
