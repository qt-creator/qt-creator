// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlimpl.h"

#include "qmlv8debuggerclientconstants.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../debuggertr.h"

#include <qmldebug/qpacketprotocol.h>

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>

#include <memory>
#include <utility>

using namespace Utils;

namespace Debugger::Internal {

static GdbMi constMi(const QString &name, const QString &data)
{
    GdbMi mi;
    mi.m_type = GdbMi::Const;
    mi.m_name = name;
    mi.m_data = data;
    return mi;
}

static DebuggerEngineSetupData qmlImplSetupData()
{
    DebuggerEngineSetupData data;
    data.capabilities = AddWatcherCapability
                      | AddWatcherWhileRunningCapability
                      | RunToLineCapability
                      | WatchComplexExpressionsCapability;
    data.extraCapabilities = DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::SourceFiles;
    data.startModes = DebuggerStartModeFlag::AttachToQmlServer;
    data.toolTipHandling = ToolTipHandling::Always;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        if (query.type == BreakpointOnQmlSignalEmit || query.type == BreakpointAtJavaScriptThrow)
            return true;
        return query.isQmlFileAndLineBreakpoint();
    };
    return data;
}

class QmlImpl::V8Client : public QmlDebug::QmlDebugClient
{
public:
    V8Client(QmlImpl *owner, QmlDebug::QmlDebugConnection *connection)
        : QmlDebug::QmlDebugClient(QLatin1String("V8Debugger"), connection)
        , m_owner(owner)
    {}

    void stateChanged(State state) override { m_owner->handleStateChanged(state); }
    void messageReceived(const QByteArray &data) override { m_owner->handleMessageReceived(data); }

private:
    QmlImpl *m_owner;
};

QmlImpl::QmlImpl(const QmlImplStartData &startData)
    : DebuggerEngineInterface(qmlImplSetupData())
    , m_startData(startData)
    , m_connection(this)
{
    m_v8Client = new V8Client(this, &m_connection);
    m_engineClient = new QmlDebug::QmlEngineDebugClient(&m_connection);
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::result,
            this, [this](quint32 queryId, const QVariant &value, const QByteArray &type) {
        if (const InspectorCallback cb = m_inspectorCallbackForQueryId.take(queryId))
            cb(value, type);
    });
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::newObject,
            this, &QmlImpl::handleObjectCreated);
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::valueChanged,
            this, &QmlImpl::handlePropertyValueChanged);
    m_objectCreatedTimer = new QTimer(this);
    m_objectCreatedTimer->setInterval(100);
    m_objectCreatedTimer->setSingleShot(true);
    connect(m_objectCreatedTimer, &QTimer::timeout, this, &QmlImpl::rebuildInspectorTree);

    connect(&m_connection, &QmlDebug::QmlDebugConnection::connectionFailed, this, [this] {
        if (m_connectRetriesLeft > 0) {
            --m_connectRetriesLeft;
            QTimer::singleShot(100, this, [this] { beginConnection(); });
            return;
        }
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
    });
    connect(&m_connection, &QmlDebug::QmlDebugConnection::disconnected, this, [this] {
        if (m_shuttingDown)
            return;
        emit inferiorEvent(InferiorEvent::EngineIll);
    });
}

QmlImpl::~QmlImpl()
{
    m_shuttingDown = true;
}

void QmlImpl::start()
{
    beginConnection();
}

void QmlImpl::beginConnection()
{
    const auto &qmlData = std::get<AttachToQmlServerData>(m_startData.inferiorStartData);
    m_connection.connectToHost(qmlData.server.host(), quint16(qmlData.server.port()));
}

void QmlImpl::shutdownInferior(ShutdownMode)
{
    emit inferiorEvent(InferiorEvent::ShutdownFinished);
}

