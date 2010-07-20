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
#include "qmljsdesigndebugclient.h"

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
    m_designClient(0),
    m_engineQuery(0),
    m_contextQuery(0),
    m_objectTreeQuery(0)
{
    Q_ASSERT(! m_instance);
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

    if (m_designClient) {

        disconnect(m_designClient, SIGNAL(currentObjectsChanged(QList<int>)),
                            this, SLOT(onCurrentObjectsChanged(QList<int>)));
        disconnect(m_designClient,
                   SIGNAL(colorPickerActivated()), this, SIGNAL(colorPickerActivated()));
        disconnect(m_designClient,
                   SIGNAL(zoomToolActivated()), this, SIGNAL(zoomToolActivated()));
        disconnect(m_designClient,
                   SIGNAL(selectToolActivated()), this, SIGNAL(selectToolActivated()));
        disconnect(m_designClient,
                   SIGNAL(selectMarqueeToolActivated()), this, SIGNAL(selectMarqueeToolActivated()));
        disconnect(m_designClient,
                   SIGNAL(animationSpeedChanged(qreal)), this, SIGNAL(animationSpeedChanged(qreal)));
        disconnect(m_designClient,
                   SIGNAL(designModeBehaviorChanged(bool)), this, SIGNAL(designModeBehaviorChanged(bool)));

        emit aboutToDisconnect();

        delete m_client;
        m_client = 0;

        delete m_designClient;
        m_designClient = 0;
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

    return true;
}

void ClientProxy::refreshObjectTree()
{
    if (!m_objectTreeQuery) {
        m_objectTreeQuery = m_client->queryObjectRecursive(m_rootObject, this);

        if (!m_objectTreeQuery->isWaiting()) {
            objectTreeFetched(m_objectTreeQuery->state());
        } else {
            connect(m_objectTreeQuery,
                    SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                    SLOT(objectTreeFetched(QDeclarativeDebugQuery::State)));
        }
    }
}

void ClientProxy::onCurrentObjectsChanged(const QList<int> &debugIds)
{
    QList<QDeclarativeDebugObjectReference> selectedItems;

    foreach(int debugId, debugIds) {
        QDeclarativeDebugObjectReference ref = objectReferenceForId(debugId);
        if (ref.debugId() != -1) {
            selectedItems << ref;
        } else {
            // ### FIXME right now, there's no way in the protocol to
            // a) get some item and know its parent (although that's possible by adding it to a separate plugin)
            // b) add children to part of an existing tree.
            // So the only choice that remains is to update the complete tree when we have an unknown debug id.
            if (!m_objectTreeQuery)
                m_objectTreeQuery = m_client->queryObjectRecursive(m_rootObject, this);
            break;
        }
    }

    if (m_objectTreeQuery) {
        if (!m_objectTreeQuery->isWaiting()) {
            objectTreeFetched(m_objectTreeQuery->state());
        } else {
            connect(m_objectTreeQuery,
                    SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                    SLOT(objectTreeFetched(QDeclarativeDebugQuery::State)));
        }
    } else {
        emit selectedItemsChanged(selectedItems);
    }
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
                m_client = new QDeclarativeEngineDebug(m_conn, this);
                m_designClient = new QmlJSDesignDebugClient(m_conn, this);
                emit connected(m_client);

                connect(m_designClient,
                        SIGNAL(currentObjectsChanged(QList<int>)),
                        SLOT(onCurrentObjectsChanged(QList<int>)));
                connect(m_designClient,
                        SIGNAL(colorPickerActivated()), SIGNAL(colorPickerActivated()));
                connect(m_designClient,
                        SIGNAL(zoomToolActivated()), SIGNAL(zoomToolActivated()));
                connect(m_designClient,
                        SIGNAL(selectToolActivated()), SIGNAL(selectToolActivated()));
                connect(m_designClient,
                        SIGNAL(selectMarqueeToolActivated()), SIGNAL(selectMarqueeToolActivated()));
                connect(m_designClient,
                        SIGNAL(animationSpeedChanged(qreal)), SIGNAL(animationSpeedChanged(qreal)));
                connect(m_designClient,
                        SIGNAL(designModeBehaviorChanged(bool)), SIGNAL(designModeBehaviorChanged(bool)));
                connect(m_designClient, SIGNAL(reloaded()), this, SIGNAL(serverReloaded()));
            }

            (void) new DebuggerClient(m_conn);

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

void ClientProxy::setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objectRefs)
{
    if (isConnected() && m_designClient)
        m_designClient->setSelectedItemsByObjectId(objectRefs);
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(int debugId) const
{
    return objectReferenceForId(debugId, m_rootObject);
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(int debugId,
                                                                   const QDeclarativeDebugObjectReference &objectRef) const
{
    if (objectRef.debugId() == debugId)
        return objectRef;

    foreach(const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        QDeclarativeDebugObjectReference result = objectReferenceForId(debugId, child);
        if (result.debugId() == debugId)
            return result;
    }

    return QDeclarativeDebugObjectReference();
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences(const QUrl &url) const
{
    return objectReferences(url, m_rootObject);
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences(const QUrl &url,
                                                                      const QDeclarativeDebugObjectReference &objectRef) const
{
    QList<QDeclarativeDebugObjectReference> result;
    if (objectRef.source().url() == url || url.isEmpty())
        result.append(objectRef);

    foreach(const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        result.append(objectReferences(url, child));
    }

    return result;
}

bool ClientProxy::setBindingForObject(int objectDebugId,
                                      const QString &propertyName,
                                      const QVariant &value,
                                      bool isLiteralValue)
{
    qDebug() << "setBindingForObject():" << objectDebugId << propertyName << value;
    if (objectDebugId == -1)
        return false;

    if (propertyName == QLatin1String("id"))
        return false; // Crashes the QMLViewer.

    return m_client->setBindingForObject(objectDebugId, propertyName, value.toString(), isLiteralValue);
}

bool ClientProxy::setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody)
{
    qDebug() << "setMethodBodyForObject():" << objectDebugId << methodName << methodBody;
    if (objectDebugId == -1)
        return 0;
    return m_client->setMethodBody(objectDebugId, methodName, methodBody);
}

bool ClientProxy::resetBindingForObject(int objectDebugId, const QString& propertyName)
{
    qDebug() << "resetBindingForObject():" << objectDebugId << propertyName;
    if (objectDebugId == -1)
        return false;
    //    if (propertyName == QLatin1String("id"))  return false;
    return m_client->resetBindingForObject(objectDebugId, propertyName);
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
        connect(m_contextQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
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

        delete m_contextQuery;
        m_contextQuery = 0;

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
    if (state == QDeclarativeDebugQuery::Error) {
        delete m_objectTreeQuery;
        m_objectTreeQuery = 0;
    }

    if (state != QDeclarativeDebugQuery::Completed) {
        m_rootObject = QDeclarativeDebugObjectReference();
        return;
    }

    m_rootObject = m_objectTreeQuery->object();

    delete m_objectTreeQuery;
    m_objectTreeQuery = 0;

    emit objectTreeUpdated(m_rootObject);

    if (isDesignClientConnected() && !m_designClient->selectedItemIds().isEmpty()) {
        onCurrentObjectsChanged(m_designClient->selectedItemIds());
    }

}

void ClientProxy::reloadQmlViewer()
{
    if (isDesignClientConnected())
        m_designClient->reloadViewer();
}

void ClientProxy::setDesignModeBehavior(bool inDesignMode)
{
    if (isDesignClientConnected())
        m_designClient->setDesignModeBehavior(inDesignMode);
}

void ClientProxy::setAnimationSpeed(qreal slowdownFactor)
{
    if (isDesignClientConnected())
        m_designClient->setAnimationSpeed(slowdownFactor);
}

void ClientProxy::changeToColorPickerTool()
{
    if (isDesignClientConnected())
        m_designClient->changeToColorPickerTool();
}

void ClientProxy::changeToZoomTool()
{
    if (isDesignClientConnected())
        m_designClient->changeToZoomTool();
}
void ClientProxy::changeToSelectTool()
{
    if (isDesignClientConnected())
        m_designClient->changeToSelectTool();
}

void ClientProxy::changeToSelectMarqueeTool()
{
    if (isDesignClientConnected())
        m_designClient->changeToSelectMarqueeTool();
}

void ClientProxy::createQmlObject(const QString &qmlText, const QDeclarativeDebugObjectReference &parentRef,
                                  const QStringList &imports, const QString &filename)
{
    if (isDesignClientConnected())
        m_designClient->createQmlObject(qmlText, parentRef, imports, filename);
}

void QmlJSInspector::Internal::ClientProxy::destroyQmlObject(int debugId)
{
    if (isDesignClientConnected())
        m_designClient->destroyQmlObject(debugId);
}


bool ClientProxy::isDesignClientConnected() const
{
    return (m_designClient && m_conn->isConnected());
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
        connect(m_engineQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(updateEngineList()));
}

QList<QDeclarativeDebugEngineReference> ClientProxy::engines() const
{
    return m_engines;
}

void ClientProxy::updateEngineList()
{
    m_engines = m_engineQuery->engines();
    delete m_engineQuery;
    m_engineQuery = 0;

    emit enginesChanged();
}
