/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_GDBENGINE_H
#define DEBUGGER_GDBENGINE_H

#include <debugger/debuggerengine.h>

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
class GdbResponse;
class GdbMi;
class MemoryAgentCookie;

class WatchData;
class DisassemblerAgentCookie;
class DisassemblerLines;

class GdbEngine : public Debugger::DebuggerEngine
{
    Q_OBJECT

public:
    explicit GdbEngine(const DebuggerStartParameters &startParameters);
    ~GdbEngine();

private: ////////// General Interface //////////

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
    virtual QByteArray qtNamespace() const { return m_qtNamespace; }
    virtual void setQtNamespace(const QByteArray &ns) { m_qtNamespace = ns; }

private: ////////// General State //////////

    DebuggerStartMode startMode() const;
    Q_SLOT void reloadLocals();

    bool m_registerNamesListed;

protected: ////////// Gdb Process Management //////////

    void startGdb(const QStringList &args = QStringList());
    void handleInferiorShutdown(const GdbResponse &response);
    void handleGdbExit(const GdbResponse &response);

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

    void handleDebugInfoLocation(const GdbResponse &response);

    // The adapter is still running just fine, but it failed to acquire a debuggee.
    void notifyInferiorSetupFailed(const QString &msg);

    void notifyAdapterShutdownOk();
    void notifyAdapterShutdownFailed();

    // Something went wrong with the adapter *after* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void handleAdapterCrashed(const QString &msg);

private slots:
    void handleInterruptDeviceInferior(const QString &error);
    void handleGdbFinished(int, QProcess::ExitStatus status);
    void handleGdbError(QProcess::ProcessError error);
    void readGdbStandardOutput();
    void readGdbStandardError();
    void readDebugeeOutput(const QByteArray &data);

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
        // Callback expects GdbResultRunning instead of GdbResultDone.
        RunRequest = 16,
        // Callback expects GdbResultExit instead of GdbResultDone.
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

    protected:
    typedef void (GdbEngine::*GdbCommandCallback)(const GdbResponse &response);

    struct GdbCommand
    {
        GdbCommand()
            : flags(0), callback(0), callbackName(0)
        {}

        int flags;
        GdbCommandCallback callback;
        const char *callbackName;
        QByteArray command;
        QVariant cookie;
        QTime postTime;
    };

    // Type and cookie are sender-internal data, opaque for the "event
    // queue". resultNeeded == true increments m_pendingResults on
    // send and decrements on receipt, effectively preventing
    // watch model updates before everything is finished.
    void flushCommand(const GdbCommand &cmd);
protected:
    void postCommand(const QByteArray &command,
                     GdbCommandFlags flags,
                     GdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());
    void postCommand(const QByteArray &command,
                     GdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());
private:
    void postCommandHelper(const GdbCommand &cmd);
    void flushQueuedCommands();
    Q_SLOT void commandTimeout();
    void setTokenBarrier();

    // Sets up an "unexpected result" for the following commeand.
    void scheduleTestResponse(int testCase, const QByteArray &response);

    QHash<int, GdbCommand> m_cookieForToken;
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

    QList<GdbCommand> m_commandsToRunOnTemporaryBreak;

private: ////////// Gdb Output, State & Capability Handling //////////
protected:
    Q_SLOT void handleResponse(const QByteArray &buff);
    void handleStopResponse(const GdbMi &data);
    void handleResultRecord(GdbResponse *response);
    void handleStop1(const GdbResponse &response);
    void handleStop1(const GdbMi &data);
    void handleStop2(const GdbResponse &response);
    void handleStop2(const GdbMi &data);
    Q_SLOT void handleStop2();
    StackFrame parseStackFrame(const GdbMi &mi, int level);
    void resetCommandQueue();

    bool isSynchronous() const { return true; }

    // Gdb initialization sequence
    void handleShowVersion(const GdbResponse &response);
    void handleListFeatures(const GdbResponse &response);
    void handlePythonSetup(const GdbResponse &response);