void QmlImpl::shutdownEngine()
{
    m_shuttingDown = true;
    m_connection.close();
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void QmlImpl::handleStateChanged(QmlDebug::QmlDebugClient::State state)
{
    if (state != QmlDebug::QmlDebugClient::Enabled)
        return;
    QTimer::singleShot(0, this, [this] { handleConnectHandshakeDone(); });
}

void QmlImpl::handleConnectHandshakeDone()
{
    QJsonObject parameters;
    parameters.insert(QLatin1String("redundantRefs"), false);
    parameters.insert(QLatin1String("namesAsObjects"), false);
    runDirectCommand(CONNECT, QJsonDocument(parameters).toJson());

    runCommand({VERSION}, [this](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        m_supportChangeBreakpoint = body.value("ChangeBreakpoint", false).toBool();
    });

    emit inferiorEvent(InferiorEvent::EngineSetupOk);
    m_inferiorRunning = true;
    emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
}

void QmlImpl::runDirectCommand(const QByteArray &type, const QByteArray &msg)
{
    emit message(QString("%1 %2").arg(QString::fromLatin1(type), QString::fromUtf8(msg)), LogInput);
    QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
    rs << QByteArray(V8DEBUG) << type << msg;
    m_v8Client->sendMessage(rs.data());
}

int QmlImpl::runCommand(const DebuggerCommand &command, const QmlCallback &cb)
{
    ++m_sequence;
    QJsonObject object;
    object.insert(QLatin1String(SEQ), m_sequence);
    object.insert(QLatin1String(TYPE), QLatin1String(REQUEST));
    object.insert(QLatin1String(COMMAND), command.function);
    object.insert(QLatin1String(ARGUMENTS), command.args);
    if (cb)
        m_callbackForToken[m_sequence] = cb;
    runDirectCommand(V8REQUEST, QJsonDocument(object).toJson(QJsonDocument::Compact));
    return m_sequence;
}

void QmlImpl::handleMessageReceived(const QByteArray &data)
{
    QmlDebug::QPacket ds(m_v8Client->dataStreamVersion(), data);
    QByteArray command;
    ds >> command;
    if (command != V8DEBUG)
        return;
    QByteArray type;
    QByteArray payload;
    ds >> type >> payload;
    emit message(QString("%1 %2").arg(QString::fromLatin1(type), QString::fromUtf8(payload)),
                  LogOutput);
    if (type == V8MESSAGE)
        handleV8Message(payload);
}

void QmlImpl::handleV8Message(const QByteArray &payload)
{
    const QVariantMap resp = QJsonDocument::fromJson(payload).toVariant().toMap();
    const QString type = resp.value(QLatin1String(TYPE)).toString();
    if (type == QLatin1String("response")) {
        const int requestSeq = resp.value(QLatin1String("request_seq")).toInt();
        const QmlCallback cb = m_callbackForToken.take(requestSeq);
        if (cb)
            cb(resp);
    } else if (type == QLatin1String("event")) {
        const QString event = resp.value(QLatin1String("event")).toString();
        if (event == QLatin1String("break"))
            handleBreakEvent(resp);
        else if (event == QLatin1String("exception"))
            handleExceptionEvent(resp);
    }
}

void QmlImpl::handleBreakEvent(const QVariantMap &response)
{
    m_inferiorRunning = false;
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();
    const QVariantMap script = body.value(QLatin1String("script")).toMap();
    const QString scriptName = script.value(QLatin1String(NAME)).toString();
    const int lineNumber = body.value(QLatin1String("sourceLine")).toInt() + 1;
    if (!scriptName.isEmpty())
        emit locationChanged(FilePath::fromUrl(QUrl(scriptName)), lineNumber);
    emit inferiorEvent(std::exchange(m_interruptRequested, false)
                       ? InferiorEvent::StopOk : InferiorEvent::SpontaneousStop);

    if (m_deferredWatchers) {
        const RefreshRequest deferred = *std::exchange(m_deferredWatchers, std::nullopt);
        refreshLocals(deferred);
    }
}

void QmlImpl::handleExceptionEvent(const QVariantMap &response)
{
    m_inferiorRunning = false;
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();

    const QVariantMap script = body.value(QLatin1String("script")).toMap();
    const QString scriptName = script.value(QLatin1String(NAME)).toString();
    const int lineNumber = body.value(QLatin1String("sourceLine")).toInt() + 1;
    if (!scriptName.isEmpty())
        emit locationChanged(FilePath::fromUrl(QUrl(scriptName)), lineNumber);

    const QVariantMap exception = body.value(QLatin1String("exception")).toMap();
    const QString text = exception.value(QLatin1String("text")).toString();
    if (!text.isEmpty())
        emit message(text, ConsoleOutput);

    emit inferiorEvent(std::exchange(m_interruptRequested, false)
                       ? InferiorEvent::StopOk : InferiorEvent::SpontaneousStop);
}

void QmlImpl::setScriptBreakpoint(quint64 requestId, const BreakpointChangeRequest &request)
{
    const BreakpointParameters &params = request.params;
    DebuggerCommand cmd(SETBREAKPOINT);
    cmd.arg(TYPE, SCRIPTREGEXP);
    cmd.arg(TARGET, params.fileName.toUrlishString());
    cmd.arg(ENABLED, params.enabled);
    cmd.arg(LINE, params.textPosition.line - 1);
    if (!params.condition.isEmpty())
        cmd.arg(CONDITION, params.condition);

    runCommand(cmd, [this, requestId, params, request](const QVariantMap &resp) {
        const bool success = resp.value(QLatin1String(SUCCESS)).toBool();
        if (!success) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, false, {});
            return;
        }
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QString responseId = QString::number(body.value(QLatin1String(BREAKPOINT)).toInt());
        BreakpointChangeRequest active = request;
        active.responseId = responseId;
        m_activeBreakpointsByResponseId[responseId] = active;

        int line = params.textPosition.line;
        const QVariantList actualLocations = body.value(QLatin1String("actual_locations")).toList();
        if (!actualLocations.isEmpty())
            line = actualLocations.constFirst().toMap().value(QLatin1String(LINE)).toInt() + 1;

        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi(QLatin1String(NUMBER), responseId));
        bkpt.addChild(constMi(QLatin1String("file"), params.fileName.toUserOutput()));
        bkpt.addChild(constMi(QLatin1String("line"), QString::number(line)));
        bkpt.addChild(constMi(QLatin1String(ENABLED), params.enabled ? QStringLiteral("y")
                                                                     : QStringLiteral("n")));
        GdbMi data;
        data.m_type = GdbMi::List;
        data.addChild(bkpt);
        emit breakpointEvent(requestId, BreakpointOp::Insert, true, data);
    });
}

