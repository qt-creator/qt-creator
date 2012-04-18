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

#include "qmlinspectoragent.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerstringutils.h"
#include "watchhandler.h"

#include <qmldebug/qmldebugconstants.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

namespace Debugger {
namespace Internal {

enum {
    debug = false
};

/*!
 * DebuggerAgent updates the watchhandler with the object tree data.
 */
QmlInspectorAgent::QmlInspectorAgent(DebuggerEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_engineClient(0)
    , m_engineQueryId(0)
    , m_rootContextQueryId(0)
    , m_objectToSelect(-1)
{
    connect(debuggerCore()->action(ShowQmlObjectTree),
            SIGNAL(valueChanged(QVariant)), SLOT(updateStatus()));
}

void QmlInspectorAgent::refreshObjectTree()
{
    if (debug)
        qDebug() << __FUNCTION__ << "()";

    if (!m_rootContextQueryId) {
        m_objectTreeQueryIds.clear();
        queryEngineContext(m_engines.value(0).debugId());
    }
}

void QmlInspectorAgent::fetchObject(int debugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << debugId << ")";

    m_fetchCurrentObjectsQueryIds
            << fetchContextObject(QmlDebugObjectReference(debugId));
}

quint32 QmlInspectorAgent::queryExpressionResult(int debugId,
                                                 const QString &expression)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << debugId << expression
                 << m_engines.value(0).debugId() << ")";

    return m_engineClient->queryExpressionResult(debugId, expression,
                                                 m_engines.value(0).debugId());
}

void QmlInspectorAgent::updateWatchData(const WatchData &data)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << data.id << ")";

    if (data.id) {
        // objects
        QmlDebugObjectReference ref(data.id);
        m_fetchCurrentObjectsQueryIds << fetchContextObject(ref);
        WatchData d = data;
        d.setAllUnneeded();
        m_engine->watchHandler()->beginCycle(InspectWatch, false);
        m_engine->watchHandler()->insertData(d);
        m_engine->watchHandler()->endCycle(InspectWatch);
    }
}

void QmlInspectorAgent::selectObjectInTree(int debugId)
{
    if (debug) {
        qDebug() << __FUNCTION__ << "(" << debugId << ")";
        qDebug() << "  " << debugId << "already fetched? "
                 << m_debugIdToIname.contains(debugId);
    }

    if (m_debugIdToIname.contains(debugId)) {
        QByteArray iname = m_debugIdToIname.value(debugId);
        QTC_ASSERT(iname.startsWith("inspect."), qDebug() << iname);
        QModelIndex itemIndex = m_engine->watchHandler()->itemIndex(iname);
        QTC_ASSERT(itemIndex.isValid(),
                   qDebug() << "No  for " << debugId << ", iname " << iname; return;);
        if (debug)
            qDebug() << "  selecting" << iname << "in tree";
        m_engine->watchHandler()->setCurrentModelIndex(InspectWatch, itemIndex);
        m_objectToSelect = 0;
    } else {
        // we've to fetch it
        m_objectToSelect = debugId;
        m_fetchCurrentObjectsQueryIds
                << fetchContextObject(QmlDebugObjectReference(debugId));
    }
}

quint32 QmlInspectorAgent::setBindingForObject(int objectDebugId,
                                         const QString &propertyName,
                                         const QVariant &value,
                                         bool isLiteralValue,
                                         QString source,
                                         int line)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << objectDebugId << propertyName
                 << value.toString() << isLiteralValue << source << line << ")";

    if (objectDebugId == -1)
        return 0;

    if (propertyName == QLatin1String("id"))
        return 0; // Crashes the QMLViewer.

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString("SET_BINDING %1 %2 %3 %4").arg(
            QString::number(objectDebugId), propertyName, value.toString(),
            QString(isLiteralValue ? "true" : "false")));

    quint32 queryId = m_engineClient->setBindingForObject(
                objectDebugId, propertyName, value.toString(), isLiteralValue,
                source, line);

    if (!queryId)
        log(LogSend, QString("SET_BINDING failed!"));

    return queryId;
}

