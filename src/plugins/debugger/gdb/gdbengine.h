/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_GDBENGINE_H
#define DEBUGGER_GDBENGINE_H

#include <debugger/debuggerengine.h>

#include <debugger/breakhandler.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>
#include <debugger/debuggertooltipmanager.h>

#include <coreplugin/id.h>

#include <projectexplorer/devicesupport/idevice.h>

#include <QProcess>
#include <QTextCodec>
#include <QTime>
#include <QTimer>

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
    explicit GdbEngine(const DebuggerStartParameters &startParameters);
    ~GdbEngine();

private: ////////// General Interface //////////
    virtual DebuggerEngine *cppEngine() { return this; }

    virtual void setupEngine() = 0;
    virtual void handleGdbStartFailed();
    virtual void setupInferior() = 0;
    virtual void notifyInferiorSetupFailed();

    virtual bool hasCapability(unsigned) const;
    virtual void detachDebugger();
    virtual void shutdownInferior();
    virtual void shutdownEngine() = 0;
    virtual void abortDebugger();
    virtual void resetInferior();

    virtual bool acceptsDebuggerCommands() const;
    virtual void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

private: ////////// General State //////////

    DebuggerStartMode startMode() const;
    Q_SLOT void reloadLocals();

    bool m_registerNamesListed;

protected: ////////// Gdb Process Management //////////

    void startGdb(const QStringList &args = QStringList());
    void handleInferiorShutdown(const DebuggerResponse &response);
    void handleGdbExit(const DebuggerResponse &response);

    void loadInitScript();

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

private slots:
    friend class GdbPlainEngine;
    void handleInterruptDeviceInferior(const QString &error);
    void handleGdbFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleGdbError(QProcess::ProcessError error);
    void readDebugeeOutput(const QByteArray &data);
    void readGdbStandardOutput();
    void readGdbStandardError();

private:
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;

    QByteArray m_inbuffer;
    bool m_busy;

    // Name of the convenience variable containing the last
    // known function return value.
    QByteArray m_resultVarName;

private: ////////// Gdb Command Management //////////

    public: // Otherwise the Qt flag macros are unhappy.
    enum GdbCommandFlag {
        NoFlags = 0,
        // The command needs a stopped inferior.
        NeedsStop = 1,
        // No need to wait for the reply before continuing inferior.
        Discardable = 2,
        // We can live without receiving an answer.
        NonCriticalResponse = 8,
        // Callback expects ResultRunning instead of ResultDone.
        RunRequest = 16,
        // Callback expects ResultExit instead of ResultDone.
        ExitRequest = 32,
        // Auto-set inferior shutdown related states.
        LosesChild = 64,
        // Trigger breakpoint model rebuild when no such commands are pending anymore.
        RebuildBreakpointModel = 128,
        // This command needs to be send immediately.
        Immediate = 256,
        // This is a command that needs to be wrapped into -interpreter-exec console
        ConsoleCommand = 512
    };
    Q_DECLARE_FLAGS(GdbCommandFlags, GdbCommandFlag)

    // Type and cookie are sender-internal data, opaque for the "event
    // queue". resultNeeded == true increments m_pendingResults on
    // send and decrements on receipt, effectively preventing
    // watch model updates before everything is finished.
    void flushCommand(const DebuggerCommand &cmd);
protected:
    void runCommand(const DebuggerCommand &command);
    void postCommand(const QByteArray &command,
                     int flags = NoFlags,
                     DebuggerCommand::Callback callback = DebuggerCommand::Callback());
private:
    void flushQueuedCommands();
    Q_SLOT void commandTimeout();
    void setTokenBarrier();

    // Sets up an "unexpected result" for the following commeand.
    void scheduleTestResponse(int testCase, const QByteArray &response);

    QHash<int, DebuggerCommand> m_commandForToken;
    int commandTimeoutTime() const;
    QTimer m_commandTimer;

    QByteArray m_pendingConsoleStreamOutput;
    QByteArray m_pendingLogStreamOutput;

    // This contains the first token number for the current round
    // of evaluation. Responses with older tokens are considers
    // out of date and discarded.
    int m_oldestAcceptableToken;
    int m_nonDiscardableCount;

    int m_pendingBreakpointRequests; // Watch updating commands in flight

    typedef void (GdbEngine::*CommandsDoneCallback)();
    // This function is called after all previous responses have been received.
    CommandsDoneCallback m_commandsDoneCallback;

    QList<DebuggerCommand> m_commandsToRunOnTemporaryBreak;

