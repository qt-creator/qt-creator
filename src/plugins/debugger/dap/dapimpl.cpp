// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dapimpl.h"

#include "dapclient.h"
#include "dapdataproviders.h"

#include "../debuggerprotocol.h"
#include "../debuggertr.h"
#include "../disassemblerlines.h"
#include "../genericdebuggerengine.h"
#include "../shared/hostutils.h"
#include "../watchutils.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

using namespace Utils;

namespace Debugger::Internal {

namespace {

class DapImplClient final : public DapClient
{
public:
    using DapClient::DapClient;

private:
    const QLoggingCategory &logCategory() final
    {
        static const QLoggingCategory category("qtc.dbg.dapimpl", QtWarningMsg);
        return category;
    }
};

} // namespace

static GdbMi constMi(const QString &name, const QString &data)
{
    GdbMi mi;
    mi.m_name = name;
    mi.m_data = data;
    mi.m_type = GdbMi::Const;
    return mi;
}

static QString quotedPath(const QString &path)
{
    QString quoted = path;
    quoted.replace('\\', "\\\\");
    quoted.replace('"', "\\\"");
    return '"' + quoted + '"';
}

QString DapImpl::localSourcePath(const QString &reported) const
{
    return mappedSourcePath(m_startData.sourcePathMap, reported, true);
}

QString DapImpl::reportedSourcePath(const QString &local) const
{
    return mappedSourcePath(m_startData.sourcePathMap, local, false);
}

// A module as the debugger behind the adapter matches one, by a pattern rather
// than by a path: what a path separator would mean there is a single character.
static QString dotEscape(QString str)
{
    str.replace(' ', '.');
    str.replace('\\', '.');
    str.replace('/', '.');
    return str;
}

static DebuggerEngineSetupData dapImplSetupData()
{
    DebuggerEngineSetupData data;
    // Only what the protocol itself defines. Memory, disassembly and running
    // backwards are optional in DAP, so they are offered here and refused per
    // session if the adapter turns out not to have them.
    data.attachToCoreCapabilities = AdditionalQmlStackCapability | AddWatcherCapability
                                  | AutoDerefPointersCapability
                                  | CreateFullBacktraceCapability | DisassemblerCapability
                                  | OperateByInstructionCapability | RegisterCapability
                                  | ShowMemoryCapability | ShowModuleSectionsCapability
                                  | ShowModuleSymbolsCapability
                                  | WatchComplexExpressionsCapability;
    data.capabilities = AddWatcherCapability | AddWatcherWhileRunningCapability
                      | ReverseSteppingCapability
                      | BreakConditionCapability | BreakIndividualLocationsCapability
                      | CreateFullBacktraceCapability | ShowMemoryCapability
                      | DisassemblerCapability | OperateByInstructionCapability
                      | WatchWidgetsCapability | AdditionalQmlStackCapability
                      | AutoDerefPointersCapability | BreakModuleCapability
                      | BreakOnThrowAndCatchCapability | TracePointCapability
                      | RegisterCapability | ReloadModuleCapability
                      | ReloadModuleSymbolsCapability
                      | ShowModuleSectionsCapability | ShowModuleSymbolsCapability
                      | JumpToLineCapability | ResetInferiorCapability
                      | ReturnFromFunctionCapability
                      | RunToLineCapability | SnapshotCapability
                      | WatchComplexExpressionsCapability
                      | WatchpointByAddressCapability | WatchpointByExpressionCapability;
    data.extraCapabilities = DebuggerExtraCapability::BreakOnMain
                           | DebuggerExtraCapability::ContinueAfterAttach
                           | DebuggerExtraCapability::ContinueInsteadOfRun
                           | DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::JumpTargetCheck
                           | DebuggerExtraCapability::LibraryEvent
                           | DebuggerExtraCapability::PeripheralRegisters
                           | DebuggerExtraCapability::RunAsUser
                           | DebuggerExtraCapability::RunCommandDeferral
                           | DebuggerExtraCapability::SignalReceived
                           | DebuggerExtraCapability::SkipKnownFrames
                           | DebuggerExtraCapability::SourceFiles
                           | DebuggerExtraCapability::SpecialBreakpoints
                           | DebuggerExtraCapability::ThreadEvent
                           | DebuggerExtraCapability::Threads;
    data.startModes = DebuggerStartModeFlag::AttachToProcess
                    | DebuggerStartModeFlag::AttachToRemoteServer
                    | DebuggerStartModeFlag::AttachToTerminalStub
                    | DebuggerStartModeFlag::AttachToCore
                    | DebuggerStartModeFlag::Launch;
    data.toolTipHandling = ToolTipHandling::IfStoppedInferiorAndCppEditor;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        // A QML file has no adapter of its own here, and the source breakpoint
        // an adapter for the native side would get is one it cannot resolve.
        if (!query.isCppBreakpoint())
            return false;
        return query.type == BreakpointByFileAndLine || query.type == BreakpointByFunction
               || query.type == BreakpointAtMain || query.type == BreakpointAtThrow
               || query.type == BreakpointAtCatch || query.type == BreakpointAtFork
               || query.type == BreakpointAtExec || query.type == BreakpointAtSysCall
               || query.type == WatchpointAtAddress || query.type == WatchpointAtExpression;
    };
    return data;
}

DapImpl::DapImpl(const DapStartData &startData)
    : DebuggerEngineInterface(dapImplSetupData())
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

DapImpl::~DapImpl()
{
    reportRunning(false);
    reportResumed();
    if (m_startData.channel)
        m_startData.channel->send = {};
}

// Once either way: whoever asked for the session is waiting to hear that it is
// up, and would wait forever if the adapter never got that far.
void DapImpl::reportRunning(bool running)
{
    if (m_runningReported || !m_startData.channel || !m_startData.channel->reportRunning)
        return;
    m_runningReported = true;
    m_startData.channel->reportRunning(running);
}

// The state machine takes a run request before the run itself, and an adapter
// that resumed on its own never made one.
void DapImpl::reportRunRequested()
{
    // The adapter's frame ids do not survive a resume, so what was addressable
    // while the debuggee was stopped is not once it runs again, the scope a
    // register belongs to included.
    m_frameIds.clear();
    m_currentFrameId = -1;
    m_registerScopeReference = 0;
    m_registerNamesFetched = false;
    m_inferiorRunning = true;
    if (m_runRequestPending || !m_runReported)
        return;
    m_runRequestPending = true;
    emit inferiorEvent(InferiorEvent::RunRequested);
}

// A resume is both announced as an event and answered as a request, in that
// order, so the outcome is only reported for the first of the two to arrive.
void DapImpl::reportRunResult(bool ok)
{
    if (!m_runRequestPending)
        return;
    m_runRequestPending = false;
    if (!ok)
        m_inferiorRunning = false;
    emit inferiorEvent(ok ? InferiorEvent::RunOk : InferiorEvent::RunFailed);
}

void DapImpl::reportStoppedLocation(const FilePath &file, int line)
{
    // A stop the adapter names no frame for is nothing a follower could ask
    // about, and passing the -1 on would tell it the program runs again.
    if (m_currentFrameId < 0)
        return;
    if (!m_startData.channel || !m_startData.channel->reportStopped)
        return;
    m_stopReported = true;
    m_startData.channel->reportStopped(m_currentFrameId, file, line);
}

void DapImpl::reportResumed()
{
    if (!std::exchange(m_stopReported, false) || !m_startData.channel
        || !m_startData.channel->reportStopped) {
        return;
    }
    m_startData.channel->reportStopped(-1, {}, 0);
}

void DapImpl::sendCustomRequest(const QString &command, const QJsonObject &arguments,
                                const DapSessionChannel::Answer &answer)
{
    QTC_ASSERT(m_client, answer(ResultError(Tr::tr("The debug adapter is not running."))); return);
    const int sequence = m_client->postRequest(command, arguments);
    if (answer)
        m_customRequests.insert(sequence, answer);
}

void DapImpl::start()
{
    const DapAdapterDescriptor &adapter = m_startData.adapter;
    IDataProvider *provider = nullptr;
    switch (adapter.kind) {
    case DapAdapterDescriptor::Kind::Executable: {
        auto processProvider = new ProcessDataProvider(adapter.runData, adapter.command, this);
        processProvider->setRunAsUser(m_startData.runAsUser);
        provider = processProvider;
        break;
    }
    case DapAdapterDescriptor::Kind::Server:
        provider = new TcpDataProvider(adapter.host, adapter.port, this);
        break;
    case DapAdapterDescriptor::Kind::Pipe:
        provider = new LocalSocketDataProvider(adapter.pipePath, this);
        break;
    }
    QTC_ASSERT(provider, emit inferiorEvent(InferiorEvent::EngineSetupFailed); return);

    m_client = new DapImplClient(provider, this);

    connect(m_client, &DapClient::requestSent,
            this, &DapImpl::logRequest);
    connect(m_client, &DapClient::started,
            this, &DapImpl::handleStarted);
    connect(m_client, &DapClient::done,
            this, &DapImpl::handleFinished);
    connect(m_client, &DapClient::readyReadStandardError,
            this, &DapImpl::handleStandardError);
    connect(m_client, &DapClient::unframedOutput,
            this, [this](const QString &text) { emit message(text, LogOutput); });
    connect(m_client, &DapClient::responseReady,
            this, &DapImpl::handleResponse);
    connect(m_client, &DapClient::eventReady,
            this, &DapImpl::handleEvent);

    if (m_startData.channel) {
        m_startData.channel->send
            = [this](const QString &command, const QJsonObject &arguments,
                     const DapSessionChannel::Answer &answer) {
                  sendCustomRequest(command, arguments, answer);
              };
    }

    emit message(provider->executable(), LogInput);
    provider->start();
}

void DapImpl::handleStarted()
{
    postRequest("initialize",
                QJsonObject{{"clientID", "QtCreator"},
                            {"clientName", "QtCreator"},
                            {"adapterID", m_startData.adapterId},
                            {"pathFormat", "path"},
                            {"linesStartAt1", true},
                            {"columnsStartAt1", true},
                            {"supportsVariableType", true},
                            {"supportsMemoryReferences", true}});
}

// The setup is over once the adapter has answered for itself, not when its
// process is up: only then is there a session to launch anything in.
void DapImpl::reportEngineSetup(bool success)
{
    if (m_setupReported)
        return;
    m_setupReported = true;
    emit inferiorEvent(success ? InferiorEvent::EngineSetupOk
                               : InferiorEvent::EngineSetupFailed);
}

void DapImpl::handleFinished()
{
    m_watchdog.stop();
    reportTheExitWithoutItsSignal();
    if (m_isResetRestart) {
        restartSession();
        return;
    }
    // An adapter that is gone without ever having answered leaves the session
    // unopened, whether it failed to start or quit on its own.
    if (!m_setupReported)
        reportEngineSetup(false);
    else if (!m_runReported)
        emit inferiorEvent(InferiorEvent::EngineRunFailed);
    auto provider = qobject_cast<ProcessDataProvider *>(m_client->dataProvider());
    emit engineProcessFinished(provider ? provider->resultData() : ProcessResultData());
}

// The session the reset ended is replaced by one of its own, which means the
// adapter is started again from scratch: nothing of what the one before it
// answered still holds, while the breakpoints the model knows do.
void DapImpl::restartSession()
{
    DapClient *previous = m_client;
    IDataProvider *provider = previous->dataProvider();
    previous->disconnect(this);
    previous->deleteLater();
    provider->deleteLater();
    m_client = nullptr;

    m_configured = false;
    m_currentThreadId = -1;
    m_currentFrameId = -1;
    m_frameIds.clear();
    m_stackTraceRequests.clear();
    m_pendingRequests.clear();
    m_customRequests.clear();
    m_inferiorCallsInFlight = 0;
    m_widgetPicksNeedingAStop.clear();
    m_breakpointRequests.clear();
    m_functionBreakpointRequests.clear();
    m_breakpointModulesSent = false;
    m_instructionBreakpointRequests.clear();
    m_ownBreakpointIds.clear();
    // What the debugger was asked for past the adapter is numbered by that
    // debugger, and the one taking over starts its numbering again, so an
    // entry kept here claims the stops of whatever gets its number next. The
    // catchpoints are the model's all the same, so they are put aside rather
    // than dropped and asked for again once the new session takes breakpoints.
    m_restartedCatchpoints = std::exchange(m_catchpoints, {});
    m_catchpointCompanions.clear();
    m_consoleWatchpoints.clear();
    m_pendingConsoleWatchpoints = 0;
    m_alienWatchpoints.clear();
    m_watchedValues.clear();
    m_stopRequested = false;
    m_stepRequested = false;
    start();
}

void DapImpl::handleStandardError()
{
    const QString error = m_client->dataProvider()->readAllStandardError();
    if (!error.isEmpty())
        emit message(error, LogError);
}

void DapImpl::reportUnsupported(const QString &what)
{
    emit message(Tr::tr("\"%1\" is not part of the Debug Adapter Protocol, so the adapter "
                        "cannot be asked for it.").arg(what), LogWarning);
}

int DapImpl::postRequest(const QString &command, const QJsonObject &arguments)
{
    QTC_ASSERT(m_client, return -1);
    return m_client->postRequest(command, arguments);
}

void DapImpl::logRequest(int seq, const QString &command, const QJsonObject &arguments)
{
    const QString text = QString::number(seq) + command + '('
                         + QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact))
                         + ')';
    m_pendingRequests.insert(seq, {text, command, QDateTime::currentMSecsSinceEpoch()});
    restartWatchdog();
    emit message(text, LogInput);
}

void DapImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_pendingRequests.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}

// The protocol has no request for a command of the debugger's own, so what the
// user configured goes in as an expression evaluated in the console: the one
// place an adapter is expected to take its own commands.
void DapImpl::runUserCommands(const QStringList &commands)
{
    for (const QString &command : commands) {
        if (!command.trimmed().isEmpty())
            postReplCommand(command);
    }
}

void DapImpl::runUserStartupCommands()
{
    const DebuggerUserCommands &commands = m_startData.userCommands;
    if (!commands.startScript.isEmpty()) {
        // A script is a file of such commands, and nothing in the protocol
        // reads a file, so the lines are sent one by one.
        const Utils::Result<QByteArray> contents = commands.startScript.fileContents();
        if (!contents) {
            emit message("The debugger start script is not accessible: "
                             + commands.startScript.toUserOutput(), LogWarning);
            return;
        }
        runUserCommands(QString::fromUtf8(*contents).split('\n'));
        return;
    }
    runUserCommands(commands.atStartup.split('\n'));
}

void DapImpl::postReplCommand(const QString &command)
{
    QJsonObject arguments{{"expression", command}, {"context", "repl"}};
    // A command that does not read the inferior's state has no frame to run
    // in, and the session has none to name before it has stopped anywhere.
    // Zero is a valid id: gdb's adapter numbers the innermost frame 0.
    if (m_currentFrameId >= 0)
        arguments.insert("frameId", m_currentFrameId);
    postRequest("evaluate", arguments);
}