quint32 QmlInspectorAgent::setMethodBodyForObject(int objectDebugId,
                                            const QString &methodName,
                                            const QString &methodBody)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << objectDebugId
                 << methodName << methodBody << ")";

    if (objectDebugId == -1)
        return 0;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString("SET_METHOD_BODY %1 %2 %3").arg(
            QString::number(objectDebugId), methodName, methodBody));

    quint32 queryId = m_engineClient->setMethodBody(
                objectDebugId, methodName, methodBody);

    if (!queryId)
        log(LogSend, QString("failed!"));

    return queryId;
}

quint32 QmlInspectorAgent::resetBindingForObject(int objectDebugId,
                                           const QString &propertyName)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << objectDebugId
                 << propertyName << ")";

    if (objectDebugId == -1)
        return 0;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString("RESET_BINDING %1 %2").arg(
            QString::number(objectDebugId), propertyName));

    quint32 queryId = m_engineClient->resetBindingForObject(
                objectDebugId, propertyName);

    if (!queryId)
        log(LogSend, QString("failed!"));

    return queryId;
}


QList<QmlDebugObjectReference> QmlInspectorAgent::objects() const
{
    QList<QmlDebugObjectReference> result;
    foreach (const QmlDebugObjectReference &it, m_rootObjects)
        result.append(objects(it));
    return result;
}

QmlDebugObjectReference QmlInspectorAgent::objectForId(int debugId) const
{
    foreach (const QmlDebugObjectReference &it, m_rootObjects) {
        QmlDebugObjectReference result = objectForId(debugId, it);
        if (result.debugId() == debugId)
            return result;
    }
    return QmlDebugObjectReference();
}

QmlDebugObjectReference QmlInspectorAgent::objectForId(
        const QString &objectId) const
{
    if (!objectId.isEmpty() && objectId[0].isLower()) {
        const QList<QmlDebugObjectReference> refs = objects();
        foreach (const QmlDebugObjectReference &ref, refs) {
            if (ref.idString() == objectId)
                return ref;
        }
    }
    return QmlDebugObjectReference();
}

QmlDebugObjectReference QmlInspectorAgent::objectForLocation(
        int line, int column) const
{
    const QList<QmlDebugObjectReference> refs = objects();
    foreach (const QmlDebugObjectReference &ref, refs) {
        if (ref.source().lineNumber() == line
                && ref.source().columnNumber() == column)
            return ref;
    }

    return QmlDebugObjectReference();
}

bool QmlInspectorAgent::addObjectWatch(int objectDebugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << objectDebugId << ")";

    if (objectDebugId == -1)
        return false;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return false;

    // already set
    if (m_objectWatches.contains(objectDebugId))
        return true;

    QmlDebugObjectReference ref = objectForId(objectDebugId);
    if (ref.debugId() != objectDebugId)
        return false;

    // is flooding the debugging output log!
    // log(LogSend, QString("WATCH_PROPERTY %1").arg(objectDebugId));

    if (m_engineClient->addWatch(ref))
        m_objectWatches.append(objectDebugId);

    return false;
}

bool QmlInspectorAgent::isObjectBeingWatched(int objectDebugId)
{
    return m_objectWatches.contains(objectDebugId);
}

bool QmlInspectorAgent::removeObjectWatch(int objectDebugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << objectDebugId << ")";

    if (objectDebugId == -1)
        return false;

    if (!m_objectWatches.contains(objectDebugId))
        return false;

    if (!isConnected())
        return false;

    m_objectWatches.removeOne(objectDebugId);
    return true;
}

void QmlInspectorAgent::removeAllObjectWatches()
{
    if (debug)
        qDebug() << __FUNCTION__ << "()";

    foreach (int watchedObject, m_objectWatches)
        removeObjectWatch(watchedObject);
}

