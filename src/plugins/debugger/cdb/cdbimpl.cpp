// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cdbimpl.h"

#include "cdbparsehelpers.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using namespace Utils;

namespace Debugger::Internal {

// Above DebuggerCommand's own flags, as CdbEngine::CommandFlags is.
enum CommandFlags {
    NoFlags = 0,
    BuiltinCommand = DebuggerCommand::Silent << 1,
    ExtensionCommand = DebuggerCommand::Silent << 2,
    ScriptCommand = DebuggerCommand::Silent << 3
};

static GdbMi constMi(const QString &name, const QString &data)
{
    GdbMi mi;
    mi.m_type = GdbMi::Const;
    mi.m_name = name;
    mi.m_data = data;
    return mi;
}

enum { CdbPromptLength = 7 };

static QString hexAddress(quint64 address)
{
    return "0x" + QString::number(address, 16);
}

static bool isCdbPrompt(const QString &line)
{
    return line.size() >= CdbPromptLength && line.at(6) == ' ' && line.at(5) == '>'
           && line.at(1) == ':' && line.at(0).isDigit() && line.at(2).isDigit()
           && line.at(3).isDigit() && line.at(4).isDigit();
}

static bool checkCommandToken(const QString &tokenPrefix, const QString &line,
                              int *token, bool *isStart)
{
    *token = 0;
    *isStart = false;
    const int prefixSize = tokenPrefix.size();
    if (line.size() < prefixSize + 2 || !line.at(prefixSize).isDigit())
        return false;
    if (line.back() == '>')
        *isStart = false;
    else if (line.back() == '<')
        *isStart = true;
    else
        return false;
    if (!line.startsWith(tokenPrefix))
        return false;
    bool ok = false;
    *token = line.mid(prefixSize, line.size() - prefixSize - 1).toInt(&ok);
    return ok;
}

static DebuggerEngineSetupData cdbImplSetupData()
{
    DebuggerEngineSetupData data;
    data.capabilities = AdditionalQmlStackCapability
                      | AddWatcherCapability
                      | BreakConditionCapability
                      | BreakIndividualLocationsCapability
                      | BreakModuleCapability
                      | BreakOnThrowAndCatchCapability
                      | CreateFullBacktraceCapability
                      | DisassemblerCapability
                      | JumpToLineCapability
                      | OperateByInstructionCapability
                      | RegisterCapability
                      | ReloadModuleCapability
                      | ResetInferiorCapability
                      | RunToLineCapability
                      | ShowMemoryCapability
                      | TracePointCapability
                      | WatchpointByAddressCapability;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        if (!query.isCppBreakpoint())
            return query.isNativeMixedEnabled;
        switch (query.type) {
        case BreakpointAtFork:
        case BreakpointAtSysCall:
        case WatchpointAtExpression:
        case UnknownBreakpointType:
        case LastBreakpointType:
            return false;
        default:
            break;
        }
        return true;
    };
    data.startModes = DebuggerStartModeFlag::Launch;
    return data;
}

CdbImpl::CdbImpl(const CdbImplStartData &startData)
    : DebuggerEngineInterface(cdbImplSetupData())
    , m_startData(startData)
{
    m_cdbProc.setProcessMode(ProcessMode::Writer);
    m_cdbProc.setUseCtrlCStub(m_startData.useCtrlCStub);

    CommandLine cdbCommand = m_startData.debuggerRunData.command;
    cdbCommand.addArg("-a" + m_startData.extensionFileName);
    cdbCommand.addArgs({"-lines", "-G", "-c", ".idle_cmd " + m_extensionCommandPrefix + "idle"});
    // cdb launches the inferior itself, so the inferior's environment and working directory
    // have to be the ones cdb is started with.
    ProcessRunData runData = m_startData.debuggerRunData;
    if (std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
        const auto &inferiorRunData = std::get<ProcessRunData>(m_startData.inferiorStartData);
        cdbCommand.addArg(inferiorRunData.command.executable().toUserOutput());
        cdbCommand.addArgs(inferiorRunData.command.arguments(), CommandLine::Raw);
        runData = inferiorRunData;
    }
    m_cdbProc.setCommand(cdbCommand);
    if (runData.workingDirectory.isDir())
        m_cdbProc.setWorkingDirectory(runData.workingDirectory);
    Environment env = runData.environment.hasChanges() ? runData.environment
                                                       : Environment::systemEnvironment();
    env.set("_NT_DEBUGGER_EXTENSION_PATH", m_startData.extensionDir.nativePath());
    m_cdbProc.setEnvironment(env);

    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        QStringList pending;
        for (const DebuggerCommand &cmd : std::as_const(m_commandForToken))
            pending << cmd.function;
        for (const DebuggerCommand &cmd : std::as_const(m_deferredCommands))
            pending << cmd.function;
        if (pending.isEmpty())
            return;
        m_watchdog.start();
        emit notResponding(m_startData.watchdogTimeout, pending);
    });

    m_cdbProc.setStdOutLineCallback([this](const QString &line) {
        restartWatchdog();
        handleCdbOutputLine(line);
    });
    m_cdbProc.setStdErrLineCallback([this](const QString &line) {
        emit message(line.endsWith('\n') ? line.chopped(1) : line, LogError);
    });
    connect(&m_cdbProc, &Process::done, this, [this] {
        m_watchdog.stop();
        if (m_cdbProc.result() == ProcessResult::StartFailed) {
            m_isResetRestart = false;
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
            emit engineProcessFinished(m_cdbProc.resultData());
            return;
        }
        if (m_isResetRestart) {
            // Process::start() must not be called from one of its own signal handlers.
            QMetaObject::invokeMethod(this, [this] { restartSession(); }, Qt::QueuedConnection);
            return;
        }
        emit engineProcessFinished(m_cdbProc.resultData());
    });
}

CdbImpl::~CdbImpl()
{
    m_shuttingDown = true;
    if (m_cdbProc.isRunning())
        m_cdbProc.kill();
}

void CdbImpl::start()
{
    m_cdbProc.start();
}

void CdbImpl::shutdownInferior(ShutdownMode mode)
{
    Q_UNUSED(mode)
    if (m_cdbProc.isRunning())
        m_cdbProc.write("q\n");
    emit inferiorEvent(InferiorEvent::ShutdownFinished);
}

void CdbImpl::shutdownEngine()
{
    m_shuttingDown = true;
    if (m_cdbProc.isRunning())
        m_cdbProc.kill();
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void CdbImpl::execute(const ExecutionRequest &request)
{
    if (m_inferiorExited && request.command != ExecutionCommand::Abort) {
        emit inferiorEvent(InferiorEvent::InferiorIll);
        return;
    }
    m_inInternalStop = false;
    switch (request.command) {
    case ExecutionCommand::Continue:
        if (m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::RunRequested);
            emit inferiorEvent(InferiorEvent::RunFailed);
            break;
        }
        m_expectSpontaneousStop = true;
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        runCommand({"g", NoFlags});
        break;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        m_interruptRequested = true;
        m_cdbProc.interrupt();
        break;
    case ExecutionCommand::StepIn:
        m_expectSpontaneousStop = true;
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        adjustOperateByInstruction(request.flag);
        runCommand({"t", NoFlags});
        break;
    case ExecutionCommand::StepOver:
        m_expectSpontaneousStop = true;
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        adjustOperateByInstruction(request.flag);
        runCommand({"p", NoFlags});
        break;
    case ExecutionCommand::StepOut:
        m_expectSpontaneousStop = true;
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        runCommand({"gu", NoFlags});
        break;
    case ExecutionCommand::Abort:
        shutdownEngine();
        break;
    case ExecutionCommand::RunToLine:
    case ExecutionCommand::RunToFunction: {
        const QString id = nextBreakpointId();
        m_internalBreakpointIds.insert(id);
        QString cmd = "bu" + id + " /1 ";
        if (request.command == ExecutionCommand::RunToFunction) {
            cmd += request.functionName;
        } else if (request.context.address) {
            cmd += "0x" + QString::number(request.context.address, 16);
        } else {
            cmd += '`' + request.context.fileName.toUserOutput() + ':'
                 + QString::number(request.context.textPosition.line) + '`';
        }
        runCommand({cmd, NoFlags});
        m_expectSpontaneousStop = true;
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        runCommand({"g", NoFlags});
        break;
    }
    case ExecutionCommand::JumpToLine: {
        const FilePath file = request.context.fileName;
        const int line = request.context.textPosition.line;
        if (request.context.address) {
            jumpToAddress(request.context.address, file, line);
            break;
        }
        const QString expr = "? `" + file.toUserOutput() + ':' + QString::number(line) + '`';
        runCommand({expr, BuiltinCommand,
                   [this, file, line](const DebuggerResponse &response) {
            const QString reply = response.data.data();
            const int eq = reply.lastIndexOf('=');
            if (eq == -1) {
                emit message("CdbImpl: could not resolve a jump target from: " + reply,
                             LogError);
                emit inferiorEvent(InferiorEvent::InferiorIll);
                return;
            }
            const QString hex = reply.mid(eq + 1).remove('`').trimmed();
            bool ok = false;
            const quint64 address = hex.toULongLong(&ok, 16);
            if (!ok) {
                emit message("CdbImpl: unparsable jump target: " + hex, LogError);
                emit inferiorEvent(InferiorEvent::InferiorIll);
                return;
            }
            jumpToAddress(address, file, line);
        }});
        break;
    }
    case ExecutionCommand::ResetInferior:
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        m_shuttingDown = true;
        m_isResetRestart = true;
        m_cdbProc.kill(); // restartSession() takes over from the done handler.
        break;
    case ExecutionCommand::RepeatLastCommand:
        if (!m_lastDebuggableCommand.function.isEmpty())
            runCommand(m_lastDebuggableCommand);
        break;
    case ExecutionCommand::Return:
    case ExecutionCommand::Detach:
    case ExecutionCommand::RecordReverse:
        emit message("CdbImpl: execution command not implemented.", LogWarning);
        break;
    }
}

