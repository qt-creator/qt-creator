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
#include <debugger/debuggerruncontrol.h>
#include <debugger/watchhandler.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

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

Q_LOGGING_CATEGORY(qmlInspectorLog, "qtc.dbg.qmlinspector", QtWarningMsg)


/*!
 * DebuggerAgent updates the watchhandler with the object tree data.
 */
QmlInspectorAgent::QmlInspectorAgent(QmlEngine *engine, QmlDebugConnection *connection)
    : m_qmlEngine(engine)
    , m_inspectorToolsContext("Debugger.QmlInspector")
    , m_selectAction(new QAction(this))
    , m_showAppOnTopAction(action(ShowAppOnTop))
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

    m_engineClient = new QmlEngineDebugClient(connection);
    connect(m_engineClient, &BaseEngineDebugClient::newState,
            this, &QmlInspectorAgent::updateState);
    connect(m_engineClient, &BaseEngineDebugClient::result,
            this, &QmlInspectorAgent::onResult);
    connect(m_engineClient, &BaseEngineDebugClient::newObject,
            this, &QmlInspectorAgent::newObject);
    connect(m_engineClient, &BaseEngineDebugClient::valueChanged,
            this, &QmlInspectorAgent::onValueChanged);
    updateState();

    m_toolsClient = new QmlToolsClient(connection);
    connect(m_toolsClient, &BaseToolsClient::newState,
            this, &QmlInspectorAgent::toolsClientStateChanged);
    connect(m_toolsClient, &BaseToolsClient::currentObjectsChanged,
            this, &QmlInspectorAgent::selectObjectsFromToolsClient);
    connect(m_toolsClient, &BaseToolsClient::logActivity,
            m_qmlEngine.data(), &QmlEngine::logServiceActivity);
    connect(m_toolsClient, &BaseToolsClient::reloaded,
            this, &QmlInspectorAgent::onReloaded);

    // toolbar
    m_selectAction->setObjectName("QML Select Action");
    m_selectAction->setCheckable(true);
    m_showAppOnTopAction->setCheckable(true);
    enableTools(m_toolsClient->state() == QmlDebugClient::Enabled);

    connect(m_selectAction, &QAction::triggered,
            this, &QmlInspectorAgent::onSelectActionTriggered);
    connect(m_showAppOnTopAction, &QAction::triggered,
            this, &QmlInspectorAgent::onShowAppOnTopChanged);
}

int QmlInspectorAgent::engineId(const WatchItem *data) const
{
    int id = -1;
    for (; data; data = data->parent())
        id = data->id >= 0 ? data->id : id;
    return id;
}

quint32 QmlInspectorAgent::queryExpressionResult(int debugId, const QString &expression,
                                                 int engineId)
{
    qCDebug(qmlInspectorLog)
            << __FUNCTION__ << '(' << debugId << expression << engineId << ')';

    return m_engineClient->queryExpressionResult(debugId, expression, engineId);
}

void QmlInspectorAgent::assignValue(const WatchItem *data,
                                    const QString &expr, const QVariant &valueV)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << data->id << ')' << data->iname;

    if (data->id != WatchItem::InvalidId) {
        QString val(valueV.toString());
        QString expression = QString("%1 = %2;").arg(expr).arg(val);
        queryExpressionResult(data->id, expression, engineId(data));
    }
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

void QmlInspectorAgent::watchDataSelected(int id)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << id << ')';

    if (id != WatchItem::InvalidId) {
        QTC_ASSERT(m_debugIdLocations.keys().contains(id), return);
        jumpToObjectDefinitionInEditor(m_debugIdLocations.value(id));
        m_toolsClient->selectObjects({id});
    }
}

void QmlInspectorAgent::selectObjectsInTree(const QList<int> &debugIds)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugIds << ')';

    for (int debugId : debugIds) {
        if (m_debugIdToIname.contains(debugId)) {
            const QString iname = m_debugIdToIname.value(debugId);
            QTC_ASSERT(iname.startsWith("inspect."), qDebug() << iname);
            qCDebug(qmlInspectorLog) << "  selecting" << iname << "in tree";

            // We can't multi-select in the watch handler for now ...
            m_qmlEngine->watchHandler()->setCurrentItem(iname);
            m_objectsToSelect.removeOne(debugId);
            continue;
        }

        // we may have to fetch it
        m_objectsToSelect.append(debugId);
        fetchObject(debugId);
    }
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

