/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
using namespace QmlDebug::Constants;

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
    , m_newObjectsCreated(false)
{
    m_debugIdToIname.insert(-1, QByteArray("inspect"));
    connect(debuggerCore()->action(ShowQmlObjectTree),
            SIGNAL(valueChanged(QVariant)), SLOT(updateStatus()));
    m_delayQueryTimer.setSingleShot(true);
    m_delayQueryTimer.setInterval(100);
    connect(&m_delayQueryTimer, SIGNAL(timeout()), SLOT(queryEngineContext()));
}

quint32 QmlInspectorAgent::queryExpressionResult(int debugId,
                                                 const QString &expression)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << debugId << expression
                 << m_engine.debugId() << ')';

    return m_engineClient->queryExpressionResult(debugId, expression,
                                                 m_engine.debugId());
}

void QmlInspectorAgent::assignValue(const WatchData *data,
                                    const QString &expr, const QVariant &valueV)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << data->id << ')' << data->iname;

    if (data->id) {
        QString val(valueV.toString());
        if (valueV.type() == QVariant::String) {
            val = val.replace(QLatin1Char('\"'), QLatin1String("\\\""));
            val = QLatin1Char('\"') + val + QLatin1Char('\"');
        }
        QString expression = QString(_("%1 = %2;")).arg(expr).arg(val);
        queryExpressionResult(data->id, expression);
    }
}

int parentIdForIname(const QByteArray &iname)
{
    // Extract the parent id
    int lastIndex = iname.lastIndexOf('.');
    int secondLastIndex = iname.lastIndexOf('.', lastIndex - 1);
    int parentId = -1;
    if (secondLastIndex != -1)
        parentId = iname.mid(secondLastIndex + 1, lastIndex - secondLastIndex - 1).toInt();
    return parentId;
}

void QmlInspectorAgent::updateWatchData(const WatchData &data)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << data.id << ')';

    if (data.id && !m_fetchDataIds.contains(data.id)) {
        // objects
        m_fetchDataIds << data.id;
        fetchObject(data.id);
    }
}

void QmlInspectorAgent::watchDataSelected(const WatchData *data)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << data->id << ')';

    if (data->id) {
        QTC_ASSERT(m_debugIdLocations.keys().contains(data->id), return);
        emit jumpToObjectDefinition(m_debugIdLocations.value(data->id), data->id);
    }
}

bool QmlInspectorAgent::selectObjectInTree(int debugId)
{
    if (debug) {
        qDebug() << __FUNCTION__ << '(' << debugId << ')';
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
        return true;
    } else {
        // we may have to fetch it
        m_objectToSelect = debugId;
        using namespace QmlDebug::Constants;
        if (m_engineClient->objectName() == QLatin1String(QDECLARATIVE_ENGINE)) {
            // reset current Selection
            QByteArray root = m_debuggerEngine->watchHandler()->watchData(QModelIndex())->iname;
            m_debuggerEngine->watchHandler()->setCurrentItem(root);
        } else {
            fetchObject(debugId);
        }
        return false;
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
        qDebug() << __FUNCTION__ << '(' << objectDebugId << propertyName
                 << value.toString() << isLiteralValue << source << line << ')';

    if (objectDebugId == -1)
        return 0;

    if (propertyName == QLatin1String("id"))
        return 0; // Crashes the QMLViewer.

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString::fromLatin1("SET_BINDING %1 %2 %3 %4").arg(
            QString::number(objectDebugId), propertyName, value.toString(),
            QString(isLiteralValue ? QLatin1String("true") : QLatin1String("false"))));

    quint32 queryId = m_engineClient->setBindingForObject(
                objectDebugId, propertyName, value.toString(), isLiteralValue,
                source, line);

    if (!queryId)
        log(LogSend, QLatin1String("SET_BINDING failed!"));

    return queryId;
}

