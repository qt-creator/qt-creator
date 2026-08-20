// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgeimpl.h"

#include "../disassemblerlines.h"

#include "../dap/dapclient.h"

#include <utils/environment.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QStringDecoder>

#include <cstring>

using namespace Utils;

namespace Debugger::Internal {

BridgeImplHostRecipe gdbHostRecipe(bool loadInitFile)
{
    BridgeImplHostRecipe recipe;
    recipe.startupArguments = {"--nw", "-q"};
    if (!loadInitFile)
        recipe.startupArguments << "--nx";
    recipe.bridgeModule = "gdbbridge";
    recipe.serverCall = "theDumper.runDapServer()";
    return recipe;
}

namespace {

class BridgeImplDataProvider final : public IDataProvider
{
public:
    BridgeImplDataProvider(const ProcessRunData &runData, const CommandLine &cmd, QObject *parent)
        : IDataProvider(parent)
        , m_runData(runData)
        , m_cmd(cmd)
    {
        connect(&m_proc, &Process::started, this, &IDataProvider::started);
        connect(&m_proc, &Process::done, this, &IDataProvider::done);
        connect(&m_proc, &Process::readyReadStandardOutput,
                this, &IDataProvider::readyReadStandardOutput);
        connect(&m_proc, &Process::readyReadStandardError,
                this, &IDataProvider::readyReadStandardError);
    }

    ~BridgeImplDataProvider() final
    {
        m_proc.kill();
        m_proc.waitForFinished();
    }

    void start() final
    {
        m_proc.setProcessMode(ProcessMode::Writer);
        if (m_runData.workingDirectory.isDir())
            m_proc.setWorkingDirectory(m_runData.workingDirectory);
        Environment env = m_runData.environment;
        env.setupEnglishOutput();
        m_proc.setEnvironment(env);
        m_proc.setCommand(m_cmd);
        m_proc.start();
    }

    bool isRunning() const final { return m_proc.isRunning(); }
    void writeRaw(const QByteArray &data) final
    {
        if (m_proc.state() == ProcessState::Running)
            m_proc.writeRaw(data);
    }
    void kill() final { m_proc.kill(); }
    void interrupt() final { m_proc.interrupt(); }
    QByteArray readAllStandardOutput() final { return m_proc.readAllStandardOutput().toUtf8(); }
    QString readAllStandardError() final { return m_proc.readAllStandardError(); }
    int exitCode() const final { return m_proc.exitCode(); }
    QString executable() const final { return m_proc.commandLine().executable().toUserOutput(); }

    QProcess::ExitStatus exitStatus() const final { return toQProcess(m_proc.exitStatus()); }
    QProcess::ProcessError error() const final { return toQProcess(m_proc.error()); }
    Utils::ProcessResult result() const final { return m_proc.result(); }
    QString exitMessage() const final { return m_proc.exitMessage(); }

    Utils::ProcessResultData resultData() const { return m_proc.resultData(); }

private:
    Process m_proc;
    const ProcessRunData m_runData;
    const CommandLine m_cmd;
};

class BridgeImplClient final : public DapClient
{
public:
    using DapClient::DapClient;

private:
    const QLoggingCategory &logCategory() final
    {
        static const QLoggingCategory category("qtc.dbg.bridgeimpl", QtWarningMsg);
        return category;
    }
};

} // namespace

static DebuggerEngineSetupData bridgeImplSetupData()
{
    DebuggerEngineSetupData data;
    data.capabilities = ReloadModuleCapability | BreakConditionCapability
                      | ShowModuleSymbolsCapability | RunToLineCapability | AddWatcherCapability
                      | RegisterCapability | ShowMemoryCapability | DisassemblerCapability
                      | OperateByInstructionCapability | JumpToLineCapability
                      | WatchpointByAddressCapability | WatchpointByExpressionCapability;
    data.startModes = DebuggerStartModeFlag::Launch | DebuggerStartModeFlag::AttachToProcess;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferior;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        if (query.type == WatchpointAtAddress || query.type == WatchpointAtExpression
            || query.type == BreakpointByFunction || query.type == BreakpointByAddress) {
            return true;
        }
        const MimeType mimeType = Utils::mimeTypeForFile(query.fileName);
        return mimeType.matchesName(Utils::Constants::C_HEADER_MIMETYPE)
               || mimeType.matchesName(Utils::Constants::C_SOURCE_MIMETYPE)
               || mimeType.matchesName(Utils::Constants::CPP_HEADER_MIMETYPE)
               || mimeType.matchesName(Utils::Constants::CPP_SOURCE_MIMETYPE);
    };
    return data;
}