void QmlInspectorAgent::setEngineClient(BaseEngineDebugClient *client)
{
    if (m_engineClient == client)
        return;

    if (m_engineClient) {
        disconnect(m_engineClient, SIGNAL(newStatus(QmlDebugClient::Status)),
                   this, SLOT(updateStatus()));
        disconnect(m_engineClient, SIGNAL(result(quint32,QVariant,QByteArray)),
                   this, SLOT(onResult(quint32,QVariant,QByteArray)));
        disconnect(m_engineClient, SIGNAL(newObjects()),
                   this, SLOT(newObjects()));
    }

    m_engineClient = client;

    if (m_engineClient) {
        connect(m_engineClient, SIGNAL(newStatus(QmlDebugClient::Status)),
                this, SLOT(updateStatus()));
        connect(m_engineClient, SIGNAL(result(quint32,QVariant,QByteArray)),
                this, SLOT(onResult(quint32,QVariant,QByteArray)));
        connect(m_engineClient, SIGNAL(newObjects()),
                this, SLOT(newObjects()));
    }

    updateStatus();
}

void QmlInspectorAgent::updateStatus()
{
    if (m_engineClient
            && (m_engineClient->status() == QmlDebugClient::Enabled)
            && debuggerCore()->boolSetting(ShowQmlObjectTree)) {
        reloadEngines();
    } else {
        // clear view
        m_engine->watchHandler()->beginCycle(InspectWatch, true);
        m_engine->watchHandler()->endCycle(InspectWatch);
    }
}

void QmlInspectorAgent::onResult(quint32 queryId, const QVariant &value,
                                 const QByteArray &type)
{
    if (debug)
        qDebug() << __FUNCTION__ << "() ...";

    if (type == _("FETCH_OBJECT_R")) {
        log(LogReceive, _("FETCH_OBJECT_R %1").arg(
                qvariant_cast<QmlDebugObjectReference>(value).idString()));
    } else {
        log(LogReceive, QLatin1String(type));
    }

    if (m_objectTreeQueryIds.contains(queryId)) {
        m_objectTreeQueryIds.removeOne(queryId);
        objectTreeFetched(qvariant_cast<QmlDebugObjectReference>(value));
    } else if (queryId == m_engineQueryId) {
        m_engineQueryId = 0;
        updateEngineList(qvariant_cast<QmlDebugEngineReferenceList>(value));
    } else if (queryId == m_rootContextQueryId) {
        m_rootContextQueryId = 0;
        rootContextChanged(qvariant_cast<QmlDebugContextReference>(value));
    } else if (m_fetchCurrentObjectsQueryIds.contains(queryId)) {
        m_fetchCurrentObjectsQueryIds.removeOne(queryId);
        QmlDebugObjectReference obj
                = qvariant_cast<QmlDebugObjectReference>(value);
        m_fetchCurrentObjects.push_front(obj);
        onCurrentObjectsFetched(obj);
    } else {
        emit expressionResult(queryId, value);
    }

    if (debug)
        qDebug() << __FUNCTION__ << "done";

}

void QmlInspectorAgent::newObjects()
{
    if (debug)
        qDebug() << __FUNCTION__ << "()";

    log(LogReceive, QString("OBJECT_CREATED"));
    refreshObjectTree();
}

void QmlInspectorAgent::reloadEngines()
{
    if (debug)
        qDebug() << __FUNCTION__ << "()";

    if (!isConnected())
        return;

    log(LogSend, _("LIST_ENGINES"));

    m_engineQueryId = m_engineClient->queryAvailableEngines();
}

void QmlInspectorAgent::queryEngineContext(int id)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << id << ")";

    if (id < 0)
        return;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QString("LIST_OBJECTS %1").arg(QString::number(id)));

    m_rootContextQueryId
            = m_engineClient->queryRootContexts(QmlDebugEngineReference(id));
}

quint32 QmlInspectorAgent::fetchContextObject(const QmlDebugObjectReference &obj)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << obj << ")";

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString("FETCH_OBJECT %1").arg(obj.idString()));
    quint32 queryId = m_engineClient->queryObject(obj);
    if (debug)
        qDebug() << __FUNCTION__ << "(" << obj.debugId() << ")"
                 << " - query id" << queryId;
    return queryId;
}

