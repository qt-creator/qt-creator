// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgeimpl.h"

#include "../debuggertr.h"

#include "../debuggerinternalconstants.h"
#include "../disassemblerlines.h"
#include "../watchutils.h"

#include "../dap/dapclient.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QStringDecoder>

#include <cstring>

using namespace Utils;

namespace Debugger::Internal {

BridgeStartData dapHostRecipe(bool loadInitFile)
{
    BridgeStartData recipe;
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
    BridgeImplDataProvider(const ProcessRunData &runData, const CommandLine &cmd,
                           const QString &runAsUser, QObject *parent)
        : IDataProvider(parent)
        , m_runData(runData)
        , m_cmd(cmd)
        , m_runAsUser(runAsUser)
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
        m_proc.setRunAsUser(m_runAsUser);
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
    qint64 processId() const { return m_proc.processId(); }

private:
    Process m_proc;
    const ProcessRunData m_runData;
    const CommandLine m_cmd;
    const QString m_runAsUser;
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
                      | WatchWidgetsCapability
                      | ReloadModuleCapability | ReloadModuleSymbolsCapability
                      | BreakConditionCapability | BreakIndividualLocationsCapability
                      | BreakOnThrowAndCatchCapability
                      | RunToLineCapability | JumpToLineCapability
                      | WatchpointByAddressCapability | WatchpointByExpressionCapability
                      | ResetInferiorCapability
                      | ReturnFromFunctionCapability | ReverseSteppingCapability
                      | SnapshotCapability
                      | TracePointCapability;
    data.extraCapabilities = DebuggerExtraCapability::BreakOnMain
                           | DebuggerExtraCapability::ContinueAfterAttach
                           | DebuggerExtraCapability::ContinueInsteadOfRun
                           | DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::ExitMonitorAtClose
                           | DebuggerExtraCapability::JumpTargetCheck
                           | DebuggerExtraCapability::LibraryEvent
                           | DebuggerExtraCapability::PeripheralRegisters
                           | DebuggerExtraCapability::RunAsUser
                           | DebuggerExtraCapability::RunCommandDeferral
                           | DebuggerExtraCapability::SignalReceived
                           | DebuggerExtraCapability::SkipKnownFrames
                           | DebuggerExtraCapability::SpecialBreakpoints
                           | DebuggerExtraCapability::ThreadEvent
                           | DebuggerExtraCapability::SourceFiles
                           | DebuggerExtraCapability::Threads;
    data.startModes = DebuggerStartModeFlag::Launch | DebuggerStartModeFlag::AttachToProcess
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

BridgeImpl::BridgeImpl(const DapStartData &startData)
    : DebuggerEngineInterface(bridgeImplSetupData())
    , m_startData(startData)
{
    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        if (m_pendingRequests.isEmpty())
            return;
        m_watchdog.start();
        QStringList pending;
        for (const PendingRequest &request : std::as_const(m_pendingRequests))
            pending.append(request.text);
        emit notResponding(m_startData.watchdogTimeout, pending,
                           m_debuginfodDownloadInProgress
                               ? NotRespondingCause::FetchingDebugInfo
                               : NotRespondingCause::Unknown);
    });
}

BridgeImpl::~BridgeImpl() = default;

void BridgeImpl::start()
{
    CommandLine cmd{m_startData.debuggerRunData.command.executable(),
                    m_startData.bridgeStartData.startupArguments};
    // A gdb that was built but never installed carries its own python
    // modules beside the binary and does not find them by itself.
    const FilePath uninstalledData
        = m_startData.debuggerRunData.command.executable().parentDir() / "data-directory/python";
    if (uninstalledData.exists())
        cmd.addArgs({"-iex", "python sys.path.append('" + uninstalledData.path() + "')"});
    cmd.addArgs({"-iex", "python sys.path.insert(1, '" + m_startData.dumperScriptsDir.path() + "')"});
    cmd.addArgs({"-iex", "python from " + m_startData.bridgeStartData.bridgeModule + " import *"});
    cmd.addArgs({"-ex", "python " + m_startData.bridgeStartData.serverCall});

    auto provider = new BridgeImplDataProvider(m_startData.debuggerRunData, cmd,
                                               m_startData.runAsUser, this);
    m_client = new BridgeImplClient(provider, this);

    connect(m_client, &DapClient::requestSent, this, &BridgeImpl::logRequest);
    connect(m_client, &DapClient::started, this, &BridgeImpl::handleStarted);
    connect(m_client, &DapClient::done, this, &BridgeImpl::handleFinished);
    connect(m_client, &DapClient::readyReadStandardError, this, &BridgeImpl::handleStandardError);
    connect(m_client, &DapClient::unframedOutput,
            this, [this](const QString &text) { emit message(text, LogOutput); });
    connect(m_client, &DapClient::responseReady, this, &BridgeImpl::handleResponse);
    connect(m_client, &DapClient::eventReady, this, &BridgeImpl::handleEvent);

    emit message(cmd.toUserOutput(), LogInput);
    provider->start();
}

void BridgeImpl::handleStarted()
{
    // Not sendInitialize(): the user's extra dumpers have to travel with it,
    // because the bridge sets them up while answering.
    QJsonObject args{{"clientID", "QtCreator"}, {"clientName", "QtCreator"},
                     {"adapterID", m_startData.bridgeStartData.bridgeModule}};
    QJsonArray dumperFiles;
    for (const FilePath &file : m_startData.extraDumperFiles) {
        if (file.isReadableFile())
            dumperFiles.append(file.path());
    }
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
    if (m_startData.useDebugInfoD)
        args.insert("debuginfod", *m_startData.useDebugInfoD);
    if (!m_startData.debugInfoLocation.isEmpty() && m_startData.debugInfoLocation.exists())
        args.insert("debugInfoLocation", m_startData.debugInfoLocation.path());
    if (!m_startData.solibSearchPath.isEmpty()) {
        QJsonArray solibSearchPath;
        for (const FilePath &path : m_startData.solibSearchPath)
            solibSearchPath.append(path.path());
        args.insert("solibSearchPath", solibSearchPath);
    }
    if (m_startData.loadSystemDumpers)
        args.insert("systemDumpers", true);
    if (m_startData.useIndexCache)
        args.insert("indexCache", true);
    if (m_startData.multiInferior)
        args.insert("multiInferior", true);
    if (!args.isEmpty())
        postRequest("qtc/configureTarget", args);
}

void BridgeImpl::createSpecialBreakpoints()
{
    // A core file has nothing to break in.
    if (std::holds_alternative<AttachToCoreData>(m_startData.inferiorStartData))
        return;
    if (!m_startData.breakOnAbort && !m_startData.breakOnWarning && !m_startData.breakOnFatal)
        return;
    postRequest("qtc/createSpecialBreakpoints",
                QJsonObject{{"breakonabort", m_startData.breakOnAbort},
                            {"breakonwarning", m_startData.breakOnWarning},
                            {"breakonfatal", m_startData.breakOnFatal}});
}