BridgeImpl::BridgeImpl(const BridgeImplStartData &startData)
    : DebuggerEngineInterface(bridgeImplSetupData())
    , m_startData(startData)
{}

BridgeImpl::~BridgeImpl() = default;

void BridgeImpl::start()
{
    CommandLine cmd{m_startData.debuggerRunData.command.executable(),
                    m_startData.hostRecipe.startupArguments};
    cmd.addArgs({"-iex", "python sys.path.insert(1, '" + m_startData.dumperScriptsDir.path() + "')"});
    cmd.addArgs({"-iex", "python from " + m_startData.hostRecipe.bridgeModule + " import *"});
    cmd.addArgs({"-ex", "python " + m_startData.hostRecipe.serverCall});

    auto provider = new BridgeImplDataProvider(m_startData.debuggerRunData, cmd, this);
    m_client = new BridgeImplClient(provider, this);

    connect(m_client, &DapClient::started, this, &BridgeImpl::handleStarted);
    connect(m_client, &DapClient::done, this, &BridgeImpl::handleFinished);
    connect(m_client, &DapClient::readyReadStandardError, this, &BridgeImpl::handleStandardError);
    connect(m_client, &DapClient::responseReady, this, &BridgeImpl::handleResponse);
    connect(m_client, &DapClient::eventReady, this, &BridgeImpl::handleEvent);

    emit message(cmd.toUserOutput(), LogInput);
    provider->start();
}

void BridgeImpl::handleStarted()
{
    emit inferiorEvent(InferiorEvent::EngineSetupOk);
    // Not sendInitialize(): the user's extra dumpers have to travel with it,
    // because the bridge sets them up while answering.
    QJsonObject args{{"clientID", "QtCreator"}, {"clientName", "QtCreator"},
                     {"adapterID", m_startData.hostRecipe.bridgeModule}};
    QJsonArray dumperFiles;
    for (const FilePath &file : m_startData.extraDumperFiles)
        dumperFiles.append(file.path());
    if (!dumperFiles.isEmpty())
        args.insert("qtcDumperFiles", dumperFiles);
    if (!m_startData.extraDumperCommands.isEmpty())
        args.insert("qtcDumperCommands", m_startData.extraDumperCommands.join('\n'));
    postRequest("initialize", args);
}

// The search paths the debuggee's symbols and sources are found under. The
// bridge applies them to its own host debugger.
void BridgeImpl::configureTarget()
{
    QJsonArray mappings;
    for (const QPair<QString, QString> &mapping : m_startData.sourcePathMap)
        mappings.append(QJsonObject{{"from", mapping.first}, {"to", mapping.second}});
    QJsonArray directories;
    for (const FilePath &directory : m_startData.sourceDirectories)
        directories.append(directory.path());

    QJsonObject args;
    if (!mappings.isEmpty())
        args.insert("sourcePathMap", mappings);
    if (!directories.isEmpty())
        args.insert("sourceDirectories", directories);
    if (!m_startData.sysroot.isEmpty())
        args.insert("sysroot", m_startData.sysroot.path());
    if (!args.isEmpty())
        postRequest("qtc/configureTarget", args);
}

void BridgeImpl::handleFinished()
{
    auto provider = static_cast<BridgeImplDataProvider *>(m_client->dataProvider());
    if (m_client->dataProvider()->result() == ProcessResult::StartFailed)
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
    emit engineProcessFinished(provider->resultData());
}

static GdbMi constMi(const QString &name, const QString &data)
{
    GdbMi mi;
    mi.m_name = name;
    mi.m_data = data;
    mi.m_type = GdbMi::Const;
    return mi;
}

// The dumpers answer in their own GdbMi shape, carried as a string.
static GdbMi dumperResultOf(const QJsonObject &response)
{
    const QString payload = response.value("body").toObject().value("dumperResult").toString();
    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi result;
    result.fromString('{' + payload + '}', decoder);
    return result;
}

void BridgeImpl::handleStandardError()
{
    const QString error = m_client->dataProvider()->readAllStandardError();
    if (!error.isEmpty())
        emit message(error, LogError);
}

