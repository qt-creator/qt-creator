// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlimpl.h"

#include "qmlv8debuggerclientconstants.h"

#include "../breakpoint.h"
#include "../debuggerconstants.h"
#include "../debuggertr.h"

#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qmldebugconstants.h>
#include <qmldebug/qpacketprotocol.h>

#include <utils/qmldebugcommandlinearguments.h>
#include <utils/qtcassert.h>
#include <utils/url.h>

#include <QDateTime>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <memory>
#include <utility>

using namespace Utils;

namespace Debugger::Internal {

static std::pair<QString, QString> v8TypeAndValue(const QVariantMap &data, int stringLimit);
static int v8ChildCount(const QVariantMap &data);

static GdbMi consoleItem(const QString &type, const QString &text)
{
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addConst(QStringLiteral("type"), type);
    item.addConst(QStringLiteral("text"), text);
    return item;
}

static QString consoleItemType(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
        return QStringLiteral("debug");
    case QtWarningMsg:
        return QStringLiteral("warning");
    case QtCriticalMsg:
    case QtFatalMsg:
        return QStringLiteral("error");
    }
    return {};
}

static void addConsoleChildren(GdbMi *item, const QList<GdbMi> &children)
{
    if (children.isEmpty())
        return;
    GdbMi list;
    list.m_type = GdbMi::List;
    list.m_name = QStringLiteral("children");
    for (const GdbMi &child : children)
        list.addChild(child);
    item->addChild(list);
}

// What the inspector answers a query with, which is plain values, maps and lists.
static GdbMi consoleItemFromValue(const QVariant &value, const QString &key = {})
{
    QList<GdbMi> children;
    QString text;
    if (value.typeId() == QMetaType::QVariantMap) {
        text = key.isEmpty() ? QStringLiteral("Object") : key + " : Object";
        const QVariantMap map = value.toMap();
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it)
            children.append(consoleItemFromValue(it.value(), it.key()));
    } else if (value.typeId() == QMetaType::QVariantList) {
        text = key.isEmpty() ? QStringLiteral("List") : QString("[%1] : List").arg(key);
        const QVariantList list = value.toList();
        for (int i = 0, end = int(list.size()); i != end; ++i)
            children.append(consoleItemFromValue(list.at(i), QString::number(i)));
    } else if (value.canConvert(QMetaType(QMetaType::QString))) {
        text = value.toString();
    } else {
        text = QStringLiteral("Unknown Value");
    }
    GdbMi item = consoleItem({}, text);
    addConsoleChildren(&item, children);
    return item;
}

// What the debug service answers an evaluation with. Properties come as
// references into the reply's "refs", and are resolved one level deep.
static GdbMi consoleItemFromV8(const QVariantMap &data, const QHash<int, QVariantMap> &refs,
                               int depth = 0)
{
    const auto [type, value] = v8TypeAndValue(data, 0);
    QString text = value == "{...}" ? type : value;
    const QString name = data.value(QLatin1String(NAME)).toString();
    if (!name.isEmpty())
        text = name + ": " + text;

    QList<GdbMi> children;
    if (depth < 2) {
        for (const QVariant &property : data.value(QLatin1String("properties")).toList()) {
            QVariantMap propertyData = property.toMap();
            const auto ref = propertyData.constFind(QLatin1String(REF));
            if (ref != propertyData.cend()) {
                const QVariantMap resolved = refs.value(ref->toInt());
                for (auto it = resolved.cbegin(), end = resolved.cend(); it != end; ++it) {
                    if (it.key() != QLatin1String(NAME))
                        propertyData.insert(it.key(), it.value());
                }
            }
            children.append(consoleItemFromV8(propertyData, refs, depth + 1));
        }
    }
    GdbMi item = consoleItem({}, text);
    addConsoleChildren(&item, children);
    // What the answer did not resolve is looked up when the entry is expanded.
    const QVariant handle = data.value(QLatin1String(REF), data.value(QLatin1String(HANDLE)));
    if (children.isEmpty() && handle.isValid() && v8ChildCount(data) > 0)
        item.addConst(QStringLiteral("handle"), QString::number(handle.toLongLong()));
    return item;
}

static QString serviceStateMessage(const QmlDebug::QmlDebugClient *client,
                                   QmlDebug::QmlDebugClient::State state)
{
    static const char *const names[] = {"not connected", "unavailable", "enabled"};
    return QString("Status of \"%1\" Version: %2 changed to '%3'.")
        .arg(client->name())
        .arg(client->serviceVersion())
        .arg(QLatin1String(names[state]));
}

// The debug service knows one exception break, which covers the caught ones as
// well, so all three spellings of the request end up as the same one.
static bool isExceptionBreakpoint(BreakpointType type)
{
    return type == BreakpointAtJavaScriptThrow || type == BreakpointAtThrow
           || type == BreakpointAtCatch;
}

static DebuggerEngineSetupData qmlImplSetupData(const QmlImplStartData &startData)
{
    DebuggerEngineSetupData data;
    data.needsRunningCompanion = startData.companionRunsRuntime;
    data.capabilities = AddWatcherCapability
                      | AddWatcherWhileRunningCapability
                      | BreakConditionCapability
                      | BreakOnThrowAndCatchCapability
                      | CreateFullBacktraceCapability
                      | ResetInferiorCapability
                      | RunToLineCapability
                      | TracePointCapability
                      | WatchComplexExpressionsCapability;
    data.extraCapabilities = DebuggerExtraCapability::Detach
                           | DebuggerExtraCapability::RunAsUser
                           | DebuggerExtraCapability::RunCommandDeferral
                           | DebuggerExtraCapability::ScriptConsole
                           | DebuggerExtraCapability::SourceFiles;
    data.startModes = DebuggerStartModeFlag::AttachToQmlServer
                    | DebuggerStartModeFlag::Launch;
    data.toolTipHandling = ToolTipHandling::Always;
    data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &query) {
        if (query.startMode == AttachToCore)
            return false;
        if (query.type == BreakpointOnQmlSignalEmit || isExceptionBreakpoint(query.type))
            return true;
        return query.isQmlFileAndLineBreakpoint();
    };
    return data;
}

class QmlImpl::V8Client : public QmlDebug::QmlDebugClient
{
public:
    V8Client(QmlImpl *owner, QmlDebug::QmlDebugConnection *connection)
        : QmlDebug::QmlDebugClient(QLatin1String("V8Debugger"), connection)
        , m_owner(owner)
    {}

    void stateChanged(State state) override { m_owner->handleStateChanged(state); }
    void messageReceived(const QByteArray &data) override { m_owner->handleMessageReceived(data); }

private:
    QmlImpl *m_owner;
};

QmlImpl::QmlImpl(const QmlImplStartData &startData)
    : DebuggerEngineInterface(qmlImplSetupData(startData))
    , m_startData(startData)
    , m_connection(this)
{
    m_v8Client = new V8Client(this, &m_connection);
    m_engineClient = new QmlDebug::QmlEngineDebugClient(&m_connection);
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::result,
            this, [this](quint32 queryId, const QVariant &value, const QByteArray &type) {
        if (const InspectorCallback cb = m_inspectorCallbackForQueryId.take(queryId))
            cb(value, type);
    });
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::newObject,
            this, &QmlImpl::handleObjectCreated);
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::newState,
            this, [this](QmlDebug::QmlDebugClient::State state) {
        emit message(serviceStateMessage(m_engineClient, state), LogMisc);
        if (state == QmlDebug::QmlDebugClient::Enabled)
            rebuildInspectorTree();
    });
    auto messageClient = new QmlDebug::QDebugMessageClient(&m_connection);
    connect(messageClient, &QmlDebug::QDebugMessageClient::newState,
            this, [this, messageClient](QmlDebug::QmlDebugClient::State state) {
        emit message(serviceStateMessage(messageClient, state), LogMisc);
    });
    connect(messageClient, &QmlDebug::QDebugMessageClient::message, this,
            [this](QtMsgType type, const QString &text, const QmlDebug::QDebugContextInfo &info) {
        GdbMi item = consoleItem(consoleItemType(type), text);
        if (!info.file.isEmpty()) {
            item.addConst(QStringLiteral("file"), toLocalFile(info.file).toUrlishString());
            item.addConst(QStringLiteral("line"), QString::number(info.line));
        }
        emit consoleItemReported(item);
    });
    connect(m_engineClient, &QmlDebug::BaseEngineDebugClient::valueChanged,
            this, &QmlImpl::handlePropertyValueChanged);
    m_toolsClient = new QmlDebug::QmlToolsClient(&m_connection);
    connect(m_toolsClient, &QmlDebug::BaseToolsClient::newState,
            this, [this](QmlDebug::QmlDebugClient::State state) {
        emit message(serviceStateMessage(m_toolsClient, state), LogMisc);
        emit inspectorToolsAvailable(state == QmlDebug::QmlDebugClient::Enabled);
    });
    connect(m_toolsClient, &QmlDebug::BaseToolsClient::currentObjectsChanged,
            this, [this](const QList<int> &debugIds) {
        if (!debugIds.isEmpty())
            reportSelectedObject(debugIds.first());
    });
    connect(m_toolsClient, &QmlDebug::BaseToolsClient::reloaded,
            this, &QmlImpl::rebuildInspectorTree);
    m_objectCreatedTimer = new QTimer(this);
    m_objectCreatedTimer->setInterval(100);
    m_objectCreatedTimer->setSingleShot(true);
    connect(m_objectCreatedTimer, &QTimer::timeout, this, &QmlImpl::rebuildInspectorTree);

    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(m_startData.watchdogTimeout);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        if (m_pendingCommands.isEmpty())
            return;
        QStringList pending;
        for (const PendingCommand &command : std::as_const(m_pendingCommands))
            pending << command.command;
        m_watchdog.start();
        emit notResponding(m_startData.watchdogTimeout, pending);
    });

    m_inferiorProcess.setProcessMode(ProcessMode::Reader);
    connect(&m_inferiorProcess, &Process::readyReadStandardOutput, this, [this] {
        const QString output = m_inferiorProcess.readAllStandardOutput();
        emit message(output, AppOutput);
        scanRuntimeOutput(output);
    });
    connect(&m_inferiorProcess, &Process::readyReadStandardError, this, [this] {
        const QString output = m_inferiorProcess.readAllStandardError();
        emit message(output, AppError);
        scanRuntimeOutput(output);
    });
    connect(&m_inferiorProcess, &Process::done, this, [this] {
        if (m_inferiorProcess.error() == ProcessError::FailedToStart) {
            emit message(m_inferiorProcess.errorString(), LogError);
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
            return;
        }
        if (m_aborting) {
            emit engineProcessFinished(m_inferiorProcess.resultData());
            return;
        }
        if (std::exchange(m_isPortRelaunch, false) && !m_shuttingDown) {
            QMetaObject::invokeMethod(this, [this] {
                if (!m_shuttingDown)
                    launchInferior();
            }, Qt::QueuedConnection);
            return;
        }
        if (m_isResetRestart) {
            // Our own kill behind a ResetInferior. An exit reported here would take
            // the engine down instead of putting a fresh runtime in its place.
            resetTransientState();
            m_connection.close();
            QMetaObject::invokeMethod(this, [this] { launchInferior(); }, Qt::QueuedConnection);
            return;
        }
        if (m_shuttingDown)
            return;
        m_inferiorRunning = false;
        m_inferiorExited = true;
        emit inferiorDone(InferiorResultData{
            m_inferiorProcess.exitCode(),
            m_inferiorProcess.exitStatus() == ProcessExitStatus::CrashExit
                ? InferiorExitStatus::Crash : InferiorExitStatus::Normal});
    });
    connect(&m_inferiorProcess, &Process::started, this, [this] {
        emit inferiorPidKnown(ProcessHandle(m_inferiorProcess.processId()));
    });

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(100);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &QmlImpl::beginConnection);

    connect(&m_connection, &QmlDebug::QmlDebugConnection::connectionFailed, this, [this] {
        if (m_isPortRelaunch || m_shuttingDown || m_aborting)
            return;
        // A runtime started here waits for this end ("block") however long it
        // takes to come up, so it gets a generous deadline instead of a count.
        // One that never listens on the port (taken, or no QML debugging) still
        // runs, and must not keep the setup waiting forever. One the companion
        // starts comes up whenever the companion lets it, and says so in its
        // output if it never does. A local server that could not listen will
        // not do better on a second attempt.
        const bool launched = std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData);
        const bool retry = m_server.scheme() != Utils::urlSocketScheme()
            && (m_startData.companionRunsRuntime
                || (launched ? m_inferiorProcess.state() != ProcessState::NotRunning
                                   && !m_connectDeadline.hasExpired()
                             : m_connectRetriesLeft-- > 0));
        if (retry) {
            m_reconnectTimer.start();
            return;
        }
        const QString server = m_server.scheme() == Utils::urlSocketScheme() ? m_server.path()
                                                                       : m_server.authority();
        emit message(QString("Gave up connecting to the Qml debug server at %1, the last "
                             "attempt failed with: %2").arg(server, m_lastConnectionError),
                     LogError);
        emit connectionFailed(Tr::tr("Could not connect to the in-process QML debugger at %1.")
                                  .arg(server));
    });
    connect(&m_connection, &QmlDebug::QmlDebugConnection::logError, this,
            [this](const QString &error) {
        if (error != std::exchange(m_lastConnectionError, error))
            emit message("QML Debugger: " + error, LogMisc);
    });
    connect(&m_connection, &QmlDebug::QmlDebugConnection::logStateChange, this,
            [this](const QString &state) {
        // The retries repeat the same states, so only the first attempt is logged.
        if (m_lastConnectionError.isEmpty())
            emit message("QML Debugger: " + state, LogStatus);
    });
    connect(&m_connection, &QmlDebug::QmlDebugConnection::disconnected, this, [this] {
        if (m_shuttingDown || m_isResetRestart || m_aborting)
            return;
        // An application this session attached to closes the connection when
        // it quits, and no process of ours reports that instead.
        if (!std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
            emit message(Tr::tr("QML Debugger disconnected."), StatusBar);
            m_inferiorRunning = false;
            m_inferiorExited = true;
            emit inferiorDone({0, InferiorExitStatus::Detached});
            return;
        }
        emit inferiorEvent(InferiorEvent::EngineIll);
    });
}

