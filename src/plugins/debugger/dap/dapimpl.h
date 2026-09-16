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
    void readWhatAWatchpointWatches(const QString &id, const QString &expression, bool report);
    void reportAlienWatchpointHit(const QString &id);
    void reportBreakpointHits(const QJsonArray &adapterIds);
    void reportThreadList();
    void handleStackTrace(const QJsonObject &response);
    void handleScopes(const QJsonObject &response);
    void handleVariables(const QJsonObject &response);
    void handleWatcher(const QJsonObject &response);
    void handleReadMemory(const QJsonObject &response);
    void handleDisassemble(const QJsonObject &response);
    void handleBreakpointsSet(const QJsonObject &response);
    void handleDataBreakpointInfo(const QJsonObject &response);
    void handleBreakpointChanged(const QJsonObject &event);
    void askWhatAnAlienBreakpointIs(const QJsonObject &item);
    void reportStop();
    void askWhetherASignalTookTheInferior(const InferiorResultData &result);
    void askWhatTheExitSignalIsCalled(int number);
    void reportThreadGroupGone();
    void reportTheExitWithoutItsSignal();
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
    void runUserStartupCommands();
    void applyTheStringLengthLimit(const DumperOptions &options);
    void liftTheDebuggerLimits();
    void setTheDisassemblyFlavor();
    void configureTheDebugInfoDaemon();
    void configureTheSearchPaths();
    void configureTheSymbolIndexAndForks();
    void runUserCommands(const QStringList &commands);
    void postReplCommand(const QString &command);
    void jumpOverTheConsole(const ContextData &context);
    void returnOverTheConsole();
    void runConsoleCommand(const QString &command, const QString &what);
    void loadSymbols(const QString &pattern, const QString &what);
    void fetchModuleSections(quint64 requestId, const Utils::FilePath &modulePath);
    void reportRegisters(quint64 requestId, const GdbMi &registers);
    void reportBreakpointInsert(quint64 requestId, bool taken, bool verified, const GdbMi &data,
                                const QString &number, int threadSpec);
    void reportModules(quint64 requestId, const GdbMi &modules);
    void fetchModuleSymbols(quint64 requestId, const Utils::FilePath &modulePath);
    void askWhereTheDebuggerSelectionIs(bool report);
    void reportUnsupported(const QString &what);
    void setBreakpointCommands(const QString &adapterId, const QString &command);

    void sendCustomRequest(const QString &command, const QJsonObject &arguments,
                           const DapSessionChannel::Answer &answer);
    void reportRunStarted(bool running = true);
    void reportRunning(bool running);
    void reportRunRequested();
    void reportRunResult(bool ok);
    void reportStoppedLocation(const Utils::FilePath &file, int line);
    void reportResumed();

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
    // The line a jump was asked for, to name it where the adapter answers with
    // no place in it to jump to.
    int m_jumpLine = 0;
    QTimer m_watchdog;
    bool m_runningReported = false;
    bool m_stopReported = false;
    // The scopes request the locals walk running now belongs to.
    int m_scopesSeq = -1;
    // The register view's own walk: the scopes request that finds the register
    // scope, the variables request that reads it, and who is waiting.
    int m_registerScopesSeq = -1;
    int m_registerVariablesSeq = -1;
    int m_registerScopeReference = 0;
    bool m_registerNamesFetched = false;
    quint64 m_registersRequestId = 0;
    // Which groups a register is in, as the debugger behind the adapter
    // answered once. It is the architecture that decides, so it stays.
    QHash<QString, QString> m_registerGroups;
    bool m_registerGroupsFetched = false;

    // What the debugger has selected, the thread and the frame in it, as it
    // answered last.
    QString m_debuggerSelection;

    // The process the debuggee's threads run in, as the adapter named it. The
    // threads view groups them by it, and it is what takes them away again.
    QString m_threadGroupId;

    int m_currentThreadId = -1;
    int m_currentFrameId = -1;
    bool m_inferiorRunning = false;
    bool m_stopRequested = false;
    bool m_stepRequested = false;
    // A resume that has gone out and has not been answered. An adapter that has
    // not answered it has not started the debuggee either, so an interrupt sent
    // in the meantime would be dropped: it waits here instead.
    bool m_resumeRequestPending = false;
    bool m_interruptWhenResumed = false;
    // Armed while the stub still has to be told to let go of the inferior.
    bool m_expectTerminalTrap = false;
    bool m_configured = false;
    bool m_setupReported = false;
    // The engine has to hear that the run began before it hears it ended,
    // and an adapter whose debuggee exits at once says both before the
    // configuration it was still answering has been acknowledged.
    bool m_runReported = false;
    bool m_runRequestPending = false;
    // Whether the next stop ends the setup rather than a run of its own.
    bool m_reportsSetupStop = false;
    // The major version gdb names in its banner, 0 for another adapter.
    int m_gdbMajorVersion = 0;
    std::optional<InferiorResultData> m_pendingResult;
    bool m_inferiorDoneReported = false;
    std::optional<InferiorResultData> m_exitAwaitingSignal;
    // A detach ends the session without ending the debuggee, and the adapter
    // reports the end of the session the same way either way.
    bool m_detaching = false;
    bool m_shuttingDown = false;
    // Armed while a reset takes the session down, so that neither its end nor
    // the debuggee's is reported as the session being over.
    bool m_isResetRestart = false;

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
    void sendInterrupt();
    void resumeAnswered(bool success);
    void addInternalFunctionBreakpoint(const QString &function, bool oneShot);
    // A condition and a hit count are optional parts of a protocol breakpoint,
    // so both are left out where the adapter announces neither.
    void addBreakpointConditions(QJsonObject &item, const BreakpointParameters &params);
    int sendBreakpointsFor(const Utils::FilePath &file);
    int sendFunctionBreakpoints();
    int sendInstructionBreakpoints();
    int sendDataBreakpoints();
    void sendExceptionBreakpoints();
    void sendDetach();
    // Whether the debuggee outlives the session is the caller's to say only
    // where the adapter announces it, so the flag is left out elsewhere.
    void sendDisconnect(bool terminateDebuggee);
    // What a reset is here: the protocol's restart request is optional, and an
    // adapter without it is put back the way the specification prescribes for
    // that case, the session ended and started anew.
    void restartSession();
    void queueVariables(const QString &iname, int reference);
    void continueLocalsWalk();
    void continueBacktrace();
    void handleBacktraceFrames(const QJsonObject &response);
    void reportLocals();
    // Whether the item is part of what the current locals fetch asks for: all
    // of them, or the one variable the request named and what is under it.
    bool isRequestedLocal(const QString &iname) const;
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
        // What the adapter calls it now: a session started anew hands out ids
        // of its own, while the id the model knows stays what it was.
        QString adapterId;
        // What the adapter calls the datum a watchpoint watches. Opaque, and
        // the only thing a data breakpoint can be expressed by.
        QString dataId;
        // How often the adapter has named it as a reason for a stop. The
        // protocol has no hit count of its own.
        int hitCount = 0;
    };
    // Which of the protocol's breakpoint arrays a breakpoint belongs to: each
    // of them goes out whole, and an answer names no more than the array it is
    // about.
    enum class BreakpointArray { Source, Function, Instruction, Data };
    QHash<Utils::FilePath, QList<Breakpoint>> m_sourceBreakpoints;
    QList<Breakpoint> m_functionBreakpoints;
    QList<Breakpoint> m_instructionBreakpoints;
    QList<Breakpoint> m_dataBreakpoints;
    // Whichever of the adapter's exception filters are on, as breakpoints of
    // their own: they have no location, and no answer of their own either.
    QList<Breakpoint> m_exceptionBreakpoints;
    // The catchpoints of the session, as the debugger behind the adapter
    // numbered them: the protocol has nothing to name one by.
    QList<Breakpoint> m_catchpoints;
    // The companion a fork catchpoint needs for vfork, by its own number.
    QHash<QString, QString> m_catchpointCompanions;
    // What a reset took away from the debugger it replaced, waiting for the
    // session that follows to take breakpoints.
    QList<Breakpoint> m_restartedCatchpoints;
    // The watchpoints an adapter that announces no data breakpoints left to
    // the debugger behind it, as that one numbered them.
    QList<Breakpoint> m_consoleWatchpoints;
    int m_pendingConsoleWatchpoints = 0;
    // What the watchpoints the debugger was asked for behind the adapter's back
    // watch, as the debugger itself spells it.
    QHash<QString, QString> m_alienWatchpoints;
    // What a watchpoint last saw, by the number the debugger gave it.
    QHash<QString, QString> m_watchedValues;
    // Which file's answer a setBreakpoints reply is, routed by sequence number.
    QHash<int, Utils::FilePath> m_breakpointRequests;
    // The same for the function and the instruction breakpoints, which are one
    // array of their own each.
    QSet<int> m_functionBreakpointRequests;
    QSet<int> m_instructionBreakpointRequests;
    QSet<int> m_dataBreakpointRequests;
    // Which watchpoint a dataBreakpointInfo answer is about, by the request the
    // model is waiting for an answer to.
    QHash<int, quint64> m_dataIdRequests;
    // What the adapter called the breakpoints it made because it was asked to
    // here: it announces the ones made elsewhere the same way, and only the
    // name tells the two apart.
    QSet<QString> m_ownBreakpointIds;

    QList<Breakpoint> &breakpointArray(BreakpointArray kind, const Utils::FilePath &file);
    int sendBreakpointArray(BreakpointArray kind, const Utils::FilePath &file);
    void reportBreakpointsSet(const QJsonObject &response, BreakpointArray kind,
                              const Utils::FilePath &file);
    void stopForBreakpoints();
    void resendBreakpointsForTheStop();
    void resumeAfterBreakpointStop();
    // An array an adapter refused because the debuggee was running, with the
    // refusal itself: if the stop it wants cannot be had, that refusal is
    // still the answer the model gets.
    struct RefusedBreakpointArray {
        BreakpointArray kind = BreakpointArray::Source;
        Utils::FilePath file;
        QJsonObject refusal;
    };
    QList<RefusedBreakpointArray> m_breakpointsNeedingAStop;
    // The arrays sent again during such a stop, by sequence number: the
    // debuggee is let go once the last of them is answered.
    QSet<int> m_breakpointResends;
    bool m_resumeAfterBreakpointStop = false;
    void changeCatchpoint(const BreakpointChangeRequest &request);
    void insertCatchpointCompanion(const std::shared_ptr<QString> &owner, bool enabled);
    void resendCatchpoints();
    QStringList catchpointNumbers(const QString &number) const;
    void insertWatchpointOverConsole(const Breakpoint &breakpoint);
    void changeWatchpoint(const BreakpointChangeRequest &request);
    // Finds the request a change the adapter reports belongs to.
    const Breakpoint *breakpointForAdapterId(const QString &adapterId) const;
    // A breakpoint as the model reads it, out of what the adapter reported.
    // The number defaults to the one the adapter used, which is the only one
    // there is for a breakpoint made elsewhere.
    GdbMi breakpointMi(const QJsonObject &item, const QString &number = {}) const;

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
        // The container this level was fetched from, which is what a named
        // assignment goes through where the adapter takes no expression.
        int parentReference = 0;
        bool hasChildren = false;
        QStringList childINames;
    };
    quint64 m_localsRequestId = 0;
    int m_stringLengthLimit = 0;
    // The locals fetch, kept for RepeatLastCommand.
    std::optional<RefreshRequest> m_lastLocalsRequest;
    // A locals fetch that arrived while the debuggee was running, waiting for
    // the frame it is to be evaluated in.
    std::optional<RefreshRequest> m_deferredLocalsRequest;
    QSet<QString> m_expandedINames;
    QString m_partialVariable;
    QMap<QString, Local> m_locals;
    QStringList m_localRoots;
    QQueue<QPair<QString, int>> m_pendingVariables;
    // A watcher as the request named it: its iname and the expression to ask
    // the adapter to evaluate for it.
    QQueue<QPair<QString, QString>> m_pendingWatchers;
    // A pending variables request: the iname of the level it answers, and the
    // container it asks for.
    QHash<int, QPair<QString, int>> m_variableRequests;
    QHash<int, QPair<QString, QString>> m_watcherRequests;
    // A full backtrace is one stack per thread, and the protocol answers one
    // request at a time, so the walk keeps what it has and what is still to come.
    quint64 m_backtraceRequestId = 0;
    int m_backtraceThreadsSeq = -1;
    int m_backtraceFramesSeq = -1;
    QQueue<QPair<int, QString>> m_backtraceThreads;
    QString m_backtrace;
    QHash<int, quint64> m_threadRequests;
    // The thread list of the protocol carries no frame, so the one the threads
    // view shows per thread is asked for separately, and the list waits for all
    // of those answers.
    struct ThreadListRequest
    {
        quint64 requestId = 0;
        QList<GdbMi> threads;
        QHash<int, int> frameRequests;
    };
    std::optional<ThreadListRequest> m_threadList;
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