void BridgeImpl::postLaunchOrAttach()
{
    if (const auto attach = std::get_if<AttachToProcessData>(&m_startData.inferiorStartData)) {
        postRequest("attach", QJsonObject{{"pid", qint64(attach->pid.pid())}});
        return;
    }

    const auto runData = std::get_if<ProcessRunData>(&m_startData.inferiorStartData);
    QTC_ASSERT(runData, emit inferiorEvent(InferiorEvent::EngineRunFailed); return);

    QJsonObject args{{"noDebug", false},
                     {"program", runData->command.executable().path()},
                     {"args", runData->command.arguments()}};
    if (!runData->workingDirectory.isEmpty())
        args.insert("cwd", runData->workingDirectory.path());

    Environment debuggerEnv = m_startData.debuggerRunData.environment;
    debuggerEnv.setupEnglishOutput();
    QJsonArray env;
    for (const EnvironmentItem &item : debuggerEnv.diff(runData->environment)) {
        const bool unset = item.operation == EnvironmentItem::Unset
                           || item.operation == EnvironmentItem::SetDisabled;
        const bool isWindowsPath = HostOsInfo::isWindowsHost()
                                   && item.name.compare("path", Qt::CaseInsensitive) == 0;
        const QString name = isWindowsPath ? QString("PATH") : item.name;
        if (!unset && name != item.name)
            env.append(QJsonObject{{"name", item.name}, {"value", QString()}, {"unset", true}});
        env.append(QJsonObject{{"name", name},
                               {"value", unset ? QString() : item.value},
                               {"unset", unset}});
    }
    if (!env.isEmpty())
        args.insert("env", env);

    postRequest("launch", args);
}

void BridgeImpl::shutdownInferior(ShutdownMode mode)
{
    if (!m_client) {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
        return;
    }
    // Answered only once the inferior is really gone: reporting it earlier
    // lets the engine shut the bridge down from under the kill.
    if (mode == ShutdownMode::Detach)
        postRequest("disconnect", QJsonObject{{"restart", false}, {"terminateDebuggee", false}});
    else
        postRequest("terminate", QJsonObject{{"restart", false}});
}

void BridgeImpl::shutdownEngine()
{
    if (m_client) {
        m_client->sendTerminate();
        m_client->dataProvider()->kill();
    }
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

// 'Operate by instruction' is a step granularity in DAP.
QJsonObject BridgeImpl::stepArguments(bool byInstruction) const
{
    QJsonObject args{{"threadId", m_currentThreadId}};
    if (byInstruction)
        args.insert("granularity", "instruction");
    return args;
}

void BridgeImpl::execute(const ExecutionRequest &request)
{
    QTC_ASSERT(m_client, return);

    switch (request.command) {
    case ExecutionCommand::Continue:
        if (m_inferiorRunning) {
            // The bridge is blocked in the resume it is already running, so it
            // cannot answer: a second request would sit in the socket and
            // resume again behind the next stop.
            emit inferiorEvent(InferiorEvent::RunFailed);
            return;
        }
        m_stopRequested = false;
        m_client->sendContinue(m_currentThreadId);
        return;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            return;
        }
        m_stopRequested = true;
        m_client->dataProvider()->interrupt();
        return;
    case ExecutionCommand::StepIn:
        postRequest("stepIn", stepArguments(request.flag));
        return;
    case ExecutionCommand::StepOver:
        postRequest("next", stepArguments(request.flag));
        return;
    case ExecutionCommand::StepOut:
        m_client->sendStepOut(m_currentThreadId);
        return;
    case ExecutionCommand::RunToLine:
        postRequest("qtc/runToLine",
                    QJsonObject{{"file", request.context.fileName.path()},
                                {"line", request.context.textPosition.line}});
        return;
    case ExecutionCommand::RunToFunction:
        postRequest("qtc/runToFunction",
                    QJsonObject{{"function", request.functionName}});
        return;
    case ExecutionCommand::JumpToLine:
        postRequest("qtc/jumpToLine",
                    QJsonObject{{"file", request.context.fileName.path()},
                                {"line", request.context.textPosition.line}});
        return;
    case ExecutionCommand::Detach:
        m_client->sendDisconnect();
        return;
    case ExecutionCommand::Abort:
        m_client->sendTerminate();
        return;
    case ExecutionCommand::RepeatLastCommand:
        if (!m_lastDebuggableCommand.isEmpty())
            postRequest(m_lastDebuggableCommand, m_lastDebuggableArguments);
        return;
    case ExecutionCommand::Return:
    case ExecutionCommand::ResetInferior:
    case ExecutionCommand::RecordReverse:
        emit message(QString("command not supported by the bridge"), LogWarning);
        return;
    }
}

