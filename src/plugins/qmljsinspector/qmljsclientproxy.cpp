/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
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

#include <QtCore/QUrl>
#include <QtNetwork/QAbstractSocket>

using namespace QmlJSInspector::Internal;

ClientProxy::ClientProxy(Debugger::QmlAdapter *adapter, QObject *parent)
    : QObject(parent)
    , m_adapter(adapter)
    , m_engineClient(0)
    , m_inspectorClient(0)
    , m_engineQuery(0)
    , m_contextQuery(0)
    , m_isConnected(false)
{
    m_requestObjectsTimer.setSingleShot(true);
    m_requestObjectsTimer.setInterval(3000);
    connect(&m_requestObjectsTimer, SIGNAL(timeout()), this, SLOT(refreshObjectTree()));
    connectToServer();
}

void ClientProxy::connectToServer()
{
    m_engineClient = new QDeclarativeEngineDebug(m_adapter.data()->connection(), this);

    connect(m_engineClient, SIGNAL(newObjects()), this, SLOT(newObjects()));

    m_inspectorClient = new QmlJSInspectorClient(m_adapter.data()->connection(), this);

    connect(m_inspectorClient, SIGNAL(connectedStatusChanged(QDeclarativeDebugClient::Status)),
             this, SLOT(clientStatusChanged(QDeclarativeDebugClient::Status)));
    connect(m_inspectorClient, SIGNAL(currentObjectsChanged(QList<int>)),
        SLOT(onCurrentObjectsChanged(QList<int>)));
    connect(m_inspectorClient, SIGNAL(colorPickerActivated()),
        SIGNAL(colorPickerActivated()));
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
    connect(m_inspectorClient, SIGNAL(selectedColorChanged(QColor)),
        SIGNAL(selectedColorChanged(QColor)));
    connect(m_inspectorClient, SIGNAL(logActivity(QString,QString)),
            m_adapter.data(), SLOT(logServiceActivity(QString,QString)));

    updateConnected();
}

void ClientProxy::clientStatusChanged(QDeclarativeDebugClient::Status status)
{
    QString serviceName;
    if (QDeclarativeDebugClient *client = qobject_cast<QDeclarativeDebugClient*>(sender())) {
        serviceName = client->name();
    }

    if (m_adapter)
        m_adapter.data()->logServiceStatusChange(serviceName, status);

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

void ClientProxy::setSelectedItemsByDebugId(const QList<int> &debugIds)
{
    if (!isConnected())
        return;

    m_inspectorClient->setCurrentObjects(debugIds);
}

void ClientProxy::setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objectRefs)
{
    if (isConnected()) {
        QList<int> debugIds;

        foreach (const QDeclarativeDebugObjectReference &ref, objectRefs) {
            debugIds << ref.debugId();
        }

        m_inspectorClient->setCurrentObjects(debugIds);
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

    if (m_adapter)
        m_adapter.data()->logServiceActivity("QDeclarativeDebug", msg);
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

    log(LogSend, QString("SET_BINDING %1 %2 %3 %4").arg(QString::number(objectDebugId), propertyName, value.toString(), QString(isLiteralValue ? "true" : "false")));

    bool result = m_engineClient->setBindingForObject(objectDebugId, propertyName, value.toString(), isLiteralValue, source, line);

    if (!result)
        log(LogSend, QString("failed!"));

    return result;
}

bool ClientProxy::setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody)
{
    if (objectDebugId == -1)
        return false;

    if (!isConnected())
        return false;

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

    if (!isConnected())
        return false;

    log(LogSend, QString("RESET_BINDING %1 %2").arg(QString::number(objectDebugId), propertyName));

    bool result = m_engineClient->resetBindingForObject(objectDebugId, propertyName);

    if (!result)
        log(LogSend, QString("failed!"));

    return result;
}

