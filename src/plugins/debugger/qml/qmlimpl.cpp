// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlimpl.h"

#include "qmlv8debuggerclientconstants.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../debuggertr.h"

#include <qmldebug/qpacketprotocol.h>

#include <utils/qtcassert.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
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

// The debug service knows one exception break, which covers the caught ones as
// well, so all three spellings of the request end up as the same one.
static bool isExceptionBreakpoint(BreakpointType type)
{
    return type == BreakpointAtJavaScriptThrow || type == BreakpointAtThrow
           || type == BreakpointAtCatch;
}

static DebuggerEngineSetupData qmlImplSetupData()
{
    DebuggerEngineSetupData data;
    data.capabilities = AddWatcherCapability
                      | AddWatcherWhileRunningCapability
                      | BreakConditionCapability
                      | BreakOnThrowAndCatchCapability
                      | CreateFullBacktraceCapability
                      | ResetInferiorCapability
                      | RunToLineCapability
                      | TracePointCapability
                      | WatchComplexExpressionsCapability;
    data.extraCapabilities = DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::RunAsUser
                           | DebuggerExtraCapability::RunCommandDeferral
                           | DebuggerExtraCapability::SourceFiles;
    data.startModes = DebuggerStartModeFlag::AttachToQmlServer
                    | DebuggerStartModeFlag::Launch;
    data.toolTipHandling = ToolTipHandling::Always;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        if (query.type == BreakpointOnQmlSignalEmit || isExceptionBreakpoint(query.type))
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

    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        if (m_pendingCommands.isEmpty())
            return;
        QStringList pending;
        for (const PendingCommand &command : std::as_const(m_pendingCommands))
            pending << command.command;
        m_watchdog.start();
        emit notResponding(m_startData.watchdogTimeout, pending);
    });

    m_inferiorProcess.setProcessMode(ProcessMode::Reader);
    connect(&m_inferiorProcess, &Process::readyReadStandardOutput, this, [this] {
        emit message(m_inferiorProcess.readAllStandardOutput(), AppOutput);
    });
    connect(&m_inferiorProcess, &Process::readyReadStandardError, this, [this] {
        emit message(m_inferiorProcess.readAllStandardError(), AppError);
    });
    connect(&m_inferiorProcess, &Process::done, this, [this] {
        if (m_inferiorProcess.error() == ProcessError::FailedToStart) {
            emit message(m_inferiorProcess.errorString(), LogError);
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
            return;
        }
        if (m_aborting) {
            emit engineProcessFinished(m_inferiorProcess.resultData());
            return;
        }
        if (m_isResetRestart) {
            // Our own kill behind a ResetInferior. An exit reported here would take
            // the engine down instead of putting a fresh runtime in its place.
            resetTransientState();
            m_connection.close();
            QMetaObject::invokeMethod(this, [this] { launchInferior(); }, Qt::QueuedConnection);
            return;
        }
        if (m_shuttingDown)
            return;
        m_inferiorRunning = false;
        m_inferiorExited = true;
        emit inferiorDone(InferiorResultData{
            m_inferiorProcess.exitCode(),
            m_inferiorProcess.exitStatus() == ProcessExitStatus::CrashExit
                ? InferiorExitStatus::Crash : InferiorExitStatus::Normal});
    });
    connect(&m_inferiorProcess, &Process::started, this, [this] {
        emit inferiorPidKnown(ProcessHandle(m_inferiorProcess.processId()));
    });

    connect(&m_connection, &QmlDebug::QmlDebugConnection::connectionFailed, this, [this] {
        if (m_connectRetriesLeft > 0) {
            --m_connectRetriesLeft;
            QTimer::singleShot(100, this, [this] { beginConnection(); });
            return;
        }
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
    });
    connect(&m_connection, &QmlDebug::QmlDebugConnection::disconnected, this, [this] {
        if (m_shuttingDown || m_isResetRestart || m_aborting)
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
    if (std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
        launchInferior();
        return;
    }
    beginConnection();
}

void QmlImpl::launchInferior()
{
    {
        // The runtime takes the port as a number, so a free one has to be
        // picked before it is started. It is handed over rather than kept, and
        // "block" makes the runtime wait for this end to arrive on it.
        QTcpServer probe;
        if (!probe.listen(QHostAddress::LocalHost)) {
            emit message("No port available for the Qml debug connection.", LogError);
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
            return;
        }
        m_port = probe.serverPort();
    }

    ProcessRunData runData = std::get<ProcessRunData>(m_startData.inferiorStartData);
    runData.command.addArg(QString("-qmljsdebugger=port:%1,block,services:V8Debugger,QmlDebugger")
                               .arg(m_port));
    m_inferiorProcess.setRunData(runData);
    m_inferiorProcess.setRunAsUser(m_startData.runAsUser);
    m_inferiorProcess.start();

    beginConnection();
}

void QmlImpl::beginConnection()
{
    if (m_port != 0) {
        m_connection.connectToHost("127.0.0.1", m_port);
        return;
    }
    const auto &qmlData = std::get<AttachToQmlServerData>(m_startData.inferiorStartData);
    m_connection.connectToHost(qmlData.server.host(), quint16(qmlData.server.port()));
}

// What the runtime that has just been replaced left behind: none of it says
// anything about the one taking its place.
void QmlImpl::resetTransientState()
{
    m_inferiorRunning = false;
    m_inferiorExited = false;
    m_interruptRequested = false;
    m_disconnected = false;
    m_currentFrameIndex = 0;
    m_connectRetriesLeft = 50;
    m_engineQueryRetriesLeft = 50;
    m_callbackForToken.clear();
    m_pendingCommands.clear();
    m_hitCountsByResponseId.clear();
    m_serviceNumberByResponseId.clear();
    m_qmlEngines.clear();
    m_inameForDebugId.clear();
    m_engineIdForDebugId.clear();
    m_objectWatches.clear();
    m_knownDelegateIds.clear();
    m_deferredWatchers.reset();
}

void QmlImpl::sendDisconnect()
{
    if (m_disconnected || !m_v8Client
        || m_v8Client->state() != QmlDebug::QmlDebugClient::Enabled) {
        return;
    }
    m_disconnected = true;
    runCommand({DISCONNECT});
}

void QmlImpl::shutdownInferior(ShutdownMode mode)
{
    if (mode == ShutdownMode::Kill && m_inferiorProcess.isRunning()) {
        m_shuttingDown = true;
        m_inferiorProcess.close();
    }
    sendDisconnect();
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
    static const QHash<QmlDebug::QmlDebugClient::State, QString> names {
        {QmlDebug::QmlDebugClient::NotConnected, "not connected"},
        {QmlDebug::QmlDebugClient::Unavailable, "unavailable"},
        {QmlDebug::QmlDebugClient::Enabled, "enabled"}};
    emit message(QString("Status of \"%1\" Version: %2 changed to '%3'.")
                     .arg(m_v8Client->name())
                     .arg(m_v8Client->serviceVersion())
                     .arg(names.value(state)), LogMisc);
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

    if (std::exchange(m_isResetRestart, false)) {
        // The fresh runtime knows none of the breakpoints, while the model knows
        // them all: they go in again under the ids it has, and it hears nothing.
        const QList<BreakpointChangeRequest> breakpoints
            = m_activeBreakpointsByResponseId.values();
        for (const BreakpointChangeRequest &breakpoint : breakpoints)
            setScriptBreakpoint(breakpoint.requestId, breakpoint, breakpoint.responseId);
        m_inferiorRunning = true;
        return;
    }

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
    if (m_startData.logTimeStamps
        || m_startData.watchdogTimeout != std::chrono::seconds::zero()) {
        // The arguments belong to the description: what a command was asked
        // about is what tells two of the same name apart.
        const QString description = command.function + ' '
            + QString::fromUtf8(QJsonDocument(command.args.toObject()).toJson(QJsonDocument::Compact));
        m_pendingCommands[m_sequence] = {description, QDateTime::currentMSecsSinceEpoch()};
        restartWatchdog();
    }
    runDirectCommand(V8REQUEST, QJsonDocument(object).toJson(QJsonDocument::Compact));
    return m_sequence;
}

void QmlImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_pendingCommands.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
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
        if (const PendingCommand answered = m_pendingCommands.take(requestSeq); answered.postTime) {
            if (m_startData.logTimeStamps) {
                const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - answered.postTime;
                emit message(QString("Response time: %1: %2 s").arg(answered.command)
                                 .arg(elapsed / 1000.), LogTime);
            }
            restartWatchdog();
        }
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

int QmlImpl::serviceNumberFor(const QString &responseId) const
{
    const QString number = m_serviceNumberByResponseId.value(responseId);
    return number.isEmpty() ? responseId.toInt() : number.toInt();
}

void QmlImpl::clearBreakpointNumber(const QString &responseId)
{
    DebuggerCommand cmd(CLEARBREAKPOINT);
    cmd.arg(BREAKPOINT, serviceNumberFor(responseId));
    runCommand(cmd);
}

// The service keeps no count of its own, and says nothing about the breakpoint
// it stopped for, so the hit the view shows is the one counted here.
void QmlImpl::reportBreakpointHit(const QString &responseId, int hits)
{
    const BreakpointParameters params
        = m_activeBreakpointsByResponseId.value(responseId).params;
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi(QStringLiteral("number"), responseId));
    bkpt.addChild(constMi(QStringLiteral("file"), params.fileName.path()));
    bkpt.addChild(constMi(QStringLiteral("fullname"), params.fileName.path()));
    bkpt.addChild(constMi(QStringLiteral("line"), QString::number(params.textPosition.line)));
    bkpt.addChild(constMi(QStringLiteral("enabled"), params.enabled ? "y" : "n"));
    bkpt.addChild(constMi(QStringLiteral("times"), QString::number(hits)));
    // The model reads a modification as the whole state of the breakpoint, so a
    // condition left out of it counts as none rather than as unchanged.
    if (!params.condition.isEmpty())
        bkpt.addChild(constMi(QStringLiteral("cond"), params.condition));
    GdbMi list;
    list.m_type = GdbMi::List;
    list.addChild(bkpt);
    emit breakpointModified(list);
}

