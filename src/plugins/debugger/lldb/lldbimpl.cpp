// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lldbimpl.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../watchutils.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

using namespace Utils;

namespace Debugger::Internal {

static void killPidHard(qint64 pid)
{
    if (pid <= 0)
        return;
#ifdef Q_OS_WIN
    if (HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(pid))) {
        TerminateProcess(handle, 1);
        CloseHandle(handle);
    }
#else
    ::kill(pid, SIGKILL);
#endif
}

static DebuggerEngineSetupData lldbImplSetupData()
{
    DebuggerEngineSetupData data;
    const unsigned coreCaps = AdditionalQmlStackCapability
                            | AddWatcherCapability
                            | AutoDerefPointersCapability
                            | CreateFullBacktraceCapability
                            | DisassemblerCapability
                            | OperateByInstructionCapability
                            | RegisterCapability
                            | ShowMemoryCapability
                            | ShowModuleSectionsCapability
                            | ShowModuleSymbolsCapability
                            | WatchComplexExpressionsCapability;
    data.attachToCoreCapabilities = coreCaps;
    data.capabilities = coreCaps
                      | BreakConditionCapability
                      | BreakIndividualLocationsCapability
                      | BreakOnThrowAndCatchCapability
                      | JumpToLineCapability
                      | ReloadModuleCapability
                      | ReloadModuleSymbolsCapability
                      | ResetInferiorCapability
                      | ReturnFromFunctionCapability
                      | RunToLineCapability
                      | TracePointCapability
                      | WatchWidgetsCapability
                      | WatchpointByAddressCapability
                      | WatchpointByExpressionCapability;
    data.extraCapabilities = DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::LibraryEvent
                           | DebuggerExtraCapability::RunCommandDeferral
                           | DebuggerExtraCapability::SignalReceived
                           | DebuggerExtraCapability::SourceFiles
                           | DebuggerExtraCapability::Threads;
    data.startModes = DebuggerStartModeFlag::Launch
                    | DebuggerStartModeFlag::AttachToProcess
                    | DebuggerStartModeFlag::AttachToTerminalStub
                    | DebuggerStartModeFlag::AttachToRemoteServer
                    | DebuggerStartModeFlag::AttachToCore;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferiorAndCppEditor;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        if (query.isCppBreakpoint())
            return true;
        return query.isNativeMixedEnabled;
    };
    return data;
}

static void addConst(GdbMi &parent, const QString &name, const QString &data)
{
    GdbMi child;
    child.m_type = GdbMi::Const;
    child.m_name = name;
    child.m_data = data;
    parent.addChild(child);
}

static GdbMi translateLldbBreakpointReply(const GdbMi &lldbBkpt)
{
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    addConst(bkpt, "number", lldbBkpt["lldbid"].data());
    if (lldbBkpt["enabled"].toInt())
        addConst(bkpt, "enabled", "y");
    else
        addConst(bkpt, "enabled", "n");
    addConst(bkpt, "file", lldbBkpt["file"].data());
    addConst(bkpt, "line", lldbBkpt["line"].data());
    addConst(bkpt, "type", "breakpoint");
    if (lldbBkpt["oneshot"].toInt())
        addConst(bkpt, "disp", "del");
    else
        addConst(bkpt, "disp", "keep");
    const QString condition = fromHex(lldbBkpt["condition"].data());
    if (!condition.isEmpty())
        addConst(bkpt, "cond", condition);
    addConst(bkpt, "times", lldbBkpt["hitcount"].data());

    const GdbMi lldbLocations = lldbBkpt["locations"];
    if (lldbLocations.childCount() > 1) {
        GdbMi locations;
        locations.m_type = GdbMi::List;
        locations.m_name = "locations";
        for (const GdbMi &lldbLocation : lldbLocations) {
            GdbMi location;
            location.m_type = GdbMi::Tuple;
            addConst(location, "number",
                     lldbBkpt["lldbid"].data() + '.' + lldbLocation["locid"].data());
            addConst(location, "func", lldbLocation["function"].data());
            addConst(location, "file", lldbLocation["file"].data());
            addConst(location, "line", lldbLocation["line"].data());
            addConst(location, "type", "breakpoint");
            if (lldbLocation["enabled"].toInt())
                addConst(location, "enabled", "y");
            else
                addConst(location, "enabled", "n");
            locations.addChild(location);
        }
        bkpt.addChild(locations);
    }

    GdbMi list;
    list.m_type = GdbMi::List;
    list.addChild(bkpt);
    return list;
}

