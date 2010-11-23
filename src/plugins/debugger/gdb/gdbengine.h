/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_GDBENGINE_H
#define DEBUGGER_GDBENGINE_H

#include "debuggerengine.h"

#include "stackframe.h"
#include "watchutils.h"

#include <QtCore/QByteArray>
#include <QtCore/QProcess>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QMultiMap>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QTextCodec>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QVariant>


namespace Debugger {
namespace Internal {

class AbstractGdbAdapter;
class AbstractGdbProcess;
class GdbResponse;
class GdbMi;

class WatchData;
class DisassemblerAgentCookie;

class AttachGdbAdapter;
class CoreGdbAdapter;
class LocalPlainGdbAdapter;
class RemoteGdbServerAdapter;
class TrkGdbAdapter;

enum DebuggingHelperState
{
    DebuggingHelperUninitialized,
    DebuggingHelperLoadTried,
    DebuggingHelperAvailable,
    DebuggingHelperUnavailable,
};


class GdbEngine : public Debugger::DebuggerEngine
{
    Q_OBJECT

public:
    explicit GdbEngine(const DebuggerStartParameters &startParameters);
    ~GdbEngine();
    AbstractGdbAdapter *gdbAdapter() const { return m_gdbAdapter; }

private:
    friend class AbstractGdbAdapter;
    friend class AbstractPlainGdbAdapter;
    friend class AttachGdbAdapter;
    friend class CoreGdbAdapter;
    friend class LocalPlainGdbAdapter;
    friend class TermGdbAdapter;
    friend class RemoteGdbServerAdapter;
    friend class RemotePlainGdbAdapter;
    friend class TrkGdbAdapter;
    friend class TcfTrkGdbAdapter;

private: ////////// General Interface //////////

    virtual void setupEngine();
    virtual void setupInferior();
    virtual void runEngine();

    virtual unsigned debuggerCapabilities() const;
    virtual void detachDebugger();
    virtual void shutdownEngine();
    virtual void shutdownInferior();
    virtual void notifyInferiorSetupFailed();

    virtual void executeDebuggerCommand(const QString &command);
    virtual QByteArray qtNamespace() const { return m_dumperHelper.qtNamespace(); }

private: ////////// General State //////////

    DebuggerStartMode startMode() const;
    Q_SLOT void reloadLocals();

    bool m_registerNamesListed;

private: ////////// Gdb Process Management //////////

    AbstractGdbAdapter *createAdapter();
    bool startGdb(const QStringList &args = QStringList(),
                  const QString &gdb = QString(),
                  const QString &settingsIdHint = QString());
    void handleInferiorShutdown(const GdbResponse &response);
    void handleGdbExit(const GdbResponse &response);

    void handleAdapterStarted();
    void defaultInferiorShutdown(const char *cmd);
    void loadPythonDumpers();
    void pythonDumpersFailed();

    // Something went wrong with the adapter *before* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void handleAdapterStartFailed(const QString &msg,
        const QString &settingsIdHint = QString());

    // This triggers the initial breakpoint synchronization and causes
    // finishInferiorSetup() being called once done.
    void handleInferiorPrepared();
    // This notifies the base of a successful inferior setup.
    void finishInferiorSetup();

    // The adapter is still running just fine, but it failed to acquire a debuggee.
    void notifyInferiorSetupFailed(const QString &msg);

    void notifyAdapterShutdownOk();
    void notifyAdapterShutdownFailed();

    // Something went wrong with the adapter *after* adapterStarted() was emitted.
    // Make sure to clean up everything before emitting this signal.
    void handleAdapterCrashed(const QString &msg);

private slots:
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

    AbstractGdbAdapter *m_gdbAdapter;

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
        // Trigger watch model rebuild when no such commands are pending anymore.
        RebuildWatchModel = 4,
        WatchUpdate = Discardable | RebuildWatchModel,
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
        ConsoleCommand = 512,
    };
    Q_DECLARE_FLAGS(GdbCommandFlags, GdbCommandFlag)
    private:

    typedef void (GdbEngine::*GdbCommandCallback)
        (const GdbResponse &response);
    typedef void (AbstractGdbAdapter::*AdapterCallback)
        (const GdbResponse &response);

    struct GdbCommand
    {
        GdbCommand()
            : flags(0), callback(0), adapterCallback(0), callbackName(0)
        {}

        int flags;
        GdbCommandCallback callback;
        AdapterCallback adapterCallback;
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
    void postCommand(const QByteArray &command,
                     GdbCommandFlags flags,
                     GdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());
    void postCommand(const QByteArray &command,
                     GdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());
    void postCommand(const QByteArray &command,
                     AdapterCallback callback,
                     const char *callbackName,
                     const QVariant &cookie = QVariant());
    void postCommand(const QByteArray &command,
                     GdbCommandFlags flags,
                     AdapterCallback callback,
                     const char *callbackName,
                     const QVariant &cookie = QVariant());
    void postCommandHelper(const GdbCommand &cmd);
    void flushQueuedCommands();
    Q_SLOT void commandTimeout();
    void setTokenBarrier();

