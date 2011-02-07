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

#include "qmljsclientproxy.h"
#include "qmljsprivateapi.h"
#include "qmljsobserverclient.h"
#include "qmljsinspector.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/qml/qmlengine.h>
#include <debugger/qml/qmladapter.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <projectexplorer/project.h>

#include <QtCore/QUrl>
#include <QtNetwork/QAbstractSocket>

using namespace QmlJSInspector::Internal;

ClientProxy::ClientProxy(Debugger::QmlAdapter *adapter, QObject *parent)
    : QObject(parent)
    , m_adapter(adapter)
    , m_engineClient(m_adapter->client())
    , m_observerClient(0)
    , m_engineQuery(0)
    , m_contextQuery(0)
    , m_isConnected(false)
{
    m_requestObjectsTimer.setSingleShot(true);
    m_requestObjectsTimer.setInterval(3000);
    connect(m_engineClient, SIGNAL(newObjects()), this, SLOT(newObjects()));
    connect(&m_requestObjectsTimer, SIGNAL(timeout()), this, SLOT(refreshObjectTree()));
    connectToServer();
}

void ClientProxy::connectToServer()
{
    m_observerClient = new QmlJSObserverClient(m_adapter->connection(), this);

    connect(m_observerClient, SIGNAL(connectedStatusChanged(QDeclarativeDebugClient::Status)),
             this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(m_observerClient, SIGNAL(currentObjectsChanged(QList<int>)),
        SLOT(onCurrentObjectsChanged(QList<int>)));
    connect(m_observerClient, SIGNAL(colorPickerActivated()),
        SIGNAL(colorPickerActivated()));
    connect(m_observerClient, SIGNAL(zoomToolActivated()),
        SIGNAL(zoomToolActivated()));
    connect(m_observerClient, SIGNAL(selectToolActivated()),
        SIGNAL(selectToolActivated()));
    connect(m_observerClient, SIGNAL(selectMarqueeToolActivated()),
        SIGNAL(selectMarqueeToolActivated()));
    connect(m_observerClient, SIGNAL(animationSpeedChanged(qreal)),
        SIGNAL(animationSpeedChanged(qreal)));
    connect(m_observerClient, SIGNAL(designModeBehaviorChanged(bool)),
        SIGNAL(designModeBehaviorChanged(bool)));
    connect(m_observerClient, SIGNAL(showAppOnTopChanged(bool)),
        SIGNAL(showAppOnTopChanged(bool)));
    connect(m_observerClient, SIGNAL(reloaded()), this,
        SIGNAL(serverReloaded()));
    connect(m_observerClient, SIGNAL(selectedColorChanged(QColor)),
        SIGNAL(selectedColorChanged(QColor)));
    connect(m_observerClient, SIGNAL(contextPathUpdated(QStringList)),
        SIGNAL(contextPathUpdated(QStringList)));
    connect(m_observerClient, SIGNAL(logActivity(QString,QString)),
            m_adapter, SLOT(logServiceActivity(QString,QString)));

    updateConnected();
}

void ClientProxy::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (QDeclarativeDebugClient *client = qobject_cast<QDeclarativeDebugClient*>(sender())) {
        serviceName = client->name();
    }

    m_adapter->logServiceStatusChange(serviceName, status);

    updateConnected();
}