static GdbMi translateLldbModulesReply(const GdbMi &lldbModules)
{
    GdbMi result;
    result.m_type = GdbMi::List;
    for (const GdbMi &lldbModule : lldbModules) {
        GdbMi module;
        module.m_type = GdbMi::Tuple;
        const auto addConst = [&module](const QString &name, const QString &data) {
            GdbMi child;
            child.m_type = GdbMi::Const;
            child.m_name = name;
            child.m_data = data;
            module.addChild(child);
        };
        addConst("modulepath", lldbModule["file"].data());
        addConst("startaddress", "0");
        addConst("endaddress", "0");
        addConst("symbolsread", "Yes");
        result.addChild(module);
    }
    return result;
}

static GdbMi translateLldbSymbolsReply(const FilePath &modulePath, const GdbMi &lldbSymbols)
{
    GdbMi result;
    result.m_type = GdbMi::Tuple;
    GdbMi modulePathItem;
    modulePathItem.m_type = GdbMi::Const;
    modulePathItem.m_name = "modulepath";
    modulePathItem.m_data = modulePath.path();
    result.addChild(modulePathItem);

    GdbMi symbols;
    symbols.m_type = GdbMi::List;
    symbols.m_name = "symbols";
    for (const GdbMi &lldbSymbol : lldbSymbols["symbols"]) {
        GdbMi symbol;
        symbol.m_type = GdbMi::Tuple;
        const auto addConst = [&symbol](const QString &name, const QString &data) {
            GdbMi child;
            child.m_type = GdbMi::Const;
            child.m_name = name;
            child.m_data = data;
            symbol.addChild(child);
        };
        addConst("address", lldbSymbol["address"].data());
        addConst("name", lldbSymbol["name"].data());
        addConst("demangled", lldbSymbol["demangled"].data());
        symbols.addChild(symbol);
    }
    result.addChild(symbols);
    return result;
}

static GdbMi translateLldbSectionsReply(const FilePath &modulePath, const GdbMi &lldbSections)
{
    GdbMi result;
    result.m_type = GdbMi::Tuple;
    GdbMi modulePathItem;
    modulePathItem.m_type = GdbMi::Const;
    modulePathItem.m_name = "modulepath";
    modulePathItem.m_data = modulePath.path();
    result.addChild(modulePathItem);

    GdbMi sections;
    sections.m_type = GdbMi::List;
    sections.m_name = "sections";
    for (const GdbMi &lldbSection : lldbSections) {
        GdbMi section;
        section.m_type = GdbMi::Tuple;
        const auto addConst = [&section](const QString &name, const QString &data) {
            GdbMi child;
            child.m_type = GdbMi::Const;
            child.m_name = name;
            child.m_data = data;
            section.addChild(child);
        };
        addConst("from", lldbSection["from"].data());
        addConst("to", lldbSection["to"].data());
        addConst("address", lldbSection["address"].data());
        addConst("name", lldbSection["name"].data());
        addConst("flags", lldbSection["flags"].data());
        sections.addChild(section);
    }
    result.addChild(sections);
    return result;
}

static QString machExceptionSignalName(const QString &meaning)
{
    static const QList<QPair<QString, QString>> exceptionToSignal = {
        {"EXC_BAD_ACCESS", "SIGSEGV"},
        {"EXC_BAD_INSTRUCTION", "SIGILL"},
        {"EXC_ARITHMETIC", "SIGFPE"},
        {"EXC_SOFTWARE", "SIGABRT"},
        {"EXC_BREAKPOINT", "SIGTRAP"},
        {"EXC_CRASH", "SIGABRT"},
    };
    for (const auto &[token, signalName] : exceptionToSignal) {
        if (meaning.contains(token))
            return signalName;
    }
    return {};
}