void CdbImpl::insertBreakpoint(quint64 requestId, const QString &id, int modelId,
                               const BreakpointParameters &params, bool report)
{
    if (!params.isCppBreakpoint()) {
        if (!m_startData.nativeMixed) {
            if (report)
                emit breakpointEvent(requestId, BreakpointOp::Insert, false, {});
            return;
        }
        if (m_pythonVersion == 0) {
            m_pendingBridgeWork.append([this, requestId, modelId, params, report] {
                if (m_pythonVersion != 0)
                    insertInterpreterBreakpoint(requestId, modelId, params, report);
                else if (report)
                    emit breakpointEvent(requestId, BreakpointOp::Insert, false, {});
            });
            return;
        }
        insertInterpreterBreakpoint(requestId, modelId, params, report);
        return;
    }
    const QString module = params.module;
    if (!params.condition.isEmpty())
        m_conditionForBreakpointId.insert(id, params.condition);
    m_insertedBreakpoints.insert(id, params);
    BreakpointType type = params.type;
    QString functionName = params.functionName;
    if (type == BreakpointAtThrow) {
        type = BreakpointByFunction;
        functionName = "CxxThrowException";
    } else if (type == BreakpointAtCatch) {
        type = BreakpointByFunction;
        functionName = "__CxxCallCatchBlock";
    }
    QString cmd = QLatin1String(type == WatchpointAtAddress ? "ba" : "bu") + id + ' ';
    if (params.oneShot)
        cmd += "/1 ";
    switch (type) {
    case BreakpointByFunction:
        if (!params.oneShot) {
            insertFunctionBreakpoint(requestId, id, params.enabled,
                                     module, functionName, report);
            return;
        }
        if (!module.isEmpty())
            cmd += module + '!';
        cmd += functionName;
        break;
    case BreakpointByFileAndLine:
        cmd += '`';
        if (!module.isEmpty())
            cmd += module + '!';
        cmd += params.fileName.toUserOutput() + ':'
             + QString::number(params.textPosition.line) + '`';
        break;
    case WatchpointAtAddress: {
        const unsigned size = params.size ? params.size : 1;
        cmd += 'r' + QString::number(size) + ' ' + "0x"
             + QString::number(params.address, 16);
        break;
    }
    default:
        if (report)
            emit breakpointEvent(requestId, BreakpointOp::Insert, false, {});
        return;
    }
    const bool enabled = params.enabled;
    const QString file = params.fileName.path();
    const int line = params.textPosition.line;
    const QString function = functionName;
    runCommand({cmd, BuiltinCommand,
               [this, requestId, id, enabled, file, line, function, report]
               (const DebuggerResponse &r) {
        const QStringList reply = r.data.data().split('\n');
        bool ambiguous = false;
        for (int i = qMax(0, reply.size() - 2); i < reply.size(); ++i)
            ambiguous |= reply.at(i).startsWith("Ambiguous symbol error");
        GdbMi locations;
        locations.m_type = GdbMi::List;
        locations.m_name = "locations";
        if (ambiguous) {
            const QLatin1String matchPrefix("Matched: ");
            int subId = 0;
            for (const QString &replyLine : reply) {
                if (!replyLine.startsWith(matchPrefix))
                    continue;
                const int addressStart = replyLine.lastIndexOf('(') + 1;
                const int addressEnd = replyLine.indexOf(')', addressStart);
                if (addressStart == 0 || addressEnd == -1)
                    continue;
                QString addressString = replyLine.mid(addressStart,
                                                      addressEnd - addressStart);
                addressString.remove('`');
                bool ok = false;
                const quint64 address = addressString.toULongLong(&ok, 16);
                if (!ok)
                    continue;
                QString matchedFunction = replyLine.mid(matchPrefix.size(),
                                              addressStart - 1 - matchPrefix.size());
                const int functionStart = matchedFunction.indexOf('!') + 1;
                const int functionOffset = matchedFunction.lastIndexOf('+');
                if (functionOffset > 0)
                    matchedFunction.truncate(functionOffset);
                if (functionStart > 0)
                    matchedFunction = matchedFunction.mid(functionStart);
                const QString target = hexAddress(address);
                const QString subResponseId = QString::number(id.toInt() + ++subId);
                m_parentForSubBreakpointId.insert(subResponseId, id);
                runCommand({"bu" + subResponseId + ' ' + target, NoFlags});
                GdbMi location;
                location.m_type = GdbMi::Tuple;
                location.addChild(constMi("number", subResponseId));
                location.addChild(constMi("func", matchedFunction));
                location.addChild(constMi("addr", target));
                location.addChild(constMi("enabled", QLatin1String(enabled ? "y" : "n")));
                if (!file.isEmpty()) {
                    location.addChild(constMi("file", file));
                    location.addChild(constMi("line", QString::number(line)));
                }
                locations.addChild(location);
            }
        }
        reportBreakpointInserted(requestId, id, enabled, file, line, function,
                                 locations, report);
    }});
}

void CdbImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    switch (request.op) {
    case BreakpointOp::Insert:
        insertBreakpoint(request.requestId, nextBreakpointId(), request.modelId,
                         request.params, true);
        break;
    case BreakpointOp::Remove:
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, request.op, false, {});
            break;
        }
        runCommand({"bc" + request.responseId, NoFlags});
        m_insertedBreakpoints.remove(request.responseId);
        m_conditionForBreakpointId.remove(request.responseId);
        m_breakpointHitCounts.remove(request.responseId);
        for (const QString &subId : m_parentForSubBreakpointId.keys(request.responseId))
            m_parentForSubBreakpointId.remove(subId);
        emit breakpointEvent(request.requestId, request.op, true, {});
        break;
    case BreakpointOp::Update:
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, request.op, false, {});
            break;
        }
        if (request.params.condition.isEmpty())
            m_conditionForBreakpointId.remove(request.responseId);
        else
            m_conditionForBreakpointId.insert(request.responseId, request.params.condition);
        runCommand({QLatin1String(request.params.enabled ? "be" : "bd") + request.responseId,
                    NoFlags});
        emit breakpointEvent(request.requestId, request.op, true, {});
        break;
    case BreakpointOp::EnableSub:
        if (request.subResponseId.isEmpty()) {
            emit breakpointEvent(request.requestId, request.op, false, {});
            break;
        }
        runCommand({QLatin1String(request.enabled ? "be" : "bd") + request.subResponseId,
                    NoFlags});
        emit breakpointEvent(request.requestId, request.op, true, {});
        break;
    }
}

