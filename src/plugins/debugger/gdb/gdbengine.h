/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
#include <utils/qtcprocess.h>

#include <QProcess>
#include <QTextCodec>
#include <QTimer>

namespace Debugger {
namespace Internal {

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
    QString foundExecutableName; // empty if no corresponding exec could be found
    bool isCore = false;

    static CoreInfo readExecutableNameFromCore(const ProjectExplorer::Runnable &debugger,
                                               const QString &coreFile);
};

class GdbEngine final : public CppDebuggerEngine
{
    Q_OBJECT

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

    // This triggers the initial breakpoint synchronization and causes
    // finishInferiorSetup() being called once done.
    void handleInferiorPrepared();
    // This notifies the base of a successful inferior setup.
    void finishInferiorSetup();

    void handleDebugInfoLocation(const DebuggerResponse &response);

    // The engine is still running just fine, but it failed to acquire a debuggee.
    void notifyInferiorSetupFailedHelper(const QString &msg);

    void handleGdbFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleGdbError(QProcess::ProcessError error);
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

    using CommandsDoneCallback = void (GdbEngine::*)();
    // This function is called after all previous responses have been received.
    CommandsDoneCallback m_commandsDoneCallback = nullptr;

    bool m_rerunPending = false;

    ////////// Gdb Output, State & Capability Handling //////////

    Q_INVOKABLE void handleResponse(const QString &buff);
    void handleAsyncOutput(const QString &asyncClass, const GdbMi &result);
    void handleStopResponse(const GdbMi &data);
    void handleResultRecord(DebuggerResponse *response);
    void handleStop1(const GdbMi &data);
    void handleStop2(const GdbMi &data);
    void handleStop3();
    void resetCommandQueue();

    // Gdb initialization sequence
    void handleShowVersion(const DebuggerResponse &response);
    void handleListFeatures(const DebuggerResponse &response);
    void handlePythonSetup(const DebuggerResponse &response);

    int m_gdbVersion = 100;    // 7.6.1 is 70601
    int m_pythonVersion = 0; // 2.7.2 is 20702
    bool m_isQnxGdb = false;

    ////////// Inferior Management //////////

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const final;
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
    void executeRecordReverse(bool reverse);

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
    void handleInsertInterpreterBreakpoint(const DebuggerResponse &response, const Breakpoint &bp);
    void handleInterpreterBreakpointModified(const GdbMi &data);
    void handleWatchInsert(const DebuggerResponse &response, const Breakpoint &bp);
    void handleCatchInsert(const DebuggerResponse &response, const Breakpoint &bp);
    void handleBkpt(const GdbMi &bkpt, const Breakpoint &bp);
    QString breakpointLocation(const BreakpointParameters &data); // For gdb/MI.
    QString breakpointLocation2(const BreakpointParameters &data); // For gdb/CLI fallback.
    QString breakLocation(const QString &file) const;

    //
    // Modules specific stuff
    //
    void loadSymbols(const QString &moduleName) final;
    void loadAllSymbols() final;
    void loadSymbolsForStack() final;
    void requestModuleSymbols(const QString &moduleName) final;
    void requestModuleSections(const QString &moduleName) final;
    void reloadModules() final;
    void examineModules() final;

    void reloadModulesInternal();
    void handleModulesList(const DebuggerResponse &response);
    void handleShowModuleSections(const DebuggerResponse &response, const QString &moduleName);

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

    QString fullName(const QString &fileName);
    QString cleanupFullName(const QString &fileName);

    // awful hack to keep track of used files
    QMap<QString, QString> m_shortToFullName;
    QMap<QString, QString> m_fullToShortName;
    QMultiMap<QString, QString> m_baseNameToFullName;

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

    bool isPlainEngine() const;
    bool isCoreEngine() const;
    bool isRemoteEngine() const;
    bool isAttachEngine() const;
    bool isTermEngine() const;

    void setupEngine() final;
    void runEngine() final;
    void shutdownEngine() final;

    void interruptInferior2();
    QChar mixedDisasmFlag() const;

    // Plain
    void handleExecRun(const DebuggerResponse &response);
    void handleFileExecAndSymbols(const DebuggerResponse &response);

    // Attach
    void handleAttach(const DebuggerResponse &response);

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

    Utils::QtcProcess m_gdbProc;
    OutputCollector m_outputCollector;
    QString m_errorString;
};

} // namespace Internal
} // namespace Debugger
