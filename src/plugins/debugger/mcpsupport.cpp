// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "breakhandler.h"
#include "debuggerengine.h"
#include "enginemanager.h"
#include "mcpsupport.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <mcp/server/toolregistry.h>

#include <utils/id.h>
#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QString>
#include <QStringList>

using namespace Utils;

namespace Debugger::Internal {

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

static Result<QJsonObject> debuggerGetStatus()
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

static void getVariables(bool includeWatchers, std::function<void(Result<QJsonArray>)> callback)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler) {
        callback(ResultError(handler.error()));
        return;
    }

    QStringList rootInames = {"local"};
    if (includeWatchers)
        rootInames.append("watch");

    const auto buildResult = [handler, rootInames]() -> Result<QJsonArray> {
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
        return result;
    };

    const bool anyLoaded = std::any_of(rootInames.begin(), rootInames.end(),
                                       [&handler](const QString &rootIname) {
                                           const WatchItem *root = (*handler)->findItem(rootIname);
                                           return root && root->childCount() > 0;
                                       });
    if (anyLoaded) {
        callback(buildResult());
        return;
    }

    WatchModelBase *model = (*handler)->model();
    QObject::connect(model, &WatchModelBase::updateFinished,
                     model, [callback, buildResult]() { callback(buildResult()); },
                     Qt::SingleShotConnection);
    (*handler)->updateLocalsWindow();
}

static void getVariable(const QString &iname, std::function<void(Result<QJsonObject>)> callback)
{
    const Result<WatchHandler *> handler = getWatchHandler();
    if (!handler) {
        callback(ResultError(handler.error()));
        return;
    }

    WatchItem *item = (*handler)->findItem(iname);
    if (!item) {
        callback(ResultError("No variable with iname: " + iname));
        return;
    }

    const auto buildResult = [handler, iname]() -> Result<QJsonObject> {
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
        return obj;
    };

    if (item->childCount() > 0 || !item->wantsChildren) {
        callback(buildResult());
        return;
    }

    (*handler)->fetchMore(iname);
    WatchModelBase *model = (*handler)->model();
    QObject::connect(model, &WatchModelBase::updateFinished,
                     model, [callback, buildResult]() { callback(buildResult()); },
                     Qt::SingleShotConnection);
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

static Result<bool> selectThread(const QString &id)
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
    return true;
}

static Result<QJsonArray> getCallStack()
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    if (!engine)
        return ResultError("No active debug session");

    if (engine->state() != InferiorStopOk)
        return ResultError("Debugger is not paused (current state: "
                           + DebuggerEngine::stateName(engine->state()) + ")");

    const StackHandler *handler = engine->stackHandler();
    if (!handler || !handler->isContentsValid())
        return ResultError("Call stack is not available");

    QJsonArray frames;
    const int count = handler->stackSize();
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

