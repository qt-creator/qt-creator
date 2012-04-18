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
#include "qmltoolsclient.h"
#include "qmljsinspector.h"

#include <qmldebug/qmldebugconstants.h>
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
    , m_inspectorHelperClient(0)
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
    DeclarativeEngineDebugCLient *client1 = new DeclarativeEngineDebugCLient(
                m_adapter.data()->connection());
    QmlEngineDebugClient *client2 = new QmlEngineDebugClient(
                m_adapter.data()->connection());

    connect(client1, SIGNAL(newStatus(QmlDebugClient::Status)),
            SLOT(clientStatusChanged(QmlDebugClient::Status)));
    connect(client1, SIGNAL(newStatus(QmlDebugClient::Status)),
            SLOT(engineClientStatusChanged(QmlDebugClient::Status)));

    connect(client2, SIGNAL(newStatus(QmlDebugClient::Status)),
            SLOT(clientStatusChanged(QmlDebugClient::Status)));
    connect(client2, SIGNAL(newStatus(QmlDebugClient::Status)),
            SLOT(engineClientStatusChanged(QmlDebugClient::Status)));

    m_inspectorHelperClient =
            new QmlToolsClient(m_adapter.data()->connection(), this);

    connect(m_inspectorHelperClient,
            SIGNAL(connectedStatusChanged(QmlDebugClient::Status)),
            this, SLOT(clientStatusChanged(QmlDebugClient::Status)));
    connect(m_inspectorHelperClient, SIGNAL(currentObjectsChanged(QList<int>)),
            SLOT(onCurrentObjectsChanged(QList<int>)));
    connect(m_inspectorHelperClient, SIGNAL(zoomToolActivated()),
            SIGNAL(zoomToolActivated()));
    connect(m_inspectorHelperClient, SIGNAL(selectToolActivated()),
            SIGNAL(selectToolActivated()));
    connect(m_inspectorHelperClient, SIGNAL(selectMarqueeToolActivated()),
            SIGNAL(selectMarqueeToolActivated()));
    connect(m_inspectorHelperClient, SIGNAL(animationSpeedChanged(qreal)),
            SIGNAL(animationSpeedChanged(qreal)));
    connect(m_inspectorHelperClient, SIGNAL(animationPausedChanged(bool)),
            SIGNAL(animationPausedChanged(bool)));
    connect(m_inspectorHelperClient, SIGNAL(designModeBehaviorChanged(bool)),
            SIGNAL(designModeBehaviorChanged(bool)));
    connect(m_inspectorHelperClient, SIGNAL(showAppOnTopChanged(bool)),
            SIGNAL(showAppOnTopChanged(bool)));
    connect(m_inspectorHelperClient, SIGNAL(reloaded()), this,
            SIGNAL(serverReloaded()));
    connect(m_inspectorHelperClient, SIGNAL(logActivity(QString,QString)),
            m_adapter.data(), SLOT(logServiceActivity(QString,QString)));

    updateConnected();
}

void ClientProxy::clientStatusChanged(QmlDebugClient::Status status)
{
    QString serviceName;
    float version = 0;
    if (QmlDebugClient *client
            = qobject_cast<QmlDebugClient*>(sender())) {
        serviceName = client->name();
        version = client->serviceVersion();
    }

    if (m_adapter)
        m_adapter.data()->logServiceStatusChange(serviceName, version, status);

    updateConnected();
}

QmlDebugClient *ClientProxy::inspectorClient() const
{
    return m_engineClient;
}

void ClientProxy::engineClientStatusChanged(
        QmlDebugClient::Status status)
{
    if (status == QmlDebugClient::Enabled) {
        m_engineClient = qobject_cast<BaseEngineDebugClient*>(sender());
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
            m_fetchCurrentObjectsQueryIds
                    << fetchContextObject(QmlDebugObjectReference(debugId));

        }
    }

    emit selectedItemsChanged(selectedItems);
}

