// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <debugger/breakhandler.h>
#include <debugger/registerhandler.h>
#include <debugger/peripheralregisterhandler.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/outputcollector.h>

#include <utils/id.h>
#include <utils/process.h>

#include <QProcess>
#include <QTextCodec>
#include <QTimer>

namespace Debugger::Internal {

class BreakpointParameters;
class DebugInfoTask;
class DebugInfoTaskHandler;
class DebuggerResponse;
class DisassemblerAgentCookie;
class GdbMi;
class MemoryAgentCookie;

struct CoreInfo
{
    QString rawStringFromCore;
    Utils::FilePath foundExecutableName; // empty if no corresponding exec could be found
    bool isCore = false;

    static CoreInfo readExecutableNameFromCore(const Utils::ProcessRunData &debugger,
                                               const Utils::FilePath &coreFile);
};

class GdbEngine final : public CppDebuggerEngine
{
public:
    GdbEngine();
    ~GdbEngine() final;

private: ////////// General Interface //////////
    void handleGdbStartFailed();
    void prepareForRestart() final;

    bool hasCapability(unsigned) const final;
    void detachDebugger() final;
    void shutdownInferior() final;
    void abortDebuggerProcess() final;
    void resetInferior() final;

    bool acceptsDebuggerCommands() const final;
    void executeDebuggerCommand(const QString &command) final;

    ////////// General State //////////

    bool m_registerNamesListed = false;

    ////////// Gdb Process Management //////////

    void handleInferiorShutdown(const DebuggerResponse &response);
    void handleGdbExit(const DebuggerResponse &response);
    void setLinuxOsAbi();

    void loadInitScript();
    void setEnvironmentVariables();

    // Something went wrong with the adapter *before* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void handleAdapterStartFailed(const QString &msg,
        Utils::Id settingsIdHint = Utils::Id());

    // Called after target setup.
    void handleInferiorPrepared();

    void handleDebugInfoLocation(const DebuggerResponse &response);

    // The engine is still running just fine, but it failed to acquire a debuggee.
    void notifyInferiorSetupFailedHelper(const QString &msg);

    void handleGdbStarted();
    void handleGdbDone();
    void readGdbStandardOutput();
    void readGdbStandardError();
    void readDebuggeeOutput(const QByteArray &ba);

    QTextCodec *m_gdbOutputCodec;
    QTextCodec::ConverterState m_gdbOutputCodecState;
    QTextCodec *m_inferiorOutputCodec;
    QTextCodec::ConverterState m_inferiorOutputCodecState;

    QByteArray m_inbuffer;
    bool m_busy = false;

    // Name of the convenience variable containing the last
    // known function return value.
    QString m_resultVarName;

    ////////// Gdb Command Management //////////

    void runCommand(const DebuggerCommand &command) final;

    void commandTimeout();
    void setTokenBarrier();

    // Sets up an "unexpected result" for the following commeand.
    void scheduleTestResponse(int testCase, const QString &response);

    QHash<int, DebuggerCommand> m_commandForToken;
    QHash<int, int> m_flagsForToken;
    int commandTimeoutTime() const;
    QTimer m_commandTimer;

    QString m_pendingConsoleStreamOutput;
    QString m_pendingLogStreamOutput;

    // This contains the first token number for the current round
    // of evaluation. Responses with older tokens are considers
    // out of date and discarded.
    int m_oldestAcceptableToken = -1;
    int m_nonDiscardableCount = 0;

    bool m_rerunPending = false;
    bool m_ignoreNextTrap = false;
    bool m_detectTargetIncompat = false;

    ////////// Gdb Output, State & Capability Handling //////////

    Q_INVOKABLE void handleResponse(const QString &buff);
    void handleAsyncOutput(const QStringView asyncClass, const GdbMi &result);
    void handleStopResponse(const GdbMi &data);
    void handleResultRecord(DebuggerResponse *response);
    void handleStop1(const GdbMi &data);
    void handleStop2(const GdbMi &data);
    void handleStop3();
    void resetCommandQueue();

    // Gdb initialization sequence
    void handleShowVersion(const DebuggerResponse &response);
    void handlePythonSetup(const DebuggerResponse &response);

    int m_gdbVersion = 100;    // 7.6.1 is 70601
    int m_pythonVersion = 0; // 2.7.2 is 20702
    bool m_isQnxGdb = false;

    ////////// Inferior Management //////////