void CdbImpl::parseFunctionDisassembly(const QString &reply, ResolvedFunction *function)
{
    int declarationLine = 0;
    bool headerSeen = false;
    bool bodySeen = false;
    for (const QString &replyLine : reply.split('\n')) {
        if (!headerSeen) {
            const QString trimmed = replyLine.trimmed();
            const int bracket = trimmed.indexOf(" [");
            const int at = trimmed.lastIndexOf(" @ ");
            if (bracket <= 0 || at <= bracket || !trimmed.endsWith("]:"))
                continue;
            function->file = trimmed.mid(bracket + 2, at - bracket - 2);
            declarationLine = trimmed.mid(at + 3, trimmed.size() - at - 5).toInt();
            headerSeen = true;
            continue;
        }
        const QStringList parts = replyLine.trimmed().split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;
        int addressIndex = -1;
        for (int i = 1; i < parts.size(); ++i) {
            if (parts.at(i).contains('`')) {
                addressIndex = i;
                break;
            }
        }
        if (addressIndex < 1)
            continue;
        QString addressString = parts.at(addressIndex);
        addressString.remove('`');
        bool addressOk = false;
        const quint64 address = addressString.toULongLong(&addressOk, 16);
        if (!addressOk)
            continue;
        bool lineOk = false;
        const int sourceLine = parts.at(addressIndex - 1).toInt(&lineOk);
        if (!lineOk)
            continue;
        if (!bodySeen) {
            bodySeen = true;
            function->address = address;
            function->line = sourceLine;
        }
        if (sourceLine != declarationLine) {
            function->address = address;
            function->line = sourceLine;
            return;
        }
    }
}

void CdbImpl::insertFunctionBreakpoint(quint64 requestId, const QString &id, bool enabled,
                                       const QString &module, const QString &functionName,
                                       bool report)
{
    const QString scope = (module.isEmpty() ? QString("*") : module) + '!';
    const QString fallbackTarget = module.isEmpty() ? functionName : module + '!' + functionName;
    runCommand({"x " + scope + functionName + '*', BuiltinCommand,
               [this, requestId, id, enabled, functionName, fallbackTarget, report]
               (const DebuggerResponse &response) {
        QList<ResolvedFunction> candidates;
        for (const QString &replyLine : response.data.data().split('\n')) {
            const QString trimmed = replyLine.trimmed();
            const int space = trimmed.indexOf(' ');
            if (space <= 0)
                continue;
            QString symbol = trimmed.mid(space).trimmed();
            const int signature = symbol.lastIndexOf(" (");
            if (signature > 0)
                symbol.truncate(signature);
            const QString name = symbol.mid(symbol.indexOf('!') + 1);
            if (name != functionName && !name.startsWith(functionName + '<'))
                continue;
            QString addressString = trimmed.left(space);
            addressString.remove('`');
            bool addressOk = false;
            const quint64 address = addressString.toULongLong(&addressOk, 16);
            if (!addressOk) {
                emit message("CdbImpl: no address for " + symbol + " in: " + trimmed, LogWarning);
                continue;
            }
            ResolvedFunction candidate;
            candidate.qualifiedName = symbol;
            candidate.name = name;
            candidate.address = address;
            candidates.append(candidate);
        }
        if (candidates.isEmpty()) {
            const QString file;
            runCommand({"bu" + id + ' ' + fallbackTarget, BuiltinCommand,
                       [this, requestId, id, enabled, functionName, file, report]
                       (const DebuggerResponse &) {
                reportBreakpointInserted(requestId, id, enabled, file, 0, functionName, {}, report);
            }});
            return;
        }
        struct Resolution
        {
            int pending = 0;
            QList<ResolvedFunction> functions;
        };
        const auto resolution = std::make_shared<Resolution>();
        resolution->pending = candidates.size();
        resolution->functions = candidates;
        for (int i = 0; i < candidates.size(); ++i) {
            // By address, not by name: cdb cannot parse the "<int>" in a template
            // instantiation's name, and the entry address already stands in should
            // the disassembly not yield a body line to skip the prologue.
            const QString target = hexAddress(candidates.at(i).address);
            runCommand({"uf " + target, BuiltinCommand,
                       [this, requestId, id, enabled, functionName, resolution, i, report]
                       (const DebuggerResponse &ufResponse) {
                ResolvedFunction &function = resolution->functions[i];
                parseFunctionDisassembly(ufResponse.data.data(), &function);
                if (--resolution->pending > 0)
                    return;
                setResolvedFunctionBreakpoints(requestId, id, enabled, functionName,
                                               resolution->functions, report);
            }});
        }
    }});
}

void CdbImpl::setResolvedFunctionBreakpoints(quint64 requestId, const QString &id, bool enabled,
                                             const QString &functionName,
                                             const QList<ResolvedFunction> &functions, bool report)
{
    QList<ResolvedFunction> resolved;
    for (const ResolvedFunction &function : functions) {
        if (function.address != 0)
            resolved.append(function);
    }
    if (resolved.isEmpty()) {
        emit breakpointEvent(requestId, BreakpointOp::Insert, false, {});
        return;
    }
    if (resolved.size() == 1) {
        const ResolvedFunction &function = resolved.constFirst();
        runCommand({"bu" + id + ' ' + hexAddress(function.address), NoFlags});
        reportBreakpointInserted(requestId, id, enabled, function.file, function.line,
                                 function.name, {}, report);
        return;
    }
    GdbMi locations;
    locations.m_type = GdbMi::List;
    locations.m_name = "locations";
    int subId = 0;
    for (const ResolvedFunction &function : resolved) {
        const QString subResponseId = QString::number(id.toInt() + ++subId);
        m_parentForSubBreakpointId.insert(subResponseId, id);
        runCommand({"bu" + subResponseId + ' ' + hexAddress(function.address), NoFlags});
        GdbMi location;
        location.m_type = GdbMi::Tuple;
        location.addChild(constMi("number", subResponseId));
        location.addChild(constMi("func", function.name));
        location.addChild(constMi("addr", hexAddress(function.address)));
        location.addChild(constMi("enabled", QLatin1String(enabled ? "y" : "n")));
        location.addChild(constMi("file", function.file));
        location.addChild(constMi("line", QString::number(function.line)));
        locations.addChild(location);
    }
    const ResolvedFunction &first = resolved.constFirst();
    reportBreakpointInserted(requestId, id, enabled, first.file, first.line, functionName,
                             locations, report);
}

void CdbImpl::reportBreakpointInserted(quint64 requestId, const QString &id, bool enabled,
                                       const QString &file, int line, const QString &function,
                                       const GdbMi &locations, bool report)
{
    if (!report)
        return;
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", id));
    bkpt.addChild(constMi("enabled", QLatin1String(enabled ? "y" : "n")));
    if (!file.isEmpty()) {
        bkpt.addChild(constMi("file", file));
        bkpt.addChild(constMi("line", QString::number(line)));
    }
    if (!function.isEmpty())
        bkpt.addChild(constMi("func", function));
    if (locations.childCount() > 0)
        bkpt.addChild(locations);
    GdbMi list;
    list.m_type = GdbMi::List;
    list.addChild(bkpt);
    emit breakpointEvent(requestId, BreakpointOp::Insert, true, list);
}

QString CdbImpl::nextBreakpointId()
{
    return QString::number(cdbBreakPointStartId
                           + (m_nextBreakpointId++) * cdbBreakPointIdMinorPart);
}

static bool isSameLocation(const BreakpointParameters &one, const BreakpointParameters &other)
{
    return one.type == other.type && one.fileName == other.fileName
        && one.textPosition.line == other.textPosition.line
        && one.functionName == other.functionName && one.address == other.address;
}

