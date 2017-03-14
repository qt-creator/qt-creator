/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlinspectoragent.h"
#include "qmlengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerengine.h>
#include <debugger/watchhandler.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <qmldebug/declarativeenginedebugclient.h>
#include <qmldebug/declarativeenginedebugclientv2.h>
#include <qmldebug/declarativetoolsclient.h>
#include <qmldebug/qmldebugconstants.h>
#include <qmldebug/qmlenginedebugclient.h>
#include <qmldebug/qmltoolsclient.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QElapsedTimer>
#include <QFileInfo>
#include <QLoggingCategory>

using namespace QmlDebug;
using namespace QmlDebug::Constants;

namespace Debugger {
namespace Internal {

Q_LOGGING_CATEGORY(qmlInspectorLog, "qtc.dbg.qmlinspector")

/*!
 * DebuggerAgent updates the watchhandler with the object tree data.
 */
QmlInspectorAgent::QmlInspectorAgent(QmlEngine *engine, QmlDebugConnection *connection)
    : m_qmlEngine(engine)
    , m_engineClient(0)
    , m_engineQueryId(0)
    , m_rootContextQueryId(0)
    , m_objectToSelect(WatchItem::InvalidId)
    , m_masterEngine(engine)
    , m_toolsClient(0)
    , m_targetToSync(NoTarget)
    , m_debugIdToSelect(WatchItem::InvalidId)
    , m_currentSelectedDebugId(WatchItem::InvalidId)
    , m_toolsClientConnected(false)
    , m_inspectorToolsContext("Debugger.QmlInspector")
    , m_selectAction(new QAction(this))
    , m_zoomAction(new QAction(this))
    , m_showAppOnTopAction(action(ShowAppOnTop))
    , m_engineClientConnected(false)
{
    m_debugIdToIname.insert(WatchItem::InvalidId, "inspect");
    connect(action(ShowQmlObjectTree),
            &Utils::SavedAction::valueChanged, this, &QmlInspectorAgent::updateState);
    connect(action(SortStructMembers), &Utils::SavedAction::valueChanged,
            this, &QmlInspectorAgent::updateState);
    m_delayQueryTimer.setSingleShot(true);
    m_delayQueryTimer.setInterval(100);
    connect(&m_delayQueryTimer, &QTimer::timeout,
            this, &QmlInspectorAgent::queryEngineContext);

    if (!m_masterEngine->isMasterEngine())
        m_masterEngine = m_masterEngine->masterEngine();
    connect(m_masterEngine, &DebuggerEngine::stateChanged,
            this, &QmlInspectorAgent::onEngineStateChanged);

    auto engineClient1 = new DeclarativeEngineDebugClient(connection);
    connect(engineClient1, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::clientStateChanged);
    connect(engineClient1, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::engineClientStateChanged);

    auto engineClient2 = new QmlEngineDebugClient(connection);
    connect(engineClient2, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::clientStateChanged);
    connect(engineClient2, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::engineClientStateChanged);

    auto engineClient3 = new DeclarativeEngineDebugClientV2(connection);
    connect(engineClient3, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::clientStateChanged);
    connect(engineClient3, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::engineClientStateChanged);

    m_engineClients.insert(engineClient1->name(), engineClient1);
    m_engineClients.insert(engineClient2->name(), engineClient2);
    m_engineClients.insert(engineClient3->name(), engineClient3);

    if (engineClient1->state() == QmlDebugClient::Enabled)
        setActiveEngineClient(engineClient1);
    if (engineClient2->state() == QmlDebugClient::Enabled)
        setActiveEngineClient(engineClient2);
    if (engineClient3->state() == QmlDebugClient::Enabled)
        setActiveEngineClient(engineClient3);

    auto toolsClient1 = new DeclarativeToolsClient(connection);
    connect(toolsClient1, &BaseToolsClient::newState,
            this, &QmlInspectorAgent::clientStateChanged);
    connect(toolsClient1, &BaseToolsClient::newState,
            this, &QmlInspectorAgent::toolsClientStateChanged);

    auto toolsClient2 = new QmlToolsClient(connection);
    connect(toolsClient2, &BaseToolsClient::newState,
            this, &QmlInspectorAgent::clientStateChanged);
    connect(toolsClient2, &BaseToolsClient::newState,
            this, &QmlInspectorAgent::toolsClientStateChanged);

    // toolbar
    m_selectAction->setObjectName(QLatin1String("QML Select Action"));
    m_zoomAction->setObjectName(QLatin1String("QML Zoom Action"));
    m_selectAction->setCheckable(true);
    m_zoomAction->setCheckable(true);
    m_showAppOnTopAction->setCheckable(true);
    m_selectAction->setEnabled(false);
    m_zoomAction->setEnabled(false);
    m_showAppOnTopAction->setEnabled(false);

    connect(m_selectAction, &QAction::triggered,
            this, &QmlInspectorAgent::onSelectActionTriggered);
    connect(m_zoomAction, &QAction::triggered,
            this, &QmlInspectorAgent::onZoomActionTriggered);
    connect(m_showAppOnTopAction, &QAction::triggered,
            this, &QmlInspectorAgent::onShowAppOnTopChanged);
}

quint32 QmlInspectorAgent::queryExpressionResult(int debugId,
                                                 const QString &expression)
{
    if (!m_engineClient)
        return 0;

    qCDebug(qmlInspectorLog)
            << __FUNCTION__ << '(' << debugId << expression
            << m_engine.debugId() << ')';

    return m_engineClient->queryExpressionResult(debugId, expression,
                                                 m_engine.debugId());
}

void QmlInspectorAgent::assignValue(const WatchItem *data,
                                    const QString &expr, const QVariant &valueV)
{
    qCDebug(qmlInspectorLog)
            << __FUNCTION__ << '(' << data->id << ')' << data->iname;

    if (data->id != WatchItem::InvalidId) {
        QString val(valueV.toString());
        if (valueV.type() == QVariant::String) {
            val = val.replace(QLatin1Char('\"'), QLatin1String("\\\""));
            val = QLatin1Char('\"') + val + QLatin1Char('\"');
        }
        QString expression = QString("%1 = %2;").arg(expr).arg(val);
        queryExpressionResult(data->id, expression);
    }
}

static int parentIdForIname(const QString &iname)
{
    // Extract the parent id
    int lastIndex = iname.lastIndexOf('.');
    int secondLastIndex = iname.lastIndexOf('.', lastIndex - 1);
    int parentId = WatchItem::InvalidId;
    if (secondLastIndex != WatchItem::InvalidId)
        parentId = iname.mid(secondLastIndex + 1, lastIndex - secondLastIndex - 1).toInt();
    return parentId;
}

void QmlInspectorAgent::updateWatchData(const WatchItem &data)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << data.id << ')';

