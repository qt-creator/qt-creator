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
#include <QElapsedTimer>

using namespace QmlDebug;

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
    , m_debuggerEngine(engine)
    , m_engineClient(0)
    , m_engineQueryId(0)
    , m_rootContextQueryId(0)
    , m_objectToSelect(-1)
{
    connect(debuggerCore()->action(ShowQmlObjectTree),
            SIGNAL(valueChanged(QVariant)), SLOT(updateStatus()));
    m_delayQueryTimer.setSingleShot(true);
    m_delayQueryTimer.setInterval(500);
    connect(&m_delayQueryTimer, SIGNAL(timeout()), SLOT(queryEngineContext()));
}

quint32 QmlInspectorAgent::queryExpressionResult(int debugId,
                                                 const QString &expression)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << debugId << expression
                 << m_engine.debugId() << ")";

    return m_engineClient->queryExpressionResult(debugId, expression,
                                                 m_engine.debugId());
}

void QmlInspectorAgent::updateWatchData(const WatchData &data)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << data.id << ")";

    if (data.id && !m_fetchDataIds.contains(data.id)) {
        // objects
        m_fetchDataIds << data.id;
        fetchObject(data.id);
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
        if (debug)
            qDebug() << "  selecting" << iname << "in tree";
        m_debuggerEngine->watchHandler()->setCurrentItem(iname);
        m_objectToSelect = 0;
    } else {
        // we've to fetch it
        m_objectToSelect = debugId;
        fetchObject(debugId);
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

ObjectReference QmlInspectorAgent::objectForId(
        const QString &objectId) const
{
    if (!objectId.isEmpty() && objectId[0].isLower()) {
        QHashIterator<int, QByteArray> iter(m_debugIdToIname);
        while (iter.hasNext()) {
            iter.next();
            if (m_debuggerEngine->watchHandler()->findData(iter.value())->name == objectId)
                return ObjectReference(iter.key());
        }
    }
    return ObjectReference();
}

int QmlInspectorAgent::objectIdForLocation(
        int line, int column) const
{
    QHashIterator<int, FileReference> iter(m_debugIdLocations);
    while (iter.hasNext()) {
        iter.next();
        const FileReference &ref = iter.value();
        if (ref.lineNumber() == line
                && ref.columnNumber() == column)
            return iter.key();
    }

    return -1;
}

QHash<int,QString> QmlInspectorAgent::rootObjectIds() const
{
    QHash<int,QString> rIds;
    foreach (const QByteArray &in, m_debugIdToIname) {
        const WatchData *data = m_debuggerEngine->watchHandler()->findData(in);
        int debugId = data->id;
        QString className = data->type;
        rIds.insert(debugId, className);
    }
     return rIds;
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

    if (!m_debugIdToIname.contains(objectDebugId))
        return false;

    // is flooding the debugging output log!
    // log(LogSend, QString("WATCH_PROPERTY %1").arg(objectDebugId));

    if (m_engineClient->addWatch(objectDebugId))
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
        disconnect(m_engineClient, SIGNAL(newStatus(QmlDebug::ClientStatus)),
                   this, SLOT(updateStatus()));
        disconnect(m_engineClient, SIGNAL(result(quint32,QVariant,QByteArray)),
                   this, SLOT(onResult(quint32,QVariant,QByteArray)));
        disconnect(m_engineClient, SIGNAL(newObject(int,int,int)),
                   this, SLOT(newObject(int,int,int)));
    }

    m_engineClient = client;

    if (m_engineClient) {
        connect(m_engineClient, SIGNAL(newStatus(QmlDebug::ClientStatus)),
                this, SLOT(updateStatus()));
        connect(m_engineClient, SIGNAL(result(quint32,QVariant,QByteArray)),
                this, SLOT(onResult(quint32,QVariant,QByteArray)));
        connect(m_engineClient, SIGNAL(newObject(int,int,int)),
                this, SLOT(newObject(int,int,int)));
    }

    updateStatus();
}

