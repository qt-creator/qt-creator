// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "breakhandler.h"
#include "console/console.h"
#include "console/consoleitem.h"
#include "debuggerengine.h"
#include "debuggerruncontrol.h"
#include "enginemanager.h"
#include "mcpsessionstate.h"
#include "mcpsupport.h"
#include "mcpsupport_p.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <mcp/server/toolregistry.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/processinterface.h>
#include <utils/result.h>

#include <QDeadlineTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPointer>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <optional>

using namespace Utils;

namespace Debugger::Internal {

enum {
    defaultDataTimeoutMs = 5000,
    defaultOperationTimeoutMs = 10000,
    maximalTimeoutMs = 120000
};

class AsyncMcpTool
{
public:
    Mcp::Schema::Tool tool;
    McpHandler handler;
};

static QHash<QString, AsyncMcpTool> &asyncMcpTools()
{
    static QHash<QString, AsyncMcpTool> tools;
    return tools;
}

static Mcp::Schema::CallToolResult toolResult(const Result<QJsonObject> &result)
{
    using namespace Mcp::Schema;
    if (!result)
        return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
    return CallToolResult{}.isError(false).structuredContent(*result);
}

/*!
    \internal

    Registers \a tool to be answered by \a handler, which may reply right away
    or later, once the debugger has delivered what the tool reports.
*/
void registerAsyncMcpTool(const Mcp::Schema::Tool &tool, const McpHandler &handler)
{
    asyncMcpTools().insert(tool.name(), {tool, handler});
    Mcp::ToolRegistry::registerTool(
        tool,
        [handler](const Mcp::Schema::CallToolRequestParams &params,
                  const Mcp::ToolInterface &toolInterface) -> Result<> {
            handler(params.argumentsAsObject(), [toolInterface](const Result<QJsonObject> &result) {
                toolInterface.finish(toolResult(result));
            });
            return ResultOk;
        });
}

void callMcpToolForTests(const QString &name,
                         const QJsonObject &args,
                         const std::function<void(Result<QJsonObject>)> &done)
{
    const auto it = asyncMcpTools().constFind(name);
    if (it == asyncMcpTools().constEnd()) {
        done(ResultError(QString("No asynchronous debugger tool named \"%1\".").arg(name)));
        return;
    }
    const Result<> valid = Mcp::validateToolArguments(
        it->tool, Mcp::Schema::CallToolRequestParams{}.arguments(args));
    if (!valid) {
        done(ResultError(valid.error()));
        return;
    }
    it->handler(args, done);
}

Result<McpSessionState *> activeSessionState()
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("No active debug session");
    return McpSessionState::forEngine(engine);
}

Result<McpSessionState *> pausedSessionState()
{
    const Result<McpSessionState *> state = activeSessionState();
    if (!state)
        return state;
    const DebuggerState engineState = (*state)->engine()->state();
    if (engineState != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName(engineState) + ")");
    return state;
}

int timeoutArgument(const QJsonObject &args, int defaultMs)
{
    return std::clamp(args.value("timeout_ms").toInt(defaultMs), 0, int(maximalTimeoutMs));
}

QJsonObject timeoutSchema(int defaultMs)
{
    return QJsonObject{
        {"type", "integer"},
        {"minimum", 0},
        {"maximum", maximalTimeoutMs},
        {"default", defaultMs},
        {"description",
         QString("How long to wait, in milliseconds, before reporting that the result is not "
                 "ready (default %1, at most %2).").arg(defaultMs).arg(maximalTimeoutMs)}};
}

QJsonObject contextSchema()
{
    return QJsonObject{
        {"type", "object"},
        {"description",
         "The debugger state the data belongs to. stop_id grows by one each time the target "
         "stops, so data with different stop_id values comes from different stops."},
        {"required", QJsonArray{"session_id", "stop_id"}},
        {"properties", QJsonObject{
            {"session_id",  QJsonObject{{"type", "string"},  {"description", "Identifies the debug session."}}},
            {"stop_id",     QJsonObject{{"type", "integer"}, {"description", "Identifies the stop within the session."}}},
            {"thread_id",   QJsonObject{{"type", "string"},  {"description", "The selected thread."}}},
            {"frame_level", QJsonObject{{"type", "integer"}, {"description", "The selected stack frame, 0 = innermost."}}},
        }}};
}

QString notReadyMessage(const QString &what, McpSessionState *state, bool sessionEnded)
{
    if (sessionEnded || !state || !state->engine())
        return QString("The debug session ended before the %1 arrived.").arg(what);
    if (!state->isStopped()) {
        return QString("The target resumed before the %1 arrived (current state: %2).")
            .arg(what, DebuggerEngine::stateName(state->engine()->state()));
    }
    return QString("The %1 of stop %2 did not arrive in time. Call again, or pass a larger "
                   "\"timeout_ms\".").arg(what).arg(state->stopId());
}

// Operations

enum class OperationKind { Resume, SelectThread, SelectFrame };
enum class OperationState { Accepted, InProgress, Completed, Failed };

static QString toString(OperationState state)
{
    switch (state) {
    case OperationState::Accepted:   return "accepted";
    case OperationState::InProgress: return "in_progress";
    case OperationState::Completed:  return "completed";
    case OperationState::Failed:     return "failed";
    }
    return {};
}

class OperationStatus
{
public:
    OperationState state;
    QString reason;

    bool isFinal() const
    {
        return state == OperationState::Completed || state == OperationState::Failed;
    }
};

class Operation
{
public:
    int id = 0;
    QString name;
    OperationKind kind = OperationKind::Resume;
    QPointer<McpSessionState> state;
    QString sessionId;
    int baseStopId = 0;
    int baseRunFailures = 0;
    QString threadId;
    int frameLevel = -1;
    QString message;
    std::optional<OperationStatus> finalStatus;
};

static QMap<int, Operation> &operations()
{
    static QMap<int, Operation> theOperations;
    return theOperations;
}

static int s_nextOperationId = 1;

static OperationStatus evaluateOperation(const Operation &op)
{
    if (op.finalStatus)
        return *op.finalStatus;

    McpSessionState *state = op.state;
    if (!state || !state->engine())
        return {OperationState::Failed, "The debug session ended."};
    if (state->hasEnded()) {
        return {OperationState::Failed, "The debug session ended (state: "
                              + DebuggerEngine::stateName(state->engine()->state()) + ")."};
    }

    if (op.kind == OperationKind::Resume) {
        // A debugger can retry a run it reported as failed, so the run failed
        // only if the target is still at the stop it was at.
        if (state->stopId() == op.baseStopId && state->runFailures() != op.baseRunFailures
            && state->isStopped()) {
            return {OperationState::Failed, "The target did not resume."};
        }
        if (state->stopId() == op.baseStopId) {
            if (state->isStopped())
                return {OperationState::Accepted, "The target has not started running yet."};
            return {OperationState::InProgress, "The target is running."};
        }
        if (!state->isStopped())
            return {OperationState::InProgress, "The target is running."};
        if (!state->isStackReady())
            return {OperationState::InProgress, "The call stack of the new stop has not arrived yet."};
        if (!state->isLocalsReady())
            return {OperationState::InProgress, "The local variables of the new stop have not arrived yet."};
        return {OperationState::Completed, {}};
    }

    if (state->stopId() != op.baseStopId || !state->isStopped())
        return {OperationState::Failed, "The target resumed before the selection completed."};
    if (op.kind == OperationKind::SelectThread && state->currentThreadId() != op.threadId) {
        return {OperationState::Failed,
                QString("Thread %1 was selected in the meantime.").arg(state->currentThreadId())};
    }
    // Engines select a frame right away, so another one being current once
    // the stack is there means the selection did not take or was replaced.
    if (op.kind == OperationKind::SelectFrame && state->isStackReady()
        && state->currentFrameLevel() != op.frameLevel) {
        return {OperationState::Failed, QString("Frame %1 is selected instead of frame %2.")
                                            .arg(state->currentFrameLevel())
                                            .arg(op.frameLevel)};
    }
    if (!state->isStackReady())
        return {OperationState::InProgress, "The call stack has not arrived yet."};
    if (!state->isLocalsReady())
        return {OperationState::InProgress, "The local variables have not arrived yet."};
    return {OperationState::Completed, {}};
}