void QmlInspectorAgent::updateState()
{
    m_qmlEngine->logServiceStateChange(m_engineClient->name(), m_engineClient->serviceVersion(),
                                       m_engineClient->state());

    if (m_engineClient->state() == QmlDebugClient::Enabled && boolSetting(ShowQmlObjectTree))
        reloadEngines();
    else
        clearObjectTree();
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
        QString msg = type + tr("Success:");
        msg += ' ';
        msg += value.toBool() ? '1' : '0';
        // if (!value.toBool())
        //     emit automaticUpdateFailed();
        log(LogReceive, msg);
    } else {
        log(LogReceive, QLatin1String(type));
    }

    if (m_objectTreeQueryIds.contains(queryId)) {
        m_objectTreeQueryIds.removeOne(queryId);
        if (value.type() == QVariant::List) {
            const QVariantList objList = value.toList();
            for (const QVariant &var : objList) {
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
        m_engines = engines;
        queryEngineContext();
    } else {
        int index = m_rootContextQueryIds.indexOf(queryId);
        if (index < 0) {
            m_qmlEngine->expressionEvaluated(queryId, value);
        } else {
            Q_ASSERT(index < m_engines.length());
            const int engineId = m_engines.at(index).debugId();
            m_rootContexts.insert(engineId, qvariant_cast<ContextReference>(value));
            if (m_rootContexts.size() == m_engines.size()) {
                clearObjectTree();
                for (const auto &engine : m_engines) {
                    QString name = engine.name();
                    if (name.isEmpty())
                        name = QString::fromLatin1("Engine %1").arg(engine.debugId());
                    verifyAndInsertObjectInTree(ObjectReference(engine.debugId(), name),
                                                engine.debugId());
                    updateObjectTree(m_rootContexts[engine.debugId()], engine.debugId());
                    fetchObject(engine.debugId());
                }
                m_rootContextQueryIds.clear();
            }
        }
    }

    qCDebug(qmlInspectorLog) << __FUNCTION__ << "done";
}

void QmlInspectorAgent::newObject(int engineId, int /*objectId*/, int /*parentId*/)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "()";

    log(LogReceive, "OBJECT_CREATED");

    for (const auto &engine : qAsConst(m_engines)) {
        if (engine.debugId() == engineId) {
            m_delayQueryTimer.start();
            break;
        }
    }
}

static void sortChildrenIfNecessary(WatchItem *propertiesWatch)
{
    if (boolSetting(SortStructMembers)) {
        propertiesWatch->sortChildren([](const WatchItem *item1, const WatchItem *item2) {
            return item1->name < item2->name;
        });
    }
}

static bool insertChildren(WatchItem *parent, const QVariant &value)
{
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.begin(), end = map.end(); it != end; ++it) {
            auto child = new WatchItem;
            child->name = it.key();
            child->value = it.value().toString();
            child->type = QLatin1String(it.value().typeName());
            child->valueEditable = false;
            child->wantsChildren = insertChildren(child, it.value());
            parent->appendChild(child);
        }
        sortChildrenIfNecessary(parent);
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        for (int i = 0, end = list.size(); i != end; ++i) {
            auto child = new WatchItem;
            const QVariant &value = list.at(i);
            child->arrayIndex = i;
            child->value = value.toString();
            child->type = QLatin1String(value.typeName());
            child->valueEditable = false;
            child->wantsChildren = insertChildren(child, value);
            parent->appendChild(child);
        }
        return true;
    }
    default:
        return false;
    }
}