quint32 QmlInspectorAgent::setMethodBodyForObject(int objectDebugId,
                                            const QString &methodName,
                                            const QString &methodBody)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << objectDebugId
                 << methodName << methodBody << ')';

    if (objectDebugId == -1)
        return 0;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString::fromLatin1("SET_METHOD_BODY %1 %2 %3").arg(
            QString::number(objectDebugId), methodName, methodBody));

    quint32 queryId = m_engineClient->setMethodBody(
                objectDebugId, methodName, methodBody);

    if (!queryId)
        log(LogSend, QLatin1String("failed!"));

    return queryId;
}

quint32 QmlInspectorAgent::resetBindingForObject(int objectDebugId,
                                           const QString &propertyName)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << objectDebugId
                 << propertyName << ')';

    if (objectDebugId == -1)
        return 0;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return 0;

    log(LogSend, QString::fromLatin1("RESET_BINDING %1 %2").arg(
            QString::number(objectDebugId), propertyName));

    quint32 queryId = m_engineClient->resetBindingForObject(
                objectDebugId, propertyName);

    if (!queryId)
        log(LogSend, QLatin1String("failed!"));

    return queryId;
}

ObjectReference QmlInspectorAgent::objectForName(
        const QString &objectId) const
{
    if (!objectId.isEmpty() && objectId[0].isLower()) {
        QHashIterator<int, QByteArray> iter(m_debugIdToIname);
        const WatchHandler *watchHandler = m_debuggerEngine->watchHandler();
        while (iter.hasNext()) {
            iter.next();
            const WatchData *wd = watchHandler->findData(iter.value());
            if (wd && wd->name == objectId)
                return ObjectReference(iter.key());
        }
    }
    return ObjectReference();
}

ObjectReference QmlInspectorAgent::objectForId(int objectDebugId) const
{
    if (!m_debugIdToIname.contains(objectDebugId))
        return ObjectReference(objectDebugId);

    int line = -1;
    int column = -1;
    QString file;
    QHashIterator<QPair<QString, int>, QHash<QPair<int, int>, QList<int> > > iter(m_debugIdHash);
    while (iter.hasNext()) {
        iter.next();
        QHashIterator<QPair<int, int>, QList<int> > i(iter.value());
        while (i.hasNext()) {
            i.next();
            if (i.value().contains(objectDebugId)) {
                line = i.key().first;
                column = i.key().second;
                break;
            }
        }
        if (line != -1) {
            file = iter.key().first;
            break;
        }
    }
    // TODO: Set correct parentId
    return ObjectReference(objectDebugId, -1,
                           FileReference(QUrl::fromLocalFile(file), line, column));
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
    const WatchHandler *watchHandler = m_debuggerEngine->watchHandler();
    foreach (const QByteArray &in, m_debugIdToIname) {
        const WatchData *data = watchHandler->findData(in);
        if (!data)
            continue;
        int debugId = data->id;
        QString className = QLatin1String(data->type);
        rIds.insert(debugId, className);
    }
     return rIds;
}

bool QmlInspectorAgent::addObjectWatch(int objectDebugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << objectDebugId << ')';

    if (objectDebugId == -1)
        return false;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return false;

    // already set
    if (m_objectWatches.contains(objectDebugId))
        return true;

    // is flooding the debugging output log!
    // log(LogSend, QString::fromLatin1("WATCH_PROPERTY %1").arg(objectDebugId));

    if (m_engineClient->addWatch(objectDebugId))
        m_objectWatches.append(objectDebugId);

    return true;
}

bool QmlInspectorAgent::isObjectBeingWatched(int objectDebugId)
{
    return m_objectWatches.contains(objectDebugId);
}