void ClientProxy::onCurrentObjectsFetched(quint32 queryId,
                                          const QVariant &result)
{
    m_fetchCurrentObjectsQueryIds.removeOne(queryId);
    QmlDebugObjectReference obj
            = qvariant_cast<QmlDebugObjectReference>(result);
    m_fetchCurrentObjects.push_front(obj);

    if (!getObjectHierarchy(obj))
        return;

    foreach (const QmlDebugObjectReference &o, m_fetchCurrentObjects)
        addObjectToTree(o, false);
    emit selectedItemsChanged(QList<QmlDebugObjectReference>() <<
                              m_fetchCurrentObjects.last());
}

bool ClientProxy::getObjectHierarchy(const QmlDebugObjectReference &obj)
{
    QmlDebugObjectReference parent = objectReferenceForId(obj.parentId());
    //for root object
    if (obj.parentId() == -1)
        return true;

    //for other objects
    if (parent.debugId() == -1 || parent.needsMoreData()) {
        m_fetchCurrentObjectsQueryIds
                << fetchContextObject(QmlDebugObjectReference(obj.parentId()));
    } else {
        return getObjectHierarchy(parent);
    }
    return false;
}

void ClientProxy::setSelectedItemsByDebugId(const QList<int> &debugIds)
{
    if (!isConnected())
        return;

    m_inspectorHelperClient->setCurrentObjects(debugIds);
}

void ClientProxy::setSelectedItemsByObjectId(
        const QList<QmlDebugObjectReference> &objectRefs)
{
    if (isConnected()) {
        QList<int> debugIds;

        foreach (const QmlDebugObjectReference &ref, objectRefs) {
            debugIds << ref.debugId();
        }

        m_inspectorHelperClient->setCurrentObjects(debugIds);
    }
}

void ClientProxy::addObjectToTree(const QmlDebugObjectReference &obj,
                                  bool notify)
{
    int count = m_rootObjects.count();
    for (int i = 0; i < count; i++) {
        if (m_rootObjects[i].insertObjectInTree(obj)) {
            buildDebugIdHashRecursive(obj);
            if (notify)
                emit objectTreeUpdated();
            break;
        }
    }
}

QmlDebugObjectReference ClientProxy::objectReferenceForId(int debugId) const
{
    foreach (const QmlDebugObjectReference &it, m_rootObjects) {
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
        m_adapter.data()->logServiceActivity("QmlDebug", msg);
}

QList<QmlDebugObjectReference>
QmlJSInspector::Internal::ClientProxy::rootObjectReference() const
{
    return m_rootObjects;
}

QmlDebugObjectReference
ClientProxy::objectReferenceForId(
        int debugId,
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
        int line, int column) const
{
    const QList<QmlDebugObjectReference> refs = objectReferences();
    foreach (const QmlDebugObjectReference &ref, refs) {
        if (ref.source().lineNumber() == line
                && ref.source().columnNumber() == column)
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
                                           const QString &propertyName)
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
    quint32 queryId
            = m_engineClient->queryExpressionResult(objectDebugId, expr);

    if (m_adapter)
        m_adapter.data()->disableJsDebugging(block);
    return queryId;
}

void ClientProxy::clearComponentCache()
{
    if (isConnected())
        m_inspectorHelperClient->clearComponentCache();
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

    m_contextQueryId
            = m_engineClient->queryRootContexts(QmlDebugEngineReference(id));
}

void ClientProxy::contextChanged(const QVariant &value)
{

    if (m_contextQueryId) {
        m_contextQueryId = 0;
        emit rootContext(value);
    }
}

quint32 ClientProxy::fetchContextObject(const QmlDebugObjectReference &obj)
{
    if (!isConnected())
        return 0;

    log(LogSend, QString("FETCH_OBJECT %1").arg(obj.idString()));
    return m_engineClient->queryObject(obj);
}

void ClientProxy::fetchRootObjects(
        const QmlDebugContextReference &context, bool clear)
{
    if (!isConnected())
        return;
    if (clear) {
        m_rootObjects.clear();
        m_objectTreeQueryIds.clear();
    }
    foreach (const QmlDebugObjectReference & obj, context.objects()) {
        quint32 queryId = 0;
        using namespace QmlDebug::Constants;
        if (m_engineClient->objectName() == QML_DEBUGGER &&
                m_engineClient->serviceVersion() >= CURRENT_SUPPORTED_VERSION) {
            //Fetch only root objects
            if (obj.parentId() == -1)
                queryId = fetchContextObject(obj);
        } else {
            queryId = m_engineClient->queryObjectRecursive(obj);
        }

        if (queryId)
            m_objectTreeQueryIds << queryId;
    }
    foreach (const QmlDebugContextReference &child, context.contexts()) {
        fetchRootObjects(child, false);
    }
}

void ClientProxy::insertObjectInTreeIfNeeded(
        const QmlDebugObjectReference &object)
{
    if (!m_rootObjects.contains(object))
        return;
    int count = m_rootObjects.count();
    for (int i = 0; i < count; i++) {
        if (m_rootObjects[i].parentId() < 0
                && m_rootObjects[i].insertObjectInTree(object)) {
            m_rootObjects.removeOne(object);
            break;
        }
    }
}

void ClientProxy::onResult(quint32 queryId, const QVariant &value,
                           const QByteArray &type)
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
    QmlDebugObjectReference obj
            = qvariant_cast<QmlDebugObjectReference>(result);
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
            if (!m_inspectorHelperClient->currentObjects().isEmpty())
                onCurrentObjectsChanged(m_inspectorHelperClient->currentObjects(),
                                        false);

            m_inspectorHelperClient->setObjectIdList(m_rootObjects);
        }
    }
}