    if (data.id != WatchItem::InvalidId && !m_fetchDataIds.contains(data.id)) {
        // objects
        m_fetchDataIds << data.id;
        fetchObject(data.id);
    }
}

void QmlInspectorAgent::watchDataSelected(qint64 id)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << id << ')';

    if (id != WatchItem::InvalidId) {
        QTC_ASSERT(m_debugIdLocations.keys().contains(id), return);
        jumpToObjectDefinitionInEditor(m_debugIdLocations.value(id), id);
        if (m_toolsClient)
            m_toolsClient->setObjectIdList({ObjectReference(static_cast<int>(id))});
    }
}

bool QmlInspectorAgent::selectObjectInTree(int debugId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')' << endl
                             << "  " << debugId << "already fetched? "
                             << m_debugIdToIname.contains(debugId);

    if (m_debugIdToIname.contains(debugId)) {
        QString iname = m_debugIdToIname.value(debugId);
        QTC_ASSERT(iname.startsWith("inspect."), qDebug() << iname);
        qCDebug(qmlInspectorLog) << "  selecting" << iname << "in tree";
        m_qmlEngine->watchHandler()->setCurrentItem(iname);
        m_objectToSelect = 0;
        return true;
    } else {
        // we may have to fetch it
        m_objectToSelect = debugId;
        using namespace QmlDebug::Constants;
        if (m_engineClient->objectName() == QLatin1String(QDECLARATIVE_ENGINE)) {
            // reset current Selection
            QString root = m_qmlEngine->watchHandler()->watchItem(QModelIndex())->iname;
            m_qmlEngine->watchHandler()->setCurrentItem(root);
        } else {
            fetchObject(debugId);
        }
        return false;
    }
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
    return ObjectReference(objectDebugId, WatchItem::InvalidId,
                           FileReference(QUrl::fromLocalFile(file), line, column));
}