// The protocol says nothing about how much of a string a value is to carry,
// and the limits travel with every request because the user can change them
// while the session runs, so the debugger is told before the values are asked
// for. How much is read and how much of that is shown are one setting here, so
// the smaller of the two is what it becomes.
void DapImpl::applyTheStringLengthLimit(const DumperOptions &options)
{
    const int limit = qMin(options.maximalStringLength, options.displayStringLimit);
    if (limit <= 0 || limit == m_stringLengthLimit)
        return;
    m_stringLengthLimit = limit;
    const QStringList commands
        = {QString("set print elements %1").arg(limit),
           QString("settings set target.max-string-summary-length %1").arg(limit)};
    for (const QString &command : commands) {
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
}

// A debugger prints only as much of a long value as a limit of its own allows,
// and the console is the one place the user reads a value in full, so the limit
// is lifted where the adapter takes the debugger's own commands. Both spellings
// go out because neither debugger knows the other's, and an adapter that knows
// neither answers with an error nobody has to read.
// The completion the console offers is a command of the debugger's own as well,
// and gdb stops counting candidates at 200 of them.
void DapImpl::liftTheDebuggerLimits()
{
    const QStringList commands = {"set print elements 10000",
                                  "settings set target.max-string-summary-length 10000",
                                  "set max-completions 1000"};
    for (const QString &command : commands) {
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
}

// The daemon fetches symbols over a network that may not answer, and a fetch
// from a server that does not can take the session with it, so what the user
// turned off has to reach the debugger as much as what the user turned on. gdb
// has a switch for it, lldb only the servers it was given, which are taken
// away instead.
void DapImpl::configureTheDebugInfoDaemon()
{
    if (!m_startData.useDebugInfoD)
        return;
    const QString setting = *m_startData.useDebugInfoD ? QString("on") : QString("off");
    QStringList commands = {"set debuginfod enabled " + setting};
    if (!*m_startData.useDebugInfoD)
        commands.append("settings set plugin.symbol-locator.debuginfod.server-urls");
    for (const QString &command : commands) {
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
}

// The protocol carries none of the places a debugger looks in, so the ones the
// session was configured with are handed over in the debugger's own words,
// before the program that needs them is loaded. Both spellings go out because
// neither debugger knows the other's.
void DapImpl::configureTheSearchPaths()
{
    QStringList commands;
    if (!m_startData.sysroot.isEmpty())
        commands << "set sysroot " + m_startData.sysroot.path();
    for (const Utils::FilePath &directory : m_startData.sourceDirectories)
        commands << "directory " + directory.path();
    if (!m_startData.solibSearchPath.isEmpty()) {
        const QString paths = Utils::transform(m_startData.solibSearchPath,
                                               &Utils::FilePath::path)
                                  .join(Utils::HostOsInfo::pathListSeparator());
        commands << "set solib-search-path " + paths
                 << "settings append target.exec-search-paths " + paths;
    }
    if (!m_startData.debugInfoLocation.isEmpty() && m_startData.debugInfoLocation.exists()) {
        const QString location = m_startData.debugInfoLocation.path();
        // The debugger already has a place of its own for separately built
        // debug information and the setting takes the whole list at once, so
        // gdb is asked to put the configured place in front of what it has
        // rather than in place of it.
        const QString separator = QString(Utils::HostOsInfo::pathListSeparator());
        commands << "eval \"set debug-file-directory %s" + separator + "%s\", \"" + location
                        + "\", $_gdb_setting_str(\"debug-file-directory\")"
                 << "settings append target.debug-file-search-paths " + location;
    }
    for (const QString &command : commands) {
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
}

// Reading the symbols again for every session is what the cache is there to
// spare, and a fork that is detached from cannot be debugged, so a session
// configured for either says so in the debugger's own words. The index cache
// is gdb's alone, and the spelling is the one gdb 13 made the command: an
// older one answers with an error nobody has to read.
void DapImpl::configureTheSymbolIndexAndForks()
{
    QStringList commands;
    if (m_startData.useIndexCache)
        commands << "set index-cache enabled on";
    if (m_startData.multiInferior) {
        commands << "set detach-on-fork off"
                 << "settings set target.process.follow-fork-mode child";
    }
    for (const QString &command : commands) {
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
}

bool DapImpl::isCoreSession() const
{
    return std::holds_alternative<AttachToCoreData>(m_startData.inferiorStartData);
}

// The protocol has no notion of a core file, and gdb's adapter can neither
// launch nor attach to one. Its console can load one, and what the requests
// read after that is the state the core recorded. There is no launch, so there
// is no configuration to be done either.
void DapImpl::loadCore()
{
    const auto &core = std::get<AttachToCoreData>(m_startData.inferiorStartData);
    if (m_startData.adapterId != "gdb") {
        reportUnsupported(Tr::tr("loading a core file"));
        emit inferiorEvent(InferiorEvent::EngineRunFailed);
        return;
    }
    if (!core.executable.isEmpty()) {
        const QString command = "file " + quotedPath(core.executable.path());
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
    // Work around gdb's adapter taking the threads a core brings for a process
    // that runs: nothing stops, so it refuses every later request that reads
    // them. The core is loaded in the one request that is still let through.
    // "core-file" takes the rest of the line verbatim and does not strip
    // quotes, so the path travels unquoted; the hex encoding keeps it intact.
    const QString command = "python gdb.execute('core-file ' + bytes.fromhex('"
                            + toHex(core.coreFile.path()) + "').decode()); "
                            "import gdb.dap.events; gdb.dap.events.inferior_running = False; "
                            "print(gdb.selected_thread().global_num "
                            "if gdb.selected_thread() else 1)";
    sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (!answer) {
            emit message(Tr::tr("The core file could not be loaded: %1").arg(answer.error()),
                         LogError);
            emit inferiorEvent(InferiorEvent::EngineRunFailed);
            return;
        }
        bool ok = false;
        const int threadId = answer->value("result").toString().trimmed().toInt(&ok);
        m_currentThreadId = ok ? threadId : 1;
        m_runReported = true;
        emit inferiorEvent(InferiorEvent::RunOkAndInferiorUnrunnable);
    });
}

void DapImpl::postLaunchOrAttach()
{
    // The configuration is the adapter's own schema, so it travels as it came.
    QJsonObject configuration = m_startData.configuration;
    // Work around gdb before 16 loading the program of a launch with an
    // unquoted "file" command, which a path with a space breaks: the program
    // is loaded with a quoted one first, and the launch then runs what is
    // loaded.
    const QString program = configuration.value("program").toString();
    if (!m_startData.attach && m_startData.adapterId == "gdb"
        && program.contains(QRegularExpression("\\s"))) {
        QString quoted = program;
        quoted.replace('\\', "\\\\").replace('"', "\\\"");
        sendCustomRequest("evaluate",
                          QJsonObject{{"expression", QString("file \"" + quoted + "\"")},
                                      {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
        configuration.remove("program");
    }
    postRequest(m_startData.attach ? "attach" : "launch", configuration);
}

// The end of the setup, which is a running debuggee for a launch and the stop
// attaching caused for an attach.
void DapImpl::reportRunStarted(bool running)
{
    reportRunning(true);
    emit inferiorEvent(running ? InferiorEvent::RunAndInferiorRunOk
                               : InferiorEvent::RunAndInferiorStopOk);
    m_inferiorRunning = running;
    m_runReported = true;
    // Whatever became of the debuggee while the launch was in flight is only
    // reportable now that the run itself has been.
    if (m_pendingResult) {
        const InferiorResultData result = *m_pendingResult;
        m_pendingResult.reset();
        m_inferiorDoneReported = true;
        emit inferiorDone(result);
    }
}

void DapImpl::shutdownInferior(ShutdownMode mode)
{
    if (!m_client) {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
        return;
    }
    m_shuttingDown = true;
    if (isCoreSession()) {
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
        return;
    }
    if (mode == ShutdownMode::Detach) {
        sendDetach();
    } else if (m_client->capabilities().supportsTerminateRequest) {
        postRequest("terminate", QJsonObject{{"restart", false}});
    } else {
        sendDisconnect(true);
    }
}

void DapImpl::shutdownEngine()
{
    if (m_client) {
        // DapClient::sendDisconnect() takes the debuggee with it, which is the
        // one thing a session that detached must not do.
        if (!m_detaching)
            m_client->sendDisconnect();
        m_client->dataProvider()->kill();
    }
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void DapImpl::execute(const ExecutionRequest &request)
{
    QTC_ASSERT(m_client, return);

    const auto stepArguments = [this](bool byInstruction) {
        QJsonObject args{{"threadId", m_currentThreadId}};
        // Stepping by instruction rather than by line is a granularity the
        // protocol takes on a step, and an optional one: an adapter that
        // announces none would take the step over a line instead.
        if (byInstruction) {
            if (m_client->capabilities().supportsSteppingGranularity)
                args.insert("granularity", "instruction");
            else
                reportUnsupported(Tr::tr("stepping by instruction"));
        }
        return args;
    };

    switch (request.command) {
    case ExecutionCommand::Continue:
        // A resume the engine asked for before it heard that the debuggee had
        // ended. There is nothing left for the adapter to continue, and a core
        // never had anything.
        if (m_inferiorDoneReported || isCoreSession()) {
            emit inferiorEvent(InferiorEvent::InferiorIll);
            return;
        }
        if (request.reverse && !m_client->capabilities().supportsStepBack) {
            m_stopRequested = false;
            m_stepRequested = false;
            runConsoleCommand("reverse-continue", Tr::tr("running the debuggee backwards"));
            return;
        }
        m_stopRequested = false;
        m_stepRequested = false;
        reportRunRequested();
        m_resumeRequestPending = true;
        if (request.reverse)
            postRequest("reverseContinue", QJsonObject{{"threadId", m_currentThreadId}});
        else
            m_client->sendContinue(m_currentThreadId);
        return;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            // A stop taken to get a breakpoint into an adapter that takes none
            // while the debuggee runs is the stop that was asked for here, so
            // the debuggee stays where it is.
            m_resumeAfterBreakpointStop = false;
            emit inferiorEvent(InferiorEvent::StopOk);
            return;
        }
        m_stopRequested = true;
        // The resume this is meant to stop has not been answered yet, so the
        // adapter has not started the debuggee and has nothing to interrupt. A
        // request to stop it now is dropped, so it goes out with the answer.
        if (m_resumeRequestPending) {
            m_interruptWhenResumed = true;
            return;
        }
        sendInterrupt();
        return;
    case ExecutionCommand::StepIn:
        if (request.reverse) {
            // Backwards, the protocol knows the line a step back lands on and
            // nothing about the functions on the way, so the console is what
            // steps into one.
            m_stopRequested = false;
            m_stepRequested = false;
            runConsoleCommand("reverse-step", Tr::tr("stepping back into a function"));
            return;
        }
        m_stepRequested = true;
        reportRunRequested();
        m_resumeRequestPending = true;
        postRequest("stepIn", stepArguments(request.flag));
        if (!request.flag)
            checkLineStep("stepIn", stepArguments(false));
        return;
    case ExecutionCommand::StepOver:
        if (request.reverse && !m_client->capabilities().supportsStepBack) {
            m_stopRequested = false;
            m_stepRequested = false;
            runConsoleCommand("reverse-next", Tr::tr("stepping the debuggee backwards"));
            return;
        }
        m_stepRequested = true;
        reportRunRequested();
        m_resumeRequestPending = true;
        postRequest(request.reverse ? QLatin1String("stepBack") : QLatin1String("next"),
                    stepArguments(request.flag));
        if (!request.reverse && !request.flag)
            checkLineStep("next", stepArguments(false));
        return;
    case ExecutionCommand::StepOut:
        if (request.reverse) {
            m_stopRequested = false;
            m_stepRequested = false;
            runConsoleCommand("reverse-finish", Tr::tr("stepping back out of a function"));
            return;
        }
        m_stepRequested = true;
        reportRunRequested();
        m_resumeRequestPending = true;
        m_client->sendStepOut(m_currentThreadId);
        return;
    case ExecutionCommand::Detach:
        sendDetach();
        return;
    case ExecutionCommand::Abort:
        // The abort is what a debugger that stopped answering is left with, so
        // asking it to terminate itself would wait for the reply that is not
        // coming.
        m_client->dataProvider()->kill();
        return;
    case ExecutionCommand::JumpToLine:
        if (!m_client->capabilities().supportsGotoTargetsRequest) {
            jumpOverTheConsole(request.context);
            return;
        }
        m_jumpLine = request.context.textPosition.line;
        postRequest("gotoTargets",
                    QJsonObject{{"source",
                                 QJsonObject{{"path", reportedSourcePath(
                                                          request.context.fileName.path())}}},
                                {"line", request.context.textPosition.line}});
        return;
    case ExecutionCommand::RunToLine: {
        // The protocol has neither a temporary breakpoint nor a run to a
        // location, so the target gets a breakpoint of its own, taken back
        // once it has been hit.
        Breakpoint breakpoint;
        breakpoint.params.type = BreakpointByFileAndLine;
        breakpoint.params.fileName = request.context.fileName;
        breakpoint.params.textPosition = request.context.textPosition;
        breakpoint.internal = true;
        breakpoint.oneShot = true;
        m_sourceBreakpoints[request.context.fileName].prepend(breakpoint);
        sendBreakpointsFor(request.context.fileName);
        resumeForRunTo();
        return;
    }
    case ExecutionCommand::RunToFunction:
        addInternalFunctionBreakpoint(request.functionName, true);
        sendFunctionBreakpoints();
        resumeForRunTo();
        return;
    case ExecutionCommand::RepeatLastCommand:
        if (m_lastLocalsRequest)
            refresh(*m_lastLocalsRequest);
        return;
    case ExecutionCommand::ResetInferior:
        if (m_startData.attach
                || m_startData.adapter.kind != DapAdapterDescriptor::Kind::Executable) {
            // Nothing here to put back: the debuggee was somebody else's, or
            // the adapter holding it is not ours to start.
            reportUnsupported(Tr::tr("Restarting the debuggee"));
            return;
        }
        // The debuggee that comes back has nothing to do with what the runtime
        // said about the one being killed.
        m_sawTerminateMessage = false;
        // Whatever has to happen while the debuggee that ran is still there.
        runUserCommands(m_startData.userCommands.forReset);
        reportRunRequested();
        reportRunResult(true);
        if (m_client->capabilities().supportsRestartRequest) {
            postRequest("restart", QJsonObject{{"arguments", m_startData.configuration}});
            return;
        }
        m_isResetRestart = true;
        m_client->sendDisconnect();
        return;
    case ExecutionCommand::Return:
        returnOverTheConsole();
        return;
    case ExecutionCommand::RecordReverse:
        if (m_client->capabilities().supportsStepBack) {
            // An adapter that walks the debuggee backwards records by itself.
            reportUnsupported(Tr::tr("recording the execution to walk it backwards"));
            return;
        }
        m_recordingActive = request.flag;
        runConsoleCommand(request.flag ? QLatin1String("record full")
                                       : QLatin1String("record stop"),
                          Tr::tr("recording the execution to walk it backwards"));
        return;
    }
}

void DapImpl::sendDetach()
{
    m_detaching = true;
    sendDisconnect(false);
}

void DapImpl::sendDisconnect(bool terminateDebuggee)
{
    QTC_ASSERT(m_client, return);
    QJsonObject arguments{{"restart", false}};
    if (m_client->capabilities().supportTerminateDebuggee) {
        arguments.insert("terminateDebuggee", terminateDebuggee);
    } else if (terminateDebuggee) {
        reportUnsupported(Tr::tr("taking the inferior down on a disconnect"));
    } else {
        // An adapter that does not take the flag does what it thinks best, and
        // what it thinks best for a debuggee it launched is to end it.
        reportUnsupported(Tr::tr("leaving the inferior running on a detach"));
    }
    postRequest("disconnect", arguments);
}

void DapImpl::addBreakpointConditions(QJsonObject &item, const BreakpointParameters &params)
{
    QTC_ASSERT(m_client, return);
    if (!params.condition.isEmpty()) {
        if (m_client->capabilities().supportsConditionalBreakpoints)
            item.insert("condition", params.condition);
        else
            reportUnsupported(Tr::tr("a condition on a breakpoint"));
    }
    if (params.ignoreCount > 0) {
        if (m_client->capabilities().supportsHitConditionalBreakpoints)
            item.insert("hitCondition", QString::number(params.ignoreCount));
        else
            reportUnsupported(Tr::tr("a hit count on a breakpoint"));
    }
}

// A breakpoint of the protocol's carries a condition and a log message, but no
// command of the user's, and the debugger keeps the commands with the
// breakpoint it made: the console the adapter offers is what puts them there.
void DapImpl::setBreakpointCommands(const QString &adapterId, const QString &command)
{
    QString script = "commands " + adapterId + "\n" + command + "\nend";
    script.replace('\\', "\\\\");
    script.replace('"', "\\\"");
    script.replace('\n', "\\n");
    const QString expression = "python gdb.execute(\"" + script + "\")";
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (!answer)
            reportUnsupported(Tr::tr("a command on a breakpoint"));
    });
}

// gdb has no linespec for "this function in that shared object", so the
// locations elsewhere are disabled, also the ones a library loaded later adds.
// This goes out ahead of the breakpoints, whose numbers are not known yet, and
// finds them by the function they are on.
void DapImpl::sendBreakpointModules()
{
    QHash<QString, QString> modules;
    for (const Breakpoint &breakpoint : std::as_const(m_functionBreakpoints)) {
        if (!breakpoint.enabled)
            continue;
        const QString &function = breakpoint.params.functionName;
        const auto it = modules.constFind(function);
        if (it == modules.cend()) {
            modules.insert(function, breakpoint.params.module);
        } else if (*it != breakpoint.params.module) {
            reportUnsupported(Tr::tr("breakpoints on one function restricted to different "
                                     "modules"));
            modules[function].clear();
        }
    }
    QStringList entries;
    for (auto it = modules.cbegin(); it != modules.cend(); ++it) {
        if (!it.value().isEmpty()) {
            entries.append("bytes.fromhex('" + toHex(it.key()) + "').decode(): bytes.fromhex('"
                           + toHex(it.value()) + "').decode()");
        }
    }
    if (entries.isEmpty() && !m_breakpointModulesSent)
        return;
    if (m_startData.adapterId != "gdb") {
        reportUnsupported(Tr::tr("breakpoints restricted to a module"));
        return;
    }
    m_breakpointModulesSent = true;
    static const QByteArray script = R"(
if 'qtcBreakpointModules' not in globals():
    import os
    qtcBreakpointModules = {}

    def qtcObjfileName(address):
        progspace = gdb.current_progspace()
        try:
            objfile = progspace.objfile_for_address(address)
            if objfile is not None:
                return objfile.filename
        except AttributeError:
            pass
        return gdb.solib_name(address) or progspace.filename or ''

    def qtcIsInModule(address, module):
        name = os.path.basename(qtcObjfileName(address)).lower()
        module = module.lower()
        return module in (name, name.split('.')[0]) or name.startswith('lib' + module + '.')

    def qtcKeepBreakpointInModule(bp):
        location = (getattr(bp, 'location', None) or '').replace('-function ', '', 1)
        module = qtcBreakpointModules.get(location.strip())
        if not module:
            return
        for location in getattr(bp, 'locations', []):
            if location.enabled and location.address is not None \
                    and not qtcIsInModule(location.address, module):
                location.enabled = False

    gdb.events.breakpoint_created.connect(qtcKeepBreakpointInModule)
    gdb.events.breakpoint_modified.connect(qtcKeepBreakpointInModule)
)";
    const QString expression = "python exec(bytes.fromhex('" + QString::fromLatin1(script.toHex())
                               + "').decode()); qtcBreakpointModules = {" + entries.join(", ")
                               + "}; [qtcKeepBreakpointInModule(bp) for bp in gdb.breakpoints()]";
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (!answer)
            reportUnsupported(Tr::tr("breakpoints restricted to a module"));
    });
}

int DapImpl::sendBreakpointsFor(const FilePath &file)
{
    QTC_ASSERT(m_client, return -1);
    QJsonArray breakpoints;
    for (const Breakpoint &breakpoint : m_sourceBreakpoints.value(file)) {
        if (!breakpoint.enabled)
            continue;
        const BreakpointParameters &params = breakpoint.params;
        QJsonObject item{{"line", params.textPosition.line}};
        if (params.textPosition.column > 0)
            item.insert("column", params.textPosition.column);
        addBreakpointConditions(item, params);
        if (params.tracepoint && !params.message.isEmpty()) {
            if (m_client->capabilities().supportsLogPoints)
                item.insert("logMessage", params.message);
            else
                reportUnsupported(Tr::tr("logging a message instead of stopping"));
        }
        breakpoints.append(item);
    }
    const int seq = m_client->postRequest(
        "setBreakpoints",
        QJsonObject{{"source", QJsonObject{{"path", reportedSourcePath(file.path())},
                                           {"name", file.fileName()}}},
                    {"breakpoints", breakpoints},
                    {"sourceModified", false}});
    if (seq >= 0)
        m_breakpointRequests.insert(seq, file);
    return seq;
}

// The adapter names the exceptions it can break on itself, so what to ask for
// is picked out of what it offered: gdb calls them "throw" and "catch",
// lldb-dap prefixes them with the language.
static QString exceptionFilter(const QStringList &offered, BreakpointType type)
{
    const QString wanted = type == BreakpointAtThrow ? QString("throw") : QString("catch");
    for (const QString &filter : offered) {
        if (filter == wanted || filter.endsWith('_' + wanted))
            return filter;
    }
    return {};
}

void DapImpl::sendExceptionBreakpoints()
{
    QTC_ASSERT(m_client, return);
    const QStringList offered = m_client->capabilities().exceptionBreakpointFilters;
    const bool takesOptions = m_client->capabilities().supportsExceptionFilterOptions;
    QJsonArray filters;
    QJsonArray filterOptions;
    for (const Breakpoint &breakpoint : m_exceptionBreakpoints) {
        if (!breakpoint.enabled)
            continue;
        const QString filter = exceptionFilter(offered, breakpoint.params.type);
        if (filter.isEmpty()) {
            // Nothing the adapter offered means what this breakpoint is.
            reportUnsupported(breakpoint.params.type == BreakpointAtThrow
                                  ? Tr::tr("breaking on a thrown exception")
                                  : Tr::tr("breaking on a caught exception"));
            emit breakpointEvent(breakpoint.requestId, breakpoint.op, false);
            continue;
        }
        if (takesOptions) {
            QJsonObject option{{"filterId", filter}};
            if (!breakpoint.params.condition.isEmpty())
                option.insert("condition", breakpoint.params.condition);
            filterOptions.append(option);
        } else {
            if (!breakpoint.params.condition.isEmpty())
                reportUnsupported(Tr::tr("a condition on an exception breakpoint"));
            filters.append(filter);
        }
    }
    // The two are alternatives, an adapter that takes the options reads those
    // and the plain list has to stay empty to not ask for the same filter twice.
    QJsonObject arguments{{"filters", filters}};
    if (takesOptions)
        arguments.insert("filterOptions", filterOptions);
    m_client->postRequest("setExceptionBreakpoints", arguments);
}

// A command the protocol has no request of its own for, run in the console the
// adapter offers. What was wanted is named where the console refuses it: the
// debugger behind the adapter is not necessarily one that knows the command.
void DapImpl::runConsoleCommand(const QString &command, const QString &what)
{
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", command}, {"context", "repl"}},
                      [this, what](const Utils::Result<QJsonObject> &answer) {
        if (!answer)
            reportUnsupported(what);
    });
}

// gdb's adapter answers a line step it could not take like one it took, and
// says why only in a log of its own: where gdb knows no bounds of the function,
// as in a stub the linker jumps through, the debuggee stays where it was and no
// stop ever follows. Whether it runs tells the two apart, and the adapter
// answers in order, so a step that ended quickly has reported its stop before
// the answer comes. One that went nowhere is taken again by instruction.
void DapImpl::checkLineStep(const QString &command, const QJsonObject &arguments)
{
    if (m_startData.adapterId != "gdb" || !m_client->capabilities().supportsSteppingGranularity)
        return;
    m_lineStepUnchecked = true;
    const QString probe = "python print(gdb.selected_thread().is_running())";
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", probe}, {"context", "repl"}},
                      [this, command, arguments](const Utils::Result<QJsonObject> &answer) {
        if (!std::exchange(m_lineStepUnchecked, false) || !answer)
            return;
        if (answer->value("result").toString().trimmed() != "False")
            return;
        QJsonObject byInstruction = arguments;
        byInstruction.insert("granularity", "instruction");
        m_resumeRequestPending = true;
        postRequest(command, byInstruction);
    });
}

// gdb's adapter answers an attach the kernel refused as a success, having
// logged the refusal where no client sees it. What is left to go by is an
// inferior without a process while the process is still there.
void DapImpl::checkAttached()
{
    const int pid = m_startData.configuration.value("pid").toInt();
    if (m_startData.adapterId != "gdb" || pid <= 0)
        return;
    const QString probe = QString("python import os; print(gdb.selected_inferior().pid,"
                                  " os.path.exists('/proc/%1'))").arg(pid);
    sendCustomRequest("evaluate", QJsonObject{{"expression", probe}, {"context", "repl"}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (!answer || answer->value("result").toString().trimmed() != "0 True")
            return;
        emit startFailed(Tr::tr("Debugger Error"), msgPtraceRefused(false),
                         Key("GdbPtraceRefusedAttach"));
        emit inferiorEvent(InferiorEvent::EngineIll);
    });
}

// Loading symbols is nothing the protocol asks for, while the debugger behind
// the adapter does it over its console, taking the modules by a pattern their
// names match.
void DapImpl::loadSymbols(const QString &pattern, const QString &what)
{
    const QString command = "sharedlibrary " + pattern;
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", command}, {"context", "repl"}},
                      [this, what](const Utils::Result<QJsonObject> &answer) {
        if (!answer) {
            emit message(Tr::tr("Loading the symbols of %1 was refused: %2")
                             .arg(what, answer.error()), LogError);
        }
    });
}

