// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dapstartdata.h"

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>

#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QQueue>
#include <QSet>
#include <QTimer>

#include <optional>

namespace Debugger::Internal {

class DapClient;
class DebuggerEngine;
enum class DapEventType;
enum class DapResponseType;

// A backend speaking stock Debug Adapter Protocol to an adapter Qt Creator does
// not own. Only what the protocol itself defines is available: no Qt dumpers,
// so values are whatever the adapter chose to print.
class DEBUGGER_EXPORT DapImpl : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit DapImpl(const DapStartData &startData);
    ~DapImpl() override;

protected:
    void start() override;
    void shutdownInferior(ShutdownMode mode) override;
    void shutdownEngine() override;

    void execute(const ExecutionRequest &request) override;
    void changeBreakpoint(const BreakpointChangeRequest &request) override;
    void refresh(const RefreshRequest &request) override;

    void selectThread(const QString &threadId) override;
    void activateFrame(int index) override;
    void setRegisterValue(const QString &name, const QString &value) override;
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) override;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) override;
    void setPeripheralRegisterValue(quint64 address, quint64 value) override;
    void watchPoint(quint64 requestId, const QPoint &pnt) override;
    void createSnapshot(quint64 requestId) override;

    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) override;
    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) override;

    // A superset announces itself differently: its initialize carries what its
    // own host needs to set up.
    virtual void handleStarted();
    void handleFinished();
    void reportEngineSetup(bool success);
    void handleStandardError();
    // A superset extends the dispatch: whatever it does not claim lands here.
    virtual void handleResponse(DapResponseType type, const QJsonObject &response);
    virtual void handleEvent(DapEventType type, const QJsonObject &event);

    void askForTheStoppingSignal(const QString &description);
    void handleStopped(const QJsonObject &event);
    void handleStackTrace(const QJsonObject &response);
    void handleScopes(const QJsonObject &response);
    void handleVariables(const QJsonObject &response);
    void handleWatcher(const QJsonObject &response);
    void handleReadMemory(const QJsonObject &response);
    void handleDisassemble(const QJsonObject &response);
    void handleBreakpointsSet(const QJsonObject &response);
    void handleBreakpointChanged(const QJsonObject &event);
    void reportStop();
    void reportInferiorDone(InferiorResultData result);

    // Where the sources are now, against what the debug information calls
    // them: an adapter has no request for that, so the map is applied here.
    QString localSourcePath(const QString &reported) const;
    QString reportedSourcePath(const QString &local) const;

    int postRequest(const QString &command, const QJsonObject &arguments = {});
    void logRequest(int seq, const QString &command, const QJsonObject &arguments);
    void restartWatchdog();
    // The launch body is this layer's passthrough configuration; a superset
    // builds its own from what it was started with.
    virtual void postLaunchOrAttach();
    void reportUnsupported(const QString &what);

    void sendCustomRequest(const QString &command, const QJsonObject &arguments,
                           const DapSessionChannel::Answer &answer);
    void reportRunStarted(bool running = true);
    void reportRunning(bool running);
    void reportRunRequested();
    void reportRunResult(bool ok);

    const DapStartData m_startData;
    DapClient *m_client = nullptr;
    // Who is waiting for the answer to a request the adapter defines itself.
    QHash<int, DapSessionChannel::Answer> m_customRequests;
    // What has gone out without an answer, by sequence number, for the
    // watchdog to name.
    class PendingRequest
    {
    public:
        QString text;
        QString command;
        qint64 postTime = 0;
    };
    QHash<int, PendingRequest> m_pendingRequests;
    QTimer m_watchdog;
    bool m_runningReported = false;
    // The scopes request the locals walk running now belongs to.
    int m_scopesSeq = -1;
    // The register view's own walk: the scopes request that finds the register
    // scope, the variables request that reads it, and who is waiting.
    int m_registerScopesSeq = -1;
    int m_registerVariablesSeq = -1;
    int m_registerScopeReference = 0;
    quint64 m_registersRequestId = 0;

    int m_currentThreadId = -1;
    int m_currentFrameId = -1;
    bool m_inferiorRunning = false;
    bool m_stopRequested = false;
    bool m_stepRequested = false;
    bool m_configured = false;
    bool m_setupReported = false;
    // The engine has to hear that the run began before it hears it ended,
    // and an adapter whose debuggee exits at once says both before the
    // configuration it was still answering has been acknowledged.
    bool m_runReported = false;
    bool m_runRequestPending = false;
    // Whether the next stop ends the setup rather than a run of its own.
    bool m_reportsSetupStop = false;
    std::optional<InferiorResultData> m_pendingResult;
    bool m_inferiorDoneReported = false;
    // A detach ends the session without ending the debuggee, and the adapter
    // reports the end of the session the same way either way.
    bool m_detaching = false;
    bool m_shuttingDown = false;

    // The stop event carries no frame, so the location has to be asked for.
    class StackTraceRequest
    {
    public:
        bool reportsStop = false;
        quint64 refreshRequestId = 0;
        // Whether a step brought the inferior here, which is the only stop the
        // skip list has a say over.
        bool fromStep = false;
    };
    QHash<int, StackTraceRequest> m_stackTraceRequests;
    // A foreign adapter hands out its own frame ids, so the view's index into
    // the stack has to be translated back through the ids it reported.
    QList<int> m_frameIds;