int BridgeImpl::postRequest(const QString &command, const QJsonObject &arguments)
{
    QTC_ASSERT(m_client, return -1);
    const int seq = m_client->postRequest(command, arguments);
    emit message(QString::number(seq) + command + '('
                     + QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact))
                     + ')',
                 LogInput);
    return seq;
}

void BridgeImpl::postBreakpointRequest(const QString &request,
                                       const BreakpointChangeRequest &change)
{
    const BreakpointParameters &params = change.params;
    m_breakpointRequestIds.insert(change.modelId, change.requestId);

    QJsonObject args{{"modelid", change.modelId},
                     {"id", change.responseId},
                     {"type", int(params.type)},
                     {"ignorecount", params.ignoreCount},
                     {"condition", QString::fromUtf8(params.condition.toUtf8().toHex())},
                     {"command", QString::fromUtf8(params.command.toUtf8().toHex())},
                     {"function", params.functionName},
                     {"oneshot", params.oneShot},
                     {"enabled", params.enabled},
                     {"line", params.textPosition.line},
                     {"address", qint64(params.address)},
                     {"expression", params.expression},
                     {"tracepoint", params.tracepoint},
                     {"message", QString::fromUtf8(params.message.toUtf8().toHex())},
                     {"file", params.fileName.path()}};
    postRequest(request, args);
}

void BridgeImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    QTC_ASSERT(m_client, return);

    switch (request.op) {
    case BreakpointOp::Insert:
        postBreakpointRequest("qtc/insertBreakpoint", request);
        return;
    case BreakpointOp::Update:
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Update, false);
            return;
        }
        postBreakpointRequest("qtc/updateBreakpoint", request);
        return;
    case BreakpointOp::Remove:
        m_breakpointRequestIds.insert(request.modelId, request.requestId);
        postRequest("qtc/removeBreakpoint",
                    QJsonObject{{"modelid", request.modelId},
                                {"id", request.responseId}});
        return;
    case BreakpointOp::EnableSub:
        m_breakpointRequestIds.insert(request.modelId, request.requestId);
        postRequest("qtc/enableSubBreakpoint",
                    QJsonObject{{"modelid", request.modelId},
                                {"id", request.subResponseId},
                                {"enabled", request.enabled}});
        return;
    }
}

void BridgeImpl::refresh(const RefreshRequest &request)
{
    QTC_ASSERT(m_client, return);

    switch (request.kind) {
    case RefreshKind::Locals: {
        m_pendingLocalsRequestId = request.requestId;
        const DumperOptions &options = request.dumperOptions;
        DebuggerCommand cmd;
        cmd.arg("fancy", options.useDebuggingHelpers);
        cmd.arg("autoderef", request.autoDerefPointers);
        cmd.arg("dyntype", options.useDynamicType);
        cmd.arg("qobjectnames", options.showQObjectNames);
        cmd.arg("timestamps", options.logTimeStamps);
        cmd.arg("stringcutoff", options.maximalStringLength);
        cmd.arg("displaystringlimit", options.displayStringLimit);
        cmd.arg("allowinferiorcalls", request.allowInferiorCalls);
        cmd.arg("partialvar", request.partialVariable);
        cmd.arg("context", request.context);
        cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
        // A map of iname to array limit, not a list: the dumpers index it by
        // iname, so a list matches nothing.
        cmd.arg("expanded", request.expandedForDumpers());
        cmd.arg("typeformats", request.typeFormats);
        cmd.arg("formats", request.individualFormats);
        cmd.arg("formattypes", request.formatTypes);
        cmd.arg("watchers", request.watchers);
        cmd.arg("qtversion", m_startData.qtVersion);
        cmd.arg("qtnamespace", m_startData.qtNamespace);
        cmd.arg("frameid", m_currentFrameId);
        postRequest("qtc/fetchVariables", cmd.args.toObject());
        // Repeating it is a debugging aid: let the dumpers throw then.
        m_lastDebuggableCommand = "qtc/fetchVariables";
        cmd.arg("passexceptions", true);
        m_lastDebuggableArguments = cmd.args.toObject();
        return;
    }
    case RefreshKind::FullStack:
        if (const int seq = m_client->stackTrace(m_currentThreadId); seq >= 0)
            m_stackTraceRequests.insert(seq, {false, request.requestId});
        return;
    case RefreshKind::Registers:
        m_pendingRegistersRequestId = request.requestId;
        postRequest("qtc/fetchRegisters", QJsonObject{{"frameId", m_currentFrameId}});
        return;
    case RefreshKind::PeripheralRegisters:
        for (const quint64 address : request.addresses) {
            const quint64 token = ++m_nextPeripheralToken;
            m_peripheralRequests.insert(token, {request.requestId, address});
            postRequest("qtc/readMemory",
                        QJsonObject{{"address", QString::number(address)},
                                    {"length", qint64(sizeof(quint32))},
                                    {"token", qint64(token)}});
        }
        return;
    case RefreshKind::Modules:
        m_pendingModulesRequestId = request.requestId;
        postRequest("qtc/fetchModules", {});
        return;
    case RefreshKind::ModuleSymbols:
        m_pendingSymbolsRequestId = request.requestId;
        postRequest("qtc/fetchSymbols", QJsonObject{{"module", request.path.path()}});
        return;
    case RefreshKind::AllSymbols:
        postRequest("qtc/loadSymbols", {});
        refresh({request.requestId, RefreshKind::Modules});
        refresh({request.requestId, RefreshKind::FullStack});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    case RefreshKind::StackSymbols:
        postRequest("qtc/loadSymbols",
                    QJsonObject{{"module", request.path.path()}});
        return;
    case RefreshKind::DebuggingHelpers:
        postRequest("qtc/reloadDumpers", {});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    default:
        emit refreshDataReceived(request.requestId, request.kind, {});
        return;
    }
}