// The adapter is the only one asked where a module is loaded, and gdb's own
// leaves that out. The debugger behind it knows, so where the answer has no
// address the console is asked for the sections, a module reaching from the
// first of them that is in memory to the last.
void DapImpl::reportModules(quint64 requestId, const GdbMi &modules)
{
    const bool complete = Utils::allOf(modules, [](const GdbMi &module) {
        return module["startaddress"].isValid();
    });
    if (complete) {
        emit refreshDataReceived(requestId, RefreshKind::Modules, modules);
        return;
    }
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "maint info sections -all-objects"},
                                  {"context", "repl"}},
                      [this, requestId, modules](const Utils::Result<QJsonObject> &answer) {
        if (!answer) {
            emit refreshDataReceived(requestId, RefreshKind::Modules, modules);
            return;
        }
        const GdbMi ranges = parseGdbModuleRanges(answer->value("result").toString());
        GdbMi all;
        all.m_type = GdbMi::List;
        for (const GdbMi &module : modules) {
            if (module["startaddress"].isValid()) {
                all.addChild(module);
                continue;
            }
            GdbMi filled = module;
            for (const GdbMi &range : ranges) {
                if (range["modulepath"].data() != module["modulepath"].data())
                    continue;
                filled.addChild(range["startaddress"]);
                filled.addChild(range["endaddress"]);
                break;
            }
            all.addChild(filled);
        }
        emit refreshDataReceived(requestId, RefreshKind::Modules, all);
    });
}

// What a module is made of is nothing the protocol asks about, while the
// console the adapter offers reaches the debugger behind it, and the listings
// it writes there are the ones the views are built from.
void DapImpl::fetchModuleSections(quint64 requestId, const FilePath &modulePath)
{
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "maint info sections -all-objects"},
                                  {"context", "repl"}},
                      [this, requestId, modulePath](const Utils::Result<QJsonObject> &answer) {
        if (!answer) {
            emit message(Tr::tr("Cannot fetch the sections of %1.")
                             .arg(modulePath.toUserOutput()), LogError);
            return;
        }
        emit refreshDataReceived(requestId, RefreshKind::ModuleSections,
                                 parseGdbModuleSections(answer->value("result").toString(),
                                                        modulePath));
    });
}

void DapImpl::fetchModuleSymbols(quint64 requestId, const FilePath &modulePath)
{
    auto tempFile = std::make_shared<TemporaryFile>("dapsymbols");
    if (!tempFile->open()) {
        emit message(Tr::tr("Cannot create a temporary file for the symbols of %1.")
                         .arg(modulePath.toUserOutput()), LogWarning);
        emit refreshFailed(requestId, RefreshKind::ModuleSymbols, modulePath);
        return;
    }
    const FilePath listingPath = tempFile->filePath();
    tempFile->close();
    // The listing is long enough that the console would cut it short, so the
    // debugger is told to write it where it can be read back whole.
    const QString expression = "maint print msymbols -objfile " + quotedPath(modulePath.path())
                               + " -- " + quotedPath(listingPath.path());
    sendCustomRequest("evaluate", QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this, requestId, modulePath, listingPath, tempFile](
                          const Utils::Result<QJsonObject> &answer) {
        const Utils::Result<QByteArray> listing = listingPath.fileContents();
        listingPath.removeFile();
        if (!answer || !listing) {
            emit message(Tr::tr("Cannot fetch the symbols of %1.")
                             .arg(modulePath.toUserOutput()), LogError);
            emit refreshFailed(requestId, RefreshKind::ModuleSymbols, modulePath);
            return;
        }
        emit refreshDataReceived(requestId, RefreshKind::ModuleSymbols,
                                 parseGdbModuleSymbols(QString::fromLocal8Bit(*listing),
                                                       modulePath));
    });
}

// An adapter without the request for a jump still has the debugger behind it,
// and its console reaches the command. What the line stands for is counted out
// first: "jump" refuses a line of several addresses, while the stop the jump
// needs to land on takes them all, and that stop would be left behind armed to
// fire as one nobody asked for.
void DapImpl::jumpOverTheConsole(const ContextData &context)
{
    const QString location = reportedSourcePath(context.fileName.path()) + ":"
                             + QString::number(context.textPosition.line);
    const QString expression = "python print(len(gdb.decode_line(\"" + location + "\")[1]))";
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this, context, location](const Utils::Result<QJsonObject> &answer) {
        if (!answer) {
            reportUnsupported(Tr::tr("Jump to Line"));
            return;
        }
        if (answer->value("result").toString().trimmed() != "1") {
            emit message(Tr::tr("Cannot jump to %1, the line stands for several addresses.")
                             .arg(location), LogError);
            return;
        }
        Breakpoint breakpoint;
        breakpoint.params.type = BreakpointByFileAndLine;
        breakpoint.params.fileName = context.fileName;
        breakpoint.params.textPosition = context.textPosition;
        breakpoint.internal = true;
        breakpoint.oneShot = true;
        m_sourceBreakpoints[context.fileName].prepend(breakpoint);
        sendBreakpointsFor(context.fileName);
        m_stopRequested = false;
        m_stepRequested = false;
        // The resume is the command's, not a request's, so the adapter reports
        // it as one of its own, and that is where it is answered.
        postReplCommand("jump " + location);
    });
}

// Leaving a frame without running the rest of it is not in the protocol, and a
// step out is not the same thing: it runs the frame to its end, which is what
// returning from it is meant to avoid. The debugger behind the adapter has the
// command, so the console reaches it. gdb's adapter hands out frame ids it
// remembers until the debuggee runs again, and a return pops a frame without
// running anything, so every id it remembers has to be dropped by hand: left
// alone, the adapter answers "Frame is invalid." to every stack it is asked for
// afterwards.
void DapImpl::returnOverTheConsole()
{
    const QString what = Tr::tr("returning from a function without finishing it");
    emit inferiorEvent(InferiorEvent::RunRequested);
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "return"}, {"context", "repl"}},
                      [this, what](const Utils::Result<QJsonObject> &answer) {
        if (!answer) {
            reportUnsupported(what);
            emit inferiorEvent(InferiorEvent::RunFailed);
            return;
        }
        m_frameIds.clear();
        m_currentFrameId = -1;
        m_registerScopeReference = 0;
        m_registerNamesFetched = false;
        const QString expression = "python import gdb.dap.frames as f; f._clear_frame_ids(None)";
        sendCustomRequest("evaluate",
                          QJsonObject{{"expression", expression}, {"context", "repl"}},
                          [this](const Utils::Result<QJsonObject> &frames) {
            if (!frames) {
                emit message(Tr::tr("The adapter kept the frames from before the return, so "
                                    "the stack stays unreadable until the debuggee runs: %1")
                                 .arg(frames.error()), LogError);
            }
            m_inferiorRunning = false;
            emit inferiorEvent(InferiorEvent::StopOk);
        });
    });
}

void DapImpl::resumeForRunTo()
{
    QTC_ASSERT(m_client, return);
    m_stopRequested = false;
    m_stepRequested = false;
    reportRunRequested();
    m_resumeRequestPending = true;
    m_client->sendContinue(m_currentThreadId);
}

// A stub-owned inferior runs in a console of its own, which is the stub's to
// interrupt: the adapter has no process of its own to reach it through.
void DapImpl::sendInterrupt()
{
    if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData))
        emit interruptTerminalRequested();
    else
        m_client->sendPause();
}

// The adapter saying the debuggee is on its way, which is what an interrupt
// asked for before it had to wait for: only now is there something to stop.
void DapImpl::resumeAnswered(bool success)
{
    m_resumeRequestPending = false;
    if (!std::exchange(m_interruptWhenResumed, false))
        return;
    if (success) {
        sendInterrupt();
        return;
    }
    // The debuggee never left where it was, which is where the interrupt
    // wanted it.
    m_stopRequested = false;
    emit inferiorEvent(InferiorEvent::StopOk);
}

void DapImpl::addInternalFunctionBreakpoint(const QString &function, bool oneShot)
{
    Breakpoint breakpoint;
    breakpoint.params.type = BreakpointByFunction;
    breakpoint.params.functionName = function;
    breakpoint.internal = true;
    breakpoint.oneShot = oneShot;
    m_functionBreakpoints.prepend(breakpoint);
}

int DapImpl::sendFunctionBreakpoints()
{
    QTC_ASSERT(m_client, return -1);
    if (!m_client->capabilities().supportsFunctionBreakpoints) {
        reportUnsupported(Tr::tr("breakpoints by function name"));
        return -1;
    }
    sendBreakpointModules();
    QJsonArray breakpoints;
    for (const Breakpoint &breakpoint : m_functionBreakpoints) {
        if (!breakpoint.enabled)
            continue;
        QJsonObject item{{"name", breakpoint.params.functionName}};
        addBreakpointConditions(item, breakpoint.params);
        breakpoints.append(item);
    }
    const int seq = m_client->postRequest("setFunctionBreakpoints",
                                          QJsonObject{{"breakpoints", breakpoints}});
    if (seq >= 0)
        m_functionBreakpointRequests.insert(seq);
    return seq;
}

int DapImpl::sendInstructionBreakpoints()
{
    QTC_ASSERT(m_client, return -1);
    if (!m_client->capabilities().supportsInstructionBreakpoints) {
        reportUnsupported(Tr::tr("breakpoints by address"));
        return -1;
    }
    QJsonArray breakpoints;
    for (const Breakpoint &breakpoint : m_instructionBreakpoints) {
        if (!breakpoint.enabled)
            continue;
        const BreakpointParameters &params = breakpoint.params;
        QJsonObject item{{"instructionReference", QString("0x%1").arg(params.address, 0, 16)}};
        addBreakpointConditions(item, params);
        breakpoints.append(item);
    }
    const int seq = m_client->postRequest("setInstructionBreakpoints",
                                          QJsonObject{{"breakpoints", breakpoints}});
    if (seq >= 0)
        m_instructionBreakpointRequests.insert(seq);
    return seq;
}

int DapImpl::sendDataBreakpoints()
{
    QTC_ASSERT(m_client, return -1);
    if (!m_client->capabilities().supportsDataBreakpoints) {
        // Nothing goes out over the protocol: what the adapter cannot take is
        // left to the debugger behind it, one console command per watchpoint.
        while (!m_dataBreakpoints.isEmpty()) {
            m_consoleWatchpoints.append(m_dataBreakpoints.takeFirst());
            insertWatchpointOverConsole(m_consoleWatchpoints.constLast());
        }
        return -1;
    }

    // A watchpoint is not expressed by what it watches: the adapter turns that
    // into an opaque id of its own first, and only ids go into the array. The
    // array goes out whole, so it waits for the last id it is missing.
    bool resolving = false;
    for (const Breakpoint &breakpoint : m_dataBreakpoints) {
        if (!breakpoint.enabled || !breakpoint.dataId.isEmpty())
            continue;
        const BreakpointParameters &params = breakpoint.params;
        const bool byAddress = params.type == WatchpointAtAddress;
        QJsonObject arguments{{"name", byAddress
                                           ? QString("0x%1").arg(params.address, 0, 16)
                                           : params.expression}};
        if (byAddress) {
            arguments.insert("asAddress", true);
            if (params.size > 0)
                arguments.insert("bytes", int(params.size));
        } else if (m_currentFrameId >= 0) {
            // What an expression means is the frame it is read in.
            arguments.insert("frameId", m_currentFrameId);
        }
        const int seq = m_client->postRequest("dataBreakpointInfo", arguments);
        if (seq >= 0) {
            m_dataIdRequests.insert(seq, breakpoint.requestId);
            resolving = true;
        }
    }
    if (resolving)
        return -1;

    QJsonArray breakpoints;
    for (const Breakpoint &breakpoint : m_dataBreakpoints) {
        if (!breakpoint.enabled)
            continue;
        QJsonObject item{{"dataId", breakpoint.dataId}};
        addBreakpointConditions(item, breakpoint.params);
        breakpoints.append(item);
    }
    const int seq = m_client->postRequest("setDataBreakpoints",
                                          QJsonObject{{"breakpoints", breakpoints}});
    if (seq >= 0)
        m_dataBreakpointRequests.insert(seq);
    return seq;
}

void DapImpl::handleDataBreakpointInfo(const QJsonObject &response)
{
    const int seq = response.value("request_seq").toInt();
    if (!m_dataIdRequests.contains(seq))
        return;
    const quint64 requestId = m_dataIdRequests.take(seq);
    const QJsonObject body = response.value("body").toObject();
    const QString dataId = body.value("dataId").toString();
    const auto it = std::find_if(m_dataBreakpoints.begin(), m_dataBreakpoints.end(),
                                 [requestId](const Breakpoint &breakpoint) {
        return breakpoint.requestId == requestId;
    });
    if (it == m_dataBreakpoints.end())
        return;
    if (dataId.isEmpty()) {
        // Nothing the adapter can watch: it has no id to name it by, and it
        // says in the description why.
        const BreakpointOp op = it->op;
        m_dataBreakpoints.erase(it);
        if (const QString why = body.value("description").toString(); !why.isEmpty())
            emit message(why, LogError);
        emit breakpointEvent(requestId, op, false);
    } else {
        it->dataId = dataId;
    }
    if (m_dataIdRequests.isEmpty())
        sendDataBreakpoints();
}

QList<DapImpl::Breakpoint> &DapImpl::breakpointArray(BreakpointArray kind, const FilePath &file)
{
    switch (kind) {
    case BreakpointArray::Function:
        return m_functionBreakpoints;
    case BreakpointArray::Instruction:
        return m_instructionBreakpoints;
    case BreakpointArray::Data:
        return m_dataBreakpoints;
    case BreakpointArray::Source:
        break;
    }
    return m_sourceBreakpoints[file];
}

int DapImpl::sendBreakpointArray(BreakpointArray kind, const FilePath &file)
{
    switch (kind) {
    case BreakpointArray::Function:
        return sendFunctionBreakpoints();
    case BreakpointArray::Instruction:
        return sendInstructionBreakpoints();
    case BreakpointArray::Data:
        return sendDataBreakpoints();
    case BreakpointArray::Source:
        break;
    }
    return sendBreakpointsFor(file);
}

// What the debugger behind the adapter calls the event a catchpoint catches.
static QString catchpointKind(BreakpointType type)
{
    switch (type) {
    case BreakpointAtFork:
        return "fork";
    case BreakpointAtExec:
        return "exec";
    case BreakpointAtSysCall:
        return "syscall";
    default:
        return {};
    }
}

// The number the debugger behind the adapter gave the catchpoint it just made,
// out of its "Catchpoint 3 (exec)".
static QString parseCatchpointNumber(const QString &answer)
{
    static const QRegularExpression numberRe("Catchpoint (\\d+)");
    const QRegularExpressionMatch match = numberRe.match(answer);
    return match.hasMatch() ? match.captured(1) : QString();
}

// What a catchpoint number stands for: a fork catchpoint also owns the
// companion the debugger needs for vfork, and the two are taken away and
// switched together.
QStringList DapImpl::catchpointNumbers(const QString &number) const
{
    return QStringList(number) + m_catchpointCompanions.keys(number);
}

// The owner is handed over as a pointer because the two commands can go out
// together, before the first of them has been answered and named.
void DapImpl::insertCatchpointCompanion(const std::shared_ptr<QString> &owner, bool enabled)
{
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "catch vfork"}, {"context", "repl"}},
                      [this, owner, enabled](const Utils::Result<QJsonObject> &answer) {
        const QString number = answer
                ? parseCatchpointNumber(answer->value("result").toString()) : QString();
        if (number.isEmpty())
            return;
        if (owner->isEmpty()) {
            postReplCommand("delete " + number);
            return;
        }
        // The adapter announces it like a breakpoint made elsewhere.
        m_ownBreakpointIds.insert(number);
        m_catchpointCompanions.insert(number, *owner);
        if (!enabled)
            postReplCommand("disable " + number);
    });
}

// A catchpoint lives in the debugger behind the adapter rather than in the
// protocol, so a session started anew has none of them and has to be asked for
// each again. What the model was told a catchpoint is called stays what it was
// told: only the number the new debugger gives it is taken over.
void DapImpl::resendCatchpoints()
{
    const QList<Breakpoint> catchpoints = std::exchange(m_restartedCatchpoints, {});
    for (const Breakpoint &catchpoint : catchpoints) {
        const QString command = "catch " + catchpointKind(catchpoint.params.type);
        const auto number = std::make_shared<QString>();
        sendCustomRequest("evaluate",
                          QJsonObject{{"expression", command}, {"context", "repl"}},
                          [this, catchpoint, number](const Utils::Result<QJsonObject> &answer) {
            *number = answer ? parseCatchpointNumber(answer->value("result").toString()) : QString();
            if (number->isEmpty())
                return;
            m_ownBreakpointIds.insert(*number);
            Breakpoint breakpoint = catchpoint;
            breakpoint.adapterId = *number;
            m_catchpoints.append(breakpoint);
            if (!breakpoint.enabled)
                postReplCommand("disable " + *number);
        });
        // Both commands go out before the debuggee does, as the debugger
        // refuses one while that runs, and the answers come in the order the
        // requests went out, so the companion has its owner named by then.
        if (catchpoint.params.type == BreakpointAtFork)
            insertCatchpointCompanion(number, catchpoint.enabled);
    }
}

// A catchpoint has no place the protocol could put it: no source, no function,
// no datum, only an event to stop on. The debugger behind the adapter takes one
// over its console, and it is also the only one that can name it afterwards, by
// the number it gave it.
void DapImpl::changeCatchpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    if (request.op == BreakpointOp::Insert) {
        const QString kind = catchpointKind(request.params.type);
        const QString command = "catch " + kind;
        const BreakpointParameters params = request.params;
        const int modelId = request.modelId;
        sendCustomRequest("evaluate",
                          QJsonObject{{"expression", command}, {"context", "repl"}},
                          [this, requestId, modelId, kind, params]
                          (const Utils::Result<QJsonObject> &answer) {
            const QString number = answer
                    ? parseCatchpointNumber(answer->value("result").toString()) : QString();
            if (number.isEmpty()) {
                emit breakpointEvent(requestId, BreakpointOp::Insert, false);
                return;
            }
            // The adapter announces it like a breakpoint made elsewhere.
            m_ownBreakpointIds.insert(number);
            Breakpoint breakpoint;
            breakpoint.requestId = requestId;
            breakpoint.modelId = modelId;
            breakpoint.responseId = number;
            breakpoint.adapterId = number;
            breakpoint.params = params;
            breakpoint.enabled = params.enabled;
            m_catchpoints.append(breakpoint);
            if (!params.enabled)
                postReplCommand("disable " + number);
            // vfork is an event of its own for the debugger behind the adapter,
            // and a fork catchpoint is asked for once and expected to take both
            // flavours of the call.
            if (params.type == BreakpointAtFork)
                insertCatchpointCompanion(std::make_shared<QString>(number), params.enabled);
            GdbMi bkpt;
            bkpt.m_type = GdbMi::Tuple;
            bkpt.addChild(constMi("number", number));
            bkpt.addChild(constMi("type", "catchpoint"));
            bkpt.addChild(constMi("catch-type", kind));
            bkpt.addChild(constMi("enabled", params.enabled ? "y" : "n"));
            GdbMi all;
            all.m_type = GdbMi::List;
            all.addChild(bkpt);
            emit breakpointEvent(requestId, BreakpointOp::Insert, true, all);
        });
        return;
    }

    const auto it = std::find_if(m_catchpoints.begin(), m_catchpoints.end(),
                                 [&request](const Breakpoint &breakpoint) {
        if (!request.responseId.isEmpty())
            return breakpoint.responseId == request.responseId;
        return breakpoint.modelId == request.modelId;
    });
    if (it == m_catchpoints.end()) {
        emit breakpointEvent(requestId, request.op, false);
        return;
    }
    QString command;
    const QStringList numbers = catchpointNumbers(it->adapterId);
    if (request.op == BreakpointOp::Remove) {
        command = "delete " + numbers.join(' ');
        for (const QString &companion : m_catchpointCompanions.keys(it->adapterId)) {
            m_ownBreakpointIds.remove(companion);
            m_catchpointCompanions.remove(companion);
        }
        m_catchpoints.erase(it);
    } else {
        command = (request.params.enabled ? "enable " : "disable ") + numbers.join(' ');
        it->enabled = request.params.enabled;
    }
    const BreakpointOp op = request.op;
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", command}, {"context", "repl"}},
                      [this, requestId, op](const Utils::Result<QJsonObject> &answer) {
        emit breakpointEvent(requestId, op, bool(answer));
    });
}

// The number the debugger behind the adapter gave the watchpoint it just made,
// out of its "Hardware watchpoint 2: y".
static QString parseWatchpointNumber(const QString &answer)
{
    static const QRegularExpression numberRe("atchpoint (\\d+)");
    const QRegularExpressionMatch match = numberRe.match(answer);
    return match.hasMatch() ? match.captured(1) : QString();
}