void QmlInspectorAgent::onValueChanged(int debugId, const QByteArray &propertyName,
                                       const QVariant &value)
{
    const QString iname = m_debugIdToIname.value(debugId) +
            ".[properties]." + QString::fromLatin1(propertyName);
    WatchHandler *watchHandler = m_qmlEngine->watchHandler();
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')' << iname << value.toString();
    if (WatchItem *item = watchHandler->findItem(iname)) {
        item->value = value.toString();
        item->removeChildren();
        item->wantsChildren = insertChildren(item, value);
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

    log(LogSend, "LIST_OBJECTS");

    m_rootContexts.clear();
    for (const auto &engine : qAsConst(m_engines))
        m_rootContextQueryIds.append(m_engineClient->queryRootContexts(engine));
}

void QmlInspectorAgent::fetchObject(int debugId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')';

    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return;

    log(LogSend, "FETCH_OBJECT " + QString::number(debugId));
    quint32 queryId = m_engineClient->queryObject(debugId);
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')'
                             << " - query id" << queryId;
    m_objectTreeQueryIds << queryId;
}

void QmlInspectorAgent::updateObjectTree(const ContextReference &context, int engineId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << context << ')';

    if (!isConnected() || !boolSetting(ShowQmlObjectTree))
        return;

    for (const ObjectReference &obj : context.objects())
        verifyAndInsertObjectInTree(obj, engineId);

    for (const ContextReference &child : context.contexts())
        updateObjectTree(child, engineId);
}

void QmlInspectorAgent::verifyAndInsertObjectInTree(const ObjectReference &object, int engineId)
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
    const int objectDebugId = object.debugId();
    int parentId = object.parentId();

    if (engineId == -1) {
        // Find engineId if missing.
        const auto it = m_debugIdToIname.find(objectDebugId);
        if (it != m_debugIdToIname.end()) {
            const QString iname = *it;
            const int firstIndex = strlen("inspect");
            const int secondIndex = iname.indexOf('.', firstIndex + 1);
            if (secondIndex != -1)
                engineId = iname.midRef(firstIndex + 1, secondIndex - firstIndex - 1).toInt();
        }

        // Still not found? Maybe we're loading the engine itself.
        if (engineId == -1) {
            for (const auto &engine : m_engines) {
                if (engine.debugId() == objectDebugId) {
                    engineId = engine.debugId();
                    break;
                }
            }
        }
    }

    if (objectDebugId == engineId) {
        // Don't load an engine's parent
        parentId = -1;
    } else if (parentId == -1) {
        // Find parentId if missing
        const auto it = m_debugIdToIname.find(objectDebugId);
        if (it != m_debugIdToIname.end()) {
            const QString iname = *it;
            int lastIndex = iname.lastIndexOf('.');
            int secondLastIndex = iname.lastIndexOf('.', lastIndex - 1);
            if (secondLastIndex != WatchItem::InvalidId)
                parentId = iname.midRef(secondLastIndex + 1, lastIndex - secondLastIndex - 1).toInt();
            else
                parentId = engineId;
        } else {
            parentId = engineId;
        }
    }

    if (m_debugIdToIname.contains(parentId)) {
        QString parentIname = m_debugIdToIname.value(parentId);
        if (parentId != WatchItem::InvalidId && !handler->isExpandedIName(parentIname)) {
            m_objectStack.push(QPair<ObjectReference, int>(object, engineId));
            handler->fetchMore(parentIname);
            return; // recursive
        }
        insertObjectInTree(object, parentId);
        if (objectDebugId == engineId)
            updateObjectTree(m_rootContexts[engineId], engineId);
    } else {
        m_objectStack.push(QPair<ObjectReference, int>(object, engineId));
        fetchObject(parentId);
        return; // recursive
    }
    if (!m_objectStack.isEmpty()) {
        const auto &top = m_objectStack.top();
        // We want to expand only a particular branch and not the whole tree. Hence, we do not
        // expand siblings. If this is the engine, we add add root objects with the same engine ID
        // as children.
        if (object.children().contains(top.first)
                || (top.first.parentId() == objectDebugId)
                || (top.first.parentId() < 0 && objectDebugId == top.second)) {
            QString objectIname = m_debugIdToIname.value(objectDebugId);
            if (!handler->isExpandedIName(objectIname)) {
                handler->fetchMore(objectIname);
            } else {
                verifyAndInsertObjectInTree(top.first, top.second);
                m_objectStack.pop();
                return; // recursive
            }
        }
    }
}

void QmlInspectorAgent::insertObjectInTree(const ObjectReference &object, int parentId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << object << ')';

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

    for (auto it = m_objectsToSelect.begin(); it != m_objectsToSelect.end();) {
        if (m_debugIdToIname.contains(*it)) {
            // select item in view
            QString iname = m_debugIdToIname.value(*it);
            qCDebug(qmlInspectorLog) << "  selecting" << iname << "in tree";
            m_qmlEngine->watchHandler()->setCurrentItem(iname);
            it = m_objectsToSelect.erase(it);
        } else {
            ++it;
        }
    }
    m_qmlEngine->watchHandler()->updateLocalsWindow();
    m_qmlEngine->watchHandler()->reexpandItems();
}