QmlImpl::~QmlImpl()
{
    m_shuttingDown = true;
}

void QmlImpl::start()
{
    if (std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
        launchInferior();
        return;
    }
    m_server = std::get<AttachToQmlServerData>(m_startData.inferiorStartData).server;
    beginConnection();
}

void QmlImpl::retryConnection()
{
    m_connectRetriesLeft = 50;
    m_connectDeadline.setRemainingTime(std::chrono::minutes(1));
    beginConnection();
}

bool QmlImpl::picksOwnPort() const
{
    return std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)
           && m_startData.qmlServer.isEmpty();
}

void QmlImpl::companionOutput(const QString &output)
{
    scanRuntimeOutput(output);
}

// What the runtime prints when its debug service does not come up. Nothing
// would ever answer then, so that ends the setup instead of the retries.
void QmlImpl::scanRuntimeOutput(const QString &output)
{
    if (m_isPortRelaunch || m_shuttingDown || m_aborting || m_connection.isConnected())
        return;
    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (!line.contains(QLatin1String(QmlDebug::Constants::STR_IGNORING_DEBUGGER)))
            continue;
        m_connection.close();
        m_reconnectTimer.stop();
        reportRuntimeFailure(line.trimmed());
        m_shuttingDown = true;
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
        if (m_inferiorProcess.isRunning())
            m_inferiorProcess.kill();
        return;
    }
    if (!output.contains(QLatin1String(QmlDebug::Constants::STR_UNABLE_TO_LISTEN)))
        return;
    m_connection.close();
    m_reconnectTimer.stop();
    // The port was free when it was picked, but something else can take it
    // before the runtime gets to listen on it. The runtime then keeps running
    // without a debug server, so it is replaced by one on a new port.
    if (picksOwnPort() && m_listenRetriesLeft-- > 0) {
        m_isPortRelaunch = true;
    } else {
        emit message(QString("The Qml runtime could not listen on %1.")
                         .arg(m_server.toString()), LogError);
        reportRuntimeFailure(Tr::tr("The QML runtime could not listen on %1.")
                                 .arg(m_server.authority()));
        m_shuttingDown = true;
        emit inferiorEvent(InferiorEvent::EngineSetupFailed);
    }
    if (m_inferiorProcess.isRunning())
        m_inferiorProcess.kill();
}

// A session that debugs more than QML goes on without it, so the user is told
// in a way that cannot be missed. Otherwise the console says it.
void QmlImpl::reportRuntimeFailure(const QString &reason)
{
    const QString error = Tr::tr("Could not connect to the in-process QML debugger. %1")
                              .arg(reason);
    emit message(error, LogError);
    if (m_startData.companionRunsRuntime)
        emit startFailed(Tr::tr("QML Debugger"), error, Key("QmlRuntimeFailed"));
    else
        emit consoleItemReported(consoleItem(QStringLiteral("warning"), error));
}

void QmlImpl::launchInferior()
{
    ProcessRunData runData = std::get<ProcessRunData>(m_startData.inferiorStartData);
    if (picksOwnPort()) {
        // The runtime takes the port as a number, so a free one has to be
        // picked before it is started. It is handed over rather than kept, and
        // "block" makes the runtime wait for this end to arrive on it.
        QTcpServer probe;
        if (!probe.listen(QHostAddress::LocalHost)) {
            emit message("No port available for the Qml debug connection.", LogError);
            emit inferiorEvent(InferiorEvent::EngineSetupFailed);
            return;
        }
        const quint16 port = probe.serverPort();
        m_server.setScheme(Utils::urlTcpScheme());
        m_server.setHost("127.0.0.1");
        m_server.setPort(port);
        runData.command.addArg(
            qmlDebugCommandLineArguments(QmlDebuggerServices, QString("port:%1").arg(port), true));
    } else {
        m_server = m_startData.qmlServer;
    }
    m_inferiorProcess.setRunData(runData);
    m_inferiorProcess.setRunAsUser(m_startData.runAsUser);
    m_connectDeadline.setRemainingTime(std::chrono::minutes(1));
    emit message(Tr::tr("Starting %1").arg(runData.command.toUserOutput()), LogMisc);
    m_inferiorProcess.start();

    beginConnection();
}

void QmlImpl::beginConnection()
{
    if (m_isPortRelaunch || m_shuttingDown || m_aborting)
        return;
    // The runtime connects to Creator here, rather than the other way round.
    if (m_server.scheme() == Utils::urlSocketScheme()) {
        if (!m_connection.isListening())
            m_connection.startLocalServer(m_server.path());
        return;
    }
    const QString host = m_server.host().isEmpty()
        ? QHostAddress(QHostAddress::LocalHost).toString() : m_server.host();
    m_connection.connectToHost(host, quint16(m_server.port()));
}

// What the runtime that has just been replaced left behind: none of it says
// anything about the one taking its place.
void QmlImpl::resetTransientState()
{
    m_inferiorRunning = false;
    m_inferiorExited = false;
    m_interruptRequested = false;
    m_disconnected = false;
    m_currentFrameIndex = 0;
    m_connectRetriesLeft = 50;
    m_listenRetriesLeft = 3;
    m_engineQueryRetriesLeft = 50;
    m_callbackForToken.clear();
    m_pendingCommands.clear();
    m_hitCountsByResponseId.clear();
    m_serviceNumberByResponseId.clear();
    m_qmlEngines.clear();
    m_inameForDebugId.clear();
    m_debugIdsWithProperties.clear();
    m_sourceForDebugId.clear();
    m_engineIdForDebugId.clear();
    m_objectWatches.clear();
    m_knownDelegateIds.clear();
    m_validatedDelegateId = -1;
    m_deferredWatchers.reset();
    m_runToLineBreakpoints.clear();
    m_runToLineLine = 0;
}

// A run to a line ends at whatever stop comes first, so the breakpoint it put
// there would otherwise stop every later pass over that line.
void QmlImpl::clearRunToLineBreakpoints()
{
    const QList<int> numbers = std::exchange(m_runToLineBreakpoints, {});
    const int line = std::exchange(m_runToLineLine, 0);
    for (const int number : numbers) {
        DebuggerCommand cmd(CLEARBREAKPOINT);
        cmd.arg(BREAKPOINT, number);
        runCommand(cmd);
    }
    if (numbers.isEmpty())
        return;

    // The temporary breakpoint replaced whatever the user had at that location,
    // and clearing it removed the location altogether.
    const QList<BreakpointChangeRequest> active = m_activeBreakpointsByResponseId.values();
    for (const BreakpointChangeRequest &breakpoint : active) {
        const BreakpointParameters &params = breakpoint.params;
        if (params.enabled && params.textPosition.line == line
            && params.fileName.fileName() == m_runToLineFile.fileName()) {
            setScriptBreakpoint(0, breakpoint, breakpoint.responseId);
        }
    }
}

void QmlImpl::sendDisconnect()
{
    if (m_disconnected || !m_v8Client
        || m_v8Client->state() != QmlDebug::QmlDebugClient::Enabled) {
        return;
    }
    m_disconnected = true;
    runCommand({DISCONNECT});
}