QString QmlInspectorAgent::displayName(int objectDebugId) const
{
    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return QString();

    if (m_debugIdToIname.contains(objectDebugId)) {
        const WatchData *data = m_debuggerEngine->watchHandler()->findData(
                    m_debugIdToIname.value(objectDebugId));
        QTC_ASSERT(data, return QString());
        return data->name;
    }
    return QString();
}

void QmlInspectorAgent::updateStatus()
{
    if (m_engineClient
            && (m_engineClient->status() == QmlDebug::Enabled)
            && debuggerCore()->boolSetting(ShowQmlObjectTree)) {
        reloadEngines();
    } else {
        clearObjectTree();
    }
}

void QmlInspectorAgent::onResult(quint32 queryId, const QVariant &value,
                                 const QByteArray &type)
{
    if (debug)
        qDebug() << __FUNCTION__ << "() ...";

    if (type == _("FETCH_OBJECT_R")) {
        log(LogReceive, _("FETCH_OBJECT_R %1").arg(
                qvariant_cast<ObjectReference>(value).idString()));
    } else {
        log(LogReceive, QLatin1String(type));
    }

    if (m_objectTreeQueryIds.contains(queryId)) {
        m_objectTreeQueryIds.removeOne(queryId);
        if (value.type() == QVariant::List) {
            QVariantList objList = value.toList();
            foreach (QVariant var, objList) {
                // TODO: check which among the list is the actual
                // object that needs to be selected.
                objectTreeFetched(qvariant_cast<ObjectReference>(var));
            }
        } else {
            objectTreeFetched(qvariant_cast<ObjectReference>(value));
        }
    } else if (queryId == m_engineQueryId) {
        m_engineQueryId = 0;
        QList<EngineReference> engines = qvariant_cast<QList<EngineReference> >(value);
        QTC_ASSERT(engines.count(), return);
        // only care about first engine atm
        m_engine = engines.at(0);
        queryEngineContext();
    } else if (queryId == m_rootContextQueryId) {
        m_rootContextQueryId = 0;
        clearObjectTree();
        fetchObjectsInContextRecursive(qvariant_cast<ContextReference>(value));
    } else {
        emit expressionResult(queryId, value);
    }

    if (debug)
        qDebug() << __FUNCTION__ << "done";

}

void QmlInspectorAgent::newObject(int engineId, int objectId, int /*parentId*/)
{
    if (debug)
        qDebug() << __FUNCTION__ << "()";

    log(LogReceive, QString("OBJECT_CREATED"));

    if (m_engine.debugId() != engineId)
        return;
    if (m_engineClient->objectName() == QmlDebug::Constants::QML_DEBUGGER)
        fetchObject(objectId);
    else
        m_delayQueryTimer.start();
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

void QmlInspectorAgent::queryEngineContext()
{
    if (debug)
        qDebug() << __FUNCTION__;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QString("LIST_OBJECTS"));

    m_rootContextQueryId
            = m_engineClient->queryRootContexts(m_engine);
}

void QmlInspectorAgent::fetchObject(int debugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << debugId << ")";

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QString("FETCH_OBJECT %1").arg(QString::number(debugId)));
    quint32 queryId = m_engineClient->queryObject(debugId);
    if (debug)
        qDebug() << __FUNCTION__ << "(" << debugId << ")"
                 << " - query id" << queryId;
    m_objectTreeQueryIds << queryId;
}