void QmlInspectorAgent::addObjectWatch(int objectDebugId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << objectDebugId << ')';

    if (objectDebugId == WatchItem::InvalidId)
        return;

    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return;

    // already set
    if (m_objectWatches.contains(objectDebugId))
        return;

    // is flooding the debugging output log!
    // log(LogSend, QString::fromLatin1("WATCH_PROPERTY %1").arg(objectDebugId));

    if (m_engineClient->addWatch(objectDebugId))
        m_objectWatches.append(objectDebugId);
}

QString QmlInspectorAgent::displayName(int objectDebugId) const
{
    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return QString();

    if (m_debugIdToIname.contains(objectDebugId)) {
        const WatchItem *item = m_qmlEngine->watchHandler()->findItem(
                    m_debugIdToIname.value(objectDebugId));
        QTC_ASSERT(item, return QString());
        return item->name;
    }
    return QString();
}

void QmlInspectorAgent::updateState()
{
    if (m_engineClient
            && (m_engineClient->state() == QmlDebugClient::Enabled)
            && boolSetting(ShowQmlObjectTree)) {
        reloadEngines();
    } else {
        clearObjectTree();
    }
}

void QmlInspectorAgent::onResult(quint32 queryId, const QVariant &value,
                                 const QByteArray &type)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "() ...";

    if (type == "FETCH_OBJECT_R") {
        log(LogReceive, QString("FETCH_OBJECT_R %1").arg(
                qvariant_cast<ObjectReference>(value).idString()));
    } else if (type == "SET_BINDING_R"
               || type == "RESET_BINDING_R"
               || type == "SET_METHOD_BODY_R") {
        // FIXME: This is not supported anymore.
        QString msg = QLatin1String(type) + tr("Success:");
        msg += QLatin1Char(' ');
        msg += value.toBool() ? QLatin1Char('1') : QLatin1Char('0');
        // if (!value.toBool())
        //     emit automaticUpdateFailed();
        log(LogReceive, msg);
    } else {
        log(LogReceive, QLatin1String(type));
    }

    if (m_objectTreeQueryIds.contains(queryId)) {
        m_objectTreeQueryIds.removeOne(queryId);
        if (value.type() == QVariant::List) {
            QVariantList objList = value.toList();
            foreach (const QVariant &var, objList) {
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
        m_qmlEngine->expressionEvaluated(queryId, value);
    }

    qCDebug(qmlInspectorLog) << __FUNCTION__ << "done";
}

void QmlInspectorAgent::newObject(int engineId, int /*objectId*/, int /*parentId*/)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "()";

    log(LogReceive, QLatin1String("OBJECT_CREATED"));

    if (m_engine.debugId() != engineId)
        return;

    // TODO: FIX THIS for qt 5.x (Needs update in the qt side)
    m_delayQueryTimer.start();
}

void QmlInspectorAgent::onValueChanged(int debugId, const QByteArray &propertyName,
                                       const QVariant &value)
{
    const QString iname = m_debugIdToIname.value(debugId) +
            ".[properties]." + QString::fromLatin1(propertyName);
    WatchHandler *watchHandler = m_qmlEngine->watchHandler();
    qCDebug(qmlInspectorLog)
            << __FUNCTION__ << '(' << debugId << ')' << iname
            << value.toString();
    if (WatchItem *item = watchHandler->findItem(iname)) {
        item->value = value.toString();
        item->update();
    }
}

void QmlInspectorAgent::reloadEngines()
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "()";

    if (!isConnected())
        return;

    log(LogSend, "LIST_ENGINES");

    m_engineQueryId = m_engineClient->queryAvailableEngines();
}

