// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlinspectoragent.h"
#include "qmlengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggertr.h>
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

#include <QAction>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>

using namespace QmlDebug;
using namespace QmlDebug::Constants;
using namespace Utils;

namespace Debugger::Internal {

static Q_LOGGING_CATEGORY(qmlInspectorLog, "qtc.dbg.qmlinspector", QtWarningMsg)

// Upper bound on the number of nodes visited while walking the QML context or
// object tree. The underlying object graph may well be cyclic, but what we walk
// here is the structure the wire decoder produced: its children are held by
// value, so it is a finite tree, and the decoder also bounds its depth (see
// BaseEngineDebugClient). The iterative traversals below are therefore
// defensive; this node cap is a further last-resort guard against a
// pathologically large tree. See QTCREATORBUG-33434.
static const int MaxInspectorTreeNodes = 100000;


/*!
 * DebuggerAgent updates the watchhandler with the object tree data.
 */
QmlInspectorAgent::QmlInspectorAgent(QmlEngine *engine, QmlDebugConnection *connection)
    : m_qmlEngine(engine)
    , m_inspectorToolsContext("Debugger.QmlInspector")
    , m_selectAction(new QAction(this))
    , m_showAppOnTopAction(settings().showAppOnTop.action())
{
    m_debugIdToIname.insert(WatchItem::InvalidId, "inspect");
    connect(&settings().showQmlObjectTree, &Utils::BaseAspect::changed,
            this, &QmlInspectorAgent::updateState);
    connect(&settings().sortStructMembers, &Utils::BaseAspect::changed,
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
        QTC_ASSERT(m_debugIdLocations.contains(id), return);
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

    if (!isConnected() || !settings().showQmlObjectTree())
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

    if (m_engineClient->state() == QmlDebugClient::Enabled && settings().showQmlObjectTree())
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
        QString msg = type + Tr::tr("Success:");
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
        if (value.typeId() == QMetaType::QVariantList) {
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
        QTC_ASSERT(!engines.isEmpty(), return);
        m_engines = engines;
        queryEngineContext();
    } else {
        int index = m_rootContextQueryIds.indexOf(queryId);
        if (index < 0) {
            if (QTC_GUARD(m_qmlEngine))
                m_qmlEngine->expressionEvaluated(queryId, value);
        } else if (QTC_GUARD(index < m_engines.length())) {
            const int engineId = m_engines.at(index).debugId();
            m_rootContexts.insert(engineId, qvariant_cast<ContextReference>(value));
            if (m_rootContexts.size() == m_engines.size()) {
                clearObjectTree();
                for (const auto &engine : std::as_const(m_engines)) {
                    QString name = engine.name();
                    if (name.isEmpty())
                        name = QString::fromLatin1("Engine %1").arg(engine.debugId());
                    verifyAndInsertObjectInTree(ObjectReference(engine.debugId(), name),
                                                engine.debugId());
                    updateObjectTree(m_rootContexts[engine.debugId()], engine.debugId());
                    fetchObject(engine.debugId());
                }
                // Objects absent from the context tree (buildObjectList() in the
                // Qt debug service only collects root-context instances, missing
                // objects in per-delegate child contexts) are tracked via
                // m_knownDelegateIds as they arrive via OBJECT_CREATED. Re-fetch
                // them now so they become visible in the Locals tree. Do this
                // after clearObjectTree() so the FETCH_OBJECT responses are not
                // discarded.
                for (int id : std::as_const(m_knownDelegateIds)) {
                    if (!m_debugIdToIname.contains(id))
                        fetchObject(id);
                }
                m_rootContextQueryIds.clear();
            }
        }
    }

    qCDebug(qmlInspectorLog) << __FUNCTION__ << "done";
}

void QmlInspectorAgent::newObject(int engineId, int objectId, int parentId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "() objectId:" << objectId
                             << "parentId:" << parentId;

    log(LogReceive, "OBJECT_CREATED");

    // Track objects with no QObject parent unconditionally, even before
    // m_engines is populated. OBJECT_CREATED for the initial scene arrives
    // synchronously during QML parsing -- before LIST_ENGINES_R has come
    // back -- so m_engines is still empty at that point. If we gated this
    // on the engine loop below, all initial delegate IDs would be missed.
    if (parentId == WatchItem::InvalidId && objectId != WatchItem::InvalidId) {
        // Objects with parentId == -1 have QObject::parent() == nullptr
        // (idForObject returns -1 for null). In Qt Quick, visual parenting
        // via QQuickItem::setParentItem() does not set QObject::parent(),
        // so delegate items created by Repeater, ListView, etc. commonly
        // have null QObject parent despite having a visual parent.
        //
        // Such objects are also absent from the context tree returned by
        // LIST_OBJECTS: buildObjectList() in the Qt debug service collects
        // only the root QQmlContext's instance list and never emits objects
        // that live in per-delegate child contexts. Remember the ID so that
        // after each context-tree rebuild we fetch them directly.
        //
        // Note: this heuristic only covers objects with null QObject
        // parent. Objects created by QQmlInstantiator do have a non-null
        // parent and are missed here despite the same context-tree gap.
        qCDebug(qmlInspectorLog) << "  no QObject parent, queuing for post-rebuild fetch:" << objectId;
        m_knownDelegateIds.insert(objectId);
    }

    for (const auto &engine : std::as_const(m_engines)) {
        if (engine.debugId() == engineId) {
            m_delayQueryTimer.start();
            break;
        }
    }
}

static void sortChildrenIfNecessary(WatchItem *propertiesWatch)
{
    if (settings().sortStructMembers()) {
        propertiesWatch->sortChildren([](const WatchItem *item1, const WatchItem *item2) {
            return item1->name < item2->name;
        });
    }
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

static bool insertChildren(WatchItem *parent, const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::QVariantMap: {
        const QVariantMap map = value.toMap();
        for (auto it = map.begin(), end = map.end(); it != end; ++it) {
            auto child = new WatchItem;
            child->iname = buildIName(parent->iname, it.key());
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
    case QMetaType::QVariantList: {
        const QVariantList list = value.toList();
        for (int i = 0, end = list.size(); i != end; ++i) {
            auto child = new WatchItem;
            const QVariant &value = list.at(i);
            child->iname = buildIName(parent->iname, QString::number(i));
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
    qCDebug(qmlInspectorLog) << __FUNCTION__ << "pending queries:" << m_rootContextQueryIds;

    if (!isConnected() || !settings().showQmlObjectTree())
        return;

    log(LogSend, "LIST_OBJECTS");

    m_rootContexts.clear();
    m_rootContextQueryIds.clear();
    for (const auto &engine : std::as_const(m_engines))
        m_rootContextQueryIds.append(m_engineClient->queryRootContexts(engine));
}

void QmlInspectorAgent::fetchObject(int debugId)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')';

    if (!isConnected() || !settings().showQmlObjectTree())
        return;

    log(LogSend, "FETCH_OBJECT " + QString::number(debugId));
    quint32 queryId = m_engineClient->queryObject(debugId);
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')'
                             << " - query id" << queryId;
    m_objectTreeQueryIds << queryId;
}

void QmlInspectorAgent::updateObjectTree(const ContextReference &context, int engineId)
{
    if (!isConnected() || !settings().showQmlObjectTree())
        return;

    // Walk the context tree iteratively rather than recursively. It is the
    // finite tree the wire decoder produced (children held by value, depth
    // bounded by the decoder), so this is defensive: iterating keeps the walk
    // safe for any depth without relying on that bound. See QTCREATORBUG-33434.
    QList<ContextReference> pending{context};
    int visited = 0;
    while (!pending.isEmpty()) {
        const ContextReference ctx = pending.takeLast();
        qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << ctx << ')';

        for (const ObjectReference &obj : ctx.objects())
            verifyAndInsertObjectInTree(obj, engineId, true);

        if (++visited > MaxInspectorTreeNodes) {
            qCWarning(qmlInspectorLog) << __FUNCTION__ << "aborting after" << visited
                                       << "contexts; tree too large or malformed.";
            break;
        }

        pending.append(ctx.contexts());
    }
}

void QmlInspectorAgent::verifyAndInsertObjectInTree(const ObjectReference &object, int engineId,
                                                     bool calledFromUpdateObjectTree)
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
            const int firstIndex = int(strlen("inspect"));
            const int secondIndex = iname.indexOf('.', firstIndex + 1);
            if (secondIndex != -1)
                engineId = iname.mid(firstIndex + 1, secondIndex - firstIndex - 1).toInt();
        }

        // Still not found? Maybe we're loading the engine itself.
        if (engineId == -1) {
            for (const auto &engine : std::as_const(m_engines)) {
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
                parentId = iname.mid(secondLastIndex + 1, lastIndex - secondLastIndex - 1).toInt();
            else
                parentId = engineId;
        } else {
            parentId = engineId;
        }
    }

    // Only attach a child once its parent WatchItem actually exists; a parent
    // merely named in m_debugIdToIname is deferred and re-fetched below like an
    // unknown one, avoiding insertItem's parent assert. QTCREATORBUG-34790.
    const auto parentIt = m_debugIdToIname.constFind(parentId);
    const QString parentIname = parentIt != m_debugIdToIname.constEnd() ? *parentIt : QString();
    const bool parentPresent = parentId == WatchItem::InvalidId
                               || (parentIt != m_debugIdToIname.constEnd()
                                   && handler->findItem(parentIname) != nullptr);
    if (parentPresent) {
        if (parentId != WatchItem::InvalidId && !handler->isExpandedIName(parentIname)
                && !m_fetchDataIds.contains(parentId)) {
            const WatchItem *parentItem = handler->findItem(parentIname);
            const int parentChildCount = parentItem ? parentItem->childCount() : -1;
            qCDebug(qmlInspectorLog)
                << "  stacking" << object.className() << object.debugId()
                << ": parent" << parentId << "not expanded"
                << "(parent childCount:" << parentChildCount
                << "- expandItem will" << (parentChildCount == 0 ? "fire" : "NOT fire")
                << "=> items with childCount>0 get stuck here)";
            m_objectStack.push(QPair<ObjectReference, int>(object, engineId));
            handler->fetchMore(parentIname);
            return; // recursive
        }
        insertObjectInTree(object, parentId);
        // After inserting the engine node, populate its children from the root context.
        // Guard against the case where we are already inside updateObjectTree for this
        // engine: that function calls us for each object in the context, so if one of
        // those objects is the engine itself, calling updateObjectTree here would restart
        // the same traversal and recurse infinitely.
        if (objectDebugId == engineId && !calledFromUpdateObjectTree)
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
            if (!handler->isExpandedIName(objectIname) && !m_fetchDataIds.contains(objectDebugId)) {
                const WatchItem *parentItem = handler->findItem(objectIname);
                const int parentChildCount = parentItem ? parentItem->childCount() : -1;
                const QString expandNote = parentChildCount == 0
                    ? QLatin1String("fire")
                    : QString("NOT fire => %1 %2 gets stuck")
                          .arg(top.first.className()).arg(top.first.debugId());
                qCDebug(qmlInspectorLog)
                    << "  stack: fetchMore on" << objectIname
                    << "(childCount:" << parentChildCount
                    << "- expandItem will" << expandNote << ")";
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
    // Walk the object tree iteratively rather than recursively. It is the finite
    // tree the wire decoder produced (children held by value, depth bounded by
    // the decoder), so this is defensive: iterating keeps the walk safe for any
    // depth. See QTCREATORBUG-33434.
    QList<ObjectReference> pending{ref};
    int visited = 0;
    while (!pending.isEmpty()) {
        const ObjectReference obj = pending.takeLast();
        qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << obj << ')';

        QUrl fileUrl = obj.source().url();
        int lineNum = obj.source().lineNumber();
        int colNum = obj.source().columnNumber();

        // handle the case where the url contains the revision number encoded.
        // (for object created by the debugger)
        static const QRegularExpression rx("^(.*)_(\\d+):(\\d+)$");
        const QRegularExpressionMatch match = rx.match(fileUrl.path());
        if (match.hasMatch()) {
            fileUrl.setPath(match.captured(1));
            lineNum += match.captured(3).toInt() - 1;
        }

        const FilePath filePath = m_qmlEngine->toFileInProject(fileUrl);
        m_debugIdLocations.insert(obj.debugId(),
                                  FileReference(filePath.toFSPathString(), lineNum, colNum));

        if (++visited > MaxInspectorTreeNodes) {
            qCWarning(qmlInspectorLog) << __FUNCTION__ << "aborting after" << visited
                                       << "objects; tree too large or malformed.";
            break;
        }

        pending.append(obj.children());
    }
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
            name = Tr::tr("<anonymous>");

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
    if (append && !obj.properties().isEmpty()) {
        QString iname = objIname + ".[properties]";
        auto propertiesWatch = new WatchItem;
        propertiesWatch->iname = iname;
        propertiesWatch->name = Tr::tr("Properties");
        propertiesWatch->id = objDebugId;
        propertiesWatch->value = "list";
        propertiesWatch->wantsChildren = true;

        const QList<PropertyReference> properties = obj.properties();
        for (const PropertyReference &property : properties) {
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
    const QList<ObjectReference> children = obj.children();
    for (const ObjectReference &child : children)
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
    if (!m_objectStack.isEmpty()) {
        qCDebug(qmlInspectorLog)
            << "clearObjectTree: discarding" << m_objectStack.size()
            << "unresolved stack items (these were never shown in Locals):";
        for (const auto &entry : std::as_const(m_objectStack)) {
            qCDebug(qmlInspectorLog)
                << "  " << entry.first.className() << entry.first.debugId()
                << "parentId:" << entry.first.parentId();
        }
    }
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
                                            Utils::Id(Constants::QML_SELECTTOOL),
                                            m_inspectorToolsContext);
        Core::ActionManager::registerAction(m_showAppOnTopAction,
                                            Utils::Id(Constants::QML_SHOW_APP_ON_TOP),
                                            m_inspectorToolsContext);

        enableTools(m_qmlEngine->state() == InferiorRunOk);
        if (m_showAppOnTopAction->isChecked())
            m_toolsClient->showAppOnTop(true);
    } else  {
        enableTools(false);

        Core::ActionManager::unregisterAction(m_selectAction, Utils::Id(Constants::QML_SELECTTOOL));
        Core::ActionManager::unregisterAction(m_showAppOnTopAction,
                                              Utils::Id(Constants::QML_SHOW_APP_ON_TOP));
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
    const FilePath filePath = m_qmlEngine->toFileInProject(objSource.url());
    Core::EditorManager::openEditorAt({filePath, objSource.lineNumber()});
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

} // Debugger::Internal
