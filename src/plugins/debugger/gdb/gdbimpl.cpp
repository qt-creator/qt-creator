// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gdbimpl.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../procinterrupt.h"
#include "../shared/hostutils.h"
#include "../watchutils.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

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

static QString breakLocation(const ContextData &context)
{
    if (context.address)
        return "*0x" + QString::number(context.address, 16);
    return '"' + context.fileName.path() + "\":" + QString::number(context.textPosition.line);
}

static QString dotEscape(QString str)
{
    str.replace(' ', '.');
    str.replace('\\', '.');
    str.replace('/', '.');
    return str;
}

static DebuggerEngineSetupData gdbImplSetupData()
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
                      | BreakOnThrowAndCatchCapability
                      | JumpToLineCapability
                      | ReloadModuleCapability
                      | ReloadModuleSymbolsCapability
                      | ResetInferiorCapability
                      | ReturnFromFunctionCapability
                      | ReverseSteppingCapability
                      | RunToLineCapability
                      | SnapshotCapability
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

GdbImpl::GdbImpl(const GdbImplStartData &startData)
    : DebuggerEngineInterface(gdbImplSetupData())
    , m_startData(startData)
{
    m_gdbProc.setProcessMode(ProcessMode::Writer);

    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        QStringList pending;
        for (const DebuggerCommand &cmd : std::as_const(m_commandForToken))
            pending << cmd.function;
        if (pending.isEmpty())
            return;
        m_watchdog.start();
        emit notResponding(m_startData.watchdogTimeout, pending);
    });

    CommandLine gdbCommand = m_startData.debuggerRunData.command;
    gdbCommand.addArgs({"-i", "mi", "-quiet"});
    if (!m_startData.isSet(GdbImplFlag::LoadGdbInit))
        gdbCommand.addArg("-nx");
    m_gdbProc.setCommand(gdbCommand);
    m_gdbProc.setEnvironment(m_startData.debuggerRunData.environment);
    if (m_startData.debuggerRunData.workingDirectory.isDir())
        m_gdbProc.setWorkingDirectory(m_startData.debuggerRunData.workingDirectory);

    connect(&m_gdbProc, &Process::started, this, [this] {
        const bool isPlainRun = std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)
            && !std::get<ProcessRunData>(m_startData.inferiorStartData).command.executable().isEmpty();
        if (!isPlainRun)
            emit inferiorEvent(InferiorEvent::EngineSetupOk);

        const bool targetAsync = m_startData.isSet(GdbImplFlag::ForceTargetAsync)
            || std::holds_alternative<AttachToRemoteServerData>(m_startData.inferiorStartData);
        runCommand({QString("-interpreter-exec console \"set target-async %1\"")
                        .arg(QLatin1String(targetAsync ? "on" : "off"))});

        // What GdbEngine::handleGdbStarted() sets, minus the settings-driven ones:
        // pending breakpoints for libraries that are not loaded yet, untruncated
        // values, and no paging of console command output.
        runCommand({"set breakpoint pending on"});
        runCommand({"set print elements 10000"});
        runCommand({"set unwindonsignal on"});
        runCommand({"set width 0"});
        runCommand({"set height 0"});
        runCommand({"set max-completions 1000"});
        if (m_startData.isSet(GdbImplFlag::UseIndexCache))
            runCommand({"set index-cache on"});
        if (m_startData.isSet(GdbImplFlag::MultiInferior))
            runCommand({"set detach-on-fork off"});
        applySearchPaths();

        runCommand({"python sys.path.insert(1, '" + m_startData.dumperScriptsDir.path() + "')"});
        runCommand({"python from gdbbridge import *"});
        loadExtraDumpers();
        runCommand({"loadDumpers", [this, isPlainRun](const DebuggerResponse &) {
            m_dumpersReady = true;
            runCommand({m_startData.isSet(GdbImplFlag::LoadSystemDumpers)
                            ? QLatin1String("importPlainDumpers on")
                            : QLatin1String("importPlainDumpers off")});
            runUserStartupCommands();
            const QList<DebuggerCommand> buffered = m_bufferedDumperCommands;
            m_bufferedDumperCommands.clear();
            for (const DebuggerCommand &cmd : buffered)
                runCommandNow(cmd);

            if (!isPlainRun)
                return;

            const auto &inferiorRunData = std::get<ProcessRunData>(m_startData.inferiorStartData);
            for (const EnvironmentItem &item
                 : m_startData.debuggerRunData.environment.diff(inferiorRunData.environment)) {
                const bool isWindowsPath = HostOsInfo::isWindowsHost()
                    && item.name.compare("path", Qt::CaseInsensitive) == 0;
                const QString name = isWindowsPath ? "PATH" : item.name;
                if (item.operation == EnvironmentItem::Unset
                        || item.operation == EnvironmentItem::SetDisabled) {
                    runCommand({"unset environment " + name});
                } else {
                    if (name != item.name)
                        runCommand({"unset environment " + item.name});
                    runCommand({"-gdb-set environment " + name + '=' + item.value});
                }
            }
            if (!inferiorRunData.workingDirectory.isEmpty())
                runCommand({"cd " + inferiorRunData.workingDirectory.path()});
            if (!inferiorRunData.command.arguments().isEmpty())
                runCommand({"-exec-arguments " + inferiorRunData.command.arguments()});

            runCommand({"-file-exec-and-symbols "
                        + inferiorRunData.command.executable().nativePath(),
                       [this](const DebuggerResponse &response) {
                if (response.resultClass != ResultDone) {
                    emit inferiorEvent(InferiorEvent::EngineSetupFailed);
                    return;
                }

                emit inferiorEvent(InferiorEvent::EngineSetupOk);

                m_runCommandPending = true;
                runCommand({"-exec-run", [this](const DebuggerResponse &response) {
                    m_runCommandPending = false;
                    if (response.resultClass == ResultRunning) {
                        m_inferiorRunning = true;
                        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
                        if (m_interruptOnceRunning) {
                            m_interruptOnceRunning = false;
                            if (!m_interruptRequested) {
                                m_interruptRequested = true;
                                requestInferiorInterrupt();
                            }
                        }
                    } else {
                        if (m_interruptOnceRunning) {
                            m_interruptOnceRunning = false;
                            const QList<DebuggerCommand> commands = m_onStopCommands;
                            m_onStopCommands.clear();
                            m_onStopWantContinue = false;
                            for (const DebuggerCommand &queuedCommand : commands) {
                                if (queuedCommand.callback) {
                                    DebuggerResponse failResponse;
                                    failResponse.resultClass = ResultFail;
                                    queuedCommand.callback(failResponse);
                                }
                            }
                        }
                        emit inferiorEvent(InferiorEvent::EngineRunFailed);
                    }
                }});
            }});
        }});

        if (const auto *attachData = std::get_if<AttachToProcessData>(&m_startData.inferiorStartData)) {
            m_attachPhase = AttachPhase::AwaitingConnect;
            runCommand({"attach " + QString::number(attachData->pid.pid()),
                       [this](const DebuggerResponse &response) {
                handleLocalAttach(response);
            }});
            runCommand({"print 24"});
            return;
        }

        if (const auto *termData = std::get_if<AttachToTerminalStubData>(&m_startData.inferiorStartData)) {
            m_expectTerminalTrap = true;
            m_attachPhase = AttachPhase::AwaitingConnect;
            const qint64 pid = termData->pid.pid();
            const qint64 mainThreadId = termData->mainThreadId;
            const FilePath executable = termData->executable;
            auto sendAttach = [this, pid, mainThreadId] {
                runCommand({"attach " + QString::number(pid),
                           [this, mainThreadId](const DebuggerResponse &response) {
                    handleTerminalStubAttach(response, mainThreadId);
                }});
            };
            if (HostOsInfo::isWindowsHost()) {
                runCommand({"show version",
                           [this, executable, sendAttach](const DebuggerResponse &response) {
                    handleShowVersion(response);
                    if (m_gdbVersion < 100000) {
                        sendAttach();
                        return;
                    }
                    runCommand({"-file-exec-and-symbols " + executable.nativePath(),
                               [this, sendAttach](const DebuggerResponse &response) {
                        if (response.resultClass == ResultDone)
                            sendAttach();
                        else
                            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
                    }});
                }});
            } else {
                sendAttach();
            }
            return;
        }

        if (const auto *remoteData = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData)) {
            const QString channel = remoteData->channel;
            const bool needsFollowUp = remoteData->attachPid.isValid()
                                       || !remoteData->remoteExecutable.isEmpty();
            const bool useQnxTarget = remoteData->useQnxTarget;
            if (HostOsInfo::isWindowsHost() && m_startData.isSet(GdbImplFlag::ElfTarget))
                runCommand({"set osabi GNU/Linux"});
            auto connectToTarget = [this, channel, needsFollowUp, useQnxTarget] {
                if (!needsFollowUp)
                    m_attachPhase = AttachPhase::AwaitingConnect;
                const QLatin1String connectCommand = useQnxTarget
                    ? QLatin1String("target qnx ")
                    : needsFollowUp ? QLatin1String("target extended-remote ")
                                    : QLatin1String("target remote ");
                runCommand({connectCommand + channel,
                           [this](const DebuggerResponse &response) {
                    handleTargetRemote(response);
                }});
            };
            if (remoteData->symbolFile.isEmpty()) {
                connectToTarget();
            } else {
                runCommand({"-file-exec-and-symbols " + remoteData->symbolFile.nativePath(),
                           [this, connectToTarget](const DebuggerResponse &response) {
                    if (response.resultClass == ResultDone)
                        connectToTarget();
                    else
                        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
                }});
            }
            return;
        }

        if (const auto *coreData = std::get_if<AttachToCoreData>(&m_startData.inferiorStartData)) {
            if (HostOsInfo::isWindowsHost() && m_startData.isSet(GdbImplFlag::ElfTarget))
                runCommand({"set osabi GNU/Linux"});
            runCommand({"-file-exec-file " + coreData->executable.nativePath()});
            runCommand({"-file-symbol-file " + coreData->executable.nativePath(),
                       [this, coreFile = coreData->coreFile](const DebuggerResponse &response) {
                if (response.resultClass != ResultDone) {
                    emit inferiorEvent(InferiorEvent::EngineSetupFailed);
                    return;
                }
                runCommand({"target core " + coreFile.nativePath(),
                           [this](const DebuggerResponse &) {
                    emit inferiorEvent(InferiorEvent::RunOkAndInferiorUnrunnable);
                }});
            }});
            return;
        }
    });
    connect(&m_gdbProc, &Process::readyReadStandardOutput, this, [this] {
        restartWatchdog();
        m_inbuffer += m_gdbProc.readAllStandardOutput();
        int newline;
        while ((newline = m_inbuffer.indexOf('\n')) >= 0) {
            QString line = m_inbuffer.left(newline);
            m_inbuffer.remove(0, newline + 1);
            if (line.endsWith('\r'))
                line.chop(1);
            handleOutputLine(line);
        }
    });
    connect(&m_gdbProc, &Process::readyReadStandardError, this, [this] {
        emit message(m_gdbProc.readAllStandardError(), LogError);
    });
    connect(&m_gdbProc, &Process::done, this, [this] {
        m_watchdog.stop();
        m_outputCollector.shutdown();
        if (m_gdbProc.result() == ProcessResult::StartFailed)
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        emit engineProcessFinished(m_gdbProc.resultData());
    });
    connect(&m_outputCollector, &OutputCollector::byteDelivery, this, [this](const QByteArray &ba) {
        emit message(m_outputDecoder.decode(ba), AppStuff);
    });
}