static std::pair<QString, QString> v8TypeAndValue(const QVariantMap &data, int stringLimit);

// The service has no tracepoint of its own, so the captures are evaluated at
// the stop the ordinary breakpoint behind it produced, and the message put
// together once the last answer is in.
void QmlImpl::reportTracepointHit(const QString &responseId,
                                  const std::function<void()> &finished)
{
    const auto hit = std::make_shared<TracepointHit>();
    hit->pattern = m_activeBreakpointsByResponseId.value(responseId).params.message;
    hit->captures = parseTracepointCaptures(hit->pattern);
    hit->values.m_type = GdbMi::List;
    hit->expressions.m_type = GdbMi::Tuple;
    hit->finished = finished;

    for (const TracepointCapture &capture : std::as_const(hit->captures)) {
        if (capture.type != TracepointCaptureType::Expression) {
            // Nothing the service can be asked for, so the capture stays as written.
            hit->values.addChild(constMi(QStringLiteral("value"),
                                         hit->pattern.mid(capture.start,
                                                          capture.end - capture.start)));
            continue;
        }
        const QString expression = capture.expression;
        hit->values.addChild(constMi(QStringLiteral("value"), expression));
        ++hit->outstanding;
        DebuggerCommand cmd(EVALUATE);
        cmd.arg(EXPRESSION, expression);
        cmd.arg(FRAME, 0);
        runCommand(cmd, [this, hit, expression](const QVariantMap &resp) {
            const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
            QString value = resp.value(QLatin1String(MESSAGE)).toString();
            if (resp.value(QLatin1String(SUCCESS)).toBool())
                value = v8TypeAndValue(body, m_stringLimit).second;
            GdbMi entry;
            entry.m_type = GdbMi::Tuple;
            entry.m_name = expression;
            entry.addChild(constMi(QStringLiteral("value"), value));
            hit->expressions.addChild(entry);
            finishTracepointHit(hit);
        });
    }
    finishTracepointHit(hit);
}