void CdbImpl::reportTracepoint(const QStringList &tracepointMessages, const GdbMi &stopData,
                               bool stopAfterwards)
{
    const auto finish = [this, stopData, stopAfterwards] {
        if (stopAfterwards) {
            reportStop(stopData);
            return;
        }
        m_expectSpontaneousStop = true;
        m_inferiorRunning = true;
        runCommand({"g", NoFlags});
    };
    static const QRegularExpression captureExpression(R"(\{([^{}]+)\})");
    QStringList captures;
    for (const QString &tracepointMessage : tracepointMessages) {
        QRegularExpressionMatchIterator it = captureExpression.globalMatch(tracepointMessage);
        while (it.hasNext()) {
            const QString capture = it.next().captured(1);
            if (!captures.contains(capture))
                captures.append(capture);
        }
    }
    if (captures.isEmpty()) {
        for (const QString &tracepointMessage : tracepointMessages) {
            if (tracepointMessage.isEmpty())
                continue;
            emit message(tracepointMessage, AppOutput);
            emit message(tracepointMessage, LogMisc);
        }
        finish();
        return;
    }

    QString args = "-v -D -W";
    for (int i = 0; i < captures.size(); ++i)
        args += QString(" -w watch.%1 \"%2\"").arg(i).arg(captures.at(i));
    args += " 0";
    DebuggerCommand cmd("locals", ExtensionCommand);
    cmd.args = args;
    m_expandingTracepoint = true;
    cmd.callback = [this, tracepointMessages, captures, finish](const DebuggerResponse &response) {
        m_expandingTracepoint = false;
        for (const QString &tracepointMessage : tracepointMessages) {
            QString expanded = tracepointMessage;
            for (int i = 0; i < captures.size(); ++i) {
                QString value;
                for (const GdbMi &item : response.data) {
                    if (item["iname"].data() != QString("watch.%1").arg(i))
                        continue;
                    value = decodeData(item["value"].data(), item["valueencoded"].data());
                    break;
                }
                if (!value.isEmpty())
                    expanded.replace('{' + captures.at(i) + '}', value);
            }
            emit message(expanded, AppOutput);
            emit message(expanded, LogMisc);
        }
        finish();
    };
    runCommand(cmd);
}

void CdbImpl::insertInterpreterBreakpoint(quint64 requestId, int modelId,
                                          const BreakpointParameters &params, bool report)
{
    DebuggerCommand cmd("theDumper.insertInterpreterBreakpoint", ScriptCommand);
    cmd.arg("modelid", modelId);
    cmd.arg("file", params.fileName.path());
    cmd.arg("line", params.textPosition.line);
    cmd.arg("enabled", params.enabled);
    cmd.arg("condition", toHex(params.condition));
    cmd.arg("ignorecount", params.ignoreCount);
    const InterpreterBreakpoint pending{modelId, params};
    cmd.callback = [this, requestId, report, pending](const DebuggerResponse &response) {
        if (response.resultClass != ResultDone) {
            if (report)
                emit breakpointEvent(requestId, BreakpointOp::Insert, false, {});
            return;
        }
        GdbMi result;
        for (const GdbMi &line : response.data["msg"]) {
            const QString text = line.data();
            if (!text.startsWith("interpreterresult="))
                continue;
            QStringDecoder decoder(QStringEncoder::Utf8);
            result.fromString(text.mid(int(strlen("interpreterresult="))), decoder);
            break;
        }
        if (!report)
            return;
        GdbMi list;
        list.m_type = GdbMi::List;
        if (result.isValid()) {
            GdbMi bkpt = result;
            bkpt.m_name = "bkpt";
            list.addChild(bkpt);
        }
        if (result["pending"].data() == "1")
            m_pendingInterpreterBreakpoints.append(pending);
        emit breakpointEvent(requestId, BreakpointOp::Insert, true, list);
    };
    runCommand(cmd);
}

// The frame the current thread stopped in, however the extension reported it.
static QString stoppedFunction(const GdbMi &stopData)
{
    const GdbMi stack = stopData["stack"];
    if (stack.childCount() > 0)
        return stack.childAt(0)["function"].data();
    const QString threadId = stopData["threadId"].data();
    for (const GdbMi &thread : stopData["threads"]["threads"]) {
        if (thread["id"].data() == threadId)
            return thread["frame"]["function"].data();
    }
    return {};
}

// The bridge decodes the pending QML debug event and says whether it is a real
// stop; anything else is machinery to continue past. Mirrors CdbEngine.
void CdbImpl::handleInterpreterMessage(const GdbMi &stopData)
{
    runCommand({"theDumper.reportInterpreterMessage()", ScriptCommand,
               [this, stopData](const DebuggerResponse &response) {
        static const QLatin1String prefix("interpretermessage=");
        bool isBreak = false;
        for (const GdbMi &line : response.data["msg"]) {
            const QString text = line.data();
            if (!text.startsWith(prefix))
                continue;
            GdbMi decoded;
            QStringDecoder decoder(QStringEncoder::Utf8);
            decoded.fromString(text.mid(prefix.size()), decoder);
            isBreak = decoded["event"].data() == "break";
            break;
        }
        if (isBreak) {
            reportStop(stopData);
            return;
        }
        resumeFromInternalStop();
    }});
}

// Hands the inferior back after the engine drove it through a stop of its own.
// m_inInternalStop stays set: dbgeng re-announces the stop the calls were made
// from after this resume, and that notification must not reach the client.
void CdbImpl::resumeFromInternalStop()
{
    m_expectSpontaneousStop = true;
    m_inferiorRunning = true;
    runCommand({"g", NoFlags});
}

// qt_qmlDebugMessageAvailable() has an empty body, so a release Qt inlines it away
// just like the connector hook. Watch the service's own message length instead; the
// plugin is loaded by the time the connector reports itself open.
void CdbImpl::armInterpreterMessageWatch()
{
    if (m_interpreterMessageWatchArmed)
        return;
    m_interpreterMessageWatchArmed = true;
    runCommand({"lm1m m qmldbg_native*", BuiltinCommand,
               [this](const DebuggerResponse &response) {
        QString service;
        for (const QString &line : response.data.data().split(QChar::LineFeed)) {
            const QString name = line.trimmed();
            if (name == "qmldbg_native" || name == "qmldbg_natived") {
                service = name;
                break;
            }
        }
        // Only a release build inlines the hook away; a debug one calls it, and the
        // breakpoint there is exact where this watch would also catch our own traffic.
        if (service != "qmldbg_native")
            return;
        const QString watchId = nextBreakpointId();
        m_internalBreakpointIds.insert(watchId);
        m_interpreterMessageIds.insert(watchId);
        m_interpreterMessageWatchId = watchId;
        runCommand({QString("ba%1 w4 %2!qt_qmlDebugMessageLength").arg(watchId, service),
                    NoFlags});
    }});
}

void CdbImpl::resolvePendingInterpreterBreakpoints()
{
    armInterpreterMessageWatch();
    const QList<InterpreterBreakpoint> pending = m_pendingInterpreterBreakpoints;
    m_pendingInterpreterBreakpoints.clear();
    if (pending.isEmpty()) {
        resumeFromInternalStop();
        return;
    }
    const auto outstanding = std::make_shared<int>(pending.size());
    for (const InterpreterBreakpoint &breakpoint : pending) {
        DebuggerCommand cmd("theDumper.resolvePendingInterpreterBreakpoint", ScriptCommand);
        cmd.arg("modelid", breakpoint.modelId);
        cmd.arg("file", breakpoint.params.fileName.path());
        cmd.arg("line", breakpoint.params.textPosition.line);
        cmd.arg("enabled", breakpoint.params.enabled);
        cmd.arg("condition", toHex(breakpoint.params.condition));
        cmd.arg("ignorecount", breakpoint.params.ignoreCount);
        cmd.callback = [this, outstanding](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone) {
                emit message(QString("CdbImpl: could not resolve a pending QML "
                                     "breakpoint: %1").arg(response.data["msg"].data()),
                             LogWarning);
            } else {
                for (const GdbMi &line : response.data["msg"]) {
                    const QString text = line.data();
                    if (!text.startsWith("interpreterasync="))
                        continue;
                    GdbMi all;
                    QStringDecoder decoder(QStringEncoder::Utf8);
                    all.fromStringMultiple(text, decoder);
                    if (all["asyncclass"].data() != "breakpointmodified")
                        continue;
                    GdbMi list;
                    list.m_type = GdbMi::List;
                    list.addChild(all["interpreterasync"]);
                    emit breakpointModified(list);
                }
            }
            if (--*outstanding > 0)
                return;
            resumeFromInternalStop();
        };
        runCommand(cmd);
    }
}

static QString moduleName(const QString &reply)
{
    const QStringList lines = reply.split(QChar::LineFeed);
    for (const QString &line : lines) {
        const QString name = line.trimmed();
        if (!name.isEmpty() && !name.contains(' '))
            return name;
    }
    return {};
}

