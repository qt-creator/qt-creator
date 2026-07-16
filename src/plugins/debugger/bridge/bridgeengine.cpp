// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgeengine.h"

#include <debugger/dap/dapclient.h>

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggertr.h>
#include <debugger/moduleshandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threaddata.h>
#include <debugger/watchhandler.h>

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/widgets.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringDecoder>

#include <cstdlib>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

// Runs the debugger process (gdb hosting gdbbridge.py) and exposes its stdio
// to the DapClient. Kept separate from the dap/ ProcessDataProvider to avoid
// coupling the bridge engine to that translation unit.
class BridgeDataProvider : public IDataProvider
{
public:
    BridgeDataProvider(const DebuggerRunParameters &rp,
                       const CommandLine &cmd,
                       QObject *parent = nullptr)
        : IDataProvider(parent)
        , m_runParameters(rp)
        , m_cmd(cmd)
    {
        connect(&m_proc, &Process::started, this, &IDataProvider::started);
        connect(&m_proc, &Process::done, this, &IDataProvider::done);
        connect(&m_proc, &Process::readyReadStandardOutput,
                this, &IDataProvider::readyReadStandardOutput);
        connect(&m_proc, &Process::readyReadStandardError,
                this, &IDataProvider::readyReadStandardError);
    }

    ~BridgeDataProvider() override
    {
        m_proc.kill();
        m_proc.waitForFinished();
    }

    void start() override
    {
        m_proc.setProcessMode(ProcessMode::Writer);
        if (m_runParameters.debugger().workingDirectory.isDir())
            m_proc.setWorkingDirectory(m_runParameters.debugger().workingDirectory);
        m_proc.setEnvironment(m_runParameters.debugger().environment);
        m_proc.setCommand(m_cmd);
        m_proc.start();
    }

    bool isRunning() const override { return m_proc.isRunning(); }
    void writeRaw(const QByteArray &data) override
    {
        if (m_proc.state() == ProcessState::Running)
            m_proc.writeRaw(data);
    }
    void kill() override { m_proc.kill(); }
    QByteArray readAllStandardOutput() override { return m_proc.readAllStandardOutput().toUtf8(); }
    QString readAllStandardError() override { return m_proc.readAllStandardError(); }
    int exitCode() const override { return m_proc.exitCode(); }
    QString executable() const override { return m_proc.commandLine().executable().toUserOutput(); }

    QProcess::ExitStatus exitStatus() const override { return toQProcess(m_proc.exitStatus()); }
    QProcess::ProcessError error() const override { return toQProcess(m_proc.error()); }
    Utils::ProcessResult result() const override { return m_proc.result(); }
    QString exitMessage() const override { return m_proc.exitMessage(); }

private:
    Utils::Process m_proc;
    const DebuggerRunParameters m_runParameters;
    const CommandLine m_cmd;
};

class BridgeClient : public DapClient
{
public:
    BridgeClient(IDataProvider *provider, QObject *parent = nullptr)
        : DapClient(provider, parent)
    {}

private:
    const QLoggingCategory &logCategory() override
    {
        static const QLoggingCategory category("qtc.dbg.bridgeengine", QtWarningMsg);
        return category;
    }
};

BridgeEngine::BridgeEngine()
{
    setObjectName("BridgeEngine");
    setDebuggerName("Gdb");
    setDebuggerType("Bridge");
    setToolTipHandling(ToolTipHandling::IfStoppedInferior);
}

void BridgeEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qCDebug(logCategory()) << state());

    const DebuggerRunParameters &rp = runParameters();
    const FilePath dumperDir = ICore::resourcePath("debugger");

    // Start gdb with the bridge loaded and hand control to its DAP-shaped
    // server loop, which frames requests/responses/events as Content-Length
    // JSON on stdio (the same framing DapClient already parses).
    // TODO: theDumper.runDapServer() is the bridge-side entry point added in
    // the next step; until then this engine starts but does not converse.
    CommandLine cmd{rp.debugger().command.executable(), {"--nx", "--nw", "-q"}};
    cmd.addArgs({"-iex", "python sys.path.insert(1, '" + dumperDir.path() + "')"});
    cmd.addArgs({"-iex", "python from gdbbridge import *"});
    cmd.addArgs({"-ex", "python theDumper.runDapServer()"});

    IDataProvider *dataProvider = new BridgeDataProvider(rp, cmd, this);
    m_dapClient = new BridgeClient(dataProvider, this);

    connectDataGeneratorSignals();
    m_dapClient->dataProvider()->start();
}