void QmlInspectorAgent::queryEngineContext()
{
    qCDebug(qmlInspectorLog) << __FUNCTION__;

    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QLatin1String("LIST_OBJECTS"));

    m_rootContextQueryId
            = m_engineClient->queryRootContexts(m_engine);
}

void QmlInspectorAgent::fetchObject(int debugId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')';

    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, QLatin1String("FETCH_OBJECT ") + QString::number(debugId));
    quint32 queryId = m_engineClient->queryObject(debugId);
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')'
                             << " - query id" << queryId;
    m_objectTreeQueryIds << queryId;
}

void QmlInspectorAgent::updateObjectTree(const ContextReference &context)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << context << ')';

    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return;

    foreach (const ObjectReference & obj, context.objects())
        verifyAndInsertObjectInTree(obj);

    foreach (const ContextReference &child, context.contexts())
        updateObjectTree(child);
}

void QmlInspectorAgent::verifyAndInsertObjectInTree(const ObjectReference &object)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << object << ')';

    if (!object.isValid())
        return;

    // Find out the correct position in the tree
    // Objects are inserted to the tree if they satisfy one of the two conditions.
    // Condition 1: Object is a root object i.e. parentId == WatchItem::InvalidId.
    // Condition 2: Object has an expanded parent i.e. siblings are known.
    // If the two conditions are not met then we push the object to a stack and recursively
    // fetch parents till we find a previously expanded parent.

    WatchHandler *handler = m_qmlEngine->watchHandler();
    const int parentId = object.parentId();
    const int objectDebugId = object.debugId();
    if (m_debugIdToIname.contains(parentId)) {
        QString parentIname = m_debugIdToIname.value(parentId);
        if (parentId != WatchItem::InvalidId && !handler->isExpandedIName(parentIname)) {
            m_objectStack.push(object);
            handler->fetchMore(parentIname);
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
            QString objectIname = m_debugIdToIname.value(objectDebugId);
            if (!handler->isExpandedIName(objectIname)) {
                handler->fetchMore(objectIname);
            } else {
                verifyAndInsertObjectInTree(m_objectStack.pop());
                return; // recursive
            }
        }
    }
}

void QmlInspectorAgent::insertObjectInTree(const ObjectReference &object)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << object << ')';

    const int objectDebugId = object.debugId();
    const int parentId = parentIdForIname(m_debugIdToIname.value(objectDebugId));

    QElapsedTimer timeElapsed;

    bool printTime = qmlInspectorLog().isDebugEnabled();
    if (printTime)
        timeElapsed.start();
    addWatchData(object, m_debugIdToIname.value(parentId), true);
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "Time: Build Watch Data took "
                             << timeElapsed.elapsed() << " ms";
    if (printTime)
        timeElapsed.start();
    buildDebugIdHashRecursive(object);
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "Time: Build Debug Id Hash took "
                             << timeElapsed.elapsed() << " ms";

    if (printTime)
        timeElapsed.start();
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "Time: Insertion took "
                             << timeElapsed.elapsed() << " ms";

    if (object.debugId() == m_debugIdToSelect) {
        m_debugIdToSelect = WatchItem::InvalidId;
        selectObject(object, m_targetToSync);
    }

    if (m_debugIdToIname.contains(m_objectToSelect)) {
        // select item in view
        QString iname = m_debugIdToIname.value(m_objectToSelect);
        qCDebug(qmlInspectorLog) << "  selecting" << iname << "in tree";
        m_qmlEngine->watchHandler()->setCurrentItem(iname);
        m_objectToSelect = WatchItem::InvalidId;
    }
    m_qmlEngine->watchHandler()->updateWatchersWindow();
    m_qmlEngine->watchHandler()->reexpandItems();
}

void QmlInspectorAgent::buildDebugIdHashRecursive(const ObjectReference &ref)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << ref << ')';

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
            = m_qmlEngine->toFileInProject(fileUrl);

    // append the debug ids in the hash
    QPair<QString, int> file = qMakePair<QString, int>(filePath, rev);
    QPair<int, int> location = qMakePair<int, int>(lineNum, colNum);
    if (!m_debugIdHash[file][location].contains(ref.debugId()))
        m_debugIdHash[file][location].append(ref.debugId());
    m_debugIdLocations.insert(ref.debugId(), FileReference(filePath, lineNum, colNum));

    foreach (const ObjectReference &it, ref.children())
        buildDebugIdHashRecursive(it);
}