// A watchpoint the adapter announces no data breakpoints for is one the
// debugger behind it still takes, over its console, and it is then also the
// only one that can name it, by the number it gave it.
void DapImpl::insertWatchpointOverConsole(const Breakpoint &breakpoint)
{
    const BreakpointParameters &params = breakpoint.params;
    const QString what = params.type == WatchpointAtAddress
                             ? "*0x" + QString::number(params.address, 16)
                             : params.expression;
    const QString command = "watch " + what;
    const quint64 requestId = breakpoint.requestId;
    ++m_pendingConsoleWatchpoints;
    sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                      [this, requestId, what](const Utils::Result<QJsonObject> &answer) {
        --m_pendingConsoleWatchpoints;
        const QString number = answer
                ? parseWatchpointNumber(answer->value("result").toString()) : QString();
        const auto it = std::find_if(m_consoleWatchpoints.begin(), m_consoleWatchpoints.end(),
                                     [requestId](const Breakpoint &watchpoint) {
            return watchpoint.requestId == requestId;
        });
        if (it == m_consoleWatchpoints.end())
            return;
        if (number.isEmpty()) {
            m_consoleWatchpoints.erase(it);
            // A console that took the command and named no watchpoint is one of
            // a debugger that does not have them.
            if (answer)
                reportUnsupported(Tr::tr("breakpoints on data access"));
            else
                emit message(answer.error(), LogError);
            emit breakpointEvent(requestId, BreakpointOp::Insert, false);
            return;
        }
        // The adapter announces it like a breakpoint made elsewhere.
        m_ownBreakpointIds.insert(number);
        it->responseId = number;
        it->adapterId = number;
        if (!what.startsWith('*'))
            readWhatAWatchpointWatches(number, what, false);
        if (!it->enabled)
            postReplCommand("disable " + number);
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi("number", number));
        if (what.startsWith('*'))
            bkpt.addChild(constMi("addr", what.mid(1)));
        bkpt.addChild(constMi("enabled", it->enabled ? "y" : "n"));
        GdbMi all;
        all.m_type = GdbMi::List;
        all.addChild(bkpt);
        emit breakpointEvent(requestId, BreakpointOp::Insert, true, all);
    });
}

// What a watchpoint of the debugger's own is named by is its number, so a
// change to one goes over the console as well.
void DapImpl::changeWatchpoint(const BreakpointChangeRequest &request)
{
    const quint64 requestId = request.requestId;
    const auto it = std::find_if(m_consoleWatchpoints.begin(), m_consoleWatchpoints.end(),
                                 [&request](const Breakpoint &watchpoint) {
        if (!request.responseId.isEmpty())
            return watchpoint.responseId == request.responseId;
        return watchpoint.modelId == request.modelId;
    });
    if (it == m_consoleWatchpoints.end()) {
        emit breakpointEvent(requestId, request.op, false);
        return;
    }
    QString command;
    if (request.op == BreakpointOp::Remove) {
        command = "delete " + it->responseId;
        m_watchedValues.remove(it->responseId);
        m_consoleWatchpoints.erase(it);
    } else {
        command = (request.params.enabled ? "enable " : "disable ") + it->responseId;
        it->enabled = request.params.enabled;
    }
    const BreakpointOp op = request.op;
    sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                      [this, requestId, op](const Utils::Result<QJsonObject> &answer) {
        emit breakpointEvent(requestId, op, bool(answer));
    });
}

void DapImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    QTC_ASSERT(m_client, return);

    if (request.op == BreakpointOp::EnableSub) {
        // A single location of a breakpoint is nothing the protocol names,
        // while the debugger behind the adapter takes one by its number.
        const QString command = (request.enabled ? "enable " : "disable ") + request.subResponseId;
        const quint64 requestId = request.requestId;
        sendCustomRequest("evaluate",
                          QJsonObject{{"expression", command}, {"context", "repl"}},
                          [this, requestId](const Utils::Result<QJsonObject> &answer) {
            emit breakpointEvent(requestId, BreakpointOp::EnableSub, bool(answer));
        });
        return;
    }

    // A breakpoint at main is one on a function whose name the model does not
    // have: only the start data knows what the entry point is called.
    BreakpointParameters params = request.params;
    if (params.type == BreakpointAtMain) {
        params.type = BreakpointByFunction;
        params.functionName = m_startData.mainFunctionName;
    }
    BreakpointArray kind = BreakpointArray::Source;
    if (params.type == BreakpointByFunction)
        kind = BreakpointArray::Function;
    else if (params.type == BreakpointByAddress)
        kind = BreakpointArray::Instruction;
    else if (params.type == WatchpointAtAddress || params.type == WatchpointAtExpression)
        kind = BreakpointArray::Data;
    FilePath file = params.fileNameForDebugger();
    bool inArray = false;

    // A change names the breakpoint by what the adapter called it and brings no
    // location along, so which array has to go out again is looked up rather
    // than taken from the request.
    const auto named = [&request](const Breakpoint &breakpoint) {
        if (!request.responseId.isEmpty())
            return breakpoint.responseId == request.responseId;
        return !request.params.fileName.isEmpty()
               && breakpoint.modelId == request.modelId;
    };

    if (!catchpointKind(params.type).isEmpty()
        || (request.op != BreakpointOp::Insert && Utils::contains(m_catchpoints, named))) {
        changeCatchpoint(request);
        return;
    }

    if (request.op != BreakpointOp::Insert && Utils::contains(m_consoleWatchpoints, named)) {
        changeWatchpoint(request);
        return;
    }

    // An exception breakpoint has no location: what it is is one of the filters
    // the adapter offered, and the set of them goes out as an array of its own.
    if (params.type == BreakpointAtThrow || params.type == BreakpointAtCatch
        || (request.op != BreakpointOp::Insert
            && Utils::contains(m_exceptionBreakpoints, named))) {
        if (request.op == BreakpointOp::Insert) {
            m_exceptionBreakpoints.append({request.requestId, request.op, request.modelId, {},
                                           params, params.enabled});
        } else {
            const auto it = std::find_if(m_exceptionBreakpoints.begin(),
                                         m_exceptionBreakpoints.end(), named);
            if (it == m_exceptionBreakpoints.end()) {
                emit breakpointEvent(request.requestId, request.op, false);
                return;
            }
            if (request.op == BreakpointOp::Remove) {
                m_exceptionBreakpoints.erase(it);
            } else {
                it->enabled = params.enabled;
                it->requestId = request.requestId;
                it->op = request.op;
            }
        }
        if (m_configured)
            sendExceptionBreakpoints();
        // Only what is in the array gets an answer of its own, and a filter
        // that is off is expressed by leaving it out.
        if (request.op == BreakpointOp::Remove || !params.enabled)
            emit breakpointEvent(request.requestId, request.op, true);
        return;
    }

    if (request.op == BreakpointOp::Insert) {
        QList<Breakpoint> &list = breakpointArray(kind, file);
        list.append({request.requestId, request.op, request.modelId, {}, params,
                     params.enabled, false, params.oneShot});
        inArray = params.enabled;
    } else {
        QList<Breakpoint> *list = nullptr;
        if (Utils::contains(m_functionBreakpoints, named)) {
            kind = BreakpointArray::Function;
            list = &m_functionBreakpoints;
        } else if (Utils::contains(m_instructionBreakpoints, named)) {
            kind = BreakpointArray::Instruction;
            list = &m_instructionBreakpoints;
        } else if (Utils::contains(m_dataBreakpoints, named)) {
            kind = BreakpointArray::Data;
            list = &m_dataBreakpoints;
        } else {
            for (auto it = m_sourceBreakpoints.begin(); it != m_sourceBreakpoints.end(); ++it) {
                if (Utils::contains(*it, named)) {
                    kind = BreakpointArray::Source;
                    file = it.key();
                    list = &*it;
                    break;
                }
            }
        }
        if (!list) {
            emit breakpointEvent(request.requestId, request.op, false);
            return;
        }
        const auto it = std::find_if(list->begin(), list->end(), named);
        if (request.op == BreakpointOp::Remove) {
            list->erase(it);
        } else {
            if (!params.fileName.isEmpty())
                it->params = params;
            it->enabled = params.enabled;
            it->requestId = request.requestId;
            it->op = request.op;
            inArray = it->enabled;
        }
    }

    // Until the adapter says it is ready for configuration, the set is only
    // remembered: DAP takes breakpoints between "initialized" and
    // "configurationDone", not before.
    if (m_configured)
        sendBreakpointArray(kind, file);

    // The answer to a setBreakpoints lists what was sent, so only what is in
    // the array gets a reply of its own. A removal is not, and neither is a
    // disabled breakpoint - DAP expresses that by leaving it out.
    if (!inArray)
        emit breakpointEvent(request.requestId, request.op, true);
}

// The stop an adapter that takes no breakpoint while the debuggee runs needs,
// taken behind the session's back: nothing of it is reported, and the debuggee
// is let go again as soon as the arrays are in. A pause of its own rather than
// the one an interrupt sends, so that the refusal of it is answered here.
void DapImpl::stopForBreakpoints()
{
    m_resumeAfterBreakpointStop = true;
    sendCustomRequest("pause", QJsonObject{{"threadId", m_currentThreadId}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (answer)
            return;
        emit message(Tr::tr("The adapter took no breakpoint while the program was running, "
                            "and refused to stop it: %1").arg(answer.error()), LogError);
        m_resumeAfterBreakpointStop = false;
        const QList<RefusedBreakpointArray> refused = std::exchange(m_breakpointsNeedingAStop, {});
        for (const RefusedBreakpointArray &array : refused)
            reportBreakpointsSet(array.refusal, array.kind, array.file);
    });
}

void DapImpl::resendBreakpointsForTheStop()
{
    const QList<RefusedBreakpointArray> refused = std::exchange(m_breakpointsNeedingAStop, {});
    for (const RefusedBreakpointArray &array : refused) {
        const int seq = sendBreakpointArray(array.kind, array.file);
        if (seq >= 0)
            m_breakpointResends.insert(seq);
        else
            reportBreakpointsSet(array.refusal, array.kind, array.file);
    }
    if (m_breakpointResends.isEmpty() && m_resumeAfterBreakpointStop)
        resumeAfterBreakpointStop();
}

void DapImpl::resumeAfterBreakpointStop()
{
    m_resumeAfterBreakpointStop = false;
    // Claimed before the request goes out, so that the adapter's own report of
    // the resume is not read as a run nobody asked for.
    m_inferiorRunning = true;
    sendCustomRequest("continue", QJsonObject{{"threadId", m_currentThreadId}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (answer)
            return;
        // The session was never told about the stop, so it still believes the
        // debuggee runs: the stop is what it has to hear now.
        m_inferiorRunning = false;
        emit message(Tr::tr("The program was stopped to take a breakpoint and could not be "
                            "resumed: %1").arg(answer.error()), LogError);
        emit inferiorEvent(InferiorEvent::SpontaneousStop);
    });
}

void DapImpl::handleBreakpointsSet(const QJsonObject &response)
{
    const int seq = response.value("request_seq").toInt();
    BreakpointArray kind = BreakpointArray::Source;
    if (m_functionBreakpointRequests.remove(seq))
        kind = BreakpointArray::Function;
    else if (m_instructionBreakpointRequests.remove(seq))
        kind = BreakpointArray::Instruction;
    else if (m_dataBreakpointRequests.remove(seq))
        kind = BreakpointArray::Data;
    const FilePath file = m_breakpointRequests.take(seq);

    // Asking for a breakpoint while the debuggee runs is something the
    // protocol lets an adapter refuse, and what the deferral capability
    // promises is that the insert goes through all the same: the stop the
    // adapter wants is taken rather than handed to the model as a failure.
    if (!response.value("success").toBool() && m_inferiorRunning
        && response.value("message").toString() == "notStopped") {
        // The array goes out whole, so one entry covers every breakpoint in it.
        const auto same = [kind, &file](const RefusedBreakpointArray &other) {
            return other.kind == kind && other.file == file;
        };
        if (!Utils::contains(m_breakpointsNeedingAStop, same)) {
            m_breakpointsNeedingAStop.append({kind, file, response});
            if (m_breakpointsNeedingAStop.size() == 1)
                stopForBreakpoints();
        }
        return;
    }

    reportBreakpointsSet(response, kind, file);
    if (m_breakpointResends.remove(seq) && m_breakpointResends.isEmpty()
        && m_resumeAfterBreakpointStop) {
        resumeAfterBreakpointStop();
    }
}

void DapImpl::reportBreakpointsSet(const QJsonObject &response, BreakpointArray kind,
                                   const FilePath &file)
{
    QList<Breakpoint> &known = breakpointArray(kind, file);
    const QJsonArray reported = response.value("body").toObject()
                                    .value("breakpoints").toArray();

    // The answer is positional: it lists the breakpoints that were sent, in the
    // order they were sent, so only the enabled ones line up with it.
    QList<Breakpoint> sent;
    for (Breakpoint &breakpoint : known) {
        if (!breakpoint.enabled)
            continue;
        // What the adapter calls it is how a later change to it will be named.
        const QJsonObject item = reported.at(sent.size()).toObject();
        if (item.contains("id")) {
            breakpoint.adapterId = QString::number(item.value("id").toInt());
            m_ownBreakpointIds.insert(breakpoint.adapterId);
            // The first answer is what the model hears, and that is the id it
            // keeps: a session started anew numbers them as it pleases.
            if (breakpoint.responseId.isEmpty())
                breakpoint.responseId = breakpoint.adapterId;
            if (!breakpoint.params.command.isEmpty())
                setBreakpointCommands(breakpoint.adapterId, breakpoint.params.command);
        }
        sent.append(breakpoint);
    }

    for (int i = 0; i < sent.size(); ++i) {
        if (sent.at(i).internal)
            continue;
        const QJsonObject item = reported.at(i).toObject();
        const bool verified = item.value("verified").toBool();
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi("number", sent.at(i).responseId));
        if (kind == BreakpointArray::Data) {
            // A watchpoint has no source location, and what it watches is what
            // it was asked for: the answer says no more than that it was taken.
            if (sent.at(i).params.type == WatchpointAtAddress) {
                bkpt.addChild(constMi("addr",
                                      QString("0x%1").arg(sent.at(i).params.address, 0, 16)));
            }
        } else {
            bkpt.addChild(constMi("line", QString::number(item.value("line").toInt())));
            const QString path = localSourcePath(item.value("source").toObject()
                                                     .value("path").toString());
            bkpt.addChild(constMi("file", path));
            bkpt.addChild(constMi("fullname", path));
            if (const QString address = item.value("instructionReference").toString();
                !address.isEmpty()) {
                bkpt.addChild(constMi("addr", address));
            }
            // Read the way gdb writes it, the field is there only for a
            // breakpoint that is still pending, and it names the location that
            // was asked for.
            if (!verified) {
                const BreakpointParameters &params = sent.at(i).params;
                bkpt.addChild(constMi("pending",
                                      params.fileName.isEmpty()
                                          ? params.functionName
                                          : params.fileName.path() + ':'
                                                + QString::number(params.textPosition.line)));
            }
        }
        GdbMi data;
        data.m_type = GdbMi::List;
        data.addChild(bkpt);
        // An unverified breakpoint is one the adapter has taken but not bound
        // yet, which is what a breakpoint set before the program is loaded
        // always is. Only "failed" says it was refused - as does an answer that
        // does not list it at all, which a refused request lists nothing in.
        const bool taken = i < reported.size()
                           && (verified || item.value("reason").toString() != "failed");
        if (sent.at(i).op == BreakpointOp::Insert && kind != BreakpointArray::Data) {
            reportBreakpointInsert(sent.at(i).requestId, taken, verified, data,
                                   sent.at(i).adapterId, sent.at(i).params.threadSpec);
            continue;
        }
        emit breakpointEvent(sent.at(i).requestId, sent.at(i).op, taken, data);
    }
}

// The locations "info breakpoints" lists under one breakpoint, as the
// breakpoints view reads them.
static GdbMi parseBreakpointLocations(const QString &listing, const QString &number)
{
    static const QRegularExpression locationRe(
        "^(\\d+\\.\\d+)\\s+(y|n)\\s+(0x[0-9A-Fa-f]+)(?:\\s+in\\s+(.+?))?"
        "(?:\\s+at\\s+([^:]+):(\\d+))?\\s*$");
    GdbMi locations;
    locations.m_type = GdbMi::List;
    locations.m_name = "locations";
    for (const QString &line : listing.split('\n')) {
        const QRegularExpressionMatch match = locationRe.match(line);
        if (!match.hasMatch() || !match.captured(1).startsWith(number + '.'))
            continue;
        GdbMi location;
        location.m_type = GdbMi::Tuple;
        location.addChild(constMi("number", match.captured(1)));
        location.addChild(constMi("enabled", match.captured(2)));
        location.addChild(constMi("addr", match.captured(3)));
        if (!match.captured(4).isEmpty())
            location.addChild(constMi("func", match.captured(4)));
        if (!match.captured(5).isEmpty()) {
            location.addChild(constMi("file", match.captured(5)));
            location.addChild(constMi("fullname", match.captured(5)));
            location.addChild(constMi("line", match.captured(6)));
        }
        locations.addChild(location);
    }
    return locations;
}

// The thread "info breakpoints" names as the only one a breakpoint stops in.
static QString parseBreakpointThread(const QString &listing)
{
    static const QRegularExpression threadRe("^\\s*stop only in thread (\\d+)");
    for (const QString &line : listing.split('\n')) {
        const QRegularExpressionMatch match = threadRe.match(line);
        if (match.hasMatch())
            return match.captured(1);
    }
    return {};
}

// A breakpoint that resolves to several places, a template instantiated more
// than once at the head of them, is one breakpoint for the protocol, while the
// view gives every place a row of its own. The debugger behind the adapter
// lists them, so its own listing is read where a breakpoint was taken.
void DapImpl::reportBreakpointInsert(quint64 requestId, bool taken, bool verified,
                                     const GdbMi &data, const QString &number, int threadSpec)
{
    // A breakpoint the debugger has not bound yet sits at no place it could
    // list, and the listing would have to wait for a debuggee that is on its
    // way: the answer to it may never come, and the insert would be lost.
    if (!taken || !verified || number.isEmpty()) {
        emit breakpointEvent(requestId, BreakpointOp::Insert, taken, data);
        return;
    }
    // A breakpoint of the protocol names no thread it is the only one to stop
    // in, while the debugger behind the adapter takes one on a breakpoint it
    // has already.
    if (threadSpec > 0) {
        postReplCommand(QString("python bp = [b for b in gdb.breakpoints() "
                                "if b.number == %1][0]; bp.thread = %2")
                            .arg(number).arg(threadSpec));
    }
    const QString listing = "info breakpoints " + number;
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", listing}, {"context", "repl"}},
                      [this, requestId, data, number](const Utils::Result<QJsonObject> &answer) {
        const QString reported = answer ? answer->value("result").toString() : QString();
        const GdbMi locations = parseBreakpointLocations(reported, number);
        const QString thread = parseBreakpointThread(reported);
        const bool multiple = locations.childCount() > 1;
        if (!multiple && thread.isEmpty()) {
            emit breakpointEvent(requestId, BreakpointOp::Insert, true, data);
            return;
        }
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        for (const GdbMi &child : data.childAt(0)) {
            if (!multiple || child.m_name != "addr")
                bkpt.addChild(child);
        }
        if (multiple) {
            bkpt.addChild(constMi("addr", "<MULTIPLE>"));
            bkpt.addChild(locations);
        }
        if (!thread.isEmpty())
            bkpt.addChild(constMi("thread", thread));
        GdbMi all;
        all.m_type = GdbMi::List;
        all.addChild(bkpt);
        emit breakpointEvent(requestId, BreakpointOp::Insert, true, all);
    });
}

const DapImpl::Breakpoint *DapImpl::breakpointForAdapterId(const QString &adapterId) const
{
    for (const QList<Breakpoint> &known : m_sourceBreakpoints) {
        for (const Breakpoint &breakpoint : known) {
            if (breakpoint.adapterId == adapterId)
                return &breakpoint;
        }
    }
    for (const Breakpoint &breakpoint : m_functionBreakpoints) {
        if (breakpoint.adapterId == adapterId)
            return &breakpoint;
    }
    for (const Breakpoint &breakpoint : m_instructionBreakpoints) {
        if (breakpoint.adapterId == adapterId)
            return &breakpoint;
    }
    for (const Breakpoint &breakpoint : m_exceptionBreakpoints) {
        if (breakpoint.adapterId == adapterId)
            return &breakpoint;
    }
    return nullptr;
}

GdbMi DapImpl::breakpointMi(const QJsonObject &item, const QString &number) const
{
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addChild(constMi("number", number.isEmpty()
                                        ? QString::number(item.value("id").toInt()) : number));
    if (item.contains("line"))
        bkpt.addChild(constMi("line", QString::number(item.value("line").toInt())));
    const QString path = localSourcePath(item.value("source").toObject()
                                             .value("path").toString());
    if (!path.isEmpty()) {
        bkpt.addChild(constMi("file", path));
        bkpt.addChild(constMi("fullname", path));
    }
    const QString address = item.value("instructionReference").toString();
    if (address.startsWith("0x"))
        bkpt.addChild(constMi("addr", address));
    return bkpt;
}