void BridgeEngine::connectDataGeneratorSignals()
{
    if (!m_dapClient)
        return;

    connect(m_dapClient, &DapClient::started, this, &BridgeEngine::handleDapStarted);
    connect(m_dapClient, &DapClient::done, this, &BridgeEngine::handleDapDone);
    connect(m_dapClient, &DapClient::readyReadStandardError,
            this, &BridgeEngine::readDapStandardError);

    connect(m_dapClient, &DapClient::responseReady, this, &BridgeEngine::handleResponse);
    connect(m_dapClient, &DapClient::eventReady, this, &BridgeEngine::handleEvent);
}

void BridgeEngine::handleDapStarted()
{
    notifyEngineSetupOk();
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());

    m_dapClient->sendInitialize();
}

void BridgeEngine::handleDapInitialize()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());

    if (runParameters().isLocalAttachEngine())
        m_dapClient->postRequest("attach",
                                 QJsonObject{{"pid", qint64(runParameters().attachPid().pid())}});
    else
        m_dapClient->sendLaunch(runParameters().inferior().command);
}

void BridgeEngine::handleDapEventInitialized()
{
    QTC_ASSERT(state() == EngineRunRequested, qCDebug(logCategory()) << state());

    m_dapClient->sendConfigurationDone();
}

void BridgeEngine::handleDapConfigurationDone()
{
    // For both launch and attach the bridge reports the initial stop via a
    // 'stopped' event (attach stops the target immediately), so the inferior
    // starts out nominally running and the stopped event drives the rest.
    notifyEngineRunAndInferiorRunOk();
}

void BridgeEngine::executeDebuggerCommand(const QString & /*command*/)
{
    QTC_ASSERT(state() == InferiorStopOk, qCDebug(logCategory()) << state());
}

void BridgeEngine::runCommand(const DebuggerCommand &cmd)
{
    if (state() == EngineSetupRequested) { // cmd has been triggered too early
        showMessage("IGNORED COMMAND: " + cmd.function);
        return;
    }
    QTC_ASSERT(m_dapClient->dataProvider()->isRunning(), notifyEngineIll());
}

void BridgeEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qCDebug(logCategory()) << state());

    m_dapClient->sendDisconnect();
    notifyInferiorShutdownFinished();
}

void BridgeEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qCDebug(logCategory()) << state());

    m_dapClient->sendTerminate();
    m_dapClient->dataProvider()->kill();
}

void BridgeEngine::interruptInferior()
{
    m_dapClient->sendPause();
}

void BridgeEngine::executeStepIn(bool)
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();
    m_dapClient->sendStepIn(m_currentThreadId);
}

void BridgeEngine::executeStepOut()
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();
    m_dapClient->sendStepOut(m_currentThreadId);
}

void BridgeEngine::executeStepOver(bool)
{
    if (m_currentThreadId == -1)
        return;

    notifyInferiorRunRequested();
    m_dapClient->sendStepOver(m_currentThreadId);
}

void BridgeEngine::continueInferior()
{
    notifyInferiorRunRequested();
    m_dapClient->sendContinue(m_currentThreadId);
}

void BridgeEngine::executeRunToLine(const ContextData &data)
{
    // Add one-shot breakpoint
    BreakpointParameters bp;
    bp.oneShot = true;
    if (data.address) {
        bp.type = BreakpointByAddress;
        bp.address = data.address;
    } else {
        bp.type = BreakpointByFileAndLine;
        bp.fileName = data.fileName;
        bp.textPosition = data.textPosition;
    }

    BreakpointManager::createBreakpointForEngine(bp, this);
}

void BridgeEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    QTC_CHECK("FIXME:  BridgeEngine::runToFunctionExec()" && false);
}

void BridgeEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    QTC_CHECK("FIXME:  BridgeEngine::jumpToLineExec()" && false);
}

void BridgeEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoLocation(handler->currentFrame());

    m_currentStackFrameId = handler->currentFrame().debuggerId;
    updateLocals();
}

void BridgeEngine::selectThread(const Thread &thread)
{
    m_currentThreadId = thread->id().toInt();
    threadsHandler()->setCurrentThread(thread);
    // Not just the locals: the frame id they are fetched for belongs to the
    // thread we are leaving.
    updateAll();
}

bool BridgeEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    const auto mimeType = Utils::mimeTypeForFile(bp.fileName);
    return mimeType.matchesName(Utils::Constants::C_HEADER_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::C_SOURCE_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::CPP_HEADER_MIMETYPE)
           || mimeType.matchesName(Utils::Constants::CPP_SOURCE_MIMETYPE)
           || bp.type == BreakpointByFunction;
}

static void setBreakpointParameters(QJsonObject &bp, const QString &condition, int ignoreCount)
{
    if (!condition.isEmpty())
        bp["condition"] = condition;
    if (ignoreCount > 0)
        bp["hitCondition"] = QString::number(ignoreCount);
}

static QJsonObject createBreakpoint(const BreakpointParameters &params)
{
    if (params.fileName.isEmpty())
        return QJsonObject();

    QJsonObject bp;
    bp["line"] = params.textPosition.line;
    setBreakpointParameters(bp, params.condition, params.ignoreCount);
    return bp;
}

static QJsonObject createFunctionBreakpoint(const BreakpointParameters &params)
{
    if (params.functionName.isEmpty())
        return QJsonObject();

    QJsonObject bp;
    bp["name"] = params.functionName;
    setBreakpointParameters(bp, params.condition, params.ignoreCount);
    return bp;
}

void BridgeEngine::insertBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointInsertionRequested);
    notifyBreakpointInsertProceeding(bp);

    BreakpointParameters parameters = bp->requestedParameters();
    if (!parameters.enabled) { // hack for disabling breakpoints
        parameters.pending = false;
        bp->setParameters(parameters);
        notifyBreakpointInsertOk(bp);
        return;
    }

    if (parameters.type == BreakpointByFunction
        && m_dapClient->capabilities().supportsFunctionBreakpoints) {
        dapInsertFunctionBreakpoint(bp);
        return;
    }

    dapInsertBreakpoint(bp);
}

void BridgeEngine::dapInsertFunctionBreakpoint(const Breakpoint &bp)
{
    Q_UNUSED(bp)
    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        const BreakpointParameters &bpParams = breakpoint->requestedParameters();
        QJsonObject jsonBp = createFunctionBreakpoint(bpParams);
        if (!jsonBp.isEmpty() && bpParams.type == BreakpointByFunction && bpParams.enabled)
            breakpoints.append(jsonBp);
    }

    m_dapClient->setFunctionBreakpoints(breakpoints);
}

void BridgeEngine::dapInsertBreakpoint(const Breakpoint &bp)
{
    const BreakpointParameters &params = bp->requestedParameters();

    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        const BreakpointParameters &bpParams = breakpoint->requestedParameters();
        QJsonObject jsonBp = createBreakpoint(bpParams);
        if (!jsonBp.isEmpty() && params.fileName.path() == bpParams.fileName.path()
            && bpParams.enabled) {
            breakpoints.append(jsonBp);
        }
    }

    m_dapClient->setBreakpoints(breakpoints, params.fileName);
}

void BridgeEngine::updateBreakpoint(const Breakpoint &bp)
{
    BreakpointParameters parameters = bp->requestedParameters();
    notifyBreakpointChangeProceeding(bp);

    auto updateBp = [this, bp](void (BridgeEngine::*insert)(const Breakpoint &),
                               void (BridgeEngine::*remove)(const Breakpoint &)) {
        if (bp->isEnabled())
            (this->*remove)(bp);
        else
            (this->*insert)(bp);
    };

    if (parameters.enabled != bp->isEnabled()) {
        if (parameters.type == BreakpointByFunction) {
            updateBp(&BridgeEngine::dapInsertFunctionBreakpoint,
                     &BridgeEngine::dapRemoveFunctionBreakpoint);
            return;
        }

        updateBp(&BridgeEngine::dapInsertBreakpoint, &BridgeEngine::dapRemoveBreakpoint);
    }
}

void BridgeEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointRemoveRequested);
    notifyBreakpointRemoveProceeding(bp);

    dapRemoveBreakpoint(bp);
}

void BridgeEngine::dapRemoveBreakpoint(const Breakpoint &bp)
{
    const BreakpointParameters &params = bp->requestedParameters();

    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        const BreakpointParameters &bpParams = breakpoint->requestedParameters();
        if (breakpoint->responseId() != bp->responseId() && params.fileName == bpParams.fileName
            && bpParams.enabled) {
            QJsonObject jsonBp = createBreakpoint(bpParams);
            breakpoints.append(jsonBp);
        }
    }

    if (params.type == BreakpointByFunction
        && m_dapClient->capabilities().supportsFunctionBreakpoints) {
        dapRemoveFunctionBreakpoint(bp);
        return;
    }

    m_dapClient->setBreakpoints(breakpoints, params.fileName);
}

void BridgeEngine::dapRemoveFunctionBreakpoint(const Breakpoint &bp)
{
    QJsonArray breakpoints;
    for (const auto &breakpoint : breakHandler()->breakpoints()) {
        const BreakpointParameters &bpParams = breakpoint->requestedParameters();
        if (breakpoint->responseId() != bp->responseId() && bpParams.type == BreakpointByFunction
            && bpParams.enabled) {
            QJsonObject jsonBp = createFunctionBreakpoint(bpParams);
            breakpoints.append(jsonBp);
        }
    }

    m_dapClient->setFunctionBreakpoints(breakpoints);
}

void BridgeEngine::loadSymbols(const Utils::FilePath & /*moduleName*/)
{
}

void BridgeEngine::loadAllSymbols()
{
}

void BridgeEngine::reloadModules()
{
    runCommand({"listModules"});
}

void BridgeEngine::refreshModules(const GdbMi &modules)
{
    ModulesHandler *handler = modulesHandler();
    handler->beginUpdateAll();
    for (const GdbMi &item : modules) {
        Module module;
        module.moduleName = item["name"].data();
        QString path = item["value"].data();
        int pos = path.indexOf("' from '");
        if (pos != -1) {
            path = path.mid(pos + 8);
            if (path.size() >= 2)
                path.chop(2);
        } else if (path.startsWith("<module '") && path.endsWith("' (built-in)>")) {
            path = "(builtin)";
        }
        module.modulePath = FilePath::fromUserInput(path);
        handler->updateModule(module);
    }
    handler->endUpdateAll();
}

void BridgeEngine::refreshState(const GdbMi &reportedState)
{
    QString newState = reportedState.data();
    if (newState == "stopped") {
        notifyInferiorSpontaneousStop();
        updateAll();
    } else if (newState == "inferiorexited") {
        notifyInferiorExited();
    }
}

void BridgeEngine::refreshLocation(const GdbMi &reportedLocation)
{
    StackFrame frame;
    frame.file = FilePath::fromUserInput(reportedLocation["file"].data());
    frame.line = reportedLocation["line"].toInt();
    frame.usable = frame.file.isReadableFile();
    if (state() == InferiorRunOk) {
        showMessage(QString("STOPPED AT: %1:%2").arg(frame.file.toUserOutput()).arg(frame.line));
        gotoLocation(frame);
        notifyInferiorSpontaneousStop();
        updateAll();
    }
}

void BridgeEngine::refreshSymbols(const GdbMi &symbols)
{
    QString moduleName = symbols["module"].data();
    Symbols syms;
    for (const GdbMi &item : symbols["symbols"]) {
        Symbol symbol;
        symbol.name = item["name"].data();
        syms.append(symbol);
    }
    showModuleSymbols(FilePath::fromUserInput(moduleName), syms);
}