bool QmlImpl::isEnabledOnlyChange(const BreakpointChangeRequest &request) const
{
    const auto it = m_activeBreakpointsByResponseId.constFind(request.responseId);
    if (it == m_activeBreakpointsByResponseId.constEnd())
        return false;
    const BreakpointParameters &was = it->params;
    const BreakpointParameters &now = request.params;
    return was.enabled != now.enabled
           && was.fileName == now.fileName
           && was.textPosition == now.textPosition
           && was.condition == now.condition
           && was.ignoreCount == now.ignoreCount
           && was.command == now.command;
}

void QmlImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const BreakpointParameters &params = request.params;
    switch (request.op) {
    case BreakpointOp::Insert:
        if (params.type == BreakpointAtJavaScriptThrow) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            if (params.enabled)
                cmd.arg(ENABLED, params.enabled);
            runCommand(cmd);
            emit breakpointEvent(request.requestId, BreakpointOp::Insert, true, {});
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << params.enabled;
            runDirectCommand(BREAKONSIGNAL, rs.data());
            emit breakpointEvent(request.requestId, BreakpointOp::Insert, true, {});
        } else {
            setScriptBreakpoint(request.requestId, request);
        }
        break;
    case BreakpointOp::Remove:
        if (params.type == BreakpointAtJavaScriptThrow) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            runCommand(cmd);
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << false;
            runDirectCommand(BREAKONSIGNAL, rs.data());
        } else {
            DebuggerCommand cmd(CLEARBREAKPOINT);
            cmd.arg(BREAKPOINT, request.responseId.toInt());
            runCommand(cmd);
            m_activeBreakpointsByResponseId.remove(request.responseId);
        }
        emit breakpointEvent(request.requestId, BreakpointOp::Remove, true, {});
        break;
    case BreakpointOp::Update:
        if (params.type == BreakpointAtJavaScriptThrow) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            if (params.enabled)
                cmd.arg(ENABLED, params.enabled);
            runCommand(cmd);
            emit breakpointEvent(request.requestId, BreakpointOp::Update, true, {});
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << params.enabled;
            runDirectCommand(BREAKONSIGNAL, rs.data());
            emit breakpointEvent(request.requestId, BreakpointOp::Update, true, {});
        } else if (m_supportChangeBreakpoint && isEnabledOnlyChange(request)) {
            DebuggerCommand cmd(CHANGEBREAKPOINT);
            cmd.arg(BREAKPOINT, request.responseId.toInt());
            cmd.arg(ENABLED, params.enabled);
            const quint64 requestId = request.requestId;
            runCommand(cmd, [this, requestId, request](const QVariantMap &resp) {
                const bool ok = resp.value(QLatin1String(SUCCESS)).toBool();
                if (ok)
                    m_activeBreakpointsByResponseId.insert(request.responseId, request);
                emit breakpointEvent(requestId, BreakpointOp::Update, ok, {});
            });
        } else {
            DebuggerCommand clearCmd(CLEARBREAKPOINT);
            clearCmd.arg(BREAKPOINT, request.responseId.toInt());
            runCommand(clearCmd);
            m_activeBreakpointsByResponseId.remove(request.responseId);
            setScriptBreakpoint(request.requestId, request);
        }
        break;
    case BreakpointOp::EnableSub:
        break;
    }
}

void QmlImpl::execute(const ExecutionRequest &request)
{
    switch (request.command) {
    case ExecutionCommand::Continue:
        emit inferiorEvent(InferiorEvent::RunRequested);
        m_inferiorRunning = true;
        runCommand({CONTINEDEBUGGING}, [this](const QVariantMap &) {
            emit inferiorEvent(InferiorEvent::RunOk);
        });
        break;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        m_interruptRequested = true;
        runDirectCommand(INTERRUPT);
        break;
    case ExecutionCommand::StepIn:
    case ExecutionCommand::StepOver:
    case ExecutionCommand::StepOut: {
        DebuggerCommand cmd(CONTINEDEBUGGING);
        cmd.arg(STEPACTION, request.command == ExecutionCommand::StepIn ? IN
                           : request.command == ExecutionCommand::StepOut ? OUT : NEXT);
        emit inferiorEvent(InferiorEvent::RunRequested);
        m_inferiorRunning = true;
        runCommand(cmd, [this](const QVariantMap &) {
            emit inferiorEvent(InferiorEvent::RunOk);
        });
        break;
    }
    case ExecutionCommand::RunToLine: {
        DebuggerCommand cmd(SETBREAKPOINT);
        cmd.arg(TYPE, SCRIPTREGEXP);
        cmd.arg(TARGET, request.context.fileName.toUrlishString());
        cmd.arg(ENABLED, true);
        cmd.arg(LINE, request.context.textPosition.line - 1);
        runCommand(cmd);

        emit inferiorEvent(InferiorEvent::RunRequested);
        m_inferiorRunning = true;
        runCommand({CONTINEDEBUGGING}, [this](const QVariantMap &) {
            emit inferiorEvent(InferiorEvent::RunOk);
        });
        break;
    }
    case ExecutionCommand::Detach:
        emit inferiorDone({0, InferiorExitStatus::Detached});
        break;
    default:
        break;
    }
}