void BridgeImpl::handleResponse(DapResponseType type, const QJsonObject &response)
{
    const QString command = response.value("command").toString();
    const bool success = response.value("success").toBool();

    switch (type) {
    case DapResponseType::Initialize:
        configureTarget();
        postLaunchOrAttach();
        return;
    case DapResponseType::ConfigurationDone:
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
        m_inferiorRunning = true;
        return;
    case DapResponseType::Continue:
        if (!success && response.value("message").toString() == "The program is not being run.") {
            emit inferiorEvent(InferiorEvent::InferiorIll);
            return;
        }
        emit inferiorEvent(success ? InferiorEvent::RunOk : InferiorEvent::RunFailed);
        m_inferiorRunning = success;
        return;
    case DapResponseType::StepIn:
    case DapResponseType::StepOut:
    case DapResponseType::StepOver:
        emit inferiorEvent(success ? InferiorEvent::RunOk : InferiorEvent::RunFailed);
        return;
    case DapResponseType::StackTrace:
        handleStackTrace(response);
        return;
    case DapResponseType::Pause:
        if (!success)
            emit inferiorEvent(InferiorEvent::StopFailed);
        return;
    case DapResponseType::Launch:
    case DapResponseType::Attach:
        if (!success) {
            emit message(response.value("message").toString(), LogError);
            emit inferiorEvent(InferiorEvent::EngineRunFailed);
        }
        return;
    default:
        break;
    }

    if (command == "qtc/fetchVariables") {
        emit refreshDataReceived(m_pendingLocalsRequestId, RefreshKind::Locals,
                                 dumperResultOf(response));
    } else if (command == "qtc/fetchModules") {
        GdbMi modules;
        modules.m_type = GdbMi::List;
        const QJsonArray reported = response.value("body").toObject()
                                        .value("modules").toArray();
        for (const QJsonValue &value : reported) {
            const QJsonObject module = value.toObject();
            GdbMi item;
            item.m_type = GdbMi::Tuple;
            item.addChild(constMi("modulepath", module.value("path").toString()));
            item.addChild(constMi("symbolsread",
                                  QLatin1String(module.value("symbolsRead").toBool() ? "Yes"
                                                                                     : "No")));
            item.addChild(constMi("startaddress",
                                  QString::number(module.value("startAddress").toDouble(), 'f', 0)));
            item.addChild(constMi("endaddress",
                                  QString::number(module.value("endAddress").toDouble(), 'f', 0)));
            modules.addChild(item);
        }
        emit refreshDataReceived(m_pendingModulesRequestId, RefreshKind::Modules, modules);
    } else if (command == "qtc/fetchRegisters") {
        GdbMi registers;
        registers.m_type = GdbMi::List;
        const QJsonArray reported = response.value("body").toObject()
                                        .value("registers").toArray();
        for (const QJsonValue &value : reported) {
            const QJsonObject reg = value.toObject();
            GdbMi item;
            item.m_type = GdbMi::Tuple;
            item.addChild(constMi("name", reg.value("name").toString()));
            item.addChild(constMi("value", reg.value("value").toString()));
            item.addChild(constMi("size", QString::number(reg.value("size").toInt())));
            registers.addChild(item);
        }
        emit refreshDataReceived(m_pendingRegistersRequestId, RefreshKind::Registers, registers);
    } else if (command == "qtc/readMemory") {
        const QJsonObject body = response.value("body").toObject();
        const QByteArray data = QByteArray::fromBase64(
            body.value("data").toString().toUtf8());
        bool ok = false;
        const quint64 address = body.value("address").toString().toULongLong(&ok, 0);
        const quint64 token = quint64(body.value("token").toDouble());
        if (const auto chunk = m_memoryRequests.take(token); chunk.requestId) {
            --*chunk.pending;
            if (response.value("success").toBool()) {
                const qsizetype copied = qMin(qsizetype(chunk.length), data.size());
                memcpy(chunk.accumulator->data() + chunk.offset, data.constData(), copied);
            } else if (chunk.length > 1) {
                // Part of the range may still be readable.
                const quint64 half = chunk.length / 2;
                fetchMemoryChunk(chunk, chunk.offset, half);
                fetchMemoryChunk(chunk, chunk.offset + half, chunk.length - half);
            }
            if (*chunk.pending <= 0)
                emit memoryDataReceived(chunk.requestId, chunk.base, *chunk.accumulator);
            return;
        }
        if (const auto peripheral = m_peripheralRequests.take(token); peripheral.requestId) {
            quint32 value = 0;
            memcpy(&value, data.constData(), qMin(data.size(), qsizetype(sizeof(value))));
            GdbMi result;
            result.m_type = GdbMi::Tuple;
            result.addChild(constMi("address", QString::number(peripheral.address)));
            result.addChild(constMi("value", QString::number(value)));
            emit refreshDataReceived(peripheral.requestId, RefreshKind::PeripheralRegisters,
                                     result);
        } else {
            emit memoryDataReceived(token, ok ? address : 0, data);
        }
    } else if (command == "qtc/disassemble") {
        const QJsonObject body = response.value("body").toObject();
        const auto request = m_disassemblyRequests.take(
            quint64(body.value("token").toDouble()));
        if (request.requestId == 0)
            return;
        const DisassemblerLines lines = parseCliDisassembly(body.value("text").toString());
        const bool usable = request.address ? lines.coversAddress(request.address)
                                            : !lines.data().isEmpty();
        if (usable) {
            emit disassemblyReceived(request.requestId, lines);
        } else if (request.address && !request.target.contains(',')) {
            // Disassembling the function the address is in did not cover it,
            // so ask for a window around it instead.
            fetchDisassemblyForTarget(request.requestId, request.address,
                                      "0x" + QString::number(request.address - 20, 16) + ",0x"
                                          + QString::number(request.address + 100, 16));
        } else {
            emit message("BridgeImpl: no usable disassembly for " + request.target, LogWarning);
        }
    } else if (command == "qtc/fetchSymbols") {
        const QJsonObject body = response.value("body").toObject();
        GdbMi symbolList;
        symbolList.m_type = GdbMi::List;
        symbolList.m_name = "symbols";
        for (const QJsonValue &value : body.value("symbols").toArray()) {
            const QJsonObject item = value.toObject();
            GdbMi symbol;
            symbol.m_type = GdbMi::Tuple;
            symbol.addChild(constMi("state", item.value("state").toString()));
            symbol.addChild(constMi("address", item.value("address").toString()));
            symbol.addChild(constMi("name", item.value("name").toString()));
            symbol.addChild(constMi("section", item.value("section").toString()));
            symbol.addChild(constMi("demangled", item.value("demangled").toString()));
            symbolList.addChild(symbol);
        }
        GdbMi result;
        result.m_type = GdbMi::Tuple;
        result.addChild(constMi("modulepath", body.value("module").toString()));
        result.addChild(symbolList);
        emit refreshDataReceived(m_pendingSymbolsRequestId, RefreshKind::ModuleSymbols, result);
    } else if (command == "terminate" || command == "disconnect") {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
    } else if (command == "qtc/executeCommand") {
        const QString output = response.value("body").toObject().value("output").toString();
        if (!output.isEmpty())
            emit message(output, LogOutput);
    } else if (command == "qtc/enableSubBreakpoint") {
        handleBreakpointResponse(BreakpointOp::EnableSub, response);
    } else if (command == "qtc/insertBreakpoint") {
        handleBreakpointResponse(BreakpointOp::Insert, response);
    } else if (command == "qtc/updateBreakpoint") {
        handleBreakpointResponse(BreakpointOp::Update, response);
    } else if (command == "qtc/removeBreakpoint") {
        handleBreakpointResponse(BreakpointOp::Remove, response);
    }
}