    QHash<int, GdbCommand> m_cookieForToken;
    int commandTimeoutTime() const;
    QTimer m_commandTimer;

    QByteArray m_pendingConsoleStreamOutput;
    QByteArray m_pendingLogStreamOutput;

    // This contains the first token number for the current round
    // of evaluation. Responses with older tokens are considers
    // out of date and discarded.
    int m_oldestAcceptableToken;

    int m_pendingWatchRequests; // Watch updating commands in flight
    int m_pendingBreakpointRequests; // Watch updating commands in flight

    typedef void (GdbEngine::*CommandsDoneCallback)();
    // This function is called after all previous responses have been received.
    CommandsDoneCallback m_commandsDoneCallback;

    QList<GdbCommand> m_commandsToRunOnTemporaryBreak;
    int gdbVersion() const { return m_gdbVersion; }

private: ////////// Gdb Output, State & Capability Handling //////////

    void handleResponse(const QByteArray &buff);
    void handleStopResponse(const GdbMi &data);
    void handleResultRecord(GdbResponse *response);
    void handleStop0(const GdbMi &data);
    void handleStop1(const GdbResponse &response);
    void handleStop1(const GdbMi &data);
    StackFrame parseStackFrame(const GdbMi &mi, int level);
    void resetCommandQueue();

    bool isSynchronous() const { return hasPython(); }
    virtual bool hasPython() const;
    bool supportsThreads() const;

    // Gdb initialization sequence
    void handleShowVersion(const GdbResponse &response);
    void handleHasPython(const GdbResponse &response);

    int m_gdbVersion; // 6.8.0 is 60800
    int m_gdbBuildVersion; // MAC only?
    bool m_isMacGdb;
    bool m_hasPython;
    bool m_hasInferiorThreadList;

private: ////////// Inferior Management //////////

    // This should be always the last call in a function.
    //Q_SLOT virtual void attemptBreakpointSynchronization();
    bool stateAcceptsBreakpointChanges() const;
    bool acceptsBreakpoint(BreakpointId id) const;
    void insertBreakpoint(BreakpointId id);
    void removeBreakpoint(BreakpointId id);
    void changeBreakpoint(BreakpointId id);

    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void continueInferiorInternal();
    void autoContinueInferior();
    void continueInferior();
    void interruptInferior();
    void interruptInferiorTemporarily();

    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);
    void executeReturn();

    void handleExecuteContinue(const GdbResponse &response);
    void handleExecuteStep(const GdbResponse &response);
    void handleExecuteNext(const GdbResponse &response);
    void handleExecuteReturn(const GdbResponse &response);
    void handleExecuteJumpToLine(const GdbResponse &response);
    void handleExecuteRunToLine(const GdbResponse &response);
    //void handleExecuteRunToFunction(const GdbResponse &response);

    void maybeHandleInferiorPidChanged(const QString &pid);
    void handleInfoProc(const GdbResponse &response);

    QByteArray m_entryPoint;

private: ////////// View & Data Stuff //////////

    void selectThread(int index);
    void activateFrame(int index);

    //
    // Breakpoint specific stuff
    //
    void handleBreakList(const GdbResponse &response);
    void handleBreakList(const GdbMi &table);
    void handleBreakIgnore(const GdbResponse &response);
    void handleBreakDisable(const GdbResponse &response);
    void handleBreakEnable(const GdbResponse &response);
    void handleBreakInsert1(const GdbResponse &response);
    void handleBreakInsert2(const GdbResponse &response);
    void handleBreakCondition(const GdbResponse &response);
    void handleBreakInfo(const GdbResponse &response);
    void handleWatchInsert(const GdbResponse &response);
    void handleInfoLine(const GdbResponse &response);
    void extractDataFromInfoBreak(const QString &output, BreakpointId);
    void updateBreakpointDataFromOutput(BreakpointId id, const GdbMi &bkpt);
    QByteArray breakpointLocation(BreakpointId id);
    QString breakLocation(const QString &file) const;
    void reloadBreakListInternal();
    void attemptAdjustBreakpointLocation(BreakpointId id);

    //
    // Modules specific stuff
    //
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void examineModules();
    void reloadModulesInternal();
    void handleModulesList(const GdbResponse &response);

    bool m_modulesListOutdated;

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

    //
    // Disassembler specific stuff
    //
    void fetchDisassembler(DisassemblerViewAgent *agent);
    void fetchDisassemblerByAddress(const DisassemblerAgentCookie &ac,
        bool useMixedMode);
    void fetchDisassemblerByCli(const DisassemblerAgentCookie &ac,
        bool useMixedMode);
    void fetchDisassemblerByAddressCli(const DisassemblerAgentCookie &ac);
    void handleFetchDisassemblerByCli(const GdbResponse &response);
    void handleFetchDisassemblerByLine(const GdbResponse &response);
    void handleFetchDisassemblerByAddress1(const GdbResponse &response);
    void handleFetchDisassemblerByAddress0(const GdbResponse &response);
    QString parseDisassembler(const GdbMi &lines);

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