void BridgeImpl::runPostAttachCommands()
{
    if (m_startData.userCommands.afterAttach.isEmpty())
        return;
    postRequest("qtc/runUserCommands",
                QJsonObject{{"commands", m_startData.userCommands.afterAttach}});
}

void BridgeImpl::runUserStartupCommands()
{
    const DebuggerUserCommands &commands = m_startData.userCommands;
    if (!commands.startScript.isEmpty()) {
        if (commands.startScript.isReadableFile()) {
            postRequest("qtc/runUserCommands",
                        QJsonObject{{"script", commands.startScript.path()}});
        } else {
            emit message("The debugger start script is not accessible: "
                             + commands.startScript.toUserOutput(), LogWarning);
        }
        return;
    }
    if (!commands.atStartup.isEmpty())
        postRequest("qtc/runUserCommands", QJsonObject{{"commands", commands.atStartup}});
}

void BridgeImpl::handleFinished()
{
    m_watchdog.stop();
    auto provider = static_cast<BridgeImplDataProvider *>(m_client->dataProvider());
    // A host that is gone without ever having answered leaves the session
    // unopened, whether it failed to start or quit on its own.
    reportEngineSetup(false);
    emit engineProcessFinished(provider->resultData());
}

// The setup is over once the host has answered for itself, not when its
// process is up: everything the session needs arrives with that answer.
void BridgeImpl::reportEngineSetup(bool success)
{
    if (m_setupReported)
        return;
    m_setupReported = true;
    emit inferiorEvent(success ? InferiorEvent::EngineSetupOk
                              : InferiorEvent::EngineSetupFailed);
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

// What the dumpers know, reported once with the initialize answer.
static GdbMi dumperTypesOf(const QJsonObject &response)
{
    const QString payload = response.value("body").toObject().value("qtcDumpers").toString();
    if (payload.isEmpty())
        return {};
    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi result;
    result.fromString('{' + payload + '}', decoder);
    return result;
}

void BridgeImpl::handleStandardError()
{
    const QString error = m_client->dataProvider()->readAllStandardError();
    if (error.isEmpty())
        return;
    // gdb announces a debug info download with one line and then fetches
    // silently, which looks exactly like a debugger that stopped answering.
    // Its console is pointed at the error channel here, so this is where the
    // announcement arrives.
    if (error.contains("Downloading") && error.contains("separate debug info"))
        m_debuginfodDownloadInProgress = true;
    emit message(error, LogError);
}

void BridgeImpl::postLaunchOrAttach()
{
    if (const auto attach = std::get_if<AttachToProcessData>(&m_startData.inferiorStartData)) {
        postRequest("attach", QJsonObject{{"pid", qint64(attach->pid.pid())}});
        return;
    }

    if (const auto stub = std::get_if<AttachToTerminalStubData>(&m_startData.inferiorStartData)) {
        postRequest("attach", QJsonObject{{"pid", qint64(stub->pid.pid())}});
        return;
    }

    if (const auto remote = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData)) {
        QJsonObject args{{"channel", remote->channel}};
        if (!remote->symbolFile.isEmpty())
            args.insert("symbolFile", remote->symbolFile.path());
        if (remote->attachPid.isValid())
            args.insert("attachPid", qint64(remote->attachPid.pid()));
        if (!remote->remoteExecutable.isEmpty())
            args.insert("remoteExecutable", remote->remoteExecutable.nativePath());
        const QStringList &afterConnect = m_startData.userCommands.afterConnect;
        if (!afterConnect.isEmpty())
            args.insert("commandsAfterConnect", afterConnect.join('\n'));
        postRequest("qtc/attachToRemoteServer", args);
        return;
    }

    if (const auto core = std::get_if<AttachToCoreData>(&m_startData.inferiorStartData)) {
        postRequest("qtc/attachToCore",
                    QJsonObject{{"coreFile", core->coreFile.nativePath()},
                                {"executable", core->executable.nativePath()}});
        return;
    }

    const auto runData = std::get_if<ProcessRunData>(&m_startData.inferiorStartData);
    QTC_ASSERT(runData, emit inferiorEvent(InferiorEvent::EngineRunFailed); return);

    QJsonArray inferiorArguments;
    for (const QString &argument : runData->command.splitArguments())
        inferiorArguments.append(argument);
    QJsonObject args{{"noDebug", false},
                     {"program", runData->command.executable().path()},
                     {"args", inferiorArguments}};
    if (!runData->workingDirectory.isEmpty())
        args.insert("cwd", runData->workingDirectory.path());
    if (m_startData.breakOnMain) {
        args.insert("stopAtMain", true);
        args.insert("mainFunction", m_startData.mainFunctionName);
    }

    Environment inferiorEnv = runData->environment;
    if (m_startData.enableHeapDebugging != TriState::Default
        && !inferiorEnv.hasKey(Constants::NO_DEBUG_HEAP)) {
        inferiorEnv.set(Constants::NO_DEBUG_HEAP,
                        m_startData.enableHeapDebugging == TriState::Enabled ? "0" : "1");
    }
    Environment debuggerEnv = m_startData.debuggerRunData.environment;
    debuggerEnv.setupEnglishOutput();
    QJsonArray env;
    for (const EnvironmentItem &item : debuggerEnv.diff(inferiorEnv)) {
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
    m_shuttingDown = true;
    // Answered only once the inferior is really gone: reporting it earlier
    // lets the engine shut the bridge down from under the kill.
    if (m_startData.exitMonitorAtClose) {
        // A monitor is shut down over the connection, so the host has to
        // outlive the inferior here: a terminate takes it with it.
        postRequest("qtc/shutdownInferior",
                    QJsonObject{{"detach", mode == ShutdownMode::Detach}});
    } else if (mode == ShutdownMode::Detach) {
        postRequest("disconnect", QJsonObject{{"restart", false}, {"terminateDebuggee", false}});
    } else {
        postRequest("terminate", QJsonObject{{"restart", false}});
    }
    // The bridge sits inside the command that resumed the inferior and reads
    // nothing while it runs, so the request above needs a stop to be seen.
    if (m_inferiorRunning)
        interruptGdb();
}

void BridgeImpl::shutdownEngine()
{
    if (m_client && m_startData.exitMonitorAtClose && !m_monitorExitRequested) {
        // The monitor answers over the connection the terminate below ends,
        // so the shutdown continues once it is gone.
        m_monitorExitRequested = true;
        postRequest("qtc/exitMonitor");
        return;
    }
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
        m_resumePending = true;
        m_stepRequested = false;
        // The engine leaves the stopped state on the request, not on the
        // answer: a refusal has nowhere to go back to otherwise.
        emit inferiorEvent(InferiorEvent::RunRequested);
        if (m_startData.nativeMixedDebugging && request.currentFrameIsQml) {
            postInterpreterStep("executeContinue");
            return;
        }
        if (request.reverse)
            postRequest("reverseContinue", QJsonObject{{"threadId", m_currentThreadId}});
        else
            m_client->sendContinue(m_currentThreadId);
        return;
    case ExecutionCommand::Interrupt:
        if (m_resumePending) {
            m_interruptOnceRunning = true;
            return;
        }
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            return;
        }
        interruptInferior();
        return;
    case ExecutionCommand::StepIn:
        m_resumePending = true;
        m_stepRequested = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        if (m_startData.nativeMixedDebugging && !request.flag) {
            if (request.currentFrameIsQml) {
                postInterpreterStep("executeStep");
                return;
            }
            // Leaving C++ for QML: the interpreter has to be told to pause at
            // the next JS statement while the native step runs.
            QJsonObject arguments = stepArguments(false);
            arguments["arminterpreter"] = true;
            postRequest("stepIn", arguments);
            return;
        }
        postRequest(request.reverse ? QLatin1String("qtc/reverseStepIn")
                                    : QLatin1String("stepIn"),
                    stepArguments(request.flag));
        return;
    case ExecutionCommand::StepOver:
        m_resumePending = true;
        m_stepRequested = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        if (m_startData.nativeMixedDebugging && !request.flag) {
            if (request.currentFrameIsQml) {
                postInterpreterStep("executeNext");
                return;
            }
            // A step over in C++ goes through the dumpers: standing in the
            // metacall trampolines a C++ method was called from QML through,
            // the next line of the program is back in QML.
            if (!request.reverse) {
                postInterpreterStep("executeNativeMixedNext");
                return;
            }
        }
        // Stepping back over a line is stock DAP, the other two directions are
        // not covered by the protocol.
        postRequest(request.reverse ? QLatin1String("stepBack") : QLatin1String("next"),
                    stepArguments(request.flag));
        return;
    case ExecutionCommand::StepOut:
        m_resumePending = true;
        m_stepRequested = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        if (m_startData.nativeMixedDebugging && !request.reverse) {
            // Out of a QML frame, or out of a C++ frame the interpreter
            // called: either way the QML caller is where this ends.
            postInterpreterStep(request.currentFrameIsQml
                                    ? QLatin1String("executeStepOut")
                                    : QLatin1String("executeNativeMixedStepOut"));
            return;
        }
        if (request.reverse)
            postRequest("qtc/reverseStepOut", QJsonObject{{"threadId", m_currentThreadId}});
        else
            m_client->sendStepOut(m_currentThreadId);
        return;
    case ExecutionCommand::RunToLine:
        m_stepRequested = false;
        m_resumePending = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        postRequest("qtc/runToLine",
                    QJsonObject{{"file", request.context.fileName.path()},
                                {"line", request.context.textPosition.line}});
        return;
    case ExecutionCommand::RunToFunction:
        m_stepRequested = false;
        m_resumePending = true;
        emit inferiorEvent(InferiorEvent::RunRequested);
        postRequest("qtc/runToFunction",
                    QJsonObject{{"function", request.functionName}});
        return;
    case ExecutionCommand::JumpToLine:
        postRequest("qtc/jumpToLine",
                    QJsonObject{{"file", request.context.fileName.path()},
                                {"line", request.context.textPosition.line}});
        return;
    case ExecutionCommand::Detach:
        // Not DapClient::sendDisconnect(), which takes the debuggee with it.
        m_detaching = true;
        postRequest("disconnect", QJsonObject{{"restart", false}, {"terminateDebuggee", false}});
        if (m_inferiorRunning)
            interruptGdb();
        return;
    case ExecutionCommand::Abort:
        // The abort is what a debugger that stopped answering is left with, so
        // asking it to terminate itself would wait for the reply that is not
        // coming.
        m_client->dataProvider()->kill();
        return;
    case ExecutionCommand::RepeatLastCommand:
        if (!m_lastDebuggableCommand.isEmpty())
            postRequest(m_lastDebuggableCommand, m_lastDebuggableArguments);
        return;
    case ExecutionCommand::ResetInferior:
        emit inferiorEvent(InferiorEvent::RunRequested);
        if (!m_startData.userCommands.forReset.isEmpty()) {
            postRequest("qtc/runUserCommands",
                        QJsonObject{{"commands", m_startData.userCommands.forReset.join('\n')}});
        }
        postRequest("restart");
        return;
    case ExecutionCommand::Return:
        emit inferiorEvent(InferiorEvent::RunRequested);
        postRequest("qtc/return");
        return;
    case ExecutionCommand::RecordReverse: {
        // Process record is gdb's own, the protocol has no shape for it.
        const QString command = request.flag ? QLatin1String("record full")
                                             : QLatin1String("record stop");
        postRequest("qtc/executeCommand", QJsonObject{{"command", command}});
        return;
    }
    }
}