static std::pair<QString, QString> v8TypeAndValue(const QVariantMap &data)
{
    const QString type = data.value(QLatin1String(TYPE)).toString();
    const QVariant value = data.value(QLatin1String(VALUE));
    if (type == "undefined")
        return {"undefined", "undefined"};
    if (type == "null")
        return {"object", "null"};
    if (type == "string")
        return {type, '"' + value.toString() + '"'};
    if (type == "boolean" || type == "number")
        return {type, value.toString()};
    if (type == "function")
        return {type, data.value(QLatin1String(NAME)).toString()};
    if (type == "object") {
        if (data.contains(QLatin1String(VALUE)) && (!value.isValid() || value.isNull()))
            return {type, "null"};
        return {type, "{...}"};
    }
    return {type, value.toString()};
}

std::shared_ptr<QmlImpl::RefreshCollector> QmlImpl::makeCollector(const RefreshRequest &request)
{
    const auto pending = std::make_shared<RefreshCollector>();
    pending->requestId = request.requestId;
    pending->kind = request.kind;
    pending->items.m_type = GdbMi::List;
    pending->items.m_name = QStringLiteral("data");
    pending->expandedINames = request.expandedINames;
    return pending;
}

std::function<void()> QmlImpl::legFinisher(const std::shared_ptr<RefreshCollector> &pending)
{
    return [this, pending] {
        if (--pending->remaining > 0)
            return;
        GdbMi all;
        all.m_type = GdbMi::Tuple;
        all.addChild(pending->items);
        emit refreshDataReceived(pending->requestId, pending->kind, all);
    };
}

void QmlImpl::refreshLocals(const RefreshRequest &request)
{
    const QJsonArray watchers = request.watchers;

    if (m_inferiorRunning) {
        m_deferredWatchers = request;
        return;
    }

    const auto pending = makeCollector(request);
    pending->remaining = 1 + int(watchers.size());
    const auto finishLeg = legFinisher(pending);

    for (const QJsonValue &watcherValue : watchers) {
        const QJsonObject watcher = watcherValue.toObject();
        const QString iname = watcher.value("iname").toString();
        const QString hexExp = watcher.value("exp").toString();
        const QString exp = QString::fromUtf8(QByteArray::fromHex(hexExp.toLatin1()));

        DebuggerCommand cmd(EVALUATE);
        cmd.arg(EXPRESSION, exp);
        cmd.arg(FRAME, m_currentFrameIndex);
        runCommand(cmd, [iname, hexExp, pending, finishLeg](const QVariantMap &resp) {
            const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
            GdbMi item;
            item.m_type = GdbMi::Tuple;
            item.addChild(constMi(QStringLiteral("iname"), iname));
            item.addChild(constMi(QStringLiteral("wname"), hexExp));
            if (resp.value(QLatin1String(SUCCESS)).toBool()) {
                const auto [type, value] = v8TypeAndValue(body);
                item.addChild(constMi(QStringLiteral("type"), type));
                item.addChild(constMi(QStringLiteral("value"), value));
            } else {
                item.addChild(constMi(QStringLiteral("value"),
                                      body.value(QLatin1String("text")).toString()));
            }
            item.addChild(constMi(QStringLiteral("numchild"), QStringLiteral("0")));
            pending->items.addChild(item);
            finishLeg();
        });
    }

    DebuggerCommand frameCmd(FRAME);
    frameCmd.arg(NUMBER, m_currentFrameIndex);
    runCommand(frameCmd, [this, pending, finishLeg](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();

        const QVariantMap receiver = body.value(QLatin1String("receiver")).toMap();
        const auto [thisType, thisValue] = v8TypeAndValue(receiver);
        GdbMi thisItem;
        thisItem.m_type = GdbMi::Tuple;
        thisItem.addChild(constMi(QStringLiteral("iname"), QStringLiteral("local.this")));
        thisItem.addChild(constMi(QStringLiteral("name"), QStringLiteral("this")));
        thisItem.addChild(constMi(QStringLiteral("type"), thisType));
        thisItem.addChild(constMi(QStringLiteral("value"), thisValue));
        thisItem.addChild(constMi(QStringLiteral("numchild"), QStringLiteral("0")));
        pending->items.addChild(thisItem);

        const QVariantList scopes = body.value(QLatin1String("scopes")).toList();
        for (const QVariant &scopeValue : scopes) {
            const QVariantMap scope = scopeValue.toMap();
            if (scope.value(QLatin1String(TYPE)).toInt() == 0)
                continue;
            ++pending->remaining;
            DebuggerCommand scopeCmd(SCOPE);
            scopeCmd.arg(NUMBER, scope.value(QLatin1String("index")).toInt());
            scopeCmd.arg(FRAMENUMBER, m_currentFrameIndex);
            runCommand(scopeCmd, [this, pending, finishLeg](const QVariantMap &scopeResp) {
                handleScopeReply(scopeResp, pending, finishLeg);
                finishLeg();
            });
        }
        finishLeg();
    });
}