    // This should be always the last call in a function.
    bool acceptsBreakpoint(const BreakpointParameters &bp) const final;
    void insertBreakpoint(const Breakpoint &bp) final;
    void removeBreakpoint(const Breakpoint &bp) final;
    void updateBreakpoint(const Breakpoint &bp) final;
    void enableSubBreakpoint(const SubBreakpoint &sbp, bool on) final;

    void executeStepIn(bool byInstruction) final;
    void executeStepOut() final;
    void executeStepOver(bool byInstruction) final;

    void continueInferiorInternal();
    void continueInferior() final;
    void interruptInferior() final;

    void executeRunToLine(const ContextData &data) final;
    void executeRunToFunction(const QString &functionName) final;
    void executeJumpToLine(const ContextData &data) final;
    void executeReturn() final;
    void executeRecordReverse(bool reverse) final;

    void handleExecuteContinue(const DebuggerResponse &response);
    void handleExecuteStep(const DebuggerResponse &response);
    void handleExecuteNext(const DebuggerResponse &response);
    void handleExecuteReturn(const DebuggerResponse &response);
    void handleExecuteJumpToLine(const DebuggerResponse &response);
    void handleExecuteRunToLine(const DebuggerResponse &response);

    QString msgPtraceError(DebuggerStartMode sm);

    ////////// View & Data Stuff //////////

    void selectThread(const Thread &thread) final;
    void activateFrame(int index) final;