void QmlImpl::shutdownInferior(ShutdownMode mode)
{
    if (mode == ShutdownMode::Kill && m_inferiorProcess.isRunning()) {
        m_shuttingDown = true;
        m_inferiorProcess.close();
    }
    sendDisconnect();
    emit inferiorEvent(InferiorEvent::ShutdownFinished);
}

void QmlImpl::shutdownEngine()
{
    m_shuttingDown = true;
    m_connection.close();
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void QmlImpl::handleStateChanged(QmlDebug::QmlDebugClient::State state)
{
    emit message(serviceStateMessage(m_v8Client, state), LogMisc);
    if (state != QmlDebug::QmlDebugClient::Enabled)
        return;
    QTimer::singleShot(0, this, [this] { handleConnectHandshakeDone(); });
}

void QmlImpl::handleConnectHandshakeDone()
{
    QJsonObject parameters;
    parameters.insert(QLatin1String("redundantRefs"), false);
    parameters.insert(QLatin1String("namesAsObjects"), false);
    runDirectCommand(CONNECT, QJsonDocument(parameters).toJson());

    // The service ignores what it is asked before it has answered the version
    // request, so the inferior is reported as running only after that.
    const bool isResetRestart = std::exchange(m_isResetRestart, false);
    runCommand({VERSION}, [this, isResetRestart](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        m_supportChangeBreakpoint = body.value("ChangeBreakpoint", false).toBool();
        m_unpausedEvaluate = body.value("UnpausedEvaluate", false).toBool();
        m_contextEvaluate = body.value("ContextEvaluate", false).toBool();
        // Without unpaused evaluation, input goes to the selected object anyway.
        emit consoleObjectContextReported(m_contextEvaluate || !m_unpausedEvaluate);

        if (isResetRestart) {
            // The fresh runtime knows none of the breakpoints, while the model knows
            // them all: they go in again under the ids it has, and it hears nothing.
            const QList<BreakpointChangeRequest> breakpoints
                = m_activeBreakpointsByResponseId.values();
            for (const BreakpointChangeRequest &breakpoint : breakpoints)
                setScriptBreakpoint(breakpoint.requestId, breakpoint, breakpoint.responseId);
            m_inferiorRunning = true;
            return;
        }

        emit inferiorEvent(InferiorEvent::EngineSetupOk);
        m_inferiorRunning = true;
        emit inferiorEvent(InferiorEvent::RunAndInferiorRunOk);
    });
}

void QmlImpl::runDirectCommand(const QByteArray &type, const QByteArray &msg)
{
    emit message(QString("%1 %2").arg(QString::fromLatin1(type), QString::fromUtf8(msg)), LogInput);
    QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
    rs << QByteArray(V8DEBUG) << type << msg;
    m_v8Client->sendMessage(rs.data());
}

int QmlImpl::runCommand(const DebuggerCommand &command, const QmlCallback &cb)
{
    ++m_sequence;
    QJsonObject object;
    object.insert(QLatin1String(SEQ), m_sequence);
    object.insert(QLatin1String(TYPE), QLatin1String(REQUEST));
    object.insert(QLatin1String(COMMAND), command.function);
    object.insert(QLatin1String(ARGUMENTS), command.args);
    if (cb)
        m_callbackForToken[m_sequence] = cb;
    if (m_startData.logTimeStamps
        || m_startData.watchdogTimeout != std::chrono::seconds::zero()) {
        // The arguments belong to the description: what a command was asked
        // about is what tells two of the same name apart.
        const QString description = command.function + ' '
            + QString::fromUtf8(QJsonDocument(command.args.toObject()).toJson(QJsonDocument::Compact));
        m_pendingCommands[m_sequence] = {description, QDateTime::currentMSecsSinceEpoch()};
        restartWatchdog();
    }
    runDirectCommand(V8REQUEST, QJsonDocument(object).toJson(QJsonDocument::Compact));
    return m_sequence;
}

void QmlImpl::restartWatchdog()
{
    if (m_startData.watchdogTimeout == std::chrono::seconds::zero())
        return;
    if (m_pendingCommands.isEmpty())
        m_watchdog.stop();
    else
        m_watchdog.start();
}

void QmlImpl::handleMessageReceived(const QByteArray &data)
{
    QmlDebug::QPacket ds(m_v8Client->dataStreamVersion(), data);
    QByteArray command;
    ds >> command;
    if (command != V8DEBUG)
        return;
    QByteArray type;
    QByteArray payload;
    ds >> type >> payload;
    emit message(QString("%1 %2").arg(QString::fromLatin1(type), QString::fromUtf8(payload)),
                  LogOutput);
    if (type == V8MESSAGE)
        handleV8Message(payload);
}

void QmlImpl::handleV8Message(const QByteArray &payload)
{
    const QVariantMap resp = QJsonDocument::fromJson(payload).toVariant().toMap();
    const QString type = resp.value(QLatin1String(TYPE)).toString();
    if (type == QLatin1String("response")) {
        const int requestSeq = resp.value(QLatin1String("request_seq")).toInt();
        if (const PendingCommand answered = m_pendingCommands.take(requestSeq); answered.postTime) {
            if (m_startData.logTimeStamps) {
                const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - answered.postTime;
                emit message(QString("Response time: %1: %2 s").arg(answered.command)
                                 .arg(elapsed / 1000.), LogTime);
            }
            restartWatchdog();
        }
        const QmlCallback cb = m_callbackForToken.take(requestSeq);
        if (cb)
            cb(resp);
    } else if (type == QLatin1String("event")) {
        const QString event = resp.value(QLatin1String("event")).toString();
        if (event == QLatin1String("break"))
            handleBreakEvent(resp);
        else if (event == QLatin1String("exception"))
            handleExceptionEvent(resp);
    }
}

int QmlImpl::serviceNumberFor(const QString &responseId) const
{
    const QString number = m_serviceNumberByResponseId.value(responseId);
    return number.isEmpty() ? responseId.toInt() : number.toInt();
}

void QmlImpl::clearBreakpointNumber(const QString &responseId)
{
    DebuggerCommand cmd(CLEARBREAKPOINT);
    cmd.arg(BREAKPOINT, serviceNumberFor(responseId));
    runCommand(cmd);
}

// The service keeps no count of its own, and says nothing about the breakpoint
// it stopped for, so the hit the view shows is the one counted here.
void QmlImpl::reportBreakpointHit(const QString &responseId, int hits,
                                  const QString &functionName)
{
    const BreakpointParameters params
        = m_activeBreakpointsByResponseId.value(responseId).params;
    GdbMi bkpt;
    bkpt.m_type = GdbMi::Tuple;
    bkpt.addConst(QStringLiteral("number"), responseId);
    bkpt.addConst(QStringLiteral("file"), params.fileName.path());
    bkpt.addConst(QStringLiteral("fullname"), params.fileName.path());
    bkpt.addConst(QStringLiteral("line"), QString::number(params.textPosition.line));
    bkpt.addConst(QStringLiteral("enabled"), params.enabled ? "y" : "n");
    bkpt.addConst(QStringLiteral("times"), QString::number(hits));
    if (!functionName.isEmpty())
        bkpt.addConst(QStringLiteral("func"), functionName);
    // The model reads a modification as the whole state of the breakpoint, so a
    // condition left out of it counts as none rather than as unchanged.
    if (!params.condition.isEmpty())
        bkpt.addConst(QStringLiteral("cond"), params.condition);
    GdbMi list;
    list.m_type = GdbMi::List;
    list.addChild(bkpt);
    emit breakpointModified(list);
}

// The service has no tracepoint of its own, so the captures are evaluated at
// the stop the ordinary breakpoint behind it produced, and the message put
// together once the last answer is in.
void QmlImpl::reportTracepointHit(const QString &responseId,
                                  const std::function<void()> &finished)
{
    const auto hit = std::make_shared<TracepointHit>();
    hit->pattern = m_activeBreakpointsByResponseId.value(responseId).params.message;
    hit->captures = parseTracepointCaptures(hit->pattern);
    hit->values.m_type = GdbMi::List;
    hit->expressions.m_type = GdbMi::Tuple;
    hit->finished = finished;

    for (const TracepointCapture &capture : std::as_const(hit->captures)) {
        if (capture.type != TracepointCaptureType::Expression) {
            // Nothing the service can be asked for, so the capture stays as written.
            hit->values.addConst(QStringLiteral("value"),
                                 hit->pattern.mid(capture.start, capture.end - capture.start));
            continue;
        }
        const QString expression = capture.expression;
        hit->values.addConst(QStringLiteral("value"), expression);
        ++hit->outstanding;
        DebuggerCommand cmd(EVALUATE);
        cmd.arg(EXPRESSION, expression);
        cmd.arg(FRAME, 0);
        runCommand(cmd, [this, hit, expression](const QVariantMap &resp) {
            const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
            QString value = resp.value(QLatin1String(MESSAGE)).toString();
            if (resp.value(QLatin1String(SUCCESS)).toBool())
                value = v8TypeAndValue(body, m_stringLimit).second;
            GdbMi entry;
            entry.m_type = GdbMi::Tuple;
            entry.m_name = expression;
            entry.addConst(QStringLiteral("value"), value);
            hit->expressions.addChild(entry);
            finishTracepointHit(hit);
        });
    }
    finishTracepointHit(hit);
}

void QmlImpl::finishTracepointHit(const std::shared_ptr<TracepointHit> &hit)
{
    if (--hit->outstanding > 0)
        return;
    emit message(formatTracepointMessage(hit->pattern, hit->captures, hit->values,
                                         hit->expressions), LogMisc);
    hit->finished();
}

FilePath QmlImpl::toLocalFile(const QString &scriptName) const
{
    const QUrl url(scriptName);
    if (m_startData.findFile) {
        const FilePath found = m_startData.findFile(url);
        if (!found.isEmpty())
            return found;
    }
    return FilePath::fromUrl(url);
}

void QmlImpl::handleBreakEvent(const QVariantMap &response)
{
    m_inferiorRunning = false;
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();
    const QVariantMap script = body.value(QLatin1String("script")).toMap();
    const QString scriptName = script.value(QLatin1String(NAME)).toString();
    const int lineNumber = body.value(QLatin1String("sourceLine")).toInt() + 1;
    const QString functionName = body.value(QLatin1String("invocationText")).toString();

    // The service keys its breakpoints by file name and line, so what this stop
    // belongs to is whatever was put at the location it stopped at.
    const QString stoppedFile = FilePath::fromUrl(QUrl(scriptName)).fileName();
    QStringList hitHere;
    for (auto it = m_activeBreakpointsByResponseId.cbegin();
         it != m_activeBreakpointsByResponseId.cend(); ++it) {
        const BreakpointParameters &params = it->params;
        if (params.textPosition.line == lineNumber
            && params.fileName.fileName() == stoppedFile) {
            hitHere.append(it.key());
        }
    }

    // The debug service has no ignore count of its own, so the hits it is
    // asked to skip are counted here, and the inferior sent on unnoticed.
    if (!hitHere.isEmpty() && !m_interruptRequested) {
        // A run to a line is to end at its target, whatever else is there.
        bool skip = m_runToLineLine == 0 || lineNumber != m_runToLineLine
                    || stoppedFile != m_runToLineFile.fileName();
        QStringList tracepoints;
        for (const QString &responseId : std::as_const(hitHere)) {
            const BreakpointParameters params
                = m_activeBreakpointsByResponseId.value(responseId).params;
            const int hits = ++m_hitCountsByResponseId[responseId];
            reportBreakpointHit(responseId, hits, functionName);
            if (params.isTracepoint()) {
                tracepoints.append(responseId);
                continue;
            }
            if (params.ignoreCount <= 0 || hits > params.ignoreCount)
                skip = false;
        }
        if (skip && tracepoints.isEmpty()) {
            m_inferiorRunning = true;
            runCommand({CONTINEDEBUGGING});
            return;
        }
        if (skip) {
            // The captures can only be evaluated while the inferior is stopped, so
            // it goes on once the last tracepoint has had its say.
            const auto outstanding = std::make_shared<int>(tracepoints.size());
            for (const QString &responseId : std::as_const(tracepoints)) {
                reportTracepointHit(responseId, [this, outstanding] {
                    if (--*outstanding > 0)
                        return;
                    m_inferiorRunning = true;
                    runCommand({CONTINEDEBUGGING});
                });
            }
            return;
        }
    }

    // The service has no breakpoint that stops only once either, so a one-shot
    // is an ordinary one taken back here, before the inferior runs again.
    QStringList takenBack;
    for (const QString &responseId : std::as_const(hitHere)) {
        if (!m_activeBreakpointsByResponseId.value(responseId).params.oneShot)
            continue;
        clearBreakpointNumber(responseId);
        m_activeBreakpointsByResponseId.remove(responseId);
        m_hitCountsByResponseId.remove(responseId);
        m_serviceNumberByResponseId.remove(responseId);
        takenBack.append(responseId);
    }

    clearRunToLineBreakpoints();
    if (!scriptName.isEmpty())
        emit locationChanged(toLocalFile(scriptName), lineNumber);
    emit inferiorEvent(std::exchange(m_interruptRequested, false)
                       ? InferiorEvent::StopOk : InferiorEvent::SpontaneousStop);

    for (const QString &responseId : takenBack) {
        GdbMi deleted;
        deleted.m_type = GdbMi::Tuple;
        deleted.addConst(QStringLiteral("number"), responseId);
        emit breakpointEvent(0, BreakpointOp::Remove, true, deleted);
    }

    if (m_deferredWatchers) {
        const RefreshRequest deferred = *std::exchange(m_deferredWatchers, std::nullopt);
        refreshLocals(deferred);
    }
}

void QmlImpl::handleExceptionEvent(const QVariantMap &response)
{
    m_inferiorRunning = false;
    clearRunToLineBreakpoints();
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();

    const QVariantMap script = body.value(QLatin1String("script")).toMap();
    const QString scriptName = script.value(QLatin1String(NAME)).toString();
    const int lineNumber = body.value(QLatin1String("sourceLine")).toInt() + 1;
    if (!scriptName.isEmpty())
        emit locationChanged(toLocalFile(scriptName), lineNumber);

    const QVariantMap exception = body.value(QLatin1String("exception")).toMap();
    const QString text = exception.value(QLatin1String("text")).toString();
    if (!text.isEmpty())
        emit message(text, ConsoleOutput);

    emit inferiorEvent(std::exchange(m_interruptRequested, false)
                       ? InferiorEvent::StopOk : InferiorEvent::SpontaneousStop);
}

void QmlImpl::setScriptBreakpoint(quint64 requestId, const BreakpointChangeRequest &request,
                                  const QString &knownResponseId)
{
    const BreakpointParameters &params = request.params;
    DebuggerCommand cmd(SETBREAKPOINT);
    cmd.arg(TYPE, SCRIPTREGEXP);
    cmd.arg(TARGET, params.fileName.toUrlishString());
    cmd.arg(ENABLED, params.enabled);
    cmd.arg(LINE, params.textPosition.line - 1);
    if (params.textPosition.column > 0)
        cmd.arg(COLUMN, params.textPosition.column - 1);
    if (!params.condition.isEmpty())
        cmd.arg(CONDITION, params.condition);
    if (params.ignoreCount > 0)
        cmd.arg(IGNORECOUNT, params.ignoreCount);

    // A change the service cannot express is carried out by putting a new
    // breakpoint in place of the old one, and the model waits for an answer to
    // the request it made, not to the one that replaced it.
    const BreakpointOp op = request.op;
    runCommand(cmd, [this, requestId, op, params, request, knownResponseId]
                    (const QVariantMap &resp) {
        const bool success = resp.value(QLatin1String(SUCCESS)).toBool();
        if (!success) {
            if (knownResponseId.isEmpty())
                emit breakpointEvent(requestId, op, false, {});
            return;
        }
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QString serviceNumber
            = QString::number(body.value(QLatin1String(BREAKPOINT)).toInt());
        const QString responseId = knownResponseId.isEmpty() ? serviceNumber : knownResponseId;
        m_serviceNumberByResponseId[responseId] = serviceNumber;
        BreakpointChangeRequest active = request;
        active.responseId = responseId;
        m_activeBreakpointsByResponseId[responseId] = active;
        if (!knownResponseId.isEmpty())
            return;

        int line = params.textPosition.line;
        const QVariantList actualLocations = body.value(QLatin1String("actual_locations")).toList();
        if (!actualLocations.isEmpty())
            line = actualLocations.constFirst().toMap().value(QLatin1String(LINE)).toInt() + 1;

        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        bkpt.addConst(QLatin1String(NUMBER), responseId);
        bkpt.addConst(QLatin1String("file"), params.fileName.toUserOutput());
        bkpt.addConst(QLatin1String("line"), QString::number(line));
        bkpt.addConst(QLatin1String(ENABLED),
                      params.enabled ? QStringLiteral("y") : QStringLiteral("n"));
        GdbMi data;
        data.m_type = GdbMi::List;
        data.addChild(bkpt);
        emit breakpointEvent(requestId, op, true, data);
    });
}

// Two locations the debug service cannot tell apart: a column is only passed
// on when there is one, so anything below the first column is the same request.
static bool isSameLocation(const Utils::Text::Position &was, const Utils::Text::Position &now)
{
    return was.line == now.line && qMax(0, was.column) == qMax(0, now.column);
}

// A change of an attribute comes with the location the model has, which can be
// none at all: what the request leaves out is what the breakpoint already has.
BreakpointChangeRequest QmlImpl::withKnownLocation(const BreakpointChangeRequest &request) const
{
    const auto it = m_activeBreakpointsByResponseId.constFind(request.responseId);
    if (it == m_activeBreakpointsByResponseId.constEnd() || !request.params.fileName.isEmpty())
        return request;
    BreakpointChangeRequest filled = request;
    filled.params.fileName = it->params.fileName;
    filled.params.textPosition = it->params.textPosition;
    return filled;
}

bool QmlImpl::isEnabledOnlyChange(const BreakpointChangeRequest &request) const
{
    const auto it = m_activeBreakpointsByResponseId.constFind(request.responseId);
    if (it == m_activeBreakpointsByResponseId.constEnd())
        return false;
    const BreakpointParameters &was = it->params;
    const BreakpointParameters &now = request.params;
    return was.enabled != now.enabled
           && was.fileName == now.fileName
           && isSameLocation(was.textPosition, now.textPosition)
           && was.condition == now.condition
           && was.ignoreCount == now.ignoreCount
           && was.command == now.command;
}

void QmlImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    const BreakpointParameters &params = request.params;
    switch (request.op) {
    case BreakpointOp::Insert:
        if (isExceptionBreakpoint(params.type)) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            if (params.enabled)
                cmd.arg(ENABLED, params.enabled);
            runCommand(cmd);
            emit breakpointEvent(request.requestId, BreakpointOp::Insert, true, {});
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << params.enabled;
            runDirectCommand(BREAKONSIGNAL, rs.data());
            emit breakpointEvent(request.requestId, BreakpointOp::Insert, true, {});
        } else {
            setScriptBreakpoint(request.requestId, request);
        }
        break;
    case BreakpointOp::Remove:
        if (isExceptionBreakpoint(params.type)) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            runCommand(cmd);
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << false;
            runDirectCommand(BREAKONSIGNAL, rs.data());
        } else if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Remove, false, {});
            break;
        } else {
            DebuggerCommand cmd(CLEARBREAKPOINT);
            cmd.arg(BREAKPOINT, serviceNumberFor(request.responseId));
            runCommand(cmd);
            m_activeBreakpointsByResponseId.remove(request.responseId);
            m_hitCountsByResponseId.remove(request.responseId);
            m_serviceNumberByResponseId.remove(request.responseId);
        }
        emit breakpointEvent(request.requestId, BreakpointOp::Remove, true, {});
        break;
    case BreakpointOp::Update: {
        const BreakpointChangeRequest update = withKnownLocation(request);
        if (isExceptionBreakpoint(params.type)) {
            DebuggerCommand cmd(SETEXCEPTIONBREAK);
            cmd.arg(TYPE, ALL);
            if (params.enabled)
                cmd.arg(ENABLED, params.enabled);
            runCommand(cmd);
            emit breakpointEvent(request.requestId, BreakpointOp::Update, true, {});
        } else if (params.type == BreakpointOnQmlSignalEmit) {
            QmlDebug::QPacket rs(m_v8Client->dataStreamVersion());
            rs << params.functionName.toUtf8() << params.enabled;
            runDirectCommand(BREAKONSIGNAL, rs.data());
            emit breakpointEvent(request.requestId, BreakpointOp::Update, true, {});
        } else if (m_supportChangeBreakpoint && isEnabledOnlyChange(update)) {
            DebuggerCommand cmd(CHANGEBREAKPOINT);
            cmd.arg(BREAKPOINT, serviceNumberFor(request.responseId));
            cmd.arg(ENABLED, params.enabled);
            const quint64 requestId = request.requestId;
            runCommand(cmd, [this, requestId, update](const QVariantMap &resp) {
                const bool ok = resp.value(QLatin1String(SUCCESS)).toBool();
                if (ok)
                    m_activeBreakpointsByResponseId.insert(update.responseId, update);
                emit breakpointEvent(requestId, BreakpointOp::Update, ok, {});
            });
        } else if (request.responseId.isEmpty()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Update, false, {});
        } else {
            DebuggerCommand clearCmd(CLEARBREAKPOINT);
            clearCmd.arg(BREAKPOINT, serviceNumberFor(request.responseId));
            runCommand(clearCmd);
            m_activeBreakpointsByResponseId.remove(request.responseId);
            m_hitCountsByResponseId.remove(request.responseId);
            m_serviceNumberByResponseId.remove(request.responseId);
            setScriptBreakpoint(request.requestId, update);
        }
        break;
    }
    case BreakpointOp::EnableSub:
        // The debug service addresses a breakpoint as a whole, so there is no
        // single location to enable, and saying so is better than saying nothing.
        emit breakpointEvent(request.requestId, BreakpointOp::EnableSub, false, {});
        break;
    }
}