GdbImpl::~GdbImpl()
{
    if (m_gdbProc.isRunning())
        m_gdbProc.write("kill\r\n");
}

bool GdbImpl::usesOutputCollector() const
{
    return std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)
           && m_startData.debuggerRunData.command.executable().isLocal();
}

void GdbImpl::start()
{
    if (usesOutputCollector()) {
        if (m_outputCollector.listen()) {
            CommandLine gdbCommand = m_gdbProc.commandLine();
            gdbCommand.addArg("--tty=" + m_outputCollector.serverName());
            m_gdbProc.setCommand(gdbCommand);
        } else {
            emit message(QString("GdbImpl: cannot set up communication with the child process: %1")
                             .arg(m_outputCollector.errorString()), LogError);
        }
    }
    m_gdbProc.start();
}

void GdbImpl::handleLocalAttach(const DebuggerResponse &response)
{
    const bool stoppedAlready = (m_attachPhase == AttachPhase::Stopped);
    m_attachPhase = AttachPhase::Idle;
    if (stoppedAlready)
        return;
    if (response.resultClass == ResultDone || response.resultClass == ResultRunning) {
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
    } else {
        emit inferiorEvent(InferiorEvent::EngineIll);
    }
}

void GdbImpl::handleTerminalStubAttach(const DebuggerResponse &response, qint64 mainThreadId)
{
    if (response.resultClass != ResultDone && response.resultClass != ResultRunning) {
        m_attachPhase = AttachPhase::Idle;
        emit inferiorEvent(InferiorEvent::EngineIll);
        return;
    }
    if (HostOsInfo::isWindowsHost()) {
        m_attachPhase = AttachPhase::Idle;
        QString errorMessage;
        if (!winResumeThread(mainThreadId, &errorMessage)) {
            emit message("Inferior attached, unable to resume thread "
                         + QString::number(mainThreadId) + ": " + errorMessage, LogWarning);
        }
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        runRunRequestCommand("-exec-continue");
        return;
    }
    if (m_attachPhase == AttachPhase::AwaitingConnect) {
        m_attachPhase = AttachPhase::Stopped;
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        continueAfterAttach();
    }
    emit kickoffTerminalProcessRequested();
}

void GdbImpl::handleShowVersion(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        return;
    int gdbBuildVersion = -1;
    bool isMacGdb = false;
    bool isQnxGdb = false;
    extractGdbVersion(response.consoleStreamOutput,
                      &m_gdbVersion, &gdbBuildVersion, &isMacGdb, &isQnxGdb);
}

void GdbImpl::handleTargetRemote(const DebuggerResponse &response)
{
    const auto &remoteData = std::get<AttachToRemoteServerData>(m_startData.inferiorStartData);
    if (!remoteData.attachPid.isValid() && remoteData.remoteExecutable.isEmpty()) {
        const bool stoppedAlready = (m_attachPhase == AttachPhase::Stopped);
        m_attachPhase = AttachPhase::Idle;
        if (stoppedAlready)
            return;
        if (response.resultClass == ResultDone) {
            for (const QString &command : m_startData.userCommands.afterConnect)
                runCommand({command, DebuggerCommand::NativeCommand});
            emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        } else {
            emit inferiorEvent(InferiorEvent::EngineIll);
        }
        return;
    }

    if (response.resultClass != ResultDone) {
        emit inferiorEvent(InferiorEvent::EngineIll);
        return;
    }
    for (const QString &command : m_startData.userCommands.afterConnect)
        runCommand({command, DebuggerCommand::NativeCommand});
    if (remoteData.attachPid.isValid()) {
        m_attachPhase = AttachPhase::AwaitingConnect;
        runCommand({"attach " + QString::number(remoteData.attachPid.pid()),
                   [this](const DebuggerResponse &response) {
            handleExtendedRemoteAttach(response);
        }});
    } else {
        QString command;
        if (remoteData.useQnxTarget)
            command = "set nto-executable " + remoteData.remoteExecutable.nativePath();
        else
            command = "-gdb-set remote exec-file " + remoteData.remoteExecutable.nativePath();
        runCommand({command, [this](const DebuggerResponse &response) {
            handleExtendedRemoteAttach(response);
        }});
    }
}

void GdbImpl::continueAfterAttach()
{
    if (m_attachPhase == AttachPhase::Continuing)
        return;
    m_attachPhase = AttachPhase::Continuing;
    runCommand({"-exec-continue", [this](const DebuggerResponse &response) {
        m_attachPhase = AttachPhase::Idle;
        if (response.resultClass == ResultRunning) {
            m_inferiorRunning = true;
            emit inferiorEvent(InferiorEvent::RunOk);
        } else {
            emit inferiorEvent(InferiorEvent::RunFailed);
        }
    }});
}

void GdbImpl::handleExtendedRemoteAttach(const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        emit inferiorEvent(InferiorEvent::EngineIll);
        return;
    }
    const auto &remoteData = std::get<AttachToRemoteServerData>(m_startData.inferiorStartData);
    if (remoteData.attachPid.isValid()) {
        if (m_attachPhase != AttachPhase::AwaitingConnect)
            return;
        m_attachPhase = AttachPhase::Stopped;
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        continueAfterAttach();
    } else {
        m_runCommandPending = true;
        runCommand({"-exec-run", [this](const DebuggerResponse &response) {
            m_runCommandPending = false;
            if (response.resultClass == ResultRunning) {
                m_inferiorRunning = true;
                emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
            } else {
                emit inferiorEvent(InferiorEvent::EngineRunFailed);
            }
        }});
    }
}

void GdbImpl::shutdownInferior(ShutdownMode mode)
{
    runCommand({mode == ShutdownMode::Detach ? QLatin1String("detach") : QLatin1String("kill"),
               DebuggerCommand::NativeCommand, [this](const DebuggerResponse &) {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
    }});
}

void GdbImpl::shutdownEngine()
{
    if (!m_gdbProc.isRunning()) {
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
        return;
    }
    runCommand({"-gdb-exit", [this](const DebuggerResponse &response) {
        if (response.resultClass == ResultExit)
            return;
        m_gdbProc.kill();
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
    }});
}

static QString parseTemporaryBreakpointNumber(const QString &consoleStreamOutput)
{
    static const QRegularExpression re("Temporary breakpoint (\\d+) at ");
    QRegularExpressionMatchIterator it = re.globalMatch(consoleStreamOutput);
    QRegularExpressionMatch match;
    while (it.hasNext())
        match = it.next();
    return match.hasMatch() ? match.captured(1) : QString();
}

static bool resumesInferior(ExecutionCommand command)
{
    switch (command) {
    case ExecutionCommand::Continue:
    case ExecutionCommand::StepOver:
    case ExecutionCommand::StepIn:
    case ExecutionCommand::StepOut:
    case ExecutionCommand::Return:
    case ExecutionCommand::RunToLine:
    case ExecutionCommand::RunToFunction:
    case ExecutionCommand::JumpToLine:
    case ExecutionCommand::ResetInferior:
        return true;
    case ExecutionCommand::Interrupt:
    case ExecutionCommand::Detach:
    case ExecutionCommand::Abort:
    case ExecutionCommand::RecordReverse:
    case ExecutionCommand::RepeatLastCommand:
        break;
    }
    return false;
}

