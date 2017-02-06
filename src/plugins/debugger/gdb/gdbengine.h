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
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggertooltipmanager.h>

#include <coreplugin/id.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/qtcprocess.h>

#include <QProcess>
#include <QTextCodec>
#include <QTime>
#include <QTimer>

#include <functional>

namespace Debugger {
namespace Internal {

class GdbProcess;
class DebugInfoTask;
class DebugInfoTaskHandler;
class DebuggerResponse;
class GdbMi;
class MemoryAgentCookie;
class BreakpointParameters;
class BreakpointResponse;

class DisassemblerAgentCookie;
class DisassemblerLines;

class GdbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit GdbEngine(const DebuggerRunParameters &runParameters);
    ~GdbEngine() override;

private: ////////// General Interface //////////
    DebuggerEngine *cppEngine() override { return this; }

    virtual void handleGdbStartFailed();
    void notifyInferiorSetupFailed() override;
    void prepareForRestart() override;

    bool hasCapability(unsigned) const override;
    void detachDebugger() override;
    void shutdownInferior() override;
    void abortDebugger() override;
    void resetInferior() override;

    bool acceptsDebuggerCommands() const override;
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;

private: ////////// General State //////////

    DebuggerStartMode startMode() const;
    void reloadLocals();

    bool m_registerNamesListed;

protected: ////////// Gdb Process Management //////////

    void startGdb(const QStringList &args = QStringList());
    void handleInferiorShutdown(const DebuggerResponse &response);
    void handleGdbExit(const DebuggerResponse &response);
    void setLinuxOsAbi();

    void loadInitScript();
    void setEnvironmentVariables();

    // Something went wrong with the adapter *before* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void handleAdapterStartFailed(const QString &msg,
        Core::Id settingsIdHint = Core::Id());

    // This triggers the initial breakpoint synchronization and causes
    // finishInferiorSetup() being called once done.
    void handleInferiorPrepared();
    // This notifies the base of a successful inferior setup.
    void finishInferiorSetup();

    void handleDebugInfoLocation(const DebuggerResponse &response);

    // The adapter is still running just fine, but it failed to acquire a debuggee.
    void notifyInferiorSetupFailed(const QString &msg);

    void notifyAdapterShutdownOk();
    void notifyAdapterShutdownFailed();

    // Something went wrong with the adapter *after* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void handleAdapterCrashed(const QString &msg);

private:
    friend class GdbPlainEngine;
    friend class GdbCoreEngine;
    void handleInterruptDeviceInferior(const QString &error);
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
    bool m_busy;

    // Name of the convenience variable containing the last
    // known function return value.
    QString m_resultVarName;

protected: ////////// Gdb Command Management //////////

    void runCommand(const DebuggerCommand &command) override;

private:
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
    int m_oldestAcceptableToken;
    int m_nonDiscardableCount;

    int m_pendingBreakpointRequests; // Watch updating commands in flight

    typedef void (GdbEngine::*CommandsDoneCallback)();
    // This function is called after all previous responses have been received.
    CommandsDoneCallback m_commandsDoneCallback;

    bool m_rerunPending;

private: ////////// Gdb Output, State & Capability Handling //////////
protected:
    Q_INVOKABLE void handleResponse(const QString &buff);
    void handleAsyncOutput(const QString &asyncClass, const GdbMi &result);
    void handleStopResponse(const GdbMi &data);
    void handleResultRecord(DebuggerResponse *response);
    void handleStop1(const GdbMi &data);
    void handleStop2(const GdbMi &data);
    void handleStop3();
    void resetCommandQueue();

    bool isSynchronous() const override { return true; }

    // Gdb initialization sequence
    void handleShowVersion(const DebuggerResponse &response);
    void handleListFeatures(const DebuggerResponse &response);
    void handlePythonSetup(const DebuggerResponse &response);

    int m_gdbVersion;    // 7.6.1 is 70601
    int m_pythonVersion; // 2.7.2 is 20702
    bool m_isQnxGdb;

private: ////////// Inferior Management //////////

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const override;
    bool acceptsBreakpoint(Breakpoint bp) const override;
    void insertBreakpoint(Breakpoint bp) override;
    void removeBreakpoint(Breakpoint bp) override;
    void changeBreakpoint(Breakpoint bp) override;

    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override;
    void executeNextI() override;

    protected:
    void continueInferiorInternal();
    void continueInferior() override;
    void interruptInferior() override;
    virtual void interruptInferior2() {}

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;
    void executeReturn() override;

    void handleExecuteContinue(const DebuggerResponse &response);
    void handleExecuteStep(const DebuggerResponse &response);
    void handleExecuteNext(const DebuggerResponse &response);
    void handleExecuteReturn(const DebuggerResponse &response);
    void handleExecuteJumpToLine(const DebuggerResponse &response);
    void handleExecuteRunToLine(const DebuggerResponse &response);

    void maybeHandleInferiorPidChanged(const QString &pid);
    void handleInfoProc(const DebuggerResponse &response);
    QString msgPtraceError(DebuggerStartMode sm);

private: ////////// View & Data Stuff //////////

    void selectThread(ThreadId threadId) override;
    void activateFrame(int index) override;
    void handleAutoContinueInferior();