// fetch the root objects from the context + any child contexts
void QmlInspectorAgent::fetchRootObjects(const QmlDebugContextReference &context,
                                         bool clear)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << context << clear << ")";

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
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
    foreach (const QmlDebugContextReference &child, context.contexts())
        fetchRootObjects(child, false);
}

void QmlInspectorAgent::updateEngineList(const QmlDebugEngineReferenceList &engines)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << engines << ")";

    m_engines = engines;

    // only care about first engine atm
    queryEngineContext(engines.first().debugId());
}

void QmlInspectorAgent::rootContextChanged(const QmlDebugContextReference &context)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << context << ")";

    fetchRootObjects(context, true);
}

void QmlInspectorAgent::objectTreeFetched(const QmlDebugObjectReference &object)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << object << ")";

    m_rootObjects.append(object);

    if (m_objectTreeQueryIds.isEmpty()) {
        int old_count = m_debugIdHash.count();
        m_debugIdHash.clear();
        m_debugIdHash.reserve(old_count + 1);
        m_debugIdToIname.clear();
        foreach (const QmlDebugObjectReference &it, m_rootObjects)
            buildDebugIdHashRecursive(it);

        emit objectTreeUpdated();

        // sync tree with watchhandler
        QList<WatchData> watchData;
        foreach (const QmlDebugObjectReference &obj, m_rootObjects)
            watchData.append(buildWatchData(obj, WatchData()));

        WatchHandler *watchHandler = m_engine->watchHandler();
        watchHandler->beginCycle(InspectWatch, true);
        watchHandler->insertBulkData(watchData);
        watchHandler->endCycle(InspectWatch);
    }
}

void QmlInspectorAgent::onCurrentObjectsFetched(const QmlDebugObjectReference &obj)
{
    if (debug)
        qDebug() << __FUNCTION__ << "( " << obj << ")";

    // get parents if not known yet
    if (!getObjectHierarchy(obj))
        return;

    if (debug)
        qDebug() << "  adding" << m_fetchCurrentObjects << "to tree";

    foreach (const QmlDebugObjectReference &o, m_fetchCurrentObjects)
        addObjectToTree(o, false);

    QmlDebugObjectReference last = m_fetchCurrentObjects.last();
    m_fetchCurrentObjects.clear();

    if (m_objectToSelect == last.debugId()) {
        // select item in view
        QByteArray iname = m_debugIdToIname.value(last.debugId());
        QModelIndex itemIndex = m_engine->watchHandler()->itemIndex(iname);
        QTC_ASSERT(itemIndex.isValid(), return);
        if (debug)
            qDebug() << "  selecting" << iname << "in tree";
        m_engine->watchHandler()->setCurrentModelIndex(InspectWatch, itemIndex);
        m_objectToSelect = -1;
    }

    emit objectFetched(last);
    emit objectTreeUpdated();
}

// Fetches all anchestors of object. Returns if all has been fetched already.
bool QmlInspectorAgent::getObjectHierarchy(const QmlDebugObjectReference &obj)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << obj << ")";

    QmlDebugObjectReference parent = objectForId(obj.parentId());
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

void QmlInspectorAgent::buildDebugIdHashRecursive(const QmlDebugObjectReference &ref)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << ref << ")";

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
            = m_engine->toFileInProject(fileUrl);

    // append the debug ids in the hash
    QPair<QString, int> file = qMakePair<QString, int>(filePath, rev);
    QPair<int, int> location = qMakePair<int, int>(lineNum, colNum);
    if (!m_debugIdHash[file][location].contains(ref.debugId()))
        m_debugIdHash[file][location].append(ref.debugId());

    foreach (const QmlDebugObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}

static QByteArray buildIName(const WatchData &parent, int debugId)
{
    if (!parent.isValid())
        return "inspect." + QByteArray::number(debugId);
    return parent.iname + "." + QByteArray::number(debugId);
}

static QByteArray buildIName(const WatchData &parent, const QString &name)
{
    return parent.iname + "." + name.toLatin1();
}