LldbImpl::LldbImpl(const LldbImplStartData &startData)
    : DebuggerEngineInterface(lldbImplSetupData())
    , m_startData(startData)
{
    m_lldbProc.setProcessMode(ProcessMode::Writer);
    m_lldbProc.setCommand(m_startData.debuggerRunData.command);
    Environment lldbEnvironment = m_startData.debuggerRunData.environment;
    lldbEnvironment.set("QT_CREATOR_LLDB_PROCESS", "1");
    lldbEnvironment.set("PYTHONUNBUFFERED", "1");
    lldbEnvironment.unset("DEBUGINFOD_URLS");
    m_lldbProc.setEnvironment(lldbEnvironment);
    if (m_startData.debuggerRunData.workingDirectory.isDir())
        m_lldbProc.setWorkingDirectory(m_startData.debuggerRunData.workingDirectory);

    connect(&m_lldbProc, &Process::started, this, [this] {
        runCommand({"script sys.path.insert(1, '" + m_startData.dumperScriptsDir.path() + "')",
                   DebuggerCommand::NativeCommand});
        runCommand({"script from lldbbridge import *", DebuggerCommand::NativeCommand});

        DebuggerCommand cmd("setupInferior");
        cmd.arg("breakonmain", false);
        cmd.arg("useterminal", false);
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("deviceUuid", QString());
        cmd.arg("platform", QString());
        FilePath coreFileForRunEngine;

        if (const auto *inferiorRunData
                = std::get_if<ProcessRunData>(&m_startData.inferiorStartData)) {
            const FilePath &executable = inferiorRunData->command.executable();
            cmd.arg("executable", executable.path());
            cmd.arg("startmode", int(StartInternal));
            cmd.arg("workingdirectory", inferiorRunData->workingDirectory.path());
            cmd.arg("environment", inferiorRunData->environment.toStringList());
            cmd.arg("processargs",
                    toHex(ProcessArgs::splitArgs(inferiorRunData->command.arguments(),
                                                 HostOsInfo::hostOs())
                         .join(QChar(0))));
            cmd.arg("symbolfile", executable.path());
        } else if (const auto *attachData
                       = std::get_if<AttachToProcessData>(&m_startData.inferiorStartData)) {
            cmd.arg("executable", QString());
            cmd.arg("startmode", int(AttachToLocalProcess));
            cmd.arg("workingdirectory", QString());
            cmd.arg("environment", QStringList());
            cmd.arg("processargs", QString());
            cmd.arg("symbolfile", QString());
            cmd.arg("attachpid", attachData->pid.pid());
        } else if (const auto *termData
                       = std::get_if<AttachToTerminalStubData>(&m_startData.inferiorStartData)) {
            cmd.arg("executable", QString());
            cmd.arg("startmode", int(AttachToLocalProcess));
            cmd.arg("workingdirectory", QString());
            cmd.arg("environment", QStringList());
            cmd.arg("processargs", QString());
            cmd.arg("symbolfile", QString());
            cmd.arg("attachpid", termData->pid.pid());
        } else if (const auto *remoteData
                       = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData)) {
            cmd.arg("executable", QString());
            cmd.arg("startmode", int(AttachToRemoteServer));
            cmd.arg("workingdirectory", QString());
            cmd.arg("environment", QStringList());
            cmd.arg("processargs", QString());
            cmd.arg("symbolfile", remoteData->symbolFile.path());
            cmd.arg("remotechannel", remoteData->channel);
        } else if (const auto *coreData
                       = std::get_if<AttachToCoreData>(&m_startData.inferiorStartData)) {
            cmd.arg("executable", coreData->executable.path());
            cmd.arg("startmode", int(AttachToCore));
            cmd.arg("workingdirectory", QString());
            cmd.arg("environment", QStringList());
            cmd.arg("processargs", QString());
            cmd.arg("symbolfile", coreData->executable.path());
            coreFileForRunEngine = coreData->coreFile;
        } else {
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
            return;
        }
        cmd.callback = [this, coreFile = coreFileForRunEngine](const DebuggerResponse &response) {
            const bool success = response.data["success"].toInt();
            if (!success) {
                emit inferiorEvent(InferiorEvent::EngineSetupFailed);
                return;
            }
            emit inferiorEvent(InferiorEvent::EngineSetupOk);
            DebuggerCommand runCmd("runEngine");
            if (!coreFile.isEmpty())
                runCmd.arg("coreFile", coreFile.path());
            runCommand(runCmd);
        };
        runCommand(cmd);
    });
    connect(&m_lldbProc, &Process::readyReadStandardOutput, this, [this] {
        m_inbuffer += m_lldbProc.readAllStandardOutput();
        while (true) {
            if (int pos = m_inbuffer.indexOf(u"@\n"); pos >= 0) {
                handleLldbOutput(m_inbuffer.left(pos).trimmed());
                m_inbuffer = m_inbuffer.mid(pos + 2);
                continue;
            }
            if (int pos = m_inbuffer.indexOf(u"@\r\n"); pos >= 0) {
                handleLldbOutput(m_inbuffer.left(pos).trimmed());
                m_inbuffer = m_inbuffer.mid(pos + 3);
                continue;
            }
            break;
        }
    });
    connect(&m_lldbProc, &Process::readyReadStandardError, this, [this] {
        emit message(m_lldbProc.readAllStandardError(), LogError);
    });
    connect(&m_lldbProc, &Process::done, this, [this] {
        if (m_lldbProc.result() == ProcessResult::StartFailed)
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        emit engineProcessFinished(m_lldbProc.resultData());
    });
}