    //
    // Breakpoint specific stuff
    //
    void handleBreakModifications(const GdbMi &bkpts);
    void handleBreakIgnore(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakDisable(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakEnable(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakInsert1(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakInsert2(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakCondition(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakThreadSpec(const DebuggerResponse &response, Breakpoint bp);
    void handleBreakLineNumber(const DebuggerResponse &response, Breakpoint bp);
    void handleInsertInterpreterBreakpoint(const DebuggerResponse &response, Breakpoint bp);
    void handleInterpreterBreakpointModified(const GdbMi &data);
    void handleWatchInsert(const DebuggerResponse &response, Breakpoint bp);
    void handleCatchInsert(const DebuggerResponse &response, Breakpoint bp);
    void handleBkpt(const GdbMi &bkpt, Breakpoint bp);
    void updateResponse(BreakpointResponse &response, const GdbMi &bkpt);
    QString breakpointLocation(const BreakpointParameters &data); // For gdb/MI.
    QString breakpointLocation2(const BreakpointParameters &data); // For gdb/CLI fallback.
    QString breakLocation(const QString &file) const;

    //
    // Modules specific stuff
    //
    protected:
    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void loadSymbolsForStack() override;
    void requestModuleSymbols(const QString &moduleName) override;
    void requestModuleSections(const QString &moduleName) override;
    void reloadModules() override;
    void examineModules() override;

    void reloadModulesInternal();
    void handleModulesList(const DebuggerResponse &response);
    void handleShowModuleSections(const DebuggerResponse &response, const QString &moduleName);

    //
    // Snapshot specific stuff
    //
    virtual void createSnapshot() override;
    void handleMakeSnapshot(const DebuggerResponse &response, const QString &coreFile);

    //
    // Register specific stuff
    //
    void reloadRegisters() override;
    void setRegisterValue(const QString &name, const QString &value) override;
    void handleRegisterListNames(const DebuggerResponse &response);
    void handleRegisterListing(const DebuggerResponse &response);
    void handleRegisterListValues(const DebuggerResponse &response);
    void handleMaintPrintRegisters(const DebuggerResponse &response);
    QHash<int, Register> m_registers; // Map GDB register numbers to indices

    //
    // Disassembler specific stuff
    //
    // Chain of fallbacks: PointMixed -> PointPlain -> RangeMixed -> RangePlain.
    void fetchDisassembler(DisassemblerAgent *agent) override;
    void fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac);
    bool handleCliDisassemblerResult(const QString &response, DisassemblerAgent *agent);

    //
    // Source file specific stuff
    //
    void reloadSourceFiles() override;
    void reloadSourceFilesInternal();
    void handleQuerySources(const DebuggerResponse &response);

    QString fullName(const QString &fileName);
    QString cleanupFullName(const QString &fileName);

    // awful hack to keep track of used files
    QMap<QString, QString> m_shortToFullName;
    QMap<QString, QString> m_fullToShortName;
    QMultiMap<QString, QString> m_baseNameToFullName;

    bool m_sourcesListUpdating;

    //
    // Stack specific stuff
    //
protected:
    void updateAll() override;
    void handleStackListFrames(const DebuggerResponse &response, bool isFull);
    void handleStackSelectThread(const DebuggerResponse &response);
    void handleThreadListIds(const DebuggerResponse &response);
    void handleThreadInfo(const DebuggerResponse &response);
    void handleThreadNames(const DebuggerResponse &response);
    DebuggerCommand stackCommand(int depth);
    void reloadStack();
    void reloadFullStack() override;
    void loadAdditionalQmlStack() override;
    int currentFrame() const;

    //
    // Watch specific stuff
    //
    virtual void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) override;

    void fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length) override;
    void fetchMemoryHelper(const MemoryAgentCookie &cookie);
    void handleChangeMemory(const DebuggerResponse &response);
    void changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data) override;
    void handleFetchMemory(const DebuggerResponse &response, MemoryAgentCookie ac);

    void showToolTip();

    void handleVarAssign(const DebuggerResponse &response);
    void handleThreadGroupCreated(const GdbMi &result);
    void handleThreadGroupExited(const GdbMi &result);

    void createFullBacktrace();

    void doUpdateLocals(const UpdateParameters &parameters) override;
    void handleFetchVariables(const DebuggerResponse &response);

    void setLocals(const QList<GdbMi> &locals);

    //
    // Dumper Management
    //
    void reloadDebuggingHelpers() override;

    //
    // Convenience Functions
    //
    QString errorMessage(QProcess::ProcessError error);
    void showExecutionError(const QString &message);
    QString failedToStartMessage();

    static QString tooltipIName(const QString &exp);

    // For short-circuiting stack and thread list evaluation.
    bool m_stackNeeded;

    // For suppressing processing *stopped and *running responses
    // while updating locals.
    bool m_inUpdateLocals;

    // HACK:
    QString m_currentThread;
    QString m_lastWinException;
    QString m_lastMissingDebugInfo;
    bool m_terminalTrap;
    bool usesExecInterrupt() const;
    bool usesTargetAsync() const;

    DebuggerCommandSequence m_onStop;

    QHash<int, QString> m_scheduledTestResponses;
    QSet<int> m_testCases;

    // Debug information
    friend class DebugInfoTaskHandler;
    void requestDebugInformation(const DebugInfoTask &task);
    DebugInfoTaskHandler *m_debugInfoTaskHandler;

    bool m_systemDumpersLoaded;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorSetupOk();
    static QString msgInferiorRunOk();
    static QString msgConnectRemoteServerFailed(const QString &why);

    void debugLastCommand() override;
    DebuggerCommand m_lastDebuggableCommand;

protected:
    bool prepareCommand();
    void interruptLocalInferior(qint64 pid);

protected:
    Utils::QtcProcess m_gdbProc;
    QString m_errorString;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr m_signalOperation;
};

} // namespace Internal
} // namespace Debugger