void ClientProxy::disconnectFromServer()
{
    if (m_observerClient) {
        disconnect(m_observerClient, SIGNAL(connectedStatusChanged(QDeclarativeDebugClient::Status)),
                 this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
        disconnect(m_observerClient, SIGNAL(currentObjectsChanged(QList<int>)),
            this, SLOT(onCurrentObjectsChanged(QList<int>)));
        disconnect(m_observerClient, SIGNAL(colorPickerActivated()),
            this, SIGNAL(colorPickerActivated()));
        disconnect(m_observerClient, SIGNAL(zoomToolActivated()),
            this, SIGNAL(zoomToolActivated()));
        disconnect(m_observerClient, SIGNAL(selectToolActivated()),
            this, SIGNAL(selectToolActivated()));
        disconnect(m_observerClient, SIGNAL(selectMarqueeToolActivated()),
            this, SIGNAL(selectMarqueeToolActivated()));
        disconnect(m_observerClient, SIGNAL(animationSpeedChanged(qreal)),
            this, SIGNAL(animationSpeedChanged(qreal)));
        disconnect(m_observerClient, SIGNAL(designModeBehaviorChanged(bool)),
            this, SIGNAL(designModeBehaviorChanged(bool)));
        disconnect(m_observerClient, SIGNAL(selectedColorChanged(QColor)),
            this, SIGNAL(selectedColorChanged(QColor)));
        disconnect(m_observerClient, SIGNAL(contextPathUpdated(QStringList)),
            this, SIGNAL(contextPathUpdated(QStringList)));
        disconnect(m_observerClient, SIGNAL(logActivity(QString,QString)),
                   m_adapter, SLOT(logServiceActivity(QString,QString)));

        delete m_observerClient;
        m_observerClient = 0;
    }

    if (m_engineQuery)
        delete m_engineQuery;
    m_engineQuery = 0;

    if (m_contextQuery)
        delete m_contextQuery;
    m_contextQuery = 0;

    qDeleteAll(m_objectTreeQuery);
    m_objectTreeQuery.clear();

    removeAllObjectWatches();

    updateConnected();
}

void ClientProxy::refreshObjectTree()
{
    if (!m_contextQuery) {
        m_requestObjectsTimer.stop();
        qDeleteAll(m_objectTreeQuery);
        m_objectTreeQuery.clear();
        queryEngineContext(m_engines.value(0).debugId());
    }
}

void ClientProxy::onCurrentObjectsChanged(const QList<int> &debugIds, bool requestIfNeeded)
{
    QList<QDeclarativeDebugObjectReference> selectedItems;

    foreach (int debugId, debugIds) {
        QDeclarativeDebugObjectReference ref = objectReferenceForId(debugId);
        if (ref.debugId() != -1) {
            selectedItems << ref;
        } else if (requestIfNeeded) {
            // ### FIXME right now, there's no way in the protocol to
            // a) get some item and know its parent (although that's possible
            //    by adding it to a separate plugin)
            // b) add children to part of an existing tree.
            // So the only choice that remains is to update the complete
            // tree when we have an unknown debug id.
            // break;
        }
    }

    emit selectedItemsChanged(selectedItems);
}

void ClientProxy::setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objectRefs)
{
    if (isConnected()) {
        QList<int> debugIds;

        foreach (const QDeclarativeDebugObjectReference &ref, objectRefs) {
            debugIds << ref.debugId();
        }

        m_observerClient->setCurrentObjects(debugIds);
    }
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(int debugId) const
{
    foreach (const QDeclarativeDebugObjectReference& it, m_rootObjects) {
        QDeclarativeDebugObjectReference result = objectReferenceForId(debugId, it);
        if (result.debugId() == debugId)
            return result;
    }
    return QDeclarativeDebugObjectReference();
}

void ClientProxy::log(LogDirection direction, const QString &message)
{
    QString msg;
    if (direction == LogSend) {
        msg += " sending ";
    } else {
        msg += " receiving ";
    }
    msg += message;

    m_adapter->logServiceActivity("QDeclarativeDebug", msg);
}

QList<QDeclarativeDebugObjectReference> QmlJSInspector::Internal::ClientProxy::rootObjectReference() const
{
    return m_rootObjects;
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(int debugId,
                                                                   const QDeclarativeDebugObjectReference &objectRef) const
{
    if (objectRef.debugId() == debugId)
        return objectRef;

    foreach (const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        QDeclarativeDebugObjectReference result = objectReferenceForId(debugId, child);
        if (result.debugId() == debugId)
            return result;
    }

    return QDeclarativeDebugObjectReference();
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForId(const QString &objectId) const
{
    if (!objectId.isEmpty() && objectId[0].isLower()) {
        const QList<QDeclarativeDebugObjectReference> refs = objectReferences();
        foreach (const QDeclarativeDebugObjectReference &ref, refs) {
            if (ref.idString() == objectId)
                return ref;
        }
    }
    return QDeclarativeDebugObjectReference();
}

QDeclarativeDebugObjectReference ClientProxy::objectReferenceForLocation(const int line, const int column) const
{
    const QList<QDeclarativeDebugObjectReference> refs = objectReferences();
    foreach (const QDeclarativeDebugObjectReference &ref, refs) {
        if (ref.source().lineNumber() == line && ref.source().columnNumber() == column)
            return ref;
    }

    return QDeclarativeDebugObjectReference();
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences() const
{
    QList<QDeclarativeDebugObjectReference> result;
    foreach (const QDeclarativeDebugObjectReference &it, m_rootObjects) {
        result.append(objectReferences(it));
    }
    return result;
}

QList<QDeclarativeDebugObjectReference> ClientProxy::objectReferences(const QDeclarativeDebugObjectReference &objectRef) const
{
    QList<QDeclarativeDebugObjectReference> result;
    result.append(objectRef);

    foreach (const QDeclarativeDebugObjectReference &child, objectRef.children()) {
        result.append(objectReferences(child));
    }

    return result;
}

bool ClientProxy::setBindingForObject(int objectDebugId,
                                      const QString &propertyName,
                                      const QVariant &value,
                                      bool isLiteralValue)
{
    if (objectDebugId == -1)
        return false;

    if (propertyName == QLatin1String("id"))
        return false; // Crashes the QMLViewer.

    log(LogSend, QString("SET_BINDING %1 %2 %3 %4").arg(QString::number(objectDebugId), propertyName, value.toString(), QString(isLiteralValue ? "true" : "false")));

    bool result = m_engineClient->setBindingForObject(objectDebugId, propertyName, value.toString(), isLiteralValue);

    if (!result)
        log(LogSend, QString("failed!"));

    return result;
}

bool ClientProxy::setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody)
{
    if (objectDebugId == -1)
        return 0;

    log(LogSend, QString("SET_METHOD_BODY %1 %2 %3").arg(QString::number(objectDebugId), methodName, methodBody));

    bool result = m_engineClient->setMethodBody(objectDebugId, methodName, methodBody);

    if (!result)
        log(LogSend, QString("failed!"));

    return result;
}

bool ClientProxy::resetBindingForObject(int objectDebugId, const QString& propertyName)
{
    if (objectDebugId == -1)
        return false;

    log(LogSend, QString("RESET_BINDING %1 %2").arg(QString::number(objectDebugId), propertyName));

    bool result = m_engineClient->resetBindingForObject(objectDebugId, propertyName);

    if (!result)
        log(LogSend, QString("failed!"));

    return result;
}

QDeclarativeDebugExpressionQuery *ClientProxy::queryExpressionResult(int objectDebugId, const QString &expr, QObject *parent)
{
    if (objectDebugId != -1) {
        bool block = m_adapter->disableJsDebugging(true);

        log(LogSend, QString("EVAL_EXPRESSION %1 %2").arg(QString::number(objectDebugId), expr));
        QDeclarativeDebugExpressionQuery *query = m_engineClient->queryExpressionResult(objectDebugId,expr,parent);

        m_adapter->disableJsDebugging(block);
        return query;
    }
    return 0;
}

void ClientProxy::clearComponentCache()
{
    if (isConnected())
        m_observerClient->clearComponentCache();
}

bool ClientProxy::addObjectWatch(int objectDebugId)
{
    if (objectDebugId == -1)
        return false;

    // already set
    if (m_objectWatches.keys().contains(objectDebugId))
        return true;

    QDeclarativeDebugObjectReference ref = objectReferenceForId(objectDebugId);
    if (ref.debugId() != objectDebugId)
        return false;

    // is flooding the debugging output log!
    // log(LogSend, QString("WATCH_PROPERTY %1").arg(objectDebugId));

    QDeclarativeDebugWatch *watch = m_engineClient->addWatch(ref, this);
    m_objectWatches.insert(objectDebugId, watch);

    connect(watch,SIGNAL(valueChanged(QByteArray,QVariant)),this,SLOT(objectWatchTriggered(QByteArray,QVariant)));

    return false;
}

void ClientProxy::objectWatchTriggered(const QByteArray &propertyName, const QVariant &propertyValue)
{
    // is flooding the debugging output log!
    // log(LogReceive, QString("UPDATE_WATCH %1 %2").arg(QString::fromAscii(propertyName), propertyValue.toString()));

    QDeclarativeDebugWatch *watch = dynamic_cast<QDeclarativeDebugWatch *>(QObject::sender());
    if (watch)
        emit propertyChanged(watch->objectDebugId(),propertyName, propertyValue);
}

bool ClientProxy::removeObjectWatch(int objectDebugId)
{
    if (objectDebugId == -1)
        return false;

    if (!m_objectWatches.keys().contains(objectDebugId))
        return false;

    QDeclarativeDebugWatch *watch = m_objectWatches.value(objectDebugId);
    disconnect(watch,SIGNAL(valueChanged(QByteArray,QVariant)), this, SLOT(objectWatchTriggered(QByteArray,QVariant)));

    // is flooding the debugging output log!
    // log(LogSend, QString("NO_WATCH %1").arg(QString::number(objectDebugId)));

    m_engineClient->removeWatch(watch);
    delete watch;
    m_objectWatches.remove(objectDebugId);


    return true;
}

void ClientProxy::removeAllObjectWatches()
{
    foreach (int watchedObject, m_objectWatches.keys())
        removeObjectWatch(watchedObject);
    Q_ASSERT(m_objectWatches.count() == 0);
}

void ClientProxy::queryEngineContext(int id)
{
    if (id < 0)
        return;

    if (m_contextQuery) {
        delete m_contextQuery;
        m_contextQuery = 0;
    }

    log(LogSend, QString("LIST_OBJECTS %1").arg(QString::number(id)));

    m_contextQuery = m_engineClient->queryRootContexts(QDeclarativeDebugEngineReference(id), this);
    if (!m_contextQuery->isWaiting())
        contextChanged();
    else
        connect(m_contextQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                this, SLOT(contextChanged()));
}

void ClientProxy::contextChanged()
{
    log(LogReceive, QString("LIST_OBJECTS_R"));
    if (m_contextQuery) {
        m_rootObjects.clear();
        QDeclarativeDebugContextReference rootContext = m_contextQuery->rootContext();
        delete m_contextQuery;
        m_contextQuery = 0;

        qDeleteAll(m_objectTreeQuery);
        m_objectTreeQuery.clear();
        m_requestObjectsTimer.stop();

        fetchContextObjectRecursive(rootContext);
    }
}

void ClientProxy::fetchContextObjectRecursive(const QDeclarativeDebugContextReference& context)
{
    foreach (const QDeclarativeDebugObjectReference & obj, context.objects()) {

        log(LogSend, QString("FETCH_OBJECT %1").arg(obj.idString()));

        QDeclarativeDebugObjectQuery* query = m_engineClient->queryObjectRecursive(obj, this);
        if (!query->isWaiting()) {
            query->deleteLater(); //ignore errors;
        } else {
            m_objectTreeQuery << query;
            connect(query,
                    SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                    SLOT(objectTreeFetched(QDeclarativeDebugQuery::State)));
        }
    }
    foreach (const QDeclarativeDebugContextReference& child, context.contexts()) {
        fetchContextObjectRecursive(child);
    }
}


void ClientProxy::objectTreeFetched(QDeclarativeDebugQuery::State state)
{
    QDeclarativeDebugObjectQuery *query = qobject_cast<QDeclarativeDebugObjectQuery *>(sender());
    if (!query || state == QDeclarativeDebugQuery::Error) {
        delete query;
        return;
    }

    log(LogReceive, QString("FETCH_OBJECT_R %1").arg(query->object().idString()));

    m_rootObjects.append(query->object());

    int removed = m_objectTreeQuery.removeAll(query);
    Q_ASSERT(removed == 1);
    Q_UNUSED(removed);
    delete query;

    if (m_objectTreeQuery.isEmpty()) {
        int old_count = m_debugIdHash.count();
        m_debugIdHash.clear();
        m_debugIdHash.reserve(old_count + 1);
        foreach (const QDeclarativeDebugObjectReference &it, m_rootObjects)
            buildDebugIdHashRecursive(it);
        emit objectTreeUpdated();

        if (isConnected()) {
            if (!m_observerClient->currentObjects().isEmpty())
                onCurrentObjectsChanged(m_observerClient->currentObjects(), false);

            m_observerClient->setObjectIdList(m_rootObjects);
        }
    }
}

void ClientProxy::buildDebugIdHashRecursive(const QDeclarativeDebugObjectReference& ref)
{
    QString filename = ref.source().url().toLocalFile();
    int lineNum = ref.source().lineNumber();
    int colNum = ref.source().columnNumber();
    int rev = 0;

    // handle the case where the url contains the revision number encoded. (for object created by the debugger)
    static QRegExp rx("(.*)_(\\d+):(\\d+)$");
    if (rx.exactMatch(filename)) {
        filename = rx.cap(1);
        rev = rx.cap(2).toInt();
        lineNum += rx.cap(3).toInt() - 1;
    }

    //convert the filename to a canonical filename in case of shadow build.
    bool isShadowBuild = InspectorUi::instance()->isShadowBuildProject();
    if (isShadowBuild && rev == 0) {
        QString shadowBuildDir = InspectorUi::instance()->debugProjectBuildDirectory();

        if (filename.startsWith(shadowBuildDir)) {
            ProjectExplorer::Project *debugProject = InspectorUi::instance()->debugProject();
            filename = debugProject->projectDirectory() + filename.mid(shadowBuildDir.length());
        }
    }

    // append the debug ids in the hash
    m_debugIdHash[qMakePair<QString, int>(filename, rev)][qMakePair<int, int>(lineNum, colNum)].append(ref.debugId());

    foreach (const QDeclarativeDebugObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}


void ClientProxy::reloadQmlViewer()
{
    if (isConnected())
        m_observerClient->reloadViewer();
}

void ClientProxy::setDesignModeBehavior(bool inDesignMode)
{
    if (isConnected())
        m_observerClient->setDesignModeBehavior(inDesignMode);
}

void ClientProxy::setAnimationSpeed(qreal slowdownFactor)
{
    if (isConnected())
        m_observerClient->setAnimationSpeed(slowdownFactor);
}

void ClientProxy::changeToColorPickerTool()
{
    if (isConnected())
        m_observerClient->changeToColorPickerTool();
}

void ClientProxy::changeToZoomTool()
{
    if (isConnected())
        m_observerClient->changeToZoomTool();
}
void ClientProxy::changeToSelectTool()
{
    if (isConnected())
        m_observerClient->changeToSelectTool();
}

void ClientProxy::changeToSelectMarqueeTool()
{
    if (isConnected())
        m_observerClient->changeToSelectMarqueeTool();
}

void ClientProxy::showAppOnTop(bool showOnTop)
{
    if (isConnected())
        m_observerClient->showAppOnTop(showOnTop);
}

void ClientProxy::createQmlObject(const QString &qmlText, int parentDebugId,
                                  const QStringList &imports, const QString &filename)
{
    if (isConnected())
        m_observerClient->createQmlObject(qmlText, parentDebugId, imports, filename);
}

void ClientProxy::destroyQmlObject(int debugId)
{
    if (isConnected())
        m_observerClient->destroyQmlObject(debugId);
}

void ClientProxy::reparentQmlObject(int debugId, int newParent)
{
    if (isConnected())
        m_observerClient->reparentQmlObject(debugId, newParent);
}

void ClientProxy::setContextPathIndex(int contextIndex)
{
    if (isConnected())
        m_observerClient->setContextPathIndex(contextIndex);
}

void ClientProxy::updateConnected()
{
    bool isConnected = m_observerClient && m_observerClient->status() == QDeclarativeDebugClient::Enabled
            && m_engineClient && m_engineClient->status() == QDeclarativeEngineDebug::Enabled;

    if (isConnected != m_isConnected) {
        m_isConnected = isConnected;
        if (isConnected) {
            emit connected();
            reloadEngines();
        } else {
            emit disconnected();
        }
    }
}

void ClientProxy::reloadEngines()
{
    if (m_engineQuery) {
        emit connectionStatusMessage("[Inspector] Waiting for response to previous engine query");
        return;
    }

    emit aboutToReloadEngines();

    log(LogSend, QString("LIST_ENGINES"));

    m_engineQuery = m_engineClient->queryAvailableEngines(this);
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
    log(LogReceive, QString("LIST_ENGINES_R"));

    m_engines = m_engineQuery->engines();
    delete m_engineQuery;
    m_engineQuery = 0;

    emit enginesChanged();
}

Debugger::QmlAdapter *ClientProxy::qmlAdapter() const
{
    return m_adapter;
}

bool ClientProxy::isConnected() const
{
    return m_isConnected;
}

void ClientProxy::newObjects()
{
    log(LogReceive, QString("OBJECT_CREATED"));
    if (!m_requestObjectsTimer.isActive())
        m_requestObjectsTimer.start();
}