void QmlImpl::finishTracepointHit(const std::shared_ptr<TracepointHit> &hit)
{
    if (--hit->outstanding > 0)
        return;
    emit message(formatTracepointMessage(hit->pattern, hit->captures, hit->values,
                                         hit->expressions), LogMisc);
    hit->finished();
}

void QmlImpl::handleBreakEvent(const QVariantMap &response)
{
    m_inferiorRunning = false;
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();
    const QVariantMap script = body.value(QLatin1String("script")).toMap();
    const QString scriptName = script.value(QLatin1String(NAME)).toString();
    const int lineNumber = body.value(QLatin1String("sourceLine")).toInt() + 1;

    // The break event names no breakpoint at all, so what this stop belongs to
    // is whatever was put at the location it stopped at.
    const QString stoppedFile = FilePath::fromUrl(QUrl(scriptName)).fileName();
    QStringList hitHere;
    for (auto it = m_activeBreakpointsByResponseId.cbegin();
         it != m_activeBreakpointsByResponseId.cend(); ++it) {
        const BreakpointParameters &params = it->params;
        if (params.textPosition.line == lineNumber
            && params.fileName.fileName() == stoppedFile) {
            hitHere.append(it.key());
        }
    }

    // The debug service has no ignore count of its own, so the hits it is
    // asked to skip are counted here, and the inferior sent on unnoticed.
    if (!hitHere.isEmpty() && !m_interruptRequested) {
        bool skip = true;
        QStringList tracepoints;
        for (const QString &responseId : std::as_const(hitHere)) {
            const BreakpointParameters params
                = m_activeBreakpointsByResponseId.value(responseId).params;
            const int hits = ++m_hitCountsByResponseId[responseId];
            reportBreakpointHit(responseId, hits);
            if (params.isTracepoint()) {
                tracepoints.append(responseId);
                continue;
            }
            if (params.ignoreCount <= 0 || hits > params.ignoreCount)
                skip = false;
        }
        if (skip && tracepoints.isEmpty()) {
            m_inferiorRunning = true;
            runCommand({CONTINEDEBUGGING});
            return;
        }
        if (skip) {
            // The captures can only be evaluated while the inferior is stopped, so
            // it goes on once the last tracepoint has had its say.
            const auto outstanding = std::make_shared<int>(tracepoints.size());
            for (const QString &responseId : std::as_const(tracepoints)) {
                reportTracepointHit(responseId, [this, outstanding] {
                    if (--*outstanding > 0)
                        return;
                    m_inferiorRunning = true;
                    runCommand({CONTINEDEBUGGING});
                });
            }
            return;
        }
    }

    // The service has no breakpoint that stops only once either, so a one-shot
    // is an ordinary one taken back here, before the inferior runs again.
    QStringList takenBack;
    for (const QString &responseId : std::as_const(hitHere)) {
        if (!m_activeBreakpointsByResponseId.value(responseId).params.oneShot)
            continue;
        clearBreakpointNumber(responseId);
        m_activeBreakpointsByResponseId.remove(responseId);
        m_hitCountsByResponseId.remove(responseId);
        m_serviceNumberByResponseId.remove(responseId);
        takenBack.append(responseId);
    }

    if (!scriptName.isEmpty())
        emit locationChanged(FilePath::fromUrl(QUrl(scriptName)), lineNumber);
    emit inferiorEvent(std::exchange(m_interruptRequested, false)
                       ? InferiorEvent::StopOk : InferiorEvent::SpontaneousStop);

    for (const QString &responseId : takenBack) {
        GdbMi deleted;
        deleted.m_type = GdbMi::Tuple;
        deleted.addChild(constMi(QStringLiteral("number"), responseId));
        emit breakpointEvent(0, BreakpointOp::Remove, true, deleted);
    }

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

void QmlImpl::setScriptBreakpoint(quint64 requestId, const BreakpointChangeRequest &request,
                                  const QString &knownResponseId)
{
    const BreakpointParameters &params = request.params;
    DebuggerCommand cmd(SETBREAKPOINT);
    cmd.arg(TYPE, SCRIPTREGEXP);
    cmd.arg(TARGET, params.fileName.toUrlishString());
    cmd.arg(ENABLED, params.enabled);
    cmd.arg(LINE, params.textPosition.line - 1);
    if (params.textPosition.column > 0)
        cmd.arg(COLUMN, params.textPosition.column - 1);
    if (!params.condition.isEmpty())
        cmd.arg(CONDITION, params.condition);
    if (params.ignoreCount > 0)
        cmd.arg(IGNORECOUNT, params.ignoreCount);

    // A change the service cannot express is carried out by putting a new
    // breakpoint in place of the old one, and the model waits for an answer to
    // the request it made, not to the one that replaced it.
    const BreakpointOp op = request.op;
    runCommand(cmd, [this, requestId, op, params, request, knownResponseId]
                    (const QVariantMap &resp) {
        const bool success = resp.value(QLatin1String(SUCCESS)).toBool();
        if (!success) {
            if (knownResponseId.isEmpty())
                emit breakpointEvent(requestId, op, false, {});
            return;
        }
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QString serviceNumber
            = QString::number(body.value(QLatin1String(BREAKPOINT)).toInt());
        const QString responseId = knownResponseId.isEmpty() ? serviceNumber : knownResponseId;
        m_serviceNumberByResponseId[responseId] = serviceNumber;
        BreakpointChangeRequest active = request;
        active.responseId = responseId;
        m_activeBreakpointsByResponseId[responseId] = active;
        if (!knownResponseId.isEmpty())
            return;

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
        emit breakpointEvent(requestId, op, true, data);
    });
}

// Two locations the debug service cannot tell apart: a column is only passed
// on when there is one, so anything below the first column is the same request.
static bool isSameLocation(const Utils::Text::Position &was, const Utils::Text::Position &now)
{
    return was.line == now.line && qMax(0, was.column) == qMax(0, now.column);
}

// A change of an attribute comes with the location the model has, which can be
// none at all: what the request leaves out is what the breakpoint already has.
BreakpointChangeRequest QmlImpl::withKnownLocation(const BreakpointChangeRequest &request) const
{
    const auto it = m_activeBreakpointsByResponseId.constFind(request.responseId);
    if (it == m_activeBreakpointsByResponseId.constEnd() || !request.params.fileName.isEmpty())
        return request;
    BreakpointChangeRequest filled = request;
    filled.params.fileName = it->params.fileName;
    filled.params.textPosition = it->params.textPosition;
    return filled;
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
           && isSameLocation(was.textPosition, now.textPosition)
           && was.condition == now.condition
           && was.ignoreCount == now.ignoreCount
           && was.command == now.command;
}

void QmlImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const BreakpointParameters &params = request.params;
    switch (request.op) {
    case BreakpointOp::Insert:
        if (isExceptionBreakpoint(params.type)) {
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
        if (isExceptionBreakpoint(params.type)) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            runCommand(cmd);
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << false;
            runDirectCommand(BREAKONSIGNAL, rs.data());
        } else if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Remove, false, {});
            break;
        } else {
            DebuggerCommand cmd(CLEARBREAKPOINT);
            cmd.arg(BREAKPOINT, serviceNumberFor(request.responseId));
            runCommand(cmd);
            m_activeBreakpointsByResponseId.remove(request.responseId);
            m_hitCountsByResponseId.remove(request.responseId);
            m_serviceNumberByResponseId.remove(request.responseId);
        }
        emit breakpointEvent(request.requestId, BreakpointOp::Remove, true, {});
        break;
    case BreakpointOp::Update: {
        const BreakpointChangeRequest update = withKnownLocation(request);
        if (isExceptionBreakpoint(params.type)) {
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
        } else if (m_supportChangeBreakpoint && isEnabledOnlyChange(update)) {
            DebuggerCommand cmd(CHANGEBREAKPOINT);
            cmd.arg(BREAKPOINT, serviceNumberFor(request.responseId));
            cmd.arg(ENABLED, params.enabled);
            const quint64 requestId = request.requestId;
            runCommand(cmd, [this, requestId, update](const QVariantMap &resp) {
                const bool ok = resp.value(QLatin1String(SUCCESS)).toBool();
                if (ok)
                    m_activeBreakpointsByResponseId.insert(update.responseId, update);
                emit breakpointEvent(requestId, BreakpointOp::Update, ok, {});
            });
        } else if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Update, false, {});
        } else {
            DebuggerCommand clearCmd(CLEARBREAKPOINT);
            clearCmd.arg(BREAKPOINT, serviceNumberFor(request.responseId));
            runCommand(clearCmd);
            m_activeBreakpointsByResponseId.remove(request.responseId);
            m_hitCountsByResponseId.remove(request.responseId);
            m_serviceNumberByResponseId.remove(request.responseId);
            setScriptBreakpoint(request.requestId, update);
        }
        break;
    }
    case BreakpointOp::EnableSub:
        // The debug service addresses a breakpoint as a whole, so there is no
        // single location to enable, and saying so is better than saying nothing.
        emit breakpointEvent(request.requestId, BreakpointOp::EnableSub, false, {});
        break;
    }
}