private:
    // The resume a run to a location is: the protocol has no request of its own
    // for it, so it is a breakpoint there plus an ordinary continue.
    void resumeForRunTo();
    void addInternalFunctionBreakpoint(const QString &function, bool oneShot);
    void sendBreakpointsFor(const Utils::FilePath &file);
    void sendFunctionBreakpoints();
    void sendExceptionBreakpoints();
    void sendDetach();
    void queueVariables(const QString &iname, int reference);
    void continueLocalsWalk();
    void continueBacktrace();
    void handleBacktraceFrames(const QJsonObject &response);
    void reportLocals();
    GdbMi localsItem(const QString &iname) const;

    // A breakpoint as the protocol wants it: whole-file arrays, so each one has
    // to be remembered even when only its neighbour changed.
    class Breakpoint
    {
    public:
        quint64 requestId = 0;
        BreakpointOp op = BreakpointOp::Insert;
        int modelId = 0;
        // What the adapter answered for it, which is how a later change to it
        // is named.
        QString responseId;
        BreakpointParameters params;
        bool enabled = true;
        // A breakpoint of the engine's own making, which nothing in the model
        // is waiting for an answer about.
        bool internal = false;
        // Whether it is taken back once it has been hit, which is how the
        // protocol's missing temporary breakpoint is made up for.
        bool oneShot = false;
    };
    QHash<Utils::FilePath, QList<Breakpoint>> m_sourceBreakpoints;
    QList<Breakpoint> m_functionBreakpoints;
    // Whichever of the adapter's exception filters are on, as breakpoints of
    // their own: they have no location, and no answer of their own either.
    QList<Breakpoint> m_exceptionBreakpoints;
    // Which file's answer a setBreakpoints reply is, routed by sequence number.
    QHash<int, Utils::FilePath> m_breakpointRequests;
    // The same for the function breakpoints, which are one array of their own.
    QSet<int> m_functionBreakpointRequests;
    // What the adapter called the breakpoints it made because it was asked to
    // here: it announces the ones made elsewhere the same way, and only the
    // name tells the two apart.
    QSet<QString> m_ownBreakpointIds;

    // Finds the request a change the adapter reports belongs to.
    const Breakpoint *breakpointForResponseId(const QString &responseId) const;
    // A breakpoint as the model reads it, out of what the adapter reported.
    GdbMi breakpointMi(const QJsonObject &item) const;

    // One node of the locals tree. The protocol answers a level at a time, so
    // the tree is collected flat and assembled once the walk is done.
    class Local
    {
    public:
        QString iname;
        QString name;
        QString type;
        QString value;
        quint64 address = 0;
        int reference = 0;
        bool hasChildren = false;
        QStringList childINames;
    };
    quint64 m_localsRequestId = 0;
    // The locals fetch, kept for RepeatLastCommand.
    std::optional<RefreshRequest> m_lastLocalsRequest;
    QSet<QString> m_expandedINames;
    QMap<QString, Local> m_locals;
    QStringList m_localRoots;
    QQueue<QPair<QString, int>> m_pendingVariables;
    // A watcher as the request named it: its iname and the expression to ask
    // the adapter to evaluate for it.
    QQueue<QPair<QString, QString>> m_pendingWatchers;
    QHash<int, QString> m_variableRequests;
    QHash<int, QPair<QString, QString>> m_watcherRequests;
    // A full backtrace is one stack per thread, and the protocol answers one
    // request at a time, so the walk keeps what it has and what is still to come.
    quint64 m_backtraceRequestId = 0;
    int m_backtraceThreadsSeq = -1;
    int m_backtraceFramesSeq = -1;
    QQueue<QPair<int, QString>> m_backtraceThreads;
    QString m_backtrace;
    QHash<int, quint64> m_threadRequests;
    QHash<int, quint64> m_sourceFilesRequests;
    QHash<int, quint64> m_moduleRequests;

    class MemoryRequest
    {
    public:
        quint64 requestId = 0;
        quint64 address = 0;
        quint64 length = 0;
    };
    QHash<int, MemoryRequest> m_memoryRequests;
    // A peripheral register is a memory read whose answer goes to the register
    // view rather than to a memory agent.
    QHash<int, MemoryRequest> m_peripheralRequests;

    class DisassemblyRequest
    {
    public:
        quint64 requestId = 0;
        quint64 address = 0;
    };
    QHash<int, DisassemblyRequest> m_disassemblyRequests;
};

DEBUGGER_EXPORT DebuggerEngine *createDapAdapterEngine(const DapStartData &data);

} // namespace Debugger::Internal