int BridgeImpl::postRequest(const QString &command, const QJsonObject &arguments)
{
    QTC_ASSERT(m_client, return -1);
    return m_client->postRequest(command, arguments);
}

void BridgeImpl::logRequest(int seq, const QString &command, const QJsonObject &arguments)
{
    const QString text = QString::number(seq) + command + '('
                         + QString::fromUtf8(
                             QJsonDocument(arguments).toJson(QJsonDocument::Compact))
                         + ')';
    m_pendingRequests.insert(seq, {text, command, QDateTime::currentMSecsSinceEpoch()});
    restartWatchdog();
    emit message(text, LogInput);
}

void BridgeImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_pendingRequests.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}

void BridgeImpl::postWhenStopped(const QString &command, const QJsonObject &arguments,
                                 const BreakpointChangeRequest &request)
{
    if (!m_inferiorRunning && !m_resumePending) {
        postRequest(command, arguments);
        return;
    }
    const bool needsStop = m_deferredRequests.isEmpty();
    m_deferredRequests.append({command, arguments, request.requestId, request.op});
    if (needsStop) {
        m_deferredStopRequested = true;
        execute({ExecutionCommand::Interrupt});
    }
}

void BridgeImpl::failDeferredRequests()
{
    m_deferredStopRequested = false;
    const QList<DeferredRequest> requests = std::exchange(m_deferredRequests, {});
    for (const DeferredRequest &request : requests)
        emit breakpointEvent(request.requestId, request.op, false);
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
                     {"threadspec", params.threadSpec},
                     {"condition", QString::fromUtf8(params.condition.toUtf8().toHex())},
                     {"command", QString::fromUtf8(params.command.toUtf8().toHex())},
                     {"function", params.type == BreakpointAtMain
                                      ? m_startData.mainFunctionName : params.functionName},
                     {"oneshot", params.oneShot},
                     {"enabled", params.enabled},
                     {"line", params.textPosition.line},
                     {"address", qint64(params.address)},
                     {"expression", params.expression},
                     {"tracepoint", params.tracepoint},
                     {"message", QString::fromUtf8(params.message.toUtf8().toHex())},
                     {"file", params.fileNameForDebugger().path()}};

    if (params.isTracepoint() && params.type == BreakpointByFileAndLine) {
        args["pseudotracepoint"] = m_startData.pseudoTracepoints;
        if (m_startData.pseudoTracepoints) {
            const QList<TracepointCapture> captures = parseTracepointCaptures(params.message);
            QJsonArray caps;
            for (const TracepointCapture &capture : captures) {
                caps.append(QJsonArray{int(capture.type),
                                       capture.expression.isEmpty()
                                           ? QJsonValue(QJsonValue::Null)
                                           : QJsonValue(capture.expression)});
            }
            args["caps"] = caps;
            m_tracepoints[change.modelId] = {params.message, captures};
        }
    }

    postWhenStopped(request, args, change);
}