void GdbImpl::execute(const ExecutionRequest &request)
{
    if (resumesInferior(request.command))
        setTokenBarrier();

    switch (request.command) {
    case ExecutionCommand::Continue:
        if (m_startData.isSet(GdbImplFlag::NativeMixedDebugging) && request.currentFrameIsQml)
            runRunRequestCommand("executeContinue");
        else
            runRunRequestCommand("-exec-continue");
        break;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning && !m_runCommandPending) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        if (!m_inferiorRunning) {
            m_interruptOnceRunning = true;
            break;
        }
        m_interruptRequested = true;
        requestInferiorInterrupt();
        break;
    case ExecutionCommand::StepOver:
        if (m_startData.isSet(GdbImplFlag::NativeMixedDebugging) && request.currentFrameIsQml && !request.flag)
            runRunRequestCommand("executeNext");
        else
            runRunRequestCommand(request.flag ? QLatin1String("-exec-next-instruction")
                                              : QLatin1String("-exec-next"));
        break;
    case ExecutionCommand::StepIn:
        if (m_startData.isSet(GdbImplFlag::NativeMixedDebugging) && request.currentFrameIsQml && !request.flag) {
            runRunRequestCommand("executeStep");
        } else if (!request.flag) {
            if (m_startData.isSet(GdbImplFlag::NativeMixedDebugging))
                runCommand({"armInterpreterStepIn"});
            runRunRequestCommand("-exec-step");
        } else {
            runRunRequestCommand("-exec-step-instruction");
        }
        break;
    case ExecutionCommand::StepOut:
        if (m_startData.isSet(GdbImplFlag::NativeMixedDebugging) && request.currentFrameIsQml)
            runRunRequestCommand("executeStepOut");
        else if (m_startData.isSet(GdbImplFlag::NativeMixedDebugging))
            runRunRequestCommand("executeNativeMixedStepOut");
        else
            runRunRequestCommand("-exec-finish");
        break;
    case ExecutionCommand::Return:
        emit inferiorEvent(InferiorEvent::RunRequested);
        runCommand({"-exec-return", [this](const DebuggerResponse &response) {
            if (response.resultClass == ResultDone) {
                m_inferiorRunning = false;
                emit inferiorEvent(InferiorEvent::StopOk);
            } else {
                emit inferiorEvent(InferiorEvent::RunFailed);
            }
        }});
        break;
    case ExecutionCommand::Detach:
        runCommand({"detach", DebuggerCommand::NativeCommand,
                   [this](const DebuggerResponse &) {
            emit inferiorDone({0, InferiorExitStatus::Detached});
        }});
        break;
    case ExecutionCommand::ResetInferior:
        for (const QString &command : m_startData.userCommands.forReset) {
            runCommand({command, DebuggerCommand::NativeCommand
                                     | DebuggerCommand::NeedsTemporaryStop});
        }
        runCommand({"kill", DebuggerCommand::NativeCommand});
        if (const auto *inferiorRunData = std::get_if<ProcessRunData>(&m_startData.inferiorStartData);
                inferiorRunData && !inferiorRunData->command.executable().isEmpty()) {
            runRunRequestCommand("-exec-run");
        }
        break;
    case ExecutionCommand::Abort:
        m_gdbProc.kill();
        break;
    case ExecutionCommand::RunToLine:
        runCommand({"tbreak " + breakLocation(request.context),
                   [this](const DebuggerResponse &response) {
            registerInternalBreakpointNumber(
                parseTemporaryBreakpointNumber(response.consoleStreamOutput));
        }});
        runRunRequestCommand("continue", DebuggerCommand::NativeCommand);
        break;
    case ExecutionCommand::RunToFunction:
        runCommand({"-break-insert -t " + request.functionName,
                   [this](const DebuggerResponse &response) {
            registerInternalBreakpointNumber(response.data["bkpt"]["number"].data());
        }});
        runRunRequestCommand("-exec-continue");
        break;
    case ExecutionCommand::JumpToLine:
        runCommand({"tbreak " + breakLocation(request.context),
                   [this](const DebuggerResponse &response) {
            registerInternalBreakpointNumber(
                parseTemporaryBreakpointNumber(response.consoleStreamOutput));
        }});
        runRunRequestCommand("jump " + breakLocation(request.context));
        break;
    case ExecutionCommand::RecordReverse:
        runCommand({request.flag ? QLatin1String("record full")
                                 : QLatin1String("record stop")});
        break;
    case ExecutionCommand::RepeatLastCommand:
        if (!m_lastDebuggableCommand.function.isEmpty())
            runCommand(m_lastDebuggableCommand);
        break;
    }
}

static QString reverseBacktrace(const QString &trace)
{
    static const QRegularExpression threadPattern(R"(Thread \d+ \(Thread )");
    QTC_CHECK(threadPattern.isValid());

    if (!trace.contains(threadPattern))
        return trace;

    const QStringView traceView{trace};
    QList<QStringView> threadTraces;
    const auto traceSize = traceView.size();
    for (qsizetype pos = 0; pos < traceSize; ) {
        auto nextThreadPos = traceView.indexOf(threadPattern, pos + 1);
        if (nextThreadPos == -1)
            nextThreadPos = traceSize;
        threadTraces.append(traceView.sliced(pos, nextThreadPos - pos));
        pos = nextThreadPos;
    }

    QString result;
    result.reserve(traceSize);
    for (auto it = threadTraces.crbegin(), end = threadTraces.crend(); it != end; ++it) {
        result += *it;
        if (result.endsWith('\n'))
            result += '\n';
    }
    return result;
}