static int v8ChildCount(const QVariantMap &data)
{
    const QString type = data.value(QLatin1String(TYPE)).toString();
    if (type != "object" && type != "function")
        return 0;
    const QVariantList properties = data.value(QLatin1String("properties")).toList();
    if (!properties.isEmpty())
        return int(properties.size());
    const QVariant value = data.value(QLatin1String(VALUE));
    if (!value.isValid() || value.isNull())
        return 0;
    return value.toInt();
}

static GdbMi watchItem(const QString &iname, const QString &name, const QString &exp,
                       const QString &type, const QString &value, int numchild,
                       int debugId = -1)
{
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addChild(constMi(QStringLiteral("iname"), iname));
    item.addChild(constMi(QStringLiteral("name"), name));
    if (debugId != -1)
        item.addChild(constMi(QStringLiteral("id"), QString::number(debugId)));
    if (!exp.isEmpty())
        item.addChild(constMi(QStringLiteral("exp"), exp));
    if (!type.isEmpty())
        item.addChild(constMi(QStringLiteral("type"), type));
    item.addChild(constMi(QStringLiteral("value"), value));
    item.addChild(constMi(QStringLiteral("numchild"), QString::number(numchild)));
    return item;
}

void QmlImpl::handleScopeReply(const QVariantMap &response,
                               const std::shared_ptr<RefreshCollector> &pending,
                               const std::function<void()> &finishLeg)
{
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();
    const QVariantMap object = body.value(QLatin1String("object")).toMap();

    QList<LookupRequest> lookups;
    for (const QVariant &propertyValue : object.value(QLatin1String("properties")).toList()) {
        const QVariantMap property = propertyValue.toMap();
        const QString name = property.value(QLatin1String(NAME)).toString();
        if (name.isEmpty() || name.startsWith('.'))
            continue;

        const QString iname = "local." + name;
        const int numchild = v8ChildCount(property);
        const auto [type, value] = v8TypeAndValue(property);
        const int handle = property.value(QLatin1String(REF),
                                         property.value(QLatin1String(HANDLE))).toInt();

        if (numchild > 0) {
            pending->items.addChild(watchItem(iname, name, name, type, value, numchild));
            if (handle != 0 && pending->expandedINames.contains(iname))
                lookups.append({handle, iname, name, name});
            continue;
        }
        if (!property.contains(QLatin1String(VALUE)) && handle != 0) {
            lookups.append({handle, iname, name, name});
            continue;
        }
        pending->items.addChild(watchItem(iname, name, name, type, value, 0));
    }

    lookupHandles(lookups, pending, finishLeg);
}

void QmlImpl::lookupHandles(const QList<LookupRequest> &requests,
                            const std::shared_ptr<RefreshCollector> &pending,
                            const std::function<void()> &finishLeg)
{
    if (requests.isEmpty())
        return;

    QHash<int, LookupRequest> requestForHandle;
    QList<int> handles;
    for (const LookupRequest &request : requests) {
        if (requestForHandle.contains(request.handle))
            continue;
        requestForHandle.insert(request.handle, request);
        handles.append(request.handle);
    }

    ++pending->remaining;
    DebuggerCommand cmd(LOOKUP);
    cmd.arg(HANDLES, handles);
    runCommand(cmd, [this, pending, requestForHandle, finishLeg](const QVariantMap &lookupResp) {
        const QVariantMap body = lookupResp.value(QLatin1String(BODY)).toMap();
        QList<LookupRequest> nextRound;
        for (auto it = body.begin(), end = body.end(); it != end; ++it) {
            const auto requestIt = requestForHandle.constFind(it.key().toInt());
            if (requestIt == requestForHandle.constEnd())
                continue;
            const QString iname = requestIt->iname;
            const QVariantMap resolved = it.value().toMap();
            const auto [type, value] = v8TypeAndValue(resolved);
            const QVariantList properties = resolved.value(QLatin1String("properties")).toList();

            const int numchild = properties.isEmpty() ? v8ChildCount(resolved)
                                                      : int(properties.size());
            pending->items.addChild(watchItem(iname, requestIt->name, requestIt->exp, type, value,
                                              numchild));

            for (const QVariant &childValue : properties) {
                const QVariantMap child = childValue.toMap();
                const QString childName = child.value(QLatin1String(NAME)).toString();
                if (childName.isEmpty() || childName.startsWith('.'))
                    continue;
                const QString childExp = value == "Array"
                        ? QString(requestIt->exp + '[' + childName + ']')
                        : QString(requestIt->exp + '.' + childName);
                const QString childIName = iname + '.' + childName;
                const auto [childType, childText] = v8TypeAndValue(child);
                const int childNumChild = v8ChildCount(child);
                pending->items.addChild(watchItem(childIName, childName, childExp, childType,
                                                  childText, childNumChild));

                const int childHandle = child.value(QLatin1String(REF),
                                                    child.value(QLatin1String(HANDLE))).toInt();
                const bool childExpanded = childNumChild > 0
                                           && pending->expandedINames.contains(childIName);
                if (childHandle != 0 && (childType.isEmpty() || childExpanded))
                    nextRound.append({childHandle, childIName, childName, childExp});
            }
        }
        lookupHandles(nextRound, pending, finishLeg);
        finishLeg();
    });
}