void QmlImpl::execute(const ExecutionRequest &request)
{
    switch (request.command) {
    case ExecutionCommand::Continue:
    case ExecutionCommand::StepIn:
    case ExecutionCommand::StepOver:
    case ExecutionCommand::StepOut:
    case ExecutionCommand::RunToLine:
        // There is nothing left to resume once the runtime is gone, and the
        // request would wait for a reply that cannot come.
        if (m_inferiorExited) {
            emit inferiorEvent(InferiorEvent::InferiorIll);
            return;
        }
        break;
    default:
        break;
    }

    switch (request.command) {
    case ExecutionCommand::Continue:
        // Symmetric to the interrupt below: a request that cannot go out is
        // answered right away, rather than left for a reply that never comes.
        if (m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::RunFailed);
            break;
        }
        emit inferiorEvent(InferiorEvent::RunRequested);
        m_inferiorRunning = true;
        runCommand({CONTINEDEBUGGING}, [this](const QVariantMap &) {
            emit inferiorEvent(InferiorEvent::RunOk);
        });
        break;
    case ExecutionCommand::Interrupt:
        if (!m_inferiorRunning) {
            emit inferiorEvent(InferiorEvent::StopOk);
            break;
        }
        m_interruptRequested = true;
        emit message(Tr::tr("Waiting for JavaScript engine to interrupt on next statement."),
                     StatusBar);
        runDirectCommand(INTERRUPT);
        break;
    case ExecutionCommand::StepIn:
    case ExecutionCommand::StepOver:
    case ExecutionCommand::StepOut: {
        DebuggerCommand cmd(CONTINEDEBUGGING);
        cmd.arg(STEPACTION, request.command == ExecutionCommand::StepIn ? IN
                           : request.command == ExecutionCommand::StepOut ? OUT : NEXT);
        emit inferiorEvent(InferiorEvent::RunRequested);
        m_inferiorRunning = true;
        runCommand(cmd, [this](const QVariantMap &) {
            emit inferiorEvent(InferiorEvent::RunOk);
        });
        break;
    }
    case ExecutionCommand::RunToLine: {
        // The service keeps one breakpoint per location, so a temporary one
        // there would replace the user's and its removal would drop both.
        const bool coveredByUserBreakpoint = std::any_of(
            m_activeBreakpointsByResponseId.cbegin(), m_activeBreakpointsByResponseId.cend(),
            [&request](const BreakpointChangeRequest &active) {
                const BreakpointParameters &params = active.params;
                return params.enabled && params.condition.isEmpty() && params.ignoreCount == 0
                       && params.fileName.fileName() == request.context.fileName.fileName()
                       && params.textPosition.line == request.context.textPosition.line;
            });
        m_runToLineFile = request.context.fileName;
        m_runToLineLine = request.context.textPosition.line;
        if (!coveredByUserBreakpoint) {
            DebuggerCommand cmd(SETBREAKPOINT);
            cmd.arg(TYPE, SCRIPTREGEXP);
            cmd.arg(TARGET, request.context.fileName.toUrlishString());
            cmd.arg(ENABLED, true);
            cmd.arg(LINE, request.context.textPosition.line - 1);
            runCommand(cmd, [this](const QVariantMap &resp) {
                if (!resp.value(QLatin1String(SUCCESS)).toBool())
                    return;
                const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
                m_runToLineBreakpoints.append(body.value(QLatin1String(BREAKPOINT)).toInt());
            });
        }
        emit message(Tr::tr("Run to line %1 (%2) requested...")
                         .arg(request.context.textPosition.line)
                         .arg(request.context.fileName.toUserOutput()),
                     StatusBar);

        emit inferiorEvent(InferiorEvent::RunRequested);
        m_inferiorRunning = true;
        runCommand({CONTINEDEBUGGING}, [this](const QVariantMap &) {
            emit inferiorEvent(InferiorEvent::RunOk);
        });
        break;
    }
    case ExecutionCommand::Detach:
        sendDisconnect();
        emit inferiorDone({0, InferiorExitStatus::Detached});
        break;
    case ExecutionCommand::ResetInferior:
        if (!std::holds_alternative<ProcessRunData>(m_startData.inferiorStartData)) {
            emit message("The runtime this session attached to is not ours to restart.",
                         LogWarning);
            emit inferiorEvent(InferiorEvent::RunFailed);
            break;
        }
        emit inferiorEvent(InferiorEvent::RunRequested);
        emit inferiorEvent(InferiorEvent::RunOk);
        // The done handler puts the fresh runtime in place, a kill of our own
        // reported as an exit would end the session instead.
        m_isResetRestart = true;
        m_inferiorProcess.kill();
        break;
    case ExecutionCommand::RunToFunction:
        // The action offering this is there whatever the backend is, and the
        // debug service places a breakpoint by file and line only, so there is
        // no function for it to run to.
        emit message(Tr::tr("The QML debug service places breakpoints by file and line only, "
                            "so it cannot be asked to run to a function."), LogWarning);
        break;
    case ExecutionCommand::Abort:
        // The user gave up, so nothing is asked of the runtime any more: the
        // connection goes, and the runtime with it where it is ours to end.
        m_aborting = true;
        m_connection.close();
        if (m_inferiorProcess.isRunning()) {
            m_inferiorProcess.kill();
            break;
        }
        emit engineProcessFinished({});
        break;
    case ExecutionCommand::RepeatLastCommand:
        if (m_lastLocalsRequest)
            refreshLocals(*m_lastLocalsRequest);
        break;
    default:
        break;
    }
}