void GdbImpl::refresh(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.kind) {
    case RefreshKind::FullBacktrace: {
        DebuggerCommand cmd("thread apply all bt full",
                            DebuggerCommand::NeedsTemporaryStop | DebuggerCommand::ConsoleCommand);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone)
                return;
            emit refreshDataReceived(requestId, RefreshKind::FullBacktrace,
                                     constMi({}, reverseBacktrace(response.consoleStreamOutput)
                                                     + response.logStreamOutput));
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Locals: {
        DebuggerCommand cmd("fetchVariables");
        const DumperOptions &options = request.dumperOptions;
        cmd.arg("fancy", options.useDebuggingHelpers);
        cmd.arg("autoderef", request.autoDerefPointers);
        cmd.arg("dyntype", options.useDynamicType);
        cmd.arg("qobjectnames", options.showQObjectNames);
        cmd.arg("timestamps", options.logTimeStamps);
        cmd.arg("stringcutoff", options.maximalStringLength);
        cmd.arg("displaystringlimit", options.displayStringLimit);
        cmd.arg("qtversion", m_startData.qtVersion);
        cmd.arg("qtnamespace", m_startData.qtNamespace);
        cmd.arg("passexceptions", qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE"));
        cmd.arg("resultvarname", m_resultVarName);
        cmd.arg("partialvar", request.partialVariable);
        cmd.arg("context", request.context);
        cmd.arg("nativemixed", m_startData.isSet(GdbImplFlag::NativeMixedDebugging));
        cmd.arg("allowinferiorcalls", request.allowInferiorCalls);
        cmd.arg("expanded", request.expandedForDumpers());
        cmd.arg("typeformats", request.typeFormats);
        cmd.arg("formats", request.individualFormats);
        cmd.arg("formattypes", request.formatTypes);
        cmd.arg("watchers", request.watchers);

        m_lastDebuggableCommand = cmd;
        m_lastDebuggableCommand.arg("passexceptions", true);

        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Locals, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::FullStack: {
        DebuggerCommand cmd("fetchStack");
        cmd.arg("limit", -1);
        cmd.arg("nativemixed", m_startData.isSet(GdbImplFlag::NativeMixedDebugging));
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::FullStack, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Threads: {
        runCommand({"-thread-info", [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Threads, response.data);
        }});
        return;
    }
    case RefreshKind::QmlStack: {
        DebuggerCommand cmd("fetchStack");
        cmd.arg("limit", -1);
        cmd.arg("nativemixed", m_startData.isSet(GdbImplFlag::NativeMixedDebugging));
        cmd.arg("extraqml", true);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::FullStack, response.data);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Registers: {
        if (m_registerNamesListed) {
            fetchRegisterValues(requestId);
            return;
        }
        DebuggerCommand cmd("maintenance print register-groups");
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            if (response.resultClass == ResultDone) {
                m_registerNamesListed = true;
                const QStringList lines = response.consoleStreamOutput.split('\n');
                for (int i = 1; i < lines.size(); ++i) {
                    const QStringList parts = lines.at(i).split(' ', Qt::SkipEmptyParts);
                    if (parts.size() < 6)
                        continue;
                    RegisterInfo info;
                    info.name = parts.at(0);
                    info.size = parts.at(4).toInt();
                    info.reportedType = parts.at(5);
                    m_registerInfoByNumber[parts.at(1).toInt()] = info;
                }
            }
            fetchRegisterValues(requestId);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::Modules: {
        DebuggerCommand cmd("info shared", DebuggerCommand::NeedsTemporaryStop);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            handleModulesList(requestId, response);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::PeripheralRegisters: {
        for (const quint64 requestedAddress : request.addresses) {
            DebuggerCommand cmd("x/1u 0x" + QString::number(requestedAddress, 16));
            cmd.callback = [this, requestId](const DebuggerResponse &response) {
                if (response.resultClass != ResultDone)
                    return;
                static const QRegularExpression re(
                    "^(0x[0-9A-Fa-f]+)(?:\\s<[^>]*>)?:\\t(\\d+)\\n$");
                const QRegularExpressionMatch m = re.match(response.consoleStreamOutput);
                if (!m.hasMatch())
                    return;
                bool addressOk = false;
                bool valueOk = false;
                const quint64 address = m.captured(1).toULongLong(&addressOk, 16);
                const quint64 value = m.captured(2).toULongLong(&valueOk, 10);
                if (!addressOk || !valueOk)
                    return;
                GdbMi result;
                result.m_type = GdbMi::Tuple;
                result.addChild(constMi("address", QString::number(address)));
                result.addChild(constMi("value", QString::number(value)));
                emit refreshDataReceived(requestId, RefreshKind::PeripheralRegisters, result);
            };
            runCommand(cmd);
        }
        return;
    }
    case RefreshKind::SourceFiles: {
        DebuggerCommand cmd("-file-list-exec-source-files",
                            DebuggerCommand::NeedsTemporaryStop);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone)
                return;
            GdbMi result;
            result.m_type = GdbMi::List;
            for (const GdbMi &item : response.data["files"]) {
                const QString file = item["file"].data();
                if (file.endsWith("<built-in>"))
                    continue;
                const GdbMi fullName = item["fullname"];
                GdbMi entry;
                entry.m_type = GdbMi::Tuple;
                entry.addChild(constMi("file", file));
                if (fullName.isValid())
                    entry.addChild(constMi("fullname", fullName.data()));
                result.addChild(entry);
            }
            emit refreshDataReceived(requestId, RefreshKind::SourceFiles, result);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::AllSymbols:
        runCommand({"sharedlibrary .*"});
        refresh({requestId, RefreshKind::Modules});
        refresh({requestId, RefreshKind::FullStack});
        refresh({requestId, RefreshKind::Locals});
        return;
    case RefreshKind::StackSymbols:
        runCommand({"sharedlibrary " + dotEscape(request.path.path())});
        return;
    case RefreshKind::DebuggingHelpers:
        runCommand({"reloadDumpers"});
        refresh({requestId, RefreshKind::Locals});
        return;
    case RefreshKind::ModuleSymbols: {
        auto tempFile = std::make_shared<TemporaryFile>("gdbsymbols");
        if (!tempFile->open()) {
            emit message("GdbImpl: cannot create a temp file for module symbols", LogWarning);
            return;
        }
        const FilePath tempFilePath = tempFile->filePath();
        tempFile->close();
        const FilePath modulePath = request.path;
        DebuggerCommand cmd("maint print msymbols -objfile " + modulePath.path()
                            + " -- \"" + tempFilePath.path() + "\"",
                            DebuggerCommand::NeedsTemporaryStop);
        cmd.callback = [this, requestId, modulePath, tempFilePath, tempFile]
                       (const DebuggerResponse &response) {
            handleModuleSymbols(requestId, modulePath, tempFilePath, response);
        };
        runCommand(cmd);
        return;
    }
    case RefreshKind::ModuleSections: {
        const FilePath modulePath = request.path;
        requestModuleSections(requestId, modulePath, false);
        return;
    }
    default:
        emit message("GdbImpl::refresh() does not support this kind yet", LogWarning);
        return;
    }
}

void GdbImpl::fetchRegisterValues(quint64 requestId)
{
    DebuggerCommand cmd("-data-list-register-values r", DebuggerCommand::Discardable);
    cmd.callback = [this, requestId](const DebuggerResponse &response) {
        GdbMi result;
        result.m_type = GdbMi::List;
        if (response.resultClass == ResultDone) {
            for (const GdbMi &item : response.data["register-values"]) {
                const auto it = m_registerInfoByNumber.constFind(item["number"].toInt());
                if (it == m_registerInfoByNumber.constEnd())
                    continue;
                GdbMi reg;
                reg.m_type = GdbMi::Tuple;
                reg.addChild(constMi("name", it->name));
                reg.addChild(constMi("size", QString::number(it->size)));
                reg.addChild(constMi("type", it->reportedType));
                reg.addChild(constMi("value", item["value"].data()));
                result.addChild(reg);
            }
        }
        emit refreshDataReceived(requestId, RefreshKind::Registers, result);
    };
    runCommand(cmd);
}

void GdbImpl::handleModulesList(quint64 requestId, const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone)
        return;

    GdbMi result;
    result.m_type = GdbMi::List;
    QString data = response.consoleStreamOutput;
    QTextStream ts(&data, QIODevice::ReadOnly);
    while (!ts.atEnd()) {
        QString line = ts.readLine();
        QTextStream lineStream(&line, QIODevice::ReadOnly);
        QString symbolsRead;
        quint64 startAddress = 0;
        quint64 endAddress = 0;
        FilePath modulePath;
        if (line.startsWith("0x")) {
            lineStream >> startAddress >> endAddress >> symbolsRead;
            modulePath = FilePath::fromUserInput(lineStream.readLine().trimmed());
        } else if (line.trimmed().startsWith("No")) {
            lineStream >> symbolsRead;
            modulePath = FilePath::fromUserInput(lineStream.readLine().trimmed());
        } else {
            continue;
        }
        GdbMi module;
        module.m_type = GdbMi::Tuple;
        module.addChild(constMi("modulepath", modulePath.toUrlishString()));
        module.addChild(constMi("startaddress", QString::number(startAddress)));
        module.addChild(constMi("endaddress", QString::number(endAddress)));
        module.addChild(constMi("symbolsread", symbolsRead));
        result.addChild(module);
    }
    emit refreshDataReceived(requestId, RefreshKind::Modules, result);
}

void GdbImpl::handleModuleSymbols(quint64 requestId, const FilePath &modulePath,
                                 const FilePath &tempFilePath, const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        emit message("GdbImpl: cannot read symbols for module " + modulePath.toUserOutput(),
                     LogWarning);
        return;
    }

    QFile file(tempFilePath.toFSPathString());
    if (!file.open(QIODevice::ReadOnly)) {
        emit message("GdbImpl: cannot open module symbols temp file", LogWarning);
        return;
    }
    GdbMi symbolList;
    symbolList.m_type = GdbMi::List;
    symbolList.m_name = "symbols";
    const QStringList lines = QString::fromLocal8Bit(file.readAll()).split('\n');
    for (const QString &line : lines) {
        if (line.isEmpty() || line.at(0) != '[')
            continue;
        int posCode = line.indexOf(']') + 2;
        int posAddress = line.indexOf("0x", posCode);
        if (posAddress == -1)
            continue;
        int posName = line.indexOf(' ', posAddress);
        int lenAddress = posName - posAddress;
        int posSection = line.indexOf(" section ");
        int lenName = 0;
        int lenSection = 0;
        int posDemangled = 0;
        if (posSection == -1) {
            lenName = line.size() - posName;
            posDemangled = posName;
        } else {
            lenName = posSection - posName;
            posSection += 10;
            posDemangled = line.indexOf(' ', posSection + 1);
            if (posDemangled == -1) {
                lenSection = line.size() - posSection;
            } else {
                lenSection = posDemangled - posSection;
                posDemangled += 1;
            }
        }
        int lenDemangled = 0;
        if (posDemangled != -1)
            lenDemangled = line.size() - posDemangled;
        GdbMi symbol;
        symbol.m_type = GdbMi::Tuple;
        symbol.addChild(constMi("state", line.mid(posCode, 1)));
        symbol.addChild(constMi("address", line.mid(posAddress, lenAddress)));
        symbol.addChild(constMi("name", line.mid(posName, lenName)));
        symbol.addChild(constMi("section", line.mid(posSection, lenSection)));
        symbol.addChild(constMi("demangled", line.mid(posDemangled, lenDemangled)));
        symbolList.addChild(symbol);
    }
    file.close();
    file.remove();

    GdbMi result;
    result.m_type = GdbMi::Tuple;
    result.addChild(constMi("modulepath", modulePath.toUrlishString()));
    result.addChild(symbolList);
    emit refreshDataReceived(requestId, RefreshKind::ModuleSymbols, result);
}

void GdbImpl::requestModuleSections(quint64 requestId, const FilePath &modulePath,
                                    bool useLegacyAllObjKeyword)
{
    DebuggerCommand cmd;
    if (useLegacyAllObjKeyword)
        cmd = DebuggerCommand("maint info sections ALLOBJ", DebuggerCommand::NeedsTemporaryStop);
    else
        cmd = DebuggerCommand("maint info sections -all-objects",
                              DebuggerCommand::NeedsTemporaryStop);
    cmd.callback = [this, requestId, modulePath, useLegacyAllObjKeyword]
                   (const DebuggerResponse &response) {
        handleModuleSections(requestId, modulePath, response, useLegacyAllObjKeyword);
    };
    runCommand(cmd);
}

void GdbImpl::handleModuleSections(quint64 requestId, const FilePath &modulePath,
                                   const DebuggerResponse &response,
                                   bool isRetryWithLegacyKeyword)
{
    if (response.resultClass != ResultDone)
        return;

    static const QRegularExpression headerRe(
        "^(?:Exec file|Object file): `(.*)', file type .*\\.$");
    static const QRegularExpression bareHeaderRe("^(?:Exec file|Object file):$");
    static const QRegularExpression headerContinuationRe("^\\s*`(.*)', file type .*\\.$");
    static const QRegularExpression sectionRe(
        "^\\s*(?:\\[\\d+\\]\\s+)?(0x[0-9A-Fa-f]+)->(0x[0-9A-Fa-f]+) at (0x[0-9A-Fa-f]+):\\s+(\\S+)(.*)$");

    const QStringList lines = response.consoleStreamOutput.split('\n');
    GdbMi sectionList;
    sectionList.m_type = GdbMi::List;
    sectionList.m_name = "sections";
    bool active = false;
    bool moduleFound = false;
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        QString headerPath;
        bool isHeader = false;
        if (const QRegularExpressionMatch headerMatch = headerRe.match(line); headerMatch.hasMatch()) {
            headerPath = headerMatch.captured(1);
            isHeader = true;
        } else if (bareHeaderRe.match(line).hasMatch() && i + 1 < lines.size()) {
            if (const QRegularExpressionMatch continuationMatch
                    = headerContinuationRe.match(lines.at(i + 1)); continuationMatch.hasMatch()) {
                headerPath = continuationMatch.captured(1);
                isHeader = true;
                ++i;
            }
        }
        if (isHeader) {
            if (active)
                break;
            active = headerPath == modulePath.path();
            moduleFound = moduleFound || active;
            continue;
        }
        if (!active)
            continue;
        const QRegularExpressionMatch sectionMatch = sectionRe.match(line);
        if (!sectionMatch.hasMatch())
            continue;
        GdbMi section;
        section.m_type = GdbMi::Tuple;
        section.addChild(constMi("from", sectionMatch.captured(1)));
        section.addChild(constMi("to", sectionMatch.captured(2)));
        section.addChild(constMi("address", sectionMatch.captured(3)));
        section.addChild(constMi("name", sectionMatch.captured(4)));
        section.addChild(constMi("flags", sectionMatch.captured(5).trimmed()));
        sectionList.addChild(section);
    }

    if (moduleFound && sectionList.childCount() == 0 && !isRetryWithLegacyKeyword) {
        requestModuleSections(requestId, modulePath, true);
        return;
    }

    GdbMi result;
    result.m_type = GdbMi::Tuple;
    result.addChild(constMi("modulepath", modulePath.toUrlishString()));
    result.addChild(sectionList);
    emit refreshDataReceived(requestId, RefreshKind::ModuleSections, result);
}

void GdbImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.op) {
    case BreakpointOp::Insert:
        insertBreakpointCommand(request);
        break;
    case BreakpointOp::Remove:
        if (!request.params.isCppBreakpoint()) {
            DebuggerCommand cmd("removeInterpreterBreakpoint");
            cmd.arg("id", request.responseId);
            runCommand(cmd);
        } else {
            runCommand({"-break-delete " + request.responseId,
                        DebuggerCommand::NeedsTemporaryStop});
        }
        m_tracepointsByNumber.remove(request.responseId);
        emit breakpointEvent(requestId, BreakpointOp::Remove, true);
        break;
    case BreakpointOp::Update:
        updateBreakpointCommand(request);
        break;
    case BreakpointOp::EnableSub:
        runCommand({(request.enabled ? "-break-enable " : "-break-disable ") + request.subResponseId,
                    [this, requestId](const DebuggerResponse &response) {
            emit breakpointEvent(requestId, BreakpointOp::EnableSub,
                                 response.resultClass == ResultDone);
        }});
        break;
    }
}

static QString gdbBreakpointLocation(const BreakpointParameters &params, const QString &mainFunction)
{
    switch (params.type) {
    case BreakpointAtThrow:
        return "__cxa_throw";
    case BreakpointAtCatch:
        return "__cxa_begin_catch";
    case BreakpointAtMain:
        return mainFunction;
    case BreakpointByFunction:
        return "--function \"" + params.functionName + '"';
    case BreakpointByAddress:
        return "*0x" + QString::number(params.address, 16);
    default:
        return "\"\\\"" + GdbMi::escapeCString(params.fileName.path()) + "\\\":"
               + QString::number(params.textPosition.line) + '"';
    }
}

static QList<GdbImplTracepointCaptureData> parseTracepointCaptures(const QString &message)
{
    static const QRegularExpression capsRegExp(
        "(^|[^\\\\])(\\$(ADDRESS|CALLER|CALLSTACK|FILEPOS|FUNCTION|PID|PNAME|TICK|TID|TNAME)"
        "|{[^}]+})");
    QList<GdbImplTracepointCaptureData> caps;
    QRegularExpressionMatch match = capsRegExp.match(message, 0);
    while (match.hasMatch()) {
        const QString t = match.captured(2);
        const int start = int(match.capturedStart(2));
        const int end = int(match.capturedEnd(2));
        if (t[0] == '$') {
            GdbImplTracepointCaptureType type;
            if (t == "$ADDRESS")
                type = GdbImplTracepointCaptureType::Address;
            else if (t == "$CALLER")
                type = GdbImplTracepointCaptureType::Caller;
            else if (t == "$CALLSTACK")
                type = GdbImplTracepointCaptureType::Callstack;
            else if (t == "$FILEPOS")
                type = GdbImplTracepointCaptureType::FilePos;
            else if (t == "$FUNCTION")
                type = GdbImplTracepointCaptureType::Function;
            else if (t == "$PID")
                type = GdbImplTracepointCaptureType::Pid;
            else if (t == "$PNAME")
                type = GdbImplTracepointCaptureType::ProcessName;
            else if (t == "$TICK")
                type = GdbImplTracepointCaptureType::Tick;
            else if (t == "$TID")
                type = GdbImplTracepointCaptureType::Tid;
            else if (t == "$TNAME")
                type = GdbImplTracepointCaptureType::ThreadName;
            else
                QTC_ASSERT(false, continue);
            caps.append({type, {}, start, end});
        } else {
            caps.append({GdbImplTracepointCaptureType::Expression,
                        t.mid(1, t.size() - 2), start, end});
        }
        match = capsRegExp.match(message, match.capturedEnd());
    }
    return caps;
}

void GdbImpl::insertBreakpointCommand(const BreakpointChangeRequest &request)
{
    const BreakpointParameters &params = request.params;
    const quint64 requestId = request.requestId;

    if (params.type == WatchpointAtAddress || params.type == WatchpointAtExpression) {
        const QString function = "watch " + (params.type == WatchpointAtAddress
                                             ? "*0x" + QString::number(params.address, 16)
                                             : params.expression);
        runCommand({function, [this, requestId](const DebuggerResponse &response) {
            handleWatchInsert(requestId, response);
        }});
        return;
    }

    if (params.type == BreakpointAtFork || params.type == BreakpointAtExec
        || params.type == BreakpointAtSysCall) {
        QString catchpoint;
        if (params.type == BreakpointAtFork)
            catchpoint = "fork";
        else if (params.type == BreakpointAtExec)
            catchpoint = "exec";
        else
            catchpoint = "syscall";
        runCommand({"catch " + catchpoint, [this, requestId](const DebuggerResponse &response) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, response.resultClass == ResultDone);
        }});
        if (params.type == BreakpointAtFork)
            runCommand({"catch vfork"});
        return;
    }

    if (!params.isCppBreakpoint()) {
        DebuggerCommand cmd("insertInterpreterBreakpoint", DebuggerCommand::NeedsTemporaryStop);
        cmd.arg("modelid", request.modelId);
        cmd.arg("file", params.fileName.path());
        cmd.arg("line", params.textPosition.line);
        cmd.arg("enabled", params.enabled);
        cmd.arg("condition", toHex(params.condition));
        cmd.arg("ignorecount", params.ignoreCount);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            handleInterpreterBreakpointInsert(requestId, response);
        };
        runCommand(cmd);
        return;
    }

    if (params.isTracepoint() && params.type == BreakpointByFileAndLine) {
        DebuggerCommand cmd("createTracepoint");
        if (params.oneShot)
            cmd.arg("temporary", true);
        if (params.ignoreCount)
            cmd.arg("ignore_count", params.ignoreCount);
        if (!params.condition.isEmpty()) {
            QString condition = params.condition;
            cmd.arg("condition", condition.replace('"', "\\\""));
        }
        if (params.threadSpec >= 0)
            cmd.arg("thread", params.threadSpec);

        const QList<GdbImplTracepointCaptureData> captures =
            parseTracepointCaptures(params.message);
        if (!captures.isEmpty()) {
            QJsonArray caps;
            for (const GdbImplTracepointCaptureData &cap : captures) {
                QJsonArray capJson;
                capJson.append(static_cast<int>(cap.type));
                capJson.append(cap.expression.isEmpty() ? QJsonValue(QJsonValue::Null)
                                                        : QJsonValue(cap.expression));
                caps.append(capJson);
            }
            cmd.arg("caps", caps);
        }

        cmd.arg("passexceptions", false);
        cmd.arg("fancy", true);
        cmd.arg("allowinferiorcalls", true);
        cmd.arg("autoderef", true);
        cmd.arg("dyntype", true);
        cmd.arg("qobjectnames", true);
        cmd.arg("nativemixed", m_startData.isSet(GdbImplFlag::NativeMixedDebugging));
        cmd.arg("stringcutoff", 10000);
        cmd.arg("displaystringlimit", 100);

        cmd.arg("spec", QString(GdbMi::escapeCString(params.fileName.path()) + ':'
                                + QString::number(params.textPosition.line)));
        cmd.flags = DebuggerCommand::NeedsTemporaryStop;
        const QString message = params.message;
        cmd.callback = [this, requestId, message, captures](const DebuggerResponse &response) {
            handleTracepointInsert(requestId, response, message, captures);
        };
        runCommand(cmd);
        return;
    }

    QString function = "-break-insert ";
    if (params.threadSpec >= 0)
        function += "-p " + QString::number(params.threadSpec) + ' ';
    function += "-f ";
    if (params.isTracepoint())
        function += "-a ";
    if (params.oneShot)
        function += "-t ";
    if (!params.enabled)
        function += "-d ";
    if (params.ignoreCount)
        function += "-i " + QString::number(params.ignoreCount) + ' ';
    if (!params.condition.isEmpty()) {
        QString condition = params.condition;
        function += " -c \"" + condition.replace('"', "\\\"") + "\" ";
    }
    function += gdbBreakpointLocation(params, m_startData.mainFunctionName);

    runCommand({function, DebuggerCommand::NeedsTemporaryStop,
               [this, requestId](const DebuggerResponse &response) {
        emit breakpointEvent(requestId, BreakpointOp::Insert,
                             response.resultClass == ResultDone, response.data);
    }});
}

void GdbImpl::handleInterpreterBreakpointInsert(quint64 requestId, const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        emit breakpointEvent(requestId, BreakpointOp::Insert, false);
        return;
    }
    if (response.data["pending"].toInt()) {
        // Retried from the object-availability hook, which the dumpers armed.
        m_interpreterBreakpointsPending = true;
        emit breakpointEvent(requestId, BreakpointOp::Insert, true);
        return;
    }
    GdbMi data;
    data.m_type = GdbMi::List;
    data.addChild(response.data);
    emit breakpointEvent(requestId, BreakpointOp::Insert, true, data);
}

void GdbImpl::handleTracepointInsert(quint64 requestId, const DebuggerResponse &response,
                                     const QString &message,
                                     const QList<GdbImplTracepointCaptureData> &captures)
{
    const GdbMi tracepoint = response.data["tracepoint"];
    if (response.resultClass != ResultDone || tracepoint.childCount() == 0) {
        emit breakpointEvent(requestId, BreakpointOp::Insert, false);
        return;
    }
    const QString number = tracepoint.childAt(0)["number"].data();
    if (!number.isEmpty())
        m_tracepointsByNumber[number] = {message, captures};
    emit breakpointEvent(requestId, BreakpointOp::Insert, true, tracepoint);
}