static QString buildIName(const QString &parentIname, int debugId)
{
    if (parentIname.isEmpty())
        return "inspect." + QString::number(debugId);
    return parentIname + "." + QString::number(debugId);
}

static QString buildIName(const QString &parentIname, const QString &name)
{
    return parentIname + "." + name;
}

void QmlInspectorAgent::addWatchData(const ObjectReference &obj,
                                     const QString &parentIname,
                                     bool append)
{
    qCDebug(qmlInspectorLog) << '(' << obj << parentIname << ')';
    QTC_ASSERT(m_qmlEngine, return);

    int objDebugId = obj.debugId();
    QString objIname = buildIName(parentIname, objDebugId);

    if (append) {
        QString name = obj.idString();
        if (name.isEmpty())
            name = obj.className();

        if (name.isEmpty())
            name = obj.name();

        if (name.isEmpty()) {
            FileReference file = obj.source();
            name = file.url().fileName() + ':' + QString::number(file.lineNumber());
        }

        if (name.isEmpty())
            name = tr("<anonymous>");

        // object
        auto objWatch = new WatchItem;
        objWatch->iname = objIname;
        objWatch->name = name;
        objWatch->id = objDebugId;
        objWatch->exp = name;
        objWatch->type = obj.className();
        objWatch->value = "object";
        objWatch->wantsChildren = true;

        m_qmlEngine->watchHandler()->insertItem(objWatch);
        addObjectWatch(objWatch->id);
        if (m_debugIdToIname.contains(objDebugId)) {
            // The data needs to be removed since we now know the parent and
            // hence we can insert the data in the correct position
            const QString oldIname = m_debugIdToIname.value(objDebugId);
            if (oldIname != objIname)
                m_qmlEngine->watchHandler()->removeItemByIName(oldIname);
        }
        m_debugIdToIname.insert(objDebugId, objIname);
    }

    if (!m_qmlEngine->watchHandler()->isExpandedIName(objIname)) {
        // we don't know the children yet. Not adding the 'properties'
        // element makes sure we're queried on expansion.
        if (obj.needsMoreData())
            return;
    }

    // properties
    if (append && obj.properties().count()) {
        QString iname = objIname + ".[properties]";
        auto propertiesWatch = new WatchItem;
        propertiesWatch->iname = iname;
        propertiesWatch->name = tr("Properties");
        propertiesWatch->id = objDebugId;
        propertiesWatch->value = "list";
        propertiesWatch->wantsChildren = true;

        foreach (const PropertyReference &property, obj.properties()) {
            const QString propertyName = property.name();
            if (propertyName.isEmpty())
                continue;
            auto propertyWatch = new WatchItem;
            propertyWatch->iname = buildIName(iname, propertyName);
            propertyWatch->name = propertyName;
            propertyWatch->id = objDebugId;
            propertyWatch->exp = propertyName;
            propertyWatch->type = property.valueTypeName();
            propertyWatch->value = property.value().toString();
            propertyWatch->wantsChildren = false;
            propertiesWatch->appendChild(propertyWatch);
        }

        if (boolSetting(SortStructMembers)) {
            propertiesWatch->sortChildren([](const WatchItem *item1, const WatchItem *item2) {
                return item1->name < item2->name;
            });
        }

        m_qmlEngine->watchHandler()->insertItem(propertiesWatch);
    }

    // recurse
    foreach (const ObjectReference &child, obj.children())
        addWatchData(child, objIname, append);
}

void QmlInspectorAgent::log(QmlInspectorAgent::LogDirection direction,
                            const QString &message)
{
    QString msg = "Inspector";
    if (direction == LogSend)
        msg += " sending ";
    else
        msg += " receiving ";
    msg += message;

    if (m_qmlEngine)
        m_qmlEngine->showMessage(msg, LogDebug);
}

