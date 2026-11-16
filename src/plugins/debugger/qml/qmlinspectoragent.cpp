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
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <qmldebug/qmlenginedebugclient.h>
#include <qmldebug/qmltoolsclient.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>

using namespace QmlDebug;
using namespace Utils;
using namespace std::chrono;

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

static constexpr milliseconds MinRebuildDelay = 100ms;
static constexpr milliseconds RebuildInterval = 1000ms;


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
    connect(&m_delayQueryTimer, &QTimer::timeout,
            this, &QmlInspectorAgent::rebuildObjectTree);
    m_updateLocalsTimer.setSingleShot(true);
    m_updateLocalsTimer.setInterval(0);
    connect(&m_updateLocalsTimer, &QTimer::timeout, this, [this] {
        if (!m_qmlEngine)
            return;
        m_qmlEngine->watchHandler()->updateLocalsWindow();
        m_qmlEngine->watchHandler()->reexpandItems();
    });
    m_valueChangeTimer.setSingleShot(true);
    m_valueChangeTimer.setInterval(100);
    connect(&m_valueChangeTimer, &QTimer::timeout,
            this, &QmlInspectorAgent::applyValueChanges);

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
    } else if (data.id == WatchItem::InvalidId) {
        const int engine = engineId(&data);
        if (engine != WatchItem::InvalidId && data.iname == parentlessIName(engine))
            fetchParentlessObjects(engine);
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

    m_seenObjectIds.insert(objectDebugId);

    // already set
    if (m_objectWatches.contains(objectDebugId))
        return;

    // is flooding the debugging output log!
    // log(LogSend, QString::fromLatin1("WATCH_PROPERTY %1").arg(objectDebugId));

    if (const quint32 watchId = m_engineClient->addWatch(objectDebugId))
        m_objectWatches.insert(objectDebugId, watchId);
}

// The debug service keeps a watch until it is removed explicitly and adds a
// duplicate for every repeated WATCH_OBJECT, so the watches must survive tree
// rebuilds.
void QmlInspectorAgent::removeObjectWatches()
{
    if (isConnected()) {
        for (const quint32 watchId : std::as_const(m_objectWatches))
            m_engineClient->removeWatch(watchId);
    }
    m_objectWatches.clear();
    m_seenObjectIds.clear();
    m_previouslySeenObjectIds.clear();
}

// There is no notification for destroyed objects. Drop the watches of the
// objects that were not inserted during the last two rebuilds. One is not
// enough: the view fetches expanded objects late, and the next rebuild can
// start before it has.
void QmlInspectorAgent::removeStaleObjectWatches()
{
    for (auto it = m_objectWatches.begin(); it != m_objectWatches.end();) {
        if (m_seenObjectIds.contains(it.key()) || m_previouslySeenObjectIds.contains(it.key())) {
            ++it;
            continue;
        }
        if (isConnected())
            m_engineClient->removeWatch(*it);
        it = m_objectWatches.erase(it);
    }
    m_previouslySeenObjectIds = std::exchange(m_seenObjectIds, {});
}