void BridgeImpl::postInterpreterStep(const QString &function)
{
    postRequest("qtc/interpreterStep", QJsonObject{{"function", function}});
}

void BridgeImpl::postInterpreterBreakpointRequest(const BreakpointChangeRequest &change)
{
    const BreakpointParameters &params = change.params;
    m_breakpointRequestIds.insert(change.modelId, change.requestId);
    postWhenStopped("qtc/insertInterpreterBreakpoint",
                    QJsonObject{{"modelid", change.modelId},
                                {"file", params.fileName.path()},
                                {"line", params.textPosition.line},
                                {"enabled", params.enabled},
                                {"condition",
                                 QString::fromUtf8(params.condition.toUtf8().toHex())},
                                {"ignorecount", params.ignoreCount}},
                    change);
}

void BridgeImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    QTC_ASSERT(m_client, return);

    switch (request.op) {
    case BreakpointOp::Insert:
        if (!request.params.isCppBreakpoint()) {
            postInterpreterBreakpointRequest(request);
            return;
        }
        postBreakpointRequest("qtc/insertBreakpoint", request);
        return;
    case BreakpointOp::Update:
        if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Update, false);
            return;
        }
        // The service knows no change command, and the numbers it hands out
        // are not the debugger's: taking the breakpoint away and setting it
        // anew is what a change is there. Handing the debugger the
        // interpreter's number instead rewrites whatever breakpoint of its
        // own carries it, the hook into the service included.
        if (!request.params.isCppBreakpoint()) {
            m_interpreterBreakpointChanges.insert(request.modelId);
            postWhenStopped("qtc/removeInterpreterBreakpoint",
                            QJsonObject{{"modelid", request.modelId},
                                        {"id", request.responseId}},
                            request);
            postInterpreterBreakpointRequest(request);
            return;
        }
        postBreakpointRequest("qtc/updateBreakpoint", request);
        return;
    case BreakpointOp::Remove:
        m_breakpointRequestIds.insert(request.modelId, request.requestId);
        m_tracepoints.remove(request.modelId);
        if (!request.params.isCppBreakpoint()) {
            postWhenStopped("qtc/removeInterpreterBreakpoint",
                            QJsonObject{{"modelid", request.modelId},
                                        {"id", request.responseId}},
                            request);
            return;
        }
        postWhenStopped("qtc/removeBreakpoint",
                        QJsonObject{{"modelid", request.modelId},
                                    {"id", request.responseId}},
                        request);
        return;
    case BreakpointOp::EnableSub:
        m_breakpointRequestIds.insert(request.modelId, request.requestId);
        postWhenStopped("qtc/enableSubBreakpoint",
                        QJsonObject{{"modelid", request.modelId},
                                    {"id", request.subResponseId},
                                    {"enabled", request.enabled}},
                        request);
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
        cmd.arg("uninitialized", request.uninitializedVariables);
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
        // Native mixed puts the QML frames of the engine into the stack, and
        // only the stack the dumpers walk knows about those.
        if (m_startData.nativeMixedDebugging) {
            postDumperStack(request, false);
            return;
        }
        if (const int seq = m_client->stackTrace(m_currentThreadId,
                                                 qMax(request.stackDepthLimit, 0));
            seq >= 0) {
            m_stackTraceRequests.insert(seq, {false, request.requestId});
        }
        return;
    case RefreshKind::Registers:
        m_pendingRegistersRequestId = request.requestId;
        postRequest("qtc/fetchRegisters", QJsonObject{{"frameId", m_currentFrameId}});
        return;
    case RefreshKind::Threads:
        m_pendingThreadsRequestId = request.requestId;
        postRequest("qtc/fetchThreads", {});
        return;
    case RefreshKind::SourceFiles:
        m_pendingSourceFilesRequestId = request.requestId;
        postRequest("qtc/fetchSourceFiles", {});
        return;
    case RefreshKind::FullBacktrace:
        m_pendingBacktraceRequestId = request.requestId;
        postRequest("qtc/fetchFullBacktrace", {});
        return;
    case RefreshKind::QmlStack:
        // The adapter's own stack trace knows the native frames only, so the
        // one the dumpers walk is asked for instead: it reads the QML stack out
        // of the engine and puts those frames in front of the native ones.
        postDumperStack(request, true);
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
    case RefreshKind::ModuleSections:
        m_pendingSectionsRequestId = request.requestId;
        postRequest("qtc/fetchSections", QJsonObject{{"module", request.path.path()}});
        return;
    case RefreshKind::AllSymbols:
        postRequest("qtc/loadSymbols", QJsonObject{{"all", true}});
        refresh({request.requestId, RefreshKind::Modules});
        refresh({request.requestId, RefreshKind::FullStack});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    case RefreshKind::StackSymbols:
        postRequest("qtc/loadSymbols",
                    QJsonObject{{"module", request.path.path()}});
        return;
    case RefreshKind::DebuggingHelpers:
        m_pendingDumpersRequestId = request.requestId;
        postRequest("qtc/reloadDumpers", {});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    default:
        emit refreshDataReceived(request.requestId, request.kind, {});
        return;
    }
}

void BridgeImpl::postDumperStack(const RefreshRequest &request, bool extraQml)
{
    m_pendingDumperStackRequestId = request.requestId;
    DebuggerCommand cmd;
    cmd.arg("limit", request.stackDepthLimit);
    cmd.arg("nativemixed", m_startData.nativeMixedDebugging);
    if (extraQml)
        cmd.arg("extraqml", true);
    postRequest("qtc/fetchStack", cmd.args.toObject());
}

static int modelIdOf(const QJsonObject &response)
{
    return response.value("body").toObject().value("modelid").toInt(-1);
}