void BridgeImpl::handleBreakpointResponse(BreakpointOp op, const QJsonObject &response)
{
    const QJsonObject body = response.value("body").toObject();
    const int modelId = body.value("modelid").toInt(-1);
    const quint64 requestId = m_breakpointRequestIds.take(modelId);
    const bool success = response.value("success").toBool();

    GdbMi data;
    if (const QString payload = body.value("bkpt").toString(); !payload.isEmpty()) {
        GdbMi bkpt;
        QStringDecoder decoder(QStringDecoder::Utf8);
        bkpt.fromString(payload, decoder);
        // A list of breakpoints, as the interface reports them: one insert can
        // yield several, and the reader iterates.
        data.m_type = GdbMi::List;
        data.addChild(bkpt);
    }
    emit breakpointEvent(requestId, op, success, data);
}

void BridgeImpl::handleStackTrace(const QJsonObject &response)
{
    const StackTraceRequest request
        = m_stackTraceRequests.take(response.value("request_seq").toInt());
    const QJsonArray frames = response.value("body").toObject().value("stackFrames").toArray();

    if (request.reportsStop) {
        const QJsonObject top = frames.isEmpty() ? QJsonObject() : frames.first().toObject();
        const int lineNumber = top.value("line").toInt();
        const FilePath fileName
            = FilePath::fromUserInput(top.value("source").toObject().value("path").toString());
        if (lineNumber != 0 && fileName.exists())
            emit locationChanged(fileName, lineNumber);
        reportStop();
        return;
    }

    GdbMi frameList;
    frameList.m_name = "frames";
    frameList.m_type = GdbMi::List;
    int level = 0;
    for (const QJsonValue &value : frames) {
        const QJsonObject item = value.toObject();
        GdbMi frame;
        frame.m_type = GdbMi::Tuple;
        const auto add = [&frame](const QString &name, const QString &data) {
            GdbMi child;
            child.m_type = GdbMi::Const;
            child.m_name = name;
            child.m_data = data;
            frame.addChild(child);
        };
        add("level", QString::number(level++));
        add("func", item.value("name").toString());
        const QString path = item.value("source").toObject().value("path").toString();
        add("file", path);
        add("fullname", path);
        add("line", QString::number(item.value("line").toInt()));
        add("addr", QString::number(item.value("instructionPointerReference").toInteger()));
        frameList.addChild(frame);
    }
    GdbMi stack;
    stack.m_type = GdbMi::Tuple;
    stack.m_name = "stack";
    stack.addChild(frameList);
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(stack);

    emit refreshDataReceived(request.refreshRequestId, RefreshKind::FullStack, all);
}