LldbImpl::~LldbImpl()
{
    if (m_detached)
        return;
    if (m_lldbProc.isRunning())
        m_lldbProc.write("script theDumper.shutdownInferior({})\n\n");
    killPidHard(m_inferiorPid);
}

void LldbImpl::start()
{
    m_lldbProc.start();
}

void LldbImpl::shutdownInferior(ShutdownMode mode)
{
    const QString function = mode == ShutdownMode::Detach ? QLatin1String("detachInferior")
                                                          : QLatin1String("shutdownInferior");
    m_detached = mode == ShutdownMode::Detach;
    runCommand({function, [this](const DebuggerResponse &) {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
    }});
}

void LldbImpl::shutdownEngine()
{
    if (!m_lldbProc.isRunning()) {
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
        return;
    }
    // The process' own done handler reports the exit, which is what finishes
    // the shutdown. Reporting it here as well would do it twice.
    m_lldbProc.write("quit\n\n");
}

void LldbImpl::execute(const ExecutionRequest &request)
{
    switch (request.command) {
    case ExecutionCommand::Continue:
        if (m_inferiorExited) {
            emit inferiorEvent(InferiorEvent::InferiorIll);
            break;
        }
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"continueInferior", [this](const DebuggerResponse &response) {
            if (response.data["success"].toInt())
                return;
            const QString error = response.data["error"]["status"].data()
                                  + response.data["status"].data();
            const bool unrunnable = error.contains("does not support resuming")
                                    || error.contains("No process");
            emit inferiorEvent(unrunnable ? InferiorEvent::InferiorIll
                                          : InferiorEvent::RunFailed);
        }});
        break;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
            runCommand({"markPendingInterrupt"});
            emit interruptTerminalRequested();
            break;
        }
        runCommand({"interruptInferior"});
        break;
    case ExecutionCommand::StepIn:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({QLatin1String(request.flag ? "executeStepI" : "executeStep")});
        break;
    case ExecutionCommand::StepOver:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({QLatin1String(request.flag ? "executeNextI" : "executeNext")});
        break;
    case ExecutionCommand::StepOut:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"executeStepOut"});
        break;
    case ExecutionCommand::Detach:
        m_detached = true;
        runCommand({"detachInferior", [this](const DebuggerResponse &) {
            emit inferiorDone({0, InferiorExitStatus::Detached});
        }});
        break;
    case ExecutionCommand::Abort:
        m_lldbProc.kill();
        break;
    case ExecutionCommand::ResetInferior:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"resetInferior"});
        break;
    case ExecutionCommand::RunToLine: {
        emit inferiorEvent(InferiorEvent::RunRequested);
        DebuggerCommand cmd("executeRunToLocation");
        cmd.arg("file", request.context.fileName.path());
        cmd.arg("line", request.context.textPosition.line);
        cmd.arg("address", request.context.address);
        runCommand(cmd);
        break;
    }
    case ExecutionCommand::RunToFunction: {
        emit inferiorEvent(InferiorEvent::RunRequested);
        DebuggerCommand cmd("executeRunToFunction");
        cmd.arg("function", request.functionName);
        runCommand(cmd);
        break;
    }
    case ExecutionCommand::JumpToLine: {
        DebuggerCommand cmd("executeJumpToLocation");
        cmd.arg("file", request.context.fileName.path());
        cmd.arg("line", request.context.textPosition.line);
        cmd.arg("address", request.context.address);
        cmd.callback = [this](const DebuggerResponse &) {
            fetchLocationAfterStop(InferiorEvent::SpontaneousStop);
        };
        runCommand(cmd);
        break;
    }
    case ExecutionCommand::Return:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"executeReturn", [this](const DebuggerResponse &response) {
            if (response.data["success"].toInt())
                emit inferiorEvent(InferiorEvent::StopOk);
        }});
        break;
    case ExecutionCommand::RepeatLastCommand:
        if (!m_lastDebuggableCommand.function.isEmpty())
            runCommand(m_lastDebuggableCommand);
        break;
    case ExecutionCommand::RecordReverse:
        emit message("LldbImpl::execute() does not support reverse recording",
                     LogWarning);
        break;
    }
}

void LldbImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.op) {
    case BreakpointOp::Insert: {
        if (request.params.type != BreakpointByFileAndLine
                && request.params.type != BreakpointByFunction
                && request.params.type != WatchpointAtAddress
                && request.params.type != WatchpointAtExpression
                && request.params.type != BreakpointAtFork
                && request.params.type != BreakpointAtThrow
                && request.params.type != BreakpointAtCatch) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, false);
            return;
        }
        DebuggerCommand cmd("insertBreakpoint");
        cmd.arg("type", int(request.params.type));
        cmd.arg("file", request.params.fileName.path());
        cmd.arg("line", request.params.textPosition.line);
        cmd.arg("ignorecount", request.params.ignoreCount);
        cmd.arg("condition", toHex(request.params.condition));
        cmd.arg("command", toHex(request.params.command));
        cmd.arg("function", request.params.functionName);
        cmd.arg("address", request.params.address);
        cmd.arg("expression", request.params.expression);
        cmd.arg("oneshot", request.params.oneShot);
        cmd.arg("enabled", request.params.enabled);
        cmd.arg("tracepoint", request.params.tracepoint);
        cmd.arg("message", toHex(request.params.message));
        cmd.arg("modelid", request.modelId);
        const bool isCppBreakpoint = request.params.isCppBreakpoint();
        cmd.callback = [this, requestId, isCppBreakpoint](const DebuggerResponse &response) {
            const bool ok = response.resultClass == ResultDone;
            if (!ok || !isCppBreakpoint) {
                emit breakpointEvent(requestId, BreakpointOp::Insert, ok);
                return;
            }
            emit breakpointEvent(requestId, BreakpointOp::Insert, true,
                                 translateLldbBreakpointReply(response.data));
        };
        runCommand(cmd);
        break;
    }
    case BreakpointOp::Remove: {
        DebuggerCommand cmd("removeBreakpoint");
        cmd.arg("lldbid", request.responseId);
        runCommand(cmd);
        emit breakpointEvent(requestId, BreakpointOp::Remove, true);
        break;
    }
    case BreakpointOp::Update: {
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(requestId, BreakpointOp::Update, false);
            break;
        }
        DebuggerCommand cmd("changeBreakpoint");
        cmd.arg("lldbid", request.responseId);
        cmd.arg("ignorecount", request.params.ignoreCount);
        cmd.arg("condition", toHex(request.params.condition));
        cmd.arg("enabled", request.params.enabled);
        cmd.arg("oneshot", request.params.oneShot);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone) {
                emit breakpointEvent(requestId, BreakpointOp::Update, false);
                return;
            }
            emit breakpointEvent(requestId, BreakpointOp::Update, true,
                                 translateLldbBreakpointReply(response.data));
        };
        runCommand(cmd);
        break;
    }
    case BreakpointOp::EnableSub: {
        QString lldbid = request.subResponseId;
        int locid = 1;
        const int dotPos = lldbid.indexOf('.');
        if (dotPos != -1) {
            locid = lldbid.mid(dotPos + 1).toInt();
            lldbid = lldbid.left(dotPos);
        }
        DebuggerCommand cmd("enableSubbreakpoint");
        cmd.arg("lldbid", lldbid);
        cmd.arg("locid", locid);
        cmd.arg("enabled", request.enabled);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit breakpointEvent(requestId, BreakpointOp::EnableSub,
                                 response.data["success"].toInt() != 0);
        };
        runCommand(cmd);
        break;
    }
    }
}