void BridgeEngine::doUpdateLocals(const UpdateParameters &params)
{
    // Drive the real Qt dumpers over the native qtc/fetchVariables request.
    // The response payload is the dumper's structured output, consumed by the
    // shared updateLocalsView() - the same path GdbEngine uses. Lazy expansion
    // and watchers ride along via WatchHandler's format/watcher requests
    // (the 'expanded' set among them).
    watchHandler()->notifyUpdateStarted(params);

    DebuggerCommand cmd("fetchVariables");
    watchHandler()->appendFormatRequests(&cmd);
    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

    const DebuggerSettings &s = settings();
    cmd.arg("passexceptions", qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE"));
    cmd.arg("fancy", s.useDebuggingHelpers());
    cmd.arg("allowinferiorcalls", s.allowInferiorCalls());
    cmd.arg("autoderef", s.autoDerefPointers());
    cmd.arg("dyntype", s.useDynamicType());
    cmd.arg("qobjectnames", s.showQObjectNames());
    cmd.arg("qtversion", runParameters().qtVersion());
    cmd.arg("qtnamespace", runParameters().configuredQtNamespace());

    const StackFrame frame = stackHandler()->currentFrame();
    cmd.arg("context", frame.context);
    cmd.arg("nativemixed", false);

    cmd.arg("stringcutoff", s.maximalStringLength());
    cmd.arg("displaystringlimit", s.displayStringLimit());

    cmd.arg("resultvarname", QString());
    cmd.arg("partialvar", params.partialVariable);
    cmd.arg("frameid", m_currentStackFrameId);

    m_dapClient->postRequest("qtc/fetchVariables", cmd.args.toObject());
}

QString BridgeEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
    case QProcess::FailedToStart:
        return Tr::tr("The debugger process failed to start. Either the "
            "invoked program \"%1\" is missing, or you may have insufficient "
            "permissions to invoke the program.")
            .arg(m_dapClient->dataProvider()->executable());
    case QProcess::Crashed:
        return Tr::tr("The debugger process crashed some time after starting "
            "successfully.");
    case QProcess::Timedout:
        return Tr::tr("The last waitFor...() function timed out. "
            "The state of QProcess is unchanged, and you can try calling "
            "waitFor...() again.");
    case QProcess::WriteError:
        return Tr::tr("An error occurred when attempting to write "
            "to the debugger process. For example, the process may not be running, "
            "or it may have closed its input channel.");
    case QProcess::ReadError:
        return Tr::tr("An error occurred when attempting to read from "
            "the debugger process. For example, the process may not be running.");
    default:
        return Tr::tr("An unknown error in the debugger process occurred.") + ' ';
    }
}

void BridgeEngine::handleDapDone()
{
    if (state() == DebuggerFinished)
        return;

    if (m_dapClient->dataProvider()->result() == ProcessResult::StartFailed) {
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        ICore::showWarningWithOptions(Tr::tr("Adapter start failed"),
                                      m_dapClient->dataProvider()->exitMessage());
        return;
    }

    const QProcess::ProcessError error = m_dapClient->dataProvider()->error();
    if (error != QProcess::UnknownError) {
        showMessage("HANDLE DAP ERROR");
        if (error != QProcess::Crashed)
            AsynchronousMessageBox::critical(Tr::tr("Bridge I/O Error"), errorMessage(error));
        if (error == QProcess::FailedToStart)
            return;
    }
    showMessage(QString("BRIDGE PROCESS FINISHED, status %1, code %2")
                    .arg(m_dapClient->dataProvider()->exitStatus())
                    .arg(m_dapClient->dataProvider()->exitCode()));
    notifyEngineSpontaneousShutdown();
}

void BridgeEngine::readDapStandardError()
{
    QString err = m_dapClient->dataProvider()->readAllStandardError();
    qCDebug(logCategory()) << "BRIDGE STDERR:" << err;
    showMessage("Unexpected bridge stderr: " + err);
}