void BridgeImpl::handleEvent(DapEventType type, const QJsonObject &event)
{
    switch (type) {
    case DapEventType::Initialized:
        m_client->sendConfigurationDone();
        return;
    case DapEventType::Stopped:
        handleStopped(event);
        return;
    case DapEventType::Exited: {
        InferiorResultData result;
        result.exitCode = event.value("body").toObject().value("exitCode").toInt();
        m_inferiorRunning = false;
        emit inferiorDone(result);
        return;
    }
    case DapEventType::Output: {
        const QJsonObject body = event.value("body").toObject();
        const QString category = body.value("category").toString();
        emit message(body.value("output").toString(),
                     category == "stderr" ? AppError : AppOutput);
        return;
    }
    default:
        // An unmapped DAP event still arrives whole: the bridge announces the
        // debuggee's pid this way, which several views and the interrupt path need.
        if (event.value("event").toString() == "qtc/breakpointModified") {
            const QString payload = event.value("body").toObject().value("bkpt").toString();
            GdbMi bkpt;
            QStringDecoder decoder(QStringDecoder::Utf8);
            bkpt.fromString(payload, decoder);
            GdbMi list;
            list.m_type = GdbMi::List;
            list.addChild(bkpt);
            emit breakpointModified(list);
            return;
        }
        if (event.value("event").toString() == "process") {
            const qint64 pid = event.value("body").toObject()
                                   .value("systemProcessId").toInteger();
            if (pid != 0)
                emit inferiorPidKnown(ProcessHandle(pid));
            return;
        }
        return;
    }
}