// A breakpoint the event places nowhere is not one the event says enough about:
// a catchpoint arrives that way, and what it catches, which is the only thing
// telling it from a breakpoint that will forever stay pending, is in what the
// debugger itself says about it rather than anywhere in the protocol.
// The line the debugger prints about the breakpoint with this number, in its
// fields: "3       catchpoint     keep y                   exception throw".
static QStringList breakpointFields(const QString &answer, const QString &id)
{
    for (const QString &line : answer.split('\n')) {
        const QStringList fields = line.simplified().split(' ');
        if (fields.size() >= 2 && fields.at(0) == id)
            return fields;
    }
    return {};
}

void DapImpl::askWhatAnAlienBreakpointIs(const QJsonObject &item)
{
    const QString id = QString::number(item.value("id").toInt());
    const QString expression = "info breakpoints " + id;
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this, id, item](const Utils::Result<QJsonObject> &answer) {
        GdbMi bkpt = breakpointMi(item);
        const QStringList fields = answer ? breakpointFields(answer->value("result").toString(), id)
                                          : QStringList();
        if (fields.value(1) == "catchpoint") {
            bkpt.addChild(constMi("type", "catchpoint"));
            bkpt.addChild(constMi("catch-type", fields.last()));
        } else if (fields.contains("watchpoint")) {
            // What it watches is the debugger's to know: a stop on it carries
            // the number and nothing else.
            m_alienWatchpoints.insert(id, fields.last());
            bkpt.addChild(constMi("type", fields.at(1) + " watchpoint"));
            bkpt.addChild(constMi("what", fields.last()));
        }
        emit breakpointEvent(0, BreakpointOp::Insert, true, bkpt);
    });
}

void DapImpl::handleBreakpointChanged(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    const QString reason = body.value("reason").toString();
    const QJsonObject item = body.value("breakpoint").toObject();
    const QString responseId = QString::number(item.value("id").toInt());

    // A breakpoint somebody else made, the user in the console for instance:
    // nothing else would ever mention it. The adapter announces its own the
    // same way, so they are told apart by what it called them.
    if (reason == "new") {
        if (m_ownBreakpointIds.contains(responseId))
            return;
        // A watchpoint the console is asked for is announced before the command
        // asking for it is answered, and that answer is the only thing naming
        // it, so it would otherwise be reported twice.
        if (m_pendingConsoleWatchpoints > 0 && !item.contains("line"))
            return;
        if (item.contains("line"))
            emit breakpointEvent(0, BreakpointOp::Insert, true, breakpointMi(item));
        else
            askWhatAnAlienBreakpointIs(item);
        return;
    }
    if (reason == "removed") {
        // A watchpoint on a local is one the debugger deletes itself where the
        // block it watches is left, and nobody asked it to.
        const auto it = std::find_if(m_consoleWatchpoints.begin(), m_consoleWatchpoints.end(),
                                     [&responseId](const Breakpoint &watchpoint) {
            return watchpoint.responseId == responseId;
        });
        const bool deletedItself = it != m_consoleWatchpoints.end();
        if (deletedItself)
            m_consoleWatchpoints.erase(it);
        m_watchedValues.remove(responseId);
        if (m_ownBreakpointIds.remove(responseId) && !deletedItself)
            return;
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addChild(constMi("number", responseId));
        emit breakpointEvent(0, BreakpointOp::Remove, true, bkpt);
        return;
    }
    if (reason != "changed")
        return;

    const Breakpoint *known = breakpointForAdapterId(responseId);
    // Nothing in the model is waiting to hear about a breakpoint of the
    // engine's own making.
    if (known && known->internal)
        return;

    GdbMi bkpt = breakpointMi(item, known ? known->responseId : QString());
    // A field the update does not carry counts as the default rather than as
    // unchanged, so what the event leaves out is filled in from the request.
    if (known) {
        bkpt.addChild(constMi("enabled", known->enabled ? "y" : "n"));
        if (!known->params.condition.isEmpty())
            bkpt.addChild(constMi("cond", known->params.condition));
    }

    GdbMi data;
    data.m_type = GdbMi::List;
    data.addChild(bkpt);
    emit breakpointModified(data);
}

void DapImpl::refresh(const RefreshRequest &request)
{
    QTC_ASSERT(m_client, return);

    switch (request.kind) {
    case RefreshKind::Locals:
        // A watcher added while the debuggee runs has no frame to be evaluated
        // in yet, so the fetch waits for the next stop instead of coming back
        // empty.
        if (m_inferiorRunning) {
            m_deferredLocalsRequest = request;
            return;
        }
        m_lastLocalsRequest = request;
        m_localsRequestId = request.requestId;
        m_expandedINames = request.expandedINames;
        m_autoDerefPointers = request.autoDerefPointers;
        m_derefINames.clear();
        m_partialVariable = request.partialVariable;
        m_locals.clear();
        m_localRoots.clear();
        m_pendingVariables.clear();
        m_variableRequests.clear();
        m_pendingWatchers.clear();
        m_watcherRequests.clear();
        for (const QJsonValue &value : request.watchers) {
            const QJsonObject watcher = value.toObject();
            const QString iname = watcher.value("iname").toString();
            if (!isRequestedLocal(iname))
                continue;
            const QString expression = QString::fromUtf8(
                QByteArray::fromHex(watcher.value("exp").toString().toUtf8()));
            m_pendingWatchers.enqueue({iname, expression});
        }
        if (m_currentFrameId < 0) {
            reportLocals();
            return;
        }
        applyTheStringLengthLimit(request.dumperOptions);
        m_scopesSeq = m_client->scopes(m_currentFrameId);
        return;
    case RefreshKind::FullStack:
        if (const int seq = m_client->stackTrace(m_currentThreadId,
                                                 qMax(request.stackDepthLimit, 0));
            seq >= 0) {
            m_stackTraceRequests.insert(seq, {false, request.requestId});
        }
        return;
    case RefreshKind::Threads:
        if (const int seq = m_client->postRequest("threads"); seq >= 0)
            m_threadRequests.insert(seq, request.requestId);
        return;
    case RefreshKind::FullBacktrace:
        m_backtraceRequestId = request.requestId;
        m_backtrace.clear();
        m_backtraceThreads.clear();
        m_backtraceFramesSeq = -1;
        m_backtraceThreadsSeq = m_client->postRequest("threads");
        return;
    case RefreshKind::Modules:
        if (!m_client->capabilities().supportsModulesRequest) {
            reportUnsupported(Tr::tr("the list of modules"));
            emit refreshDataReceived(request.requestId, request.kind, {});
            return;
        }
        if (const int seq = m_client->postRequest("modules"); seq >= 0)
            m_moduleRequests.insert(seq, request.requestId);
        return;
    case RefreshKind::ModuleSections:
        if (request.path.isEmpty()) {
            // The listing the console answers with names every module the
            // debugger knows of, and picking one out of it needs a name.
            emit message(Tr::tr("Cannot fetch the sections of no module."), LogError);
            return;
        }
        fetchModuleSections(request.requestId, request.path);
        return;
    case RefreshKind::ModuleSymbols:
        if (request.path.isEmpty()) {
            emit message(Tr::tr("Cannot fetch the symbols of no module."), LogError);
            return;
        }
        fetchModuleSymbols(request.requestId, request.path);
        return;
    case RefreshKind::SourceFiles:
        if (!m_client->capabilities().supportsLoadedSourcesRequest) {
            reportUnsupported(Tr::tr("the list of source files"));
            emit refreshDataReceived(request.requestId, request.kind, {});
            return;
        }
        if (const int seq = m_client->postRequest("loadedSources"); seq >= 0)
            m_sourceFilesRequests.insert(seq, request.requestId);
        return;
    case RefreshKind::AllSymbols:
        // Nothing loads symbols over the protocol, while the debugger behind
        // the adapter does it over its console. What the caller is after is
        // what reads them, so the answers they feed come next.
        loadSymbols(".*", Tr::tr("all modules"));
        refresh({request.requestId, RefreshKind::Modules});
        refresh({request.requestId, RefreshKind::FullStack});
        refresh({request.requestId, RefreshKind::Locals});
        return;
    case RefreshKind::StackSymbols:
        if (request.path.isEmpty()) {
            emit message(Tr::tr("Cannot load the symbols of no module."), LogError);
            return;
        }
        // What the debugger matches the module by is a pattern, so a path goes
        // in with whatever could be read as more than itself taken out.
        loadSymbols(dotEscape(request.path.path()), request.path.toUserOutput());
        return;
    case RefreshKind::DebuggingHelpers:
        // There are no dumpers behind a stock adapter, so reloading them is
        // asking it for the values again.
        refresh({request.requestId, RefreshKind::Locals});
        return;
    case RefreshKind::PeripheralRegisters:
        // A peripheral register is not a register of the machine but a word at
        // a known address, so each one is a memory read of its own.
        if (!m_client->capabilities().supportsReadMemoryRequest) {
            reportUnsupported(Tr::tr("reading memory"));
            emit refreshDataReceived(request.requestId, request.kind, {});
            return;
        }
        for (const quint64 address : request.addresses) {
            const QString reference = "0x" + QString::number(address, 16);
            const int seq = m_client->postRequest(
                "readMemory", QJsonObject{{"memoryReference", reference},
                                          {"count", qint64(sizeof(quint32))}});
            if (seq >= 0)
                m_peripheralRequests.insert(seq, {request.requestId, address, sizeof(quint32)});
        }
        return;
    case RefreshKind::Registers:
        // The registers are a scope of the current frame, so they are fetched
        // the way the locals are: the scopes first, then the one that turns
        // out to be them.
        if (m_currentFrameId < 0) {
            emit refreshDataReceived(request.requestId, request.kind, {});
            return;
        }
        m_registersRequestId = request.requestId;
        m_registerScopesSeq = m_client->scopes(m_currentFrameId);
        return;
    case RefreshKind::QmlStack:
        fetchQmlStack(request);
        return;
    default:
        // Symbols and snapshots have no counterpart the protocol defines, so
        // the view is answered with nothing rather than being left waiting.
        emit refreshDataReceived(request.requestId, request.kind, {});
        return;
    }
}

void DapImpl::handleResponse(DapResponseType type, const QJsonObject &response)
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

    if (const DapSessionChannel::Answer answer
        = m_customRequests.take(response.value("request_seq").toInt())) {
        if (success)
            answer(response.value("body").toObject());
        else
            answer(ResultError(response.value("message").toString()));
        return;
    }

    // The handlers below read a body an unsuccessful answer does not have, so
    // the reason it gives is passed on here or nowhere.
    if (!success && !command.isEmpty()) {
        // "notStopped" is the one reason the protocol spells as an identifier
        // to act on rather than as something to read, and it is the one a user
        // runs into, by asking for something while the debuggee runs.
        const QString reason = response.value("message").toString();
        // A breakpoint array refused that way is not reported: the stop it
        // needs is taken below, and the array goes out again there.
        const bool deferred = reason == "notStopped" && m_inferiorRunning
                              && (type == DapResponseType::SetBreakpoints
                                  || type == DapResponseType::SetFunctionBreakpoints
                                  || type == DapResponseType::SetInstructionBreakpoints
                                  || type == DapResponseType::SetDataBreakpoints);
        if (!deferred) {
            emit message(reason == "notStopped"
                             ? Tr::tr("The %1 request needs the program to be stopped.")
                                   .arg(command)
                             : command + ": " + reason,
                         LogError);
        }
    }

    switch (type) {
    case DapResponseType::Initialize:
        if (!success) {
            reportEngineSetup(false);
            return;
        }
        liftTheDebuggerLimits();
        configureTheDebugInfoDaemon();
        configureTheSearchPaths();
        configureTheSymbolIndexAndForks();
        runUserStartupCommands();
        reportEngineSetup(true);
        return;
    case DapResponseType::ConfigurationDone:
        return;
    case DapResponseType::Continue:
    case DapResponseType::ReverseContinue:
        reportRunResult(success);
        m_inferiorRunning = success;
        if (success)
            reportResumed();
        resumeAnswered(success);
        return;
    case DapResponseType::StepIn:
    case DapResponseType::StepOut:
    case DapResponseType::StepOver:
    case DapResponseType::StepBack:
        reportRunResult(success);
        resumeAnswered(success);
        return;
    case DapResponseType::StackTrace:
        handleStackTrace(response);
        return;
    case DapResponseType::Scopes:
        handleScopes(response);
        return;
    case DapResponseType::Variables:
        handleVariables(response);
        return;
    case DapResponseType::Pause:
        if (!success)
            emit inferiorEvent(InferiorEvent::StopFailed);
        return;
    case DapResponseType::SetBreakpoints:
    case DapResponseType::SetFunctionBreakpoints:
    case DapResponseType::SetInstructionBreakpoints:
    case DapResponseType::SetDataBreakpoints:
        handleBreakpointsSet(response);
        return;
    case DapResponseType::Attach:
        // Attaching is the one thing in the protocol that connects to a
        // session somebody else is holding, so it is what the commands for
        // after connecting are for.
        if (success) {
            runUserCommands(m_startData.userCommands.afterConnect);
            runUserCommands(m_startData.userCommands.afterAttach.split('\n'));
            checkAttached();
        }
        Q_FALLTHROUGH();
    case DapResponseType::Launch:
        if (!success) {
            // The run is claimed when the request goes out, so a refusal that
            // comes back after that is the session ending rather than a run
            // that never started.
            emit inferiorEvent(m_runReported ? InferiorEvent::InferiorIll
                                             : InferiorEvent::EngineRunFailed);
        }
        return;
    case DapResponseType::Evaluate:
        if (m_watcherRequests.contains(response.value("request_seq").toInt())) {
            handleWatcher(response);
            return;
        }
        emit message(response.value("body").toObject().value("result").toString(),
                     LogMisc);
        return;
    default:
        break;
    }

    if (command == "threads") {
        const int seq = response.value("request_seq").toInt();
        const QJsonArray items = response.value("body").toObject().value("threads").toArray();
        if (seq == m_backtraceThreadsSeq) {
            m_backtraceThreadsSeq = -1;
            for (const QJsonValue &value : items) {
                const QJsonObject item = value.toObject();
                m_backtraceThreads.enqueue({item.value("id").toInt(),
                                            item.value("name").toString()});
            }
            continueBacktrace();
            return;
        }
        ThreadListRequest pending;
        pending.requestId = m_threadRequests.take(seq);
        for (const QJsonValue &value : items) {
            const QJsonObject item = value.toObject();
            const int threadId = item.value("id").toInt();
            GdbMi thread;
            thread.m_type = GdbMi::Tuple;
            thread.addChild(constMi("id", QString::number(threadId)));
            thread.addChild(constMi("target-id", item.value("name").toString()));
            thread.addChild(constMi("name", item.value("name").toString()));
            thread.addChild(constMi("state", m_inferiorRunning ? "running" : "stopped"));
            // Where the thread stands is a stack of its own, and only a stopped
            // thread has one to ask for.
            if (!m_inferiorRunning) {
                if (const int frameSeq = m_client->stackTrace(threadId, 1); frameSeq >= 0)
                    pending.frameRequests.insert(frameSeq, pending.threads.size());
            }
            pending.threads.append(thread);
        }
        m_threadList = pending;
        if (pending.frameRequests.isEmpty())
            reportThreadList();
        return;
    }
    if (command == "exceptionInfo") {
        if (!success)
            return;
        const QJsonObject info = response.value("body").toObject();
        const QJsonObject details = info.value("details").toObject();
        QString name = info.value("exceptionId").toString();
        if (name.isEmpty())
            name = details.value("typeName").toString();
        QString meaning = info.value("description").toString();
        if (meaning.isEmpty())
            meaning = details.value("message").toString();
        emit signalReceived(name, meaning);
        return;
    }
    if (command == "modules") {
        const quint64 requestId = m_moduleRequests.take(response.value("request_seq").toInt());
        GdbMi modules;
        modules.m_type = GdbMi::List;
        for (const QJsonValue &value : response.value("body").toObject()
                                           .value("modules").toArray()) {
            const QJsonObject item = value.toObject();
            GdbMi module;
            module.m_type = GdbMi::Tuple;
            module.addChild(constMi("modulepath",
                                    item.value("path").toString(item.value("name").toString())));
            // The protocol leaves the spelling of the range open, so take an address
            // on its own as well as a start-end pair, in whatever base it is written.
            const QStringList range = item.value("addressRange").toString()
                                          .split('-', Qt::SkipEmptyParts);
            bool ok = false;
            const quint64 start = range.isEmpty()
                                      ? 0 : range.first().trimmed().toULongLong(&ok, 0);
            if (ok) {
                module.addChild(constMi("startaddress", QString::number(start)));
                if (range.size() > 1) {
                    const quint64 end = range.at(1).trimmed().toULongLong(&ok, 0);
                    if (ok)
                        module.addChild(constMi("endaddress", QString::number(end)));
                }
            }
            // The protocol says nothing about whether a module's symbols were
            // read, only where they came from, so a symbol file the adapter
            // names is the one thing that answers it.
            if (!item.value("symbolFilePath").toString().isEmpty())
                module.addChild(constMi("symbolsread", "Yes"));
            modules.addChild(module);
        }
        reportModules(requestId, modules);
        return;
    }
    if (command == "loadedSources") {
        const quint64 requestId
            = m_sourceFilesRequests.take(response.value("request_seq").toInt());
        GdbMi files;
        files.m_type = GdbMi::List;
        for (const QJsonValue &value : response.value("body").toObject()
                                           .value("sources").toArray()) {
            const QJsonObject item = value.toObject();
            const QString path = localSourcePath(item.value("path").toString());
            GdbMi file;
            file.m_type = GdbMi::Tuple;
            file.addChild(constMi("file", item.value("name").toString()));
            if (!path.isEmpty())
                file.addChild(constMi("fullname", path));
            files.addChild(file);
        }
        emit refreshDataReceived(requestId, RefreshKind::SourceFiles, files);
        return;
    }
    if (command == "setExceptionBreakpoints") {
        // The answer lists the filters in the order they were asked for, if
        // the adapter describes them at all - what it says about one of them
        // is otherwise the outcome of the request as a whole.
        const QStringList offered = m_client->capabilities().exceptionBreakpointFilters;
        const QJsonArray reported = response.value("body").toObject()
                                        .value("breakpoints").toArray();
        int index = 0;
        for (Breakpoint &breakpoint : m_exceptionBreakpoints) {
            if (!breakpoint.enabled
                || exceptionFilter(offered, breakpoint.params.type).isEmpty()) {
                continue;
            }
            const QJsonObject item = reported.at(index++).toObject();
            GdbMi data;
            if (item.contains("id")) {
                // How a later change to it is named, which is all there is to
                // name it by: it has no location of its own.
                breakpoint.adapterId = QString::number(item.value("id").toInt());
                if (breakpoint.responseId.isEmpty())
                    breakpoint.responseId = breakpoint.adapterId;
                GdbMi bkpt;
                bkpt.m_type = GdbMi::Tuple;
                bkpt.addChild(constMi("number", breakpoint.responseId));
                data.m_type = GdbMi::List;
                data.addChild(bkpt);
            }
            emit breakpointEvent(breakpoint.requestId, breakpoint.op, success, data);
        }
        return;
    }
    if (command == "dataBreakpointInfo") {
        handleDataBreakpointInfo(response);
        return;
    }
    if (command == "readMemory") {
        handleReadMemory(response);
        return;
    }
    if (command == "disassemble") {
        if (success)
            handleDisassemble(response);
        return;
    }
    if (m_shuttingDown && (command == "terminate" || command == "disconnect")) {
        // The shutdown the engine asked for is over once the adapter has
        // answered for it, whether or not it could do what was asked.
        emit inferiorEvent(InferiorEvent::ShutdownFinished);
        return;
    }
    if (command == "disconnect" && m_isResetRestart) {
        // The adapter has let go of the session. One that does not leave by
        // itself after that is not left behind either, there is another to put
        // in its place.
        m_client->dataProvider()->kill();
        return;
    }
    if (command == "disconnect" && m_detaching) {
        reportInferiorDone({0, InferiorExitStatus::Detached});
        return;
    }
    if (command == "gotoTargets" && success) {
        const QJsonArray targets = response.value("body").toObject()
                                       .value("targets").toArray();
        if (targets.isEmpty()) {
            // The adapter took the question and has nowhere in that line to go
            // to, which is as much of an answer as a target would have been.
            emit message(Tr::tr("The adapter names no place to jump to in line %1.")
                             .arg(m_jumpLine), LogError);
            return;
        }
        // Where several places in a line can be jumped to, the first is the one
        // the line stands for.
        postRequest("goto", QJsonObject{{"threadId", m_currentThreadId},
                                        {"targetId", targets.first().toObject()
                                                         .value("id").toInt()}});
        return;
    }
}