void QmlInspectorAgent::buildDebugIdHashRecursive(const ObjectReference &ref)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << ref << ')';

    QUrl fileUrl = ref.source().url();
    int lineNum = ref.source().lineNumber();
    int colNum = ref.source().columnNumber();

    // handle the case where the url contains the revision number encoded.
    // (for object created by the debugger)
    static QRegExp rx("(.*)_(\\d+):(\\d+)$");
    if (rx.exactMatch(fileUrl.path())) {
        fileUrl.setPath(rx.cap(1));
        lineNum += rx.cap(3).toInt() - 1;
    }

    const QString filePath = m_qmlEngine->toFileInProject(fileUrl);
    m_debugIdLocations.insert(ref.debugId(), FileReference(filePath, lineNum, colNum));

    const auto children = ref.children();
    for (const ObjectReference &it : children)
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
            propertyWatch->wantsChildren = insertChildren(propertyWatch, property.value());
            propertiesWatch->appendChild(propertyWatch);
        }

        sortChildrenIfNecessary(propertiesWatch);
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
    return m_engineClient->state() == QmlDebugClient::Enabled;
}

void QmlInspectorAgent::clearObjectTree()
{
    if (m_qmlEngine)
        m_qmlEngine->watchHandler()->removeAllData(true);
    m_objectTreeQueryIds.clear();
    m_fetchDataIds.clear();
    m_debugIdToIname.clear();
    m_debugIdToIname.insert(WatchItem::InvalidId, "inspect");
    m_objectStack.clear();
    m_objectWatches.clear();
}

void QmlInspectorAgent::toolsClientStateChanged(QmlDebugClient::State state)
{
    QTC_ASSERT(m_toolsClient, return);
    m_qmlEngine->logServiceStateChange(m_toolsClient->name(), m_toolsClient->serviceVersion(),
                                       state);
    if (state == QmlDebugClient::Enabled) {
        Core::ICore::addAdditionalContext(m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_selectAction,
                                            Core::Id(Constants::QML_SELECTTOOL),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_showAppOnTopAction,
                                            Core::Id(Constants::QML_SHOW_APP_ON_TOP),
                                            m_inspectorToolsContext);

        enableTools(m_qmlEngine->state() == InferiorRunOk);
        if (m_showAppOnTopAction->isChecked())
            m_toolsClient->showAppOnTop(true);
    } else  {
        enableTools(false);

        Core::ActionManager::unregisterAction(m_selectAction, Core::Id(Constants::QML_SELECTTOOL));
        Core::ActionManager::unregisterAction(m_showAppOnTopAction,
                                              Core::Id(Constants::QML_SHOW_APP_ON_TOP));
        Core::ICore::removeAdditionalContext(m_inspectorToolsContext);
    }
}

void QmlInspectorAgent::selectObjectsFromToolsClient(const QList<int> &debugIds)
{
    if (!debugIds.isEmpty())
        selectObjects(debugIds, m_debugIdLocations.value(debugIds.first()));
}

void QmlInspectorAgent::onSelectActionTriggered(bool checked)
{
    QTC_ASSERT(m_toolsClient, return);
    if (checked) {
        m_toolsClient->setDesignModeBehavior(true);
        m_toolsClient->changeToSelectTool();
    } else {
        m_toolsClient->setDesignModeBehavior(false);
    }
}

void QmlInspectorAgent::onShowAppOnTopChanged(bool checked)
{
    QTC_ASSERT(m_toolsClient, return);
    m_toolsClient->showAppOnTop(checked);
}

void QmlInspectorAgent::jumpToObjectDefinitionInEditor(const FileReference &objSource)
{
    const QString fileName = m_qmlEngine->toFileInProject(objSource.url());
    Core::EditorManager::openEditorAt(fileName, objSource.lineNumber());
}

void QmlInspectorAgent::selectObjects(const QList<int> &debugIds,
                                      const QmlDebug::FileReference &source)
{
    jumpToObjectDefinitionInEditor(source);
    selectObjectsInTree(debugIds);
}

void QmlInspectorAgent::enableTools(const bool enable)
{
    m_selectAction->setEnabled(enable);
    m_showAppOnTopAction->setEnabled(enable);
}

void QmlInspectorAgent::onReloaded()
{
    reloadEngines();
}

} // namespace Internal
} // namespace Debugger