static OperationStatus updateOperation(Operation &op)
{
    const OperationStatus status = evaluateOperation(op);
    if (status.isFinal() && !op.finalStatus)
        op.finalStatus = status;
    return status;
}

static QJsonObject operationReport(Operation &op, bool timedOut)
{
    const OperationStatus status = updateOperation(op);
    QJsonObject report{{"operation_id", op.id},
                       {"operation", op.name},
                       {"session_id", op.sessionId},
                       {"status", timedOut && !status.isFinal() ? QString("timed_out")
                                                                : toString(status.state)},
                       {"message", op.message}};
    if (!status.reason.isEmpty())
        report["reason"] = status.reason;
    if (timedOut && !status.isFinal()) {
        report["operation_status"] = toString(status.state);
        report["reason"] = QString("The wait ended before the operation completed. The "
                                   "operation was not cancelled. %1").arg(status.reason);
    }
    if (op.state) {
        report["context"] = op.state->context();
        report["data_ready"] = op.state->readiness();
    }
    return report;
}

static void waitForOperation(int id, int timeoutMs, const McpReply &reply)
{
    auto it = operations().find(id);
    if (it == operations().end()) {
        reply(ResultError(QString("No operation with id %1. Only the most recent operations "
                                  "are kept.").arg(id)));
        return;
    }
    if (updateOperation(*it).isFinal() || !it->state || timeoutMs == 0) {
        reply(operationReport(*it, false));
        return;
    }
    waitUntil(it->state, timeoutMs,
              [id] {
                  auto it = operations().find(id);
                  return it == operations().end() || updateOperation(*it).isFinal();
              },
              [id, reply](WaitOutcome outcome) {
                  auto it = operations().find(id);
                  if (it == operations().end()) {
                      reply(ResultError(QString("Operation %1 was discarded.").arg(id)));
                      return;
                  }
                  reply(operationReport(*it, outcome == WaitOutcome::TimedOut));
              });
}

static void startOperation(const QString &name,
                           OperationKind kind,
                           const std::function<Result<QString>()> &request,
                           const QJsonObject &args,
                           const McpReply &reply,
                           const QString &threadId = {},
                           int frameLevel = -1)
{
    const Result<McpSessionState *> state = activeSessionState();
    if (!state) {
        reply(ResultError(state.error()));
        return;
    }
    const int baseStopId = (*state)->stopId();
    const int baseRunFailures = (*state)->runFailures();
    const Result<QString> message = request();
    if (!message) {
        reply(ResultError(message.error()));
        return;
    }

    Operation op;
    op.id = s_nextOperationId++;
    op.name = name;
    op.kind = kind;
    op.state = *state;
    op.sessionId = (*state)->sessionId();
    op.baseStopId = baseStopId;
    op.baseRunFailures = baseRunFailures;
    op.threadId = threadId;
    op.frameLevel = frameLevel;
    op.message = *message;
    operations().insert(op.id, op);
    while (operations().size() > 100)
        operations().erase(operations().begin());

    if (!args.value("wait_for_completion").toBool(false)) {
        reply(operationReport(operations()[op.id], false));
        return;
    }
    waitForOperation(op.id, timeoutArgument(args, defaultOperationTimeoutMs), reply);
}


static Result<DebuggerEngine *> getActiveEngine()
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("No active debug session");
    return engine.data();
}

static Result<WatchHandler *> getWatchHandler()
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("No active debug session");
    if (engine->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName(engine->state()) + ")");
    return engine->watchHandler();
}

static QJsonObject watchItemToJson(const WatchItem *item)
{
    QJsonObject obj;
    obj["iname"] = item->iname;
    obj["name"] = item->name;
    obj["value"] = item->value;
    item->updateValueCache();
    obj["display_value"] = item->valueCache;
    obj["type"] = item->type;
    obj["value_editable"] = item->valueEditable;
    obj["has_children"] = item->wantsChildren || item->childCount() > 0;
    if (item->address != 0)
        obj["address"] = QString("0x%1").arg(item->address, 0, 16);
    return obj;
}

static Result<ThreadsHandler *> getThreadsHandler()
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("No active debug session");
    if (engine->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName(engine->state()) + ")");
    return engine->threadsHandler();
}

static QString breakpointTypeToString(BreakpointType type)
{
    switch (type) {
    case BreakpointByFileAndLine:      return "fileAndLine";
    case BreakpointByFunction:         return "function";
    case BreakpointByAddress:          return "address";
    case BreakpointAtThrow:            return "throw";
    case BreakpointAtCatch:            return "catch";
    case BreakpointAtMain:             return "main";
    case BreakpointAtFork:             return "fork";
    case BreakpointAtExec:             return "exec";
    case BreakpointAtSysCall:          return "syscall";
    case WatchpointAtAddress:          return "watchAddress";
    case WatchpointAtExpression:       return "watchExpression";
    case BreakpointOnQmlSignalEmit:    return "qmlSignal";
    case BreakpointAtJavaScriptThrow:  return "jsThrow";
    default:                           return "unknown";
    }
}

static Result<QString> startDebug(const QJsonObject &args)
{
    const Result<> canRun = ProjectExplorer::ProjectExplorerPlugin::canRunStartupProject(
        ProjectExplorer::Constants::DEBUG_RUN_MODE);
    if (!canRun)
        return ResultError(canRun.error());
    // The flag is only consumed once a run reaches fixupParameters(), which a build
    // failure prevents. Assign it either way so a stale one cannot survive.
    DebuggerRunParameters::setBreakOnMainNextTime(args.value("break_at_main").toBool(false));
    ProjectExplorer::ProjectExplorerPlugin::runStartupProject(
        ProjectExplorer::Constants::DEBUG_RUN_MODE);
    return QString("Debug session start requested for the startup project.");
}

static Result<QString> startDebugExecutable(const QJsonObject &args)
{
    const FilePath executable = FilePath::fromUserInput(args.value("executable").toString());
    if (!executable.exists())
        return ResultError(QString("Executable not found: %1").arg(executable.toUserOutput()));

    const QString kitId = args.value("kit").toString();
    ProjectExplorer::Kit *kit = kitId.isEmpty()
                                    ? ProjectExplorer::KitManager::defaultKit()
                                    : ProjectExplorer::KitManager::kit(Id::fromString(kitId));
    if (!kit) {
        return ResultError(kitId.isEmpty() ? QString("No default kit available.")
                                           : QString("No kit with id \"%1\".").arg(kitId));
    }

    QStringList arguments;
    for (const QJsonValue &v : args.value("arguments").toArray())
        arguments << v.toString();

    const bool qmlDebugging = args.value("qml_debugging").toBool(false);
    const QString remoteChannel = args.value("remote_channel").toString();
    if (!remoteChannel.isEmpty() && qmlDebugging) {
        return ResultError(QString("\"qml_debugging\" is not supported when attaching to a "
                                   "remote server."));
    }
    if (args.value("native_mixed").toBool(false) && !qmlDebugging) {
        return ResultError(QString("\"native_mixed\" needs \"qml_debugging\" as well: the "
                                   "combined engine debugs C++ and QML in one session."));
    }

    const Utils::Id runMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto runControl = new ProjectExplorer::RunControl(runMode);
    runControl->setKit(kit);
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    ProcessRunData inferior;
    inferior.command = CommandLine(executable, arguments);
    const QString workingDir = args.value("working_directory").toString();
    inferior.workingDirectory = workingDir.isEmpty() ? executable.parentDir()
                                                     : FilePath::fromUserInput(workingDir);
    rp.setInferior(inferior);
    rp.setQmlDebugging(qmlDebugging);
    // An external run has no DebuggerRunConfigurationAspect to take the combined
    // engine from, so it says so itself. QTC_DEBUGGER_NATIVE_MIXED still wins.
    if (args.contains("native_mixed"))
        rp.setNativeMixedEnabled(args.value("native_mixed").toBool(false));
    DebuggerRunParameters::setBreakOnMainNextTime(false);
    rp.setBreakOnMain(args.value("break_at_main").toBool(false));
    if (remoteChannel.isEmpty()) {
        // Locally launched inferiors inherit Creator's environment, so GUI apps
        // find DISPLAY etc.
        rp.setInferiorEnvironment(Utils::Environment::systemEnvironment());
        rp.setStartMode(StartExternal);
        rp.setDisplayName(QString("External: %1").arg(executable.fileName()));
    } else {
        // Attach to an already-running gdbserver/stub; the executable supplies symbols.
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel(remoteChannel);
        rp.setCloseMode(KillAtClose);
        // With target extended-remote (a "gdbserver --multi" style server) the
        // program is launched by us via run, so its arguments apply; plain
        // target remote has it already started, so continue.
        const bool extended = args.value("extended_remote").toBool(false);
        rp.setUseExtendedRemote(extended);
        rp.setUseContinueInsteadOfRun(!extended);
        rp.setDisplayName(QString("Attach to %1").arg(remoteChannel));
    }
    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();
    if (!remoteChannel.isEmpty())
        return QString("Attach to remote server %1 requested.").arg(remoteChannel);
    return QString("Debug session start requested for %1.").arg(executable.toUserOutput());
}