QDeclarativeDebugExpressionQuery *ClientProxy::queryExpressionResult(int objectDebugId, const QString &expr)
{
    if (objectDebugId == -1)
        return 0;

    if (!isConnected())
        return 0;

    bool block = false;
    if (m_adapter)
        block = m_adapter.data()->disableJsDebugging(true);

    log(LogSend, QString("EVAL_EXPRESSION %1 %2").arg(QString::number(objectDebugId), expr));
    QDeclarativeDebugExpressionQuery *query
            = m_engineClient->queryExpressionResult(objectDebugId, expr, m_engineClient);

    if (m_adapter)
        m_adapter.data()->disableJsDebugging(block);
    return query;
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
    if (m_objectWatches.keys().contains(objectDebugId))
        return true;

    QDeclarativeDebugObjectReference ref = objectReferenceForId(objectDebugId);
    if (ref.debugId() != objectDebugId)
        return false;

    // is flooding the debugging output log!
    // log(LogSend, QString("WATCH_PROPERTY %1").arg(objectDebugId));

    QDeclarativeDebugWatch *watch = m_engineClient->addWatch(ref, m_engineClient);
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

    if (!isConnected())
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
}

void ClientProxy::queryEngineContext(int id)
{
    if (id < 0)
        return;

    if (!isConnected())
        return;

    if (m_contextQuery) {
        delete m_contextQuery;
        m_contextQuery = 0;
    }

    log(LogSend, QString("LIST_OBJECTS %1").arg(QString::number(id)));

    m_contextQuery = m_engineClient->queryRootContexts(QDeclarativeDebugEngineReference(id),
                                                       m_engineClient);
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
    if (!isConnected())
        return;

    foreach (const QDeclarativeDebugObjectReference & obj, context.objects()) {

        log(LogSend, QString("FETCH_OBJECT %1").arg(obj.idString()));

        QDeclarativeDebugObjectQuery* query
                = m_engineClient->queryObjectRecursive(obj, m_engineClient);
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
            if (!m_inspectorClient->currentObjects().isEmpty())
                onCurrentObjectsChanged(m_inspectorClient->currentObjects(), false);

            m_inspectorClient->setObjectIdList(m_rootObjects);
        }
    }
}

void ClientProxy::buildDebugIdHashRecursive(const QDeclarativeDebugObjectReference& ref)
{
    QUrl fileUrl = ref.source().url();
    int lineNum = ref.source().lineNumber();
    int colNum = ref.source().columnNumber();
    int rev = 0;

    // handle the case where the url contains the revision number encoded. (for object created by the debugger)
    static QRegExp rx("(.*)_(\\d+):(\\d+)$");
    if (rx.exactMatch(fileUrl.path())) {
        fileUrl.setPath(rx.cap(1));
        rev = rx.cap(2).toInt();
        lineNum += rx.cap(3).toInt() - 1;
    }

    const QString filePath = InspectorUi::instance()->findFileInProject(fileUrl);

    // append the debug ids in the hash
    m_debugIdHash[qMakePair<QString, int>(filePath, rev)][qMakePair<int, int>(lineNum, colNum)].append(ref.debugId());

    foreach (const QDeclarativeDebugObjectReference &it, ref.children())
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

void ClientProxy::changeToColorPickerTool()
{
    if (isConnected())
        m_inspectorClient->changeToColorPickerTool();
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
                                  const QStringList &imports, const QString &filename, int order)
{
    if (isConnected())
        m_inspectorClient->createQmlObject(qmlText, parentDebugId, imports, filename, order);
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
    bool isConnected = m_inspectorClient && m_inspectorClient->status() == QDeclarativeDebugClient::Enabled
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

    if (!isConnected())
        return;

    emit aboutToReloadEngines();

    log(LogSend, QString("LIST_ENGINES"));

    m_engineQuery = m_engineClient->queryAvailableEngines(m_engineClient);
    if (!m_engineQuery->isWaiting()) {
        updateEngineList();
    } else {
        connect(m_engineQuery, SIGNAL(stateChanged(QDeclarativeDebugQuery::State)),
                         this, SLOT(updateEngineList()));
    }
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
    return m_adapter.data();
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