GdbMi QmlImpl::emptyLocalsData()
{
    GdbMi items;
    items.m_type = GdbMi::List;
    items.m_name = QStringLiteral("data");
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(items);
    return all;
}

void QmlImpl::refreshSourceFiles(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    DebuggerCommand cmd(SCRIPTS);
    cmd.arg(TYPES, 4);
    runCommand(cmd, [this, requestId](const QVariantMap &resp) {
        GdbMi files;
        files.m_type = GdbMi::List;
        const QVariantList scripts = resp.value(QLatin1String(BODY)).toList();
        for (const QVariant &scriptValue : scripts) {
            const QString name = scriptValue.toMap().value(QLatin1String(NAME)).toString();
            if (name.isEmpty())
                continue;
            GdbMi entry;
            entry.m_type = GdbMi::Tuple;
            entry.addChild(constMi(QStringLiteral("file"), name));
            files.addChild(entry);
        }
        emit refreshDataReceived(requestId, RefreshKind::SourceFiles, files);
    });
}

static const int MaxInspectorTreeNodes = 100000;

static int valueChildCount(const QVariant &value)
{
    if (value.typeId() == QMetaType::QVariantMap)
        return int(value.toMap().size());
    if (value.typeId() == QMetaType::QVariantList)
        return int(value.toList().size());
    return 0;
}

static void appendValueChildren(const QString &parentIname, const QVariant &value,
                                int debugId, GdbMi *items)
{
    const auto appendOne = [&](const QString &name, const QVariant &childValue) {
        const QString iname = parentIname + '.' + name;
        GdbMi child = watchItem(iname, name, {}, QLatin1String(childValue.typeName()),
                                childValue.toString(), valueChildCount(childValue), debugId);
        child.addChild(constMi(QStringLiteral("valueeditable"), QStringLiteral("false")));
        items->addChild(child);
        appendValueChildren(iname, childValue, debugId, items);
    };

    if (value.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        for (auto it = map.begin(), end = map.end(); it != end; ++it)
            appendOne(it.key(), it.value());
    } else if (value.typeId() == QMetaType::QVariantList) {
        const QVariantList list = value.toList();
        for (int i = 0, end = int(list.size()); i != end; ++i)
            appendOne(QString::number(i), list.at(i));
    }
}

bool QmlImpl::runInspectorQuery(quint32 queryId, const InspectorCallback &cb)
{
    if (queryId == 0)
        return false;
    m_inspectorCallbackForQueryId.insert(queryId, cb);
    return true;
}

// A query the client refused to send never answers, so the leg counted on it here
// has to be finished right away.
void QmlImpl::runInspectorLeg(quint32 queryId, const std::shared_ptr<RefreshCollector> &pending,
                              const std::function<void()> &finishLeg, const InspectorCallback &cb)
{
    ++pending->remaining;
    if (!runInspectorQuery(queryId, cb))
        finishLeg();
}

void QmlImpl::addObjectWatch(int debugId)
{
    if (debugId == -1 || m_objectWatches.contains(debugId))
        return;
    if (m_engineClient->addWatch(debugId))
        m_objectWatches.append(debugId);
}

void QmlImpl::appendObjectItems(const QmlDebug::ObjectReference &object,
                                const QString &parentIname, int engineId,
                                const std::shared_ptr<RefreshCollector> &pending,
                                const std::function<void()> &finishLeg)
{
    const int debugId = object.debugId();
    if (!object.isValid())
        return;
    if (pending->items.childCount() > MaxInspectorTreeNodes)
        return;

    const QString iname = parentIname + '.' + QString::number(debugId);
    m_inameForDebugId.insert(debugId, iname);
    m_engineIdForDebugId.insert(debugId, engineId);
    pending->seenDebugIds.insert(debugId);

    QString name = object.idString();
    if (name.isEmpty())
        name = object.className();
    if (name.isEmpty())
        name = object.name();
    if (name.isEmpty()) {
        const QmlDebug::FileReference file = object.source();
        name = file.url().fileName() + ':' + QString::number(file.lineNumber());
    }
    if (name.isEmpty())
        name = Tr::tr("<anonymous>");

    pending->items.addChild(watchItem(iname, name, name, object.className(),
                                     QStringLiteral("object"), 1, debugId));
    addObjectWatch(debugId);

    const bool expanded = pending->expandedINames.contains(iname);
    if (!expanded && object.needsMoreData())
        return;

    if (expanded && object.needsMoreData()) {
        runInspectorLeg(m_engineClient->queryObject(debugId), pending, finishLeg,
                        [this, parentIname, engineId, pending, finishLeg]
                        (const QVariant &value, const QByteArray &) {
            appendObjectItems(qvariant_cast<QmlDebug::ObjectReference>(value), parentIname,
                              engineId, pending, finishLeg);
            finishLeg();
        });
        return;
    }

    const QList<QmlDebug::PropertyReference> properties = object.properties();
    if (!properties.isEmpty()) {
        const QString propertiesIName = iname + ".[properties]";
        pending->items.addChild(watchItem(propertiesIName, Tr::tr("Properties"), {}, {},
                                         QStringLiteral("list"), int(properties.size()),
                                         debugId));
        for (const QmlDebug::PropertyReference &property : properties) {
            const QString propertyName = property.name();
            if (propertyName.isEmpty())
                continue;
            const QString propertyIName = propertiesIName + '.' + propertyName;
            pending->items.addChild(watchItem(propertyIName, propertyName, propertyName,
                                             property.valueTypeName(),
                                             property.value().toString(),
                                             valueChildCount(property.value()), debugId));
            appendValueChildren(propertyIName, property.value(), debugId, &pending->items);
        }
    }

    const QList<QmlDebug::ObjectReference> children = object.children();
    for (const QmlDebug::ObjectReference &child : children)
        appendObjectItems(child, iname, engineId, pending, finishLeg);
}