static QString stopDebug()
{
    QStringList results;
    results.append("=== STOP DEBUGGING ===");

    Core::ActionManager *actionManager = Core::ActionManager::instance();
    if (!actionManager) {
        results.append("ERROR: ActionManager not available");
        return results.join("\n");
    }

    QStringList stopActionIds
        = {"Debugger.StopDebugger",
           "Debugger.Stop",
           "ProjectExplorer.StopDebugging",
           "ProjectExplorer.Stop",
           "Debugger.StopDebugging"};

    bool actionTriggered = false;
    for (const QString &actionId : stopActionIds) {
        results.append("Trying stop debug action: " + actionId);

        Core::Command *command = actionManager->command(Utils::Id::fromString(actionId));
        if (command && command->action()) {
            results.append("Found stop debug action, triggering...");
            command->action()->trigger();
            results.append("Stop debug action triggered successfully");
            actionTriggered = true;
            break;
        } else {
            results.append("Stop debug action not found: " + actionId);
        }
    }

    if (!actionTriggered) {
        results.append("WARNING: No stop debug action found among tried IDs");
        results.append(
            "You may need to stop debugging manually from Qt Creator's debugger interface");
    }

    results.append("");
    results.append("=== STOP DEBUG RESULT ===");
    results.append("Stop debug command completed.");

    return results.join("\n");
}

static Result<QString> debuggerStepOver()
{
    const auto engine = getActiveEngine();
    if (!engine)
        return ResultError(engine.error());
    if ((*engine)->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName((*engine)->state()) + ")");
    (*engine)->handleExecStepOver();
    return QString("Step over executed");
}

static Result<QString> debuggerStepIn()
{
    const auto engine = getActiveEngine();
    if (!engine)
        return ResultError(engine.error());
    if ((*engine)->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName((*engine)->state()) + ")");
    (*engine)->handleExecStepIn();
    return QString("Step in executed");
}

static Result<QString> debuggerStepOut()
{
    const auto engine = getActiveEngine();
    if (!engine)
        return ResultError(engine.error());
    if ((*engine)->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName((*engine)->state()) + ")");
    (*engine)->handleExecStepOut();
    return QString("Step out executed");
}

static Result<QString> debuggerContinue()
{
    const auto engine = getActiveEngine();
    if (!engine)
        return ResultError(engine.error());
    if ((*engine)->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName((*engine)->state()) + ")");
    (*engine)->handleExecContinue();
    return QString("Continue executed");
}

static Result<QString> debuggerInterrupt()
{
    const auto engine = getActiveEngine();
    if (!engine)
        return ResultError(engine.error());
    if ((*engine)->state() != InferiorRunOk)
        return ResultError("Debugger is not running (current state: "
                           + DebuggerEngine::stateName((*engine)->state()) + ")");
    (*engine)->handleExecInterrupt();
    return QString("Interrupt requested");
}

static Result<QString> debuggerRunToLine(const QString &file, int line)
{
    const auto engine = getActiveEngine();
    if (!engine)
        return ResultError(engine.error());
    if (file.isEmpty() || line <= 0)
        return ResultError("Requires \"file\" and a 1-based \"line\".");
    if ((*engine)->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName((*engine)->state()) + ")");
    if (!(*engine)->hasCapability(RunToLineCapability))
        return ResultError("The current debugger engine cannot run to a line.");
    ContextData data;
    data.type = LocationByFile;
    data.fileName = FilePath::fromUserInput(file);
    data.textPosition.line = line;
    (*engine)->runToLine(data);
    return QString("Running to %1:%2").arg(data.fileName.toUserOutput()).arg(line);
}

static Result<QJsonObject> debuggerGetStatus(bool includeLog)
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();

    QJsonObject result;
    if (!engine) {
        result["has_session"] = false;
        result["state"] = "none";
        return result;
    }

    result["has_session"] = true;
    result["state"] = DebuggerEngine::stateName(engine->state());
    const McpSessionState *session = McpSessionState::forEngine(engine);
    result["context"] = session->context();
    result["data_ready"] = session->readiness();
    if (includeLog)
        result["log"] = engine->logContents();

    const bool isPaused = engine->state() == InferiorStopOk;
    const bool isRunning = engine->state() == InferiorRunOk;
    result["is_paused"] = isPaused;
    result["is_running"] = isRunning;

    if (isPaused) {
        const StackHandler *handler = engine->stackHandler();
        if (handler && handler->isContentsValid() && handler->stackSize() > 0) {
            const StackFrame frame = handler->frameAt(handler->currentIndex());
            QJsonObject pos;
            if (!frame.file.isEmpty())
                pos["file"] = frame.file.toUserOutput();
            if (frame.line >= 0)
                pos["line"] = frame.line;
            if (!frame.function.isEmpty())
                pos["function"] = frame.function;
            result["current_position"] = pos;
        }
    }

    return result;
}

static void evaluateExpression(
    const QString &expression, std::function<void(Result<QJsonObject>)> callback)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler) {
        callback(ResultError(handler.error()));
        return;
    }

    (*handler)->watchExpression(expression, expression);
    const QString iname = (*handler)->watcherName(expression);

    WatchModelBase *model = (*handler)->model();
    QObject::connect(
        model,
        &WatchModelBase::updateFinished,
        model,
        [handler, iname, expression, callback]() {
            WatchItem *item = (*handler)->findItem(iname);
            if (!item) {
                callback(ResultError("Expression evaluation failed: " + expression));
                return;
            }
            QJsonObject result;
            result["expression"] = expression;
            result["value"] = item->value;
            result["type"] = item->type;
            if (item->address != 0)
                result["address"] = QString("0x%1").arg(item->address, 0, 16);
            callback(result);
            (*handler)->removeItemByIName(iname);
        },
        Qt::SingleShotConnection);
}