private: ////////// Gdb Output, State & Capability Handling //////////
protected:
    Q_SLOT void handleResponse(const QByteArray &buff);
    void handleStopResponse(const GdbMi &data);
    void handleResultRecord(DebuggerResponse *response);
    void handleStop1(const GdbMi &data);
    void handleStop2(const GdbMi &data);
    Q_SLOT void handleStop2();
    StackFrame parseStackFrame(const GdbMi &mi, int level);
    void resetCommandQueue();

    bool isSynchronous() const { return true; }

    // Gdb initialization sequence
    void handleShowVersion(const DebuggerResponse &response);
    void handleListFeatures(const DebuggerResponse &response);
    void handlePythonSetup(const DebuggerResponse &response);

    int m_gdbVersion; // 7.6.1 is 70601
    bool m_isQnxGdb;

private: ////////// Inferior Management //////////

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const;
    bool acceptsBreakpoint(Breakpoint bp) const;
    void insertBreakpoint(Breakpoint bp);
    void removeBreakpoint(Breakpoint bp);
    void changeBreakpoint(Breakpoint bp);

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    protected:
    void continueInferiorInternal();
    void autoContinueInferior();
    void continueInferior();
    void interruptInferior();
    virtual void interruptInferior2() {}
    void interruptInferiorTemporarily();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);
    void executeReturn();

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

    void selectThread(ThreadId threadId);
    void activateFrame(int index);

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
    void handleWatchInsert(const DebuggerResponse &response, Breakpoint bp);
    void handleCatchInsert(const DebuggerResponse &response, Breakpoint bp);
    void handleBkpt(const GdbMi &bkpt, Breakpoint bp);
    void updateResponse(BreakpointResponse &response, const GdbMi &bkpt);
    QByteArray breakpointLocation(const BreakpointParameters &data); // For gdb/MI.
    QByteArray breakpointLocation2(const BreakpointParameters &data); // For gdb/CLI fallback.
    QString breakLocation(const QString &file) const;

    //
    // Modules specific stuff
    //
    protected:
    void loadSymbols(const QString &moduleName);
    Q_SLOT void loadAllSymbols();
    void loadSymbolsForStack();
    void requestModuleSymbols(const QString &moduleName);
    void requestModuleSections(const QString &moduleName);
    void reloadModules();
    void examineModules();

    void reloadModulesInternal();
    void handleModulesList(const DebuggerResponse &response);
    void handleShowModuleSections(const DebuggerResponse &response, const QString &moduleName);

    //
    // Snapshot specific stuff
    //
    virtual void createSnapshot();
    void handleMakeSnapshot(const DebuggerResponse &response, const QString &coreFile);

    //
    // Register specific stuff
    //
    Q_SLOT void reloadRegisters();
    void setRegisterValue(const QByteArray &name, const QString &value);
    void handleRegisterListNames(const DebuggerResponse &response);
    void handleRegisterListValues(const DebuggerResponse &response);
    void handleMaintPrintRegisters(const DebuggerResponse &response);
    QHash<int, QByteArray> m_registerNames; // Map GDB register numbers to indices

    //
    // Disassembler specific stuff
    //
    // Chain of fallbacks: PointMixed -> PointPlain -> RangeMixed -> RangePlain.
    void fetchDisassembler(DisassemblerAgent *agent);
    void fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac);
    bool handleCliDisassemblerResult(const QByteArray &response, DisassemblerAgent *agent);

    void handleBreakOnQFatal(const DebuggerResponse &response, bool continueSetup);

    //
    // Source file specific stuff
    //
    void reloadSourceFiles();
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
    void updateAll();
    void handleStackListFrames(const DebuggerResponse &response, bool isFull);
    void handleStackSelectThread(const DebuggerResponse &response);
    void handleThreadListIds(const DebuggerResponse &response);
    void handleThreadInfo(const DebuggerResponse &response);
    void handleThreadNames(const DebuggerResponse &response);
    DebuggerCommand stackCommand(int depth);
    Q_SLOT void reloadStack();
    Q_SLOT virtual void reloadFullStack();
    virtual void loadAdditionalQmlStack();
    void handleQmlStackFrameArguments(const DebuggerResponse &response);
    void handleQmlStackTrace(const DebuggerResponse &response);
    int currentFrame() const;

    QList<GdbMi> m_currentFunctionArgs;

    //
    // Watch specific stuff
    //
    virtual bool setToolTipExpression(const DebuggerToolTipContext &);
    virtual void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);

    virtual void fetchMemory(MemoryAgent *agent, QObject *token,
        quint64 addr, quint64 length);
    void fetchMemoryHelper(const MemoryAgentCookie &cookie);
    void handleChangeMemory(const DebuggerResponse &response);
    virtual void changeMemory(MemoryAgent *agent, QObject *token,
        quint64 addr, const QByteArray &data);
    void handleFetchMemory(const DebuggerResponse &response, MemoryAgentCookie ac);

    virtual void watchPoint(const QPoint &);
    void handleWatchPoint(const DebuggerResponse &response);

    void updateWatchItem(WatchItem *item);
    void showToolTip();

    void handleVarAssign(const DebuggerResponse &response);
    void handleDetach(const DebuggerResponse &response);
    void handleThreadGroupCreated(const GdbMi &result);
    void handleThreadGroupExited(const GdbMi &result);

    Q_SLOT void createFullBacktrace();
    void handleCreateFullBacktrace(const DebuggerResponse &response);

    void updateLocals();
        void doUpdateLocals(const UpdateParameters &parameters);
        void handleStackFramePython(const DebuggerResponse &response);

    void setLocals(const QList<GdbMi> &locals);

    //
    // Dumper Management
    //
    void reloadDebuggingHelpers();

    QString m_gdb;

    //
    // Convenience Functions
    //
    QString errorMessage(QProcess::ProcessError error);
    void showExecutionError(const QString &message);

    static QByteArray tooltipIName(const QString &exp);

    // For short-circuiting stack and thread list evaluation.
    bool m_stackNeeded;

    bool isQFatalBreakpoint(const BreakpointResponseId &id) const;
    bool isHiddenBreakpoint(const BreakpointResponseId &id) const;

    // HACK:
    QByteArray m_currentThread;
    QString m_lastWinException;
    QString m_lastMissingDebugInfo;
    BreakpointResponseId m_qFatalBreakpointResponseId;
    bool m_terminalTrap;

    bool usesExecInterrupt() const;

    QHash<int, QByteArray> m_scheduledTestResponses;
    QSet<int> m_testCases;

    // Debug information
    friend class DebugInfoTaskHandler;
    void requestDebugInformation(const DebugInfoTask &task);
    DebugInfoTaskHandler *m_debugInfoTaskHandler;

    // Indicates whether we had at least one full attempt to load
    // debug information.
    bool attemptQuickStart() const;
    bool m_fullStartDone;
    bool m_systemDumpersLoaded;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorSetupOk();
    static QString msgInferiorRunOk();
    static QString msgConnectRemoteServerFailed(const QString &why);
    static QByteArray dotEscape(QByteArray str);

    void debugLastCommand();
    DebuggerCommand m_lastDebuggableCommand;

protected:
    virtual void write(const QByteArray &data);

protected:
    bool prepareCommand();
    void interruptLocalInferior(qint64 pid);

    GdbProcess *m_gdbProc;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr m_signalOperation;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_OPERATORS_FOR_FLAGS(Debugger::Internal::GdbEngine::GdbCommandFlags)

#endif // DEBUGGER_GDBENGINE_H
