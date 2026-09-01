// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pdbimpl.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../procinterrupt.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QStringDecoder>

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

static GdbMi wrapped(const GdbMi &node)
{
    GdbMi wrapper;
    wrapper.m_type = GdbMi::Tuple;
    wrapper.addChild(node);
    return wrapper;
}

static QString breakpointLocation(const BreakpointParameters &params)
{
    QString loc;
    if (params.type == BreakpointByFunction)
        loc = params.functionName;
    else
        loc = params.fileName.path() + ':' + QString::number(params.textPosition.line);
    if (!params.condition.isEmpty())
        loc += ", " + params.condition;
    return loc;
}

static QString conditionCommand(const QString &pdbNumber, const BreakpointParameters &params)
{
    QString command = "condition " + pdbNumber;
    if (!params.condition.isEmpty())
        command += ' ' + params.condition;
    return command;
}

// pdb answers a file/line "break" with exactly that line, or refuses the location without a
// word, so the line is what identifies the insertion a reply belongs to. A function
// breakpoint resolves to a line we cannot predict, hence it matches anything.
static bool mayAnswer(const BreakpointParameters &params, const QString &file, int line)
{
    if (params.type == BreakpointByFunction)
        return true;
    return line == params.textPosition.line
        && params.fileName.fileName() == FilePath::fromString(file).fileName();
}

static DebuggerEngineSetupData pdbImplSetupData()
{
    DebuggerEngineSetupData data;
    data.capabilities = BreakConditionCapability
                      | JumpToLineCapability
                      | ReloadModuleCapability
                      | ReloadModuleSymbolsCapability
                      | ResetInferiorCapability
                      | RunToLineCapability
                      | ShowModuleSymbolsCapability;
    data.startModes = DebuggerStartModeFlag::Launch;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferior;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        return query.fileName.endsWith(".py");
    };
    return data;
}

PdbImpl::PdbImpl(const PdbImplStartData &startData)
    : DebuggerEngineInterface(pdbImplSetupData())
    , m_startData(startData)
{
    m_pdbProc.setProcessMode(ProcessMode::Writer);

    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        if (m_watchedCommands.isEmpty())
            return;
        QStringList pending;
        for (const auto &[token, description] : std::as_const(m_watchedCommands))
            pending << description;
        m_watchdog.start();
        emit notResponding(m_startData.watchdogTimeout, pending, NotRespondingCause::Unknown);
    });

    connect(&m_pdbProc, &Process::started, this, [this] {
        emit inferiorPidKnown(ProcessHandle(m_pdbProc.processId()));
        if (m_isResetRestart) {
            m_isResetRestart = false;
            const QList<ActiveBreakpoint> breakpoints = m_activeBreakpoints;
            for (const ActiveBreakpoint &bp : breakpoints)
                insertBreakpoint(bp.request, BreakpointReply::Reinsert);
            m_inferiorRunning = true;
            postDirectCommand("continue");
            return;
        }
        emit inferiorEvent(InferiorEvent::EngineSetupOk);
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        if (m_startData.breakOnMain)
            return; // pdb stops on the script's first line by itself.
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
        postDirectCommand("continue");
    });
    connect(&m_pdbProc, &Process::readyReadStandardOutput, this, [this] {
        handlePdbOutput(m_pdbProc.readAllStandardOutput());
    });
    connect(&m_pdbProc, &Process::readyReadStandardError, this, [this] {
        emit message(m_pdbProc.readAllStandardError(), LogError);
    });
    connect(&m_pdbProc, &Process::done, this, [this] {
        const bool startFailed = m_pdbProc.result() == ProcessResult::StartFailed;
        if (m_isResetRestart && !startFailed) {
            // Our own kill() behind a ResetInferior. Reporting an exit here would take the
            // engine down instead of restarting it.
            resetTransientState();
            QMetaObject::invokeMethod(this, [this] { startPdbProcess(); }, Qt::QueuedConnection);
            return;
        }
        m_isResetRestart = false;
        m_inferiorExited = true;
        if (startFailed) {
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        } else if (!m_shuttingDown) {
            emit inferiorDone({});
        }
        emit engineProcessFinished(m_pdbProc.resultData());
    });
}