bool QmlInspectorAgent::removeObjectWatch(int objectDebugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << objectDebugId << ')';

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
        disconnect(m_engineClient, SIGNAL(valueChanged(int,QByteArray,QVariant)),
                   this, SLOT(onValueChanged(int,QByteArray,QVariant)));
    }

    m_engineClient = client;

    if (m_engineClient) {
        connect(m_engineClient, SIGNAL(newStatus(QmlDebug::ClientStatus)),
                this, SLOT(updateStatus()));
        connect(m_engineClient, SIGNAL(result(quint32,QVariant,QByteArray)),
                this, SLOT(onResult(quint32,QVariant,QByteArray)));
        connect(m_engineClient, SIGNAL(newObject(int,int,int)),
                this, SLOT(newObject(int,int,int)));
        connect(m_engineClient, SIGNAL(valueChanged(int,QByteArray,QVariant)),
                this, SLOT(onValueChanged(int,QByteArray,QVariant)));
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

    if (type == "FETCH_OBJECT_R") {
        log(LogReceive, _("FETCH_OBJECT_R %1").arg(
                qvariant_cast<ObjectReference>(value).idString()));
    } else if (type == "SET_BINDING_R"
               || type == "RESET_BINDING_R"
               || type == "SET_METHOD_BODY_R") {
        QString msg = QLatin1String(type) + tr("Success: ");
        msg += value.toBool() ? QLatin1Char('1') : QLatin1Char('0');
        if (!value.toBool())
            emit automaticUpdateFailed();
        log(LogReceive, msg);
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
                verifyAndInsertObjectInTree(qvariant_cast<ObjectReference>(var));
            }
        } else {
            verifyAndInsertObjectInTree(qvariant_cast<ObjectReference>(value));
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
        updateObjectTree(qvariant_cast<ContextReference>(value));
    } else {
        emit expressionResult(queryId, value);
    }

    if (debug)
        qDebug() << __FUNCTION__ << "done";

}

void QmlInspectorAgent::newObject(int engineId, int /*objectId*/, int /*parentId*/)
{
    if (debug)
        qDebug() << __FUNCTION__ << "()";

    log(LogReceive, QLatin1String("OBJECT_CREATED"));

    if (m_engine.debugId() != engineId)
        return;

    // TODO: FIX THIS for qt 5.x (Needs update in the qt side)
    m_delayQueryTimer.start();
}

void QmlInspectorAgent::onValueChanged(int debugId, const QByteArray &propertyName,
                                       const QVariant &value)
{
    const QByteArray iname = m_debugIdToIname.value(debugId) +
            ".[properties]." + propertyName;
    WatchHandler *watchHandler = m_debuggerEngine->watchHandler();
    const WatchData *data = watchHandler->findData(iname);
    if (debug)
        qDebug() << __FUNCTION__ << '(' << debugId << ')' << iname << value.toString();
    if (data) {
        WatchData d(*data);
        d.value = value.toString();
        watchHandler->insertData(d);
    }
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

int QmlInspectorAgent::parentIdForObject(int objectDebugId)
{
    int pid = -1;

    if (m_debugIdToIname.contains(objectDebugId)) {
        QByteArray iname = m_debugIdToIname.value(objectDebugId);
        if (iname.count('.') > 1) {
            int offset = iname.lastIndexOf('.');
            QTC_ASSERT(offset > 0, return pid);
            iname = iname.left(offset);
            pid = m_debugIdToIname.key(iname);
        }
    }
    return pid;
}

void QmlInspectorAgent::queryEngineContext()
{
    if (debug)
        qDebug() << __FUNCTION__;

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QLatin1String("LIST_OBJECTS"));

    m_rootContextQueryId
            = m_engineClient->queryRootContexts(m_engine);
}

void QmlInspectorAgent::fetchObject(int debugId)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << debugId << ')';

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QLatin1String("FETCH_OBJECT ") + QString::number(debugId));
    quint32 queryId = m_engineClient->queryObject(debugId);
    if (debug)
        qDebug() << __FUNCTION__ << '(' << debugId << ')'
                 << " - query id" << queryId;
    m_objectTreeQueryIds << queryId;
}