static std::pair<QString, QString> v8TypeAndValue(const QVariantMap &data, int stringLimit)
{
    const QString type = data.value(QLatin1String(TYPE)).toString();
    const QVariant value = data.value(QLatin1String(VALUE));
    if (type == "undefined")
        return {"undefined", "undefined"};
    if (type == "null")
        return {"object", "null"};
    if (type == "string") {
        QString text = value.toString();
        if (stringLimit > 0 && text.size() > stringLimit)
            text = text.left(stringLimit) + "...";
        return {type, '"' + text + '"'};
    }
    if (type == "boolean" || type == "number")
        return {type, value.toString()};
    if (type == "function")
        return {type, data.value(QLatin1String(NAME)).toString()};
    if (type == "object") {
        if (data.contains(QLatin1String(VALUE)) && (!value.isValid() || value.isNull()))
            return {type, "null"};
        return {type, "{...}"};
    }
    return {type, value.toString()};
}

std::shared_ptr<QmlImpl::RefreshCollector> QmlImpl::makeCollector(const RefreshRequest &request)
{
    const auto pending = std::make_shared<RefreshCollector>();
    pending->requestId = request.requestId;
    pending->kind = request.kind;
    pending->items.m_type = GdbMi::List;
    pending->items.m_name = QStringLiteral("data");
    pending->expandedINames = request.expandedINames;
    return pending;
}

// The view merges a partial answer into the tree it already has, so what such
// an answer must not carry is the locals the request did not name.
static GdbMi keptForPartialRefresh(const GdbMi &items, const QString &iname)
{
    GdbMi kept;
    kept.m_type = items.m_type;
    kept.m_name = items.m_name;
    for (const GdbMi &item : items) {
        const QString itemIName = item["iname"].data();
        if (itemIName == iname || itemIName.startsWith(iname + '.'))
            kept.addChild(item);
    }
    return kept;
}

std::function<void()> QmlImpl::legFinisher(const std::shared_ptr<RefreshCollector> &pending)
{
    return [this, pending] {
        if (--pending->remaining > 0)
            return;
        if (pending->kind == RefreshKind::InspectorTree && pending->items.childCount() > 0) {
            for (const int debugId : std::as_const(pending->seenDebugIds))
                addObjectWatch(debugId);
            removeObjectWatchesExcept(pending->seenDebugIds);
            if (pending->partialVariable.isEmpty())
                m_debugIdsWithProperties = pending->debugIdsWithProperties;
            else
                m_debugIdsWithProperties.unite(pending->debugIdsWithProperties);
        }
        GdbMi all;
        all.m_type = GdbMi::Tuple;
        if (pending->partialVariable.isEmpty()) {
            all.addChild(pending->items);
        } else {
            all.addChild(keptForPartialRefresh(pending->items, pending->partialVariable));
            all.addConst(QStringLiteral("partial"), QStringLiteral("1"));
        }
        emit refreshDataReceived(pending->requestId, pending->kind, all);
    };
}

static int v8ChildCount(const QVariantMap &data)
{
    const QString type = data.value(QLatin1String(TYPE)).toString();
    if (type != "object" && type != "function")
        return 0;
    const QVariantList properties = data.value(QLatin1String("properties")).toList();
    if (!properties.isEmpty())
        return int(properties.size());
    const QVariant value = data.value(QLatin1String(VALUE));
    if (!value.isValid() || value.isNull())
        return 0;
    return value.toInt();
}