bool QmlInspectorAgent::isConnected() const
{
    return m_engineClient && m_engineClient->state() == QmlDebugClient::Enabled;
}

void QmlInspectorAgent::clearObjectTree()
{
    if (m_qmlEngine)
        m_qmlEngine->watchHandler()->removeAllData(true);
    m_objectTreeQueryIds.clear();
    m_fetchDataIds.clear();
    int old_count = m_debugIdHash.count();
    m_debugIdHash.clear();
    m_debugIdHash.reserve(old_count + 1);
    m_debugIdToIname.clear();
    m_debugIdToIname.insert(WatchItem::InvalidId, "inspect");
    m_objectStack.clear();
    m_objectWatches.clear();
}

void QmlInspectorAgent::clientStateChanged(QmlDebugClient::State state)
{
    QString serviceName;
    float version = 0;
    if (QmlDebugClient *client = qobject_cast<QmlDebugClient*>(sender())) {
        serviceName = client->name();
        version = client->serviceVersion();
    }

    m_qmlEngine->logServiceStateChange(serviceName, version, state);
}

void QmlInspectorAgent::toolsClientStateChanged(QmlDebugClient::State state)
{
    BaseToolsClient *client = qobject_cast<BaseToolsClient*>(sender());
    QTC_ASSERT(client, return);
    if (state == QmlDebugClient::Enabled) {
        m_toolsClient = client;

        connect(client, &BaseToolsClient::currentObjectsChanged,
                this, &QmlInspectorAgent::selectObjectsFromToolsClient);
        connect(client, &BaseToolsClient::logActivity,
                m_qmlEngine.data(), &QmlEngine::logServiceActivity);
        connect(client, &BaseToolsClient::reloaded,
                this, &QmlInspectorAgent::onReloaded);

        // register actions here
        // because there can be multiple QmlEngines
        // at the same time (but hopefully one one is connected)
        Core::ActionManager::registerAction(m_selectAction,
                                            Core::Id(Constants::QML_SELECTTOOL),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_zoomAction, Core::Id(Constants::QML_ZOOMTOOL),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_showAppOnTopAction,
                                            Core::Id(Constants::QML_SHOW_APP_ON_TOP),
                                            m_inspectorToolsContext);

        Core::ICore::addAdditionalContext(m_inspectorToolsContext);

        m_toolsClientConnected = true;
        onEngineStateChanged(m_masterEngine->state());
        if (m_showAppOnTopAction->isChecked())
            m_toolsClient->showAppOnTop(true);

    } else if (m_toolsClientConnected && client == m_toolsClient) {
        disconnect(client, &BaseToolsClient::currentObjectsChanged,
                   this, &QmlInspectorAgent::selectObjectsFromToolsClient);
        disconnect(client, &BaseToolsClient::logActivity,
                   m_qmlEngine.data(), &QmlEngine::logServiceActivity);

        Core::ActionManager::unregisterAction(m_selectAction, Core::Id(Constants::QML_SELECTTOOL));
        Core::ActionManager::unregisterAction(m_zoomAction, Core::Id(Constants::QML_ZOOMTOOL));
        Core::ActionManager::unregisterAction(m_showAppOnTopAction,
                                              Core::Id(Constants::QML_SHOW_APP_ON_TOP));

        Core::ICore::removeAdditionalContext(m_inspectorToolsContext);

        enableTools(false);
        m_toolsClientConnected = false;
        m_selectAction->setCheckable(false);
        m_zoomAction->setCheckable(false);
        m_showAppOnTopAction->setCheckable(false);
    }
}

void QmlInspectorAgent::engineClientStateChanged(QmlDebugClient::State state)
{
    BaseEngineDebugClient *client
            = qobject_cast<BaseEngineDebugClient*>(sender());

    if (state == QmlDebugClient::Enabled && !m_engineClientConnected) {
        // We accept the first client that is enabled and reject the others.
        QTC_ASSERT(client, return);
        setActiveEngineClient(client);
    } else if (m_engineClientConnected && client == m_engineClient) {
        m_engineClientConnected = false;
    }
}

