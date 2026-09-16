// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lldbimpl.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../debuggerinternalconstants.h"
#include "../watchutils.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QRegularExpression>
#include <QTime>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <utility>

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
                      | AddWatcherWhileRunningCapability
                      | BreakConditionCapability
                      | BreakIndividualLocationsCapability
                      | BreakModuleCapability
                      | BreakOnThrowAndCatchCapability
                      | JumpToLineCapability
                      | ReloadModuleCapability
                      | ReloadModuleSymbolsCapability
                      | ResetInferiorCapability
                      | ReturnFromFunctionCapability
                      | RunToLineCapability
                      | SnapshotCapability
                      | TracePointCapability
                      | WatchWidgetsCapability
                      | WatchpointByAddressCapability
                      | WatchpointByExpressionCapability;
    data.extraCapabilities = DebuggerExtraCapability::ContinueAfterAttach
                           | DebuggerExtraCapability::ContinueInsteadOfRun
                           | DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::LibraryEvent
                           | DebuggerExtraCapability::RunCommandDeferral
                           | DebuggerExtraCapability::SignalReceived
                           | DebuggerExtraCapability::SkipKnownFrames
                           | DebuggerExtraCapability::SourceFiles
                           | DebuggerExtraCapability::Threads
                           | DebuggerExtraCapability::BreakOnMain
                           | DebuggerExtraCapability::JumpTargetCheck
                           | DebuggerExtraCapability::PeripheralRegisters
                           | DebuggerExtraCapability::RunAsUser
                           | DebuggerExtraCapability::SpecialBreakpoints
                           | DebuggerExtraCapability::ThreadEvent;
    data.startModes = DebuggerStartModeFlag::Launch
                    | DebuggerStartModeFlag::AttachToProcess
                    | DebuggerStartModeFlag::AttachToTerminalStub
                    | DebuggerStartModeFlag::AttachToRemoteServer
                    | DebuggerStartModeFlag::AttachToCore;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferiorAndCppEditor;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        // lldb stops on a fork or an exec by the libc entry point they go
        // through, and a system call has none.
        if (query.type == BreakpointAtSysCall)
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

// lldb reports a location's address as a plain number, and as -1 for one that
// is not resolved yet.
static QString locationAddress(const GdbMi &lldbLocation)
{
    bool ok = false;
    const quint64 address = lldbLocation["addr"].data().toULongLong(&ok);
    if (!ok || address == quint64(-1))
        return {};
    return "0x" + QString::number(address, 16);
}

static GdbMi translateLldbBreakpoint(const GdbMi &lldbBkpt)
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
    const QString expression = lldbBkpt["expression"].data();
    const QString catchType = lldbBkpt["catchtype"].data();
    if (!expression.isEmpty()) {
        // A watchpoint sits at no line, so what it watches is the only thing
        // telling it apart from a breakpoint that is forever pending.
        addConst(bkpt, "type", "hw watchpoint");
        addConst(bkpt, "what", expression);
    } else if (catchType.isEmpty()) {
        addConst(bkpt, "type", "breakpoint");
    } else {
        addConst(bkpt, "type", "catchpoint");
        addConst(bkpt, "catch-type", catchType);
    }
    if (lldbBkpt["oneshot"].toInt())
        addConst(bkpt, "disp", "del");
    else
        addConst(bkpt, "disp", "keep");
    const QString condition = fromHex(lldbBkpt["condition"].data());
    if (!condition.isEmpty())
        addConst(bkpt, "cond", condition);
    addConst(bkpt, "times", lldbBkpt["hitcount"].data());
    if (const GdbMi thread = lldbBkpt["thread"]; thread.isValid())
        addConst(bkpt, "thread", thread.data());

    const GdbMi lldbLocations = lldbBkpt["locations"];
    if (lldbLocations.childCount() > 0) {
        // The view names a breakpoint after its first location.
        const GdbMi first = lldbLocations.childAt(0);
        addConst(bkpt, "func", first["function"].data());
        if (const QString address = locationAddress(first); !address.isEmpty())
            addConst(bkpt, "addr", address);
    }
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
            if (const QString address = locationAddress(lldbLocation); !address.isEmpty())
                addConst(location, "addr", address);
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

    return bkpt;
}

static GdbMi translateLldbBreakpointReply(const GdbMi &lldbBkpt)
{
    GdbMi list;
    list.m_type = GdbMi::List;
    list.addChild(translateLldbBreakpoint(lldbBkpt));
    return list;
}