void LldbImpl::refresh(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.kind) {
    case RefreshKind::FullBacktrace: {
        DebuggerCommand cmd("fetchFullBacktrace");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            GdbMi trace;
            trace.m_type = GdbMi::Const;
            trace.m_data = fromHex(response.data["fulltrace"].data());
            emit refreshDataReceived(requestId, RefreshKind::FullBacktrace, trace);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Locals: {
        DebuggerCommand cmd("fetchVariables");
        cmd.arg("fancy", true);
        cmd.arg("autoderef", request.autoDerefPointers);
        cmd.arg("dyntype", true);
        cmd.arg("partialvar", request.partialVariable);
        cmd.arg("context", request.context);
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("expanded", QStringList());
        cmd.arg("watchers", request.watchers);
        m_lastDebuggableCommand = cmd;
        m_lastDebuggableCommand.arg("passexceptions", "1");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Locals, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::FullStack: {
        DebuggerCommand cmd("fetchStack");
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("stacklimit", -1);
        cmd.arg("context", request.context);
        cmd.arg("extraqml", 0);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::FullStack, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::QmlStack: {
        DebuggerCommand cmd("fetchStack");
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("stacklimit", -1);
        cmd.arg("context", request.context);
        cmd.arg("extraqml", 1);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::FullStack, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Registers: {
        DebuggerCommand cmd("fetchRegisters");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Registers, response.data["registers"]);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Threads: {
        DebuggerCommand cmd("fetchThreads");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Threads, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Modules: {
        DebuggerCommand cmd("fetchModules");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Modules,
                                     translateLldbModulesReply(response.data["modules"]));
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::ModuleSymbols: {
        const FilePath modulePath = request.path;
        DebuggerCommand cmd("fetchSymbols");
        cmd.arg("module", modulePath.path());
        cmd.callback = [this, requestId, modulePath](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::ModuleSymbols,
                                     translateLldbSymbolsReply(modulePath, response.data["symbols"]));
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::ModuleSections: {
        const FilePath modulePath = request.path;
        DebuggerCommand cmd("fetchSections");
        cmd.arg("module", modulePath.path());
        cmd.callback = [this, requestId, modulePath](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::ModuleSections,
                                     translateLldbSectionsReply(modulePath, response.data["sections"]));
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::SourceFiles: {
        DebuggerCommand cmd("fetchSourceFiles");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::SourceFiles, response.data["files"]);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::PeripheralRegisters: {
        for (const quint64 requestedAddress : request.addresses) {
            DebuggerCommand cmd("fetchMemory");
            cmd.arg("address", requestedAddress);
            cmd.arg("length", 4);
            cmd.callback = [this, requestId, requestedAddress](const DebuggerResponse &response) {
                if (!response.data["success"].toInt())
                    return;
                const QByteArray contents =
                    QByteArray::fromHex(response.data["contents"].data().toUtf8());
                if (contents.size() != 4)
                    return;
                quint32 value = 0;
                for (int i = 0; i < contents.size(); ++i)
                    value |= quint32(uchar(contents.at(i))) << (8 * i);
                GdbMi result;
                result.m_type = GdbMi::Tuple;
                const auto addConst = [&result](const QString &name, const QString &data) {
                    GdbMi child;
                    child.m_type = GdbMi::Const;
                    child.m_name = name;
                    child.m_data = data;
                    result.addChild(child);
                };
                addConst("address", QString::number(requestedAddress));
                addConst("value", QString::number(value));
                emit refreshDataReceived(requestId, RefreshKind::PeripheralRegisters, result);
            };
            runCommand(cmd);
        }
        return;
    }
    case RefreshKind::DebuggingHelpers:
        runCommand({"reloadDumpers"});
        refresh({requestId, RefreshKind::Locals});
        return;
    case RefreshKind::AllSymbols:
        refresh({requestId, RefreshKind::Modules});
        refresh({requestId, RefreshKind::FullStack});
        refresh({requestId, RefreshKind::Locals});
        return;
    case RefreshKind::StackSymbols:
        return;
    default:
        emit message("LldbImpl::refresh() does not support this kind yet", LogWarning);
        return;
    }
}

void LldbImpl::fetchLocationAfterStop(InferiorEvent event)
{
    DebuggerCommand cmd("fetchStack");
    cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
    cmd.arg("stacklimit", 1);
    cmd.arg("context", QString());
    cmd.arg("extraqml", 0);
    cmd.callback = [this, event](const DebuggerResponse &response) {
        const GdbMi frames = response.data["stack"]["frames"];
        if (frames.childCount() != 0) {
            const GdbMi frame = frames.childAt(0);
            const FilePath fileName = FilePath::fromUserInput(frame["file"].data());
            const int lineNumber = frame["line"].toInt();
            if (lineNumber != 0)
                emit locationChanged(fileName, lineNumber);
        }
        emit inferiorEvent(event);
    };
    runCommand(cmd);
}

void LldbImpl::selectThread(const QString &threadId)
{
    DebuggerCommand cmd("selectThread");
    cmd.arg("id", threadId);
    runCommand(cmd);
}

void LldbImpl::activateFrame(int index)
{
    DebuggerCommand cmd("activateFrame");
    cmd.arg("index", index);
    runCommand(cmd);
}

void LldbImpl::setRegisterValue(const QString &name, const QString &value)
{
    DebuggerCommand cmd("setRegister");
    cmd.arg("name", name);
    cmd.arg("value", value);
    runCommand(cmd);
}

void LldbImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                            const QByteArray &data)
{
    if (op == MemoryOp::Change) {
        DebuggerCommand cmd("writeMemory");
        cmd.arg("address", addr);
        cmd.arg("data", QString::fromUtf8(data.toHex()));
        runCommand(cmd);
        return;
    }

    DebuggerCommand cmd("fetchMemory");
    cmd.arg("address", addr);
    cmd.arg("length", lengthOrSize);
    cmd.callback = [this, requestId, addr, lengthOrSize](const DebuggerResponse &response) {
        QByteArray contents;
        if (response.data["success"].toInt())
            contents = QByteArray::fromHex(response.data["contents"].data().toUtf8());
        if (contents.size() != int(lengthOrSize))
            contents = QByteArray(int(lengthOrSize), char());
        emit memoryDataReceived(requestId, addr, contents);
    };
    runCommand(cmd);
}

void LldbImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName)
{
    DebuggerCommand cmd("fetchDisassembler");
    cmd.arg("address", address);
    cmd.arg("function", functionName);
    cmd.callback = [this, requestId](const DebuggerResponse &response) {
        DisassemblerLines result;
        for (const GdbMi &line : response.data["lines"]) {
            DisassemblerLine dl;
            dl.address = line["address"].toAddress();
            dl.data = line["rawdata"].data();
            if (!dl.data.isEmpty())
                dl.data += QString(30 - dl.data.size(), ' ');
            dl.data += fromHex(line["hexdata"].data());
            dl.data += line["data"].data();
            dl.offset = line["offset"].toInt();
            dl.lineNumber = line["line"].toInt();
            dl.fileName = line["file"].data();
            dl.function = line["function"].data();
            dl.hunk = line["hunk"].toInt();
            const QString comment = fromHex(line["comment"].data());
            if (!comment.isEmpty())
                dl.data += " # " + comment;
            result.appendLine(dl);
        }
        emit disassemblyReceived(requestId, result);
    };
    runCommand(cmd);
}

void LldbImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                     const QString &value)
{
    DebuggerCommand cmd("assignValue");
    cmd.arg("type", toHex(item.type));
    cmd.arg("expr", toHex(expr));
    cmd.arg("value", toHex(value));
    cmd.arg("simpleType", isIntOrFloatType(item.type));
    runCommand(cmd);
}

void LldbImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    const int intValue = int(value);
    const QByteArray bytes(reinterpret_cast<const char *>(&intValue), sizeof(int));
    DebuggerCommand cmd("writeMemory");
    cmd.arg("address", address);
    cmd.arg("data", QString::fromUtf8(bytes.toHex()));
    runCommand(cmd);
}

void LldbImpl::watchPoint(quint64 requestId, const QPoint &pnt)
{
    DebuggerCommand cmd("watchPoint");
    cmd.arg("x", pnt.x());
    cmd.arg("y", pnt.y());
    cmd.callback = [this, requestId](const DebuggerResponse &response) {
        emit watchPointResolved(requestId, response.data["selected"].toAddress(),
                                response.data["expr"].data());
    };
    runCommand(cmd);
}

void LldbImpl::createSnapshot(quint64)
{
}

void LldbImpl::executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem)
{
    Q_UNUSED(inspectorItem)
    DebuggerCommand cmd("executeDebuggerCommand");
    cmd.arg("command", command);
    cmd.callback = [this](const DebuggerResponse &response) {
        const QString output = response.data["output"].data();
        if (!output.isEmpty())
            emit message(output, LogOutput);
        const QString error = response.data["error"].data();
        if (!error.isEmpty())
            emit message(error, LogError);
    };
    runCommand(cmd);
}

void LldbImpl::handleTracepointHit(const GdbMi &item)
{
    QMap<QString, QString> values;
    for (const GdbMi &capture : item["expressions"]) {
        values.insert(fromHex(capture["expr"].data()),
                      decodeData(capture["value"].data(), capture["valueencoded"].data()));
    }

    const QString templ = fromHex(item["message"].data());
    static const QRegularExpression re("\\{([^}]+)\\}");
    QString formatted;
    qsizetype pos = 0;
    QRegularExpressionMatchIterator it = re.globalMatch(templ);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        formatted += templ.mid(pos, match.capturedStart() - pos);
        const auto value = values.constFind(match.captured(1));
        formatted += value != values.constEnd() ? *value : match.captured(0);
        pos = match.capturedEnd();
    }
    formatted += templ.mid(pos);

    emit message(formatted, LogMisc);
}