static Result<bool> selectFrame(int level)
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
    return true;
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

    ToolRegistry::registerTool(
        Tool{}
            .name("get_breakpoints")
            .title("Get current breakpoints")
            .description("Returns all breakpoints currently set in Qt Creator's debugger")
            .annotations(ToolAnnotations{}.readOnlyHint(true).destructiveHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "breakpoints",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "List of breakpoints"}})
                    .addRequired("breakpoints")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"breakpoints", getBreakpoints()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("get_threads")
            .title("Get current threads")
            .description(
                "Returns all threads of the current debug session. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema([] {
                const QJsonObject threadProperties{
                    {"id",       QJsonObject{{"type", "string"},  {"description", "Thread ID"}}},
                    {"current",  QJsonObject{{"type", "boolean"}, {"description", "True for the currently selected thread"}}},
                    {"name",     QJsonObject{{"type", "string"},  {"description", "Thread name"}}},
                    {"state",    QJsonObject{{"type", "string"},  {"description", "Thread state, e.g. \"stopped\""}}},
                    {"target_id", QJsonObject{{"type", "string"},  {"description", "Target-level thread identifier"}}},
                    {"details",  QJsonObject{{"type", "string"},  {"description", "Additional details from the debugger"}}},
                    {"function", QJsonObject{{"type", "string"},  {"description", "Current function name"}}},
                    {"file",     QJsonObject{{"type", "string"},  {"description", "Current source file"}}},
                    {"line",     QJsonObject{{"type", "integer"}, {"description", "Current line number"}}},
                    {"address",  QJsonObject{{"type", "string"},  {"description", "Current instruction address"}}},
                };
                const QJsonObject threadItem{
                    {"type", "object"},
                    {"required", QJsonArray{"id", "current"}},
                    {"properties", threadProperties},
                };
                return Tool::OutputSchema{}
                    .addProperty(
                        "threads",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "List of threads"},
                            {"items", threadItem}})
                    .addRequired("threads");
            }()),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const Utils::Result<QJsonArray> threads = getThreads();
            if (!threads)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(threads.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"threads", *threads}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("select_thread")
            .title("Select a thread")
            .description(
                "Switches the current thread in the active debug session. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).idempotentHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Thread ID to select (as returned by get_threads)"}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const Utils::Result<bool> ok = selectThread(params.argumentsAsObject().value("id").toString());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"success", true}});
        });

    const auto varItemSchema = [] {
        return QJsonObject{
            {"type", "object"},
            {"required", QJsonArray{"iname", "name", "value", "type", "value_editable", "has_children"}},
            {"properties", QJsonObject{
                {"iname",          QJsonObject{{"type", "string"},  {"description", "Internal name, e.g. \"local.myVar\". Use as key for get_variable / set_variable."}}},
                {"name",           QJsonObject{{"type", "string"},  {"description", "Display name"}}},
                {"value",          QJsonObject{{"type", "string"},  {"description", "Current value as a string"}}},
                {"type",           QJsonObject{{"type", "string"},  {"description", "Type name"}}},
                {"address",        QJsonObject{{"type", "string"},  {"description", "Memory address, e.g. \"0x1234\""}}},
                {"value_editable", QJsonObject{{"type", "boolean"}, {"description", "Whether the value can be changed via set_variable"}}},
                {"has_children",   QJsonObject{{"type", "boolean"}, {"description", "Whether the variable has child members"}}},
            }},
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("get_variables")
            .title("List local variables")
            .description(
                "Returns local variables for the current stack frame. "
                "Optionally includes watch expressions. "
                "Variables with has_children=true may include a children array if already expanded; "
                "otherwise call get_variable with the variable's iname to retrieve sub-fields. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "include_watchers",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Also return watch expressions (default: false)"},
                            {"default", false}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "variables",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "List of variables"},
                            {"items", varItemSchema()}})
                    .addRequired("variables")),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const bool includeWatchers = params.argumentsAsObject().value("include_watchers").toBool(false);
            getVariables(includeWatchers, [toolInterface](Utils::Result<QJsonArray> vars) {
                if (!vars)
                    toolInterface.finish(CallToolResult{}.isError(true).addContent(
                        TextContent{}.text(vars.error())));
                else
                    toolInterface.finish(CallToolResult{}.isError(false).structuredContent(
                        QJsonObject{{"variables", *vars}}));
            });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("get_variable")
            .title("Get a variable")
            .description(
                "Returns the details of a single variable by its iname, including its children "
                "if it has any (e.g. struct members or array elements). "
                "If a child also has has_children=true, call get_variable again with that child's iname "
                "to retrieve its sub-fields. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Internal name of the variable (e.g. \"local.myVar\")"}})
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
                        {"type",           QJsonObject{{"type", "string"}}},
                        {"address",        QJsonObject{{"type", "string"}}},
                        {"value_editable", QJsonObject{{"type", "boolean"}}},
                        {"has_children",   QJsonObject{{"type", "boolean"}}},
                        {"children",       QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "object"}}},
                                                      {"description", "Child members, present when has_children is true"}}},
                    };
                    return s;
                }();
                return Tool::OutputSchema{}.addProperty("variable", schema).addRequired("variable");
            }()),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            getVariable(
                params.argumentsAsObject().value("iname").toString(),
                [toolInterface](Utils::Result<QJsonObject> var) {
                    if (!var)
                        toolInterface.finish(CallToolResult{}.isError(true).addContent(
                            TextContent{}.text(var.error())));
                    else
                        toolInterface.finish(CallToolResult{}.isError(false).structuredContent(
                            QJsonObject{{"variable", *var}}));
                });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("set_variable")
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
                            {"description", "Internal name of the variable (e.g. \"local.myVar\")"}})
                    .addProperty(
                        "value",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "New value to assign"}})
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
            .name("add_watch_expression")
            .title("Add a watch expression")
            .description(
                "Adds an expression to the watch list in the current debug session. "
                "The expression is evaluated and its value updated as execution progresses. "
                "Returns the iname of the new watch entry (e.g. \"watch.0\"), which can be used "
                "with get_variable, set_variable, and remove_watch_expression. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "expression",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Expression to watch (e.g. \"myVar\", \"ptr->field\")"}})
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Optional display name; defaults to the expression"}})
                    .addRequired("expression"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "iname",
                        QJsonObject{{"type", "string"},
                                    {"description", "Internal name of the watch entry (e.g. \"watch.0\")"}})
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
            .name("remove_watch_expression")
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
                             "Internal name of the watch entry to remove (e.g. \"watch.0\")"}})
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

    ToolRegistry::registerTool(
        Tool{}
            .name("get_call_stack")
            .title("Get current call stack")
            .description(
                "Returns the call stack (stack frames) of the current debug session. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema([] {
                const QJsonObject frameProperties{
                    {"level",    QJsonObject{{"type", "integer"}, {"description", "Frame index, 0 = innermost"}}},
                    {"current",  QJsonObject{{"type", "boolean"}, {"description", "True for the currently active frame"}}},
                    {"function", QJsonObject{{"type", "string"},  {"description", "Function or method name"}}},
                    {"file",     QJsonObject{{"type", "string"},  {"description", "Absolute path to the source file"}}},
                    {"line",     QJsonObject{{"type", "integer"}, {"description", "Line number in the source file"}}},
                    {"address",  QJsonObject{{"type", "string"},  {"description", "Instruction address, e.g. \"0x1234abcd\""}}},
                    {"module",   QJsonObject{{"type", "string"},  {"description", "Module or shared library name"}}},
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
                            {"description", "Stack frames, innermost first"},
                            {"items", frameItem}})
                    .addRequired("frames");
            }()),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const Utils::Result<QJsonArray> frames = getCallStack();
            if (!frames)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(frames.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"frames", *frames}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("select_frame")
            .title("Select a stack frame")
            .description(
                "Switches the current stack frame in the active debug session. "
                "Subsequent get_variables / evaluate_expression calls operate on the selected frame. "
                "Returns an error if no debug session is active or the debugger is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).idempotentHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "level",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Frame level to select (as returned by get_call_stack, 0 = innermost)"}})
                    .addRequired("level"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const Utils::Result<bool> ok = selectFrame(params.argumentsAsObject().value("level").toInt());
            if (!ok)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(ok.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"success", true}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("delete_breakpoint")
            .title("Delete a breakpoint")
            .description(
                "Deletes a breakpoint by its ID (as returned by get_breakpoints or add_breakpoint)")
            .annotations(ToolAnnotations().destructiveHint(true).idempotentHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "integer"}, {"description", "ID of the breakpoint to delete"}})
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
            .name("add_breakpoint")
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
                             "Absolute path to the source file (for fileAndLine type)"}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Line number in the source file (for fileAndLine type)"}})
                    .addProperty(
                        "function",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Function name (for function type)"}})
                    .addProperty(
                        "address",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Memory address (for address or watchAddress type)"}})
                    .addProperty(
                        "condition",
                        QJsonObject{
                            {"type", "string"}, {"description", "Optional condition expression"}})
                    .addProperty(
                        "ignore_count",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Number of hits to ignore before breaking"}})
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
                             "If true, the breakpoint is removed after the first hit"},
                            {"default", false}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "integer"}, {"description", "ID of the created breakpoint"}})
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
    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_step_over")
            .title("Step over")
            .description(
                "Steps over the current line in the debugger. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const auto result = debuggerStepOver();
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"message", *result}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_step_in")
            .title("Step into")
            .description(
                "Steps into the next function call in the debugger. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const auto result = debuggerStepIn();
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"message", *result}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_step_out")
            .title("Step out")
            .description(
                "Steps out of the current function in the debugger. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const auto result = debuggerStepOut();
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"message", *result}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_continue")
            .title("Continue execution")
            .description(
                "Resumes program execution in the debugger until the next breakpoint. "
                "Requires an active debug session that is paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const auto result = debuggerContinue();
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"message", *result}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_interrupt")
            .title("Pause execution")
            .description(
                "Pauses the currently running debuggee. "
                "Requires an active debug session that is running.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("message")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const auto result = debuggerInterrupt();
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{{"message", *result}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("debugger_get_status")
            .title("Get debugger status")
            .description(
                "Returns the current status of the debugger including whether a session is active, "
                "its state (paused/running/stopped), and the current position if paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("has_session", QJsonObject{{"type", "boolean"}})
                    .addProperty("state", QJsonObject{{"type", "string"}})
                    .addProperty("is_paused", QJsonObject{{"type", "boolean"}})
                    .addProperty("is_running", QJsonObject{{"type", "boolean"}})
                    .addProperty("current_position", QJsonObject{{"type", "object"}})
                    .addRequired("has_session")
                    .addRequired("state")),
        [](const Schema::CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const auto result = debuggerGetStatus();
            if (!result)
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(result.error()));
            return CallToolResult{}.isError(false).structuredContent(*result);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("evaluate_expression")
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
                            {"description", "Expression to evaluate (e.g. \"myVar\", \"ptr->field\", \"a + b\")"}})
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
            .name("stop_debug")
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
}

} // namespace Debugger::Internal