static void getVariables(bool includeWatchers, int timeoutMs, const McpReply &reply)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler) {
        reply(ResultError(handler.error()));
        return;
    }

    QStringList rootInames = {"local"};
    if (includeWatchers)
        rootInames.append("watch");

    const QPointer<McpSessionState> session = McpSessionState::forEngine(
        EngineManager::currentEngine());
    const auto buildResult = [handler, rootInames, session]() -> QJsonObject {
        QJsonArray result;
        for (const QString &rootIname : rootInames) {
            WatchItem *root = (*handler)->findItem(rootIname);
            if (!root)
                continue;
            root->forFirstLevelChildren([&](WatchItem *item) {
                QJsonObject obj = watchItemToJson(item);
                if (item->childCount() > 0) {
                    QJsonArray children;
                    item->forFirstLevelChildren([&](WatchItem *child) {
                        children.append(watchItemToJson(child));
                    });
                    obj["children"] = children;
                }
                result.append(obj);
            });
        }
        return QJsonObject{{"variables", result}, {"context", session->context()}};
    };

    waitUntil(session, timeoutMs,
              [session] { return session && session->isLocalsReady(); },
              [session, reply, buildResult](WaitOutcome outcome) {
                  if (outcome != WaitOutcome::Ready) {
                      reply(ResultError(notReadyMessage("local variables", session,
                                                        outcome == WaitOutcome::SessionEnded)));
                      return;
                  }
                  reply(buildResult());
              });
}

static void getVariable(const QString &iname, int timeoutMs, const McpReply &reply)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler) {
        reply(ResultError(handler.error()));
        return;
    }

    const QPointer<McpSessionState> session = McpSessionState::forEngine(
        EngineManager::currentEngine());
    const auto buildResult = [handler, iname, session]() -> Result<QJsonObject> {
        WatchItem *item = (*handler)->findItem(iname);
        if (!item)
            return ResultError("Variable no longer available: " + iname);
        QJsonObject obj = watchItemToJson(item);
        if (item->wantsChildren || item->childCount() > 0) {
            QJsonArray children;
            item->forFirstLevelChildren([&](WatchItem *child) {
                children.append(watchItemToJson(child));
            });
            obj["children"] = children;
        }
        if (!session)
            return ResultError("The debug session ended.");
        return QJsonObject{{"variable", obj}, {"context", session->context()}};
    };

    const QDeadlineTimer deadline(timeoutMs);
    const auto fetch = [handler, iname, reply, buildResult, session, deadline] {
        WatchItem *item = (*handler)->findItem(iname);
        if (!item) {
            reply(ResultError("No variable with iname: " + iname));
            return;
        }
        if (item->childCount() > 0 || !item->wantsChildren) {
            reply(buildResult());
            return;
        }
        const int updatesBefore = session->localsUpdates();
        (*handler)->fetchMore(iname);
        waitUntil(session, int(deadline.remainingTime()),
                  [session, updatesBefore] {
                      return session && session->isLocalsReady()
                             && session->localsUpdates() > updatesBefore;
                  },
                  [session, reply, buildResult, iname](WaitOutcome outcome) {
                      if (outcome != WaitOutcome::Ready) {
                          reply(ResultError(notReadyMessage("children of " + iname, session,
                                                            outcome == WaitOutcome::SessionEnded)));
                          return;
                      }
                      reply(buildResult());
                  });
    };

    waitUntil(session, timeoutMs,
              [session] { return session && session->isLocalsReady(); },
              [session, reply, fetch](WaitOutcome outcome) {
                  if (outcome != WaitOutcome::Ready) {
                      reply(ResultError(notReadyMessage("local variables", session,
                                                        outcome == WaitOutcome::SessionEnded)));
                      return;
                  }
                  fetch();
              });
}

static Result<bool> setVariable(const QString &iname, const QString &value)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler)
        return ResultError(handler.error());

    WatchItem *item = (*handler)->findItem(iname);
    if (!item)
        return ResultError("No variable with iname: " + iname);
    if (!item->valueEditable)
        return ResultError("Variable is not editable: " + iname);

    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("Debug session ended before value could be assigned");
    engine->assignValueInDebugger(item, item->expression(), QVariant(value));
    return true;
}

static Result<bool> setDisplayFormat(const QString &iname, int format)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler)
        return ResultError(handler.error());
    if (!(*handler)->findItem(iname))
        return ResultError("No variable with iname: " + iname);
    (*handler)->setFormat(iname, format);
    return true;
}

static Result<bool> collapseAllChildren(const QString &iname)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler)
        return ResultError(handler.error());
    if (!(*handler)->findItem(iname))
        return ResultError("No variable with iname: " + iname);
    (*handler)->collapseAllChildren(iname);
    return true;
}

static Result<QJsonArray> expandedINames()
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler)
        return ResultError(handler.error());
    QStringList names = (*handler)->expandedINames().values();
    names.sort();
    QJsonArray result;
    for (const QString &name : std::as_const(names))
        result.append(name);
    return result;
}

static Result<bool> printConsoleMessage(const QString &type, const QString &text)
{
    ConsoleItem::ItemType itemType = ConsoleItem::DefaultType;
    if (type == "error")
        itemType = ConsoleItem::ErrorType;
    else if (type == "warning")
        itemType = ConsoleItem::WarningType;
    else if (type == "debug" || type == "log")
        itemType = ConsoleItem::DebugType;
    debuggerConsole()->printItem(itemType, text);
    return true;
}

static Result<QString> addWatchExpression(const QString &expression, const QString &name)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler)
        return ResultError(handler.error());
    if (expression.isEmpty())
        return ResultError("Expression must not be empty");
    (*handler)->watchExpression(expression, name);
    return (*handler)->watcherName(expression);
}

static Result<bool> removeWatchExpression(const QString &iname)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler)
        return ResultError(handler.error());
    WatchItem *item = (*handler)->findItem(iname);
    if (!item)
        return ResultError("No watch expression with iname: " + iname);
    if (!item->isWatcher())
        return ResultError("Item is not a watch expression: " + iname);
    (*handler)->removeItemByIName(iname);
    return true;
}

static Result<QJsonArray> getThreads()
{
    const Result<ThreadsHandler *> handler = getThreadsHandler();
    if (!handler)
        return ResultError(handler.error());

    const Thread current = (*handler)->currentThread();
    QJsonArray result;
    (*handler)->forItemsAtLevel<1>([&](ThreadItem *item) {
        const ThreadData &d = item->threadData;
        QJsonObject obj;
        obj["id"] = d.id;
        obj["current"] = (current && current->id() == d.id);
        if (!d.name.isEmpty())
            obj["name"] = d.name;
        if (!d.state.isEmpty())
            obj["state"] = d.state;
        if (!d.targetId.isEmpty())
            obj["target_id"] = d.targetId;
        if (!d.details.isEmpty())
            obj["details"] = d.details;
        if (!d.function.isEmpty())
            obj["function"] = d.function;
        if (!d.fileName.isEmpty())
            obj["file"] = d.fileName;
        if (d.lineNumber >= 0)
            obj["line"] = d.lineNumber;
        if (d.address != 0)
            obj["address"] = QString("0x%1").arg(d.address, 0, 16);
        result.append(obj);
    });
    return result;
}

static Result<QString> selectThread(const QString &id)
{
    const Result<ThreadsHandler *> handler = getThreadsHandler();
    if (!handler)
        return ResultError(handler.error());

    const Thread thread = (*handler)->threadForId(id);
    if (!thread)
        return ResultError("No thread with id: " + id);

    (*handler)->setCurrentThread(thread);
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("Debug session ended before thread could be selected");
    engine->selectThread(thread);
    return QString("Thread %1 selection requested").arg(id);
}

static QJsonArray callStackFrames(const StackHandler *handler, int maxFrames = -1)
{
    QJsonArray frames;
    const int count = maxFrames < 0 ? handler->stackSize()
                                    : std::min(handler->stackSize(), maxFrames);
    const int currentIndex = handler->currentIndex();
    for (int i = 0; i < count; ++i) {
        const StackFrame frame = handler->frameAt(i);
        QJsonObject obj;
        obj["level"] = i;
        obj["current"] = (i == currentIndex);
        if (!frame.function.isEmpty())
            obj["function"] = frame.function;
        if (!frame.file.isEmpty())
            obj["file"] = frame.file.toUserOutput();
        if (frame.line >= 0)
            obj["line"] = frame.line;
        if (frame.address != 0)
            obj["address"] = QString("0x%1").arg(frame.address, 0, 16);
        if (!frame.module.isEmpty())
            obj["module"] = frame.module;
        frames.append(obj);
    }
    return frames;
}