void BridgeEngine::handleResponse(DapResponseType type, const QJsonObject &response)
{
    const QString command = response.value("command").toString();
    const bool success = response.value("success").toBool();

    switch (type) {
    case DapResponseType::Initialize: {
        // setupDumpers() reports which types the dumpers know and which
        // display formats they offer, in the same shape the loadDumpers reply
        // had; without it the format menus lose the dumper-provided entries.
        const QString dumpers = response.value("body").toObject()
                                    .value("qtcDumpers").toString();
        if (!dumpers.isEmpty()) {
            QStringDecoder decoder(QStringDecoder::Utf8);
            GdbMi reported;
            reported.fromString('{' + dumpers + '}', decoder);
            watchHandler()->addDumpers(reported["dumpers"]);
        }
        handleDapInitialize();
        break;
    }
    case DapResponseType::ConfigurationDone:
        showMessage("configurationDone", LogDebug);
        handleDapConfigurationDone();
        break;
    case DapResponseType::Continue:
        showMessage("continue", LogDebug);
        notifyInferiorRunOk();
        break;
    case DapResponseType::StackTrace:
        handleStackTraceResponse(response);
        break;
    case DapResponseType::StepIn:
    case DapResponseType::StepOut:
    case DapResponseType::StepOver:
        if (success) {
            showMessage(command, LogDebug);
            notifyInferiorRunOk();
        } else {
            notifyInferiorRunFailed();
        }
        break;
    case DapResponseType::DapThreads:
        handleThreadsResponse(response);
        break;
    case DapResponseType::Pause:
        // A refused pause must not leave the engine waiting in
        // InferiorStopRequested for a stop that will never be reported.
        if (!success)
            notifyInferiorStopFailed();
        break;
    case DapResponseType::SetFunctionBreakpoints:
    case DapResponseType::SetBreakpoints:
        handleBreakpointResponse(response);
        break;
    case DapResponseType::Launch:
        if (!success) {
            notifyEngineRunFailed();
            AsynchronousMessageBox::critical(
                Tr::tr("Failed to Start Application"),
                Tr::tr("\"%1\" could not be started. Error message: %2")
                    .arg(runParameters().inferior().command.toUserOutput())
                    .arg(response.value("message").toString()));
        }
        break;
    default:
        if (!success) {
            // A failed request has no body, and handing that to the handlers
            // below would wipe the view each one fills. The locals view has to
            // be released even so, or it stays in its updating state - with
            // stale, unexpandable items - until the next successful fetch.
            if (command == "qtc/fetchVariables") {
                showMessage("FETCHING VARIABLES FAILED: "
                            + response.value("message").toString());
                watchHandler()->notifyUpdateFinished();
            }
            break;
        }
        if (command == "qtc/fetchVariables")
            handleFetchVariablesResponse(response);
        else
            showMessage("UNKNOWN RESPONSE:" + command);
    }

    if (!success) {
        showMessage(QString("BRIDGE COMMAND FAILED: %1").arg(command));
        return;
    }
}

void BridgeEngine::handleStackTraceResponse(const QJsonObject &response)
{
    QJsonArray stackFrames = response.value("body").toObject().value("stackFrames").toArray();
    if (stackFrames.isEmpty())
        return;

    // Do not navigate to frame 0 here: it is often a frame without readable
    // source (libc after an interrupt), and opening it pops a modal "file does
    // not exist" dialog. refreshStack() goes to the first usable frame.
    refreshStack(stackFrames);

    // Locals must come from the frame the views ended up showing, which is not
    // necessarily frame 0.
    const uint frameId = stackHandler()->currentFrame().debuggerId;
    m_currentStackFrameId = frameId ? int(frameId)
                                    : stackFrames.first().toObject().value("id").toInt();
    updateLocals();
}

void BridgeEngine::handleThreadsResponse(const QJsonObject &response)
{
    const QJsonArray threads = response.value("body").toObject().value("threads").toArray();

    if (threads.isEmpty())
        return;

    ThreadsHandler *handler = threadsHandler();
    for (const QJsonValueConstRef &thread : threads) {
        ThreadData threadData;
        threadData.id = QString::number(thread.toObject().value("id").toInt());
        threadData.name = thread.toObject().value("name").toString();
        handler->updateThread(threadData);
    }

    if (m_currentThreadId) {
        Thread thread = threadsHandler()->threadForId(QString::number(m_currentThreadId));
        if (thread && thread != threadsHandler()->currentThread())
            handler->setCurrentThread(thread);
    }
}