void DapImpl::handleEvent(DapEventType type, const QJsonObject &event)
{
    switch (type) {
    case DapEventType::Initialized: {
        // The one window in which DAP accepts breakpoints, so whatever was
        // collected while the adapter started goes out now.
        m_configured = true;
        if (isCoreSession()) {
            loadCore();
            return;
        }
        // A session started anew behind a reset has the breakpoints already,
        // the ones made here among them, and the run it stands for has been
        // reported when the reset was asked for.
        const bool restarted = std::exchange(m_isResetRestart, false);
        // The protocol has no temporary breakpoint and no stop at the entry
        // point either, so the setting becomes a breakpoint on the function,
        // taken back once it has been hit.
        if (m_startData.breakOnMain && !restarted)
            addInternalFunctionBreakpoint(m_startData.mainFunctionName, true);
        // The names a namespaced Qt gives these are out of reach: there is no
        // request that would tell what the namespace is.
        if (m_startData.breakOnAbort && !restarted)
            addInternalFunctionBreakpoint("abort", false);
        if (m_startData.breakOnWarning && !restarted) {
            addInternalFunctionBreakpoint("qWarning", false);
            addInternalFunctionBreakpoint("QMessageLogger::warning", false);
        }
        if (m_startData.breakOnFatal && !restarted) {
            addInternalFunctionBreakpoint("qFatal", false);
            addInternalFunctionBreakpoint("QMessageLogger::fatal", false);
        }
        for (auto it = m_sourceBreakpoints.cbegin(); it != m_sourceBreakpoints.cend(); ++it)
            sendBreakpointsFor(it.key());
        if (!m_functionBreakpoints.isEmpty())
            sendFunctionBreakpoints();
        if (!m_instructionBreakpoints.isEmpty())
            sendInstructionBreakpoints();
        if (!m_dataBreakpoints.isEmpty())
            sendDataBreakpoints();
        if (!m_exceptionBreakpoints.isEmpty())
            sendExceptionBreakpoints();
        if (!m_restartedCatchpoints.isEmpty())
            resendCatchpoints();
        // The launch goes out once the breakpoints have, and before the
        // configuration is done: an adapter that starts the debuggee in its
        // launch handler would otherwise run past them, and one that waits
        // for the configuration still has the request in hand by then.
        postLaunchOrAttach();
        // Attaching stops the debuggee, and the adapter reports that stop: it
        // ends the setup, so what the engine hears is the stop rather than a
        // run that never happened.
        if (restarted)
            m_inferiorRunning = true;
        else if (m_startData.attach)
            m_reportsSetupStop = true;
        else
            reportRunStarted();
        // The protocol asks for this request only where the adapter announces
        // it, and one that announces none answers it with an error.
        // Work around gdb before 16 not always answering it while the launch
        // still starts the debuggee, which holds back every later request.
        // It does nothing there: the launch runs the debuggee by itself.
        const bool isOldGdb = m_gdbMajorVersion > 0 && m_gdbMajorVersion < 16;
        if (m_client->capabilities().supportsConfigurationDoneRequest && !isOldGdb)
            m_client->sendConfigurationDone();
        return;
    }
    case DapEventType::Stopped:
        handleStopped(event);
        return;
    case DapEventType::DapBreakpoint:
        handleBreakpointChanged(event);
        return;
    case DapEventType::Exited: {
        reportThreadGroupGone();
        InferiorResultData result;
        result.exitCode = event.value("body").toObject().value("exitCode").toInt();
        result.terminatedByRuntime = std::exchange(m_sawTerminateMessage, false);
        askWhetherASignalTookTheInferior(result);
        return;
    }
    case DapEventType::DapThread: {
        const QJsonObject body = event.value("body").toObject();
        GdbMi data;
        data.m_type = GdbMi::Tuple;
        data.addChild(constMi("id", QString::number(body.value("threadId").toInt())));
        data.addChild(constMi("group-id", m_threadGroupId));
        const bool exited = body.value("reason").toString() == "exited";
        emit threadEvent(exited ? ThreadEvent::Exited : ThreadEvent::Created, data);
        // Attaching to the threads is one of the slow parts of a start, and
        // the protocol says so as it goes.
        if (!exited)
            emit progressMessage(Tr::tr("New thread %1").arg(body.value("threadId").toInt()));
        return;
    }
    case DapEventType::Output: {
        const QJsonObject body = event.value("body").toObject();
        const QString category = body.value("category").toString();
        // "console" is the adapter speaking, not the debuggee - what a log
        // point prints comes that way. "telemetry" is addressed to the client
        // itself, and the protocol asks that the user not be shown it, so the
        // log is as far as it goes.
        const int channel = category == "stderr" ? AppError
                          : (category == "console" || category == "telemetry")
                                ? LogMisc : AppOutput;
        const QString output = body.value("output").toString();
        if (m_startData.adapterId == "gdb" && m_gdbMajorVersion == 0) {
            static const QRegularExpression banner("^GNU gdb .*\\s(\\d+)\\.\\d+");
            const QRegularExpressionMatch match = banner.match(output);
            if (match.hasMatch())
                m_gdbMajorVersion = match.captured(1).toInt();
        }
        // gdb announces a debug info download with one line and then fetches
        // silently, which looks exactly like a debugger that stopped answering.
        // Its own output shares the pipe of the debuggee's.
        if (m_startData.adapterId == "gdb" && output.contains("Downloading")
                && output.contains("separate debug info")) {
            m_debuginfodDownloadInProgress = true;
        }
        if ((channel == AppOutput || channel == AppError) && isTerminateMessage(output))
            m_sawTerminateMessage = true;
        emit message(output, channel);
        return;
    }
    default:
        break;
    }

    const QString name = event.value("event").toString();
    if (name == "continued") {
        if (m_inferiorCallsInFlight > 0)
            return;
        const QJsonObject body = event.value("body").toObject();
        GdbMi runningThread;
        runningThread.m_type = GdbMi::Tuple;
        runningThread.addChild(constMi("thread-id",
                                       body.value("allThreadsContinued").toBool()
                                           ? QString("all")
                                           : QString::number(body.value("threadId").toInt())));
        emit threadEvent(ThreadEvent::Running, runningThread);
        // The adapter resumed on its own - cortex-debug does once it has the
        // target reset, and a launch that runs to an entry point does too -
        // and until that is passed on, the views keep showing the stop it
        // reported before and a Continue goes out to something already
        // running, which the adapter can only refuse.
        // An adapter that resumes an attached debuggee without reporting the
        // stop first ends the setup with the run after all.
        if (std::exchange(m_reportsSetupStop, false)) {
            reportRunStarted();
            return;
        }
        const bool wasRunning = m_inferiorRunning;
        m_inferiorRunning = true;
        reportResumed();
        if (!wasRunning) {
            reportRunRequested();
            reportRunResult(true);
        }
        resumeAnswered(true);
    } else if (name == "terminated") {
        reportThreadGroupGone();
        reportInferiorDone({});
    } else if (name == "process") {
        const qint64 pid = event.value("body").toObject().value("systemProcessId").toInteger();
        if (pid != 0) {
            emit inferiorPidKnown(ProcessHandle(pid));
            m_threadGroupId = QString::number(pid);
            GdbMi group;
            group.m_type = GdbMi::Tuple;
            group.addChild(constMi("id", m_threadGroupId));
            group.addChild(constMi("pid", m_threadGroupId));
            emit threadEvent(ThreadEvent::GroupCreated, group);
        }
    } else if (name == "module") {
        const QJsonObject body = event.value("body").toObject();
        const QJsonObject module = body.value("module").toObject();
        // A module the adapter did not locate is still named, and the name is
        // all there is to tell one from another.
        const QString path = module.value("path").toString(module.value("name").toString());
        GdbMi data;
        data.m_type = GdbMi::Tuple;
        data.addChild(constMi("id", module.value("id").toVariant().toString()));
        data.addChild(constMi("target-name", path));
        data.addChild(constMi("host-name", path));
        const bool removed = body.value("reason").toString() == "removed";
        emit libraryEvent(removed ? LibraryEvent::Unloaded : LibraryEvent::Loaded, data);
        // Reading the symbols of a module is the other one, and a module is
        // announced once they have been read.
        if (!removed)
            emit progressMessage(Tr::tr("Read the symbols of %1").arg(path));
    } else if (name == "progressStart" || name == "progressUpdate"
               || name == "progressEnd") {
        reportProgress(name, event.value("body").toObject());
    }
}

// What an adapter is spending its time on while a request of ours is out. The
// protocol has the client show it and take it away again, while the views here
// have one line for it, so the end is a message like any other.
void DapImpl::reportProgress(const QString &event, const QJsonObject &body)
{
    const QString id = body.value("progressId").toVariant().toString();
    if (event == "progressStart")
        m_progressTitles.insert(id, body.value("title").toString());
    const QString title = m_progressTitles.value(id);
    const QString message = body.value("message").toString();
    QString text = title.isEmpty() || message.isEmpty() ? title + message
                                                        : Tr::tr("%1: %2").arg(title, message);
    if (event == "progressEnd")
        m_progressTitles.remove(id);
    if (text.isEmpty())
        return;
    if (const QJsonValue percentage = body.value("percentage"); percentage.isDouble())
        text = Tr::tr("%1 (%2%)").arg(text).arg(qRound(percentage.toDouble()));
    emit progressMessage(text);
}

// An adapter that leaves the signal out of the stop event still knows it, and
// gdb's own wording for it ("It stopped with signal SIGSEGV, Segmentation
// fault.") carries both the name and the meaning the views want.
void DapImpl::askForTheStoppingSignal(const QString &description)
{
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "info program"}, {"context", "repl"}},
                      [this, description](const Utils::Result<QJsonObject> &answer) {
        QString name;
        QString meaning = description;
        if (answer) {
            static const QString marker = "with signal ";
            const QString reply = answer->value("result").toString();
            const int start = reply.indexOf(marker);
            const QString rest = start < 0 ? QString() : reply.mid(start + marker.size());
            const int comma = rest.indexOf(", ");
            const int end = rest.indexOf('\n');
            if (comma > 0 && end > comma) {
                name = rest.left(comma);
                meaning = rest.mid(comma + 2, end - comma - 2);
                if (meaning.endsWith('.'))
                    meaning.chop(1);
            }
        }
        emit signalReceived(name, meaning);
    });
}

// The breakpoints a stop was made by are what the protocol reports, so the
// hits are counted here: it has no hit count of its own.
// Nothing in the protocol carries a value with a stop on a watchpoint, so what
// the watchpoint saw change is the value read where it was set against the one
// read where it stopped.
void DapImpl::readWhatAWatchpointWatches(const QString &id, const QString &expression, bool report)
{
    QJsonObject arguments{{"expression", expression}, {"context", "watch"}};
    if (m_currentFrameId >= 0)
        arguments.insert("frameId", m_currentFrameId);
    sendCustomRequest("evaluate", arguments,
                      [this, id, expression, report](const Utils::Result<QJsonObject> &answer) {
        const QString before = m_watchedValues.value(id);
        const QString now = answer ? answer->value("result").toString() : QString();
        m_watchedValues.insert(id, now);
        if (report)
            emit watchpointTriggered(id, expression, before, now);
    });
}

// A watchpoint the debugger was asked for behind the adapter's back is named in
// a stop by its number alone, and only the debugger knows what it watches. The
// announcement of such a watchpoint can arrive after the stop on it does.
void DapImpl::reportAlienWatchpointHit(const QString &id)
{
    const auto known = m_alienWatchpoints.constFind(id);
    if (known != m_alienWatchpoints.constEnd()) {
        emit watchpointTriggered(id, *known, {}, {});
        return;
    }
    const QString expression = "info breakpoints " + id;
    sendCustomRequest("evaluate", QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this, id](const Utils::Result<QJsonObject> &answer) {
        if (!answer)
            return;
        const QStringList fields = breakpointFields(answer->value("result").toString(), id);
        if (!fields.contains("watchpoint"))
            return;
        m_alienWatchpoints.insert(id, fields.last());
        emit watchpointTriggered(id, fields.last(), {}, {});
    });
}

void DapImpl::reportBreakpointHits(const QJsonArray &adapterIds)
{
    QStringList ids;
    for (const QJsonValue &id : adapterIds)
        ids.append(QString::number(id.toInt()));
    if (ids.isEmpty())
        return;
    // A stop on the companion of a fork catchpoint belongs to the catchpoint
    // that was asked for.
    for (QString &id : ids)
        id = m_catchpointCompanions.value(id, id);

    GdbMi data;
    data.m_type = GdbMi::List;
    QStringList unknown = ids;
    const auto countHits = [this, &data, &ids, &unknown](QList<Breakpoint> &list, bool watching) {
        for (Breakpoint &breakpoint : list) {
            if (breakpoint.internal || !ids.contains(breakpoint.adapterId))
                continue;
            unknown.removeAll(breakpoint.adapterId);
            ++breakpoint.hitCount;
            // Which breakpoint a stop belongs to is in the stop event only, and a
            // stop the user cannot place is one they have to go looking for. A
            // watchpoint sits at no line at all, so it is the only thing that
            // places its stop, and what it saw change is not in the protocol.
            if (watching) {
                const BreakpointParameters &params = breakpoint.params;
                const QString expression = params.type == WatchpointAtAddress
                        ? QString("*0x%1").arg(params.address, 0, 16)
                        : params.expression;
                if (m_watchedValues.contains(breakpoint.responseId))
                    readWhatAWatchpointWatches(breakpoint.responseId, expression, true);
                else
                    emit watchpointTriggered(breakpoint.responseId, expression, {}, {});
            } else {
                emit breakpointTriggered(breakpoint.responseId,
                                         QString::number(m_currentThreadId));
            }
            GdbMi bkpt;
            bkpt.m_type = GdbMi::Tuple;
            bkpt.addChild(constMi("number", breakpoint.responseId));
            bkpt.addChild(constMi("times", QString::number(breakpoint.hitCount)));
            // A field a modification leaves out counts as the default rather
            // than as unchanged, so the rest of the state goes along.
            bkpt.addChild(constMi("enabled", breakpoint.enabled ? "y" : "n"));
            if (!breakpoint.params.condition.isEmpty())
                bkpt.addChild(constMi("cond", breakpoint.params.condition));
            data.addChild(bkpt);
        }
    };
    for (auto it = m_sourceBreakpoints.begin(); it != m_sourceBreakpoints.end(); ++it)
        countHits(it.value(), false);
    countHits(m_functionBreakpoints, false);
    countHits(m_instructionBreakpoints, false);
    countHits(m_dataBreakpoints, true);
    countHits(m_consoleWatchpoints, true);
    countHits(m_catchpoints, false);
    for (const QString &id : std::as_const(unknown))
        reportAlienWatchpointHit(id);

    if (data.childCount() > 0)
        emit breakpointModified(data);
}

void DapImpl::handleStopped(const QJsonObject &event)
{
    const QJsonObject body = event.value("body").toObject();
    // gdb reports each call made on the debugger's own behalf as a run of its
    // own, which ends where the stop the call was made in already is.
    if (m_inferiorCallsInFlight > 0 && body.value("reason").toString() == "function call")
        return;
    m_currentThreadId = body.value("threadId").toInt();
    m_inferiorRunning = false;
    m_lineStepUnchecked = false;
    // A stop explains itself, whatever the runtime said before it.
    m_sawTerminateMessage = false;
    // The debuggee stopped before the resume was answered, so the interrupt
    // waiting for that answer has what it wanted.
    m_interruptWhenResumed = false;

    // The stop was asked for only to get a breakpoint into an adapter that
    // takes none while the debuggee runs. The session knows nothing of it, so
    // it hears nothing of it either, and the debuggee is let go again as soon
    // as the array is in. A stop for any other reason is the debuggee's own.
    if (!m_breakpointsNeedingAStop.isEmpty() && !m_stopRequested
        && body.value("reason").toString() == "pause") {
        resendBreakpointsForTheStop();
        return;
    }

    GdbMi stoppedThread;
    stoppedThread.m_type = GdbMi::Tuple;
    stoppedThread.addChild(constMi("id", body.value("allThreadsStopped").toBool()
                                             ? QString("all")
                                             : QString::number(m_currentThreadId)));
    emit threadEvent(ThreadEvent::Stopped, stoppedThread);

    const QString reason = body.value("reason").toString();
    // The stub lets go of the inferior with a SIGCONT, which the adapter
    // reports as a stop of its own: it is on the way to running rather than one
    // anybody asked for, and nothing else can stop the debuggee before it, as
    // the SIGCONT is what lets it run at all.
    if (std::exchange(m_expectTerminalTrap, false)
            && (reason == "signal" || reason == "exception")) {
        execute({ExecutionCommand::Continue});
        return;
    }
    // gdb's own DAP support has no reason for a stop that a recording aborted
    // by itself (a "signal 0"): nothing set the field it reports one in, and
    // the fallback it answers with otherwise is meant for a stop made by a
    // "repl" command such as an "attach" typed in by hand, which is not a
    // shape this engine's own requests ever produce. So this is the only way
    // that fallback reaches this engine, and the one place the failure is
    // told apart from an ordinary stop.
    const bool recordingJustFailed = m_recordingActive && reason == "stopped";
    if (recordingJustFailed) {
        m_recordingActive = false;
        emit recordingFailed();
    } else if (reason == "exception" || reason == "signal") {
        // The protocol names no signals. "text" is where an adapter says what
        // it was, if it says anything at all.
        const QString text = body.value("text").toString();
        const QString description = body.value("description").toString();
        if (reason == "signal" && text.isEmpty())
            askForTheStoppingSignal(description);
        // What was thrown is a request of its own where the adapter has one, the
        // stop itself carrying at best a label for it.
        else if (reason == "exception" && m_client->capabilities().supportsExceptionInfoRequest)
            m_client->postRequest("exceptionInfo", QJsonObject{{"threadId", m_currentThreadId}});
        else
            emit signalReceived(text, description);
    }

    const QJsonArray hit = body.value("hitBreakpointIds").toArray();
    reportBreakpointHits(hit);
    const auto wasHit = [&hit](const Breakpoint &breakpoint) {
        if (!breakpoint.oneShot)
            return false;
        for (const QJsonValue &id : hit) {
            if (QString::number(id.toInt()) == breakpoint.adapterId)
                return true;
        }
        return false;
    };
    const auto reportTakenBack = [this](const QList<Breakpoint> &list) {
        for (const Breakpoint &breakpoint : list) {
            if (breakpoint.internal)
                continue;
            GdbMi deleted;
            deleted.m_type = GdbMi::Tuple;
            deleted.addChild(constMi("number", breakpoint.responseId));
            emit breakpointEvent(0, BreakpointOp::Remove, true, deleted);
        }
    };
    if (Utils::contains(m_functionBreakpoints, wasHit)) {
        reportTakenBack(Utils::filtered(m_functionBreakpoints, wasHit));
        Utils::erase(m_functionBreakpoints, wasHit);
        sendFunctionBreakpoints();
    }
    if (Utils::contains(m_instructionBreakpoints, wasHit)) {
        reportTakenBack(Utils::filtered(m_instructionBreakpoints, wasHit));
        Utils::erase(m_instructionBreakpoints, wasHit);
        sendInstructionBreakpoints();
    }
    for (auto it = m_sourceBreakpoints.begin(); it != m_sourceBreakpoints.end(); ++it) {
        if (!Utils::contains(it.value(), wasHit))
            continue;
        reportTakenBack(Utils::filtered(it.value(), wasHit));
        Utils::erase(it.value(), wasHit);
        sendBreakpointsFor(it.key());
    }

    if (!recordingJustFailed && reason != u"exception" && reason != u"signal" && hit.isEmpty())
        emit stopReasonReported(reason);

    // Report the stop only once the location is known, as the other backends do.
    const int seq = m_client->stackTrace(m_currentThreadId, 0);
    if (seq < 0) {
        reportStop();
        return;
    }
    m_stackTraceRequests.insert(seq, {true, 0, m_stepRequested && reason == u"step"});
}

// The debuggee's threads go with the process they ran in, and an adapter that
// reports the end of the session without an exit of its own ends the group
// there.
void DapImpl::reportThreadGroupGone()
{
    if (m_threadGroupId.isEmpty())
        return;
    GdbMi group;
    group.m_type = GdbMi::Tuple;
    group.addChild(constMi("id", m_threadGroupId));
    m_threadGroupId.clear();
    emit threadEvent(ThreadEvent::GroupExited, group);
}