void QmlImpl::refreshLocals(const RefreshRequest &request)
{
    const QJsonArray watchers = request.watchers;
    m_lastLocalsRequest = request;
    // Nothing here reads a string in pieces, so the smaller of the two limits is
    // what decides how much of one reaches the view.
    m_stringLimit = qMin(request.dumperOptions.maximalStringLength,
                         request.dumperOptions.displayStringLimit);

    if (m_inferiorRunning) {
        m_deferredWatchers = request;
        return;
    }

    const auto pending = makeCollector(request);
    pending->partialVariable = request.partialVariable;
    pending->remaining = 2 + int(watchers.size());
    const auto finishLeg = legFinisher(pending);

    for (const QJsonValue &watcherValue : watchers) {
        const QJsonObject watcher = watcherValue.toObject();
        const QString iname = watcher.value("iname").toString();
        const QString hexExp = watcher.value("exp").toString();
        const QString exp = QString::fromUtf8(QByteArray::fromHex(hexExp.toLatin1()));
        GdbMi item;
        item.m_type = GdbMi::Tuple;
        item.addConst(QStringLiteral("iname"), iname);
        item.addConst(QStringLiteral("wname"), hexExp);
        evaluateIntoItem(item, iname, exp, pending, finishLeg);
    }

    // The frame names its receiver by a handle alone, with nothing to show.
    GdbMi thisItem;
    thisItem.m_type = GdbMi::Tuple;
    thisItem.addConst(QStringLiteral("iname"), QStringLiteral("local.this"));
    thisItem.addConst(QStringLiteral("name"), QStringLiteral("this"));
    thisItem.addConst(QStringLiteral("exp"), QStringLiteral("this"));
    evaluateIntoItem(thisItem, QStringLiteral("local.this"), QStringLiteral("this"), pending,
                     finishLeg);

    DebuggerCommand frameCmd(FRAME);
    frameCmd.arg(NUMBER, m_currentFrameIndex);
    runCommand(frameCmd, [this, pending, finishLeg](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QVariantList scopes = body.value(QLatin1String("scopes")).toList();
        for (const QVariant &scopeValue : scopes) {
            const QVariantMap scope = scopeValue.toMap();
            if (scope.value(QLatin1String(TYPE)).toInt() == 0)
                continue;
            ++pending->remaining;
            DebuggerCommand scopeCmd(SCOPE);
            scopeCmd.arg(NUMBER, scope.value(QLatin1String("index")).toInt());
            scopeCmd.arg(FRAMENUMBER, m_currentFrameIndex);
            runCommand(scopeCmd, [this, pending, finishLeg](const QVariantMap &scopeResp) {
                handleScopeReply(scopeResp, pending, finishLeg);
                finishLeg();
            });
        }
        finishLeg();
    });
}

// The item comes with its names, the evaluation adds what it is.
void QmlImpl::evaluateIntoItem(GdbMi item, const QString &iname, const QString &exp,
                               const std::shared_ptr<RefreshCollector> &pending,
                               const std::function<void()> &finishLeg)
{
    DebuggerCommand cmd(EVALUATE);
    cmd.arg(EXPRESSION, exp);
    cmd.arg(FRAME, m_currentFrameIndex);
    runCommand(cmd, [this, item, iname, exp, pending, finishLeg](const QVariantMap &resp) mutable {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        int numchild = 0;
        if (resp.value(QLatin1String(SUCCESS)).toBool()) {
            const auto [type, value] = v8TypeAndValue(body, m_stringLimit);
            numchild = v8ChildCount(body);
            item.addConst(QStringLiteral("type"), type);
            item.addConst(QStringLiteral("value"), value);
        } else {
            item.addConst(QStringLiteral("value"), body.value(QLatin1String("text")).toString());
        }
        item.addConst(QStringLiteral("numchild"), QString::number(numchild));
        pending->items.addChild(item);
        if (numchild > 0 && pending->expandedINames.contains(iname)) {
            const int handle = body.value(QLatin1String(REF),
                                          body.value(QLatin1String(HANDLE))).toInt();
            QList<LookupRequest> lookups = appendV8Children(iname, exp, body, pending);
            if (lookups.isEmpty() && handle != 0 && !body.contains(QLatin1String("properties")))
                lookups.append({handle, iname, exp, exp});
            lookupHandles(lookups, pending, finishLeg);
        }
        finishLeg();
    });
}

static GdbMi watchItem(const QString &iname, const QString &name, const QString &exp,
                       const QString &type, const QString &value, int numchild,
                       int debugId = -1)
{
    GdbMi item;
    item.m_type = GdbMi::Tuple;
    item.addConst(QStringLiteral("iname"), iname);
    item.addConst(QStringLiteral("name"), name);
    if (debugId != -1)
        item.addConst(QStringLiteral("id"), QString::number(debugId));
    if (!exp.isEmpty())
        item.addConst(QStringLiteral("exp"), exp);
    if (!type.isEmpty())
        item.addConst(QStringLiteral("type"), type);
    item.addConst(QStringLiteral("value"), value);
    item.addConst(QStringLiteral("numchild"), QString::number(numchild));
    return item;
}

void QmlImpl::handleScopeReply(const QVariantMap &response,
                               const std::shared_ptr<RefreshCollector> &pending,
                               const std::function<void()> &finishLeg)
{
    const QVariantMap body = response.value(QLatin1String(BODY)).toMap();
    const QVariantMap object = body.value(QLatin1String("object")).toMap();

    QList<LookupRequest> lookups;
    for (const QVariant &propertyValue : object.value(QLatin1String("properties")).toList()) {
        const QVariantMap property = propertyValue.toMap();
        const QString name = property.value(QLatin1String(NAME)).toString();
        if (name.isEmpty() || name.startsWith('.'))
            continue;

        const QString iname = "local." + name;
        const int numchild = v8ChildCount(property);
        const auto [type, value] = v8TypeAndValue(property, m_stringLimit);
        const int handle = property.value(QLatin1String(REF),
                                         property.value(QLatin1String(HANDLE))).toInt();

        if (numchild > 0) {
            pending->items.addChild(watchItem(iname, name, name, type, value, numchild));
            if (handle != 0 && pending->expandedINames.contains(iname))
                lookups.append({handle, iname, name, name});
            continue;
        }
        if (!property.contains(QLatin1String(VALUE)) && handle != 0) {
            lookups.append({handle, iname, name, name});
            continue;
        }
        pending->items.addChild(watchItem(iname, name, name, type, value, 0));
    }

    lookupHandles(lookups, pending, finishLeg);
}

QList<QmlImpl::LookupRequest> QmlImpl::appendV8Children(
    const QString &iname, const QString &exp, const QVariantMap &resolved,
    const std::shared_ptr<RefreshCollector> &pending)
{
    QList<LookupRequest> nextRound;
    const auto [parentType, parentValue] = v8TypeAndValue(resolved, m_stringLimit);
    for (const QVariant &childValue : resolved.value(QLatin1String("properties")).toList()) {
        const QVariantMap child = childValue.toMap();
        const QString childName = child.value(QLatin1String(NAME)).toString();
        if (childName.isEmpty() || childName.startsWith('.'))
            continue;
        const QString childExp = parentValue == "Array" ? QString(exp + '[' + childName + ']')
                                                        : QString(exp + '.' + childName);
        const QString childIName = iname + '.' + childName;
        const auto [childType, childText] = v8TypeAndValue(child, m_stringLimit);
        const int childNumChild = v8ChildCount(child);
        pending->items.addChild(watchItem(childIName, childName, childExp, childType, childText,
                                          childNumChild));

        const int childHandle = child.value(QLatin1String(REF),
                                            child.value(QLatin1String(HANDLE))).toInt();
        const bool childExpanded = childNumChild > 0
                                   && pending->expandedINames.contains(childIName);
        if (childHandle != 0 && (childType.isEmpty() || childExpanded))
            nextRound.append({childHandle, childIName, childName, childExp});
    }
    return nextRound;
}

void QmlImpl::lookupHandles(const QList<LookupRequest> &requests,
                            const std::shared_ptr<RefreshCollector> &pending,
                            const std::function<void()> &finishLeg)
{
    if (requests.isEmpty())
        return;

    QHash<int, LookupRequest> requestForHandle;
    QList<int> handles;
    for (const LookupRequest &request : requests) {
        if (requestForHandle.contains(request.handle))
            continue;
        requestForHandle.insert(request.handle, request);
        handles.append(request.handle);
    }

    ++pending->remaining;
    DebuggerCommand cmd(LOOKUP);
    cmd.arg(HANDLES, handles);
    runCommand(cmd, [this, pending, requestForHandle, finishLeg](const QVariantMap &lookupResp) {
        const QVariantMap body = lookupResp.value(QLatin1String(BODY)).toMap();
        QList<LookupRequest> nextRound;
        for (auto it = body.begin(), end = body.end(); it != end; ++it) {
            const auto requestIt = requestForHandle.constFind(it.key().toInt());
            if (requestIt == requestForHandle.constEnd())
                continue;
            const QString iname = requestIt->iname;
            const QVariantMap resolved = it.value().toMap();
            const auto [type, value] = v8TypeAndValue(resolved, m_stringLimit);
            const QVariantList properties = resolved.value(QLatin1String("properties")).toList();

            const int numchild = properties.isEmpty() ? v8ChildCount(resolved)
                                                      : int(properties.size());
            pending->items.addChild(watchItem(iname, requestIt->name, requestIt->exp, type, value,
                                              numchild));

            nextRound.append(appendV8Children(iname, requestIt->exp, resolved, pending));
        }
        lookupHandles(nextRound, pending, finishLeg);
        finishLeg();
    });
}

GdbMi QmlImpl::emptyLocalsData()
{
    GdbMi items;
    items.m_type = GdbMi::List;
    items.m_name = QStringLiteral("data");
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(items);
    return all;
}

void QmlImpl::refreshConsoleItem(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    const qint64 handle = request.handle;
    DebuggerCommand cmd(LOOKUP);
    cmd.arg(HANDLES, QList<int>{int(handle)});
    runCommand(cmd, [this, requestId, handle](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QString key = QString::number(handle);
        if (!resp.value(QLatin1String(SUCCESS)).toBool() || !body.contains(key)) {
            emit refreshFailed(requestId, RefreshKind::ConsoleItem, {});
            return;
        }
        emit refreshDataReceived(requestId, RefreshKind::ConsoleItem,
                                 consoleItemFromV8(body.value(key).toMap(), {}));
    });
}

void QmlImpl::refreshSourceFiles(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    DebuggerCommand cmd(SCRIPTS);
    cmd.arg(TYPES, 4);
    runCommand(cmd, [this, requestId](const QVariantMap &resp) {
        GdbMi files;
        files.m_type = GdbMi::List;
        const QVariantList scripts = resp.value(QLatin1String(BODY)).toList();
        for (const QVariant &scriptValue : scripts) {
            const QString name = scriptValue.toMap().value(QLatin1String(NAME)).toString();
            if (name.isEmpty())
                continue;
            // The view opens a source file by its full name, and a script names
            // itself by a url.
            const QUrl url(name);
            GdbMi entry;
            entry.m_type = GdbMi::Tuple;
            entry.addConst(QStringLiteral("file"), name);
            entry.addConst(QStringLiteral("fullname"),
                           url.isLocalFile() ? url.toLocalFile() : name);
            files.addChild(entry);
        }
        emit refreshDataReceived(requestId, RefreshKind::SourceFiles, files);
    });
}

static const int MaxInspectorTreeNodes = 100000;

static int valueChildCount(const QVariant &value)
{
    if (value.typeId() == QMetaType::QVariantMap)
        return int(value.toMap().size());
    if (value.typeId() == QMetaType::QVariantList)
        return int(value.toList().size());
    return 0;
}