void CdbImpl::armInterpreterHooks()
{
    if (!m_interpreterResolverIds.isEmpty())
        return;
    for (const QString &module : {QString("qmldbg_natived"), QString("qmldbg_native")}) {
        const QString resolverId = nextBreakpointId();
        m_internalBreakpointIds.insert(resolverId);
        m_interpreterResolverIds.insert(resolverId);
        runCommand({"bu" + resolverId + ' ' + module + "!qt_qmlDebugConnectorOpen", NoFlags});

        const QString messageId = nextBreakpointId();
        m_internalBreakpointIds.insert(messageId);
        m_interpreterMessageIds.insert(messageId);
        runCommand({"bu" + messageId + ' ' + module + "!qt_qmlDebugMessageAvailable", NoFlags});
    }
    // A release Qt inlines qt_qmlDebugConnectorOpen() into its only caller,
    // QQmlNativeDebugConnector::open(), which leaves the exported symbol the
    // breakpoints above name orphaned. Watch the store that function makes to
    // qtHookData[QHooks::Startup] instead: that happens wherever the compiler
    // put the code. Qt linked into the executable is not covered.
    runCommand({"lm1m m Qt?Core*", BuiltinCommand, [this](const DebuggerResponse &response) {
        const QString qtCore = moduleName(response.data.data());
        if (qtCore.isEmpty()) {
            emit message("CdbImpl: no Qt core module, so no QML breakpoint resolver hook.",
                         LogWarning);
            return;
        }
        const QString hookId = nextBreakpointId();
        m_internalBreakpointIds.insert(hookId);
        m_interpreterResolverIds.insert(hookId);
        runCommand({QString("ba%1 w1 %2!qtHookData+@$ptrsize*5").arg(hookId, qtCore), NoFlags});
    }});
}

void CdbImpl::flushPendingBridgeWork()
{
    const QList<std::function<void()>> pending = m_pendingBridgeWork;
    m_pendingBridgeWork.clear();
    for (const std::function<void()> &work : pending)
        work();
}

void CdbImpl::setupScripting()
{
    runCommand({"print(sys.version)", ScriptCommand, [this](const DebuggerResponse &response) {
        const GdbMi data = response.data["msg"];
        if (response.resultClass != ResultDone || data.childCount() == 0) {
            emit message(QString("CdbImpl: no Python in the cdb extension, so no dumper "
                                 "bridge: %1").arg(response.data["msg"].data()), LogWarning);
            flushPendingBridgeWork();
            return;
        }
        const QString reportedVersion = data.childAt(0).data();
        const QStringList version = reportedVersion.split(' ').constFirst().split('.');
        bool ok = version.size() == 3;
        unsigned packed = ok ? version.at(0).toUInt(&ok) : 0;
        for (int i = 1; ok && i < 3; ++i)
            packed = (packed << 8) | version.at(i).toUInt(&ok);
        if (!ok) {
            emit message(QString("CdbImpl: cannot parse sys.version: %1").arg(reportedVersion),
                         LogWarning);
            flushPendingBridgeWork();
            return;
        }
        m_pythonVersion = packed;
        emit message(QString("CdbImpl: Python %1 in the cdb extension")
                         .arg(reportedVersion.trimmed()), LogMisc);

        QString dumperPath = m_startData.dumperScriptsDir.toUserOutput();
        dumperPath.replace('\\', "\\\\");
        runCommand({"sys.path.insert(1, '" + dumperPath + "')", ScriptCommand});
        runCommand({"from cdbbridge import Dumper", ScriptCommand});
        runCommand({"theDumper = Dumper()", ScriptCommand,
                   [this](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone) {
                emit message(QString("CdbImpl: could not construct the dumper bridge: %1")
                                 .arg(response.data["msg"].data()), LogError);
                m_pythonVersion = 0;
            }
            flushPendingBridgeWork();
        }});
    }});
}

// Continues ExecutionCommand::ResetInferior once the killed session is gone.
// m_isResetRestart stays set until the fresh session is idle, which is what tells
// the session_idle handler to re-insert the breakpoints instead of reporting a setup.
void CdbImpl::restartSession()
{
    m_shuttingDown = false;
    m_accessible = false;
    m_initialSessionIdleHandled = false;
    m_inferiorExited = false;
    m_inferiorRunning = false;
    m_expectSpontaneousStop = false;
    m_interruptRequested = false;
    m_inInternalStop = false;
    m_callbackStop = false;
    m_deferredCommands.clear();
    m_evaluatingCondition = false;
    m_expandingTracepoint = false;
    m_lastOperateByInstruction.reset();
    m_parentForSubBreakpointId.clear();
    m_conditionForBreakpointId.clear();
    m_internalBreakpointIds.clear();
    m_breakpointHitCounts.clear();
    m_pythonVersion = 0;
    m_interpreterResolverIds.clear();
    m_interpreterMessageIds.clear();
    m_interpreterMessageWatchArmed = false;
    m_interpreterMessageWatchId.clear();
    m_pendingBridgeWork.clear();
    m_currentBuiltinResponseToken = -1;
    m_currentBuiltinResponse.clear();
    m_extensionMessageBuffer.clear();
    m_commandForToken.clear();
    m_cdbProc.start();
}

void CdbImpl::resumeAfterSetup()
{
    if (!m_commandForToken.isEmpty()) {
        m_resumeWhenRepliesDrain = true;
        return;
    }
    m_resumeWhenRepliesDrain = false;
    m_expectSpontaneousStop = true;
    m_inferiorRunning = true;
    runCommand({"g", NoFlags});
}

void CdbImpl::initializeSession(const std::function<void()> &whenReady)
{
    runCommand({"sxn ibp", NoFlags});
    runCommand({".asm source_line", NoFlags});
    if (std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
        const FilePath inferiorDir = std::get<ProcessRunData>(
            m_startData.inferiorStartData).command.executable().parentDir();
        if (!inferiorDir.isEmpty())
            runCommand({".sympath \"" + inferiorDir.nativePath() + '"', NoFlags});
    }
    if (m_startData.nativeMixed)
        armInterpreterHooks();
    setupScripting();
    runCommand({"pid", ExtensionCommand, [this, whenReady](const DebuggerResponse &response) {
        if (response.resultClass == ResultDone)
            emit inferiorPidKnown(response.data.toProcessHandle());
        else
            emit message(QString("CdbImpl: failed to determine the inferior pid: %1")
                             .arg(response.data["msg"].data()), LogError);
        if (whenReady)
            whenReady();
    }});
}

static GdbMi dumperShapedLocals(const GdbMi &reply)
{
    GdbMi items;
    items.m_type = GdbMi::List;
    items.m_name = "data";
    for (const GdbMi &item : reply) {
        const QString encoding = item["valueencoded"].data();
        GdbMi decoded;
        decoded.m_type = GdbMi::Tuple;
        for (const GdbMi &field : item) {
            if (field.m_name == "valueencoded")
                continue;
            if (field.m_name == "value" && !encoding.isEmpty())
                decoded.addChild(constMi("value", decodeData(field.data(), encoding)));
            else
                decoded.addChild(field);
        }
        items.addChild(decoded);
    }
    GdbMi result;
    result.m_type = GdbMi::Tuple;
    result.addChild(items);
    return result;
}

static GdbMi interpreterStackFrames(const GdbMi &msg)
{
    static const QLatin1String prefix("qmlstack=");
    GdbMi frames;
    frames.m_type = GdbMi::List;
    for (const GdbMi &line : msg) {
        const QString text = line.data();
        if (!text.startsWith(prefix))
            continue;
        const QJsonObject root = QJsonDocument::fromJson(
                                     text.mid(prefix.size()).toUtf8()).object();
        const QJsonArray reported = root.value("frames").toArray();
        for (const QJsonValue &value : reported) {
            const QJsonObject frame = value.toObject();
            GdbMi item;
            item.m_type = GdbMi::Tuple;
            item.m_name = "frame";
            item.addChild(constMi("function", frame.value("function").toString()));
            item.addChild(constMi("file", frame.value("file").toString()));
            item.addChild(constMi("line", QString::number(frame.value("line").toInt())));
            item.addChild(constMi("language", frame.value("language").toString()));
            item.addChild(constMi("context", frame.value("context").toString()));
            frames.addChild(item);
        }
        break;
    }
    return frames;
}