void QmlInspectorAgent::updateState()
{
    m_qmlEngine->logServiceStateChange(m_engineClient->name(), m_engineClient->serviceVersion(),
                                       m_engineClient->state());

    if (m_engineClient->state() == QmlDebugClient::Enabled && settings().showQmlObjectTree()) {
        reloadEngines();
    } else {
        clearObjectTree();
        removeObjectWatches();
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
        QString msg = type + Tr::tr("Success:");
        msg += ' ';
        msg += value.toBool() ? '1' : '0';
        // if (!value.toBool())
        //     emit automaticUpdateFailed();
        log(LogReceive, msg);
    } else {
        log(LogReceive, QLatin1String(type));
    }

    if (const auto it = m_delegateQueryIds.constFind(queryId); it != m_delegateQueryIds.cend()) {
        const int debugId = *it;
        m_delegateQueryIds.erase(it);
        const ObjectReference object = qvariant_cast<ObjectReference>(value);
        // The application answers with nothing for an object it has destroyed.
        if (!object.isValid()) {
            m_knownDelegateIds.remove(debugId);
        } else {
            WatchHandler *handler = m_qmlEngine->watchHandler();
            const QString parentless
                = parentlessIName(m_knownDelegateIds.value(debugId, WatchItem::InvalidId));
            if (handler->isExpandedIName(parentless)) {
                verifyAndInsertObjectInTree(object);
            } else if (WatchItem *item = handler->findItem(parentless);
                       item && !handler->findItem(m_debugIdToIname.value(debugId))) {
                // The item was collapsed in the meantime. Make its next
                // expansion fetch what was dropped here.
                item->forAllChildren([this](WatchItem *child) {
                    m_fetchDataIds.removeAll(int(child->id));
                });
                item->removeChildren();
            }
        }
    } else if (m_objectTreeQueryIds.contains(queryId)) {
        m_objectTreeQueryIds.removeOne(queryId);
        if (value.typeId() == QMetaType::QVariantList) {
            const QVariantList objList = value.toList();
            for (const QVariant &var : objList) {
                // TODO: check which among the list is the actual
                // object that needs to be selected.
                verifyAndInsertObjectInTree(qvariant_cast<ObjectReference>(var));
            }
        } else {
            const ObjectReference object = qvariant_cast<ObjectReference>(value);
            verifyAndInsertObjectInTree(object);
            // The objects without a parent are fetched after the engine, whose
            // item replaces them otherwise.
            const int debugId = object.debugId();
            const bool isEngine = Utils::anyOf(m_engines, [debugId](const EngineReference &engine) {
                return engine.debugId() == debugId;
            });
            if (isEngine && m_qmlEngine->watchHandler()->isExpandedIName(parentlessIName(debugId)))
                fetchParentlessObjects(debugId);
        }
    } else if (queryId == m_engineQueryId) {
        m_engineQueryId = 0;
        const QList<EngineReference> engines = qvariant_cast<QList<EngineReference> >(value);
        if (engines.isEmpty()) {
            // No QML engine has registered yet. This is a legitimate transient,
            // not an error (OBJECT_CREATED for the initial scene can even arrive
            // first, see newObject()), and the engine may be created much later,
            // e.g. only once the user triggers some code path. Keep retrying;
            // reloadEngines() stops on its own once the connection drops.
            QTimer::singleShot(100, this, [this] { reloadEngines(); });
            return;
        }
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
                removeStaleObjectWatches();
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
                validateParentlessObjects();
                m_rootContextQueryIds.clear();
                m_lastRebuild.start();
                if (std::exchange(m_rebuildPending, false))
                    scheduleRebuild();
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
        m_knownDelegateIds.insert(objectId, engineId);
    }

    const bool knownEngine = Utils::anyOf(m_engines, [engineId](const EngineReference &engine) {
        return engine.debugId() == engineId;
    });
    if (!knownEngine)
        return;

    // Insert objects that would be visible right away. The full rebuild is
    // still needed to get rid of destroyed objects, but it is expensive for
    // large trees, so do not run it more often than once a second.
    if (!m_debugIdToIname.contains(objectId)) {
        const auto parentIt = m_debugIdToIname.constFind(parentId);
        if (parentId == WatchItem::InvalidId) {
            if (insertParentlessItem(engineId)
                    && m_qmlEngine->watchHandler()->isExpandedIName(parentlessIName(engineId))) {
                fetchObjectHeader(objectId);
            }
        } else if (parentIt != m_debugIdToIname.constEnd()
                   && m_qmlEngine->watchHandler()->isExpandedIName(*parentIt)) {
            fetchObject(objectId);
        }
    }

    scheduleRebuild();
}

void QmlInspectorAgent::scheduleRebuild()
{
    if (m_delayQueryTimer.isActive())
        return;
    const milliseconds sinceLastRebuild = m_lastRebuild.isValid()
            ? milliseconds(m_lastRebuild.elapsed()) : RebuildInterval;
    m_delayQueryTimer.start(std::max(MinRebuildDelay, RebuildInterval - sinceLastRebuild));
}

// A new LIST_OBJECTS drops the answers to the previous one, so a tree that takes
// longer than the throttle interval to answer would never be rebuilt.
void QmlInspectorAgent::rebuildObjectTree()
{
    if (m_rootContextQueryIds.isEmpty())
        queryEngineContext();
    else
        m_rebuildPending = true;
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
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << debugId << ')' << propertyName
                             << value.toString();
    if (!m_debugIdToIname.contains(debugId))
        return;
    m_valueChanges[debugId].insert(QString::fromLatin1(propertyName), value);
    if (!m_valueChangeTimer.isActive())
        m_valueChangeTimer.start();
}