static void getCallStack(const QJsonObject &args, const McpReply &reply)
{
    const Result<McpSessionState *> state = pausedSessionState();
    if (!state) {
        reply(ResultError(state.error()));
        return;
    }
    const QPointer<McpSessionState> session = *state;
    waitUntil(session, timeoutArgument(args, defaultDataTimeoutMs),
              [session] { return session && session->isStackReady(); },
              [session, reply](WaitOutcome outcome) {
                  if (outcome != WaitOutcome::Ready) {
                      reply(ResultError(notReadyMessage("call stack", session,
                                                        outcome == WaitOutcome::SessionEnded)));
                      return;
                  }
                  reply(QJsonObject{{"frames", callStackFrames(session->engine()->stackHandler())},
                                    {"context", session->context()}});
              });
}

static Result<QString> selectFrame(int level)
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("No active debug session");

    if (engine->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName(engine->state()) + ")");

    StackHandler *handler = engine->stackHandler();
    if (!handler || !handler->isContentsValid())
        return ResultError("Call stack is not available");

    if (level < 0 || level >= handler->stackSize())
        return ResultError("Invalid frame level: " + QString::number(level));

    engine->activateFrame(level);
    return QString("Frame %1 selection requested").arg(level);
}

static bool deleteBreakpoint(int id)
{
    const GlobalBreakpoints bps = BreakpointManager::globalBreakpoints();
    for (const GlobalBreakpoint &gbp : bps) {
        if (gbp && gbp->modelId() == id) {
            gbp->deleteBreakpoint();
            return true;
        }
    }
    return false;
}

static QJsonArray getBreakpoints()
{
    QJsonArray result;
    const GlobalBreakpoints bps = BreakpointManager::globalBreakpoints();
    for (const GlobalBreakpoint &gbp : bps) {
        if (!gbp)
            continue;
        const BreakpointParameters &p = gbp->requestedParameters();
        QJsonObject obj;
        obj["id"] = gbp->modelId();
        obj["type"] = breakpointTypeToString(p.type);
        obj["enabled"] = p.enabled;
        if (!p.fileName.isEmpty())
            obj["file"] = p.fileName.toUserOutput();
        if (p.textPosition.line > 0)
            obj["line"] = p.textPosition.line;
        if (!p.functionName.isEmpty())
            obj["function"] = p.functionName;
        if (p.address != 0)
            obj["address"] = QString("0x%1").arg(p.address, 0, 16);
        if (!p.condition.isEmpty())
            obj["condition"] = p.condition;
        if (p.ignoreCount != 0)
            obj["ignore_count"] = p.ignoreCount;
        if (p.oneShot)
            obj["one_shot"] = true;
        if (!p.message.isEmpty())
            obj["message"] = p.message;
        result.append(obj);
    }
    return result;
}

static QJsonObject addBreakpoint(
    const QString &type,
    const QString &file,
    int line,
    const QString &functionName,
    quint64 address,
    const QString &condition,
    int ignoreCount,
    bool enabled,
    bool oneShot)
{
    BreakpointType bpType = BreakpointByFileAndLine;
    if (type == "function")
        bpType = BreakpointByFunction;
    else if (type == "address")
        bpType = BreakpointByAddress;
    else if (type == "throw")
        bpType = BreakpointAtThrow;
    else if (type == "catch")
        bpType = BreakpointAtCatch;
    else if (type == "main")
        bpType = BreakpointAtMain;
    else if (type == "watchAddress")
        bpType = WatchpointAtAddress;
    else if (type == "watchExpression")
        bpType = WatchpointAtExpression;

    BreakpointParameters params(bpType);
    params.enabled = enabled;
    params.oneShot = oneShot;
    if (!file.isEmpty())
        params.fileName = Utils::FilePath::fromUserInput(file);
    if (line > 0)
        params.textPosition.line = line;
    if (!functionName.isEmpty())
        params.functionName = functionName;
    if (address != 0)
        params.address = address;
    if (!condition.isEmpty())
        params.condition = condition;
    if (ignoreCount > 0)
        params.ignoreCount = ignoreCount;

    const GlobalBreakpoint gbp = BreakpointManager::createBreakpoint(params);
    if (!gbp)
        return QJsonObject{{"success", false}, {"error", "Failed to create breakpoint"}};

    return QJsonObject{{"success", true}, {"id", gbp->modelId()}};
}