void BridgeImpl::handleResponse(DapResponseType type, const QJsonObject &response)
{
    const QString command = response.value("command").toString();
    const bool success = response.value("success").toBool();
    const PendingRequest answered = m_pendingRequests.take(response.value("request_seq").toInt());
    if (m_startData.logTimeStamps && answered.postTime) {
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - answered.postTime;
        emit message(QString("Response time: %1: %2 s").arg(answered.command)
                         .arg(elapsed / 1000.), LogTime);
    }
    m_debuginfodDownloadInProgress = false;
    restartWatchdog();

    switch (type) {
    case DapResponseType::Initialize: {
        if (!success) {
            emit message(response.value("message").toString(), LogError);
            reportEngineSetup(false);
            return;
        }
        reportEngineSetup(true);
        const GdbMi dumpers = dumperTypesOf(response);
        if (dumpers.isValid())
            emit refreshDataReceived(0, RefreshKind::DebuggingHelpers, dumpers);
        runUserStartupCommands();
        createSpecialBreakpoints();
        configureTarget();
        if (m_startData.nativeMixedDebugging)
            postRequest("qtc/setupNativeMixed", {});
        postLaunchOrAttach();
        return;
    }
    case DapResponseType::ConfigurationDone: {
        if (!success) {
            emit message(response.value("message").toString(), LogError);
            emit inferiorEvent(InferiorEvent::EngineRunFailed);
            return;
        }
        if (std::holds_alternative<AttachToCoreData>(m_startData.inferiorStartData)) {
            // A core is read, not run: the state it recorded ends the setup.
            m_reportsSetupStop = true;
            return;
        }
        const auto remote = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData);
        if (remote && remote->remoteExecutable.isEmpty()) {
            // The server hands its target over stopped, so the engine never
            // sees it run: the stop that follows is the end of the setup.
            m_reportsSetupStop = true;
            m_inferiorRunning = false;
            return;
        }
        if (std::holds_alternative<AttachToProcessData>(m_startData.inferiorStartData)
            || std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
            // Attaching stops the process, so that stop ends the setup too.
            m_reportsSetupStop = true;
            m_inferiorRunning = false;
            return;
        }
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
        m_inferiorRunning = true;
        return;
    }
    case DapResponseType::Continue:
        if (!success && response.value("message").toString() == "The program is not being run.") {
            m_resumePending = false;
            emit inferiorEvent(InferiorEvent::InferiorIll);
            return;
        }
        handleResumeResponse(success);
        return;
    case DapResponseType::StepIn:
    case DapResponseType::StepOut:
    case DapResponseType::StepOver:
        handleResumeResponse(success);
        return;
    case DapResponseType::StackTrace:
        handleStackTrace(response);
        return;
    case DapResponseType::Pause:
        if (!success) {
            failDeferredRequests();
            emit inferiorEvent(InferiorEvent::StopFailed);
        }
        return;
    case DapResponseType::Attach:
        if (success)
            runPostAttachCommands();
        Q_FALLTHROUGH();
    case DapResponseType::Launch:
        if (!success) {
            emit message(response.value("message").toString(), LogError);
            emit inferiorEvent(InferiorEvent::EngineRunFailed);
        }
        return;
    default:
        break;
    }

    // A fetch that failed carries the reason and no body, so the view it feeds
    // shows nothing at all: the reason reaches the log here or nowhere.
    if (!success && command.startsWith("qtc/fetch")) {
        emit message("BridgeImpl: " + command + " failed: "
                         + response.value("message").toString(), LogError);
    }

    if (command == "stepBack" || command == "reverseContinue"
        || command == "qtc/reverseStepIn" || command == "qtc/reverseStepOut"
        || command == "qtc/runToLine" || command == "qtc/runToFunction") {
        handleResumeResponse(success);
    } else if (command == "qtc/attachToCore" || command == "qtc/attachToRemoteServer") {
        if (success) {
            runPostAttachCommands();
        } else {
            emit message(response.value("message").toString(), LogError);
            emit inferiorEvent(InferiorEvent::EngineRunFailed);
        }
    } else if (command == "qtc/createSnapshot") {
        const SnapshotRequest request
            = m_snapshotRequests.take(response.value("request_seq").toInt());
        if (!success)
            emit message(response.value("message").toString(), LogError);
        emit snapshotCreated(request.requestId, success, request.filePath);
    } else if (command == "qtc/return") {
        if (success) {
            m_inferiorRunning = false;
            emit inferiorEvent(InferiorEvent::StopOk);
        } else {
            emit message(response.value("message").toString(), LogError);
            emit inferiorEvent(InferiorEvent::RunFailed);
        }
    } else if (command == "restart") {
        emit inferiorEvent(success ? InferiorEvent::RunOk : InferiorEvent::RunFailed);
        m_inferiorRunning = success;
    } else if (command == "qtc/reloadDumpers") {
        const GdbMi dumpers = dumperTypesOf(response);
        if (dumpers.isValid())
            emit refreshDataReceived(m_pendingDumpersRequestId, RefreshKind::DebuggingHelpers,
                                     dumpers);
    } else if (command == "qtc/fetchVariables") {
        emit refreshDataReceived(m_pendingLocalsRequestId, RefreshKind::Locals,
                                 dumperResultOf(response));
    } else if (command == "qtc/watchPoint") {
        const GdbMi result = dumperResultOf(response);
        emit watchPointResolved(m_pendingWatchPointRequestId, result["selected"].toAddress(),
                                result["expr"].data());
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
            item.addChild(constMi("groups", reg.value("groups").toString()));
            item.addChild(constMi("type", gdbRegisterTypeName(reg.value("type").toString())));
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
    } else if (command == "qtc/fetchSections") {
        if (!success)
            return;
        const QJsonObject body = response.value("body").toObject();
        GdbMi sectionList;
        sectionList.m_type = GdbMi::List;
        sectionList.m_name = "sections";
        for (const QJsonValue &value : body.value("sections").toArray()) {
            const QJsonObject item = value.toObject();
            GdbMi section;
            section.m_type = GdbMi::Tuple;
            section.addChild(constMi("from", item.value("from").toString()));
            section.addChild(constMi("to", item.value("to").toString()));
            section.addChild(constMi("address", item.value("address").toString()));
            section.addChild(constMi("name", item.value("name").toString()));
            section.addChild(constMi("flags", item.value("flags").toString()));
            sectionList.addChild(section);
        }
        GdbMi result;
        result.m_type = GdbMi::Tuple;
        result.addChild(constMi("modulepath", body.value("module").toString()));
        result.addChild(sectionList);
        emit refreshDataReceived(m_pendingSectionsRequestId, RefreshKind::ModuleSections, result);
    } else if (command == "qtc/loadSymbols") {
        if (!success)
            emit message("BridgeImpl: loading symbols failed: "
                         + response.value("message").toString(), LogError);
    } else if (command == "qtc/exitMonitor") {
        if (!success)
            emit message("BridgeImpl: shutting the debug monitor down failed: "
                         + response.value("message").toString(), LogError);
        shutdownEngine();
    } else if (command == "terminate" || command == "disconnect"
               || command == "qtc/shutdownInferior") {
        m_shuttingDown = false;
        // A detach ends the session too, but the engine did not ask for it and
        // hears about the debuggee the way it hears an exit.
        if (m_detaching) {
            m_inferiorRunning = false;
            failDeferredRequests();
            emit inferiorDone({0, InferiorExitStatus::Detached});
        } else {
            emit inferiorEvent(InferiorEvent::ShutdownFinished);
        }
    } else if (command == "qtc/fetchSourceFiles") {
        const GdbMi reported = dumperResultOf(response);
        GdbMi files;
        files.m_type = GdbMi::List;
        for (const GdbMi &item : reported["files"]) {
            const QString file = item["file"].data();
            if (file.endsWith("<built-in>"))
                continue;
            GdbMi entry;
            entry.m_type = GdbMi::Tuple;
            entry.addChild(constMi("file", file));
            if (const GdbMi fullName = item["fullname"]; fullName.isValid())
                entry.addChild(constMi("fullname", fullName.data()));
            files.addChild(entry);
        }
        emit refreshDataReceived(m_pendingSourceFilesRequestId, RefreshKind::SourceFiles, files);
    } else if (command == "qtc/fetchStack") {
        emit refreshDataReceived(m_pendingDumperStackRequestId, RefreshKind::FullStack,
                                 dumperResultOf(response));
    } else if (command == "qtc/fetchThreads") {
        emit refreshDataReceived(m_pendingThreadsRequestId, RefreshKind::Threads,
                                 dumperResultOf(response));
    } else if (command == "qtc/fetchFullBacktrace") {
        emit refreshDataReceived(m_pendingBacktraceRequestId, RefreshKind::FullBacktrace,
                                 constMi({}, response.value("body").toObject()
                                                 .value("output").toString()));
    } else if (command == "qtc/executeCommand") {
        const QJsonObject body = response.value("body").toObject();
        if (const QString output = body.value("output").toString(); !output.isEmpty())
            emit message(output, LogOutput);
        if (const QString error = body.value("error").toString(); !error.isEmpty())
            emit message(error, LogError);
    } else if (command == "qtc/enableSubBreakpoint") {
        handleBreakpointResponse(BreakpointOp::EnableSub, response);
    } else if (command == "qtc/insertBreakpoint"
               || command == "qtc/insertInterpreterBreakpoint") {
        // An insertion that puts a changed interpreter breakpoint back is
        // what answers the change.
        const bool isChange = m_interpreterBreakpointChanges.remove(modelIdOf(response));
        handleBreakpointResponse(isChange ? BreakpointOp::Update : BreakpointOp::Insert,
                                 response);
    } else if (command == "qtc/updateBreakpoint") {
        handleBreakpointResponse(BreakpointOp::Update, response);
    } else if (command == "qtc/removeBreakpoint"
               || command == "qtc/removeInterpreterBreakpoint") {
        // The removal half of an interpreter breakpoint's change answers
        // nothing, the insertion that follows it does.
        if (!m_interpreterBreakpointChanges.contains(modelIdOf(response)))
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

void BridgeImpl::handleTracepointHit(const QJsonObject &body)
{
    const auto it = m_tracepoints.constFind(body.value("modelid").toInt(-1));
    if (it == m_tracepoints.constEnd())
        return;

    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi result;
    result.fromString(body.value("result").toString(), decoder);
    GdbMi expressions;
    expressions.fromString(body.value("expressions").toString(), decoder);
    emit message(formatTracepointMessage(it->message, it->captures, result["caps"], expressions),
                 LogMisc);
}

// The interpreter announces a QML event by calling a hook in Qt, so the frame a
// QML breakpoint stops in is Qt's own, and the QML frame the user asked about
// only arrives with the stack the dumpers splice. The prefixes are the ones
// DumperBase.isInterpreterMachineryFrame() marks the same frames by.
static bool isInterpreterHookFrame(const QString &function)
{
    return function.startsWith("qt_qmlDebug") || function.startsWith("qt_v4");
}

void BridgeImpl::handleStackTrace(const QJsonObject &response)
{
    const StackTraceRequest request
        = m_stackTraceRequests.take(response.value("request_seq").toInt());
    const QJsonArray frames = response.value("body").toObject().value("stackFrames").toArray();

    // A stack the bridge refused is not an empty stack: reported, it would
    // empty the stack view and leave the engine on a frame that is not there.
    if (!response.value("success").toBool()) {
        emit message("BridgeImpl: stackTrace failed: " + response.value("message").toString(),
                     LogError);
        if (request.reportsStop)
            reportStop();
        return;
    }

    if (request.reportsStop) {
        const QJsonObject top = frames.isEmpty() ? QJsonObject() : frames.first().toObject();
        const int lineNumber = top.value("line").toInt();
        const FilePath fileName
            = FilePath::fromUserInput(top.value("source").toObject().value("path").toString());
        const QString function = top.value("name").toString();
        // A step that ended in a frame the user did not ask to see is continued
        // rather than reported: out of a function that only forwards, into one
        // that only wraps.
        if (request.fromStep && m_startData.skipKnownFrames) {
            if (isLeavableFunction(function, fileName.path())) {
                execute({ExecutionCommand::StepOut});
                return;
            }
            if (isSkippableFunction(function, fileName.path())) {
                execute({ExecutionCommand::StepIn});
                return;
            }
        }
        if (lineNumber != 0 && fileName.exists() && !isInterpreterHookFrame(function))
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
        add("function", item.value("name").toString());
        const QString path = item.value("source").toObject().value("path").toString();
        add("file", path);
        add("fullname", path);
        add("line", QString::number(item.value("line").toInt()));
        add("address", QString::number(item.value("instructionPointerReference").toInteger()));
        add("module", dapModuleName(item.value("moduleId")));
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
        const QJsonObject body = event.value("body").toObject();
        InferiorResultData result;
        result.exitCode = body.value("exitCode").toInt();
        if (body.value("exitSignal").toInt() != 0) {
            result.exitStatus = InferiorExitStatus::Crash;
            result.signalName = body.value("exitSignalName").toString();
        }
        m_inferiorRunning = false;
        failDeferredRequests();
        emit inferiorDone(result);
        return;
    }
    case DapEventType::DapThread: {
        const QJsonObject body = event.value("body").toObject();
        const QString id = QString::number(body.value("threadId").toInteger());
        const bool started = body.value("reason").toString() == "started";
        GdbMi data;
        data.m_type = GdbMi::Tuple;
        data.addChild(constMi("id", id));
        emit threadEvent(started ? ThreadEvent::Created : ThreadEvent::Exited, data);
        return;
    }
    case DapEventType::Output: {
        const QJsonObject body = event.value("body").toObject();
        const QString category = body.value("category").toString();
        if (category == "console") {
            // The host debugger talking, not the debuggee.
            const QString text = body.value("output").toString();
            emit message(text, LogOutput);
            // Reading symbols, downloading them, and attaching to the threads
            // are the slow parts of a start, and gdb says so as it goes.
            const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                const QString trimmed = line.trimmed();
                if (trimmed.startsWith("Reading symbols from ")
                    || trimmed.startsWith("Downloading")
                    || trimmed.startsWith("[New ") || trimmed.startsWith("[Thread "))
                    emit progressMessage(trimmed);
            }
            return;
        }
        emit message(body.value("output").toString(),
                     category == "stderr" ? AppError : AppOutput);
        return;
    }
    default:
        // An unmapped DAP event still arrives whole: the bridge announces the
        // debuggee's pid this way, which several views and the interrupt path need.
        const QString name = event.value("event").toString();
        if (name == "continued") {
            const QJsonObject body = event.value("body").toObject();
            GdbMi runningThread;
            runningThread.m_type = GdbMi::Tuple;
            runningThread.addChild(
                constMi("thread-id",
                        body.value("allThreadsContinued").toBool()
                            ? QString("all")
                            : QString::number(body.value("threadId").toInteger())));
            emit threadEvent(ThreadEvent::Running, runningThread);
            // A console command can resume the inferior behind the engine's
            // back, and the stop that follows can only be reported from a
            // running state.
            if (!m_inferiorRunning) {
                m_inferiorRunning = true;
                emit inferiorEvent(InferiorEvent::RunRequested);
                emit inferiorEvent(InferiorEvent::RunOk);
            }
            return;
        }
        if (name == "qtc/inferiorResumed") {
            m_inferiorResumed = true;
            if (std::exchange(m_interruptOnceResumed, false))
                interruptGdb();
            return;
        }
        if (name == "qtc/interruptIgnored") {
            failDeferredRequests();
            if (m_stopRequested) {
                m_stopRequested = false;
                emit inferiorEvent(InferiorEvent::StopFailed);
            }
            return;
        }
        if (name == "qtc/breakpointModified") {
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
        if (name == "qtc/breakpointCreated") {
            // A breakpoint somebody else made, the user in the console for
            // instance: nothing else would ever mention it.
            const QString payload = event.value("body").toObject().value("bkpt").toString();
            GdbMi bkpt;
            QStringDecoder decoder(QStringDecoder::Utf8);
            bkpt.fromString(payload, decoder);
            emit breakpointEvent(0, BreakpointOp::Insert, true, bkpt);
            return;
        }
        if (name == "qtc/breakpointDeleted") {
            const QString number = event.value("body").toObject().value("number").toString();
            GdbMi deleted;
            deleted.m_type = GdbMi::Tuple;
            deleted.addChild(constMi("number", number));
            emit breakpointEvent(0, BreakpointOp::Remove, true, deleted);
            return;
        }
        if (name == "qtc/tracepointHit") {
            handleTracepointHit(event.value("body").toObject());
            return;
        }
        if (name == "qtc/library") {
            const QJsonObject body = event.value("body").toObject();
            const QString path = body.value("path").toString();
            GdbMi library;
            library.m_type = GdbMi::Tuple;
            library.addChild(constMi("id", path));
            library.addChild(constMi("target-name", path));
            library.addChild(constMi("host-name", path));
            emit libraryEvent(body.value("reason").toString() == "unloaded"
                                  ? LibraryEvent::Unloaded : LibraryEvent::Loaded,
                              library);
            return;
        }
        if (name == "qtc/threadSelected") {
            GdbMi selected;
            selected.m_type = GdbMi::Tuple;
            selected.addChild(constMi("id", event.value("body").toObject()
                                                .value("id").toString()));
            emit threadEvent(ThreadEvent::Selected, selected);
            return;
        }
        if (name == "qtc/threadGroup") {
            const QJsonObject body = event.value("body").toObject();
            GdbMi group;
            group.m_type = GdbMi::Tuple;
            group.addChild(constMi("id", body.value("id").toString()));
            group.addChild(constMi("pid", body.value("pid").toString()));
            emit threadEvent(body.value("reason").toString() == "exited"
                                 ? ThreadEvent::GroupExited : ThreadEvent::GroupCreated,
                             group);
            return;
        }
        if (name == "process") {
            const qint64 pid = event.value("body").toObject()
                                   .value("systemProcessId").toInteger();
            if (pid != 0)
                emit inferiorPidKnown(ProcessHandle(pid));
            return;
        }
        return;
    }
}