void QmlInspectorAgent::fetchContextObjectsForLocation(const QString &file,
                                     int lineNumber, int columnNumber)
{
    // This can be an expensive operation as it may return multiple
    // objects. Use fetchContextObject() where possible.
    if (debug)
        qDebug() << __FUNCTION__ << "(" << file << ":" << lineNumber
                 << ":" << columnNumber << ")";

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QString("FETCH_OBJECTS_FOR_LOCATION %1:%2:%3").arg(file)
        .arg(QString::number(lineNumber)).arg(QString::number(columnNumber)));
    quint32 queryId = m_engineClient->queryObjectsForLocation(QFileInfo(file).fileName(),
                                                             lineNumber, columnNumber);
    if (debug)
        qDebug() << __FUNCTION__ << "(" << file << ":" << lineNumber
                 << ":" << columnNumber << ")" << " - query id" << queryId;

    m_objectTreeQueryIds << queryId;
}

// fetch the root objects from the context + any child contexts
void QmlInspectorAgent::fetchObjectsInContextRecursive(const ContextReference &context)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << context << ")";

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    foreach (const ObjectReference & obj, context.objects()) {
        using namespace QmlDebug::Constants;
        if (m_engineClient->objectName() == QML_DEBUGGER &&
                m_engineClient->serviceVersion() >= CURRENT_SUPPORTED_VERSION) {
            //Fetch only root objects
            if (obj.parentId() == -1)
                fetchObject(obj.debugId());
        } else {
            m_objectTreeQueryIds << m_engineClient->queryObjectRecursive(obj.debugId());
        }
    }
    foreach (const ContextReference &child, context.contexts())
        fetchObjectsInContextRecursive(child);
}

void QmlInspectorAgent::objectTreeFetched(const ObjectReference &object)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << object << ")";

    if (!object.isValid())
        return;

    m_objectStack.push(object);

    if (m_engineClient->objectName() == QmlDebug::Constants::QML_DEBUGGER) {
        if (object.parentId() != -1 && !m_debugIdToIname.contains(object.parentId())) {
            fetchObject(object.parentId());
            return;
        }
    } else if (m_objectTreeQueryIds.count()) {
        return;
    }

    // sync tree with watchhandler
    QList<WatchData> watchData;
    ObjectReference last;
    if (m_engineClient->objectName() == QmlDebug::Constants::QML_DEBUGGER) {
        while (!m_objectStack.isEmpty()) {
            last = m_objectStack.pop();
            QByteArray parentIname;
            if (last.parentId() != -1) {
                QTC_ASSERT(m_debugIdToIname.contains(last.parentId()), return);
                parentIname = m_debugIdToIname.value(last.parentId());
            }
            watchData.append(buildWatchData(last, parentIname));
            buildDebugIdHashRecursive(last);
        }
    } else {
        QVectorIterator<ObjectReference> iter(m_objectStack);
        while (iter.hasNext()) {
            const ObjectReference &r = iter.next();
            int pid = -1;
            QHashIterator<int, QList<int> > i(m_debugIdChildIds);
            while (i.hasNext()) {
                QList<int> cids = i.value();
                foreach (int cid, cids) {
                    if (cid == r.debugId()) {
                        pid = i.key();
                        break;
                    }
                }
                if (pid != -1)
                    break;
            }
            QByteArray parentIname;
            if (m_debugIdToIname.contains(pid))
                parentIname = m_debugIdToIname.value(r.parentId());
            watchData.append(buildWatchData(r, parentIname));
            buildDebugIdHashRecursive(r);
        }
        m_objectStack.clear();
    }

    m_debuggerEngine->watchHandler()->insertData(watchData);
    emit objectTreeUpdated();

    if (m_engineClient->objectName() == QmlDebug::Constants::QML_DEBUGGER) {
        emit objectFetched(last);

        if (m_objectToSelect == last.debugId()) {
            // select item in view
            QByteArray iname = m_debugIdToIname.value(last.debugId());
            if (debug)
                qDebug() << "  selecting" << iname << "in tree";
            m_debuggerEngine->watchHandler()->setCurrentItem(iname);
            m_objectToSelect = -1;
        }
    }
}