// The extension reports the full path as "fullname" and the plain base name as "file", and
// the module as "from". StackFrame::parseFrame() expects the path in "file" and the module
// in "module" - a base name there would be resolved against the build directory.
static GdbMi normalizedFrame(const GdbMi &frameMi)
{
    const QString fullName = frameMi["fullname"].data();
    const QString module = frameMi["from"].data();
    if (fullName.isEmpty() && module.isEmpty())
        return frameMi;
    GdbMi frame;
    frame.m_type = GdbMi::Tuple;
    frame.m_name = frameMi.m_name;
    for (const GdbMi &child : frameMi) {
        if (child.m_name == "file" && !fullName.isEmpty())
            continue;
        frame.addChild(child);
    }
    if (!fullName.isEmpty()) {
        frame.addChild(constMi("file", FilePath::fromUserInput(fullName)
                                           .normalizedPathName().toUrlishString()));
    }
    if (!module.isEmpty())
        frame.addChild(constMi("module", module));
    return frame;
}

static GdbMi stackTreeFromFrames(const GdbMi &reply)
{
    GdbMi frames;
    frames.m_type = GdbMi::List;
    for (const GdbMi &frameMi : reply)
        frames.addChild(normalizedFrame(frameMi));
    frames.m_name = "frames";
    GdbMi stack;
    stack.m_type = GdbMi::Tuple;
    stack.m_name = "stack";
    stack.addChild(frames);
    GdbMi wrapper;
    wrapper.m_type = GdbMi::Tuple;
    wrapper.addChild(stack);
    return wrapper;
}

void CdbImpl::refresh(const RefreshRequest &request)
{
    if (request.kind == RefreshKind::FullBacktrace) {
        const quint64 requestId = request.requestId;
        runCommand({"~*kp", BuiltinCommand,
                   [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::FullBacktrace,
                                     constMi({}, response.data.data()));
        }});
        return;
    }
    if (request.kind == RefreshKind::Modules) {
        const quint64 requestId = request.requestId;
        runCommand({"modules", ExtensionCommand,
                   [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::Modules, response.data);
        }});
        return;
    }
    if (request.kind == RefreshKind::Registers) {
        const quint64 requestId = request.requestId;
        runCommand({"registers", ExtensionCommand,
                   [this, requestId](const DebuggerResponse &response) {
            if (response.resultClass != ResultDone) {
                emit message(QString("CdbImpl: failed to determine registers: %1")
                                 .arg(response.data["msg"].data()), LogError);
                GdbMi empty;
                empty.m_type = GdbMi::List;
                emit refreshDataReceived(requestId, RefreshKind::Registers, empty);
                return;
            }
            emit refreshDataReceived(requestId, RefreshKind::Registers, response.data);
        }});
        return;
    }
    if (request.kind == RefreshKind::FullStack) {
        const quint64 requestId = request.requestId;
        DebuggerCommand cmd("stack", ExtensionCommand,
                           [this, requestId](const DebuggerResponse &response) {
            emit refreshDataReceived(requestId, RefreshKind::FullStack,
                                     stackTreeFromFrames(response.data));
        });
        cmd.args = QString("unlimited");
        runCommand(cmd);
        return;
    }
    if (request.kind == RefreshKind::QmlStack) {
        const quint64 requestId = request.requestId;
        DebuggerCommand cmd("print('qmlstack=%s' % __import__('json').dumps("
                            "theDumper.extractInterpreterStack()))", ScriptCommand);
        cmd.callback = [this, requestId](const DebuggerResponse &response) {
            const GdbMi frames = response.resultClass == ResultDone
                                     ? interpreterStackFrames(response.data["msg"])
                                     : GdbMi();
            if (frames.childCount() != 0) {
                emit refreshDataReceived(requestId, RefreshKind::FullStack,
                                         stackTreeFromFrames(frames));
                return;
            }
            runCommand({"qmlstack", ExtensionCommand,
                       [this, requestId](const DebuggerResponse &fallback) {
                if (fallback.resultClass != ResultDone) {
                    emit message("CdbImpl: could not create a QML stack trace: "
                                     + fallback.data["msg"].data(), LogWarning);
                    emit refreshDataReceived(requestId, RefreshKind::FullStack, {});
                    return;
                }
                emit refreshDataReceived(requestId, RefreshKind::FullStack,
                                         stackTreeFromFrames(fallback.data));
            }});
        };
        runCommand(cmd);
        return;
    }
    if (request.kind == RefreshKind::DebuggingHelpers) {
        refresh({request.requestId, RefreshKind::Locals});
        return;
    }
    if (request.kind == RefreshKind::AllSymbols) {
        runCommand({".reload /f", NoFlags});
        refresh({request.requestId, RefreshKind::FullStack});
        return;
    }
    if (request.kind != RefreshKind::Locals) {
        emit message("CdbImpl: refresh kind not implemented.", LogWarning);
        return;
    }
    const quint64 requestId = request.requestId;
    DebuggerCommand cmd("locals", ExtensionCommand,
                       [this, requestId](const DebuggerResponse &response) {
        emit refreshDataReceived(requestId, RefreshKind::Locals,
                                 dumperShapedLocals(response.data));
    });
    QString args = "-v -D";
    if (request.dumperOptions.useDebuggingHelpers)
        args += " -c";
    const QStringList expanded(request.expandedINames.cbegin(), request.expandedINames.cend());
    if (!expanded.isEmpty())
        args += " -e " + expanded.join(',');
    for (const QJsonValue &value : request.watchers) {
        const QJsonObject watcher = value.toObject();
        const QString expr = QString::fromUtf8(
            QByteArray::fromHex(watcher.value("exp").toString().toUtf8()));
        if (expr.isEmpty())
            continue;
        if (!args.contains(" -W"))
            args += " -W";
        args += " -w " + watcher.value("iname").toString() + " \"" + expr + '"';
    }
    args += ' ' + QString::number(m_currentFrameIndex);
    cmd.args = args;
    m_lastDebuggableCommand = cmd;
    m_lastDebuggableCommand.callback = {};
    runCommand(cmd);
}

void CdbImpl::activateFrame(int index)
{
    m_currentFrameIndex = index;
}

void CdbImpl::selectThread(const QString &threadId)
{
    Q_UNUSED(threadId)
}

void CdbImpl::setRegisterValue(const QString &name, const QString &value)
{
    runCommand({"r " + name + '=' + value, NoFlags});
}

void CdbImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                           const QByteArray &data)
{
    if (op == MemoryOp::Change) {
        QTC_ASSERT(!data.isEmpty(), return);
        runCommand({cdbWriteMemoryCommand(addr, data), NoFlags});
        return;
    }
    const quint64 length = lengthOrSize;
    DebuggerCommand cmd("memory", ExtensionCommand);
    cmd.args = QString("%1 %2").arg(addr).arg(length);
    cmd.callback = [this, requestId, addr, length](const DebuggerResponse &response) {
        QByteArray bytes;
        if (response.resultClass == ResultDone) {
            bytes = QByteArray::fromHex(response.data.data().toUtf8());
        } else {
            emit message(QString("CdbImpl: failed to read %1 bytes at 0x%2: %3")
                             .arg(length).arg(addr, 0, 16)
                             .arg(response.data["msg"].data()), LogWarning);
        }
        if (quint64(bytes.size()) != length)
            bytes = QByteArray(int(length), char(0));
        emit memoryDataReceived(requestId, addr, bytes);
    };
    runCommand(cmd);
}

static quint64 firstSymbolAddress(const QString &reply)
{
    const QStringList lines = reply.split(QChar::LineFeed);
    for (const QString &line : lines) {
        QString token = line.trimmed().section(' ', 0, 0);
        token.remove('`');
        bool ok = false;
        const quint64 address = token.toULongLong(&ok, 16);
        if (ok && address)
            return address;
    }
    return 0;
}

void CdbImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName)
{
    if (!address) {
        if (functionName.isEmpty()) {
            emit message("CdbImpl: cannot disassemble without an address or a name.",
                         LogWarning);
            emit disassemblyReceived(requestId, {});
            return;
        }
        runCommand({"x *!" + functionName, BuiltinCommand,
                   [this, requestId, functionName](const DebuggerResponse &response) {
            const quint64 resolved = firstSymbolAddress(response.data.data());
            if (!resolved) {
                emit message(QString("CdbImpl: cannot resolve \"%1\" to disassemble it.")
                                 .arg(functionName), LogWarning);
                emit disassemblyReceived(requestId, {});
                return;
            }
            fetchDisassembly(requestId, resolved, functionName);
        }});
        return;
    }
    enum { DisassemblerRange = 512 };
    const quint64 start = address - DisassemblerRange / 2;
    const quint64 end = address + DisassemblerRange / 2;
    const QString cmd = QString("u 0x%1 0x%2").arg(start, 0, 16).arg(end, 0, 16);
    runCommand({cmd, BuiltinCommand, [this, requestId](const DebuggerResponse &response) {
        emit disassemblyReceived(requestId, parseCdbDisassembler(response.data.data()));
    }});
}

void CdbImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    const quint32 intValue = quint32(value);
    const QByteArray data(reinterpret_cast<const char *>(&intValue), sizeof(intValue));
    runCommand({cdbWriteMemoryCommand(address, data), NoFlags});
}

void CdbImpl::watchPoint(quint64 requestId, const QPoint &pnt)
{
    Q_UNUSED(requestId)
    Q_UNUSED(pnt)
}

void CdbImpl::createSnapshot(quint64 requestId)
{
    Q_UNUSED(requestId)
}

void CdbImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    Q_UNUSED(item)
    const auto hex = [](const QString &s) { return QString::fromUtf8(s.toUtf8().toHex()); };
    const QString assignment = "-h -e " + hex(expr) + '=' + hex(value);
    DebuggerCommand cmd("assign", ExtensionCommand);
    cmd.args = assignment;
    runCommand(cmd);
}

void CdbImpl::executeDebuggerCommand(const QString &command,
                                     const WatchItemData &inspectorItem)
{
    Q_UNUSED(inspectorItem)
    // Tokenized, so that the command counts as one awaiting a reply, and its
    // output is reported the way a plain one would print it.
    runCommand({command, BuiltinCommand, [this](const DebuggerResponse &response) {
        const QString output = response.data.data();
        if (!output.isEmpty())
            emit message(output, LogMisc);
    }});
}

void CdbImpl::handleCdbOutputLine(const QString &rawLine)
{
    QString line = rawLine;
    if (line.endsWith('\n'))
        line.chop(1);
    while (isCdbPrompt(line))
        line.remove(0, CdbPromptLength);

    static const QString extPrefix = "<qtcreatorcdbext>|";
    if (line.size() > extPrefix.size() && line.startsWith(extPrefix)) {
        const char type = char(line.at(extPrefix.size()).unicode());
        const int tokenPos = extPrefix.size() + 2;
        const int tokenEnd = line.indexOf('|', tokenPos);
        QTC_ASSERT(tokenEnd != -1, return);
        const int token = line.mid(tokenPos, tokenEnd - tokenPos).toInt();
        const int chunksPos = tokenEnd + 1;
        const int chunksEnd = line.indexOf('|', chunksPos);
        QTC_ASSERT(chunksEnd != -1, return);
        const int remainingChunks = line.mid(chunksPos, chunksEnd - chunksPos).toInt();
        const int whatPos = chunksEnd + 1;
        const int whatEnd = line.indexOf('|', whatPos);
        QTC_ASSERT(whatEnd != -1, return);
        const QString what = line.mid(whatPos, whatEnd - whatPos);
        m_extensionMessageBuffer += line.mid(whatEnd + 1);
        if (remainingChunks == 0) {
            handleExtensionMessage(type, token, what, m_extensionMessageBuffer);
            m_extensionMessageBuffer.clear();
        }
        return;
    }

    int token = 0;
    bool isStart = false;
    const bool isCommandToken = checkCommandToken(m_tokenPrefix, line, &token, &isStart);
    if (m_currentBuiltinResponseToken != -1) {
        QTC_ASSERT(!isStart, return);
        if (isCommandToken) {
            const DebuggerCommand command = m_commandForToken.take(token);
            if (command.callback) {
                DebuggerResponse response;
                response.token = token;
                response.resultClass = ResultDone;
                response.data.m_name = "data";
                response.data.m_data = m_currentBuiltinResponse;
                response.data.m_type = GdbMi::Tuple;
                command.callback(response);
            }
            m_currentBuiltinResponseToken = -1;
            m_currentBuiltinResponse.clear();
            if (m_resumeWhenRepliesDrain)
                resumeAfterSetup();
        } else {
            if (!m_currentBuiltinResponse.isEmpty())
                m_currentBuiltinResponse.push_back('\n');
            m_currentBuiltinResponse.push_back(line);
        }
        return;
    }
    if (isCommandToken) {
        m_currentBuiltinResponseToken = token;
        return;
    }
    static const QRegularExpression hitRe("^Breakpoint (\\d+) hit$");
    const QRegularExpressionMatch hit = hitRe.match(line);
    if (hit.hasMatch()) {
        const QString number = hit.captured(1);
        if (m_internalBreakpointIds.contains(number)) {
            emit message(line, LogMisc);
            return;
        }
        const QString reportedNumber = m_parentForSubBreakpointId.value(number, number);
        const int times = ++m_breakpointHitCounts[reportedNumber];
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi("number", reportedNumber));
        bkpt.addChild(constMi("enabled", "y"));
        bkpt.addChild(constMi("times", QString::number(times)));
        GdbMi list;
        list.m_type = GdbMi::List;
        list.addChild(bkpt);
        emit breakpointModified(list);
        emit message(line, LogMisc);
        return;
    }
    // What cdb relays inline while the inferior runs is the debuggee's own output,
    // except for cdb's own notifications - ModLoad being by far the most frequent.
    const bool isDebuggeeOutput = m_inferiorRunning && !line.startsWith("ModLoad: ");
    emit message(line, isDebuggeeOutput ? AppOutput : LogMisc);
}