void BridgeImpl::interruptInferior()
{
    m_stopRequested = true;
    interruptGdb();
}

void BridgeImpl::interruptGdb()
{
    // An interrupt is answered only once gdb has the target going: sent while
    // it is still getting there it is dropped, and gdb can die on it. The
    // bridge reports the resume from inside the command that does it, which is
    // the earliest point one can land.
    if (!m_inferiorResumed) {
        m_interruptOnceResumed = true;
        return;
    }
    // A stub-owned inferior runs in a console of its own, which is the stub's
    // to interrupt: signalling the debugger reaches it on a host that has
    // signals at all, and nowhere else.
    if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
        emit interruptTerminalRequested();
        return;
    }
    interruptHost();
}

void BridgeImpl::interruptHost()
{
    if (m_startData.runAsUser.isEmpty()) {
        m_client->dataProvider()->interrupt();
        return;
    }
    // A host running as somebody else cannot be signalled from here: the
    // signal has to be sent with the rights it was started with.
    auto provider = static_cast<BridgeImplDataProvider *>(m_client->dataProvider());
    Process interrupter;
    interrupter.setCommand({"kill", {"-s", "SIGINT", QString::number(provider->processId())}});
    interrupter.setRunAsUser(m_startData.runAsUser);
    interrupter.setEnvironment(m_startData.debuggerRunData.environment);
    interrupter.runBlocking();
    if (interrupter.result() != ProcessResult::FinishedWithSuccess) {
        m_stopRequested = false;
        emit message(QString("Interrupting the debugger as %1 failed: %2")
                         .arg(m_startData.runAsUser, interrupter.cleanedStdErr().trimmed()),
                     LogError);
        emit inferiorEvent(InferiorEvent::StopFailed);
    }
}