PdbImpl::~PdbImpl()
{
    if (m_pdbProc.isRunning())
        m_pdbProc.kill();
}

void PdbImpl::start()
{
    const auto *inferiorRunData = std::get_if<ProcessRunData>(&m_startData.inferiorStartData);
    if (!inferiorRunData) {
        emit message("PdbImpl: only launching a script is supported", LogError);
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        return;
    }
    const FilePath script = inferiorRunData->command.executable();
    if (!script.isReadableFile()) {
        emit message("Cannot open script file " + script.toUserOutput(), LogError);
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        return;
    }
    startPdbProcess();
}

void PdbImpl::startPdbProcess()
{
    const auto &inferiorRunData = std::get<ProcessRunData>(m_startData.inferiorStartData);
    CommandLine cmd{m_startData.debuggerRunData.command.executable(),
                    {m_startData.dumperScriptsDir.pathAppended("pdbbridge.py").path(),
                     inferiorRunData.command.executable().path()}};
    cmd.addArg(inferiorRunData.workingDirectory.path());
    cmd.addArg("--");
    cmd.addArgs(inferiorRunData.command.arguments(), CommandLine::Raw);
    m_pdbProc.setCommand(cmd);

    m_pdbProc.setEnvironment(inferiorRunData.environment.appliedToEnvironment(
        m_startData.debuggerRunData.environment));
    if (inferiorRunData.workingDirectory.isDir())
        m_pdbProc.setWorkingDirectory(inferiorRunData.workingDirectory);
    m_pdbProc.start();
}

void PdbImpl::resetTransientState()
{
    m_inbuffer.clear();
    m_inferiorRunning = false;
    m_sawInitialLocation = false;
    m_interruptRequested = false;
    m_continueConfirmedRunning = false;
    m_interruptPending = false;
    m_inferiorExited = false;
    m_expectLocationOnly = false;
    m_currentFrame = 0;
    m_pendingBreakpointReplies.clear();
    m_pendingStackReplies.clear();
}

QString PdbImpl::pdbNumberFor(const QString &responseId) const
{
    for (const ActiveBreakpoint &bp : m_activeBreakpoints) {
        if (bp.request.responseId == responseId)
            return bp.pdbNumber;
    }
    return responseId;
}

QString PdbImpl::responseIdFor(const QString &pdbNumber) const
{
    for (const ActiveBreakpoint &bp : m_activeBreakpoints) {
        if (bp.pdbNumber == pdbNumber)
            return bp.request.responseId;
    }
    return pdbNumber;
}

void PdbImpl::insertBreakpoint(const BreakpointChangeRequest &request, BreakpointReply kind)
{
    PendingBreakpointReply pending;
    pending.kind = kind;
    pending.request = request;
    pending.fenceToken = ++m_lastFenceToken;
    m_pendingBreakpointReplies.append(pending);

    postDirectCommand((kind == BreakpointReply::Temporary ? QLatin1String("tbreak ")
                                                          : QLatin1String("break "))
                      + breakpointLocation(request.params));
    // pdb stays silent about a location it refuses, so bracket the insertion with a round
    // trip: once the fence comes back, a reply that has not arrived is never going to.
    DebuggerCommand fence("breakpointFence");
    fence.arg("token", QString::number(pending.fenceToken));
    runCommand(fence);
}

void PdbImpl::shutdownInferior(ShutdownMode)
{
    emit inferiorEvent(InferiorEvent::ShutdownFinished);
}

void PdbImpl::shutdownEngine()
{
    if (!m_pdbProc.isRunning()) {
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
        return;
    }
    m_isResetRestart = false;
    m_shuttingDown = true;
    connect(&m_pdbProc, &Process::done, this, [this] {
        emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
    }, Qt::SingleShotConnection);
    m_pdbProc.kill();
}