void QmlImpl::execute(const ExecutionRequest &request)
{
    switch (request.command) {
    case ExecutionCommand::Continue:
    case ExecutionCommand::StepIn:
    case ExecutionCommand::StepOver:
    case ExecutionCommand::StepOut:
    case ExecutionCommand::RunToLine:
        // There is nothing left to resume once the runtime is gone, and the
        // request would wait for a reply that cannot come.
        if (m_inferiorExited) {
            emit inferiorEvent(InferiorEvent::InferiorIll);
            return;
        }
        break;
    default:
        break;
    }

    switch (request.command) {
    case ExecutionCommand::Continue:
        // Symmetric to the interrupt below: a request that cannot go out is
        // answered right away, rather than left for a reply that never comes.
        if (m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::RunFailed);
            break;
        }
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
        sendDisconnect();
        emit inferiorDone({0, InferiorExitStatus::Detached});
        break;
    case ExecutionCommand::ResetInferior:
        if (!std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
            emit message("The runtime this session attached to is not ours to restart.",
                         LogWarning);
            emit inferiorEvent(InferiorEvent::RunFailed);
            break;
        }
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        // The done handler puts the fresh runtime in place, a kill of our own
        // reported as an exit would end the session instead.
        m_isResetRestart = true;
        m_inferiorProcess.kill();
        break;
    case ExecutionCommand::RunToFunction:
        // The action offering this is there whatever the backend is, and the
        // debug service places a breakpoint by file and line only, so there is
        // no function for it to run to.
        emit message(Tr::tr("The QML debug service places breakpoints by file and line only, "
                            "so it cannot be asked to run to a function."), LogWarning);
        break;
    case ExecutionCommand::Abort:
        // The user gave up, so nothing is asked of the runtime any more: the
        // connection goes, and the runtime with it where it is ours to end.
        m_aborting = true;
        m_connection.close();
        if (m_inferiorProcess.isRunning()) {
            m_inferiorProcess.kill();
            break;
        }
        emit engineProcessFinished({});
        break;
    case ExecutionCommand::RepeatLastCommand:
        if (m_lastLocalsRequest)
            refreshLocals(*m_lastLocalsRequest);
        break;
    default:
        break;
    }
}