static GdbMi translateLldbRegistersReply(const GdbMi &lldbRegisters)
{
    GdbMi result;
    result.m_type = GdbMi::List;
    for (const GdbMi &lldbRegister : lldbRegisters) {
        GdbMi reg;
        reg.m_type = GdbMi::Tuple;
        addConst(reg, "name", lldbRegister["name"].data());
        addConst(reg, "value", lldbRegister["value"].data());
        addConst(reg, "size", lldbRegister["size"].data());
        addConst(reg, "groups", lldbRegister["groups"].data());
        addConst(reg, "type", gdbRegisterTypeName(lldbRegister["type"].data()));
        result.addChild(reg);
    }
    return result;
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
        addConst("startaddress", lldbModule["loadstart"].data());
        addConst("endaddress", lldbModule["loadend"].data());
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
        addConst("state", lldbSymbol["state"].data());
        addConst("section", lldbSymbol["section"].data());
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
    // Interrupting a Windows inferior goes through the stub, as in LldbEngine.
    m_lldbProc.setUseCtrlCStub(true);

    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        QStringList pending;
        for (const DebuggerCommand &cmd : std::as_const(m_commandForToken)) {
            // A resume command is answered by the next stop, which may never come.
            if (cmd.flags & DebuggerCommand::RunRequest)
                continue;
            pending << cmd.function + "(" + cmd.argsToPython() + ")";
        }
        if (pending.isEmpty())
            return;
        m_watchdog.start();
        emit notResponding(m_startData.watchdogTimeout, pending,
                           m_debuginfodDownloadInProgress
                               ? NotRespondingCause::FetchingDebugInfo
                               : NotRespondingCause::Unknown);
    });

    Utils::CommandLine lldbCommand = m_startData.debuggerRunData.command;
    if (!m_startData.loadInitFile)
        lldbCommand.addArg("--no-lldbinit");
    m_lldbProc.setRunAsUser(m_startData.runAsUser);
    m_lldbProc.setCommand(lldbCommand);
    Environment lldbEnvironment = m_startData.debuggerRunData.environment;
    lldbEnvironment.set("QT_CREATOR_LLDB_PROCESS", "1");
    lldbEnvironment.set("PYTHONUNBUFFERED", "1");
    // lldb has no switch for the daemon, only the servers its environment
    // names, and a fetch from one that does not answer takes the session with
    // it, so they are taken away unless the daemon was asked for.
    if (m_startData.useDebugInfoD.toInt() != Utils::TriState::EnabledValue)
        lldbEnvironment.unset("DEBUGINFOD_URLS");
    m_lldbProc.setEnvironment(lldbEnvironment);
    if (m_startData.debuggerRunData.workingDirectory.isDir())
        m_lldbProc.setWorkingDirectory(m_startData.debuggerRunData.workingDirectory);

    connect(&m_lldbProc, &Process::started, this, [this] {
        runCommand({"script sys.path.insert(1, '" + m_startData.dumperScriptsDir.path() + "')",
                   DebuggerCommand::NativeCommand});
        runCommand({"script from lldbbridge import *", DebuggerCommand::NativeCommand});

        // lldb's own counterpart to gdb's "set print elements": without it a value
        // printed by a console command stops after 1024 characters.
        runCommand({"settings set target.max-string-summary-length 10000",
                    DebuggerCommand::NativeCommand});

        // The loader probes the inferior for a descriptor of its own, which
        // has nothing to answer where no code is generated at run time.
        if (!m_startData.useJitLoader) {
            runCommand({"settings set plugin.jit-loader.gdb.enable off",
                        DebuggerCommand::NativeCommand});
        }

        // lldb builds its symbol index on every run and keeps it only when asked,
        // while the setting is about the cache rather than about who fills it.
        if (m_startData.useIndexCache) {
            runCommand({"settings set symbols.enable-lldb-index-cache true",
                        DebuggerCommand::NativeCommand});
        }

        // lldb avoids the standard library on its own account, which is a part of
        // what the setting asks for, so switching it off has to reach lldb too.
        // A regex setting cannot be unset, only overwritten with a value no
        // symbol name carries.
        if (!m_startData.skipKnownFrames) {
            runCommand({"settings set -- target.process.thread.step-avoid-regexp \"\"",
                        DebuggerCommand::NativeCommand});
        }

        // Through the same path the engine used: it reports what the command
        // printed, which a native command drops on the floor.
        if (!m_startData.startScript.isEmpty()) {
            if (m_startData.startScript.isReadableFile()) {
                executeDebuggerCommand("command source " + m_startData.startScript.path(), {});
            } else {
                emit message("The debugger start script is not accessible: "
                                 + m_startData.startScript.toUserOutput(), LogWarning);
            }
        } else {
            for (const QString &command : m_startData.startupCommands)
                executeDebuggerCommand(command, {});
        }

        // Quoted, since lldb splits the arguments the way a shell does, and
        // through the reporting path, since it refuses a replacement path that
        // does not exist.
        for (const QPair<QString, QString> &mapping : m_startData.sourcePathMap) {
            executeDebuggerCommand("settings append target.source-map \"" + mapping.first
                                       + "\" \"" + mapping.second + '"', {});
        }

        for (const Utils::FilePath &path : m_startData.solibSearchPath) {
            runCommand({"settings append target.exec-search-paths " + path.path(),
                        DebuggerCommand::NativeCommand});
        }

        if (!m_startData.debugInfoLocation.isEmpty()) {
            runCommand({"settings append target.debug-file-search-paths "
                            + m_startData.debugInfoLocation.path(),
                        DebuggerCommand::NativeCommand});
        }

        if (m_startData.extraDumperFile.isReadableFile()) {
            DebuggerCommand dumperModule("addDumperModule");
            dumperModule.arg("path", m_startData.extraDumperFile.path());
            runCommand(dumperModule);
        }
        if (!m_startData.extraDumperCommands.isEmpty())
            executeDebuggerCommand(m_startData.extraDumperCommands, {});

        // addDumperModule only remembers the module; this is what imports it.
        runCommand({"loadDumpers", [this](const DebuggerResponse &response) {
            emit refreshDataReceived(0, RefreshKind::DebuggingHelpers, response.data);
        }});

        DebuggerCommand cmd("setupInferior");
        cmd.arg("breakonmain", m_startData.breakOnMain);
        cmd.arg("mainfunction", m_startData.mainFunctionName);
        cmd.arg("useterminal", false);
        cmd.arg("breakonabort", m_startData.breakOnAbort);
        cmd.arg("breakonwarning", m_startData.breakOnWarning);
        cmd.arg("breakonfatal", m_startData.breakOnFatal);
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("deviceUuid", m_startData.deviceUuid);
        cmd.arg("platform", m_startData.platform);
        if (!m_startData.deviceSymbolsRoot.isEmpty())
            cmd.arg("sysroot", m_startData.deviceSymbolsRoot);
        else if (!m_startData.sysroot.isEmpty())
            cmd.arg("sysroot", m_startData.sysroot.path());
        FilePath coreFileForRunEngine;

        if (const auto *inferiorRunData
                = std::get_if<ProcessRunData>(&m_startData.inferiorStartData)) {
            const FilePath &executable = inferiorRunData->command.executable();
            cmd.arg("executable", executable.path());
            cmd.arg("startmode", int(StartInternal));
            cmd.arg("workingdirectory", inferiorRunData->workingDirectory.path());
            Environment inferiorEnvironment = inferiorRunData->environment;
            // Left alone, lldb sets OS_ACTIVITY_DT_MODE to mirror NSLog to
            // stderr, which takes os_log along with it and turns Qt's own
            // stderr logger off.
            inferiorEnvironment.set("IDE_DISABLED_OS_ACTIVITY_DT_MODE", "1");
            if (m_startData.enableHeapDebugging != TriState::Default
                && !inferiorEnvironment.hasKey(Constants::NO_DEBUG_HEAP)) {
                inferiorEnvironment.set(
                    Constants::NO_DEBUG_HEAP,
                    m_startData.enableHeapDebugging == TriState::Enabled ? "0" : "1");
            }
            cmd.arg("environment", inferiorEnvironment.toStringList());
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
            reportEngineSetupFailed();
            return;
        }
        cmd.callback = [this, coreFile = coreFileForRunEngine](const DebuggerResponse &response) {
            const bool success = response.data["success"].toInt();
            if (!success) {
                reportEngineSetupFailed();
                return;
            }
            reportEngineSetupOk();
            if (!std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
                for (const QString &command : m_startData.postAttachCommands) {
                    const QString trimmed = command.trimmed();
                    if (!trimmed.isEmpty() && !trimmed.startsWith('#'))
                        runCommand({trimmed, DebuggerCommand::NativeCommand});
                }
                // Through the reporting path, so what they print is not lost.
                for (const QString &command : m_startData.afterConnectCommands)
                    executeDebuggerCommand(command, {});
            }
            DebuggerCommand runCmd("runEngine", DebuggerCommand::RunRequest);
            if (!coreFile.isEmpty())
                runCmd.arg("coreFile", coreFile.path());
            runCommand(runCmd);
        };
        runCommand(cmd);
    });
    connect(&m_lldbProc, &Process::readyReadStandardOutput, this, [this] {
        restartWatchdog();
        m_debuginfodDownloadInProgress = false;
        const QString out = m_lldbProc.readAllStandardOutput();
        // Whatever the debugger itself prints comes this way too, and only the
        // raw output shows it: the protocol items below are all it parses.
        emit message(out, LogOutput);
        m_inbuffer += out;
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
        m_watchdog.stop();
        if (!m_engineSetupReported)
            reportEngineSetupFailed();
        // Before the process result: that one is what a caller deriving the
        // shutdown itself acts on, and it must not arrive first.
        if (m_shuttingDown)
            emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
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
    runCommand({function, [this, mode](const DebuggerResponse &response) {
        // ShutdownFinished is the only outcome the interface has, so a kill that did
        // not work is logged here or nowhere.
        if (mode != ShutdownMode::Detach && !response.data["success"].toInt())
            emit message("Killing the inferior failed: "
                         + response.data["error"]["status"].data()
                         + response.data["status"].data(), LogError);
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
    }});
}

void LldbImpl::shutdownEngine()
{
    if (!m_lldbProc.isRunning()) {
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
        return;
    }
    // The lldb command interpreter's own quit, which the process' done handler
    // then reports: the debugger is gone by the time it is answered, so an
    // answer to wait for is not what ends the shutdown.
    m_shuttingDown = true;
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
        runCommand({"continueInferior", DebuggerCommand::RunRequest,
                    [this](const DebuggerResponse &response) {
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
        // Attaching reports its stop and resumes right after, so "not running"
        // does not mean the inferior is being held: with a resume in flight,
        // lldb refuses the interrupt, and the caller would wait for a stop that
        // is never coming. Ask again once the inferior really runs.
        if (!m_inferiorRunning && !m_resumeAfterAttachPending) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
            runCommand({"markPendingInterrupt"});
            emit interruptTerminalRequested();
            break;
        }
        if (!m_inferiorRunning) {
            m_interruptOnceRunning = true;
            break;
        }
        interruptInferior();
        break;
    case ExecutionCommand::StepIn:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({QLatin1String(request.flag ? "executeStepI" : "executeStep"),
                    DebuggerCommand::RunRequest});
        break;
    case ExecutionCommand::StepOver:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({QLatin1String(request.flag ? "executeNextI" : "executeNext"),
                    DebuggerCommand::RunRequest});
        break;
    case ExecutionCommand::StepOut:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"executeStepOut", DebuggerCommand::RunRequest});
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
        // Whatever puts the hardware back, which has to happen while the old
        // inferior is still there.
        for (const QString &command : m_startData.forResetCommands)
            executeDebuggerCommand(command, {});
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"resetInferior", DebuggerCommand::RunRequest});
        break;
    case ExecutionCommand::RunToLine: {
        emit inferiorEvent(InferiorEvent::RunRequested);
        DebuggerCommand cmd("executeRunToLocation", DebuggerCommand::RunRequest);
        cmd.arg("file", request.context.fileName.path());
        cmd.arg("line", request.context.textPosition.line);
        cmd.arg("address", request.context.address);
        runCommand(cmd);
        break;
    }
    case ExecutionCommand::RunToFunction: {
        emit inferiorEvent(InferiorEvent::RunRequested);
        DebuggerCommand cmd("executeRunToFunction", DebuggerCommand::RunRequest);
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
        runCommand({"executeReturn", DebuggerCommand::RunRequest,
                    [this](const DebuggerResponse &response) {
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

static void addBreakpointArgs(DebuggerCommand &cmd, const BreakpointChangeRequest &request)
{
    cmd.arg("type", int(request.params.type));
    cmd.arg("file", request.params.fileNameForDebugger().path());
    cmd.arg("line", request.params.textPosition.line);
    cmd.arg("ignorecount", request.params.ignoreCount);
    cmd.arg("condition", toHex(request.params.condition));
    cmd.arg("command", toHex(request.params.command));
    cmd.arg("function", request.params.functionName);
    cmd.arg("module", request.params.module);
    cmd.arg("address", request.params.address);
    cmd.arg("expression", request.params.expression);
    cmd.arg("oneshot", request.params.oneShot);
    cmd.arg("enabled", request.params.enabled);
    cmd.arg("threadspec", request.params.threadSpec);
    cmd.arg("tracepoint", request.params.tracepoint);
    cmd.arg("message", toHex(request.params.message));
    cmd.arg("modelid", request.modelId);
}

void LldbImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.op) {
    case BreakpointOp::Insert: {
        if (request.params.type != BreakpointByFileAndLine
                && request.params.type != BreakpointByFunction
                && request.params.type != BreakpointByAddress
                && request.params.type != WatchpointAtAddress
                && request.params.type != WatchpointAtExpression
                && request.params.type != BreakpointAtMain
                && request.params.type != BreakpointAtFork
                && request.params.type != BreakpointAtExec
                && request.params.type != BreakpointAtThrow
                && request.params.type != BreakpointAtCatch) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, false);
            return;
        }
        DebuggerCommand cmd("insertBreakpoint");
        addBreakpointArgs(cmd, request);
        const bool isCppBreakpoint = request.params.isCppBreakpoint();
        if (!isCppBreakpoint)
            cmd.flags |= DebuggerCommand::NeedsTemporaryStop;
        cmd.callback = [this, requestId, isCppBreakpoint](const DebuggerResponse &response) {
            const bool ok = response.resultClass == ResultDone;
            if (!ok) {
                emit breakpointEvent(requestId, BreakpointOp::Insert, false);
                return;
            }
            // The bridge answers a breakpoint it could not make with an invalid
            // one, which is a failed insert and not a breakpoint to show.
            if (isCppBreakpoint && response.data["valid"].toInt() == 0) {
                emit breakpointEvent(requestId, BreakpointOp::Insert, false);
                return;
            }
            if (!isCppBreakpoint) {
                // An interpreter breakpoint the service has not taken yet has no
                // number to report; the availability hook retries it, and the
                // reply to that carries one.
                if (response.data["pending"].toInt()) {
                    emit breakpointEvent(requestId, BreakpointOp::Insert, true);
                    return;
                }
                GdbMi reply;
                reply.m_type = GdbMi::List;
                reply.addChild(response.data);
                emit breakpointEvent(requestId, BreakpointOp::Insert, true, reply);
                return;
            }
            emit breakpointEvent(requestId, BreakpointOp::Insert, true,
                                 translateLldbBreakpointReply(response.data));
        };
        runCommand(cmd);
        break;
    }
    case BreakpointOp::Remove: {
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(requestId, BreakpointOp::Remove, false);
            break;
        }
        // The interpreter hands out numbers of its own, which mean nothing to
        // lldb - and both count from 1, so the wrong one is a live id there.
        if (!request.params.isCppBreakpoint()) {
            DebuggerCommand cmd("removeInterpreterBreakpoint",
                                DebuggerCommand::NeedsTemporaryStop);
            cmd.arg("id", request.responseId);
            runCommand(cmd);
        } else {
            DebuggerCommand cmd("removeBreakpoint");
            cmd.arg("lldbid", request.responseId);
            runCommand(cmd);
        }
        emit breakpointEvent(requestId, BreakpointOp::Remove, true);
        break;
    }
    case BreakpointOp::Update: {
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(requestId, BreakpointOp::Update, false);
            break;
        }
        // The service knows no change command, and its numbers are not lldb's:
        // taking the breakpoint away and setting it anew is what a change is
        // there. Handing lldb the interpreter's number instead rewrites
        // whatever native breakpoint carries it - in a native mixed session
        // that is the debugger's own hook into the service.
        if (!request.params.isCppBreakpoint()) {
            DebuggerCommand removal("removeInterpreterBreakpoint",
                                    DebuggerCommand::NeedsTemporaryStop);
            removal.arg("id", request.responseId);
            runCommand(removal);

            DebuggerCommand cmd("insertBreakpoint", DebuggerCommand::NeedsTemporaryStop);
            addBreakpointArgs(cmd, request);
            cmd.callback = [this, requestId](const DebuggerResponse &response) {
                const bool ok = response.resultClass == ResultDone;
                if (!ok || response.data["pending"].toInt()) {
                    emit breakpointEvent(requestId, BreakpointOp::Update, ok);
                    return;
                }
                GdbMi reply;
                reply.m_type = GdbMi::List;
                reply.addChild(response.data);
                emit breakpointEvent(requestId, BreakpointOp::Update, true, reply);
            };
            runCommand(cmd);
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
        const DumperOptions &options = request.dumperOptions;
        cmd.arg("fancy", options.useDebuggingHelpers);
        cmd.arg("autoderef", request.autoDerefPointers);
        cmd.arg("allowinferiorcalls", request.allowInferiorCalls);
        cmd.arg("dyntype", options.useDynamicType);
        cmd.arg("qobjectnames", options.showQObjectNames);
        cmd.arg("timestamps", options.logTimeStamps);
        cmd.arg("stringcutoff", options.maximalStringLength);
        cmd.arg("displaystringlimit", options.displayStringLimit);
        cmd.arg("qtversion", m_startData.qtVersion);
        cmd.arg("qtnamespace", m_startData.qtNamespace);
        cmd.arg("passexceptions", qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE"));
        cmd.arg("partialvar", request.partialVariable);
        cmd.arg("uninitialized", request.uninitializedVariables);
        cmd.arg("context", request.context);
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("expanded", request.expandedForDumpers());
        cmd.arg("typeformats", request.typeFormats);
        cmd.arg("formats", request.individualFormats);
        cmd.arg("formattypes", request.formatTypes);
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
        cmd.arg("stacklimit", request.stackDepthLimit);
        cmd.arg("context", request.context);
        cmd.arg("extraqml", 0);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            // A fetch the inferior outran answers "No thread" and carries no
            // stack at all. Reporting that empties the view and leaves the
            // engine activating a frame that is not there.
            if (!response.data["stack"].isValid())
                return;
            emit refreshDataReceived(requestId, RefreshKind::FullStack, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::QmlStack: {
        DebuggerCommand cmd("fetchStack");
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        cmd.arg("stacklimit", request.stackDepthLimit);
        cmd.arg("context", request.context);
        cmd.arg("extraqml", 1);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            if (!response.data["stack"].isValid())
                return;
            emit refreshDataReceived(requestId, RefreshKind::FullStack, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Registers: {
        DebuggerCommand cmd("fetchRegisters");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Registers,
                                     translateLldbRegistersReply(response.data["registers"]));
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
        if (modulePath.isEmpty()) {
            // The dumpers answer a fetch without a module about whichever
            // one they looked at last, which is not the module asked about.
            emit message("LldbImpl: cannot fetch the symbols of no module", LogError);
            return;
        }
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
        if (modulePath.isEmpty()) {
            emit message("LldbImpl: cannot fetch the sections of no module", LogError);
            return;
        }
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
        runCommand({"reloadDumpers", [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::DebuggingHelpers, response.data);
        }});
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

void LldbImpl::reportEngineSetupOk()
{
    m_engineSetupReported = true;
    emit inferiorEvent(InferiorEvent::EngineSetupOk);
}

void LldbImpl::reportEngineSetupFailed()
{
    m_engineSetupReported = true;
    emit inferiorEvent(InferiorEvent::EngineSetupFailed);
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
            if (lineNumber != 0 && fileName.exists())
                emit locationChanged(fileName, lineNumber);
        }
        // lldb stops the whole process, so every thread it knows is stopped.
        GdbMi stoppedThread;
        stoppedThread.m_type = GdbMi::Tuple;
        addConst(stoppedThread, "id", "all");
        emit threadEvent(ThreadEvent::Stopped, stoppedThread);
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
    cmd.arg("flavor", m_startData.intelDisassembly ? "intel" : "att");
    cmd.callback = [this, requestId](const DebuggerResponse &response) {
        DisassemblerLines result;
        int bytesLength = 0;
        for (const GdbMi &line : response.data["lines"]) {
            DisassemblerLine dl;
            dl.address = line["address"].toAddress();
            dl.bytes = line["rawdata"].data();
            bytesLength = qMax(bytesLength, int(dl.bytes.size()));
            dl.data = fromHex(line["hexdata"].data());
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
        result.setBytesLength(bytesLength);
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

void LldbImpl::createSnapshot(quint64 requestId)
{
    // The temporary file is only there to pick a free name, lldb writes the
    // snapshot itself.
    FilePath filePath;
    {
        TemporaryFile tf("lldbsnapshot");
        if (!tf.open()) {
            emit snapshotCreated(requestId, false, {});
            return;
        }
        filePath = tf.filePath();
    }
    DebuggerCommand cmd("createSnapshot");
    cmd.arg("path", filePath.path());
    cmd.callback = [this, requestId, filePath](const DebuggerResponse &response) {
        emit snapshotCreated(requestId, response.data["ok"].data() == "1", filePath);
    };
    runCommand(cmd);
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

void LldbImpl::interruptInferior()
{
    runCommand({"interruptInferior", [this](const DebuggerResponse &response) {
        if (response.data["success"].toInt())
            return;
        // A refusal has to be heard: nothing else reports this stop as failed.
        emit message("LldbImpl: cannot interrupt the inferior: "
                     + response.data["error"]["status"].data(), LogError);
        failTemporaryStopQueue();
        emit inferiorEvent(InferiorEvent::StopFailed);
    }});
}

// Nothing is going to serve the queue any more. Left standing it would swallow
// the next stop, including one the user asked for, and the callers would wait
// for an answer that is not coming.
void LldbImpl::failTemporaryStopQueue()
{
    if (m_onStopCommands.isEmpty())
        return;
    m_temporaryStopRequested = false;
    m_onStopWantContinue = false;
    const QList<DebuggerCommand> commands = std::exchange(m_onStopCommands, {});
    for (const DebuggerCommand &queued : commands) {
        if (queued.callback) {
            DebuggerResponse response;
            response.resultClass = ResultFail;
            queued.callback(response);
        }
    }
}

// Whether this stop is one the queue asked for rather than one the user did.
bool LldbImpl::serveTemporaryStop()
{
    // Only the interrupt this queue asked for may serve it. A stop the loader
    // or the user produced leaves the inferior somewhere an inferior call
    // cannot run from - lldb's own image notifier, say.
    if (m_onStopCommands.isEmpty() || !m_temporaryStopRequested)
        return false;
    m_temporaryStopRequested = false;
    m_inferiorRunning = false;
    const QList<DebuggerCommand> commands = m_onStopCommands;
    const bool wantContinue = m_onStopWantContinue;
    m_onStopCommands.clear();
    m_onStopWantContinue = false;
    // The engine did not ask for this stop and must not hear about it. Telling
    // it would have it reload a stack from an inferior that is about to run
    // again, and re-sync the breakpoints - which queues another command of the
    // same kind, so the interrupt repeats for as long as the engine answers.
    for (const DebuggerCommand &queued : commands)
        runCommand(queued);
    if (wantContinue) {
        m_resumingFromTemporaryStop = true;
        runCommand({"continueInferior", DebuggerCommand::RunRequest,
                    [this](const DebuggerResponse &response) {
            if (response.data["success"].toInt())
                return;
            // The inferior stays where the interrupt left it, so the stop that
            // was kept from the engine is now the truth and has to be reported.
            m_resumingFromTemporaryStop = false;
            fetchLocationAfterStop(InferiorEvent::SpontaneousStop);
            runCommand({"reportBreakpointHit"});
        }});
    }
    return true;
}

void LldbImpl::handleStateReport(const GdbMi &item)
{
    const QString state = item.data();
    if (state == "running") {
        m_resumeAfterAttachPending = false;
        m_inferiorRunning = true;
        // lldb resumes the whole process, so every thread it knows runs again.
        GdbMi runningThread;
        runningThread.m_type = GdbMi::Tuple;
        addConst(runningThread, "thread-id", "all");
        emit threadEvent(ThreadEvent::Running, runningThread);
        if (std::exchange(m_resumingFromTemporaryStop, false))
            return;
        // A console command of the user's resumes the inferior without the engine
        // asking, and the run itself can only be reported after a request.
        if (!std::exchange(m_resumeRequested, false))
            emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        if (std::exchange(m_interruptOnceRunning, false))
            interruptInferior();
    } else if (state == "inferiorrunfailed")
        emit inferiorEvent(InferiorEvent::RunFailed);
    else if (state == "continueafternextstop") {
        // A tracepoint, and a breakpoint command that declines its stop, say so
        // before the stop they belong to arrives.
        m_continueAtNextSpontaneousStop = true;
    } else if (state == "stopped") {
        m_resumeAfterAttachPending = false;
        if (std::exchange(m_interruptOnceRunning, false)) {
            m_inferiorRunning = false;
            fetchLocationAfterStop(InferiorEvent::StopOk);
            runCommand({"reportBreakpointHit"});
            return;
        }
        if (std::exchange(m_continueAtNextSpontaneousStop, false)) {
            runCommand({"continueInferior", DebuggerCommand::RunRequest});
            return;
        }
        m_inferiorRunning = false;
        fetchLocationAfterStop(InferiorEvent::SpontaneousStop);
        runCommand({"reportBreakpointHit"});
    } else if (state == "inferiorstopok") {
        if (serveTemporaryStop())
            return;
        m_inferiorRunning = false;
        fetchLocationAfterStop(InferiorEvent::StopOk);
        runCommand({"reportBreakpointHit"});
    } else if (state == "inferiorstopfailed") {
        failTemporaryStopQueue();
        emit inferiorEvent(InferiorEvent::StopFailed);
    } else if (state == "inferiorill")
        emit inferiorEvent(InferiorEvent::InferiorIll);
    else if (state == "enginesetupfailed")
        reportEngineSetupFailed();
    else if (state == "enginerunfailed")
        emit inferiorEvent(InferiorEvent::EngineRunFailed);
    else if (state == "enginerunandinferiorrunok") {
        m_inferiorRunning = true;
        // Attaching leaves the inferior running here; what the run asked for is
        // that the stop which follows does not hold it.
        m_continueAtNextSpontaneousStop = m_startData.continueAfterAttach;
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
    } else if (state == "enginerunandinferiorstopok") {
        m_inferiorRunning = false;
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        // Only the attaching paths report this state, and they all leave the inferior
        // stopped. Whether it stays there is for whoever asked for the attach to say:
        // a process and a bare server are held, while a server that was handed a
        // process or an executable of its own has an inferior to run. The other paths
        // have nobody to hold it for. Note the resume before sending it, or an
        // interrupt arriving in between is refused and then lost.
        bool resume = m_startData.continueAfterAttach;
        if (const auto *remote
                = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData)) {
            resume = resume || m_startData.continueInsteadOfRun || remote->attachPid.isValid()
                     || !remote->remoteExecutable.isEmpty();
        } else if (!std::holds_alternative<AttachToProcessData>(m_startData.inferiorStartData)) {
            resume = true;
        }
        if (resume) {
            m_resumeAfterAttachPending = true;
            runCommand({"continueInferior", DebuggerCommand::RunRequest});
        }
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
        failTemporaryStopQueue();
        reportInferiorExitIfComplete();
    }
}

void LldbImpl::reportInferiorExitIfComplete()
{
    if (!m_inferiorExited || !m_inferiorExitCode || m_inferiorExitReported)
        return;
    m_inferiorExitReported = true;
    emit inferiorDone({*m_inferiorExitCode,
                       m_inferiorExitSignalled ? InferiorExitStatus::Crash
                                               : InferiorExitStatus::Normal,
                       m_inferiorExitSignalName});
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
                reportResponseTime(cmd);
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
            const QString channel = item["channel"].data();
            const LogChannel logChannel = channel == "stdout" ? AppOutput
                                          : channel == "stderr" ? AppError : AppStuff;
            emit message(fromHex(item["data"].data()), logChannel);
        } else if (name == "bridgemessage") {
            emit message(item["msg"].data(), item["channel"].toInt());
        } else if (name == "pid") {
            m_inferiorPid = item.data().toLongLong();
            emit inferiorPidKnown(ProcessHandle(m_inferiorPid));
        } else if (name == "progress") {
            const QString text = item["message"].data();
            // The fetch lldb announces here happens without another word, which
            // is exactly what a debugger that stopped answering looks like.
            if (text.startsWith("Downloading"))
                m_debuginfodDownloadInProgress = true;
            emit progressMessage(text);
        } else if (name == "stopreason") {
            emit stopReasonReported(item.data());
        } else if (name == "breakpointmodified") {
            emit breakpointModified(translateLldbBreakpointReply(item));
        } else if (name == "breakpointhit") {
            emit breakpointTriggered(item["lldbid"].data(), item["thread"].data());
        } else if (name == "watchpointhit") {
            emit watchpointTriggered(item["lldbid"].data(), item["expression"].data(),
                                     item["old"].data(), item["new"].data());
        } else if (name == "breakpointadded") {
            emit breakpointEvent(0, BreakpointOp::Insert, true, translateLldbBreakpoint(item));
        } else if (name == "breakpointremoved") {
            GdbMi removed;
            removed.m_type = GdbMi::Tuple;
            addConst(removed, "number", item["lldbid"].data());
            emit breakpointEvent(0, BreakpointOp::Remove, true, removed);
        } else if (name == "interpreterresult") {
            const int token = all["token"].toInt();
            if (const auto it = m_commandForToken.find(token); it != m_commandForToken.end()) {
                DebuggerCommand cmd = it.value();
                m_commandForToken.erase(it);
                reportResponseTime(cmd);
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
            m_inferiorExitSignalled = item["signalled"].data() == u"1";
            m_inferiorExitSignalName = item["signame"].data();
            reportInferiorExitIfComplete();
        } else if (name == "thread-created") {
            emit threadEvent(ThreadEvent::Created, item);
        } else if (name == "thread-exited") {
            emit threadEvent(ThreadEvent::Exited, item);
        } else if (name == "thread-selected") {
            emit threadEvent(ThreadEvent::Selected, item);
        } else if (name == "thread-group-created") {
            emit threadEvent(ThreadEvent::GroupCreated, item);
        } else if (name == "thread-group-exited") {
            emit threadEvent(ThreadEvent::GroupExited, item);
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

// Talking to the QML service, and anything else that calls into the inferior,
// only works while it is stopped. Hold the command back, interrupt, and resume
// once it has run - the way GdbImpl serves the same flag.
void LldbImpl::runCommand(const DebuggerCommand &command)
{
    if (command.flags & DebuggerCommand::NeedsTemporaryStop) {
        DebuggerCommand cmd = command;
        cmd.flags &= ~DebuggerCommand::NeedsTemporaryStop;
        if (!m_inferiorRunning && !m_resumeAfterAttachPending) {
            runCommand(cmd);
            return;
        }
        m_onStopCommands.append(cmd);
        m_onStopWantContinue = true;
        if (!std::exchange(m_temporaryStopRequested, true)) {
            // With a resume in flight lldb refuses the interrupt. Ask once the
            // inferior really runs, the way execute() does for the user's own.
            if (m_inferiorRunning)
                interruptInferior();
            else
                m_interruptOnceRunning = true;
        }
        return;
    }

    if (command.flags & DebuggerCommand::RunRequest)
        m_resumeRequested = true;

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

    cmd.postTime = QTime::currentTime().msecsSinceStartOfDay();

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
    restartWatchdog();
}

void LldbImpl::reportResponseTime(const DebuggerCommand &command)
{
    if (!m_startData.logTimeStamps)
        return;
    const int elapsed = QTime::fromMSecsSinceStartOfDay(command.postTime)
                            .msecsTo(QTime::currentTime());
    emit message(QString("Response time: %1: %2 s").arg(command.function).arg(elapsed / 1000.),
                 LogTime);
}

void LldbImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_commandForToken.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}
} // namespace Debugger::Internal