void BridgeImpl::handleResumeResponse(bool success)
{
    if (std::exchange(m_resumingFromDeferredStop, false) && success) {
        m_resumePending = false;
        m_inferiorRunning = true;
        return;
    }
    emit inferiorEvent(success ? InferiorEvent::RunOk : InferiorEvent::RunFailed);
    m_resumePending = false;
    m_inferiorRunning = success;
    if (std::exchange(m_interruptOnceRunning, false)) {
        if (success)
            interruptInferior();
        else
            emit inferiorEvent(InferiorEvent::StopFailed);
    }
}

void BridgeImpl::handleStopped(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    m_currentThreadId = body.value("threadId").toInt();
    m_currentFrameId = 1;
    m_inferiorRunning = false;
    GdbMi stoppedThread;
    stoppedThread.m_type = GdbMi::Tuple;
    stoppedThread.addChild(constMi("id", body.value("allThreadsStopped").toBool()
                                             ? QString("all")
                                             : QString::number(m_currentThreadId)));
    emit threadEvent(ThreadEvent::Stopped, stoppedThread);
    m_inferiorResumed = false;
    m_interruptOnceResumed = false;

    // The stop a shutdown had to force to get its own request read is not one
    // anybody asked for, and neither is the signal that made it.
    if (m_shuttingDown)
        return;

    const QString reason = body.value("reason").toString();
    // The stub holds the inferior until it is told to let go, and letting go is
    // a stop on the way rather than one anybody asked for.
    if (m_expectTerminalTrap) {
        if (Utils::HostOsInfo::isWindowsHost() && reason.isEmpty()) {
            m_expectTerminalTrap = false;
            return;
        }
        if (!Utils::HostOsInfo::isWindowsHost() && reason == u"exception"
            && body.value("text").toString() == u"SIGCONT") {
            m_expectTerminalTrap = false;
            execute({ExecutionCommand::Continue});
            return;
        }
    }
    if (reason == u"exception") {
        emit signalReceived(body.value("text").toString(),
                            body.value("description").toString());
    }

    // Which breakpoint a stop belongs to is in this event only, and a stop the
    // user cannot place is one they have to go looking for.
    const QJsonArray hitBreakpointIds = body.value("hitBreakpointIds").toArray();
    if (!hitBreakpointIds.isEmpty()) {
        emit breakpointTriggered(QString::number(hitBreakpointIds.first().toInteger()),
                                 QString::number(m_currentThreadId));
    }
    // A watchpoint sits at no line, so nothing but the watchpoint places the
    // stop, and what it saw change is the only thing it was set for.
    const QJsonArray hitWatchpoints = body.value("hitWatchpoints").toArray();
    if (!hitWatchpoints.isEmpty()) {
        const QJsonObject watchpoint = hitWatchpoints.first().toObject();
        emit watchpointTriggered(QString::number(watchpoint.value("id").toInteger()),
                                 watchpoint.value("expression").toString(),
                                 watchpoint.value("old").toString(),
                                 watchpoint.value("new").toString());
    }

    if (reason != u"exception" && hitBreakpointIds.isEmpty() && hitWatchpoints.isEmpty())
        emit stopReasonReported(reason);

    // Report the stop only once the location is known, as the other backends do.
    const int seq = m_client->stackTrace(m_currentThreadId, 0);
    if (seq < 0) {
        reportStop();
        return;
    }
    m_stackTraceRequests.insert(seq, {true, 0, m_stepRequested && reason == u"step"});
}