static void appendValueChildren(const QString &parentIname, const QVariant &value,
                                int debugId, GdbMi *items)
{
    const auto appendOne = [&](const QString &name, const QVariant &childValue) {
        const QString iname = parentIname + '.' + name;
        GdbMi child = watchItem(iname, name, {}, QLatin1String(childValue.typeName()),
                                childValue.toString(), valueChildCount(childValue), debugId);
        child.addConst(QStringLiteral("valueeditable"), QStringLiteral("false"));
        items->addChild(child);
        appendValueChildren(iname, childValue, debugId, items);
    };

    if (value.typeId() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        for (auto it = map.begin(), end = map.end(); it != end; ++it)
            appendOne(it.key(), it.value());
    } else if (value.typeId() == QMetaType::QVariantList) {
        const QVariantList list = value.toList();
        for (int i = 0, end = int(list.size()); i != end; ++i)
            appendOne(QString::number(i), list.at(i));
    }
}

bool QmlImpl::runInspectorQuery(quint32 queryId, const InspectorCallback &cb)
{
    if (queryId == 0)
        return false;
    m_inspectorCallbackForQueryId.insert(queryId, cb);
    return true;
}

// A query the client refused to send never answers, so the leg counted on it here
// has to be finished right away.
void QmlImpl::runInspectorLeg(quint32 queryId, const std::shared_ptr<RefreshCollector> &pending,
                              const std::function<void()> &finishLeg, const InspectorCallback &cb)
{
    ++pending->remaining;
    if (!runInspectorQuery(queryId, cb))
        finishLeg();
}

void QmlImpl::addObjectWatch(int debugId)
{
    if (debugId == -1 || m_objectWatches.contains(debugId))
        return;
    if (const quint32 watch = m_engineClient->addWatch(debugId))
        m_objectWatches.insert(debugId, watch);
}

// The protocol does not report destroyed objects, so whatever a complete tree
// no longer has is gone or out of sight, and its changes are of no interest.
void QmlImpl::removeObjectWatchesExcept(const QSet<int> &debugIds)
{
    for (auto it = m_objectWatches.begin(); it != m_objectWatches.end();) {
        if (debugIds.contains(it.key())) {
            ++it;
        } else {
            m_engineClient->removeWatch(it.value());
            it = m_objectWatches.erase(it);
        }
    }
}

void QmlImpl::appendObjectItems(const QmlDebug::ObjectReference &object,
                                const QString &parentIname, int engineId,
                                const std::shared_ptr<RefreshCollector> &pending,
                                const std::function<void()> &finishLeg)
{
    const int debugId = object.debugId();
    if (!object.isValid())
        return;
    if (pending->items.childCount() > MaxInspectorTreeNodes)
        return;

    const QString iname = parentIname + '.' + QString::number(debugId);
    m_inameForDebugId.insert(debugId, iname);
    m_sourceForDebugId.insert(debugId, object.source());
    m_engineIdForDebugId.insert(debugId, engineId);
    pending->seenDebugIds.insert(debugId);

    QString name = object.idString();
    if (name.isEmpty())
        name = object.className();
    if (name.isEmpty())
        name = object.name();
    if (name.isEmpty()) {
        const QmlDebug::FileReference file = object.source();
        name = file.url().fileName() + ':' + QString::number(file.lineNumber());
    }
    if (name.isEmpty())
        name = Tr::tr("<anonymous>");

    pending->items.addChild(watchItem(iname, name, name, object.className(),
                                     QStringLiteral("object"), 1, debugId));
    addObjectWatch(debugId);

    const bool expanded = pending->expandedINames.contains(iname);
    if (!expanded && object.needsMoreData())
        return;

    if (expanded && object.needsMoreData()) {
        runInspectorLeg(m_engineClient->queryObject(debugId), pending, finishLeg,
                        [this, parentIname, engineId, pending, finishLeg]
                        (const QVariant &value, const QByteArray &) {
            appendObjectItems(qvariant_cast<QmlDebug::ObjectReference>(value), parentIname,
                              engineId, pending, finishLeg);
            finishLeg();
        });
        return;
    }

    const QList<QmlDebug::PropertyReference> properties = object.properties();
    if (!properties.isEmpty()) {
        const QString propertiesIName = iname + ".[properties]";
        pending->debugIdsWithProperties.insert(debugId);
        pending->items.addChild(watchItem(propertiesIName, Tr::tr("Properties"), {}, {},
                                         QStringLiteral("list"), int(properties.size()),
                                         debugId));
        for (const QmlDebug::PropertyReference &property : properties) {
            const QString propertyName = property.name();
            if (propertyName.isEmpty())
                continue;
            const QString propertyIName = propertiesIName + '.' + propertyName;
            pending->items.addChild(watchItem(propertyIName, propertyName, propertyName,
                                             property.valueTypeName(),
                                             property.value().toString(),
                                             valueChildCount(property.value()), debugId));
            appendValueChildren(propertyIName, property.value(), debugId, &pending->items);
        }
    }

    const QList<QmlDebug::ObjectReference> children = object.children();
    for (const QmlDebug::ObjectReference &child : children)
        appendObjectItems(child, iname, engineId, pending, finishLeg);
}

void QmlImpl::refreshInspectorTree(const RefreshRequest &request)
{
    const quint64 requestId = request.requestId;
    if (!m_engineClient || m_engineClient->state() != QmlDebug::QmlDebugClient::Enabled) {
        emit refreshDataReceived(requestId, RefreshKind::InspectorTree, emptyLocalsData());
        return;
    }

    const auto pending = makeCollector(request);
    const auto finishLeg = legFinisher(pending);

    runInspectorLeg(m_engineClient->queryAvailableEngines(), pending, finishLeg,
                    [this, pending, finishLeg](const QVariant &value, const QByteArray &) {
        const auto engines = qvariant_cast<QList<QmlDebug::EngineReference>>(value);
        if (engines.isEmpty()) {
            if (m_engineQueryRetriesLeft > 0) {
                --m_engineQueryRetriesLeft;
                QTimer::singleShot(100, this, [this] { rebuildInspectorTree(); });
            }
            finishLeg();
            return;
        }
        m_qmlEngines = engines;
        for (const QmlDebug::EngineReference &engine : engines) {
            const int engineId = engine.debugId();
            QString name = engine.name();
            if (name.isEmpty())
                name = Tr::tr("Engine %1").arg(engineId);
            const QString iname = "inspect." + QString::number(engineId);
            m_inameForDebugId.insert(engineId, iname);
            m_engineIdForDebugId.insert(engineId, engineId);
            pending->items.addChild(watchItem(iname, name, {}, {},
                                             QStringLiteral("object"), 1, engineId));
            runInspectorLeg(m_engineClient->queryRootContexts(engine), pending, finishLeg,
                            [this, pending, finishLeg, iname, engineId]
                            (const QVariant &contextValue, const QByteArray &) {
                QList<QmlDebug::ContextReference> contexts{
                    qvariant_cast<QmlDebug::ContextReference>(contextValue)};
                int visited = 0;
                while (!contexts.isEmpty()) {
                    const QmlDebug::ContextReference context = contexts.takeLast();
                    const QList<QmlDebug::ObjectReference> objects = context.objects();
                    for (const QmlDebug::ObjectReference &object : objects)
                        appendObjectItems(object, iname, engineId, pending, finishLeg);
                    if (++visited > MaxInspectorTreeNodes)
                        break;
                    contexts.append(context.contexts());
                }
                appendParentlessObjects(iname, engineId, pending, finishLeg);
                finishLeg();
            });
        }
        finishLeg();
    });
}

// Delegates have no parent, and there can be thousands of them. Keep them
// below a collapsed item of their engine, and fetch them only when it is
// expanded. Only the headers: properties come on expansion.
void QmlImpl::appendParentlessObjects(const QString &engineIName, int engineId,
                                      const std::shared_ptr<RefreshCollector> &pending,
                                      const std::function<void()> &finishLeg)
{
    QList<int> debugIds;
    for (auto it = m_knownDelegateIds.cbegin(), end = m_knownDelegateIds.cend(); it != end; ++it) {
        if (it.value() == engineId && !pending->seenDebugIds.contains(it.key()))
            debugIds.append(it.key());
    }
    if (debugIds.isEmpty())
        return;

    const QString iname = engineIName + ".[parentless]";
    const auto itemAdded = std::make_shared<bool>(false);
    const auto addItem = [pending, iname, itemAdded] {
        if (std::exchange(*itemAdded, true))
            return;
        pending->items.addChild(watchItem(iname, Tr::tr("Other"), {}, {},
                                         QStringLiteral("list"), 1));
    };
    const bool expanded = pending->expandedINames.contains(iname);
    if (expanded) {
        addItem();
    } else {
        // The protocol does not report destroyed objects, only the answer to
        // a query for one tells. Check a bounded batch per rebuild, and show
        // the item once one of them answers.
        constexpr int BatchSize = 256;
        std::sort(debugIds.begin(), debugIds.end());
        const auto first = std::upper_bound(debugIds.begin(), debugIds.end(),
                                            m_validatedDelegateId);
        const qsizetype count = std::min<qsizetype>(debugIds.end() - first, BatchSize);
        if (count < debugIds.size())
            addItem();
        debugIds = QList<int>(first, first + count);
        m_validatedDelegateId = count < BatchSize ? -1 : debugIds.last();
    }
    for (const int debugId : std::as_const(debugIds)) {
        runInspectorLeg(m_engineClient->queryObjectHeader(debugId), pending, finishLeg,
                        [this, pending, finishLeg, iname, engineId, debugId, expanded, addItem]
                        (const QVariant &objectValue, const QByteArray &) {
            const auto object = qvariant_cast<QmlDebug::ObjectReference>(objectValue);
            // The application answers with nothing for a destroyed object.
            if (!object.isValid())
                m_knownDelegateIds.remove(debugId);
            else if (expanded)
                appendObjectItems(object, iname, engineId, pending, finishLeg);
            else
                addItem();
            finishLeg();
        });
    }
}

void QmlImpl::rebuildInspectorTree()
{
    if (m_inspectorTreeShown)
        emit inspectorTreeOutdated();
}

void QmlImpl::showInspectorTree(bool shown)
{
    if (shown == m_inspectorTreeShown)
        return;
    m_inspectorTreeShown = shown;
    if (shown)
        rebuildInspectorTree();
    else
        m_objectCreatedTimer->stop();
}

void QmlImpl::handleObjectCreated(int engineId, int objectId, int parentId)
{
    if (parentId == -1 && objectId != -1)
        m_knownDelegateIds.insert(objectId, engineId);

    for (const QmlDebug::EngineReference &engine : std::as_const(m_qmlEngines)) {
        if (engine.debugId() == engineId) {
            m_objectCreatedTimer->start();
            return;
        }
    }
}