void PdbImpl::execute(const ExecutionRequest &request)
{
    if (m_inferiorExited && request.command != ExecutionCommand::Abort) {
        emit inferiorEvent(InferiorEvent::InferiorIll);
        return;
    }
    switch (request.command) {
    case ExecutionCommand::Continue:
        if (m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::RunRequested);
            emit inferiorEvent(InferiorEvent::RunFailed);
            break;
        }
        m_inferiorRunning = true;
        m_currentFrame = 0;
        m_continueConfirmedRunning = false;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        postDirectCommand("continue");
        break;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        if (!m_continueConfirmedRunning) {
            m_interruptPending = true;
            break;
        }
        requestInterrupt();
        break;
    case ExecutionCommand::StepIn:
        m_inferiorRunning = true;
        m_currentFrame = 0;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        postDirectCommand("step");
        break;
    case ExecutionCommand::StepOver:
        m_inferiorRunning = true;
        m_currentFrame = 0;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        postDirectCommand("next");
        break;
    case ExecutionCommand::StepOut:
        m_inferiorRunning = true;
        m_currentFrame = 0;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        postDirectCommand("return");
        break;
    case ExecutionCommand::Abort:
        m_pdbProc.kill();
        break;
    case ExecutionCommand::RunToLine: {
        m_inferiorRunning = true;
        m_currentFrame = 0;
        emit inferiorEvent(InferiorEvent::RunRequested);
        BreakpointChangeRequest temporary;
        temporary.params.type = BreakpointByFileAndLine;
        temporary.params.fileName = request.context.fileName;
        temporary.params.textPosition = request.context.textPosition;
        insertBreakpoint(temporary, BreakpointReply::Temporary);
        postDirectCommand("continue");
        break;
    }
    case ExecutionCommand::RunToFunction: {
        m_inferiorRunning = true;
        m_currentFrame = 0;
        emit inferiorEvent(InferiorEvent::RunRequested);
        BreakpointChangeRequest temporary;
        temporary.params.type = BreakpointByFunction;
        temporary.params.functionName = request.functionName;
        insertBreakpoint(temporary, BreakpointReply::Temporary);
        postDirectCommand("continue");
        break;
    }
    case ExecutionCommand::JumpToLine:
        postDirectCommand("jump " + QString::number(request.context.textPosition.line));
        m_pendingStackReplies.append({true, 0});
        runCommand({"stackListFrames"});
        break;
    case ExecutionCommand::ResetInferior:
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        // The done handler restarts us; killing synchronously here would report an exit.
        m_isResetRestart = true;
        m_pdbProc.kill();
        break;
    case ExecutionCommand::RepeatLastCommand:
        if (!m_lastDebuggableCommand.function.isEmpty())
            runCommand(m_lastDebuggableCommand);
        break;
    case ExecutionCommand::Detach:
    case ExecutionCommand::Return:
    case ExecutionCommand::RecordReverse:
        emit message("PdbImpl::execute() does not support this command", LogWarning);
        break;
    }
}

void PdbImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.op) {
    case BreakpointOp::Insert:
        insertBreakpoint(request, BreakpointReply::Insert);
        break;
    case BreakpointOp::Remove: {
        const QString pdbNumber = pdbNumberFor(request.responseId);
        for (int i = m_activeBreakpoints.size() - 1; i >= 0; --i) {
            if (m_activeBreakpoints.at(i).request.responseId == request.responseId)
                m_activeBreakpoints.removeAt(i);
        }
        postDirectCommand("clear " + pdbNumber);
        emit breakpointEvent(requestId, BreakpointOp::Remove, true);
        break;
    }
    case BreakpointOp::Update: {
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(requestId, BreakpointOp::Update, false);
            break;
        }
        const QString pdbNumber = pdbNumberFor(request.responseId);
        postDirectCommand(conditionCommand(pdbNumber, request.params));
        postDirectCommand((request.params.enabled ? QLatin1String("enable ")
                                                  : QLatin1String("disable "))
                          + pdbNumber);
        emit breakpointEvent(requestId, BreakpointOp::Update, true);
        break;
    }
    case BreakpointOp::EnableSub:
        postDirectCommand((request.enabled ? QLatin1String("enable ") : QLatin1String("disable "))
                          + request.subResponseId);
        emit breakpointEvent(requestId, BreakpointOp::EnableSub, true);
        break;
    }
}