void BridgeEngine::handleFetchVariablesResponse(const QJsonObject &response)
{
    // The body carries the dumper's structured output verbatim; parse it with
    // the existing GdbMi reader and feed the shared updateLocalsView().
    const QString payload = response.value("body").toObject().value("dumperResult").toString();
    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi all;
    all.fromString('{' + payload + '}', decoder);
    updateLocalsView(all);
    watchHandler()->notifyUpdateFinished();
    updateToolTips();
}

void BridgeEngine::handleBreakpointResponse(const QJsonObject &response)
{
    const QJsonObject body = response.value("body").toObject();
    const QJsonArray breakpoints = body.value("breakpoints").toArray();

    QHash<QString, QJsonObject> map;
    for (const QJsonValueConstRef &jsonbp : breakpoints) {
        QJsonObject breakpoint = jsonbp.toObject();
        QString fileName = breakpoint.value("source").toObject().value("path").toString();
        int line = breakpoint.value("line").toInt();

        map.insert(fileName + ":" + QString::number(line), breakpoint);
    }

    const Breakpoints bps = breakHandler()->breakpoints();
    for (const Breakpoint &bp : bps) {
        BreakpointParameters parameters = bp->requestedParameters();
        QString mapKey = parameters.fileName.toUrlishString() + ":"
                         + QString::number(parameters.textPosition.line);
        if (map.find(mapKey) != map.end()) {
            if (bp->state() == BreakpointRemoveProceeding) {
                notifyBreakpointRemoveFailed(bp);
            } else if (bp->state() == BreakpointInsertionProceeding
                       && !map.value(mapKey).value("verified").toBool()) {
                notifyBreakpointInsertFailed(bp);
            } else if (bp->state() == BreakpointInsertionProceeding) {
                parameters.pending = false;
                bp->setParameters(parameters);
                notifyBreakpointInsertOk(bp);
                if (parameters.oneShot)
                    continueInferior();
            }
            if (!bp.isNull())
                bp->setResponseId(QString::number(map.value(mapKey).value("id").toInt()));
            map.remove(mapKey);
        } else {
            if (bp->state() == BreakpointRemoveProceeding)
                notifyBreakpointRemoveOk(bp);
        }

        if (!bp.isNull() && bp->state() == BreakpointUpdateProceeding) {
            BreakpointParameters parameters = bp->requestedParameters();
            if (parameters.enabled != bp->isEnabled()) {
                parameters.pending = false;
                bp->setParameters(parameters);
                notifyBreakpointChangeOk(bp);
                continue;
            }
        }
    }

    for (const Breakpoint &bp : breakHandler()->breakpoints()) {
        if (bp->state() == BreakpointInsertionProceeding) {
            if (!bp->isEnabled())
                continue;

            FilePath path = bp->requestedParameters().fileName;
            int line = bp->requestedParameters().textPosition.line;

            QJsonObject jsonBreakpoint;
            QString key;
            for (auto it = map.cbegin(); it != map.cend(); ++it) {
                const QJsonObject breakpoint = *it;
                if (path == bp->requestedParameters().fileName
                    && std::abs(breakpoint.value("line").toInt() - line)
                           < std::abs(jsonBreakpoint.value("line").toInt() - line)) {
                    jsonBreakpoint = breakpoint;
                    key = it.key();
                }
            }

            if (!jsonBreakpoint.isEmpty() && jsonBreakpoint.value("verified").toBool()) {
                BreakpointParameters parameters = bp->requestedParameters();
                parameters.pending = false;
                parameters.textPosition.line = jsonBreakpoint.value("line").toInt();
                parameters.textPosition.column = jsonBreakpoint.value("column").toInt();
                bp->setParameters(parameters);
                bp->setResponseId(QString::number(jsonBreakpoint.value("id").toInt()));
                notifyBreakpointInsertOk(bp);
                if (parameters.oneShot)
                    continueInferior();
                map.remove(key);
            }
        }
    }
}