void BridgeImpl::handleStopped(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    m_currentThreadId = body.value("threadId").toInt();
    m_currentFrameId = 1;
    m_inferiorRunning = false;

    // Report the stop only once the location is known, as the other backends do.
    const int seq = m_client->stackTrace(m_currentThreadId);
    if (seq < 0) {
        reportStop();
        return;
    }
    m_stackTraceRequests.insert(seq, {true, 0});
}

void BridgeImpl::reportStop()
{
    emit inferiorEvent(m_stopRequested ? InferiorEvent::StopOk
                                       : InferiorEvent::SpontaneousStop);
    m_stopRequested = false;
}

void BridgeImpl::selectThread(const QString &threadId)
{
    m_currentThreadId = threadId.toInt();
}

void BridgeImpl::activateFrame(int index)
{
    // A DAP frame id, which the bridge hands out from 1 for the newest frame.
    m_currentFrameId = index + 1;
}

void BridgeImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                              const QByteArray &data)
{
    QTC_ASSERT(m_client, return);
    if (op == MemoryOp::Fetch) {
        MemoryRequest request;
        request.requestId = requestId;
        request.base = addr;
        request.accumulator = std::make_shared<QByteArray>(int(lengthOrSize), 0);
        request.pending = std::make_shared<int>(0);
        fetchMemoryChunk(request, 0, lengthOrSize);
    } else {
        postRequest("qtc/writeMemory",
                    QJsonObject{{"address", QString::number(addr)},
                                {"data", QString::fromUtf8(data.toBase64())}});
    }
}

void BridgeImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName)
{
    QTC_ASSERT(m_client, return);
    if (address == 0 && functionName.isEmpty()) {
        emit message("BridgeImpl::fetchDisassembly() needs an address or a function name",
                     LogWarning);
        return;
    }
    fetchDisassemblyForTarget(requestId, address,
                              address ? "0x" + QString::number(address, 16) : functionName);
}

void BridgeImpl::fetchDisassemblyForTarget(quint64 requestId, quint64 address,
                                           const QString &target)
{
    const quint64 token = ++m_nextDisassemblyToken;
    m_disassemblyRequests.insert(token, {requestId, address, target});
    postRequest("qtc/disassemble",
                QJsonObject{{"target", target}, {"token", qint64(token)}});
}

void BridgeImpl::executeDebuggerCommand(const QString &command, const WatchItemData &)
{
    QTC_ASSERT(m_client, return);
    postRequest("qtc/executeCommand", QJsonObject{{"command", command}});
}

void BridgeImpl::assignValueInDebugger(const WatchItemData &, const QString &expr,
                                       const QString &value)
{
    QTC_ASSERT(m_client, return);
    postRequest("evaluate",
                QJsonObject{{"expression", QString(expr + '=' + value)},
                            {"frameId", m_currentFrameId}});
}

void BridgeImpl::setRegisterValue(const QString &name, const QString &value)
{
    QTC_ASSERT(m_client, return);
    postRequest("evaluate",
                QJsonObject{{"expression", QString('$' + name + '=' + value)},
                            {"frameId", m_currentFrameId}});
}

void BridgeImpl::fetchMemoryChunk(const MemoryRequest &request, quint64 offset, quint64 length)
{
    const quint64 token = ++m_nextMemoryToken;
    MemoryRequest chunk = request;
    chunk.offset = offset;
    chunk.length = length;
    ++*chunk.pending;
    m_memoryRequests.insert(token, chunk);
    postRequest("qtc/readMemory",
                QJsonObject{{"address", QString::number(request.base + offset)},
                            {"length", qint64(length)},
                            {"token", qint64(token)}});
}

void BridgeImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    QTC_ASSERT(m_client, return);
    const quint32 word = quint32(value);
    const QByteArray data(reinterpret_cast<const char *>(&word), sizeof(word));
    postRequest("qtc/writeMemory",
                QJsonObject{{"address", QString::number(address)},
                            {"data", QString::fromUtf8(data.toBase64())}});
}

void BridgeImpl::watchPoint(quint64 requestId, const QPoint &)
{
    emit watchPointResolved(requestId, 0, {});
}

void BridgeImpl::createSnapshot(quint64 requestId)
{
    emit snapshotCreated(requestId, false, {});
}

} // namespace Debugger::Internal