void PdbImpl::refresh(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    switch (request.kind) {
    case RefreshKind::Locals: {
        m_pendingLocalsRequestId = requestId;
        DebuggerCommand cmd("updateData");
        cmd.arg("nativeMixed", false);
        cmd.arg("fancy", true);
        cmd.arg("frame", m_currentFrame);
        cmd.arg("watchers", request.watchers);
        m_lastDebuggableCommand = cmd;
        runCommand(cmd);
        return;
    }
    case RefreshKind::FullStack:
        m_pendingStackReplies.append({false, requestId});
        runCommand({"stackListFrames"});
        return;
    case RefreshKind::Modules:
        m_pendingModulesRequestId = requestId;
        runCommand({"listModules"});
        return;
    case RefreshKind::ModuleSymbols: {
        m_pendingModuleSymbolsRequestId = requestId;
        DebuggerCommand cmd("listSymbols");
        cmd.arg("module", request.path.path());
        runCommand(cmd);
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
    case RefreshKind::Threads: // asked on every stop, and pdb debugs one thread
        return;
    default:
        emit message("PdbImpl::refresh() does not support this kind yet", LogWarning);
        return;
    }
}

void PdbImpl::selectThread(const QString &)
{
}

void PdbImpl::activateFrame(int index)
{
    m_currentFrame = index;
}

void PdbImpl::setRegisterValue(const QString &, const QString &)
{
}

void PdbImpl::accessMemory(MemoryOp, quint64, quint64, quint64, const QByteArray &)
{
}

void PdbImpl::fetchDisassembly(quint64, quint64, const QString &)
{
}

void PdbImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    if (item.isLocal)
        postDirectCommand(expr + '=' + value);
    else
        postDirectCommand("global " + expr + ';' + expr + '=' + value);
}

void PdbImpl::setPeripheralRegisterValue(quint64, quint64)
{
}

void PdbImpl::watchPoint(quint64, const QPoint &)
{
}

void PdbImpl::createSnapshot(quint64)
{
}

void PdbImpl::executeDebuggerCommand(const QString &command, const WatchItemData &)
{
    postDirectCommand(command);
    watchCommand(command);
}

void PdbImpl::requestInterrupt()
{
    m_interruptRequested = true;
    QString error;
    if (!interruptProcess(m_pdbProc.processId(), &error)) {
        m_interruptRequested = false;
        emit message(error, LogError);
        emit inferiorEvent(InferiorEvent::StopFailed);
    }
}

void PdbImpl::postDirectCommand(const QString &command)
{
    QTC_ASSERT(m_pdbProc.isRunning(), return);
    emit message(command, LogInput);
    m_pdbProc.write(command + '\n');
}

void PdbImpl::watchCommand(const QString &description)
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    const quint64 token = ++m_lastWatchdogToken;
    m_watchedCommands.append({token, description});
    DebuggerCommand fence("watchdogFence");
    fence.arg("token", QString::number(token));
    const QString command = "qdebug('" + fence.function + "'," + fence.argsToPython() + ")";
    m_pdbProc.write(command + '\n');
    restartWatchdog();
}

void PdbImpl::handleWatchdogFence(quint64 token)
{
    // Commands are answered in order, so the fence clears everything up to it.
    const auto after = std::find_if(m_watchedCommands.cbegin(), m_watchedCommands.cend(),
                                    [token](const QPair<quint64, QString> &watched) {
        return watched.first > token;
    });
    m_watchedCommands.erase(m_watchedCommands.cbegin(), after);
    restartWatchdog();
}

void PdbImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_watchedCommands.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}