void GdbImpl::handleWatchInsert(quint64 requestId, const DebuggerResponse &response)
{
    if (response.resultClass != ResultDone) {
        emit breakpointEvent(requestId, BreakpointOp::Insert, false);
        return;
    }

    QString numberStr;
    quint64 address = 0;
    const GdbMi wpt = response.data["wpt"];
    if (wpt.isValid()) {
        numberStr = wpt["number"].data();
        const QString exp = wpt["exp"].data();
        if (exp.startsWith('*'))
            address = exp.mid(1).toULongLong(nullptr, 0);
    } else {
        const QString consoleOutput = response.consoleStreamOutput;
        if (consoleOutput.startsWith("Hardware watchpoint ")
            || consoleOutput.startsWith("Watchpoint ")) {
            const int end = consoleOutput.indexOf(':');
            const int begin = consoleOutput.lastIndexOf(' ', end) + 1;
            const QString addressStr = consoleOutput.mid(end + 2).trimmed();
            numberStr = consoleOutput.mid(begin, end - begin);
            if (addressStr.startsWith('*'))
                address = addressStr.mid(1).toULongLong(nullptr, 0);
        }
    }
    if (numberStr.isEmpty()) {
        emit message("GdbImpl: cannot parse watchpoint from " + response.consoleStreamOutput,
                     LogWarning);
        emit breakpointEvent(requestId, BreakpointOp::Insert, false);
        return;
    }

    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", numberStr));
    if (address)
        bkpt.addChild(constMi("addr", "0x" + QString::number(address, 16)));
    GdbMi data;
    data.m_type = GdbMi::List;
    data.addChild(bkpt);
    emit breakpointEvent(requestId, BreakpointOp::Insert, true, data);
}

void GdbImpl::registerInternalBreakpointNumber(const QString &number)
{
    if (!number.isEmpty())
        m_internalBreakpointNumbers.insert(number);
}

void GdbImpl::handleTracepointHit(const GdbMi &data)
{
    const GdbMi result = data["result"];
    const QString number = result["number"].data();
    const auto it = m_tracepointsByNumber.constFind(number);
    if (it == m_tracepointsByNumber.constEnd())
        return;

    QString formatted = it->message;
    const GdbMi miCaps = result["caps"];
    const QList<GdbImplTracepointCaptureData> &caps = it->captures;
    if (caps.size() == miCaps.childCount()) {
        for (int i = caps.size() - 1; i >= 0; --i) {
            const GdbImplTracepointCaptureData &cap = caps.at(i);
            const GdbMi miCap = miCaps.childAt(i);
            switch (cap.type) {
            case GdbImplTracepointCaptureType::Callstack: {
                QStringList frames;
                for (const GdbMi &frame : miCap)
                    frames.append(frame.data());
                formatted.replace(cap.start, cap.end - cap.start, frames.join(" <- "));
                break;
            }
            case GdbImplTracepointCaptureType::Expression: {
                const QString key = miCap.data();
                const GdbMi expression = data["expressions"][key.toLatin1().data()];
                if (expression.isValid()) {
                    const QString value = decodeData(expression["value"].data(),
                                                     expression["valueencoded"].data());
                    formatted.replace(cap.start, cap.end - cap.start, value);
                }
                break;
            }
            default:
                formatted.replace(cap.start, cap.end - cap.start, miCap.data());
            }
        }
    }
    emit message(formatted, LogMisc);
}

void GdbImpl::updateBreakpointCommand(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    if (request.responseId.isEmpty()) {
        emit breakpointEvent(requestId, BreakpointOp::Update, false);
        return;
    }

    const BreakpointParameters &params = request.params;
    const QString bpnr = request.responseId;
    runCommand({(params.enabled ? "-break-enable " : "-break-disable ") + bpnr,
                DebuggerCommand::NeedsTemporaryStop});
    // An empty condition clears a previously set one.
    runCommand({"condition " + bpnr + ' ' + params.condition,
                DebuggerCommand::NeedsTemporaryStop});
    runCommand({"ignore " + bpnr + ' ' + QString::number(params.ignoreCount),
                DebuggerCommand::NeedsTemporaryStop});

    emit breakpointEvent(requestId, BreakpointOp::Update, true);
}

void GdbImpl::selectThread(const QString &threadId)
{
    runCommand({"-thread-select " + threadId, DebuggerCommand::Discardable});
}

void GdbImpl::activateFrame(int index)
{
    runCommand({"-stack-select-frame " + QString::number(index), DebuggerCommand::Discardable});
}

void GdbImpl::setRegisterValue(const QString &name, const QString &value)
{
    QString fullName = name;
    if (name.startsWith("xmm"))
        fullName += ".uint128";
    runCommand({"set $" + fullName + "=" + value, DebuggerCommand::Discardable});
}

void GdbImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                           const QByteArray &data)
{
    if (op == MemoryOp::Change) {
        for (int i = 0; i < data.size(); ++i) {
            runCommand({"-data-write-memory 0x" + QString::number(addr + i, 16) + " d 1 "
                        + QString::number(uint(static_cast<unsigned char>(data.at(i)))),
                        DebuggerCommand::NeedsTemporaryStop});
        }
        return;
    }

    MemoryRequestCookie cookie;
    cookie.accumulator = std::make_shared<QByteArray>(lengthOrSize, char());
    cookie.pendingRequests = std::make_shared<int>(1);
    cookie.requestId = requestId;
    cookie.base = addr;
    cookie.length = lengthOrSize;
    fetchMemoryHelper(cookie);
}

void GdbImpl::fetchMemoryHelper(const MemoryRequestCookie &cookie)
{
    DebuggerCommand cmd("-data-read-memory 0x" + QString::number(cookie.base + cookie.offset, 16)
                        + " x 1 1 " + QString::number(cookie.length),
                        DebuggerCommand::NeedsTemporaryStop);
    cmd.callback = [this, cookie](const DebuggerResponse &response) {
        handleFetchMemory(response, cookie);
    };
    runCommand(cmd);
}

void GdbImpl::handleFetchMemory(const DebuggerResponse &response, const MemoryRequestCookie &cookie)
{
    --*cookie.pendingRequests;
    if (response.resultClass == ResultDone) {
        const GdbMi memory = response.data["memory"];
        if (memory.childCount() != 0) {
            int i = 0;
            for (const GdbMi &byte : memory.childAt(0)["data"])
                (*cookie.accumulator)[cookie.offset + i++] = char(byte.data().toUInt(nullptr, 0));
        }
    } else if (cookie.length > 1) {
        *cookie.pendingRequests += 2;
        const quint64 hunk = cookie.length / 2;
        MemoryRequestCookie first = cookie;
        first.length = hunk;
        MemoryRequestCookie second = cookie;
        second.length = cookie.length - hunk;
        second.offset = cookie.offset + hunk;
        fetchMemoryHelper(first);
        fetchMemoryHelper(second);
    }

    if (*cookie.pendingRequests <= 0)
        emit memoryDataReceived(cookie.requestId, cookie.base, *cookie.accumulator);
}

QChar GdbImpl::mixedDisasmFlag() const
{
    return m_gdbVersion >= 71100 ? 's' : 'm';
}

bool GdbImpl::reportDisassemblyIfUsable(quint64 requestId, quint64 address,
                                       const QString &consoleStreamOutput)
{
    const DisassemblerLines lines = parseCliDisassembly(consoleStreamOutput);
    const bool usable = address ? lines.coversAddress(address) : !lines.data().isEmpty();
    if (usable)
        emit disassemblyReceived(requestId, lines);
    return usable;
}

void GdbImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName)
{
    if (address == 0 && functionName.isEmpty()) {
        emit message("GdbImpl::fetchDisassembly() needs an address or a function name",
                     LogWarning);
        return;
    }
    fetchDisassemblyPointMixed(requestId, address, functionName);
}

void GdbImpl::fetchDisassemblyPointMixed(quint64 requestId, quint64 address,
                                         const QString &functionName)
{
    const QString target = address ? "0x" + QString::number(address, 16) : functionName;
    DebuggerCommand cmd("disassemble /r" + QString(mixedDisasmFlag()) + ' ' + target,
                        DebuggerCommand::Discardable | DebuggerCommand::ConsoleCommand);
    cmd.callback = [this, requestId, address](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone
            && reportDisassemblyIfUsable(requestId, address, response.consoleStreamOutput)) {
            return;
        }
        if (address == 0) {
            emit message("GdbImpl: disassembly by function name failed: "
                         + response.data["msg"].data(), LogWarning);
            return;
        }
        fetchDisassemblyRangeMixed(requestId, address);
    };
    runCommand(cmd);
}

void GdbImpl::fetchDisassemblyRangeMixed(quint64 requestId, quint64 address)
{
    const QString start = QString::number(address - 20, 16);
    const QString end = QString::number(address + 100, 16);
    DebuggerCommand cmd("disassemble /r" + QString(mixedDisasmFlag())
                            + " 0x" + start + ",0x" + end,
                        DebuggerCommand::Discardable | DebuggerCommand::ConsoleCommand);
    cmd.callback = [this, requestId, address](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone
            && reportDisassemblyIfUsable(requestId, address, response.consoleStreamOutput)) {
            return;
        }
        fetchDisassemblyRangePlain(requestId, address);
    };
    runCommand(cmd);
}

void GdbImpl::fetchDisassemblyRangePlain(quint64 requestId, quint64 address)
{
    const QString start = QString::number(address - 20, 16);
    const QString end = QString::number(address + 100, 16);
    DebuggerCommand cmd("disassemble /r 0x" + start + ",0x" + end,
                        DebuggerCommand::Discardable);
    cmd.callback = [this, requestId, address](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone
            && reportDisassemblyIfUsable(requestId, address, response.consoleStreamOutput)) {
            return;
        }
        emit message("GdbImpl: disassembly failed: " + response.data["msg"].data(), LogWarning);
    };
    runCommand(cmd);
}

void GdbImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    DebuggerCommand cmd("assignValue");
    cmd.arg("type", toHex(item.type));
    cmd.arg("expr", toHex(expr));
    cmd.arg("value", toHex(value));
    cmd.arg("simpleType", isIntOrFloatType(item.type));
    runCommand(cmd);
}

void GdbImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    runCommand({"set {int}0x" + QString::number(address, 16) + '=' + QString::number(value)});
}