// Property updates can arrive at a high rate and the inspector tree can be
// huge, so apply them in batches and look up only the changed objects.
void QmlInspectorAgent::applyValueChanges()
{
    if (!m_qmlEngine)
        return;
    WatchHandler *handler = m_qmlEngine->watchHandler();
    for (auto it = m_valueChanges.cbegin(), end = m_valueChanges.cend(); it != end; ++it) {
        // Collapsed objects are watched, but get their values with the next fetch.
        const QString iname = m_debugIdToIname.value(it.key()) + ".[properties]";
        WatchItem *properties = handler->findItem(iname);
        if (!properties)
            continue;
        for (WatchItem *item : *properties) {
            const auto valueIt = it->constFind(item->name);
            if (valueIt == it->constEnd())
                continue;
            item->value = valueIt->toString();
            item->updateValueCache();
            item->removeChildren();
            item->wantsChildren = insertChildren(item, *valueIt);
            item->update();
        }
    }
    m_valueChanges.clear();
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

    m_rebuildPending = false;
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

void QmlInspectorAgent::fetchObjectHeader(int debugId)
{
    if (!isConnected() || !settings().showQmlObjectTree())
        return;

    log(LogSend, "FETCH_OBJECT " + QString::number(debugId));
    if (const quint32 queryId = m_engineClient->queryObjectHeader(debugId))
        m_delegateQueryIds.insert(queryId, debugId);
}

QString QmlInspectorAgent::parentlessIName(int engineId) const
{
    return buildIName(m_debugIdToIname.value(engineId), "[parentless]");
}

// The item below the engine item that holds the objects without a parent.
bool QmlInspectorAgent::insertParentlessItem(int engineId)
{
    WatchHandler *handler = m_qmlEngine->watchHandler();
    const auto engineIt = m_debugIdToIname.constFind(engineId);
    if (engineIt == m_debugIdToIname.constEnd() || !handler->findItem(*engineIt))
        return false;
    const QString iname = parentlessIName(engineId);
    if (handler->findItem(iname))
        return true;
    auto item = new WatchItem;
    item->iname = iname;
    item->name = Core::Tr::tr("Other");
    item->value = "list";
    item->wantsChildren = true;
    handler->appendNewItem(item);
    if (!m_updateLocalsTimer.isActive())
        m_updateLocalsTimer.start();
    return true;
}

bool QmlInspectorAgent::hasParentlessObjects(int engineId) const
{
    return m_knownDelegateIds.key(engineId, WatchItem::InvalidId) != WatchItem::InvalidId;
}

// Fetches the objects that are neither shown nor on their way.
void QmlInspectorAgent::fetchParentlessObjects(int engineId)
{
    const WatchHandler *handler = m_qmlEngine->watchHandler();
    const QSet<int> pending(m_delegateQueryIds.cbegin(), m_delegateQueryIds.cend());
    for (auto it = m_knownDelegateIds.cbegin(); it != m_knownDelegateIds.cend(); ++it) {
        if (*it != engineId || pending.contains(it.key()))
            continue;
        const auto inameIt = m_debugIdToIname.constFind(it.key());
        if (inameIt == m_debugIdToIname.cend() || !handler->findItem(*inameIt))
            fetchObjectHeader(it.key());
    }
}

// The protocol does not report destroyed objects, only the answer to a query
// for one tells. Check a bounded batch per rebuild of the objects whose item
// nobody expands.
void QmlInspectorAgent::validateParentlessObjects()
{
    constexpr int BatchSize = 256;
    const WatchHandler *handler = m_qmlEngine->watchHandler();
    const QSet<int> pending(m_delegateQueryIds.cbegin(), m_delegateQueryIds.cend());
    QList<int> candidates;
    for (auto it = m_knownDelegateIds.cbegin(); it != m_knownDelegateIds.cend(); ++it) {
        if (it.key() > m_validatedDelegateId && !pending.contains(it.key())
                && !handler->isExpandedIName(parentlessIName(*it))) {
            candidates.append(it.key());
        }
    }
    const int count = int(std::min<qsizetype>(candidates.size(), BatchSize));
    std::partial_sort(candidates.begin(), candidates.begin() + count, candidates.end());
    for (int i = 0; i < count; ++i)
        fetchObjectHeader(candidates.at(i));
    m_validatedDelegateId = count < BatchSize ? -1 : candidates.at(count - 1);
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

        for (const ObjectReference &obj : ctx.objects()) {
            // Root objects have no parent either, but get their place from here.
            m_knownDelegateIds.remove(obj.debugId());
            verifyAndInsertObjectInTree(obj, engineId, true);
        }

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

    if (parentId == WatchItem::InvalidId && !calledFromUpdateObjectTree) {
        if (const auto it = m_knownDelegateIds.constFind(objectDebugId);
                it != m_knownDelegateIds.cend()) {
            insertParentlessObject(object, *it);
            continueObjectStack(object);
            return;
        }
    }

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
        // The engine item is filled when it is inserted. Fetching more of it would
        // only expand it again after the user collapsed it.
        if (parentId != WatchItem::InvalidId && parentId != engineId
                && !handler->isExpandedIName(parentIname)
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
        insertObjectInTree(object, parentIname);
        // After inserting the engine node, populate its children from the root context.
        // Guard against the case where we are already inside updateObjectTree for this
        // engine: that function calls us for each object in the context, so if one of
        // those objects is the engine itself, calling updateObjectTree here would restart
        // the same traversal and recurse infinitely.
        if (objectDebugId == engineId && !calledFromUpdateObjectTree)
            updateObjectTree(m_rootContexts[engineId], engineId);
        // Inserting the engine item drops the old one with all its children.
        if (objectDebugId == engineId && hasParentlessObjects(engineId))
            insertParentlessItem(engineId);
    } else {
        m_objectStack.push(QPair<ObjectReference, int>(object, engineId));
        fetchObject(parentId);
        return; // recursive
    }
    continueObjectStack(object);
}

void QmlInspectorAgent::continueObjectStack(const ObjectReference &object)
{
    WatchHandler *handler = m_qmlEngine->watchHandler();
    const int objectDebugId = object.debugId();
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

// Expanding the item of the objects without a parent fetches all of them, so
// showing one of them, e.g. for a selection in the application, expands it.
void QmlInspectorAgent::insertParentlessObject(const ObjectReference &object, int engineId)
{
    if (!insertParentlessItem(engineId))
        return;
    const QString parentIname = parentlessIName(engineId);
    if (!m_qmlEngine->watchHandler()->isExpandedIName(parentIname))
        m_qmlEngine->watchHandler()->fetchMore(parentIname);
    insertObjectInTree(object, parentIname);
}

void QmlInspectorAgent::insertObjectInTree(const ObjectReference &object,
                                           const QString &parentIname)
{
    qCDebug(qmlInspectorLog) << __FUNCTION__ << '(' << object << ')';

    QElapsedTimer timeElapsed;

    bool printTime = qmlInspectorLog().isDebugEnabled();
    if (printTime)
        timeElapsed.start();
    addWatchData(object, parentIname, true);
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
    if (!m_updateLocalsTimer.isActive())
        m_updateLocalsTimer.start();
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
    // All items are registered, so there is nothing to replace unless the object
    // is registered with this iname.
    const bool isNew = m_debugIdToIname.value(objDebugId) != objIname;
    WatchHandler *handler = m_qmlEngine->watchHandler();
    const auto insert = [handler, isNew](WatchItem *item) {
        if (isNew)
            handler->appendNewItem(item);
        else
            handler->insertItem(item);
    };

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

        insert(objWatch);
        addObjectWatch(objDebugId);
        // Pending changes are older than the values fetched with obj.
        m_valueChanges.remove(objDebugId);
        if (m_debugIdToIname.contains(objDebugId)) {
            // The data needs to be removed since we now know the parent and
            // hence we can insert the data in the correct position
            const QString oldIname = m_debugIdToIname.value(objDebugId);
            if (oldIname != objIname)
                m_qmlEngine->watchHandler()->removeItemByIName(oldIname);
        }
        m_debugIdToIname.insert(objDebugId, objIname);
    }

    if (obj.needsMoreData()) {
        // we don't know the children yet. Not adding the 'properties'
        // element makes sure we're queried on expansion.
        if (!m_qmlEngine->watchHandler()->isExpandedIName(objIname))
            return;
        // Nothing expands an object again that was expanded before a rebuild,
        // or whose fetched data the answer for its engine just replaced.
        if (append) {
            if (!m_fetchDataIds.contains(objDebugId))
                m_fetchDataIds << objDebugId;
            fetchObject(objDebugId);
        }
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
        insert(propertiesWatch);
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
    m_delegateQueryIds.clear();
    m_valueChanges.clear();
    m_fetchDataIds.clear();
    m_debugIdToIname.clear();
    m_debugIdToIname.insert(WatchItem::InvalidId, "inspect");
    m_objectStack.clear();
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