    void invalidateSourcesList();
    bool m_sourcesListOutdated;
    bool m_sourcesListUpdating;
    bool m_breakListOutdated;

    //
    // Stack specific stuff
    //
    void updateAll();
        void updateAllClassic();
        void updateAllPython();
    void handleStackListFrames(const GdbResponse &response);
    void handleStackSelectThread(const GdbResponse &response);
    void handleStackSelectFrame(const GdbResponse &response);
    void handleThreadListIds(const GdbResponse &response);
    void handleThreadInfo(const GdbResponse &response);
    void handleThreadNames(const GdbResponse &response);
    Q_SLOT void reloadStack(bool forceGotoLocation);
    Q_SLOT virtual void reloadFullStack();
    int currentFrame() const;

    QList<GdbMi> m_currentFunctionArgs;

    //
    // Watch specific stuff
    //
    virtual void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

    virtual void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);

    virtual void fetchMemory(MemoryViewAgent *agent, QObject *token,
        quint64 addr, quint64 length);
    void handleFetchMemory(const GdbResponse &response);

    virtual void watchPoint(const QPoint &);
    void handleWatchPoint(const GdbResponse &response);

    // FIXME: BaseClass. called to improve situation for a watch item
    void updateSubItemClassic(const WatchData &data);

    void virtual updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);
    Q_SLOT void updateWatchDataHelper(const WatchData &data);
    void rebuildWatchModel();
    bool showToolTip();

    void insertData(const WatchData &data);
    void sendWatchParameters(const QByteArray &params0);
    void createGdbVariableClassic(const WatchData &data);

    void runDebuggingHelperClassic(const WatchData &data, bool dumpChildren);
    void runDirectDebuggingHelperClassic(const WatchData &data, bool dumpChildren);
    bool hasDebuggingHelperForType(const QByteArray &type) const;

    void handleVarListChildrenClassic(const GdbResponse &response);
    void handleVarListChildrenHelperClassic(const GdbMi &child,
        const WatchData &parent);
    void handleVarCreate(const GdbResponse &response);
    void handleVarAssign(const GdbResponse &response);
    void handleEvaluateExpressionClassic(const GdbResponse &response);
    void handleQueryDebuggingHelperClassic(const GdbResponse &response);
    void handleDebuggingHelperValue2Classic(const GdbResponse &response);
    void handleDebuggingHelperValue3Classic(const GdbResponse &response);
    void handleDebuggingHelperEditValue(const GdbResponse &response);
    void handleDebuggingHelperSetup(const GdbResponse &response);
    void handleDebuggingHelperVersionCheckClassic(const GdbResponse &response);
    void handleDetach(const GdbResponse &response);

    Q_SLOT void createFullBacktrace();
    void handleCreateFullBacktrace(const GdbResponse &response);

    void updateLocals(const QVariant &cookie = QVariant());
        void updateLocalsClassic(const QVariant &cookie);
        void updateLocalsPython(bool tryPartial, const QByteArray &varList);
            void handleStackFramePython(const GdbResponse &response);

    void handleStackListLocalsClassic(const GdbResponse &response);
    void handleStackListLocalsPython(const GdbResponse &response);

    WatchData localVariable(const GdbMi &item,
                            const QStringList &uninitializedVariables,
                            QMap<QByteArray, int> *seen);
    void setLocals(const QList<GdbMi> &locals);
    void handleStackListArgumentsClassic(const GdbResponse &response);

    QSet<QByteArray> m_processedNames;

    //
    // Dumper Management
    //
    bool checkDebuggingHelpersClassic();
    void setDebuggingHelperStateClassic(DebuggingHelperState);
    void tryLoadDebuggingHelpersClassic();
    void tryQueryDebuggingHelpersClassic();
    Q_SLOT void setDebugDebuggingHelpersClassic(const QVariant &on);
    Q_SLOT void setUseDebuggingHelpers(const QVariant &on);

    DebuggingHelperState m_debuggingHelperState;
    QtDumperHelper m_dumperHelper;
    QString m_gdb;

    // 
    // Convenience Functions
    //
    QString errorMessage(QProcess::ProcessError error);
    AbstractGdbProcess *gdbProc() const;
    void showExecutionError(const QString &message);

    void removeTooltip();
    static QByteArray tooltipIName(const QString &exp);
    QString m_toolTipExpression;
    QPoint m_toolTipPos;

    // HACK:
    StackFrame m_targetFrame;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_OPERATORS_FOR_FLAGS(Debugger::Internal::GdbEngine::GdbCommandFlags)

#endif // DEBUGGER_GDBENGINE_H