void LldbImpl::handleStateReport(const GdbMi &item)
{
    const QString state = item.data();
    if (state == "running") {
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunOk);
    } else if (state == "inferiorrunfailed")
        emit inferiorEvent(InferiorEvent::RunFailed);
    else if (state == "stopped") {
        m_inferiorRunning = false;
        fetchLocationAfterStop(InferiorEvent::SpontaneousStop);
        runCommand({"reportBreakpointHit"});
    } else if (state == "inferiorstopok") {
        m_inferiorRunning = false;
        fetchLocationAfterStop(InferiorEvent::StopOk);
        runCommand({"reportBreakpointHit"});
    } else if (state == "inferiorstopfailed")
        emit inferiorEvent(InferiorEvent::StopFailed);
    else if (state == "inferiorill")
        emit inferiorEvent(InferiorEvent::InferiorIll);
    else if (state == "enginesetupfailed")
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
    else if (state == "enginerunfailed")
        emit inferiorEvent(InferiorEvent::EngineRunFailed);
    else if (state == "enginerunandinferiorrunok") {
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
    } else if (state == "enginerunandinferiorstopok") {
        m_inferiorRunning = false;
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        // Only the attaching paths report this state, and they all leave the inferior
        // stopped. Resume it, as LldbEngine does.
        runCommand({"continueInferior"});
        if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData))
            emit kickoffTerminalProcessRequested();
    }
    else if (state == "enginerunokandinferiorunrunnable")
        emit inferiorEvent(InferiorEvent::RunOkAndInferiorUnrunnable);
    else if (state == "inferiorshutdownfinished")
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
    else if (state == "engineshutdownfinished")
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
    else if (state == "inferiorexited") {
        m_inferiorExited = true;
        reportInferiorExitIfComplete();
    }
}