void QmlImpl::handlePropertyValueChanged(int debugId, const QByteArray &name,
                                         const QVariant &value)
{
    if (!m_debugIdsWithProperties.contains(debugId))
        return;
    const QString objectIName = m_inameForDebugId.value(debugId);
    if (objectIName.isEmpty())
        return;
    const QString iname = objectIName + ".[properties]." + QString::fromLatin1(name);

    GdbMi items;
    items.m_type = GdbMi::List;
    items.m_name = QStringLiteral("data");
    items.addChild(watchItem(iname, QString::fromLatin1(name), QString::fromLatin1(name), {},
                             value.toString(), valueChildCount(value), debugId));
    appendValueChildren(iname, value, debugId, &items);
    GdbMi all;
    all.m_type = GdbMi::Tuple;
    all.addChild(items);
    emit refreshDataReceived(0, RefreshKind::InspectorTree, all);
}

void QmlImpl::queryObjectExpression(int debugId, const QString &expression)
{
    const int engineId = m_engineIdForDebugId.value(debugId, -1);
    const quint32 queryId = m_engineClient
                            ? m_engineClient->queryExpressionResult(debugId, expression, engineId)
                            : 0;
    const bool queried = runInspectorQuery(queryId, [this](const QVariant &value,
                                                           const QByteArray &) {
        if (value.isValid())
            emit consoleItemReported(consoleItemFromValue(value));
    });
    if (!queried) {
        emit message(Tr::tr("The application has to be stopped in a breakpoint in order to "
                            "evaluate expressions."), ConsoleOutput);
    }
}

void QmlImpl::refresh(const RefreshRequest &request)
{
    if (request.kind == RefreshKind::Locals) {
        refreshLocals(request);
        return;
    }
    if (request.kind == RefreshKind::InspectorTree) {
        refreshInspectorTree(request);
        return;
    }
    if (request.kind == RefreshKind::ConsoleItem) {
        refreshConsoleItem(request);
        return;
    }
    if (request.kind == RefreshKind::SourceFiles) {
        refreshSourceFiles(request);
        return;
    }
    // Nothing behind the debug service has dumpers or symbols to load.
    if (request.kind == RefreshKind::DebuggingHelpers || request.kind == RefreshKind::AllSymbols)
        return;
    if (request.kind != RefreshKind::FullStack && request.kind != RefreshKind::FullBacktrace)
        return;

    const quint64 requestId = request.requestId;
    const RefreshKind kind = request.kind;
    // A full backtrace is about everything there is, so a depth limit meant for
    // the stack view does not apply to it.
    const int depthLimit = kind == RefreshKind::FullBacktrace ? -1 : request.stackDepthLimit;
    // The debug service answers with the first ten frames only unless the range
    // is spelled out, which is not what either the stack view or a backtrace is
    // after when nothing limits them.
    DebuggerCommand cmd(BACKTRACE);
    cmd.arg("fromFrame", 0);
    cmd.arg("toFrame", depthLimit >= 0 ? depthLimit : 10000);
    runCommand(cmd, [this, requestId, kind](const QVariantMap &resp) {
        const QVariantMap body = resp.value(QLatin1String(BODY)).toMap();
        const QVariantList v8Frames = body.value(QLatin1String("frames")).toList();

        GdbMi frames;
        frames.m_type = GdbMi::List;
        for (const QVariant &v8FrameVal : v8Frames) {
            const QVariantMap v8Frame = v8FrameVal.toMap();
            QString function = v8Frame.value(QLatin1String(FUNCTION)).toString();
            if (function.isEmpty())
                function = v8Frame.value(QLatin1String("func")).toString();
            if (function.isEmpty())
                function = Tr::tr("Anonymous Function");

            GdbMi frame;
            frame.m_type = GdbMi::Tuple;
            frame.addConst(QLatin1String("level"),
                           QString::number(v8Frame.value(QLatin1String("index")).toInt()));
            frame.addConst(QLatin1String(FUNCTION), function);
            // A frame names its script as a URL, which is what a stop reports too.
            const QVariant script = v8Frame.value(QLatin1String("script"));
            const QString scriptName = script.typeId() == QMetaType::QVariantMap
                                           ? script.toMap().value(QLatin1String(NAME)).toString()
                                           : script.toString();
            frame.addConst(QLatin1String("file"), toLocalFile(scriptName).toUrlishString());
            frame.addConst(QLatin1String(LINE),
                           QString::number(v8Frame.value(QLatin1String(LINE)).toInt() + 1));
            frame.addConst(QLatin1String("language"), QStringLiteral("js"));
            frames.addChild(frame);
        }

        if (kind == RefreshKind::FullBacktrace) {
            QString text;
            for (const GdbMi &frame : frames) {
                text += QString("#%1  %2 at %3:%4\n")
                            .arg(frame["level"].data(), frame[FUNCTION].data(),
                                 frame["file"].data(), frame[LINE].data());
            }
            emit refreshDataReceived(requestId, RefreshKind::FullBacktrace,
                                     GdbMi::makeConst({}, text));
            return;
        }

        frames.m_name = QStringLiteral("frames");
        GdbMi stack;
        stack.m_type = GdbMi::Tuple;
        stack.m_name = QStringLiteral("stack");
        stack.addChild(frames);

        GdbMi data;
        data.m_type = GdbMi::Tuple;
        data.addChild(stack);
        emit refreshDataReceived(requestId, RefreshKind::FullStack, data);
    });
}

void QmlImpl::activateFrame(int index)
{
    m_currentFrameIndex = index;
}

void QmlImpl::selectThread(const QString &)
{
}

void QmlImpl::selectInspectorItem(const QString &iname)
{
    // The rows below an object stand for the object they belong to.
    const qsizetype properties = iname.indexOf(".[properties]");
    const int debugId = m_inameForDebugId.key(properties == -1 ? iname : iname.left(properties),
                                              -1);
    if (debugId == -1)
        return;
    m_toolsClient->selectObjects({debugId});
    reportSelectedObject(debugId, iname);
}

// An object the tree has not fetched yet is looked up for its source alone.
void QmlImpl::reportSelectedObject(int debugId, const QString &iname)
{
    const auto report = [this, debugId, iname](const QmlDebug::FileReference &source) {
        const FilePath file = source.url().isEmpty() ? FilePath()
                                                     : toLocalFile(source.url().toString());
        emit inspectorObjectSelected(iname.isEmpty() ? m_inameForDebugId.value(debugId) : iname,
                                     file, source.lineNumber());
    };
    if (m_sourceForDebugId.contains(debugId)) {
        report(m_sourceForDebugId.value(debugId));
        return;
    }
    runInspectorQuery(m_engineClient->queryObject(debugId),
                      [report](const QVariant &value, const QByteArray &) {
        report(qvariant_cast<QmlDebug::ObjectReference>(value).source());
    });
}

void QmlImpl::setSelectTool(bool active)
{
    if (m_toolsClient->state() != QmlDebug::QmlDebugClient::Enabled)
        return;
    m_toolsClient->setDesignModeBehavior(active);
    if (active)
        m_toolsClient->changeToSelectTool();
}

void QmlImpl::setShowAppOnTop(bool onTop)
{
    if (m_toolsClient->state() == QmlDebug::QmlDebugClient::Enabled)
        m_toolsClient->showAppOnTop(onTop);
}

void QmlImpl::executeDebuggerCommand(const QString &command, const WatchItemData &inspectorItem)
{
    const bool hasObject = inspectorItem.isInspect && inspectorItem.id != -1;
    if (!m_inferiorRunning) {
        evaluateForConsole(command, m_currentFrameIndex, {});
    } else if (m_unpausedEvaluate) {
        evaluateForConsole(command, {},
                           hasObject && m_contextEvaluate ? std::optional(inspectorItem.id)
                                                          : std::nullopt);
    } else if (hasObject) {
        queryObjectExpression(int(inspectorItem.id), command);
    } else {
        emit message(Tr::tr("The application has to be stopped in a breakpoint in order to "
                            "evaluate expressions."), ConsoleOutput);
    }
}

void QmlImpl::evaluateForConsole(const QString &expression, std::optional<int> frame,
                                 std::optional<qint64> context)
{
    DebuggerCommand cmd(EVALUATE);
    cmd.arg(EXPRESSION, expression);
    if (frame)
        cmd.arg(FRAME, *frame);
    if (context)
        cmd.arg(CONTEXT, *context);
    runCommand(cmd, [this](const QVariantMap &resp) {
        const bool success = resp.value(QLatin1String(SUCCESS)).toBool();
        if (!success) {
            emit consoleItemReported(consoleItem(QStringLiteral("error"),
                                                 resp.value(QLatin1String(MESSAGE)).toString()));
            return;
        }
        QHash<int, QVariantMap> refs;
        for (const QVariant &ref : resp.value(QLatin1String(REFS)).toList()) {
            const QVariantMap refData = ref.toMap();
            refs.insert(refData.value(QLatin1String(HANDLE)).toInt(), refData);
        }
        emit consoleItemReported(
            consoleItemFromV8(resp.value(QLatin1String(BODY)).toMap(), refs));
        emit debuggerCommandDone();
    });
}

void QmlImpl::setRegisterValue(const QString &, const QString &) {}
void QmlImpl::accessMemory(MemoryOp, quint64, quint64, quint64, const QByteArray &) {}
void QmlImpl::fetchDisassembly(quint64, quint64, const QString &, const QString &,
                               DisassemblyFlavor) {}
void QmlImpl::setPeripheralRegisterValue(quint64, quint64) {}
void QmlImpl::watchPoint(quint64, const QPoint &) {}
void QmlImpl::createSnapshot(quint64) {}
void QmlImpl::assignValueInDebugger(const WatchItemData &item, const QString &expr,
                                    const QString &value)
{
    QString literal;
    if (item.type == "boolean") {
        literal = (value != "false" && value != "0") ? QStringLiteral("true")
                                                    : QStringLiteral("false");
    } else if (item.type == "number") {
        literal = value;
    } else {
        literal = '"' + QString(value).replace('"', QLatin1String("\\\"")) + '"';
    }
    const QString assignment = expr + " = " + literal + ';';

    if (item.isInspect) {
        if (item.id != -1)
            queryObjectExpression(int(item.id), assignment);
        return;
    }

    DebuggerCommand cmd(EVALUATE);
    cmd.arg(EXPRESSION, assignment);
    cmd.arg(FRAME, m_currentFrameIndex);
    runCommand(cmd, [this](const QVariantMap &resp) {
        if (!resp.value(QLatin1String(SUCCESS)).toBool()) {
            emit message(resp.value(QLatin1String(MESSAGE)).toString(), LogError);
            return;
        }
    });
}
} // namespace Debugger::Internal