void QmlInspectorAgent::buildDebugIdHashRecursive(const ObjectReference &ref)
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
            = m_debuggerEngine->toFileInProject(fileUrl);

    // append the debug ids in the hash
    QPair<QString, int> file = qMakePair<QString, int>(filePath, rev);
    QPair<int, int> location = qMakePair<int, int>(lineNum, colNum);
    if (!m_debugIdHash[file][location].contains(ref.debugId()))
        m_debugIdHash[file][location].append(ref.debugId());
    m_debugIdLocations.insert(ref.debugId(), FileReference(filePath, lineNum, colNum));
    foreach (const ObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}

static QByteArray buildIName(const QByteArray &parentIname, int debugId)
{
    if (parentIname.isEmpty())
        return "inspect." + QByteArray::number(debugId);
    return parentIname + "." + QByteArray::number(debugId);
}

static QByteArray buildIName(const QByteArray &parentIname, const QString &name)
{
    return parentIname + "." + name.toLatin1();
}

QList<WatchData> QmlInspectorAgent::buildWatchData(const ObjectReference &obj,
                                       const QByteArray &parentIname)
{
    if (debug)
        qDebug() << __FUNCTION__ << "(" << obj << parentIname << ")";

    QList<WatchData> list;

    WatchData objWatch;
    QString name = obj.idString();
    if (name.isEmpty())
        name = obj.className();

    if (name.isEmpty())
        return list;

    // object
    objWatch.id = obj.debugId();
    objWatch.exp = name.toLatin1();
    objWatch.name = name;
    objWatch.iname = buildIName(parentIname, obj.debugId());
    objWatch.type = obj.className().toLatin1();
    objWatch.value = _("object");
    objWatch.setHasChildren(true);
    objWatch.setAllUnneeded();

    list.append(objWatch);
    m_debugIdToIname.insert(objWatch.id, objWatch.iname);

    if (m_engineClient->objectName() != QmlDebug::Constants::QML_DEBUGGER) {
        QList<int> childIds;
        foreach (const ObjectReference &c, obj.children()) {
            childIds << c.debugId();
        }
        // For 4.x, we do not get the parentId. Hence, store the child ids
        // to look up correct insertion places later
        m_debugIdChildIds.insert(objWatch.id, childIds);
    }

    if (!m_debuggerEngine->watchHandler()->isExpandedIName(objWatch.iname)
            && obj.needsMoreData()) {
        // we don't know the children yet. Not adding the 'properties'
        // element makes sure we're queried on expansion.
        return list;
    }

    // properties
    if (obj.properties().count()) {
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

        foreach (const PropertyReference &property, obj.properties()) {
            WatchData propertyWatch;
            propertyWatch.exp = property.name().toLatin1();
            propertyWatch.name = property.name();
            propertyWatch.iname = buildIName(propertiesWatch.iname, property.name());
            propertyWatch.type = property.valueTypeName().toLatin1();
            propertyWatch.value = property.value().toString();
            propertyWatch.setAllUnneeded();
            propertyWatch.setHasChildren(false);
            list.append(propertyWatch);
        }
    }

    // recurse
    foreach (const ObjectReference &child, obj.children())
        list.append(buildWatchData(child, objWatch.iname));
    return list;
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

    if (m_debuggerEngine)
        m_debuggerEngine->showMessage(msg, LogDebug);
}

bool QmlInspectorAgent::isConnected() const
{
    return m_engineClient
            && (m_engineClient->status() == QmlDebug::Enabled);
}

void QmlInspectorAgent::clearObjectTree()
{
    // clear view
    m_debuggerEngine->watchHandler()->removeChildren("inspect");

    m_objectTreeQueryIds.clear();

    int old_count = m_debugIdHash.count();
    m_debugIdHash.clear();
    m_debugIdHash.reserve(old_count + 1);
    m_debugIdToIname.clear();
    m_debugIdChildIds.clear();
    m_objectStack.clear();
}
} // Internal
} // Debugger