void CdbImpl::handleExtensionMessage(char type, int token, const QString &what,
                                     const QString &payload)
{
    if (type == 'R' || type == 'N') {
        if (token == -1)
            return;
        const DebuggerCommand command = m_commandForToken.take(token);
        if (!command.callback)
            return;
        DebuggerResponse response;
        response.token = token;
        response.data.m_name = "data";
        if (type == 'R') {
            response.resultClass = ResultDone;
            QStringDecoder decoder(QStringEncoder::System);
            response.data.fromString(payload, decoder);
            if (!response.data.isValid()) {
                response.data.m_data = payload;
                response.data.m_type = GdbMi::Tuple;
            }
        } else {
            response.resultClass = ResultFail;
            GdbMi msg;
            msg.m_name = "msg";
            msg.m_data = payload;
            msg.m_type = GdbMi::Tuple;
            response.data.m_type = GdbMi::Tuple;
            response.data.addChild(msg);
        }
        command.callback(response);
        if (m_resumeWhenRepliesDrain)
            resumeAfterSetup();
        return;
    }

    if (what == "event" && payload.startsWith("Process exited")) {
        m_inferiorRunning = false;
        if (!m_shuttingDown && !m_inferiorExited) {
            m_inferiorExited = true;
            static const QRegularExpression exitCode(R"(\((\d+)\))");
            const QRegularExpressionMatch match = exitCode.match(payload);
            emit inferiorDone({match.hasMatch() ? match.captured(1).toInt() : 0,
                               InferiorExitStatus::Normal});
        }
        return;
    }

    if (what == "session_accessible") {
        m_accessible = true;
        return;
    }

    // 7 is dbgeng's DEBUG_SESSION_END, and only a session that was accessible
    // can end - cdb reports one inaccessible session before the first one opens.
    if (what == "session_inaccessible") {
        if (!m_accessible)
            return;
        m_accessible = false;
        if (payload.trimmed() != "7")
            return;
        m_inferiorRunning = false;
        if (!m_shuttingDown && !m_inferiorExited) {
            m_inferiorExited = true;
            emit inferiorDone({0, InferiorExitStatus::Normal});
        }
        return;
    }

    if (what == "session_idle") {
        const bool firstTime = !m_initialSessionIdleHandled;
        m_initialSessionIdleHandled = true;
        if (firstTime) {
            initializeSession([this] {
                if (m_isResetRestart) {
                    m_isResetRestart = false;
                    // insertBreakpoint() writes back into m_insertedBreakpoints.
                    const QHash<QString, BreakpointParameters> restored = m_insertedBreakpoints;
                    for (auto it = restored.cbegin(); it != restored.cend(); ++it)
                        insertBreakpoint(0, it.key(), 0, it.value(), false);
                } else {
                    emit inferiorEvent(InferiorEvent::EngineSetupOk);
                    emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
                }
                resumeAfterSetup();
            });
            return;
        }
        m_inferiorRunning = false;
        // Held back while the session ran; cdb runs them before any resume
        // written after them.
        const QList<DebuggerCommand> deferred = m_deferredCommands;
        m_deferredCommands.clear();
        for (const DebuggerCommand &cmd : deferred)
            runCommand(cmd);
        if (m_callbackStop) {
            // Ours, to get a command in: the client never learns about it.
            m_callbackStop = false;
            resumeFromInternalStop();
            return;
        }
        GdbMi stopData;
        QStringDecoder decoder(QStringEncoder::System);
        stopData.fromString(payload, decoder);
        // An inferior call of ours makes dbgeng re-announce the stop it was made
        // from, without a reason or a breakpoint id to route by. Recognizing it
        // by location, as CdbEngine does, needs private symbols, which a release
        // Qt does not ship; that the engine itself is driving the inferior is
        // known either way.
        const QString stopFunction = stoppedFunction(stopData);
        if (m_inInternalStop && !m_interruptRequested
                && stopData["reason"].data() != "breakpoint"
                && stopData["breakpointId"].data().isEmpty()) {
            return;
        }
        if (stopFunction.endsWith("qt_qmlDebugMessageAvailable")
                || m_interpreterMessageIds.contains(stopData["breakpointId"].data())) {
            m_inInternalStop = true;
            handleInterpreterMessage(stopData);
            return;
        }
        if (stopFunction.endsWith("qt_qmlDebugConnectorOpen")
                || (stopData["reason"].data() == "breakpoint"
                    && m_interpreterResolverIds.contains(stopData["breakpointId"].data()))) {
            m_inInternalStop = true;
            resolvePendingInterpreterBreakpoints();
            return;
        }
        const QString stoppedId = m_parentForSubBreakpointId.value(
            stopData["breakpointId"].data(), stopData["breakpointId"].data());
        const QString condition = m_conditionForBreakpointId.value(stoppedId);
        if (!condition.isEmpty() && !m_evaluatingCondition
                && stopData["reason"].data() == "breakpoint") {
            QString args = condition;
            if (args.contains(' ') && !args.startsWith('"'))
                args = '"' + args + '"';
            m_evaluatingCondition = true;
            DebuggerCommand cmd("expression", ExtensionCommand);
            cmd.args = args;
            cmd.callback = [this, stopData, condition](const DebuggerResponse &response) {
                m_evaluatingCondition = false;
                const bool failed = response.resultClass != ResultDone;
                if (failed) {
                    emit message(QString("CdbImpl: could not evaluate the condition "
                                         "\"%1\": %2")
                                     .arg(condition, response.data["msg"].data()), LogError);
                }
                const int value = failed ? 1 : response.data.data().toInt();
                emit message(QString("CdbImpl: condition \"%1\" evaluated to %2, %3.")
                                 .arg(condition).arg(value)
                                 .arg(QLatin1String(value ? "stopping" : "continuing")),
                             LogMisc);
                if (value) {
                    reportStop(stopData);
                    return;
                }
                m_expectSpontaneousStop = true;
                m_inferiorRunning = true;
                runCommand({"g", NoFlags});
            };
            runCommand(cmd);
            return;
        }
        if (stopData["reason"].data() == "breakpoint" && !m_expandingTracepoint
                && m_insertedBreakpoints.contains(stoppedId)) {
            const BreakpointParameters stopped = m_insertedBreakpoints.value(stoppedId);
            QStringList tracepointMessages;
            bool stopAfterwards = false;
            for (const BreakpointParameters &params : std::as_const(m_insertedBreakpoints)) {
                if (!isSameLocation(params, stopped))
                    continue;
                if (params.tracepoint)
                    tracepointMessages.append(params.message);
                else
                    stopAfterwards = true;
            }
            if (!tracepointMessages.isEmpty()) {
                reportTracepoint(tracepointMessages, stopData, stopAfterwards);
                return;
            }
        }
        reportStop(stopData);
        return;
    }
}

void CdbImpl::reportStop(const GdbMi &stopData)
{
    const GdbMi stack = stopData["stack"];
    if (stack.childCount() > 0) {
        const GdbMi &topFrame = stack.childAt(0);
        const QString fullName = topFrame["fullname"].data();
        if (!fullName.isEmpty())
            emit locationChanged(FilePath::fromUserInput(fullName), topFrame["line"].toInt());
    }
    m_inferiorRunning = false;
    m_inInternalStop = false;
    if (m_interruptRequested) {
        m_interruptRequested = false;
        emit inferiorEvent(InferiorEvent::StopOk);
    } else if (m_expectSpontaneousStop) {
        emit inferiorEvent(InferiorEvent::SpontaneousStop);
    } else {
        emit inferiorEvent(InferiorEvent::StopOk);
    }
    m_expectSpontaneousStop = false;
}

void CdbImpl::adjustOperateByInstruction(bool operateByInstruction)
{
    if (m_lastOperateByInstruction == operateByInstruction)
        return;
    m_lastOperateByInstruction = operateByInstruction;
    runCommand({operateByInstruction ? QLatin1String("l-t") : QLatin1String("l+t"), NoFlags});
}

void CdbImpl::jumpToAddress(quint64 address, const Utils::FilePath &file, int line)
{
    const QLatin1String pcRegister(m_startData.inferiorWordWidth == 64 ? "rip" : "eip");
    runCommand({"r " + pcRegister + "=0x" + QString::number(address, 16), NoFlags});
    if (!file.isEmpty())
        emit locationChanged(file, line);
    emit inferiorEvent(InferiorEvent::SpontaneousStop);
}

void CdbImpl::runCommand(const DebuggerCommand &dbgCmd)
{
    if (!m_cdbProc.isRunning()) {
        if (!m_shuttingDown) {
            emit message("CdbImpl: dropping \"" + dbgCmd.function + "\", the session is gone.",
                         LogWarning);
        }
        return;
    }
    // A command written into a running session is executed by cdb at the next
    // stop, ahead of the engine's own handling of it. Interrupt and re-issue it
    // once the inferior is back, as CdbEngine does for a non-accessible session.
    if (m_inferiorRunning && dbgCmd.flags != NoFlags) {
        m_deferredCommands.append(dbgCmd);
        if (!m_callbackStop) {
            m_callbackStop = true;
            m_cdbProc.interrupt();
        }
        restartWatchdog();
        return;
    }
    if (dbgCmd.flags & ScriptCommand) {
        // A dumper script may talk to the QML service, whose replies write the very
        // global the message watch sits on. Keep it out of our own traffic.
        const QString watchId = m_interpreterMessageWatchId;
        if (!watchId.isEmpty())
            runCommand({"bd" + watchId, NoFlags});
        const DebuggerCommand::Callback callback = dbgCmd.callback;
        DebuggerCommand scriptCmd("script", ExtensionCommand,
                                  [this, watchId, callback](const DebuggerResponse &r) {
            if (!watchId.isEmpty())
                runCommand({"be" + watchId, NoFlags});
            if (callback)
                callback(r);
        });
        scriptCmd.args = dbgCmd.args.isNull()
                       ? dbgCmd.function
                       : dbgCmd.function + '(' + dbgCmd.argsToPython() + ')';
        runCommand(scriptCmd);
        return;
    }
    QString fullCmd;
    if (dbgCmd.flags == NoFlags) {
        fullCmd = dbgCmd.function + '\n';
    } else if (dbgCmd.flags & BuiltinCommand) {
        const int token = ++m_nextCommandToken;
        m_commandForToken.insert(token, dbgCmd);
        fullCmd = ".echo \"" + m_tokenPrefix + QString::number(token) + "<\"\n"
                + dbgCmd.function + "\n"
                + ".echo \"" + m_tokenPrefix + QString::number(token) + ">\"\n";
    } else if (dbgCmd.flags & ExtensionCommand) {
        const int token = ++m_nextCommandToken;
        m_commandForToken.insert(token, dbgCmd);
        const QString prefix = m_extensionCommandPrefix + dbgCmd.function;
        const QString arguments = dbgCmd.args.isString() ? dbgCmd.args.toString() : QString();
        fullCmd = prefix + " -t " + QString::number(token) + ".0 " + arguments + '\n';
    }
    emit message(fullCmd.trimmed(), LogInput);
    m_cdbProc.write(fullCmd);
    restartWatchdog();
}

// Only commands carrying a token are waited for: cdb answers nothing for the
// rest, and a resumed inferior may keep it silent for as long as it likes.
void CdbImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_commandForToken.isEmpty() && m_deferredCommands.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}

} // namespace Debugger::Internal