void PdbImpl::runCommand(const DebuggerCommand &cmd)
{
    QTC_ASSERT(m_pdbProc.isRunning(), return);
    const QString command = "qdebug('" + cmd.function + "'," + cmd.argsToPython() + ")";
    emit message(command, LogInput);
    m_pdbProc.write(command + '\n');
}

void PdbImpl::handlePdbOutput(const QString &output)
{
    m_inbuffer.append(output);
    while (true) {
        const int pos = m_inbuffer.indexOf('\n');
        if (pos == -1)
            break;
        const QString line = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 1);
        handleOutputLine(line);
    }
}

void PdbImpl::handleOutputLine(const QString &line)
{
    if (line.isEmpty())
        return;

    GdbMi item;
    QStringDecoder decoder(QStringEncoder::System);
    item.fromString(line, decoder);

    emit message(line, LogOutput);

    if (line.startsWith("stack={")) {
        handleStackReply(item);
    } else if (line.startsWith("data={")) {
        emit refreshDataReceived(m_pendingLocalsRequestId, RefreshKind::Locals, wrapped(item));
    } else if (line.startsWith("modules=[")) {
        emit refreshDataReceived(m_pendingModulesRequestId, RefreshKind::Modules, item);
    } else if (line.startsWith("symbols={")) {
        GdbMi moduleSymbols;
        moduleSymbols.m_type = GdbMi::Tuple;
        moduleSymbols.addChild(constMi("modulepath", item["module"].data()));
        GdbMi symbols = item["symbols"];
        symbols.m_name = "symbols";
        moduleSymbols.addChild(symbols);
        emit refreshDataReceived(m_pendingModuleSymbolsRequestId, RefreshKind::ModuleSymbols,
                                 moduleSymbols);
    } else if (line.startsWith("location={")) {
        if (!m_sawInitialLocation) {
            m_sawInitialLocation = true;
            return;
        }
        const FilePath file = FilePath::fromString(item["file"].data());
        const int lineNumber = item["line"].toInt();
        if (m_expectLocationOnly) {
            // An interrupt was already reported from the state report below; pdb only tells
            // us where it landed once it has single-stepped out of the signal handler.
            m_expectLocationOnly = false;
            emit locationChanged(file, lineNumber);
            return;
        }
        if (!m_inferiorRunning)
            return;
        m_inferiorRunning = false;
        emit locationChanged(file, lineNumber);
        emit inferiorEvent(InferiorEvent::SpontaneousStop);
    } else if (line.startsWith("state=")) {
        if (item.data() == "stopped") {
            m_inferiorRunning = false;
            m_expectLocationOnly = true;
            if (m_interruptRequested) {
                m_interruptRequested = false;
                emit inferiorEvent(InferiorEvent::StopOk);
            } else {
                emit inferiorEvent(InferiorEvent::SpontaneousStop);
            }
        } else if (item.data() == "running") {
            m_continueConfirmedRunning = true;
            if (m_interruptPending) {
                m_interruptPending = false;
                requestInterrupt();
            }
        }
    } else if (line.startsWith("Breakpoint")) {
        handleBreakpointReply(line);
    } else if (line.startsWith("breakpointfence={")) {
        handleBreakpointFence(item["token"].data().toULongLong());
    } else if (line.startsWith("watchdogfence={")) {
        handleWatchdogFence(item["token"].data().toULongLong());
    } else if (line.startsWith("breakpointmodified=")) {
        const QString responseId = responseIdFor(item["number"].data());
        const auto it = std::find_if(m_activeBreakpoints.cbegin(), m_activeBreakpoints.cend(),
                                     [&responseId](const ActiveBreakpoint &bp) {
            return bp.request.responseId == responseId;
        });
        if (it == m_activeBreakpoints.cend())
            return;

        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi("number", responseId));
        bkpt.addChild(constMi("file", it->request.params.fileName.path()));
        bkpt.addChild(constMi("fullname", it->request.params.fileName.path()));
        bkpt.addChild(constMi("line", QString::number(it->request.params.textPosition.line)));
        bkpt.addChild(constMi("enabled", "y"));
        bkpt.addChild(constMi("times", item["times"].data()));
        GdbMi list;
        list.m_type = GdbMi::List;
        list.addChild(bkpt);
        emit breakpointModified(list);
    } else {
        emit message(line, AppOutput);
    }
}