void BridgeEngine::handleEvent(DapEventType type, const QJsonObject &event)
{
    const QString eventType = event.value("event").toString();
    const QJsonObject body = event.value("body").toObject();
    showMessage(eventType, LogDebug);

    switch (type) {
    case DapEventType::Initialized:
        claimInitialBreakpoints();
        handleDapEventInitialized();
        break;
    case DapEventType::Stopped:
        handleStoppedEvent(event);
        break;
    case DapEventType::Exited:
        notifyInferiorExited();
        break;
    case DapEventType::DapThread:
        m_dapClient->threads();
        if (body.value("reason").toString() == "started" && body.value("threadId").toInt() == 1)
            claimInitialBreakpoints();
        break;
    case DapEventType::Output: {
        const QString category = body.value("category").toString();
        const QString output = body.value("output").toString();
        if (category == "stdout")
            showMessage(output, AppOutput);
        else if (category == "stderr")
            showMessage(output, AppError);
        else
            showMessage(output, LogDebug);
        break;
    }
    default:
        showMessage("UNKNOWN EVENT:" + eventType);
    }
}

void BridgeEngine::handleStoppedEvent(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    m_currentThreadId = body.value("threadId").toInt();

    if (body.value("reason").toString() == "breakpoint") {
        QString id = QString::number(body.value("hitBreakpointIds").toArray().first().toInteger());

        Breakpoint bp = breakHandler()->findBreakpointByResponseId(id);
        if (bp) {
            const BreakpointParameters &params = bp->requestedParameters();
            gotoLocation(Location(params.fileName, params.textPosition));
            if (params.oneShot)
                bp->globalBreakpoint()->deleteBreakpoint();
        }
    }

    if (state() == InferiorStopRequested)
        notifyInferiorStopOk();
    else
        notifyInferiorSpontaneousStop();

    m_dapClient->stackTrace(m_currentThreadId);
    m_dapClient->threads();
}

void BridgeEngine::refreshStack(const QJsonArray &stackFrames)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    for (const auto &value : stackFrames) {
        StackFrame frame;
        QJsonObject item = value.toObject();
        // The "id" is an opaque DAP frame id kept in debuggerId below for
        // frame selection; it is not a sequential level, so leave
        // frame.level empty and let the view show the row number.
        frame.function = item.value("name").toString();
        frame.line = item.value("line").toInt();
        QJsonObject source = item.value("source").toObject();
        frame.file = FilePath::fromUserInput(source.value("path").toString());
        frame.address = quint64(item.value("instructionPointerReference").toInteger());
        frame.usable = frame.file.isReadableFile();
        frame.debuggerId = item.value("id").toInt();
        frames.append(frame);
    }
    handler->setFrames(frames, false);

    int index = stackHandler()->firstUsableIndex();
    handler->setCurrentIndex(index);
    if (index >= 0 && index < handler->stackSize())
        gotoLocation(handler->frameAt(index));
}

void BridgeEngine::reloadFullStack()
{
    updateAll();
}

void BridgeEngine::updateAll()
{
    // Refresh the stack; its response chains into the base updateLocals(),
    // which drives doUpdateLocals() -> qtc/fetchVariables.
    if (m_currentThreadId != -1)
        m_dapClient->stackTrace(m_currentThreadId);
}

bool BridgeEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability | BreakConditionCapability | ShowModuleSymbolsCapability
                  | RunToLineCapability | AddWatcherCapability);
}

void BridgeEngine::claimInitialBreakpoints()
{
    BreakpointManager::claimBreakpointsForEngine(this);
}

const QLoggingCategory &BridgeEngine::logCategory()
{
    static const QLoggingCategory category("qtc.dbg.bridgeengine", QtWarningMsg);
    return category;
}

DebuggerEngine *createBridgeEngine()
{
    return new BridgeEngine;
}

} // namespace Debugger::Internal