void QmlInspectorAgent::fetchContextObjectsForLocation(const QString &file,
                                     int lineNumber, int columnNumber)
{
    // This can be an expensive operation as it may return multiple
    // objects. Use fetchContextObject() where possible.
    if (debug)
        qDebug() << __FUNCTION__ << '(' << file << ':' << lineNumber
                 << ':' << columnNumber << ')';

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QString::fromLatin1("FETCH_OBJECTS_FOR_LOCATION %1:%2:%3").arg(file)
        .arg(QString::number(lineNumber)).arg(QString::number(columnNumber)));
    quint32 queryId = m_engineClient->queryObjectsForLocation(QFileInfo(file).fileName(),
                                                             lineNumber, columnNumber);
    if (debug)
        qDebug() << __FUNCTION__ << '(' << file << ':' << lineNumber
                 << ':' << columnNumber << ')' << " - query id" << queryId;

    m_objectTreeQueryIds << queryId;
}

void QmlInspectorAgent::updateObjectTree(const ContextReference &context)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << context << ')';

    if (!isConnected()
            || !debuggerCore()->boolSetting(ShowQmlObjectTree))
        return;

    foreach (const ObjectReference & obj, context.objects())
        verifyAndInsertObjectInTree(obj);

    foreach (const ContextReference &child, context.contexts())
        updateObjectTree(child);
}

void QmlInspectorAgent::verifyAndInsertObjectInTree(const ObjectReference &object)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << object << ')';

    if (!object.isValid())
        return;

    // Find out the correct position in the tree
    // Objects are inserted to the tree if they satisfy one of the two conditions.
    // Condition 1: Object is a root object i.e. parentId == -1.
    // Condition 2: Object has an expanded parent i.e. siblings are known.
    // If the two conditions are not met then we push the object to a stack and recursively
    // fetch parents till we find a previously expanded parent.

    WatchHandler *handler = m_debuggerEngine->watchHandler();
    const int parentId = object.parentId();
    const int objectDebugId = object.debugId();
    if (m_debugIdToIname.contains(parentId)) {
        QByteArray parentIname = m_debugIdToIname.value(parentId);
        if (parentId != -1 && !handler->isExpandedIName(parentIname)) {
            m_objectStack.push(object);
            handler->model()->fetchMore(handler->watchDataIndex(parentIname));
            return; // recursive
        }
        insertObjectInTree(object);

    } else {
        m_objectStack.push(object);
        fetchObject(parentId);
        return; // recursive
    }
    if (!m_objectStack.isEmpty()) {
        const ObjectReference &top = m_objectStack.top();
        // We want to expand only a particular branch and not the whole tree. Hence, we do not
        // expand siblings.
        if (object.children().contains(top)) {
            QByteArray objectIname = m_debugIdToIname.value(objectDebugId);
            if (!handler->isExpandedIName(objectIname)) {
                handler->model()->fetchMore(handler->watchDataIndex(objectIname));
            } else {
                verifyAndInsertObjectInTree(m_objectStack.pop());
                return; // recursive
            }
        }
    }
}

void QmlInspectorAgent::insertObjectInTree(const ObjectReference &object)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << object << ')';

    const int objectDebugId = object.debugId();
    const int parentId = parentIdForIname(m_debugIdToIname.value(objectDebugId));

    QElapsedTimer timeElapsed;
    QList<WatchData> watchData;
    if (debug)
        timeElapsed.start();
    watchData.append(buildWatchData(object, m_debugIdToIname.value(parentId), true));
    if (debug)
        qDebug() << __FUNCTION__ << "Time: Build Watch Data took "
                 << timeElapsed.elapsed() << " ms";
    if (debug)
        timeElapsed.start();
    buildDebugIdHashRecursive(object);
    if (debug)
        qDebug() << __FUNCTION__ << "Time: Build Debug Id Hash took "
                 << timeElapsed.elapsed() << " ms";

    WatchHandler *watchHandler = m_debuggerEngine->watchHandler();
    if (debug)
        timeElapsed.start();
    watchHandler->insertData(watchData);
    if (debug)
        qDebug() << __FUNCTION__ << "Time: Insertion took " << timeElapsed.elapsed() << " ms";

    emit objectTreeUpdated();
    emit objectFetched(object);

    if (m_debugIdToIname.contains(m_objectToSelect)) {
        // select item in view
        QByteArray iname = m_debugIdToIname.value(m_objectToSelect);
        if (debug)
            qDebug() << "  selecting" << iname << "in tree";
        watchHandler->setCurrentItem(iname);
        m_objectToSelect = -1;
    }
}