void GdbImpl::watchPoint(quint64 requestId, const QPoint &pnt)
{
    DebuggerCommand cmd("watchPoint", DebuggerCommand::NeedsFullStop);
    cmd.arg("x", pnt.x());
    cmd.arg("y", pnt.y());
    cmd.callback = [this, requestId](const DebuggerResponse &response) {
        emit watchPointResolved(requestId, response.data["selected"].toAddress(),
                                response.data["expr"].data());
    };
    runCommand(cmd);
}

void GdbImpl::createSnapshot(quint64 requestId)
{
    FilePath filePath;
    TemporaryFile tf("gdbsnapshot");
    if (!tf.open()) {
        emit snapshotCreated(requestId, false, {});
        return;
    }
    filePath = tf.filePath();
    tf.close();
    DebuggerCommand cmd("gcore " + filePath.path(),
                        DebuggerCommand::NeedsTemporaryStop | DebuggerCommand::ConsoleCommand);
    cmd.callback = [this, requestId, filePath](const DebuggerResponse &response) {
        emit snapshotCreated(requestId, response.resultClass == ResultDone, filePath);
    };
    runCommand(cmd);
}

void GdbImpl::executeDebuggerCommand(const QString &command,
                               const WatchItemData &inspectorItem)
{
    Q_UNUSED(inspectorItem)
    runCommand({command, DebuggerCommand::NativeCommand | DebuggerCommand::NeedsTemporaryStop});
}

void GdbImpl::runRunRequestCommand(const QString &function, int flags)
{
    emit inferiorEvent(InferiorEvent::RunRequested);
    m_runCommandPending = true;
    runCommand({function, flags, [this](const DebuggerResponse &response) {
        m_runCommandPending = false;
        if (response.resultClass == ResultRunning) {
            m_inferiorRunning = true;
            emit inferiorEvent(InferiorEvent::RunOk);
            if (m_interruptOnceRunning) {
                m_interruptOnceRunning = false;
                if (!m_interruptRequested) {
                    m_interruptRequested = true;
                    requestInferiorInterrupt();
                }
            }
            return;
        }
        if (m_interruptOnceRunning) {
            m_interruptOnceRunning = false;
            const QList<DebuggerCommand> commands = m_onStopCommands;
            m_onStopCommands.clear();
            m_onStopWantContinue = false;
            for (const DebuggerCommand &queuedCommand : commands) {
                if (queuedCommand.callback) {
                    DebuggerResponse failResponse;
                    failResponse.resultClass = ResultFail;
                    queuedCommand.callback(failResponse);
                }
            }
        }
        emit inferiorEvent(response.data["msg"].data() == "The program is not being run."
                           ? InferiorEvent::InferiorIll : InferiorEvent::RunFailed);
    }});
}

void GdbImpl::runCommand(const DebuggerCommand &command)
{
    if (command.flags & (DebuggerCommand::NeedsTemporaryStop | DebuggerCommand::NeedsFullStop)) {
        DebuggerCommand cmd = command;
        const bool wantContinue = bool(cmd.flags & DebuggerCommand::NeedsTemporaryStop);
        cmd.flags &= ~(DebuggerCommand::NeedsTemporaryStop | DebuggerCommand::NeedsFullStop);
        if (m_inferiorRunning) {
            m_onStopCommands.append(cmd);
            m_onStopWantContinue = wantContinue;
            if (!m_interruptRequested) {
                m_interruptRequested = true;
                requestInferiorInterrupt();
            }
            return;
        }
        if (m_runCommandPending) {
            m_onStopCommands.append(cmd);
            m_onStopWantContinue = wantContinue;
            m_interruptOnceRunning = true;
            return;
        }
        runCommandNow(cmd);
        return;
    }
    runCommandNow(command);
}

void GdbImpl::requestInferiorInterrupt()
{
    if (const auto *attachData = std::get_if<AttachToProcessData>(&m_startData.inferiorStartData)) {
        QString errorMessage;
        if (!interruptProcess(attachData->pid.pid(), &errorMessage))
            emit message(errorMessage, LogError);
    } else if (std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
        QString errorMessage;
        if (!interruptProcess(m_inferiorPid, &errorMessage))
            emit message(errorMessage, LogError);
    } else if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
        emit interruptTerminalRequested();
    } else {
        runCommandNow({"-exec-interrupt"});
    }
}

void GdbImpl::runCommandNow(const DebuggerCommand &command)
{
    bool isPythonCommand = true;
    if ((command.flags & DebuggerCommand::NativeCommand) || command.function.contains('-')
        || command.function.contains(' '))
        isPythonCommand = false;

    if (isPythonCommand && !m_dumpersReady && command.function != "loadDumpers"
        && command.function != "addDumperModule") {
        m_bufferedDumperCommands.append(command);
        return;
    }

    const int token = ++m_lastToken;
    DebuggerCommand cmd = command;

    if (!m_gdbProc.isRunning()) {
        emit message(
            QString("GdbImpl: no gdb process running, command ignored: %1").arg(cmd.function),
            LogError);
        if (cmd.callback) {
            DebuggerResponse response;
            response.resultClass = ResultFail;
            cmd.callback(response);
        }
        return;
    }

    if (isPythonCommand) {
        cmd.arg("token", token);
        cmd.function = "python theDumper." + cmd.function + "(" + cmd.argsToPython() + ")";
    }

    m_commandForToken[token] = cmd;
    const QString line = QString::number(token) + cmd.function;
    emit message(line, LogInput);
    m_gdbProc.write(line + "\r\n");
    if (!cmd.function.endsWith("-gdb-exit"))
        restartWatchdog();
}

void GdbImpl::loadExtraDumpers()
{
    if (m_startData.extraDumperFile.isReadableFile()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", m_startData.extraDumperFile.path());
        runCommand(cmd);
    }
    if (!m_startData.extraDumperCommands.isEmpty())
        runCommand({m_startData.extraDumperCommands, DebuggerCommand::NativeCommand});
}

void GdbImpl::applySearchPaths()
{
    const GdbImplSearchPaths &paths = m_startData.searchPaths;
    if (!paths.sysRoot.isEmpty()) {
        runCommand({"set sysroot " + paths.sysRoot.path()});
        // sysroot alone does not locate the sources, so relocate the most likely
        // place for the debug source as well.
        runCommand({"set substitute-path /usr/src " + paths.sysRoot.path() + "/usr/src"});
    }
    for (auto it = paths.sourcePathMap.cbegin(), end = paths.sourcePathMap.cend(); it != end; ++it)
        runCommand({"set substitute-path " + it.key() + " " + it.value()});
    for (const QString &directory : paths.debugSourceLocation) {
        if (Utils::FilePath::fromUserInput(directory).isDir())
            runCommand({"directory " + directory});
        else
            emit message("# directory does not exist: " + directory, LogInput);
    }
    if (!paths.debugInfoLocation.isEmpty() && paths.debugInfoLocation.exists()) {
        runCommand({"show debug-file-directory", [this](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone)
                return;
            const QString current = response.consoleStreamOutput.split('"').value(1);
            QString command = "set debug-file-directory "
                              + m_startData.searchPaths.debugInfoLocation.path();
            if (!current.isEmpty())
                command += Utils::HostOsInfo::pathListSeparator() + current;
            runCommand({command});
        }});
    }
    if (!paths.solibSearchPath.isEmpty()) {
        DebuggerCommand cmd("appendSolibSearchPath");
        cmd.arg("path", Utils::transform(paths.solibSearchPath, &Utils::FilePath::path));
        cmd.arg("separator", Utils::HostOsInfo::pathListSeparator());
        runCommand(cmd);
    }
}

void GdbImpl::runUserStartupCommands()
{
    const GdbImplUserCommands &commands = m_startData.userCommands;
    if (!commands.startScript.isEmpty()) {
        if (commands.startScript.isReadableFile()) {
            runCommand({"source " + commands.startScript.path()});
        } else {
            emit message("The debugger start script is not accessible: "
                             + commands.startScript.toUserOutput(), LogWarning);
        }
        return;
    }
    if (!commands.atStartup.isEmpty())
        runCommand({commands.atStartup, DebuggerCommand::NativeCommand});
}

void GdbImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_commandForToken.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}

void GdbImpl::setTokenBarrier()
{
    emit message("--- token barrier ---", LogMiscInput);
    m_oldestAcceptableToken = m_lastToken;
}