    int m_gdbVersion; // 7.6.1 is 70601
    bool m_isQnxGdb;

private: ////////// Inferior Management //////////

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const;
    bool acceptsBreakpoint(BreakpointModelId id) const;
    void insertBreakpoint(BreakpointModelId id);
    void removeBreakpoint(BreakpointModelId id);
    void changeBreakpoint(BreakpointModelId id);

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

    void handleExecuteContinue(const GdbResponse &response);
    void handleExecuteStep(const GdbResponse &response);
    void handleExecuteNext(const GdbResponse &response);
    void handleExecuteReturn(const GdbResponse &response);
    void handleExecuteJumpToLine(const GdbResponse &response);
    void handleExecuteRunToLine(const GdbResponse &response);

    void maybeHandleInferiorPidChanged(const QString &pid);
    void handleInfoProc(const GdbResponse &response);
    QString msgPtraceError(DebuggerStartMode sm);

private: ////////// View & Data Stuff //////////

    void selectThread(ThreadId threadId);
    void activateFrame(int index);
    void resetLocation();

    //
    // Breakpoint specific stuff
    //
    void handleBreakModifications(const GdbMi &bkpts);
    void handleBreakIgnore(const GdbResponse &response);
    void handleBreakDisable(const GdbResponse &response);
    void handleBreakEnable(const GdbResponse &response);
    void handleBreakInsert1(const GdbResponse &response);
    void handleBreakInsert2(const GdbResponse &response);
    void handleTraceInsert2(const GdbResponse &response);
    void handleBreakCondition(const GdbResponse &response);
    void handleBreakThreadSpec(const GdbResponse &response);
    void handleBreakLineNumber(const GdbResponse &response);
    void handleWatchInsert(const GdbResponse &response);
    void handleCatchInsert(const GdbResponse &response);
    void handleBkpt(const GdbMi &bkpt, const BreakpointModelId &id);
    void updateResponse(BreakpointResponse &response, const GdbMi &bkpt);
    QByteArray breakpointLocation(BreakpointModelId id); // For gdb/MI.
    QByteArray breakpointLocation2(BreakpointModelId id); // For gdb/CLI fallback.
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
    void handleModulesList(const GdbResponse &response);
    void handleShowModuleSymbols(const GdbResponse &response);
    void handleShowModuleSections(const GdbResponse &response);

    //
    // Snapshot specific stuff
    //
    virtual void createSnapshot();
    void handleMakeSnapshot(const GdbResponse &response);

    //
    // Register specific stuff
    //
    Q_SLOT void reloadRegisters();
    void setRegisterValue(int nr, const QString &value);
    void handleRegisterListNames(const GdbResponse &response);
    void handleRegisterListValues(const GdbResponse &response);
    QVector<int> m_registerNumbers; // Map GDB register numbers to indices

    //
    // Disassembler specific stuff
    //
    // Chain of fallbacks: PointMixed -> PointPlain -> RangeMixed -> RangePlain.
    void fetchDisassembler(DisassemblerAgent *agent);
    void fetchDisassemblerByCliPointMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangeMixed(const DisassemblerAgentCookie &ac);
    void fetchDisassemblerByCliRangePlain(const DisassemblerAgentCookie &ac);
    void handleFetchDisassemblerByCliPointMixed(const GdbResponse &response);
    void handleFetchDisassemblerByCliRangeMixed(const GdbResponse &response);
    void handleFetchDisassemblerByCliRangePlain(const GdbResponse &response);
    bool handleCliDisassemblerResult(const QByteArray &response, DisassemblerAgent *agent);

    void handleBreakOnQFatal(const GdbResponse &response);