// The protocol's exit event carries an exit code and nothing else, so an
// inferior a signal took down is not told from one that returned. gdb keeps the
// signal in a convenience variable of its own, which is there to be read once
// the exit has happened, and the exit waits for the answer: what follows it is
// the end of the session, which would report the exit first.
void DapImpl::askWhetherASignalTookTheInferior(const InferiorResultData &result)
{
    m_exitAwaitingSignal = result;
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "print $_exitsignal"}, {"context", "repl"}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (!m_exitAwaitingSignal)
            return;
        InferiorResultData reported = *m_exitAwaitingSignal;
        m_exitAwaitingSignal.reset();
        static const QRegularExpression signalNumber("=\\s*([1-9][0-9]*)");
        const QRegularExpressionMatch match
            = signalNumber.match(answer ? answer->value("result").toString() : QString());
        if (!answer || !match.hasMatch()) {
            reportInferiorDone(reported);
            return;
        }
        reported.exitStatus = InferiorExitStatus::Crash;
        m_exitAwaitingSignal = reported;
        askWhatTheExitSignalIsCalled(match.captured(1).toInt());
    });
}

// A number is not what the views say, and the one the debugger keeps is its own
// rather than the host's, so the debugger is asked what it calls it: the answer
// is a table of one row, headed the way the full table is.
void DapImpl::askWhatTheExitSignalIsCalled(int number)
{
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", QString("info signals %1").arg(number)},
                                  {"context", "repl"}},
                      [this](const Utils::Result<QJsonObject> &answer) {
        if (!m_exitAwaitingSignal)
            return;
        InferiorResultData reported = *m_exitAwaitingSignal;
        m_exitAwaitingSignal.reset();
        if (answer) {
            const QStringList lines = answer->value("result").toString().split('\n');
            if (lines.size() > 1)
                reported.signalName = lines.at(1).section(' ', 0, 0);
        }
        reportInferiorDone(reported);
    });
}

// The debugger is what keeps the signal, so one that is gone has taken it with
// it: what the exit event said about the exit is all there is going to be.
void DapImpl::reportTheExitWithoutItsSignal()
{
    if (!m_exitAwaitingSignal)
        return;
    const InferiorResultData result = *m_exitAwaitingSignal;
    m_exitAwaitingSignal.reset();
    reportInferiorDone(result);
}

// An adapter can be done before the engine has been told the run started, and
// the engine only accepts the two in that order.
void DapImpl::reportInferiorDone(InferiorResultData result)
{
    if (m_inferiorDoneReported || m_exitAwaitingSignal)
        return;
    m_inferiorRunning = false;
    // The engine asked for this end. It hears about it as the answer to its own
    // request, and an exit reported on top of that is one it would act on.
    if (m_shuttingDown)
        return;
    // The debuggee a reset took down, which the debuggee taking its place is
    // the answer to.
    if (m_isResetRestart)
        return;
    if (m_detaching)
        result.exitStatus = InferiorExitStatus::Detached;
    if (!m_runReported) {
        m_pendingResult = result;
        return;
    }
    m_inferiorDoneReported = true;
    if (m_deferredLocalsRequest) {
        const quint64 requestId = m_deferredLocalsRequest->requestId;
        m_deferredLocalsRequest.reset();
        emit refreshDataReceived(requestId, RefreshKind::Locals, {});
    }
    emit inferiorDone(result);
}

void DapImpl::reportStop()
{
    if (std::exchange(m_reportsSetupStop, false)) {
        m_stopRequested = false;
        reportRunStarted(false);
        if (std::holds_alternative<AttachToTerminalStubData>(m_startData.inferiorStartData)) {
            // The stub holds the inferior stopped until it is told to let it
            // go, which is only safe once the debugger has it running.
            m_expectTerminalTrap = true;
            execute({ExecutionCommand::Continue});
            emit kickoffTerminalProcessRequested();
            return;
        }
        // A target the server was pointed at is handed over stopped, and only
        // the session knows whether it is meant to run from here.
        if (m_startData.continueAfterAttach || m_startData.continueInsteadOfRun)
            execute({ExecutionCommand::Continue});
        return;
    }
    // The debuggee stopped by itself before the stop asked for to get a
    // breakpoint in arrived. The session hears about this one, so it stays
    // where it is, and the arrays go out while it is there anyway.
    if (!m_breakpointsNeedingAStop.isEmpty()) {
        m_resumeAfterBreakpointStop = false;
        resendBreakpointsForTheStop();
    }

    emit inferiorEvent(m_stopRequested ? InferiorEvent::StopOk
                                       : InferiorEvent::SpontaneousStop);
    m_stopRequested = false;
    const QList<WidgetPick> picks = std::exchange(m_widgetPicksNeedingAStop, {});
    for (const WidgetPick &pick : picks)
        pickWidget(pick);
    if (m_deferredLocalsRequest) {
        const RefreshRequest deferred = *m_deferredLocalsRequest;
        m_deferredLocalsRequest.reset();
        refresh(deferred);
    }
}

void DapImpl::reportThreadList()
{
    QTC_ASSERT(m_threadList, return);
    GdbMi threads;
    threads.m_type = GdbMi::List;
    threads.m_name = "threads";
    for (const GdbMi &thread : std::as_const(m_threadList->threads))
        threads.addChild(thread);
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(threads);
    all.addChild(constMi("current-thread-id", QString::number(m_currentThreadId)));
    const quint64 requestId = m_threadList->requestId;
    m_threadList.reset();
    emit refreshDataReceived(requestId, RefreshKind::Threads, all);
}

void DapImpl::handleStackTrace(const QJsonObject &response)
{
    const int seq = response.value("request_seq").toInt();
    if (seq == m_backtraceFramesSeq) {
        handleBacktraceFrames(response);
        return;
    }
    if (m_threadList && m_threadList->frameRequests.contains(seq)) {
        const int index = m_threadList->frameRequests.take(seq);
        const QJsonObject top = response.value("body").toObject().value("stackFrames")
                                    .toArray().first().toObject();
        if (!top.isEmpty()) {
            GdbMi frame;
            frame.m_type = GdbMi::Tuple;
            frame.m_name = "frame";
            frame.addChild(constMi("level", "0"));
            frame.addChild(constMi("func", top.value("name").toString()));
            frame.addChild(constMi("addr",
                                   top.value("instructionPointerReference").toString()));
            const QString path = localSourcePath(top.value("source").toObject()
                                                     .value("path").toString());
            frame.addChild(constMi("file", path));
            frame.addChild(constMi("fullname", path));
            frame.addChild(constMi("line", QString::number(top.value("line").toInt())));
            m_threadList->threads[index].addChild(frame);
        }
        if (m_threadList->frameRequests.isEmpty())
            reportThreadList();
        return;
    }
    if (!m_stackTraceRequests.contains(seq))
        return;
    const StackTraceRequest request = m_stackTraceRequests.take(seq);
    const QJsonArray frames = response.value("body").toObject()
                                  .value("stackFrames").toArray();

    // A stack the adapter refused is not an empty stack: reported, it would
    // empty the stack view and leave the engine on a frame that is not there.
    if (!response.value("success").toBool()) {
        if (request.reportsStop)
            reportStop();
        return;
    }

    m_frameIds.clear();
    for (const QJsonValue &value : frames)
        m_frameIds.append(value.toObject().value("id").toInt());
    m_currentFrameId = m_frameIds.isEmpty() ? -1 : m_frameIds.first();

    if (request.reportsStop) {
        const QJsonObject top = frames.isEmpty() ? QJsonObject() : frames.first().toObject();
        const int lineNumber = top.value("line").toInt();
        const FilePath fileName
            = FilePath::fromUserInput(localSourcePath(top.value("source").toObject()
                                                          .value("path").toString()));
        // A step that ended in a frame the user did not ask to see is continued
        // rather than reported: out of a function that only forwards, into one
        // that only wraps.
        if (request.fromStep && m_startData.skipKnownFrames) {
            const QString function = top.value("name").toString();
            if (isLeavableFunction(function, fileName.path())) {
                execute({ExecutionCommand::StepOut});
                return;
            }
            if (isSkippableFunction(function, fileName.path())) {
                execute({ExecutionCommand::StepIn});
                return;
            }
        }
        if (lineNumber != 0 && fileName.exists())
            emit locationChanged(fileName, lineNumber);
        reportStoppedLocation(fileName, lineNumber);
        reportStop();
        return;
    }

    GdbMi frameList;
    frameList.m_name = "frames";
    frameList.m_type = GdbMi::List;
    for (int i = 0; i < request.qmlFrames.childCount(); ++i)
        frameList.addChild(request.qmlFrames.childAt(i));
    int level = 0;
    for (const QJsonValue &value : frames) {
        const QJsonObject item = value.toObject();
        GdbMi frame;
        frame.m_type = GdbMi::Tuple;
        frame.addChild(constMi("level", QString::number(level++)));
        frame.addChild(constMi("function", item.value("name").toString()));
        const QString path = localSourcePath(item.value("source").toObject()
                                                 .value("path").toString());
        frame.addChild(constMi("file", path));
        frame.addChild(constMi("fullname", path));
        frame.addChild(constMi("line", QString::number(item.value("line").toInt())));
        frame.addChild(constMi("address",
                               item.value("instructionPointerReference").toString()));
        frame.addChild(constMi("module", dapModuleName(item.value("moduleId"))));
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

void DapImpl::continueBacktrace()
{
    while (!m_backtraceThreads.isEmpty()) {
        const QPair<int, QString> next = m_backtraceThreads.dequeue();
        const int seq = m_client->stackTrace(next.first, 0);
        if (seq >= 0) {
            m_backtraceFramesSeq = seq;
            m_backtrace += QString("Thread %1 (%2):\n").arg(next.first).arg(next.second);
            return;
        }
    }
    emit refreshDataReceived(m_backtraceRequestId, RefreshKind::FullBacktrace,
                             constMi({}, m_backtrace));
}

void DapImpl::handleBacktraceFrames(const QJsonObject &response)
{
    m_backtraceFramesSeq = -1;
    int level = 0;
    for (const QJsonValue &value : response.value("body").toObject()
                                       .value("stackFrames").toArray()) {
        const QJsonObject item = value.toObject();
        QString frame = QString("#%1  %2").arg(level++).arg(item.value("name").toString());
        const QString path = localSourcePath(item.value("source").toObject()
                                                 .value("path").toString());
        if (!path.isEmpty())
            frame += QString(" at %1:%2").arg(path).arg(item.value("line").toInt());
        m_backtrace += frame + '\n';
    }
    m_backtrace += '\n';
    continueBacktrace();
}

void DapImpl::selectThread(const QString &threadId)
{
    m_currentThreadId = threadId.toInt();
}

void DapImpl::activateFrame(int index)
{
    if (index >= 0 && index < m_frameIds.size())
        m_currentFrameId = m_frameIds.at(index);
}

void DapImpl::queueVariables(const QString &iname, int reference)
{
    m_pendingVariables.enqueue({iname, reference});
}

void DapImpl::continueLocalsWalk()
{
    while (!m_pendingVariables.isEmpty()) {
        const QPair<QString, int> next = m_pendingVariables.dequeue();
        const int seq = m_client->postRequest("variables",
                                              QJsonObject{{"variablesReference", next.second}});
        if (seq >= 0) {
            m_variableRequests.insert(seq, next);
            return;
        }
    }
    while (!m_pendingWatchers.isEmpty()) {
        const QPair<QString, QString> next = m_pendingWatchers.dequeue();
        const int seq = m_client->postRequest("evaluate",
                                              QJsonObject{{"expression", next.second},
                                                          {"frameId", m_currentFrameId},
                                                          {"context", "watch"}});
        if (seq >= 0) {
            m_watcherRequests.insert(seq, next);
            return;
        }
    }
    reportLocals();
}

// Whether a scope holds the registers rather than something the Locals view
// shows. Not every adapter sets the hint - cortex-debug names its scopes
// "Local", "Global", "Static: <file>" and "Registers" and hints none of them -
// so the name has to say what the hint does not.
static bool isRegisterScope(const QJsonObject &scope)
{
    const QString hint = scope.value("presentationHint").toString();
    if (!hint.isEmpty())
        return hint == u"registers";
    return scope.value("name").toString().startsWith("register", Qt::CaseInsensitive);
}

void DapImpl::handleScopes(const QJsonObject &response)
{
    const QJsonArray scopes = response.value("body").toObject().value("scopes").toArray();
    // Remembered whichever walk this answers: an assignment to a register
    // names the scope it belongs to, and there is no request that would ask
    // for that scope alone.
    for (const QJsonValue &value : scopes) {
        if (isRegisterScope(value.toObject())) {
            m_registerScopeReference = value.toObject().value("variablesReference").toInt();
            break;
        }
    }

    if (response.value("request_seq").toInt() == m_registerScopesSeq) {
        m_registerScopesSeq = -1;
        if (m_registerScopeReference == 0) {
            emit refreshDataReceived(m_registersRequestId, RefreshKind::Registers, {});
            return;
        }
        m_registerVariablesSeq = m_client->postRequest(
            "variables", QJsonObject{{"variablesReference", m_registerScopeReference}});
        return;
    }

    // A stop can be reported more than once - cortex-debug reports each of
    // them twice - and each report asks for the locals again. Answers to a
    // walk that has been started over would otherwise be added to the one
    // running now, listing everything twice.
    if (response.value("request_seq").toInt() != m_scopesSeq)
        return;

    // The adapter offers more than the Locals view is about: registers have a
    // view of their own, and neither globals nor file statics are locals.
    static const QSet<QString> notLocalHints = {"registers", "globals"};
    // Not every adapter sets the hint - cortex-debug names its scopes "Local",
    // "Global", "Static: <file>" and "Registers" and hints none of them - so
    // the name has to say what the hint does not.
    static const QStringList notLocalNames = {"register", "global", "static"};
    for (const QJsonValue &value : response.value("body").toObject()
                                       .value("scopes").toArray()) {
        const QJsonObject scope = value.toObject();
        const QString hint = scope.value("presentationHint").toString();
        if (!hint.isEmpty()) {
            if (notLocalHints.contains(hint))
                continue;
        } else {
            const QString name = scope.value("name").toString().toLower();
            if (Utils::anyOf(notLocalNames, [&name](const QString &prefix) {
                    return name.startsWith(prefix);
                })) {
                continue;
            }
        }
        const int reference = scope.value("variablesReference").toInt();
        if (reference != 0)
            queueVariables("local", reference);
    }
    continueLocalsWalk();
}

// A register as the view reads it, which parses the value as hexadecimal,
// while an adapter prints what it likes: gdb answers most registers in decimal,
// the pointers in hex with the symbol they point at appended, and the flag
// words as a list of the flags that are set.
// The number of bytes a register type name spells out, zero if it does not.
static int registerWidth(const QString &typeName)
{
    static const QRegularExpression bits("^(?:u?int|vec)(\\d+)(?:_t)?$");
    const QRegularExpressionMatch match = bits.match(typeName);
    return match.hasMatch() ? match.captured(1).toInt() / 8 : 0;
}

static QString registerValue(const QString &reported)
{
    const QString text = reported.section(' ', 0, 0);
    bool ok = false;
    if (const qulonglong number = text.toULongLong(&ok, 0); ok)
        return "0x" + QString::number(number, 16);
    if (const qlonglong number = text.toLongLong(&ok, 0); ok)
        return "0x" + QString::number(qulonglong(number), 16);
    return reported;
}

// The listing "maint print register-groups" writes, as the groups each
// register it names is in.
static QHash<QString, QString> parseRegisterGroups(const QString &listing)
{
    QHash<QString, QString> groups;
    const QStringList lines = listing.split('\n');
    for (int i = 1; i < lines.size(); ++i) {
        const QStringList parts = lines.at(i).split(' ', Qt::SkipEmptyParts);
        if (parts.size() > 6)
            groups.insert(parts.at(0), parts.at(6));
    }
    return groups;
}

static GdbMi registersWithGroups(const GdbMi &registers, const QHash<QString, QString> &groups)
{
    if (groups.isEmpty())
        return registers;
    GdbMi result;
    result.m_type = GdbMi::List;
    for (const GdbMi &reg : registers) {
        GdbMi filled = reg;
        if (const auto it = groups.constFind(reg["name"].data()); it != groups.constEnd())
            filled.addChild(constMi("groups", *it));
        result.addChild(filled);
    }
    return result;
}

// Which group a register is in is nothing the protocol reports, while the view
// sorts by it. The debugger behind the adapter has the listing, and what it
// says holds for the whole session, so it is asked once.
void DapImpl::reportRegisters(quint64 requestId, const GdbMi &registers)
{
    if (m_registerGroupsFetched) {
        emit refreshDataReceived(requestId, RefreshKind::Registers,
                                 registersWithGroups(registers, m_registerGroups));
        return;
    }
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "maint print register-groups"},
                                  {"context", "repl"}},
                      [this, requestId, registers](const Utils::Result<QJsonObject> &answer) {
        m_registerGroupsFetched = true;
        if (answer)
            m_registerGroups = parseRegisterGroups(answer->value("result").toString());
        emit refreshDataReceived(requestId, RefreshKind::Registers,
                                 registersWithGroups(registers, m_registerGroups));
    });
}

void DapImpl::handleVariables(const QJsonObject &response)
{
    if (response.value("request_seq").toInt() == m_registerVariablesSeq) {
        m_registerVariablesSeq = -1;
        m_registerNamesFetched = true;
        GdbMi registers;
        registers.m_type = GdbMi::List;
        for (const QJsonValue &value : response.value("body").toObject()
                                           .value("variables").toArray()) {
            const QJsonObject item = value.toObject();
            GdbMi reg;
            reg.m_type = GdbMi::Tuple;
            const QString type = item.value("type").toString();
            reg.addChild(constMi("name", item.value("name").toString()));
            reg.addChild(constMi("value", registerValue(item.value("value").toString())));
            reg.addChild(constMi("type", gdbRegisterTypeName(type)));
            // The protocol has no field for a register's width, so the only
            // source for it is a type name that spells it out.
            if (const int size = registerWidth(type))
                reg.addChild(constMi("size", QString::number(size)));
            registers.addChild(reg);
        }
        reportRegisters(m_registersRequestId, registers);
        return;
    }

    const QPair<QString, int> request
        = m_variableRequests.take(response.value("request_seq").toInt());
    const QString parent = request.first;
    const QJsonArray variables = response.value("body").toObject()
                                     .value("variables").toArray();

    // A pointer's only child is its pointee, which takes the pointer's place.
    if (m_derefINames.remove(parent) && variables.size() == 1) {
        const QJsonObject item = variables.first().toObject();
        Local &local = m_locals[parent];
        local.type = item.value("type").toString().section('\n', 0, 0);
        local.value = item.value("value").toString();
        local.reference = item.value("variablesReference").toInt();
        local.hasChildren = local.reference != 0;
        local.address = item.value("memoryReference").toString().toULongLong(nullptr, 0);
        local.derefed = true;
        if (local.hasChildren && m_expandedINames.contains(parent))
            queueVariables(parent, local.reference);
        continueLocalsWalk();
        return;
    }

    // Where in the parent this level continues. More than one scope can be
    // locals - arguments are their own scope for some adapters - so the root
    // count carries across the answers rather than restarting per scope.
    int index = parent == "local" ? m_localRoots.size()
                                  : m_locals.value(parent).childINames.size();
    for (const QJsonValue &value : variables) {
        const QJsonObject item = value.toObject();
        Local local;
        // The name is whatever the adapter chose and can hold anything, so the
        // path uses the position instead and keeps the name for display only.
        local.iname = parent + '.' + QString::number(index++);
        local.name = item.value("name").toString();
        // An adapter is free to put more than a type name here - cortex-debug
        // sends a whole hover text - and the type column is one line.
        local.type = item.value("type").toString().section('\n', 0, 0);
        local.value = item.value("value").toString();
        local.reference = item.value("variablesReference").toInt();
        local.parentReference = request.second;
        local.hasChildren = local.reference != 0;
        local.address = item.value("memoryReference").toString().toULongLong(nullptr, 0);

        if (parent == "local")
            m_localRoots.append(local.iname);
        else
            m_locals[parent].childINames.append(local.iname);
        m_locals.insert(local.iname, local);

        queueChildren(local);
    }
    continueLocalsWalk();
}

void DapImpl::handleWatcher(const QJsonObject &response)
{
    const QPair<QString, QString> watcher
        = m_watcherRequests.take(response.value("request_seq").toInt());
    const QJsonObject body = response.value("body").toObject();

    Local local;
    local.iname = watcher.first;
    local.name = watcher.second;
    local.type = body.value("type").toString().section('\n', 0, 0);
    // An expression the adapter could not evaluate has no value of its own, so
    // what it said about it takes its place.
    local.value = response.value("success").toBool() ? body.value("result").toString()
                                                     : response.value("message").toString();
    local.reference = body.value("variablesReference").toInt();
    local.hasChildren = local.reference != 0;
    local.address = body.value("memoryReference").toString().toULongLong(nullptr, 0);
    m_localRoots.append(local.iname);
    m_locals.insert(local.iname, local);
    queueChildren(local);
    continueLocalsWalk();
}