void ClientProxy::buildDebugIdHashRecursive(const QmlDebugObjectReference &ref)
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

    const QString filePath
            = InspectorUi::instance()->findFileInProject(fileUrl);

    // append the debug ids in the hash
    m_debugIdHash[qMakePair<QString, int>(filePath, rev)][qMakePair<int, int>(
                lineNum, colNum)].append(ref.debugId());

    foreach (const QmlDebugObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}


void ClientProxy::reloadQmlViewer()
{
    if (isConnected())
        m_inspectorHelperClient->reloadViewer();
}

void ClientProxy::setDesignModeBehavior(bool inDesignMode)
{
    if (isConnected())
        m_inspectorHelperClient->setDesignModeBehavior(inDesignMode);
}

void ClientProxy::setAnimationSpeed(qreal slowDownFactor)
{
    if (isConnected())
        m_inspectorHelperClient->setAnimationSpeed(slowDownFactor);
}

void ClientProxy::setAnimationPaused(bool paused)
{
    if (isConnected())
        m_inspectorHelperClient->setAnimationPaused(paused);
}

void ClientProxy::changeToZoomTool()
{
    if (isConnected())
        m_inspectorHelperClient->changeToZoomTool();
}
void ClientProxy::changeToSelectTool()
{
    if (isConnected())
        m_inspectorHelperClient->changeToSelectTool();
}

void ClientProxy::changeToSelectMarqueeTool()
{
    if (isConnected())
        m_inspectorHelperClient->changeToSelectMarqueeTool();
}

void ClientProxy::showAppOnTop(bool showOnTop)
{
    if (isConnected())
        m_inspectorHelperClient->showAppOnTop(showOnTop);
}

void ClientProxy::createQmlObject(const QString &qmlText, int parentDebugId,
                                  const QStringList &imports,
                                  const QString &filename, int order)
{
    if (isConnected())
        m_inspectorHelperClient->createQmlObject(qmlText, parentDebugId, imports,
                                           filename, order);
}

void ClientProxy::destroyQmlObject(int debugId)
{
    if (isConnected())
        m_inspectorHelperClient->destroyQmlObject(debugId);
}

void ClientProxy::reparentQmlObject(int debugId, int newParent)
{
    if (isConnected())
        m_inspectorHelperClient->reparentQmlObject(debugId, newParent);
}

void ClientProxy::updateConnected()
{
    bool isConnected = m_inspectorHelperClient &&
            m_inspectorHelperClient->status() == QmlDebugClient::Enabled &&
            m_engineClient &&
            m_engineClient->status() == QmlDebugClient::Enabled;

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