    //
    // Source file specific stuff
    //
    void reloadSourceFiles();
    void reloadSourceFilesInternal();
    void handleQuerySources(const GdbResponse &response);

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
    void handleStackListFrames(const GdbResponse &response);
    void handleStackSelectThread(const GdbResponse &response);
    void handleStackSelectFrame(const GdbResponse &response);
    void handleThreadListIds(const GdbResponse &response);
    void handleThreadInfo(const GdbResponse &response);
    void handleThreadNames(const GdbResponse &response);
    Q_SLOT void reloadStack(bool forceGotoLocation);
    Q_SLOT virtual void reloadFullStack();
    virtual void loadAdditionalQmlStack();
    void handleQmlStackFrameArguments(const GdbResponse &response);
    void handleQmlStackTrace(const GdbResponse &response);
    int currentFrame() const;

    QList<GdbMi> m_currentFunctionArgs;

    //
    // Watch specific stuff
    //
    virtual bool setToolTipExpression(TextEditor::BaseTextEditor *editor,
        const DebuggerToolTipContext &);
    virtual void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);

    virtual void fetchMemory(MemoryAgent *agent, QObject *token,
        quint64 addr, quint64 length);
    void fetchMemoryHelper(const MemoryAgentCookie &cookie);
    void handleChangeMemory(const GdbResponse &response);
    virtual void changeMemory(MemoryAgent *agent, QObject *token,
        quint64 addr, const QByteArray &data);
    void handleFetchMemory(const GdbResponse &response);

    virtual void watchPoint(const QPoint &);
    void handleWatchPoint(const GdbResponse &response);

    void updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);
    void rebuildWatchModel();
    void showToolTip();

    void insertData(const WatchData &data);

    void handleVarAssign(const GdbResponse &response);
    void handleDetach(const GdbResponse &response);
    void handleThreadGroupCreated(const GdbMi &result);
    void handleThreadGroupExited(const GdbMi &result);

    Q_SLOT void createFullBacktrace();
    void handleCreateFullBacktrace(const GdbResponse &response);

    void updateLocals();
        void updateLocalsPython(const UpdateParameters &parameters);
            void handleStackFramePython(const GdbResponse &response);

    void setLocals(const QList<GdbMi> &locals);

    QSet<QByteArray> m_processedNames;
    struct TypeInfo
    {
        TypeInfo(uint s = 0) : size(s) {}

        uint size;
    };

    QHash<QByteArray, TypeInfo> m_typeInfoCache;

    //
    // Dumper Management
    //
    void reloadDebuggingHelpers();

    QByteArray m_qtNamespace;
    QString m_gdb;

    //
    // Convenience Functions
    //
    QString errorMessage(QProcess::ProcessError error);
    void showExecutionError(const QString &message);

    static QByteArray tooltipIName(const QString &exp);
    DebuggerToolTipContext m_toolTipContext;

    // For short-circuiting stack and thread list evaluation.
    bool m_stackNeeded;

    //
    // Qml
    //
    BreakpointResponseId m_qmlBreakpointResponseId1;
    BreakpointResponseId m_qmlBreakpointResponseId2;
    bool m_preparedForQmlBreak;
    bool setupQmlStep(bool on);
    void handleSetQmlStepBreakpoint(const GdbResponse &response);
    bool isQmlStepBreakpoint(const BreakpointResponseId &id) const;
    bool isQmlStepBreakpoint1(const BreakpointResponseId &id) const;
    bool isQmlStepBreakpoint2(const BreakpointResponseId &id) const;
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

    // Test
    QList<WatchData> m_completed;
    QSet<QByteArray> m_uncompleted;

    static QString msgGdbStopFailed(const QString &why);
    static QString msgInferiorStopFailed(const QString &why);
    static QString msgAttachedToStoppedInferior();
    static QString msgInferiorSetupOk();
    static QString msgInferiorRunOk();
    static QString msgConnectRemoteServerFailed(const QString &why);
    static QByteArray dotEscape(QByteArray str);

    void debugLastCommand();
    QByteArray m_lastDebuggableCommand;

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