// What the dumpers leave alone: strings, untyped memory, functions, and a
// pointer to a pointer, which is dereferenced a level at a time.
static bool isDerefablePointer(const QString &type, const QString &value)
{
    const QString simplified = type.simplified();
    if (!simplified.endsWith('*') || simplified.contains('('))
        return false;
    static const QRegularExpression notDerefed(
        "^(const |volatile )*(void|char|signed char|unsigned char|wchar_t|char8_t|char16_t"
        "|char32_t)( const| volatile)* ?\\*$|\\*\\s*\\*$");
    if (notDerefed.match(simplified).hasMatch())
        return false;
    return !value.startsWith("0x") || value.section(' ', 0, 0).toULongLong(nullptr, 0) != 0;
}

void DapImpl::queueChildren(const Local &local)
{
    if (!local.hasChildren || !isRequestedLocal(local.iname))
        return;
    if (m_autoDerefPointers && isDerefablePointer(local.type, local.value)) {
        m_derefINames.insert(local.iname);
        queueVariables(local.iname, local.reference);
    } else if (m_expandedINames.contains(local.iname)) {
        queueVariables(local.iname, local.reference);
    }
}

GdbMi DapImpl::localsItem(const QString &iname) const
{
    const Local local = m_locals.value(iname);
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addChild(constMi("iname", local.iname));
    item.addChild(constMi("name", local.name));
    item.addChild(constMi("type", local.type));
    item.addChild(constMi("value", local.value));
    item.addChild(constMi("numchild", local.hasChildren ? "1" : "0"));
    if (local.derefed)
        item.addChild(constMi("autoderefcount", "1"));
    if (local.address != 0)
        item.addChild(constMi("address", QString::number(local.address)));

    if (!local.childINames.isEmpty()) {
        GdbMi children;
        children.m_type = GdbMi::List;
        children.m_name = "children";
        for (const QString &child : local.childINames)
            children.addChild(localsItem(child));
        item.addChild(children);
    }
    return item;
}

bool DapImpl::isRequestedLocal(const QString &iname) const
{
    return m_partialVariable.isEmpty() || iname == m_partialVariable
           || iname.startsWith(m_partialVariable + '.');
}

void DapImpl::reportLocals()
{
    GdbMi data;
    data.m_type = GdbMi::List;
    data.m_name = "data";
    for (const QString &iname : m_localRoots) {
        if (isRequestedLocal(iname))
            data.addChild(localsItem(iname));
    }

    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(data);
    // The view keeps what a partial answer does not talk about.
    all.addChild(constMi("partial", m_partialVariable.isEmpty() ? "0" : "1"));
    emit refreshDataReceived(m_localsRequestId, RefreshKind::Locals, all);
}

void DapImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                           const QByteArray &data)
{
    QTC_ASSERT(m_client, return);
    const QString reference = "0x" + QString::number(addr, 16);

    if (op == MemoryOp::Fetch) {
        if (!m_client->capabilities().supportsReadMemoryRequest) {
            reportUnsupported(Tr::tr("reading memory"));
            return;
        }
        const int seq = m_client->postRequest("readMemory",
                                              QJsonObject{{"memoryReference", reference},
                                                          {"count", qint64(lengthOrSize)}});
        if (seq >= 0)
            m_memoryRequests.insert(seq, {requestId, addr, lengthOrSize});
        return;
    }
    if (!m_client->capabilities().supportsWriteMemoryRequest) {
        reportUnsupported(Tr::tr("writing memory"));
        return;
    }
    postRequest("writeMemory",
                QJsonObject{{"memoryReference", reference},
                            {"data", QString::fromUtf8(data.toBase64())}});
}

void DapImpl::handleReadMemory(const QJsonObject &response)
{
    const int seq = response.value("request_seq").toInt();
    if (const MemoryRequest peripheral = m_peripheralRequests.take(seq); peripheral.length != 0) {
        const QByteArray data = QByteArray::fromBase64(
            response.value("body").toObject().value("data").toString().toUtf8());
        quint32 value = 0;
        memcpy(&value, data.constData(), qMin(data.size(), qsizetype(sizeof(value))));
        GdbMi result;
        result.m_type = GdbMi::Tuple;
        result.addChild(constMi("address", QString::number(peripheral.address)));
        result.addChild(constMi("value", QString::number(value)));
        emit refreshDataReceived(peripheral.requestId, RefreshKind::PeripheralRegisters, result);
        return;
    }

    const MemoryRequest request = m_memoryRequests.take(seq);
    if (request.length == 0)
        return;
    QByteArray data = QByteArray::fromBase64(
        response.value("body").toObject().value("data").toString().toUtf8());
    // A read the adapter refused, or answered only in part, is still an answer:
    // what could not be read reads as zero, as it does in the other backends.
    data.truncate(qsizetype(request.length));
    data.append(QByteArray(qsizetype(request.length) - data.size(), char(0)));
    emit memoryDataReceived(request.requestId, request.address, data);
}

// What an adapter evaluates a function name to names the function's address,
// either in a field of its own or, more commonly, inside the value it prints.
static quint64 addressOfEvaluated(const QJsonObject &body)
{
    const quint64 reference
        = body.value("memoryReference").toString().toULongLong(nullptr, 0);
    if (reference != 0)
        return reference;
    static const QRegularExpression hex("0[xX][0-9a-fA-F]+");
    const QRegularExpressionMatch match = hex.match(body.value("result").toString());
    return match.hasMatch() ? match.captured().toULongLong(nullptr, 0) : 0;
}

// The protocol's disassemble request says nothing about the flavor of the text
// it answers with, so the debugger behind the adapter is told in words of its
// own, ahead of every request, the way gdb's own backend does it. Both
// spellings go out because neither debugger knows the other's.
void DapImpl::setTheDisassemblyFlavor()
{
    const QString flavor = m_startData.intelDisassembly ? QString("intel") : QString("att");
    const QString gdbCommand = "set disassembly-flavor " + flavor;
    const QString lldbCommand = "settings set target.x86-disassembly-flavor " + flavor;
    for (const QString &command : {gdbCommand, lldbCommand}) {
        sendCustomRequest("evaluate", QJsonObject{{"expression", command}, {"context", "repl"}},
                          [](const Utils::Result<QJsonObject> &) {});
    }
}

void DapImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName)
{
    QTC_ASSERT(m_client, return);
    if (!m_client->capabilities().supportsDisassembleRequest) {
        reportUnsupported(Tr::tr("disassembly"));
        return;
    }
    if (address == 0) {
        if (functionName.isEmpty()) {
            emit message(Tr::tr("Disassembling needs an address or a function name."),
                         LogWarning);
            return;
        }
        // The protocol has no request for a function's address, but what the
        // adapter evaluates the function's name to carries it.
        const QString expression = "&" + functionName;
        QJsonObject arguments{{"expression", expression}, {"context", "watch"}};
        if (m_currentFrameId >= 0)
            arguments.insert("frameId", m_currentFrameId);
        sendCustomRequest("evaluate", arguments, [this, requestId, functionName]
                          (const Utils::Result<QJsonObject> &answer) {
            const quint64 found = answer ? addressOfEvaluated(*answer) : 0;
            if (found == 0) {
                emit message(Tr::tr("Disassembling \"%1\" needs its address, which the adapter "
                                    "did not give away.").arg(functionName), LogWarning);
                return;
            }
            fetchDisassembly(requestId, found, functionName);
        });
        return;
    }
    setTheDisassemblyFlavor();
    const QString reference = "0x" + QString::number(address, 16);
    const int seq = m_client->postRequest(
        "disassemble", QJsonObject{{"memoryReference", reference},
                                   {"instructionCount", 100}});
    if (seq >= 0)
        m_disassemblyRequests.insert(seq, {requestId, address});
}

void DapImpl::handleDisassemble(const QJsonObject &response)
{
    const DisassemblyRequest request
        = m_disassemblyRequests.take(response.value("request_seq").toInt());
    DisassemblerLines lines;
    QString function;
    quint64 functionAddress = 0;
    QString sourceFile;
    int sourceLine = 0;
    int bytesLength = 0;
    int locatedInstructions = 0;
    QString unreadableFile;
    int unreadableLine = 0;
    bool unreadableIsUnknownToTheAdapter = false;
    for (const QJsonValue &value : response.value("body").toObject()
                                       .value("instructions").toArray()) {
        const QJsonObject item = value.toObject();
        DisassemblerLine line;
        line.address = item.value("address").toString().toULongLong(nullptr, 0);
        line.data = item.value("instruction").toString();
        line.bytes = item.value("instructionBytes").toString();
        bytesLength = qMax(bytesLength, int(line.bytes.size()));

        // Only the instruction a function starts at is named, so the ones after
        // it belong to the last name seen.
        const QString symbol = item.value("symbol").toString();
        if (!symbol.isEmpty() && symbol != function) {
            function = symbol;
            functionAddress = line.address;
            DisassemblerLine header;
            header.data = "Function: " + symbol;
            lines.appendLine(header);
        }
        line.function = function;
        if (functionAddress != 0 && line.address >= functionAddress)
            line.offset = uint(line.address - functionAddress);

        const QJsonObject location = item.value("location").toObject();
        const QString file = localSourcePath(location.value("path").toString());
        const int number = item.value("line").toInt();
        if (!file.isEmpty() && number != 0) {
            ++locatedInstructions;
            if (file != sourceFile || number != sourceLine) {
                sourceFile = file;
                sourceLine = number;
                const int before = lines.size();
                lines.appendSourceLine(file, number);
                if (lines.size() == before && unreadableFile.isEmpty()) {
                    unreadableFile = file;
                    unreadableLine = number;
                    unreadableIsUnknownToTheAdapter = location.contains("sourceReference");
                }
            }
        }
        lines.appendLine(line);
    }
    // Source lines are what makes a disassembly readable, and both ways of
    // losing them are silent: an answer that names no source at all, and a
    // path that names one this side cannot read.
    if (lines.size() > 0 && locatedInstructions == 0) {
        emit message(Tr::tr("The disassembly the adapter sent names no source line."),
                     LogOutput);
    } else if (!unreadableFile.isEmpty()) {
        if (unreadableIsUnknownToTheAdapter) {
            emit message(Tr::tr("The disassembly names line %1 of \"%2\", which neither the "
                                "adapter nor this side can find.")
                             .arg(unreadableLine).arg(unreadableFile), LogOutput);
        } else {
            emit message(Tr::tr("The disassembly names line %1 of \"%2\", which cannot be "
                                "read here.").arg(unreadableLine).arg(unreadableFile), LogOutput);
        }
    }
    lines.setBytesLength(bytesLength);
    emit disassemblyReceived(request.requestId, lines);
}

void DapImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    QTC_ASSERT(m_client, return);
    if (m_client->capabilities().supportsSetExpression) {
        postRequest("setExpression", QJsonObject{{"expression", expr},
                                                 {"value", value},
                                                 {"frameId", m_currentFrameId}});
        return;
    }
    // Without an expression to assign to, what is left is the name the item
    // was fetched under, which only its own container can resolve.
    const Local local = m_locals.value(item.iname);
    if (m_client->capabilities().supportsSetVariable && local.parentReference != 0) {
        postRequest("setVariable", QJsonObject{{"variablesReference", local.parentReference},
                                               {"name", local.name},
                                               {"value", value}});
        return;
    }
    reportUnsupported(Tr::tr("assigning a value"));
}

void DapImpl::executeDebuggerCommand(const QString &command, const WatchItemData &inspectorItem)
{
    Q_UNUSED(inspectorItem)
    QTC_ASSERT(m_client, return);
    // A command of the user's can move the debugger's selection without
    // resuming anything - up, down, frame, thread - and the views follow the
    // selection wherever it goes. A resume moves it by itself, and the stop
    // that ends it says where to.
    askWhereTheDebuggerSelectionIs(false);
    postReplCommand(command);
    askWhereTheDebuggerSelectionIs(true);
}

// The protocol has no selection of its own: the client is what picks the frame
// a request is about, so a selection the debugger made on its own is only there
// to be asked for, and asking before and after leaves the answers to compare.
void DapImpl::askWhereTheDebuggerSelectionIs(bool report)
{
    sendCustomRequest("evaluate",
                      QJsonObject{{"expression", "python print(\"%d %d\" % "
                                                 "(gdb.selected_thread().num, "
                                                 "gdb.selected_frame().level()))"},
                                  {"context", "repl"}},
                      [this, report](const Utils::Result<QJsonObject> &answer) {
        if (!answer)
            return;
        const QString selection = answer->value("result").toString().trimmed();
        if (selection.isEmpty())
            return;
        const QString moved = std::exchange(m_debuggerSelection, selection);
        if (!report || moved.isEmpty() || moved == selection)
            return;
        GdbMi data;
        data.m_type = GdbMi::Tuple;
        data.addChild(constMi("id", selection.section(' ', 0, 0)));
        emit threadEvent(ThreadEvent::Selected, data);
    });
}

void DapImpl::setRegisterValue(const QString &name, const QString &value)
{
    QTC_ASSERT(m_client, return);
    if (!m_client->capabilities().supportsSetVariable) {
        reportUnsupported(Tr::tr("setting a register"));
        return;
    }
    if (m_registerNamesFetched) {
        postRequest("setVariable", QJsonObject{{"variablesReference", m_registerScopeReference},
                                               {"name", name},
                                               {"value", value}});
        return;
    }
    if (m_currentFrameId < 0) {
        reportUnsupported(Tr::tr("setting a register"));
        return;
    }
    // A register is addressed by the name it has inside the scope that holds
    // it, and an adapter knows the names it has reported: gdb answers one it
    // never listed with an error. Both are what the Registers view asks for,
    // so a register assigned before it ever ran asks for them here.
    sendCustomRequest("scopes", QJsonObject{{"frameId", m_currentFrameId}},
                      [this, name, value](const Utils::Result<QJsonObject> &answer) {
        int reference = 0;
        if (answer) {
            for (const QJsonValue &scope : answer->value("scopes").toArray()) {
                if (isRegisterScope(scope.toObject())) {
                    reference = scope.toObject().value("variablesReference").toInt();
                    break;
                }
            }
        }
        if (reference == 0) {
            reportUnsupported(Tr::tr("setting a register"));
            return;
        }
        m_registerScopeReference = reference;
        sendCustomRequest("variables", QJsonObject{{"variablesReference", reference}},
                          [this, name, value](const Utils::Result<QJsonObject> &listed) {
            if (!listed) {
                reportUnsupported(Tr::tr("setting a register"));
                return;
            }
            m_registerNamesFetched = true;
            setRegisterValue(name, value);
        });
    });
}

void DapImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    const quint32 word = quint32(value);
    const QByteArray data(reinterpret_cast<const char *>(&word), sizeof(word));
    accessMemory(MemoryOp::Change, 0, address, sizeof(word), data);
}

static QString widgetExpression(quint64 address)
{
    return "(QWidget*)0x" + QString::number(address, 16);
}

// The widget is looked up the way the user would do it in the console, which
// takes a stopped debuggee. A running one is stopped for good, the way gdb's
// own backend does it: what is picked is shown in a stop, not in a run.
void DapImpl::watchPoint(quint64 requestId, const QPoint &pnt)
{
    if (!m_inferiorRunning) {
        pickWidget({requestId, pnt});
        return;
    }
    m_widgetPicksNeedingAStop.append({requestId, pnt});
    if (m_widgetPicksNeedingAStop.size() == 1)
        execute({ExecutionCommand::Interrupt});
}

// The overload taking two ints is inline, so the one taking a QPoint is
// called, with the point in memory the call can see. gdb calls it by the name
// the library exports, which needs no debug information: the point is put
// where the debuggee's own allocator says, once the function is known to exist.
void DapImpl::pickWidget(const WidgetPick &pick)
{
    const int x = pick.point.x();
    const int y = pick.point.y();
    QJsonObject arguments;
    if (m_startData.adapterId == "gdb") {
        const QString script = QString(
            "python f = int(gdb.parse_and_eval('_ZN12QApplication8widgetAtERK6QPoint').address);"
            " b = int(gdb.parse_and_eval('((void *(*)(unsigned long)) malloc)(8)'));"
            " gdb.parse_and_eval('*(int *) %d = %1' % b);"
            " gdb.parse_and_eval('*(int *) %d = %2' % (b + 4));"
            " w = int(gdb.parse_and_eval('((void *(*)(void *)) %d)((void *) %d)' % (f, b)));"
            " gdb.parse_and_eval('((void (*)(void *)) free)((void *) %d)' % b);"
            " print(hex(w))").arg(x).arg(y);
        arguments = {{"expression", script}, {"context", "repl"}};
    } else {
        const QString expression = QString("int point[2] = {%1, %2};"
                                           " (void *) QApplication::widgetAt(*(QPoint *) point)")
                                       .arg(x).arg(y);
        arguments = {{"expression", expression}, {"context", "watch"}};
    }
    ++m_inferiorCallsInFlight;
    sendCustomRequest("evaluate", arguments,
                      [this, requestId = pick.requestId](const Utils::Result<QJsonObject> &answer) {
        --m_inferiorCallsInFlight;
        const quint64 address = answer ? addressOfEvaluated(*answer) : 0;
        emit watchPointResolved(requestId, address, widgetExpression(address));
    });
}

// The QML engine is asked for its stack with a call into the debuggee, as the
// gdb dumper does it. Only gdb's adapter runs the Python that finds the engine,
// the others give the native stack alone.
void DapImpl::fetchQmlStack(const RefreshRequest &request)
{
    auto fetchStack = [this, request](const GdbMi &qmlFrames) {
        const int seq = m_client->stackTrace(m_currentThreadId,
                                             qMax(request.stackDepthLimit, 0));
        if (seq >= 0)
            m_stackTraceRequests.insert(seq, {false, request.requestId, false, qmlFrames});
    };
    // A core has no process to run the call in.
    if (m_startData.adapterId != "gdb" || isCoreSession()) {
        fetchStack({});
        return;
    }
    static const QByteArray script = R"(
q = chr(34)
old = gdb.parameter('print elements')
gdb.execute('set print elements unlimited')
out = ''
try:
    f = gdb.newest_frame()
    while f is not None and not out:
        try:
            symbols = list(f.block())
        except RuntimeError:
            symbols = []
        for s in symbols:
            if not (s.is_variable or s.is_argument) or s.type is None:
                continue
            t = s.type.strip_typedefs()
            if t.code != gdb.TYPE_CODE_PTR:
                continue
            if t.target().unqualified().name != 'QV4::ExecutionEngine':
                continue
            engine = int(s.value(f))
            r = str(gdb.parse_and_eval('qt_v4StackTraceForEngine((void *) 0x%x)' % engine))
            p = r.find(q + 'stack=[')
            if p != -1:
                out = r[p + 8:-2].replace(chr(92) + q, q).replace('func=', 'function=')
            break
        f = f.older()
finally:
    gdb.execute('set print elements %s' % ('unlimited' if old is None else old))
print(out)
)";
    const QString command = "python exec(bytes.fromhex('" + QString::fromLatin1(script.toHex())
                            + "').decode())";
    ++m_inferiorCallsInFlight;
    sendCustomRequest("evaluate", {{"expression", command}, {"context", "repl"}},
                      [this, fetchStack](const Utils::Result<QJsonObject> &answer) {
        --m_inferiorCallsInFlight;
        GdbMi all;
        if (answer) {
            QStringDecoder decoder(QStringDecoder::Utf8);
            all.fromString("{frames=[" + answer->value("result").toString().trimmed() + "]}",
                           decoder);
        }
        fetchStack(all["frames"]);
    });
}

void DapImpl::createSnapshot(quint64 requestId)
{
    // Nothing in the protocol writes a core file, while the debugger behind
    // the adapter does it over its console. The temporary file is there for
    // its name only, the debugger writes the core itself.
    FilePath filePath;
    {
        TemporaryFile coreFile("dapsnapshot");
        if (!coreFile.open()) {
            emit snapshotCreated(requestId, false, {});
            return;
        }
        filePath = coreFile.filePath();
    }
    // The command takes the rest of the line as the name, quotes and all.
    const QString expression = "gcore " + filePath.path();
    sendCustomRequest("evaluate", QJsonObject{{"expression", expression}, {"context", "repl"}},
                      [this, requestId, filePath](const Utils::Result<QJsonObject> &answer) {
        emit snapshotCreated(requestId, bool(answer), filePath);
    });
}

DebuggerEngine *createDapAdapterEngine(const DapStartData &data)
{
    return new GenericDebuggerEngine("DAP", new DapImpl(data));
}

} // namespace Debugger::Internal