void PdbImpl::handleStackReply(const GdbMi &item)
{
    const PendingStackReply pending = m_pendingStackReplies.isEmpty()
                                          ? PendingStackReply{}
                                          : m_pendingStackReplies.takeFirst();
    if (pending.forJumpToLine) {
        const GdbMi frames = item["frames"];
        if (frames.childCount() > 0) {
            const GdbMi &topFrame = frames.childAt(0);
            emit locationChanged(FilePath::fromString(topFrame["file"].data()),
                                 topFrame["line"].toInt());
        }
        emit inferiorEvent(InferiorEvent::SpontaneousStop);
        return;
    }
    // The bridge reports the payload as the whole line, so what the line
    // parses into is the "stack" node itself. The interface hands out a
    // tuple that *contains* it, as data["stack"]["frames"] is what the
    // engine reads.
    emit refreshDataReceived(pending.requestId, RefreshKind::FullStack, wrapped(item));
}

void PdbImpl::handleBreakpointReply(const QString &line)
{
    const int pos1 = line.indexOf(" at ");
    QTC_ASSERT(pos1 != -1, return);
    const int pos2 = line.lastIndexOf(':');
    QTC_ASSERT(pos2 != -1, return);
    const QString fileName = line.mid(pos1 + 4, pos2 - pos1 - 4);
    const QString lineNumber = line.mid(pos2 + 1);
    const QString bpnr = line.mid(11, pos1 - 11);

    // pdb answers in command order, but a location it refuses is not answered at all, so
    // the first pending insertion is not necessarily the one this line belongs to.
    int index = 0;
    while (index < m_pendingBreakpointReplies.size()
           && !mayAnswer(m_pendingBreakpointReplies.at(index).request.params, fileName,
                         lineNumber.toInt())) {
        ++index;
    }
    QTC_ASSERT(index < m_pendingBreakpointReplies.size(), return);
    const PendingBreakpointReply pending = m_pendingBreakpointReplies.takeAt(index);

    if (pending.kind == BreakpointReply::Temporary)
        return;

    if (pending.kind == BreakpointReply::Reinsert) {
        for (ActiveBreakpoint &bp : m_activeBreakpoints) {
            if (bp.request.responseId == pending.request.responseId)
                bp.pdbNumber = bpnr;
        }
        return;
    }

    ActiveBreakpoint active;
    active.request = pending.request;
    active.request.responseId = bpnr;
    active.pdbNumber = bpnr;
    m_activeBreakpoints.append(active);

    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", bpnr));
    bkpt.addChild(constMi("file", fileName));
    bkpt.addChild(constMi("fullname", fileName));
    bkpt.addChild(constMi("line", lineNumber));
    bkpt.addChild(constMi("enabled", "y"));
    GdbMi data;
    data.m_type = GdbMi::List;
    data.addChild(bkpt);
    emit breakpointEvent(pending.request.requestId, BreakpointOp::Insert, true, data);
}

void PdbImpl::handleBreakpointFence(quint64 token)
{
    for (int i = m_pendingBreakpointReplies.size() - 1; i >= 0; --i) {
        const PendingBreakpointReply pending = m_pendingBreakpointReplies.at(i);
        if (pending.fenceToken > token)
            continue;
        m_pendingBreakpointReplies.removeAt(i);
        if (pending.kind == BreakpointReply::Insert) {
            emit breakpointEvent(pending.request.requestId, BreakpointOp::Insert, false);
        } else {
            emit message("pdb refused the breakpoint location "
                             + breakpointLocation(pending.request.params),
                         LogWarning);
        }
    }
}
} // namespace Debugger::Internal