void QmlImpl::refreshInspectorTree(const RefreshRequest &request)
{
    m_expandedInspectorINames = request.expandedINames;

    const quint64 requestId = request.requestId;
    if (!m_engineClient || m_engineClient->state() != QmlDebug::QmlDebugClient::Enabled) {
        emit refreshDataReceived(requestId, RefreshKind::InspectorTree, emptyLocalsData());
        return;
    }

    const auto pending = makeCollector(request);
    const auto finishLeg = legFinisher(pending);

    runInspectorLeg(m_engineClient->queryAvailableEngines(), pending, finishLeg,
                    [this, pending, finishLeg](const QVariant &value, const QByteArray &) {
        const auto engines = qvariant_cast<QList<QmlDebug::EngineReference>>(value);
        if (engines.isEmpty()) {
            if (m_engineQueryRetriesLeft > 0) {
                --m_engineQueryRetriesLeft;
                QTimer::singleShot(100, this, [this] { rebuildInspectorTree(); });
            }
            finishLeg();
            return;
        }
        m_qmlEngines = engines;
        for (const QmlDebug::EngineReference &engine : engines) {
            const int engineId = engine.debugId();
            QString name = engine.name();
            if (name.isEmpty())
                name = Tr::tr("Engine %1").arg(engineId);
            const QString iname = "inspect." + QString::number(engineId);
            m_inameForDebugId.insert(engineId, iname);
            m_engineIdForDebugId.insert(engineId, engineId);
            pending->items.addChild(watchItem(iname, name, {}, {},
                                             QStringLiteral("object"), 1, engineId));
            runInspectorLeg(m_engineClient->queryRootContexts(engine), pending, finishLeg,
                            [this, pending, finishLeg, iname, engineId]
                            (const QVariant &contextValue, const QByteArray &) {
                QList<QmlDebug::ContextReference> contexts{
                    qvariant_cast<QmlDebug::ContextReference>(contextValue)};
                int visited = 0;
                while (!contexts.isEmpty()) {
                    const QmlDebug::ContextReference context = contexts.takeLast();
                    const QList<QmlDebug::ObjectReference> objects = context.objects();
                    for (const QmlDebug::ObjectReference &object : objects)
                        appendObjectItems(object, iname, engineId, pending, finishLeg);
                    if (++visited > MaxInspectorTreeNodes)
                        break;
                    contexts.append(context.contexts());
                }
                for (auto it = m_knownDelegateIds.cbegin(), end = m_knownDelegateIds.cend();
                     it != end; ++it) {
                    if (it.value() != engineId || pending->seenDebugIds.contains(it.key()))
                        continue;
                    runInspectorLeg(m_engineClient->queryObject(it.key()), pending, finishLeg,
                                    [this, pending, finishLeg, iname, engineId]
                                    (const QVariant &objectValue, const QByteArray &) {
                        appendObjectItems(qvariant_cast<QmlDebug::ObjectReference>(objectValue),
                                          iname, engineId, pending, finishLeg);
                        finishLeg();
                    });
                }
                finishLeg();
            });
        }
        finishLeg();
    });
}

void QmlImpl::rebuildInspectorTree()
{
    RefreshRequest request;
    request.kind = RefreshKind::InspectorTree;
    request.expandedINames = m_expandedInspectorINames;
    refreshInspectorTree(request);
}

void QmlImpl::handleObjectCreated(int engineId, int objectId, int parentId)
{
    if (parentId == -1 && objectId != -1)
        m_knownDelegateIds.insert(objectId, engineId);

    for (const QmlDebug::EngineReference &engine : std::as_const(m_qmlEngines)) {
        if (engine.debugId() == engineId) {
            m_objectCreatedTimer->start();
            return;
        }
    }
}

void QmlImpl::handlePropertyValueChanged(int debugId, const QByteArray &name,
                                         const QVariant &value)
{
    const QString objectIName = m_inameForDebugId.value(debugId);
    if (objectIName.isEmpty())
        return;
    const QString iname = objectIName + ".[properties]." + QString::fromLatin1(name);

    GdbMi items;
    items.m_type = GdbMi::List;
    items.m_name = QStringLiteral("data");
    items.addChild(watchItem(iname, QString::fromLatin1(name), QString::fromLatin1(name), {},
                             value.toString(), valueChildCount(value), debugId));
    appendValueChildren(iname, value, debugId, &items);
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(items);
    emit refreshDataReceived(0, RefreshKind::InspectorTree, all);
}