static std::pair<QString, QString> v8TypeAndValue(const QVariantMap &data, int stringLimit)
{
    const QString type = data.value(QLatin1String(TYPE)).toString();
    const QVariant value = data.value(QLatin1String(VALUE));
    if (type == "undefined")
        return {"undefined", "undefined"};
    if (type == "null")
        return {"object", "null"};
    if (type == "string") {
        QString text = value.toString();
        if (stringLimit > 0 && text.size() > stringLimit)
            text = text.left(stringLimit) + "...";
        return {type, '"' + text + '"'};
    }
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

// The view merges a partial answer into the tree it already has, so what such
// an answer must not carry is the locals the request did not name.
static GdbMi keptForPartialRefresh(const GdbMi &items, const QString &iname)
{
    GdbMi kept;
    kept.m_type = items.m_type;
    kept.m_name = items.m_name;
    for (const GdbMi &item : items) {
        const QString itemIName = item["iname"].data();
        if (itemIName == iname || itemIName.startsWith(iname + '.'))
            kept.addChild(item);
    }
    return kept;
}

std::function<void()> QmlImpl::legFinisher(const std::shared_ptr<RefreshCollector> &pending)
{
    return [this, pending] {
        if (--pending->remaining > 0)
            return;
        GdbMi all;
        all.m_type = GdbMi::Tuple;
        if (pending->partialVariable.isEmpty()) {
            all.addChild(pending->items);
        } else {
            all.addChild(keptForPartialRefresh(pending->items, pending->partialVariable));
            all.addChild(constMi(QStringLiteral("partial"), QStringLiteral("1")));
        }
        emit refreshDataReceived(pending->requestId, pending->kind, all);
    };
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

void QmlImpl::refreshLocals(const RefreshRequest &request)
{
    const QJsonArray watchers = request.watchers;
    m_lastLocalsRequest = request;
    // Nothing here reads a string in pieces, so the smaller of the two limits is
    // what decides how much of one reaches the view.
    m_stringLimit = qMin(request.dumperOptions.maximalStringLength,
                         request.dumperOptions.displayStringLimit);

    if (m_inferiorRunning) {
        m_deferredWatchers = request;
        return;
    }

    const auto pending = makeCollector(request);
    pending->partialVariable = request.partialVariable;
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
        runCommand(cmd, [this, iname, exp, hexExp, pending, finishLeg](const QVariantMap &resp) {
            const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
            GdbMi item;
            item.m_type = GdbMi::Tuple;
            item.addChild(constMi(QStringLiteral("iname"), iname));
            item.addChild(constMi(QStringLiteral("wname"), hexExp));
            int numchild = 0;
            if (resp.value(QLatin1String(SUCCESS)).toBool()) {
                const auto [type, value] = v8TypeAndValue(body, m_stringLimit);
                numchild = v8ChildCount(body);
                item.addChild(constMi(QStringLiteral("type"), type));
                item.addChild(constMi(QStringLiteral("value"), value));
            } else {
                item.addChild(constMi(QStringLiteral("value"),
                                      body.value(QLatin1String("text")).toString()));
            }
            item.addChild(constMi(QStringLiteral("numchild"), QString::number(numchild)));
            pending->items.addChild(item);
            if (numchild > 0 && pending->expandedINames.contains(iname)) {
                const int handle = body.value(QLatin1String(REF),
                                              body.value(QLatin1String(HANDLE))).toInt();
                QList<LookupRequest> lookups = appendV8Children(iname, exp, body, pending);
                if (lookups.isEmpty() && handle != 0
                    && !body.contains(QLatin1String("properties"))) {
                    lookups.append({handle, iname, exp, exp});
                }
                lookupHandles(lookups, pending, finishLeg);
            }
            finishLeg();
        });
    }

    DebuggerCommand frameCmd(FRAME);
    frameCmd.arg(NUMBER, m_currentFrameIndex);
    runCommand(frameCmd, [this, pending, finishLeg](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();

        const QVariantMap receiver = body.value(QLatin1String("receiver")).toMap();
        const auto [thisType, thisValue] = v8TypeAndValue(receiver, m_stringLimit);
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
        const auto [type, value] = v8TypeAndValue(property, m_stringLimit);
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

QList<QmlImpl::LookupRequest> QmlImpl::appendV8Children(
    const QString &iname, const QString &exp, const QVariantMap &resolved,
    const std::shared_ptr<RefreshCollector> &pending)
{
    QList<LookupRequest> nextRound;
    const auto [parentType, parentValue] = v8TypeAndValue(resolved, m_stringLimit);
    for (const QVariant &childValue : resolved.value(QLatin1String("properties")).toList()) {
        const QVariantMap child = childValue.toMap();
        const QString childName = child.value(QLatin1String(NAME)).toString();
        if (childName.isEmpty() || childName.startsWith('.'))
            continue;
        const QString childExp = parentValue == "Array" ? QString(exp + '[' + childName + ']')
                                                        : QString(exp + '.' + childName);
        const QString childIName = iname + '.' + childName;
        const auto [childType, childText] = v8TypeAndValue(child, m_stringLimit);
        const int childNumChild = v8ChildCount(child);
        pending->items.addChild(watchItem(childIName, childName, childExp, childType, childText,
                                          childNumChild));

        const int childHandle = child.value(QLatin1String(REF),
                                            child.value(QLatin1String(HANDLE))).toInt();
        const bool childExpanded = childNumChild > 0
                                   && pending->expandedINames.contains(childIName);
        if (childHandle != 0 && (childType.isEmpty() || childExpanded))
            nextRound.append({childHandle, childIName, childName, childExp});
    }
    return nextRound;
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
            const auto [type, value] = v8TypeAndValue(resolved, m_stringLimit);
            const QVariantList properties = resolved.value(QLatin1String("properties")).toList();

            const int numchild = properties.isEmpty() ? v8ChildCount(resolved)
                                                      : int(properties.size());
            pending->items.addChild(watchItem(iname, requestIt->name, requestIt->exp, type, value,
                                              numchild));

            nextRound.append(appendV8Children(iname, requestIt->exp, resolved, pending));
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
            // The view opens a source file by its full name, and a script names
            // itself by a url.
            const QUrl url(name);
            GdbMi entry;
            entry.m_type = GdbMi::Tuple;
            entry.addChild(constMi(QStringLiteral("file"), name));
            entry.addChild(constMi(QStringLiteral("fullname"),
                                   url.isLocalFile() ? url.toLocalFile() : name));
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
    // Nothing behind the debug service has dumpers or symbols to load, so what
    // the caller is after is the values, and the frames they belong to, again.
    if (request.kind == RefreshKind::DebuggingHelpers) {
        refreshLocals({request.requestId, RefreshKind::Locals});
        return;
    }
    if (request.kind == RefreshKind::AllSymbols) {
        refresh({request.requestId, RefreshKind::FullStack});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    }
    if (request.kind != RefreshKind::FullStack && request.kind != RefreshKind::FullBacktrace)
        return;

    const quint64 requestId = request.requestId;
    const RefreshKind kind = request.kind;
    // A full backtrace is about everything there is, so a depth limit meant for
    // the stack view does not apply to it.
    const int depthLimit = kind == RefreshKind::FullBacktrace ? -1 : request.stackDepthLimit;
    // The debug service answers with the first ten frames only unless the range
    // is spelled out, which is not what either the stack view or a backtrace is
    // after when nothing limits them.
    DebuggerCommand cmd(BACKTRACE);
    cmd.arg("fromFrame", 0);
    cmd.arg("toFrame", depthLimit >= 0 ? depthLimit : 10000);
    runCommand(cmd, [this, requestId, kind](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QVariantList v8Frames = body.value(QLatin1String("frames")).toList();

        GdbMi frames;
        frames.m_type = GdbMi::List;
        for (const QVariant &v8FrameVal : v8Frames) {
            const QVariantMap v8Frame = v8FrameVal.toMap();
            QString function = v8Frame.value(QLatin1String(FUNCTION)).toString();
            if (function.isEmpty())
                function = v8Frame.value(QLatin1String("func")).toString();

            GdbMi frame;
            frame.m_type = GdbMi::Tuple;
            frame.addChild(constMi(QLatin1String("level"),
                                   QString::number(v8Frame.value(QLatin1String("index")).toInt())));
            frame.addChild(constMi(QLatin1String(FUNCTION), function));
            // A frame names its script as a URL, which is what a stop reports too,
            // and what the editor has to be handed to open the source.
            const QVariant script = v8Frame.value(QLatin1String("script"));
            const QString scriptName = script.typeId() == QMetaType::QVariantMap
                                           ? script.toMap().value(QLatin1String(NAME)).toString()
                                           : script.toString();
            frame.addChild(constMi(QLatin1String("file"),
                                   FilePath::fromUrl(QUrl(scriptName)).toUrlishString()));
            frame.addChild(constMi(QLatin1String(LINE),
                                   QString::number(v8Frame.value(QLatin1String(LINE)).toInt() + 1)));
            frame.addChild(constMi(QLatin1String("language"), QStringLiteral("js")));
            frames.addChild(frame);
        }

        if (kind == RefreshKind::FullBacktrace) {
            QString text;
            for (const GdbMi &frame : frames) {
                text += QString("#%1  %2 at %3:%4\n")
                            .arg(frame["level"].data(), frame[FUNCTION].data(),
                                 frame["file"].data(), frame[LINE].data());
            }
            emit refreshDataReceived(requestId, RefreshKind::FullBacktrace, constMi({}, text));
            return;
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