QList<WatchData> QmlInspectorAgent::buildWatchData(const QmlDebugObjectReference &obj,
                                       const WatchData &parent)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << obj << parent.iname << ")";

    QList<WatchData> list;

    WatchData objWatch;
    QString name = obj.idString();
    if (name.isEmpty())
        name = obj.className();

    // object
    objWatch.id = obj.debugId();
    objWatch.exp = name.toLatin1();
    objWatch.name = name;
    objWatch.iname = buildIName(parent, obj.debugId());
    objWatch.type = obj.className().toLatin1();
    objWatch.value = _("object");
    objWatch.setHasChildren(true);
    objWatch.setAllUnneeded();

    list.append(objWatch);
    m_debugIdToIname.insert(objWatch.id, objWatch.iname);

    // properties
    WatchData propertiesWatch;
    propertiesWatch.id = objWatch.id;
    propertiesWatch.exp = "";
    propertiesWatch.name = tr("properties");
    propertiesWatch.iname = objWatch.iname + ".[properties]";
    propertiesWatch.type = "";
    propertiesWatch.value = _("list");
    propertiesWatch.setHasChildren(true);
    propertiesWatch.setAllUnneeded();

    list.append(propertiesWatch);

    foreach (const QmlDebugPropertyReference &property, obj.properties()) {
        WatchData propertyWatch;
        propertyWatch.exp = property.name().toLatin1();
        propertyWatch.name = property.name();
        propertyWatch.iname = buildIName(propertiesWatch, property.name());
        propertyWatch.type = property.valueTypeName().toLatin1();
        propertyWatch.value = property.value().toString();
        propertyWatch.setAllUnneeded();
        propertyWatch.setHasChildren(false);
        list.append(propertyWatch);
    }

    // recurse
    foreach (const QmlDebugObjectReference &child, obj.children())
        list.append(buildWatchData(child, objWatch));
    return list;
}

void QmlInspectorAgent::addObjectToTree(const QmlDebugObjectReference &obj,
                                        bool notify)
{
    int count = m_rootObjects.count();
    for (int i = 0; i < count; i++) {
        int parentId = obj.parentId();
        if (m_engineClient->serviceVersion() < 2) {
            // we don't get parentId in qt 4.x
            parentId = m_rootObjects[i].insertObjectInTree(obj);
        }

        if (parentId >= 0) {
            buildDebugIdHashRecursive(obj);
            if (notify)
                emit objectTreeUpdated();

            // find parent
            QTC_ASSERT(m_debugIdToIname.contains(parentId), break);
            QByteArray iname = m_debugIdToIname.value(parentId);
            const WatchData *parent = m_engine->watchHandler()->findItem(iname);
            if (parent) {
                QList<WatchData> watches = buildWatchData(obj, *parent);
                m_engine->watchHandler()->beginCycle(false);
                m_engine->watchHandler()->insertBulkData(watches);
                m_engine->watchHandler()->endCycle();
                break;
            }
        }
    }
}

QmlDebugObjectReference QmlInspectorAgent::objectForId(int debugId,
        const QmlDebugObjectReference &objectRef) const
{
    if (objectRef.debugId() == debugId)
        return objectRef;

    foreach (const QmlDebugObjectReference &child, objectRef.children()) {
        QmlDebugObjectReference result = objectForId(debugId, child);
        if (result.debugId() == debugId)
            return result;
    }

    return QmlDebugObjectReference();
}

QList<QmlDebugObjectReference> QmlInspectorAgent::objects(
        const QmlDebugObjectReference &objectRef) const
{
    QList<QmlDebugObjectReference> result;
    result.append(objectRef);

    foreach (const QmlDebugObjectReference &child, objectRef.children())
        result.append(objects(child));

    return result;
}

void QmlInspectorAgent::log(QmlInspectorAgent::LogDirection direction,
                            const QString &message)
{
    QString msg = _("Inspector");
    if (direction == LogSend)
        msg += _(" sending ");
    else
        msg += _(" receiving ");
    msg += message;

    if (m_engine)
        m_engine->showMessage(msg, LogDebug);
}

bool QmlInspectorAgent::isConnected()
{
    return m_engineClient
            && (m_engineClient->status() == QmlDebugClient::Enabled);
}

} // Internal
} // Debugger