void QmlImpl::queryObjectExpression(int debugId, const QString &expression)
{
    const int engineId = m_engineIdForDebugId.value(debugId, -1);
    const quint32 queryId = m_engineClient
                            ? m_engineClient->queryExpressionResult(debugId, expression, engineId)
                            : 0;
    const bool queried = runInspectorQuery(queryId, [this](const QVariant &value,
                                                           const QByteArray &) {
        emit message(value.toString(), ConsoleOutput);
    });
    if (!queried) {
        emit message(Tr::tr("The application has to be stopped in a breakpoint in order to "
                            "evaluate expressions."), ConsoleOutput);
    }
}

void QmlImpl::refresh(const RefreshRequest &request)
{
    if (request.kind == RefreshKind::Locals) {
        refreshLocals(request);
        return;
    }
    if (request.kind == RefreshKind::InspectorTree) {
        refreshInspectorTree(request);
        return;
    }
    if (request.kind == RefreshKind::SourceFiles) {
        refreshSourceFiles(request);
        return;
    }
    if (request.kind != RefreshKind::FullStack)
        return;

    const quint64 requestId = request.requestId;
    runCommand({BACKTRACE}, [this, requestId](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QVariantList v8Frames = body.value(QLatin1String("frames")).toList();

        GdbMi frames;
        frames.m_type = GdbMi::List;
        for (const QVariant &v8FrameVal : v8Frames) {
            const QVariantMap v8Frame = v8FrameVal.toMap();
            const QVariantMap script = v8Frame.value(QLatin1String("script")).toMap();
            QString function = v8Frame.value(QLatin1String(FUNCTION)).toString();
            if (function.isEmpty())
                function = v8Frame.value(QLatin1String("func")).toString();

            GdbMi frame;
            frame.m_type = GdbMi::Tuple;
            frame.addChild(constMi(QLatin1String("level"),
                                   QString::number(v8Frame.value(QLatin1String("index")).toInt())));
            frame.addChild(constMi(QLatin1String(FUNCTION), function));
            frame.addChild(constMi(QLatin1String("file"), script.value(QLatin1String(NAME)).toString()));
            frame.addChild(constMi(QLatin1String(LINE),
                                   QString::number(v8Frame.value(QLatin1String(LINE)).toInt() + 1)));
            frame.addChild(constMi(QLatin1String("language"), QStringLiteral("js")));
            frames.addChild(frame);
        }

        frames.m_name = QStringLiteral("frames");
        GdbMi stack;
        stack.m_type = GdbMi::Tuple;
        stack.m_name = QStringLiteral("stack");
        stack.addChild(frames);

        GdbMi data;
        data.m_type = GdbMi::Tuple;
        data.addChild(stack);
        emit refreshDataReceived(requestId, RefreshKind::FullStack, data);
    });
}

void QmlImpl::activateFrame(int index)
{
    m_currentFrameIndex = index;
}

void QmlImpl::selectThread(const QString &)
{
}

void QmlImpl::executeDebuggerCommand(const QString &command, const WatchItemData &inspectorItem)
{
    if (m_inferiorRunning) {
        if (!inspectorItem.isInspect || inspectorItem.id == -1) {
            emit message(Tr::tr("The application has to be stopped in a breakpoint in order to "
                                "evaluate expressions."), ConsoleOutput);
            return;
        }
        queryObjectExpression(int(inspectorItem.id), command);
        return;
    }
    DebuggerCommand cmd(EVALUATE);
    cmd.arg(EXPRESSION, command);
    cmd.arg(FRAME, m_currentFrameIndex);
    runCommand(cmd, [this](const QVariantMap &resp) {
        const bool success = resp.value(QLatin1String(SUCCESS)).toBool();
        if (!success) {
            emit message(resp.value(QLatin1String(MESSAGE)).toString(), LogError);
            return;
        }
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        emit message(body.value(QLatin1String(VALUE)).toString(), ConsoleOutput);
    });
}

void QmlImpl::setRegisterValue(const QString &, const QString &) {}
void QmlImpl::accessMemory(MemoryOp, quint64, quint64, quint64, const QByteArray &) {}
void QmlImpl::fetchDisassembly(quint64, quint64, const QString &) {}
void QmlImpl::setPeripheralRegisterValue(quint64, quint64) {}
void QmlImpl::watchPoint(quint64, const QPoint &) {}
void QmlImpl::createSnapshot(quint64) {}
void QmlImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    QString literal;
    if (item.type == "boolean") {
        literal = (value != "false" && value != "0") ? QStringLiteral("true")
                                                    : QStringLiteral("false");
    } else if (item.type == "number") {
        literal = value;
    } else {
        literal = '"' + QString(value).replace('"', QLatin1String("\\\"")) + '"';
    }
    const QString assignment = expr + " = " + literal + ';';

    if (item.isInspect) {
        if (item.id != -1)
            queryObjectExpression(int(item.id), assignment);
        return;
    }

    DebuggerCommand cmd(EVALUATE);
    cmd.arg(EXPRESSION, assignment);
    cmd.arg(FRAME, m_currentFrameIndex);
    runCommand(cmd, [this](const QVariantMap &resp) {
        if (!resp.value(QLatin1String(SUCCESS)).toBool()) {
            emit message(resp.value(QLatin1String(MESSAGE)).toString(), LogError);
            return;
        }
    });
}
} // namespace Debugger::Internal