void GdbImpl::handleOutputLine(const QString &line)
{
    if (line.isEmpty() || line == "(gdb) ")
        return;

    emit message(line, LogOutput);

    DebuggerOutputParser parser(line, m_outputDecoder);
    const int token = parser.readInt();

    switch (parser.readChar().unicode()) {
    case '^': {
        DebuggerResponse response;
        response.token = token;

        const QStringView resultClass = parser.readString([](char c) {
            return (c >= 'a' && c <= 'z') || c == '-';
        });
        if (resultClass == u"done")
            response.resultClass = ResultDone;
        else if (resultClass == u"running")
            response.resultClass = ResultRunning;
        else if (resultClass == u"connected")
            response.resultClass = ResultConnected;
        else if (resultClass == u"error")
            response.resultClass = ResultFail;
        else if (resultClass == u"exit")
            response.resultClass = ResultExit;
        else
            response.resultClass = ResultUnknown;

        if (!parser.isAtEnd() && parser.isCurrent(',')) {
            parser.advance();
            response.data.parseTuple_helper(parser);
            response.data.m_type = GdbMi::Tuple;
        }

        response.consoleStreamOutput = m_pendingConsoleStreamOutput;
        response.logStreamOutput = m_pendingLogStreamOutput;

        if (response.data.data().isEmpty()) {
            const int pos = m_pendingConsoleStreamOutput.indexOf("result={");
            response.data.fromString(
                pos >= 0 ? m_pendingConsoleStreamOutput.mid(pos) : m_pendingConsoleStreamOutput,
                m_outputDecoder);
        }
        m_pendingConsoleStreamOutput.clear();
        m_pendingLogStreamOutput.clear();

        handleResultRecord(&response);
        break;
    }
    case '*': {
        const QStringView asyncClass = parser.readString([](char c) {
            return (c >= 'a' && c <= 'z') || c == '-';
        });

        GdbMi result;
        if (!parser.isAtEnd() && parser.isCurrent(',')) {
            parser.advance();
            result.parseTuple_helper(parser);
            result.m_type = GdbMi::Tuple;
        }

        if (asyncClass == u"stopped") {
            if (m_inNativeMixedStep) {
                // The dumper steps from QML into a C++ method internally. Only the
                // final landing, reported after the 'left' marker, should surface.
                emit message("INTERMEDIATE *stopped NOTIFICATION IGNORED", LogWarning);
                break;
            }

            m_inferiorRunning = false;
            m_pendingConsoleStreamOutput.clear();
            m_pendingLogStreamOutput.clear();

            // A finished function leaves its value in a convenience variable, which
            // the dumpers report as the return value of the frame just left.
            const GdbMi resultVar = result["gdb-result-var"];
            m_resultVarName = resultVar.isValid() ? resultVar.data() : QString();

            const QString reason = result["reason"].data();

            // The interpreter is reachable from here on, so a breakpoint it refused
            // earlier can be inserted now. The dumpers stay out of it: an inferior
            // call from inside this frame suspends the inferior's other threads.
            // Only the hook's own stop is swallowed - the dumpers announce it,
            // since an internal breakpoint carries no number to recognize - and a
            // queued command or a requested interrupt is served first: the hook
            // stays until the queue is empty, so it comes back.
            const bool fromInterpreterHook = std::exchange(m_interpreterHookStop, false);
            if (fromInterpreterHook && m_interpreterBreakpointsPending
                    && m_onStopCommands.isEmpty() && !m_interruptRequested
                    && result["frame"]["func"].data() == u"qt_qmlDebugObjectAvailable") {
                runCommand({"resolveInterpreterBreakpoints",
                            DebuggerCommand::NeedsTemporaryStop,
                           [this](const DebuggerResponse &response) {
                    m_interpreterBreakpointsPending = response.data["pending"].toInt() > 0;
                    runCommand({"-exec-continue", [this](const DebuggerResponse &reply) {
                        m_inferiorRunning = reply.resultClass == ResultRunning;
                    }});
                }});
                break;
            }

            if (m_expectTerminalTrap) {
                if (HostOsInfo::isWindowsHost() && reason.isEmpty()) {
                    m_expectTerminalTrap = false;
                    break;
                }
                if (!HostOsInfo::isWindowsHost() && reason == u"signal-received"
                        && result["signal-name"].data() == u"SIGCONT") {
                    m_expectTerminalTrap = false;
                    runRunRequestCommand("-exec-continue");
                    break;
                }
            }

            if (reason == u"exited" || reason == u"exited-normally"
                    || reason == u"exited-signalled") {
                if (!m_onStopCommands.isEmpty()) {
                    const QList<DebuggerCommand> commands = m_onStopCommands;
                    m_onStopCommands.clear();
                    m_onStopWantContinue = false;
                    m_interruptRequested = false;
                    for (const DebuggerCommand &queuedCommand : commands) {
                        if (queuedCommand.callback) {
                            DebuggerResponse response;
                            response.resultClass = ResultFail;
                            queuedCommand.callback(response);
                        }
                    }
                }
                if (reason == u"exited")
                    emit inferiorDone({result["exit-code"].toInt(), InferiorExitStatus::Normal});
                else if (reason == u"exited-normally")
                    emit inferiorDone({0, InferiorExitStatus::Normal});
                else
                    emit inferiorDone({0, InferiorExitStatus::Crash});
                break;
            }

            if (m_attachPhase == AttachPhase::AwaitingConnect) {
                const auto *remoteData = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData);
                const bool isRemoteAttachPid = remoteData && remoteData->attachPid.isValid();
                const bool isTerminalStub
                    = std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData);
                m_attachPhase = AttachPhase::Stopped;
                if (isTerminalStub)
                    emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
                if (isRemoteAttachPid || isTerminalStub)
                    continueAfterAttach();
                else
                    emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
            } else if (m_attachPhase == AttachPhase::Continuing) {
            } else if (!m_onStopCommands.isEmpty()) {
                m_interruptRequested = false;
                const QList<DebuggerCommand> commands = m_onStopCommands;
                const bool wantContinue = m_onStopWantContinue;
                m_onStopCommands.clear();
                emit inferiorEvent(InferiorEvent::StopOk);
                for (const DebuggerCommand &queuedCommand : commands)
                    runCommandNow(queuedCommand);
                if (wantContinue)
                    runRunRequestCommand("-exec-continue");
                break;
            } else {
                const bool wasInterruptRequested = m_interruptRequested;
                m_interruptRequested = false;
                emit inferiorEvent(wasInterruptRequested ? InferiorEvent::StopOk
                                                         : InferiorEvent::SpontaneousStop);
                if (reason == u"signal-received")
                    emit signalReceived(result["signal-name"].data(), result["signal-meaning"].data());
            }

            const GdbMi frame = result["frame"];
            const int lineNumber = frame["line"].toInt();
            if (lineNumber != 0) {
                FilePath fileName = FilePath::fromUserInput(frame["fullname"].data());
                if (fileName.isEmpty())
                    fileName = FilePath::fromUserInput(frame["file"].data());
                if (fileName.exists())
                    emit locationChanged(fileName, lineNumber);
            }
        }
        break;
    }
    case '=': {
        const QStringView asyncClass = parser.readString([](char c) {
            return (c >= 'a' && c <= 'z') || c == '-';
        });

        GdbMi result;
        if (!parser.isAtEnd() && parser.isCurrent(',')) {
            parser.advance();
            result.parseTuple_helper(parser);
            result.m_type = GdbMi::Tuple;
        }

        if (asyncClass == u"library-loaded")
            emit libraryEvent(LibraryEvent::Loaded, result);
        else if (asyncClass == u"library-unloaded")
            emit libraryEvent(LibraryEvent::Unloaded, result);
        else if (asyncClass == u"thread-group-started") {
            m_inferiorPid = result["pid"].data().toLongLong();
            emit inferiorPidKnown(ProcessHandle(m_inferiorPid));
        }
        else if (asyncClass == u"breakpoint-created") {
            const GdbMi bkpt = result["bkpt"];
            if (!m_internalBreakpointNumbers.contains(bkpt["number"].data()))
                emit breakpointEvent(0, BreakpointOp::Insert, true, bkpt);
        }
        else if (asyncClass == u"breakpoint-deleted") {
            const QString number = result["id"].data();
            if (!m_internalBreakpointNumbers.contains(number)) {
                GdbMi deleted;
                deleted.m_type = GdbMi::Tuple;
                deleted.addChild(constMi("number", number));
                emit breakpointEvent(0, BreakpointOp::Remove, true, deleted);
            }
        }
        else if (asyncClass == u"breakpoint-modified") {
            const QString number = result["bkpt"]["number"].data();
            if (!m_internalBreakpointNumbers.contains(number)) {
                GdbMi list;
                list.m_type = GdbMi::List;
                list.addChild(result["bkpt"]);
                emit breakpointModified(list);
            }
        }
        break;
    }
    case '~': {
        const QString data = parser.readCString();
        emit message(data, LogOutput);
        if (data.startsWith("interpreteravailable={")) {
            m_interpreterHookStop = true;
            break;
        }
        if (data.startsWith("nativemixedstep={")) {
            GdbMi allData;
            allData.fromStringMultiple(data, m_outputDecoder);
            m_inNativeMixedStep = allData["nativemixedstep"]["state"].data() == "entered";
            break;
        }
        if (data.startsWith("tracepointhit={")) {
            GdbMi allData;
            allData.fromStringMultiple(data, m_outputDecoder);
            handleTracepointHit(allData["tracepointhit"]);
            break;
        }
        if (data.startsWith("tracepointmodified=")) {
            GdbMi allData;
            allData.fromStringMultiple(data, m_outputDecoder);
            emit breakpointModified(allData["tracepointmodified"]);
            break;
        }
        if (data.startsWith("interpreterasync={")) {
            GdbMi allData;
            allData.fromStringMultiple(data, m_outputDecoder);
            if (allData["asyncclass"].data() == "breakpointmodified") {
                GdbMi list;
                list.m_type = GdbMi::List;
                list.addChild(allData["interpreterasync"]);
                emit breakpointModified(list);
            }
            break;
        }
        if (data.startsWith("interpreterresult={")) {
            GdbMi allData;
            allData.fromStringMultiple(data, m_outputDecoder);
            DebuggerResponse response;
            response.resultClass = ResultDone;
            response.data = allData["interpreterresult"];
            response.token = allData["token"].toInt();
            handleResultRecord(&response);
            break;
        }
        m_pendingConsoleStreamOutput += data;
        break;
    }
    case '&': {
        const QString data = parser.readCString();
        emit message(data, LogOutput);
        m_pendingLogStreamOutput += data;
        break;
    }
    case '@':
        emit message(parser.readCString(), AppOutput);
        break;
    default:
        emit message(line, AppOutput);
        break;
    }
}

void GdbImpl::handleResultRecord(DebuggerResponse *response)
{
    const int token = response->token;
    if (token <= 0)
        return;

    if (!m_commandForToken.contains(token)) {
        emit message(QString("GdbImpl: no command found for token %1").arg(token), LogError);
        return;
    }
    const DebuggerCommand cmd = m_commandForToken.take(token);
    if (token < m_oldestAcceptableToken && (cmd.flags & DebuggerCommand::Discardable)) {
        emit message(QString("GdbImpl: skipping stale result for token %1").arg(token),
                     LogMiscInput);
        return;
    }
    if (cmd.callback)
        cmd.callback(*response);
}
} // namespace Debugger::Internal