void QmlInspectorAgent::buildDebugIdHashRecursive(const ObjectReference &ref)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << ref << ')';

    QUrl fileUrl = ref.source().url();
    int lineNum = ref.source().lineNumber();
    int colNum = ref.source().columnNumber();
    int rev = 0;

    // handle the case where the url contains the revision number encoded.
    //(for object created by the debugger)
    static QRegExp rx(QLatin1String("(.*)_(\\d+):(\\d+)$"));
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
                                       const QByteArray &parentIname,
                                       bool append)
{
    if (debug)
        qDebug() << __FUNCTION__ << '(' << obj << parentIname << ')';

    QList<WatchData> list;

    int objDebugId = obj.debugId();
    QByteArray objIname = buildIName(parentIname, objDebugId);

    if (append) {
        WatchData objWatch;
        QString name = obj.idString();
        if (name.isEmpty())
            name = obj.className();

        if (name.isEmpty())
            return list;

        // object
        objWatch.id = objDebugId;
        objWatch.exp = name.toLatin1();
        objWatch.name = name;
        objWatch.iname = objIname;
        objWatch.type = obj.className().toLatin1();
        objWatch.value = _("object");
        objWatch.setHasChildren(true);
        objWatch.setAllUnneeded();

        list.append(objWatch);
        addObjectWatch(objWatch.id);
        if (m_debugIdToIname.contains(objDebugId)) {
            // The data needs to be removed since we now know the parent and
            // hence we can insert the data in the correct position
            const QByteArray oldIname = m_debugIdToIname.value(objDebugId);
            if (oldIname != objIname)
                m_debuggerEngine->watchHandler()->removeData(oldIname);
        }
        m_debugIdToIname.insert(objDebugId, objIname);
    }

    if (!m_debuggerEngine->watchHandler()->isExpandedIName(objIname)) {
        // we don't know the children yet. Not adding the 'properties'
        // element makes sure we're queried on expansion.
        if (obj.needsMoreData())
            return list;

        // To improve performance, we do not insert data for items
        // that have not been previously queried when the object tree is refreshed.
        if (m_newObjectsCreated)
            append = false;
    }

    // properties
    if (append && obj.properties().count()) {
        WatchData propertiesWatch;
        propertiesWatch.id = objDebugId;
        propertiesWatch.exp = "";
        propertiesWatch.name = tr("Properties");
        propertiesWatch.iname = objIname + ".[properties]";
        propertiesWatch.type = "";
        propertiesWatch.value = _("list");
        propertiesWatch.setHasChildren(true);
        propertiesWatch.setAllUnneeded();

        list.append(propertiesWatch);

        foreach (const PropertyReference &property, obj.properties()) {
            WatchData propertyWatch;
            propertyWatch.id = objDebugId;
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
        list.append(buildWatchData(child, objIname, append));
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
    m_debuggerEngine->watchHandler()->removeAllData(true);
    m_objectTreeQueryIds.clear();
    m_fetchDataIds.clear();
    int old_count = m_debugIdHash.count();
    m_debugIdHash.clear();
    m_debugIdHash.reserve(old_count + 1);
    m_debugIdToIname.clear();
    m_debugIdToIname.insert(-1, QByteArray("inspect"));
    m_objectStack.clear();
    // reset only for qt > 4.8.3.
    if (m_engineClient->objectName() != QLatin1String(QDECLARATIVE_ENGINE))
        m_newObjectsCreated = false;

    removeAllObjectWatches();
}
} // Internal
} // Debugger