void registerMcpTools()
{
    McpSessionState::startTracking();

    using namespace Mcp::Schema;
    namespace Schema = Mcp::Schema;
    using Mcp::ToolInterface;
    using Mcp::ToolRegistry;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    const auto operationInputs = [](Tool::InputSchema schema) {
        return schema
            .addProperty(
                "wait_for_completion",
                QJsonObject{
                    {"type", "boolean"},
                    {"default", false},
                    {"description",
                     "Wait until the operation has completed and the data it changes (call "
                     "stack, local variables) has been refreshed, instead of returning once "
                     "the request was accepted. Default false."}})
            .addProperty("timeout_ms", timeoutSchema(defaultOperationTimeoutMs));
    };
    const auto operationOutputs = [] {
        return Tool::OutputSchema{}
            .addProperty("operation_id", QJsonObject{{"type", "integer"},
                                                     {"description", "Pass to debugger_wait_for_operation."}})
            .addProperty("operation", QJsonObject{{"type", "string"}})
            .addProperty("session_id", QJsonObject{{"type", "string"}})
            .addProperty(
                "status",
                QJsonObject{
                    {"type", "string"},
                    {"enum", QJsonArray{toString(OperationState::Accepted),
                                        toString(OperationState::InProgress),
                                        toString(OperationState::Completed),
                                        toString(OperationState::Failed), "timed_out"}},
                    {"description",
                     "accepted: requested, nothing observed yet. in_progress: the target runs "
                     "or its data is still loading. completed: done, and the data reflects it. "
                     "failed: see reason. timed_out: the wait ended first; the operation was not "
                     "cancelled and may still complete."}})
            .addProperty("operation_status",
                         QJsonObject{{"type", "string"},
                                     {"description", "With status timed_out, the status the operation had then."}})
            .addProperty("message", QJsonObject{{"type", "string"}})
            .addProperty("reason", QJsonObject{{"type", "string"}})
            .addProperty("context", contextSchema())
            .addProperty("data_ready",
                         QJsonObject{{"type", "object"},
                                     {"description", "Whether the call stack and the local variables belong to the current stop, thread and frame."}})
            .addRequired("operation_id")
            .addRequired("status")
            .addRequired("message");
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_get_breakpoints")
            .title("Get current breakpoints")
            .description("Returns all breakpoints currently set in Qt Creator's debugger.")
            .annotations(ToolAnnotations{}.readOnlyHint(true).destructiveHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "breakpoints",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "List of breakpoints."}})
                    .addRequired("breakpoints")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"breakpoints", getBreakpoints()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_get_threads")
            .title("Get current threads")
            .description(
                "Returns all threads of the current debug session. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema([] {
                const QJsonObject threadProperties{
                    {"id",       QJsonObject{{"type", "string"},  {"description", "Thread ID."}}},
                    {"current",  QJsonObject{{"type", "boolean"}, {"description", "True for the currently selected thread."}}},
                    {"name",     QJsonObject{{"type", "string"},  {"description", "Thread name."}}},
                    {"state",    QJsonObject{{"type", "string"},  {"description", "Thread state, such as \"stopped\"."}}},
                    {"target_id", QJsonObject{{"type", "string"},  {"description", "Target-level thread identifier."}}},
                    {"details",  QJsonObject{{"type", "string"},  {"description", "Additional details from the debugger."}}},
                    {"function", QJsonObject{{"type", "string"},  {"description", "Current function name."}}},
                    {"file",     QJsonObject{{"type", "string"},  {"description", "Current source file."}}},
                    {"line",     QJsonObject{{"type", "integer"}, {"description", "Current line number."}}},
                    {"address",  QJsonObject{{"type", "string"},  {"description", "Current instruction address."}}},
                };
                const QJsonObject threadItem{
                    {"type", "object"},
                    {"required", QJsonArray{"id", "current"}},
                    {"properties", threadProperties},
                };
                return Tool::OutputSchema{}
                    .addProperty("context", contextSchema())
                    .addProperty(
                        "threads",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "List of threads."},
                            {"items", threadItem}})
                    .addRequired("threads");
            }()),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const Utils::Result<QJsonArray> threads = getThreads();
            if (!threads)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(threads.error()));
            QJsonObject result{{"threads", *threads}};
            if (const Utils::Result<McpSessionState *> state = activeSessionState())
                result["context"] = (*state)->context();
            return CallToolResult{}.isError(false).structuredContent(result);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_select_thread")
            .title("Select a thread")
            .description(
                "Switches the current thread in the active debug session. The call stack and "
                "variables of the new thread arrive asynchronously: pass wait_for_completion to "
                "wait for them, or use debugger_wait_for_operation. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).idempotentHint(true))
            .inputSchema(
                operationInputs(Tool::InputSchema{})
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Thread ID to select (as returned by debugger_get_threads)."}})
                    .addRequired("id"))
            .outputSchema(operationOutputs().addProperty("success", QJsonObject{{"type", "boolean"}})),
        [](const QJsonObject &args, const McpReply &reply) {
            const QString id = args.value("id").toString();
            startOperation("select_thread", OperationKind::SelectThread,
                           [id] { return selectThread(id); }, args,
                           [reply](const Utils::Result<QJsonObject> &result) {
                               if (!result) {
                                   reply(result);
                                   return;
                               }
                               QJsonObject report = *result;
                               report["success"] = report.value("status").toString()
                                                   != toString(OperationState::Failed);
                               reply(report);
                           },
                           id);
        });

    const auto varItemSchema = [] {
        return QJsonObject{
            {"type", "object"},
            {"required", QJsonArray{"iname", "name", "value", "type", "value_editable", "has_children"}},
            {"properties", QJsonObject{
                {"iname",          QJsonObject{{"type", "string"},  {"description", "Internal name, such as \"local.myVar\". Use as key for debugger_get_variable / debugger_set_variable."}}},
                {"name",           QJsonObject{{"type", "string"},  {"description", "Display name."}}},
                {"value",          QJsonObject{{"type", "string"},  {"description", "Raw value as reported by the debugger."}}},
                {"display_value",  QJsonObject{{"type", "string"},  {"description", "Value as shown in the Locals view, honoring the display format set with debugger_set_display_format."}}},
                {"type",           QJsonObject{{"type", "string"},  {"description", "Type name."}}},
                {"address",        QJsonObject{{"type", "string"},  {"description", "Memory address, such as \"0x1234\"."}}},
                {"value_editable", QJsonObject{{"type", "boolean"}, {"description", "Whether the value can be changed with debugger_set_variable."}}},
                {"has_children",   QJsonObject{{"type", "boolean"}, {"description", "Whether the variable has child members."}}},
            }},
        };
    };

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_get_variables")
            .title("List local variables")
            .description(
                "Returns local variables for the current stack frame. "
                "Optionally includes watch expressions. "
                "Variables with has_children=true may include a children array if already "
                "expanded. Otherwise call debugger_get_variable with the variable's iname to "
                "retrieve sub-fields. Waits until the variables of the current stop, thread and "
                "frame have arrived, and reports an error instead of older values if they do not "
                "arrive in time. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "include_watchers",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Also return watch expressions (default: false)."},
                            {"default", false}})
                    .addProperty("timeout_ms", timeoutSchema(defaultDataTimeoutMs)))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "variables",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "List of variables."},
                            {"items", varItemSchema()}})
                    .addProperty("context", contextSchema())
                    .addRequired("variables")
                    .addRequired("context")),
        [](const QJsonObject &args, const McpReply &reply) {
            getVariables(args.value("include_watchers").toBool(false),
                         timeoutArgument(args, defaultDataTimeoutMs), reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_get_variable")
            .title("Get a variable")
            .description(
                "Returns the details of a single variable by its iname, including its children "
                "if it has any (such as struct members or array elements). "
                "If a child also has has_children=true, call debugger_get_variable again with that child's iname "
                "to retrieve its sub-fields. Waits until the variables of the current stop have "
                "arrived. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Internal name of the variable (such as \"local.myVar\")."}})
                    .addProperty("timeout_ms", timeoutSchema(defaultDataTimeoutMs))
                    .addRequired("iname"))
            .outputSchema([] {
                QJsonObject schema = [] {
                    QJsonObject s;
                    s["type"] = "object";
                    s["required"] = QJsonArray{"iname", "name", "value", "type", "value_editable", "has_children"};
                    s["properties"] = QJsonObject{
                        {"iname",          QJsonObject{{"type", "string"}}},
                        {"name",           QJsonObject{{"type", "string"}}},
                        {"value",          QJsonObject{{"type", "string"}}},
                        {"display_value",  QJsonObject{{"type", "string"}}},
                        {"type",           QJsonObject{{"type", "string"}}},
                        {"address",        QJsonObject{{"type", "string"}}},
                        {"value_editable", QJsonObject{{"type", "boolean"}}},
                        {"has_children",   QJsonObject{{"type", "boolean"}}},
                        {"children",       QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "object"}}},
                                                      {"description", "Child members, present when has_children is true."}}},
                    };
                    return s;
                }();
                return Tool::OutputSchema{}
                    .addProperty("variable", schema)
                    .addProperty("context", contextSchema())
                    .addRequired("variable")
                    .addRequired("context");
            }()),
        [](const QJsonObject &args, const McpReply &reply) {
            getVariable(args.value("iname").toString(),
                        timeoutArgument(args, defaultDataTimeoutMs), reply);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_set_variable")
            .title("Set a variable value")
            .description(
                "Changes the value of a variable in the current debug session. "
                "Only works for variables where value_editable is true. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Internal name of the variable (such as \"local.myVar\")."}})
                    .addProperty(
                        "value",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "New value to assign."}})
                    .addRequired("iname")
                    .addRequired("value"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const Utils::Result<bool> ok = setVariable(p.value("iname").toString(), p.value("value").toString());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"success", true}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_collapse_all_children")
            .title("Collapse all children of a variable")
            .description(
                "Recursively clears the expanded state of every descendant of the given variable, "
                "so re-expanding it shows its whole subtree collapsed at all levels. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Internal name of the variable (such as \"local.myVar\")."}})
                    .addRequired("iname"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const Utils::Result<bool> ok = collapseAllChildren(p.value("iname").toString());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"success", true}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_get_expanded_inames")
            .title("List expanded variable inames")
            .description(
                "Returns the sorted set of inames currently marked as expanded in the Locals and "
                "Expressions view. Returns an error if no debug session is active or the debugger "
                "is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "inames",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("inames")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const Utils::Result<QJsonArray> names = expandedINames();
            if (!names)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(names.error()));
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"inames", *names}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_print_console_message")
            .title("Print a message to the QML Debugger Console")
            .description(
                "Appends a message of the given type to the QML Debugger Console, exactly as the "
                "debugger would. Useful for exercising the console's auto-popup behavior. Does not "
                "require an active debug session.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "type",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"error", "warning", "debug", "default"}},
                            {"description", "Message type: error, warning, debug, or default."}})
                    .addProperty(
                        "text",
                        QJsonObject{{"type", "string"}, {"description", "Message text."}})
                    .addRequired("type"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const Utils::Result<bool> ok = printConsoleMessage(p.value("type").toString(),
                                                               p.value("text").toString());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"success", true}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_set_display_format")
            .title("Set a variable's display format")
            .description(
                "Sets the display format of a single variable (by iname) in the current debug "
                "session, as the Locals view context menu does. Use 0 to reset to Automatic. "
                "Common format codes: 0=Automatic, 5=Latin1String, 7=Utf8String, 12=Array of 10, "
                "22=Decimal, 23=Hexadecimal, 24=Binary, 25=Octal. The format is remembered for the "
                "variable's current type only. Returns an error if no debug session is active or "
                "the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).idempotentHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Internal name of the variable (such as \"local.myVar\")."}})
                    .addProperty(
                        "format",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Display format code (0 = Automatic)."}})
                    .addRequired("iname")
                    .addRequired("format"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const Utils::Result<bool> ok = setDisplayFormat(
                p.value("iname").toString(), p.value("format").toInt());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"success", true}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_add_watch_expression")
            .title("Add a watch expression")
            .description(
                "Adds an expression to the watch list in the current debug session. "
                "The expression is evaluated and its value updated as execution progresses. "
                "Returns the iname of the new watch entry (such as \"watch.0\"), which can be used "
                "with debugger_get_variable, debugger_set_variable, and "
                "debugger_remove_watch_expression. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "expression",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Expression to watch (such as \"myVar\", \"ptr->field\")."}})
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Optional display name. Defaults to the expression."}})
                    .addRequired("expression"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{{"type", "string"},
                                    {"description", "Internal name of the watch entry (such as \"watch.0\")."}})
                    .addRequired("iname")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject p = params.argumentsAsObject();
            const Utils::Result<QString> iname = addWatchExpression(
                p.value("expression").toString(), p.value("name").toString());
            if (!iname)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(iname.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"iname", *iname}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_remove_watch_expression")
            .title("Remove a watch expression")
            .description(
                "Removes a watch expression from the current debug session by its iname. "
                "Returns an error if the iname is not found or is not a watch expression, "
                "or if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Internal name of the watch entry to remove (such as \"watch.0\")."}})
                    .addRequired("iname"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const Utils::Result<bool> ok = removeWatchExpression(
                params.argumentsAsObject().value("iname").toString());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"success", true}});
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_get_call_stack")
            .title("Get current call stack")
            .description(
                "Returns the call stack (stack frames) of the selected thread. Waits until the "
                "stack of the current stop and thread has arrived, and reports an error instead "
                "of an older stack if it does not arrive in time. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}.addProperty("timeout_ms", timeoutSchema(defaultDataTimeoutMs)))
            .outputSchema([] {
                const QJsonObject frameProperties{
                    {"level",    QJsonObject{{"type", "integer"}, {"description", "Frame index, 0 = innermost."}}},
                    {"current",  QJsonObject{{"type", "boolean"}, {"description", "True for the currently active frame."}}},
                    {"function", QJsonObject{{"type", "string"},  {"description", "Function or method name."}}},
                    {"file",     QJsonObject{{"type", "string"},  {"description", "Absolute path to the source file."}}},
                    {"line",     QJsonObject{{"type", "integer"}, {"description", "Line number in the source file."}}},
                    {"address",  QJsonObject{{"type", "string"},  {"description", "Instruction address, such as \"0x1234abcd\"."}}},
                    {"module",   QJsonObject{{"type", "string"},  {"description", "Module or shared library name."}}},
                };
                const QJsonObject frameItem{
                    {"type", "object"},
                    {"required", QJsonArray{"level", "current"}},
                    {"properties", frameProperties},
                };
                return Tool::OutputSchema{}
                    .addProperty(
                        "frames",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "Stack frames, innermost first."},
                            {"items", frameItem}})
                    .addProperty("context", contextSchema())
                    .addRequired("frames")
                    .addRequired("context");
            }()),
        [](const QJsonObject &args, const McpReply &reply) { getCallStack(args, reply); });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_select_frame")
            .title("Select a stack frame")
            .description(
                "Switches the current stack frame in the active debug session. "
                "Subsequent debugger_get_variables / debugger_evaluate_expression calls operate on "
                "the selected frame. The variables of the frame arrive asynchronously: pass "
                "wait_for_completion to wait for them, or use debugger_wait_for_operation. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).idempotentHint(true))
            .inputSchema(
                operationInputs(Tool::InputSchema{})
                    .addProperty(
                        "level",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Frame level to select (as returned by debugger_get_call_stack, 0 = innermost)."}})
                    .addRequired("level"))
            .outputSchema(operationOutputs().addProperty("success", QJsonObject{{"type", "boolean"}})),
        [](const QJsonObject &args, const McpReply &reply) {
            const int level = args.value("level").toInt();
            startOperation("select_frame", OperationKind::SelectFrame,
                           [level] { return selectFrame(level); }, args,
                           [reply](const Utils::Result<QJsonObject> &result) {
                               if (!result) {
                                   reply(result);
                                   return;
                               }
                               QJsonObject report = *result;
                               report["success"] = report.value("status").toString()
                                                   != toString(OperationState::Failed);
                               reply(report);
                           },
                           {}, level);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_delete_breakpoint")
            .title("Delete a breakpoint")
            .description(
                "Deletes a breakpoint by its ID (as returned by debugger_get_breakpoints or "
                "debugger_add_breakpoint).")
            .annotations(ToolAnnotations().destructiveHint(true).idempotentHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "ID of the breakpoint to delete."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const bool ok = deleteBreakpoint(p.value("id").toInt());
            return QJsonObject{{"success", ok}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_add_breakpoint")
            .title("Add a breakpoint")
            .description("Adds a new breakpoint in Qt Creator's debugger.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "type",
                        QJsonObject{
                            {"type", "string"},
                            {"enum",
                             QJsonArray{
                                 "fileAndLine",
                                 "function",
                                 "address",
                                 "throw",
                                 "catch",
                                 "main",
                                 "watchAddress",
                                 "watchExpression"}},
                            {"description", "Breakpoint type. Defaults to fileAndLine."},
                            {"default", "fileAndLine"}})
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the source file (for fileAndLine type)."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Line number in the source file (for fileAndLine type)."}})
                    .addProperty(
                        "function",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Function name (for function type)."}})
                    .addProperty(
                        "address",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Memory address (for address or watchAddress type)."}})
                    .addProperty(
                        "condition",
                        QJsonObject{
                            {"type", "string"}, {"description", "Optional condition expression."}})
                    .addProperty(
                        "ignore_count",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Number of hits to ignore before breaking."}})
                    .addProperty(
                        "enabled",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the breakpoint is enabled. Defaults to true."},
                            {"default", true}})
                    .addProperty(
                        "one_shot",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "If true, the breakpoint is removed after the first hit."},
                            {"default", false}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "integer"}, {"description", "ID of the created breakpoint."}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            return addBreakpoint(
                p.value("type").toString("fileAndLine"),
                p.value("file").toString(),
                p.value("line").toInt(0),
                p.value("function").toString(),
                static_cast<quint64>(p.value("address").toInteger(0)),
                p.value("condition").toString(),
                p.value("ignore_count").toInt(0),
                p.value("enabled").toBool(true),
                p.value("one_shot").toBool(false));
        }));

    // Debugger stepping tools
    registerAsyncMcpTool(
        Tool{}
            .name("debugger_step_over")
            .title("Step over")
            .description(
                "Steps over the current line in the debugger. "
                "Pass wait_for_completion to wait until the target has stopped and its call stack and "
                "local variables have been refreshed. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(operationInputs(Tool::InputSchema{}))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            startOperation("step_over", OperationKind::Resume,
                           [] { return debuggerStepOver(); }, args, reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_step_in")
            .title("Step into")
            .description(
                "Steps into the next function call in the debugger. "
                "Pass wait_for_completion to wait until the target has stopped and its call stack and "
                "local variables have been refreshed. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(operationInputs(Tool::InputSchema{}))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            startOperation("step_in", OperationKind::Resume,
                           [] { return debuggerStepIn(); }, args, reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_step_out")
            .title("Step out")
            .description(
                "Steps out of the current function in the debugger. "
                "Pass wait_for_completion to wait until the target has stopped and its call stack and "
                "local variables have been refreshed. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(operationInputs(Tool::InputSchema{}))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            startOperation("step_out", OperationKind::Resume,
                           [] { return debuggerStepOut(); }, args, reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_continue")
            .title("Continue execution")
            .description(
                "Resumes program execution in the debugger until the next breakpoint. "
                "Pass wait_for_completion to wait until the target stops again. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(operationInputs(Tool::InputSchema{}))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            startOperation("continue", OperationKind::Resume,
                           [] { return debuggerContinue(); }, args, reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_interrupt")
            .title("Pause execution")
            .description(
                "Pauses the currently running debuggee. "
                "Pass wait_for_completion to wait until the target has stopped and its call stack and "
                "local variables have been refreshed. "
                "Requires an active debug session that is running.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(operationInputs(Tool::InputSchema{}))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            startOperation("interrupt", OperationKind::Resume,
                           [] { return debuggerInterrupt(); }, args, reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_run_to_line")
            .title("Run to line")
            .description(
                "Resumes execution until it reaches the given 1-based line in the given file, "
                "then stops (like the debugger's \"Run to Line\"). "
                "Pass wait_for_completion to wait until the target has stopped. Requires an active debug "
                "session that is paused and an engine that supports running to a line.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(operationInputs(Tool::InputSchema{})
                             .addProperty(
                                 "file",
                                 QJsonObject{
                                     {"type", "string"},
                                     {"description", "Absolute path of the source file."}})
                             .addProperty(
                                 "line",
                                 QJsonObject{
                                     {"type", "integer"},
                                     {"description", "1-based line to run to."}})
                             .addRequired("file")
                             .addRequired("line"))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            startOperation("run_to_line", OperationKind::Resume,
                           [args] {
                               return debuggerRunToLine(args.value("file").toString(),
                                                        args.value("line").toInt());
                           },
                           args, reply);
        });

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_wait_for_operation")
            .title("Wait for a debugger operation")
            .description(
                "Reports the status of an operation started by a stepping, continue, interrupt, "
                "run-to-line, thread selection or frame selection tool, waiting up to timeout_ms "
                "for it to complete. With timeout_ms 0, reports the status without waiting. A "
                "status of timed_out means the wait ended first: the operation is not cancelled.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("operation_id",
                                 QJsonObject{{"type", "integer"},
                                             {"description", "The operation_id an operation tool returned."}})
                    .addProperty("timeout_ms", timeoutSchema(defaultOperationTimeoutMs))
                    .addRequired("operation_id"))
            .outputSchema(operationOutputs()),
        [](const QJsonObject &args, const McpReply &reply) {
            waitForOperation(args.value("operation_id").toInt(),
                             timeoutArgument(args, defaultOperationTimeoutMs), reply);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_get_status")
            .title("Get debugger status")
            .description(
                "Returns the current status of the debugger including whether a session is active, "
                "its state (paused/running/stopped), and the current position if paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "include_log",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Also return the raw debugger log (the commands exchanged with the "
                             "backend, such as GDB/MI) in a \"log\" field. Default false."},
                            {"default", false}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("has_session", QJsonObject{{"type", "boolean"}})
                    .addProperty("state", QJsonObject{{"type", "string"}})
                    .addProperty("is_paused", QJsonObject{{"type", "boolean"}})
                    .addProperty("is_running", QJsonObject{{"type", "boolean"}})
                    .addProperty("current_position", QJsonObject{{"type", "object"}})
                    .addProperty("context", contextSchema())
                    .addProperty("data_ready", QJsonObject{{"type", "object"}})
                    .addProperty("log", QJsonObject{{"type", "string"}})
                    .addRequired("has_session")
                    .addRequired("state")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const auto result = debuggerGetStatus(
                params.argumentsAsObject().value("include_log").toBool(false));
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(*result);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_evaluate_expression")
            .title("Evaluate expression in debugger")
            .description(
                "Evaluates an expression in the context of the current debug session. "
                "Returns the expression's value and type. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "expression",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Expression to evaluate (such as \"myVar\", \"ptr->field\", \"a + b\")."}})
                    .addRequired("expression"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("expression", QJsonObject{{"type", "string"}})
                    .addProperty("value", QJsonObject{{"type", "string"}})
                    .addProperty("type", QJsonObject{{"type", "string"}})
                    .addRequired("expression")
                    .addRequired("value")),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QString expr = params.argumentsAsObject().value("expression").toString();
            evaluateExpression(expr, [toolInterface](Utils::Result<QJsonObject> result) {
                if (!result)
                    toolInterface.finish(CallToolResult{}.isError(true).addContent(
                        TextContent{}.text(result.error())));
                else
                    toolInterface.finish(
                        CallToolResult{}.isError(false).structuredContent(*result));
            });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_start")
            .title("Start debugging")
            .description(
                "Starts a debug session and returns once the launch has been requested. Poll "
                "debugger_get_status for the session state. With no arguments, debugs the "
                "current startup project using its active run configuration and kit (does not "
                "build first - use the build_project tool beforehand if it may be out of date). If "
                "\"executable\" is given, debugs that executable directly (no project or build "
                "needed) with an optional kit, arguments, working directory and QML debugging. "
                "If \"remote_channel\" is also given, attaches to an already-running gdbserver "
                "or stub at that channel (for example, a bare-metal target) instead of launching "
                "the executable locally. The executable then only supplies symbols.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "executable",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to an executable to debug directly. If omitted, "
                             "the startup project is used."}})
                    .addProperty(
                        "kit",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Kit ID to use (defaults to the default kit). Only used with "
                             "\"executable\"."}})
                    .addProperty(
                        "arguments",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description", "Command-line arguments for the executable."}})
                    .addProperty(
                        "working_directory",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Working directory (defaults to the executable's directory)."}})
                    .addProperty(
                        "qml_debugging",
                        QJsonObject{
                            {"type", "boolean"},
                            {"default", false},
                            {"description",
                             "Enable QML debugging. Cannot be combined with "
                             "\"remote_channel\"."}})
                    .addProperty(
                        "native_mixed",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Debug C++ and QML in one native combined session instead of "
                             "starting a separate QML engine. Needs \"qml_debugging\" as well. "
                             "The QTC_DEBUGGER_NATIVE_MIXED environment variable overrides "
                             "this."}})
                    .addProperty(
                        "remote_channel",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Attach to an already-running gdbserver/stub at this channel "
                             "(such as \"localhost:1234\" or \"tcp:localhost:1234\", CDB kits need "
                             "the cdb form, such as \"tcp:server=localhost,port=1234\") instead of "
                             "launching the executable. Requires \"executable\" for symbols."}})
                    .addProperty(
                        "extended_remote",
                        QJsonObject{
                            {"type", "boolean"},
                            {"default", false},
                            {"description",
                             "Use \"target extended-remote\" for \"remote_channel\" (a "
                             "\"gdbserver --multi\" server): the program is launched with run, so "
                             "\"arguments\" are passed to it, instead of continuing an "
                             "already-started process."}})
                    .addProperty(
                        "break_at_main",
                        QJsonObject{
                            {"type", "boolean"},
                            {"default", false},
                            {"description",
                             "Set a temporary breakpoint at main and stop there. Without "
                             "\"executable\" it applies to the next debug start, which a "
                             "pending build may delay. When attaching to a remote server it "
                             "is honored by GDB and CDB kits only."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString executable = args.value("executable").toString();
            if (executable.isEmpty() && !args.value("remote_channel").toString().isEmpty()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "\"remote_channel\" requires \"executable\" to supply symbols."));
            }
            const Utils::Result<QString> result = executable.isEmpty()
                                                      ? startDebug(args)
                                                      : startDebugExecutable(args);
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"message", *result}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_stop")
            .title("Stop debugging")
            .description(
                "Stops the current debug session. "
                "Returns a message indicating whether the stop was successful.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"message", stopDebug()}};
        }));

    registerIntrospectionMcpTools();
}

} // namespace Debugger::Internal