void LldbImpl::reportInferiorExitIfComplete()
{
    if (!m_inferiorExited || !m_inferiorExitCode || m_inferiorExitReported)
        return;
    m_inferiorExitReported = true;
    emit inferiorDone({*m_inferiorExitCode, InferiorExitStatus::Normal});
}

void LldbImpl::handleLldbOutput(const QString &output)
{
    QStringDecoder decoder(QStringEncoder::System);
    GdbMi all;
    all.fromStringMultiple(output, decoder);

    for (const GdbMi &item : all) {
        const QString name = item.name();
        if (name == "result") {
            const int token = item["token"].toInt();
            if (const auto it = m_commandForToken.find(token); it != m_commandForToken.end()) {
                DebuggerCommand cmd = it.value();
                m_commandForToken.erase(it);
                if (cmd.callback) {
                    DebuggerResponse response;
                    response.token = token;
                    response.resultClass = ResultDone;
                    response.data = item;
                    cmd.callback(response);
                }
            }
        } else if (name == "state") {
            handleStateReport(item);
        } else if (name == "output") {
            emit message(fromHex(item["data"].data()), LogOutput);
        } else if (name == "bridgemessage") {
            emit message(item["msg"].data(), item["channel"].toInt());
        } else if (name == "pid") {
            m_inferiorPid = item.data().toLongLong();
            emit inferiorPidKnown(ProcessHandle(m_inferiorPid));
        } else if (name == "breakpointmodified") {
            emit breakpointModified(translateLldbBreakpointReply(item));
        } else if (name == "interpreterresult") {
            const int token = all["token"].toInt();
            if (const auto it = m_commandForToken.find(token); it != m_commandForToken.end()) {
                DebuggerCommand cmd = it.value();
                m_commandForToken.erase(it);
                if (cmd.callback) {
                    DebuggerResponse response;
                    response.token = token;
                    response.resultClass = ResultDone;
                    response.data = item;
                    cmd.callback(response);
                }
            }
        } else if (name == "interpreterasync") {
            if (all["asyncclass"].data() == "breakpointmodified") {
                GdbMi list;
                list.m_type = GdbMi::List;
                list.addChild(item);
                emit breakpointModified(list);
            }
        } else if (name == "exited") {
            m_inferiorExitCode = item["status"].toInt();
            reportInferiorExitIfComplete();
        } else if (name == "library-loaded") {
            emit libraryEvent(LibraryEvent::Loaded, item);
        } else if (name == "library-unloaded") {
            emit libraryEvent(LibraryEvent::Unloaded, item);
        } else if (name == "tracepointhit") {
            handleTracepointHit(item);
        } else if (name == "signal-received") {
            QString signalName = item["name"].data();
            const QString meaning = item["meaning"].data();
            if (signalName.isEmpty())
                signalName = machExceptionSignalName(meaning);
            emit signalReceived(signalName, meaning);
        }
    }
}

void LldbImpl::runCommand(const DebuggerCommand &command)
{
    const int token = ++m_lastToken;
    DebuggerCommand cmd = command;

    if (!m_lldbProc.isRunning()) {
        emit message(
            QString("LldbImpl: no lldb process running, command ignored: %1").arg(cmd.function),
            LogError);
        if (cmd.callback) {
            DebuggerResponse response;
            response.resultClass = ResultFail;
            cmd.callback(response);
        }
        return;
    }

    QString line;
    if (cmd.flags & DebuggerCommand::NativeCommand) {
        line = cmd.function;
    } else {
        cmd.arg("token", token);
        line = "script theDumper." + cmd.function + "(" + cmd.argsToPython() + ")";
        m_commandForToken[token] = cmd;
    }
    emit message(line, LogInput);
    m_lldbProc.write(line + "\n\n");
}
} // namespace Debugger::Internal