void BridgeImpl::reportStop()
{
    if (std::exchange(m_reportsSetupStop, false)) {
        m_stopRequested = false;
        if (std::holds_alternative<AttachToCoreData>(m_startData.inferiorStartData)) {
            emit inferiorEvent(InferiorEvent::RunOkAndInferiorUnrunnable);
            return;
        }
        emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
        if (std::holds_alternative<AttachToProcessData>(m_startData.inferiorStartData)) {
            if (m_startData.continueAfterAttach)
                execute({ExecutionCommand::Continue});
            return;
        }
        if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
            // The stub holds the inferior stopped until it is told to let it
            // go, which is only safe once the debugger has it running.
            m_expectTerminalTrap = true;
            execute({ExecutionCommand::Continue});
            emit kickoffTerminalProcessRequested();
            return;
        }
        // Attaching to a process the server was pointed at stops it, and the
        // session is expected to hand it back running, while a target that was
        // handed over loaded is only resumed when the run asked for it.
        const auto remote = std::get_if<AttachToRemoteServerData>(&m_startData.inferiorStartData);
        if (remote && (remote->attachPid.isValid() || m_startData.continueInsteadOfRun))
            execute({ExecutionCommand::Continue});
        return;
    }
    // Only the interrupt the queue asked for may serve it: a stop of anybody
    // else's leaves the inferior where a call into it can fail, and resuming
    // from it would take the inferior away from whoever stopped it.
    if (!m_deferredRequests.isEmpty() && std::exchange(m_deferredStopRequested, false)) {
        const QList<DeferredRequest> requests = std::exchange(m_deferredRequests, {});
        m_stopRequested = false;
        // The engine did not ask for this stop and must not hear about it.
        // Reporting it has the engine reload a stack from an inferior that runs
        // again before the answer arrives, and re-sync its breakpoints, which
        // queues the next command of the same kind.
        for (const DeferredRequest &request : requests)
            postRequest(request.command, request.arguments);
        m_resumingFromDeferredStop = true;
        execute({ExecutionCommand::Continue});
        return;
    }
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
                QJsonObject{{"target", target}, {"token", qint64(token)},
                            {"flavor", m_startData.intelDisassembly ? "intel" : "att"}});
}

void BridgeImpl::executeDebuggerCommand(const QString &command, const WatchItemData &)
{
    QTC_ASSERT(m_client, return);
    if (m_inferiorRunning || m_resumePending) {
        // The debugger sits in the loop that runs the inferior, so the request
        // would be read at the next stop and run long after it was typed.
        emit message(Tr::tr("The debugger console needs the program to be stopped."), LogError);
        return;
    }
    postRequest("qtc/executeCommand",
                QJsonObject{{"command", command}, {"frameid", m_currentFrameId}});
}

void BridgeImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                       const QString &value)
{
    QTC_ASSERT(m_client, return);
    // The dumpers know how to put a value into a type the debugger cannot
    // assign to by itself, a std::string among them.
    postRequest("qtc/assignValue",
                QJsonObject{{"type", QString::fromUtf8(item.type.toUtf8().toHex())},
                            {"expr", QString::fromUtf8(expr.toUtf8().toHex())},
                            {"value", QString::fromUtf8(value.toUtf8().toHex())},
                            {"simpleType", isIntOrFloatType(item.type)},
                            {"frameid", m_currentFrameId}});
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

void BridgeImpl::watchPoint(quint64 requestId, const QPoint &pnt)
{
    QTC_ASSERT(m_client, return);
    m_pendingWatchPointRequestId = requestId;
    postRequest("qtc/watchPoint", QJsonObject{{"x", pnt.x()}, {"y", pnt.y()}});
}

void BridgeImpl::createSnapshot(quint64 requestId)
{
    // The temporary file is only there for its name, gcore writes the core
    // itself.
    TemporaryFile file("bridgesnapshot");
    if (!file.open()) {
        emit snapshotCreated(requestId, false, {});
        return;
    }
    const FilePath filePath = file.filePath();
    file.close();
    const int seq = postRequest("qtc/createSnapshot", QJsonObject{{"path", filePath.path()}});
    if (seq < 0) {
        emit snapshotCreated(requestId, false, {});
        return;
    }
    m_snapshotRequests.insert(seq, {requestId, filePath});
    if (m_inferiorRunning)
        interruptGdb();
}

} // namespace Debugger::Internal