    //
    // Breakpoint specific stuff
    //
    void handleBreakModifications(const GdbMi &bkpts);
    void handleBreakIgnore(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakDisable(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakEnable(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakInsert1(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakInsert2(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakCondition(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakThreadSpec(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBreakLineNumber(const DebuggerResponse &response, const Breakpoint &bp);
    void handleTracepointInsert(const DebuggerResponse &response, const Breakpoint &bp);
    void handleTracepointHit(const GdbMi &data);
    void handleTracepointModified(const GdbMi &data);
    void handleInsertInterpreterBreakpoint(const DebuggerResponse &response, const Breakpoint &bp);
    void handleInterpreterBreakpointModified(const GdbMi &data);
    void handleWatchInsert(const DebuggerResponse &response, const Breakpoint &bp);
    void handleCatchInsert(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBkpt(const GdbMi &bkpt, const Breakpoint &bp);
    QString breakpointLocation(const BreakpointParameters &data); // For gdb/MI.
    QString breakpointLocation2(const BreakpointParameters &data); // For gdb/CLI fallback.
    QString breakLocation(const Utils::FilePath &file) const;
    void updateTracepointCaptures(const Breakpoint &bp);

    //
    // Modules specific stuff
    //
    void loadSymbols(const Utils::FilePath &moduleName) final;
    void loadAllSymbols() final;
    void loadSymbolsForStack() final;
    void requestModuleSymbols(const Utils::FilePath &moduleName) final;
    void requestModuleSections(const Utils::FilePath &moduleName) final;
    void reloadModules() final;
    void examineModules() final;

    void reloadModulesInternal();
    void handleModulesList(const DebuggerResponse &response);
    void handleShowModuleSections(const DebuggerResponse &response, const Utils::FilePath &moduleName);

    //
    // Snapshot specific stuff
    //
    void createSnapshot() final;
    void handleMakeSnapshot(const DebuggerResponse &response, const QString &coreFile);

    //
    // Register specific stuff
    //
    void reloadRegisters() final;
    void reloadPeripheralRegisters() final;
    void setRegisterValue(const QString &name, const QString &value) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void handleRegisterListNames(const DebuggerResponse &response);
    void handleRegisterListing(const DebuggerResponse &response);
    void handleRegisterListValues(const DebuggerResponse &response);
    void handlePeripheralRegisterListValues(const DebuggerResponse &response);
    void handleMaintPrintRegisters(const DebuggerResponse &response);
    QHash<int, Register> m_registers; // Map GDB register numbers to indices

    //
    // Disassembler specific stuff
    //
    // Chain of fallbacks: PointMixed -> PointPlain -> RangeMixed -> RangePlain.
    void fetchDisassembler(DisassemblerAgent *agent) final;
    void fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac);
    bool handleCliDisassemblerResult(const QString &response, DisassemblerAgent *agent);

    //
    // Source file specific stuff
    //
    void reloadSourceFiles() final;
    void reloadSourceFilesInternal();
    void handleQuerySources(const DebuggerResponse &response);

    Utils::FilePath fullName(const QString &fileName);
    Utils::FilePath cleanupFullName(const QString &fileName);

    // awful hack to keep track of used files
    QMap<QString, Utils::FilePath> m_shortToFullName;
    QMap<Utils::FilePath, QString> m_fullToShortName;
    QMultiMap<QString, Utils::FilePath> m_baseNameToFullName;

    bool m_sourcesListUpdating = false;

    //
    // Stack specific stuff
    //
    void updateAll() final;
    void handleStackListFrames(const DebuggerResponse &response, bool isFull);
    void handleStackSelectThread(const DebuggerResponse &response);
    void handleThreadListIds(const DebuggerResponse &response);
    void handleThreadInfo(const DebuggerResponse &response);
    void handleThreadNames(const DebuggerResponse &response);
    DebuggerCommand stackCommand(int depth);
    void reloadStack();
    void reloadFullStack() final;
    void loadAdditionalQmlStack() final;
    int currentFrame() const;

    //
    // Watch specific stuff
    //
    void reloadLocals();
    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) final;

    void fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length) final;
    void fetchMemoryHelper(const MemoryAgentCookie &cookie);
    void handleChangeMemory(const DebuggerResponse &response);
    void changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data) final;
    void handleFetchMemory(const DebuggerResponse &response, MemoryAgentCookie ac);

    void showToolTip();

    void handleVarAssign(const DebuggerResponse &response);
    void handleThreadGroupCreated(const GdbMi &result);
    void handleThreadGroupExited(const GdbMi &result);

    void createFullBacktrace();

    void doUpdateLocals(const UpdateParameters &parameters) final;
    void handleFetchVariables(const DebuggerResponse &response);

    void setLocals(const QList<GdbMi> &locals);

    //
    // Dumper Management
    //
    void reloadDebuggingHelpers() final;

    //
    // Convenience Functions
    //
    void showExecutionError(const QString &message);
    QString failedToStartMessage();

    // For short-circuiting stack and thread list evaluation.
    bool m_stackNeeded = false;

    // For suppressing processing *stopped and *running responses
    // while updating locals.
    bool m_inUpdateLocals = false;

    // HACK:
    QString m_currentThread;
    QString m_lastWinException;
    bool m_expectTerminalTrap = false;
    bool usesExecInterrupt() const;
    bool usesTargetAsync() const;

    DebuggerCommandSequence m_onStop;

    QHash<int, QString> m_scheduledTestResponses;
    QSet<int> m_testCases;

    bool m_systemDumpersLoaded = false;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorSetupOk();
    static QString msgInferiorRunOk();
    static QString msgConnectRemoteServerFailed(const QString &why);

    void debugLastCommand() final;
    DebuggerCommand m_lastDebuggableCommand;

    bool isLocalRunEngine() const;
    bool isPlainEngine() const;
    bool isCoreEngine() const;
    bool isRemoteEngine() const;
    bool isLocalAttachEngine() const;
    bool isTermEngine() const;

    void setupEngine() final;
    void runEngine();
    void shutdownEngine() final;

    void interruptInferior2();
    QChar mixedDisasmFlag() const;

    // Plain
    void handleExecRun(const DebuggerResponse &response);
    void handleFileExecAndSymbols(const DebuggerResponse &response);

    // Attach
    void handleLocalAttach(const DebuggerResponse &response);
    void handleRemoteAttach(const DebuggerResponse &response);

    // Remote
    void callTargetRemote();
    void handleSetTargetAsync(const DebuggerResponse &response);
    void handleTargetRemote(const DebuggerResponse &response);
    void handleTargetExtendedRemote(const DebuggerResponse &response);
    void handleTargetExtendedAttach(const DebuggerResponse &response);
    void handleTargetQnx(const DebuggerResponse &response);
    void handleSetNtoExecutable(const DebuggerResponse &response);
    void handleInterruptInferior(const DebuggerResponse &response);
    void interruptLocalInferior(qint64 pid);

    // Terminal
    void handleStubAttached(const DebuggerResponse &response, qint64 mainThreadId);

    // Core
    void handleTargetCore(const DebuggerResponse &response);
    void handleCoreRoundTrip(const DebuggerResponse &response);
    QString coreFileName() const;

    QString mainFunction() const;
    void setupInferior();
    void claimInitialBreakpoints();

    bool usesOutputCollector() const;

    Utils::Process m_gdbProc;
    OutputCollector m_outputCollector;
    QString m_errorString;
};

} // Debugger::Internal