void QmlInspectorAgent::selectObjectsFromToolsClient(const QList<int> &debugIds)
{
    if (debugIds.isEmpty())
        return;

    m_targetToSync = EditorTarget;
    m_debugIdToSelect = debugIds.first();
    selectObject(objectForId(m_debugIdToSelect), EditorTarget);
}

void QmlInspectorAgent::onSelectActionTriggered(bool checked)
{
    QTC_ASSERT(m_toolsClient, return);
    if (checked) {
        m_toolsClient->setDesignModeBehavior(true);
        m_toolsClient->changeToSelectTool();
        m_zoomAction->setChecked(false);
    } else {
        m_toolsClient->setDesignModeBehavior(false);
    }
}

void QmlInspectorAgent::onZoomActionTriggered(bool checked)
{
    QTC_ASSERT(m_toolsClient, return);
    if (checked) {
        m_toolsClient->setDesignModeBehavior(true);
        m_toolsClient->changeToZoomTool();
        m_selectAction->setChecked(false);
    } else {
        m_toolsClient->setDesignModeBehavior(false);
    }
}

void QmlInspectorAgent::onShowAppOnTopChanged(bool checked)
{
    QTC_ASSERT(m_toolsClient, return);
    m_toolsClient->showAppOnTop(checked);
}

void QmlInspectorAgent::setActiveEngineClient(BaseEngineDebugClient *client)
{
    if (m_engineClient == client)
        return;

    if (m_engineClient) {
        disconnect(m_engineClient, &BaseEngineDebugClient::newState,
                   this, &QmlInspectorAgent::updateState);
        disconnect(m_engineClient, &BaseEngineDebugClient::result,
                   this, &QmlInspectorAgent::onResult);
        disconnect(m_engineClient, &BaseEngineDebugClient::newObject,
                   this, &QmlInspectorAgent::newObject);
        disconnect(m_engineClient, &BaseEngineDebugClient::valueChanged,
                   this, &QmlInspectorAgent::onValueChanged);
    }

    m_engineClient = client;

    if (m_engineClient) {
        connect(m_engineClient, &BaseEngineDebugClient::newState,
                this, &QmlInspectorAgent::updateState);
        connect(m_engineClient, &BaseEngineDebugClient::result,
                this, &QmlInspectorAgent::onResult);
        connect(m_engineClient, &BaseEngineDebugClient::newObject,
                this, &QmlInspectorAgent::newObject);
        connect(m_engineClient, &BaseEngineDebugClient::valueChanged,
                this, &QmlInspectorAgent::onValueChanged);
    }

    updateState();

    m_engineClientConnected = true;
}

void QmlInspectorAgent::jumpToObjectDefinitionInEditor(
        const FileReference &objSource, int debugId)
{
    const QString fileName = m_masterEngine->toFileInProject(objSource.url());

    Core::EditorManager::openEditorAt(fileName, objSource.lineNumber());
    if (debugId != WatchItem::InvalidId && debugId != m_currentSelectedDebugId) {
        m_currentSelectedDebugId = debugId;
        m_currentSelectedDebugName = displayName(debugId);
    }
}

void QmlInspectorAgent::selectObject(const ObjectReference &obj, SelectionTarget target)
{
    if (m_toolsClient && target == ToolTarget)
        m_toolsClient->setObjectIdList(QList<ObjectReference>() << obj);

    if (target == EditorTarget)
        jumpToObjectDefinitionInEditor(obj.source());

    selectObjectInTree(obj.debugId());
}

void QmlInspectorAgent::enableTools(const bool enable)
{
    if (!m_toolsClientConnected)
        return;
    m_selectAction->setEnabled(enable);
    m_showAppOnTopAction->setEnabled(enable);
    // only enable zoom action for Qt 4.x/old client
    // (zooming is integrated into selection tool in Qt 5).
    if (!qobject_cast<QmlToolsClient*>(m_toolsClient))
        m_zoomAction->setEnabled(enable);
}

void QmlInspectorAgent::onReloaded()
{
    reloadEngines();
}

void QmlInspectorAgent::onEngineStateChanged(const DebuggerState state)
{
    enableTools(state == InferiorRunOk);
}

} // namespace Internal
} // namespace Debugger
