/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljsclientproxy.h"
#include "qmljsprivateapi.h"
#include "qmljsinspectorclient.h"
#include "qmljsinspector.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/qml/qmlengine.h>
#include <debugger/qml/qmladapter.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <projectexplorer/project.h>

#include <QUrl>
#include <QAbstractSocket>

using namespace QmlJSInspector::Internal;

ClientProxy::ClientProxy(Debugger::QmlAdapter *adapter, QObject *parent)
    : QObject(parent)
    , m_adapter(adapter)
    , m_engineClient(0)
    , m_inspectorClient(0)
    , m_engineQueryId(0)
    , m_contextQueryId(0)
    , m_isConnected(false)
{
    connectToServer();
}

ClientProxy::~ClientProxy()
{
    m_adapter.data()->setEngineDebugClient(0);
    m_adapter.data()->setCurrentSelectedDebugInfo(-1);
}

void ClientProxy::connectToServer()
{
    QmlEngineDebugClient *client1 = new QDeclarativeEngineClient(
                m_adapter.data()->connection());
    QmlEngineDebugClient *client2 = new QmlDebuggerClient(
                m_adapter.data()->connection());

    connect(client1, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(client1, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            SLOT(engineClientStatusChanged(QDeclarativeDebugClient::Status)));

    connect(client2, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(client2, SIGNAL(newStatus(QDeclarativeDebugClient::Status)),
            SLOT(engineClientStatusChanged(QDeclarativeDebugClient::Status)));

    m_inspectorClient =
            new QmlJSInspectorClient(m_adapter.data()->connection(), this);

    connect(m_inspectorClient,
            SIGNAL(connectedStatusChanged(QDeclarativeDebugClient::Status)),
            this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(m_inspectorClient, SIGNAL(currentObjectsChanged(QList<int>)),
        SLOT(onCurrentObjectsChanged(QList<int>)));
    connect(m_inspectorClient, SIGNAL(zoomToolActivated()),
        SIGNAL(zoomToolActivated()));
    connect(m_inspectorClient, SIGNAL(selectToolActivated()),
        SIGNAL(selectToolActivated()));
    connect(m_inspectorClient, SIGNAL(selectMarqueeToolActivated()),
        SIGNAL(selectMarqueeToolActivated()));
    connect(m_inspectorClient, SIGNAL(animationSpeedChanged(qreal)),
        SIGNAL(animationSpeedChanged(qreal)));
    connect(m_inspectorClient, SIGNAL(animationPausedChanged(bool)),
        SIGNAL(animationPausedChanged(bool)));
    connect(m_inspectorClient, SIGNAL(designModeBehaviorChanged(bool)),
        SIGNAL(designModeBehaviorChanged(bool)));
    connect(m_inspectorClient, SIGNAL(showAppOnTopChanged(bool)),
        SIGNAL(showAppOnTopChanged(bool)));
    connect(m_inspectorClient, SIGNAL(reloaded()), this,
        SIGNAL(serverReloaded()));
    connect(m_inspectorClient, SIGNAL(logActivity(QString,QString)),
            m_adapter.data(), SLOT(logServiceActivity(QString,QString)));

    updateConnected();
}

void ClientProxy::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (sender()) {
        serviceName = sender()->objectName();
    }

    if (m_adapter)
        m_adapter.data()->logServiceStatusChange(serviceName, status);

    updateConnected();
}

void ClientProxy::engineClientStatusChanged(QDeclarativeDebugClient::Status status)
{
    if (status == QDeclarativeDebugClient::Enabled) {
        m_engineClient = qobject_cast<QmlEngineDebugClient *>(sender());
        connect(m_engineClient, SIGNAL(newObjects()), this, SLOT(newObjects()));
        connect(m_engineClient, SIGNAL(result(quint32,QVariant,QByteArray)),
                SLOT(onResult(quint32,QVariant,QByteArray)));
        connect(m_engineClient, SIGNAL(valueChanged(int,QByteArray,QVariant)),
                SLOT(objectWatchTriggered(int,QByteArray,QVariant)));
        m_adapter.data()->setEngineDebugClient(m_engineClient);
        updateConnected();
    }
}

void ClientProxy::refreshObjectTree()
{
    if (!m_contextQueryId) {
        m_objectTreeQueryIds.clear();
        queryEngineContext(m_engines.value(0).debugId());
    }
}

void ClientProxy::onCurrentObjectsChanged(const QList<int> &debugIds,
                                          bool requestIfNeeded)
{
    QList<QmlDebugObjectReference> selectedItems;
    m_fetchCurrentObjects.clear();
    m_fetchCurrentObjectsQueryIds.clear();
    foreach (int debugId, debugIds) {
        QmlDebugObjectReference ref = objectReferenceForId(debugId);
        if (ref.debugId() != -1 && !ref.needsMoreData()) {
            selectedItems << ref;
        } else if (requestIfNeeded) {
            m_fetchCurrentObjectsQueryIds << fetchContextObject(
                                                 QmlDebugObjectReference(debugId));

        }
    }

    emit selectedItemsChanged(selectedItems);
}

void ClientProxy::onCurrentObjectsFetched(quint32 queryId, const QVariant &result)
{
    m_fetchCurrentObjectsQueryIds.removeOne(queryId);
    QmlDebugObjectReference obj = qvariant_cast<QmlDebugObjectReference>(result);
    m_fetchCurrentObjects.push_front(obj);

    //If this is not a root object, check if we have the parent
    QmlDebugObjectReference parent = objectReferenceForId(obj.parentId());
    if (obj.parentId() != -1 && (parent.debugId() == -1 || parent.needsMoreData())) {
        m_fetchCurrentObjectsQueryIds << fetchContextObject(
                                             QmlDebugObjectReference(obj.parentId()));
        return;
    }

    foreach (const QmlDebugObjectReference &o, m_fetchCurrentObjects)
        addObjectToTree(o);
    emit selectedItemsChanged(QList<QmlDebugObjectReference>() <<
                              m_fetchCurrentObjects.last());
}

void ClientProxy::setSelectedItemsByDebugId(const QList<int> &debugIds)
{
    if (!isConnected())
        return;

    m_inspectorClient->setCurrentObjects(debugIds);
}

void ClientProxy::setSelectedItemsByObjectId(
        const QList<QmlDebugObjectReference> &objectRefs)
{
    if (isConnected()) {
        QList<int> debugIds;

        foreach (const QmlDebugObjectReference &ref, objectRefs) {
            debugIds << ref.debugId();
        }

        m_inspectorClient->setCurrentObjects(debugIds);
    }
}

void ClientProxy::addObjectToTree(const QmlDebugObjectReference &obj)
{
    int count = m_rootObjects.count();
    for (int i = 0; i < count; i++) {
        if (m_rootObjects[i].insertObjectInTree(obj)) {
            buildDebugIdHashRecursive(obj);
            emit objectTreeUpdated();
            break;
        }
    }
}

QmlDebugObjectReference ClientProxy::objectReferenceForId(int debugId) const
{
    foreach (const QmlDebugObjectReference& it, m_rootObjects) {
        QmlDebugObjectReference result = objectReferenceForId(debugId, it);
        if (result.debugId() == debugId)
            return result;
    }
    return QmlDebugObjectReference();
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

    if (m_adapter)
        m_adapter.data()->logServiceActivity("QDeclarativeDebug", msg);
}

QList<QmlDebugObjectReference>
QmlJSInspector::Internal::ClientProxy::rootObjectReference() const
{
    return m_rootObjects;
}

QmlDebugObjectReference
ClientProxy::objectReferenceForId(int debugId,
                                  const QmlDebugObjectReference &objectRef) const
{
    if (objectRef.debugId() == debugId)
        return objectRef;

    foreach (const QmlDebugObjectReference &child, objectRef.children()) {
        QmlDebugObjectReference result = objectReferenceForId(debugId, child);
        if (result.debugId() == debugId)
            return result;
    }

    return QmlDebugObjectReference();
}

QmlDebugObjectReference ClientProxy::objectReferenceForId(
        const QString &objectId) const
{
    if (!objectId.isEmpty() && objectId[0].isLower()) {
        const QList<QmlDebugObjectReference> refs = objectReferences();
        foreach (const QmlDebugObjectReference &ref, refs) {
            if (ref.idString() == objectId)
                return ref;
        }
    }
    return QmlDebugObjectReference();
}

QmlDebugObjectReference ClientProxy::objectReferenceForLocation(
        const int line, const int column) const
{
    const QList<QmlDebugObjectReference> refs = objectReferences();
    foreach (const QmlDebugObjectReference &ref, refs) {
        if (ref.source().lineNumber() == line && ref.source().columnNumber() == column)
            return ref;
    }

    return QmlDebugObjectReference();
}

QList<QmlDebugObjectReference> ClientProxy::objectReferences() const
{
    QList<QmlDebugObjectReference> result;
    foreach (const QmlDebugObjectReference &it, m_rootObjects) {
        result.append(objectReferences(it));
    }
    return result;
}

QList<QmlDebugObjectReference>
ClientProxy::objectReferences(const QmlDebugObjectReference &objectRef) const
{
    QList<QmlDebugObjectReference> result;
    result.append(objectRef);

    foreach (const QmlDebugObjectReference &child, objectRef.children()) {
        result.append(objectReferences(child));
    }

    return result;
}

quint32 ClientProxy::setBindingForObject(int objectDebugId,
                                      const QString &propertyName,
                                      const QVariant &value,
                                      bool isLiteralValue,
                                      QString source,
                                      int line)
{
    if (objectDebugId == -1)
        return false;

    if (propertyName == QLatin1String("id"))
        return false; // Crashes the QMLViewer.

    if (!isConnected())
        return false;

    log(LogSend, QString("SET_BINDING %1 %2 %3 %4").arg(
            QString::number(objectDebugId), propertyName, value.toString(),
            QString(isLiteralValue ? "true" : "false")));

    quint32 queryId = m_engineClient->setBindingForObject(
                objectDebugId, propertyName, value.toString(), isLiteralValue,
                source, line);

    if (!queryId)
        log(LogSend, QString("failed!"));

    return queryId;
}

quint32 ClientProxy::setMethodBodyForObject(int objectDebugId,
                                            const QString &methodName,
                                            const QString &methodBody)
{
    if (objectDebugId == -1)
        return false;

    if (!isConnected())
        return false;

    log(LogSend, QString("SET_METHOD_BODY %1 %2 %3").arg(
            QString::number(objectDebugId), methodName, methodBody));

    quint32 queryId = m_engineClient->setMethodBody(
                objectDebugId, methodName, methodBody);

    if (!queryId)
        log(LogSend, QString("failed!"));

    return queryId;
}

quint32 ClientProxy::resetBindingForObject(int objectDebugId,
                                           const QString& propertyName)
{
    if (objectDebugId == -1)
        return false;

    if (!isConnected())
        return false;

    log(LogSend, QString("RESET_BINDING %1 %2").arg(
            QString::number(objectDebugId), propertyName));

    quint32 queryId = m_engineClient->resetBindingForObject(
                objectDebugId, propertyName);

    if (!queryId)
        log(LogSend, QString("failed!"));

    return queryId;
}

quint32 ClientProxy::queryExpressionResult(int objectDebugId,
                                           const QString &expr)
{
    if (objectDebugId == -1)
        return 0;

    if (!isConnected())
        return 0;

    bool block = false;
    if (m_adapter)
        block = m_adapter.data()->disableJsDebugging(true);

    log(LogSend, QString("EVAL_EXPRESSION %1 %2").arg(
            QString::number(objectDebugId), expr));
    quint32 queryId = m_engineClient->queryExpressionResult(objectDebugId, expr);

    if (m_adapter)
        m_adapter.data()->disableJsDebugging(block);
    return queryId;
}

void ClientProxy::clearComponentCache()
{
    if (isConnected())
        m_inspectorClient->clearComponentCache();
}

bool ClientProxy::addObjectWatch(int objectDebugId)
{
    if (objectDebugId == -1)
        return false;

    if (!isConnected())
        return false;

    // already set
    if (m_objectWatches.contains(objectDebugId))
        return true;

    QmlDebugObjectReference ref = objectReferenceForId(objectDebugId);
    if (ref.debugId() != objectDebugId)
        return false;

    // is flooding the debugging output log!
    // log(LogSend, QString("WATCH_PROPERTY %1").arg(objectDebugId));

    if (m_engineClient->addWatch(ref))
        m_objectWatches.append(objectDebugId);

    return false;
}

bool ClientProxy::isObjectBeingWatched(int objectDebugId)
{
    return m_objectWatches.contains(objectDebugId);
}


void ClientProxy::objectWatchTriggered(int objectDebugId,
                                       const QByteArray &propertyName,
                                       const QVariant &propertyValue)
{
    if (m_objectWatches.contains(objectDebugId))
        emit propertyChanged(objectDebugId, propertyName, propertyValue);
}

bool ClientProxy::removeObjectWatch(int objectDebugId)
{
    if (objectDebugId == -1)
        return false;

    if (!m_objectWatches.contains(objectDebugId))
        return false;

    if (!isConnected())
        return false;

    m_objectWatches.removeOne(objectDebugId);
    return true;
}

void ClientProxy::removeAllObjectWatches()
{
    foreach (int watchedObject, m_objectWatches)
        removeObjectWatch(watchedObject);
}

void ClientProxy::queryEngineContext(int id)
{
    if (id < 0)
        return;

    if (!isConnected())
        return;

    if (m_contextQueryId)
        m_contextQueryId = 0;

    log(LogSend, QString("LIST_OBJECTS %1").arg(QString::number(id)));

    m_contextQueryId = m_engineClient->queryRootContexts(QmlDebugEngineReference(id));
}

void ClientProxy::contextChanged(const QVariant &value)
{

    if (m_contextQueryId) {
        m_contextQueryId = 0;
        emit rootContext(value);
    }
}

quint32 ClientProxy::fetchContextObject(const QmlDebugObjectReference& obj)
{
    log(LogSend, QString("FETCH_OBJECT %1").arg(obj.idString()));

    return m_engineClient->queryObject(obj);
}

void ClientProxy::fetchContextObjectRecursive(
        const QmlDebugContextReference& context, bool clear)
{
    if (!isConnected())
        return;
    if (clear) {
        m_rootObjects.clear();
        m_objectTreeQueryIds.clear();
    }
    foreach (const QmlDebugObjectReference & obj, context.objects()) {
        quint32 queryId = fetchContextObject(obj);
        if (queryId)
            m_objectTreeQueryIds << queryId;
    }
    foreach (const QmlDebugContextReference& child, context.contexts()) {
        fetchContextObjectRecursive(child, false);
    }
}

void ClientProxy::insertObjectInTreeIfNeeded(const QmlDebugObjectReference &object)
{
    if (!m_rootObjects.contains(object))
        return;
    int count = m_rootObjects.count();
    for (int i = 0; i < count; i++) {
        if (m_rootObjects[i].parentId() < 0 && m_rootObjects[i].insertObjectInTree(object)) {
            m_rootObjects.removeOne(object);
            break;
        }
    }
}

void ClientProxy::onResult(quint32 queryId, const QVariant &value, const QByteArray &type)
{
    if (type == "FETCH_OBJECT_R") {
        log(LogReceive, QString("FETCH_OBJECT_R %1").arg(
                qvariant_cast<QmlDebugObjectReference>(value).idString()));
    } else {
        log(LogReceive, QLatin1String(type));
    }

    if (m_objectTreeQueryIds.contains(queryId))
        objectTreeFetched(queryId, value);
    else if (queryId == m_engineQueryId)
        updateEngineList(value);
    else if (queryId == m_contextQueryId)
        contextChanged(value);
    else if (m_fetchCurrentObjectsQueryIds.contains(queryId))
        onCurrentObjectsFetched(queryId, value);
    else
        emit result(queryId, value);
}

void ClientProxy::objectTreeFetched(quint32 queryId, const QVariant &result)
{
    QmlDebugObjectReference obj = qvariant_cast<QmlDebugObjectReference>(result);
    m_rootObjects.append(obj);

    m_objectTreeQueryIds.removeOne(queryId);
    if (m_objectTreeQueryIds.isEmpty()) {
        int old_count = m_debugIdHash.count();
        m_debugIdHash.clear();
        m_debugIdHash.reserve(old_count + 1);
        foreach (const QmlDebugObjectReference &it, m_rootObjects)
            buildDebugIdHashRecursive(it);
        emit objectTreeUpdated();

        if (isConnected()) {
            if (!m_inspectorClient->currentObjects().isEmpty())
                onCurrentObjectsChanged(m_inspectorClient->currentObjects(), false);

            m_inspectorClient->setObjectIdList(m_rootObjects);
        }
    }
}

void ClientProxy::buildDebugIdHashRecursive(const QmlDebugObjectReference& ref)
{
    QUrl fileUrl = ref.source().url();
    int lineNum = ref.source().lineNumber();
    int colNum = ref.source().columnNumber();
    int rev = 0;

    // handle the case where the url contains the revision number encoded.
    //(for object created by the debugger)
    static QRegExp rx("(.*)_(\\d+):(\\d+)$");
    if (rx.exactMatch(fileUrl.path())) {
        fileUrl.setPath(rx.cap(1));
        rev = rx.cap(2).toInt();
        lineNum += rx.cap(3).toInt() - 1;
    }

    const QString filePath = InspectorUi::instance()->findFileInProject(fileUrl);

    // append the debug ids in the hash
    m_debugIdHash[qMakePair<QString, int>(filePath, rev)][qMakePair<int, int>(
                lineNum, colNum)].append(ref.debugId());

    foreach (const QmlDebugObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}


void ClientProxy::reloadQmlViewer()
{
    if (isConnected())
        m_inspectorClient->reloadViewer();
}

void ClientProxy::setDesignModeBehavior(bool inDesignMode)
{
    if (isConnected())
        m_inspectorClient->setDesignModeBehavior(inDesignMode);
}

void ClientProxy::setAnimationSpeed(qreal slowDownFactor)
{
    if (isConnected())
        m_inspectorClient->setAnimationSpeed(slowDownFactor);
}

void ClientProxy::setAnimationPaused(bool paused)
{
    if (isConnected())
        m_inspectorClient->setAnimationPaused(paused);
}

void ClientProxy::changeToZoomTool()
{
    if (isConnected())
        m_inspectorClient->changeToZoomTool();
}
void ClientProxy::changeToSelectTool()
{
    if (isConnected())
        m_inspectorClient->changeToSelectTool();
}

void ClientProxy::changeToSelectMarqueeTool()
{
    if (isConnected())
        m_inspectorClient->changeToSelectMarqueeTool();
}

void ClientProxy::showAppOnTop(bool showOnTop)
{
    if (isConnected())
        m_inspectorClient->showAppOnTop(showOnTop);
}

void ClientProxy::createQmlObject(const QString &qmlText, int parentDebugId,
                                  const QStringList &imports,
                                  const QString &filename, int order)
{
    if (isConnected())
        m_inspectorClient->createQmlObject(qmlText, parentDebugId, imports,
                                           filename, order);
}

void ClientProxy::destroyQmlObject(int debugId)
{
    if (isConnected())
        m_inspectorClient->destroyQmlObject(debugId);
}

void ClientProxy::reparentQmlObject(int debugId, int newParent)
{
    if (isConnected())
        m_inspectorClient->reparentQmlObject(debugId, newParent);
}

void ClientProxy::updateConnected()
{
    bool isConnected = m_inspectorClient &&
            m_inspectorClient->status() == QDeclarativeDebugClient::Enabled &&
            m_engineClient &&
            m_engineClient->status() == QDeclarativeDebugClient::Enabled;

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
    if (!isConnected())
        return;

    emit aboutToReloadEngines();

    log(LogSend, QString("LIST_ENGINES"));

    m_engineQueryId = m_engineClient->queryAvailableEngines();
}

QList<QmlDebugEngineReference> ClientProxy::engines() const
{
    return m_engines;
}

void ClientProxy::updateEngineList(const QVariant &value)
{
    m_engines = qvariant_cast<QmlDebugEngineReferenceList>(value);
    m_engineQueryId = 0;
    emit enginesChanged();
}

Debugger::QmlAdapter *ClientProxy::qmlAdapter() const
{
    return m_adapter.data();
}

bool ClientProxy::isConnected() const
{
    return m_isConnected;
}

void ClientProxy::newObjects()
{
    log(LogReceive, QString("OBJECT_CREATED"));
    refreshObjectTree();
}
