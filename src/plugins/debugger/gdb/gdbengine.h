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

#include "idebuggerengine.h"
#include "debuggermanager.h" // only for StartParameters
#include "gdbmi.h"
#include "watchutils.h"

#include <QtCore/QByteArray>
#include <QtCore/QFutureInterface>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QTextCodec>
#include <QtCore/QTime>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QMessageBox;
class QTimer;
QT_END_NAMESPACE

namespace Debugger {
class DebuggerManager;
namespace Internal {

class AbstractGdbAdapter;
class GdbResponse;
class GdbMi;

class BreakpointData;
class WatchData;

class AttachGdbAdapter;
class CoreGdbAdapter;
class PlainGdbAdapter;
class RemoteGdbAdapter;
class TrkGdbAdapter;
struct TrkOptions;

enum DebuggingHelperState
{
    DebuggingHelperUninitialized,
    DebuggingHelperLoadTried,
    DebuggingHelperAvailable,
    DebuggingHelperUnavailable,
};

class GdbEngine : public IDebuggerEngine
{
    Q_OBJECT

public:
    explicit GdbEngine(DebuggerManager *manager);
    ~GdbEngine();

private:
    friend class AbstractGdbAdapter;
    friend class AttachGdbAdapter;
    friend class CoreGdbAdapter;
    friend class PlainGdbAdapter;
    friend class TermGdbAdapter;
    friend class RemoteGdbAdapter;
    friend class TrkGdbAdapter;

private: ////////// General Interface //////////

    virtual void addOptionPages(QList<Core::IOptionsPage*> *opts) const;

    virtual bool checkConfiguration(int toolChain, QString *errorMessage,
        QString *settingsPage = 0) const;
    virtual void startDebugger(const DebuggerStartParametersPtr &sp);
    virtual unsigned debuggerCapabilities() const;
    virtual void exitDebugger();
    virtual void detachDebugger();
    virtual void shutdown();

    virtual void executeDebuggerCommand(const QString &command);
    virtual QString qtNamespace() const { return m_dumperHelper.qtNamespace(); }

private: ////////// General State //////////

    void initializeVariables();
    DebuggerStartMode startMode() const;
    const DebuggerStartParameters &startParameters() const
        { return *m_startParameters; }
    Q_SLOT void setAutoDerefPointers(const QVariant &on);

    DebuggerStartParametersPtr m_startParameters;
    QSharedPointer<TrkOptions> m_trkOptions;
    bool m_registerNamesListed;

private: ////////// Gdb Process Management //////////

    AbstractGdbAdapter *createAdapter(const DebuggerStartParametersPtr &dp);
    void connectAdapter();
    bool startGdb(const QStringList &args = QStringList(),
                  const QString &gdb = QString(),
                  const QString &settingsIdHint = QString());
    void startInferiorPhase2();

    void handleInferiorShutdown(const GdbResponse &response);
    void handleGdbExit(const GdbResponse &response);

    void showDebuggerInput(int channel, const QString &msg)
        { m_manager->showDebuggerInput(channel, msg); }
    void showDebuggerOutput(int channel, const QString &msg)
        { m_manager->showDebuggerOutput(channel, msg); }

private slots:
    void handleGdbFinished(int, QProcess::ExitStatus status);
    void handleGdbError(QProcess::ProcessError error);
    void readGdbStandardOutput();
    void readGdbStandardError();
    void readDebugeeOutput(const QByteArray &data);

    void handleAdapterStarted();
    void handleAdapterStartFailed(const QString &msg,
        const QString &settingsIdHint = QString());

    void handleInferiorPrepared();

    void handleInferiorStartFailed(const QString &msg);

    void handleAdapterCrashed(const QString &msg);

private:
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;

    QByteArray m_inbuffer;
    bool m_busy;

    QProcess m_gdbProc;
    AbstractGdbAdapter *m_gdbAdapter;

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
        // We can live without recieving an answer.
        NonCriticalResponse = 8,
        // Callback expects GdbResultRunning instead of GdbResultDone.
        RunRequest = 16,
        // Callback expects GdbResultExit instead of GdbResultDone.
        ExitRequest = 32,
        // Auto-set inferior shutdown related states.
        LosesChild = 64,
        // Trigger breakpoint model rebuild when no such commands are pending anymore.
        RebuildBreakpointModel = 128,
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
    QTimer *m_commandTimer;

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
    void handleStop1(const GdbResponse &response);
    void handleStop1(const GdbMi &data);
    StackFrame parseStackFrame(const GdbMi &mi, int level);

    bool isSynchroneous() const { return hasPython(); }
    virtual bool hasPython() const;
    bool supportsThreads() const;

    // Gdb initialization sequence
    void handleShowVersion(const GdbResponse &response);
    void handleHasPython(const GdbResponse &response);

    int m_gdbVersion; // 6.8.0 is 60800
    int m_gdbBuildVersion; // MAC only?
    bool m_isMacGdb;
    bool m_hasPython;

private: ////////// Inferior Management //////////

    // This should be always the last call in a function.
    Q_SLOT virtual void attemptBreakpointSynchronization();

    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();

    void continueInferiorInternal();
    void autoContinueInferior();
    virtual void continueInferior();
    virtual void interruptInferior();
    void interruptInferiorTemporarily();

    virtual void executeRunToLine(const QString &fileName, int lineNumber);
    virtual void executeRunToFunction(const QString &functionName);
//    void handleExecRunToFunction(const GdbResponse &response);
    virtual void executeJumpToLine(const QString &fileName, int lineNumber);
    virtual void executeReturn();

    void handleExecuteContinue(const GdbResponse &response);
    void handleExecuteStep(const GdbResponse &response);
    void handleExecuteNext(const GdbResponse &response);
    void handleExecuteReturn(const GdbResponse &response);

    qint64 inferiorPid() const { return m_manager->inferiorPid(); }
    void handleInferiorPidChanged(qint64 pid) { manager()->notifyInferiorPidChanged(pid); }
    void maybeHandleInferiorPidChanged(const QString &pid);

#ifdef Q_OS_LINUX
    void handleInfoProc(const GdbResponse &response);

    QByteArray m_entryPoint;
#endif
    QFutureInterface<void> *m_progress;

private: ////////// View & Data Stuff //////////

    virtual void selectThread(int index);
    virtual void activateFrame(int index);

    void gotoLocation(const StackFrame &frame, bool setLocationMarker);

    //
    // Breakpoint specific stuff
    //
    void handleBreakList(const GdbResponse &response);
    void handleBreakList(const GdbMi &table);
    void handleBreakIgnore(const GdbResponse &response);
    void handleBreakDisable(const GdbResponse &response);
    void handleBreakInsert1(const GdbResponse &response);
    void handleBreakInsert2(const GdbResponse &response);
    void handleBreakCondition(const GdbResponse &response);
    void handleBreakInfo(const GdbResponse &response);
    void extractDataFromInfoBreak(const QString &output, BreakpointData *data);
    void breakpointDataFromOutput(BreakpointData *data, const GdbMi &bkpt);
    QByteArray breakpointLocation(int index);
    void sendInsertBreakpoint(int index);
    QString breakLocation(const QString &file) const;
    void reloadBreakListInternal();

    //
    // Modules specific stuff
    //
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();
    virtual QList<Symbol> moduleSymbols(const QString &moduleName);
    virtual void reloadModules();
    void reloadModulesInternal();
    void handleModulesList(const GdbResponse &response);

    bool m_modulesListOutdated;

    //
    // Snapshot specific stuff
    //
    virtual void makeSnapshot();
    void handleMakeSnapshot(const GdbResponse &response);
    void handleActivateSnapshot(const GdbResponse &response);
    void activateSnapshot(int index);
    void activateSnapshot2();

    //
    // Register specific stuff
    //
    Q_SLOT virtual void reloadRegisters();
    virtual void setRegisterValue(int nr, const QString &value);
    void handleRegisterListNames(const GdbResponse &response);
    void handleRegisterListValues(const GdbResponse &response);

    //
    // Disassembler specific stuff
    //
    virtual void fetchDisassembler(DisassemblerViewAgent *agent);
    void fetchDisassemblerByAddress(DisassemblerViewAgent *agent,
        bool useMixedMode);
    void fetchDisassemblerByCli(DisassemblerViewAgent *agent,
        bool useMixedMode);
    void fetchDisassemblerByAddressCli(DisassemblerViewAgent *agent);
    void handleFetchDisassemblerByCli(const GdbResponse &response);
    void handleFetchDisassemblerByLine(const GdbResponse &response);
    void handleFetchDisassemblerByAddress1(const GdbResponse &response);
    void handleFetchDisassemblerByAddress0(const GdbResponse &response);
    QString parseDisassembler(const GdbMi &lines);

    //
    // Source file specific stuff
    //
    virtual void reloadSourceFiles();
    void reloadSourceFilesInternal();
    void handleQuerySources(const GdbResponse &response);

    QString fullName(const QString &fileName);
#ifdef Q_OS_WIN
    QString cleanupFullName(const QString &fileName);
#else
    QString cleanupFullName(const QString &fileName) { return fileName; }
#endif

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
    void handleStackListThreads(const GdbResponse &response);
    Q_SLOT void reloadStack(bool forceGotoLocation);
    Q_SLOT virtual void reloadFullStack();
    int currentFrame() const;

    QList<GdbMi> m_currentFunctionArgs;
    QString m_currentFrame;

    //
    // Watch specific stuff
    //
    virtual void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

    virtual void assignValueInDebugger(const QString &expr, const QString &value);

    virtual void fetchMemory(MemoryViewAgent *agent, QObject *token,
        quint64 addr, quint64 length);
    void handleFetchMemory(const GdbResponse &response);

    virtual void watchPoint(const QPoint &);
    void handleWatchPoint(const GdbResponse &response);

    // FIXME: BaseClass. called to improve situation for a watch item
    void updateSubItemClassic(const WatchData &data);
    void handleChildren(const WatchData &parent, const GdbMi &child,
        QList<WatchData> *insertions);

    void virtual updateWatchData(const WatchData &data);
    Q_SLOT void updateWatchDataHelper(const WatchData &data);
    void rebuildWatchModel();
    bool showToolTip();

    void insertData(const WatchData &data);
    void sendWatchParameters(const QByteArray &params0);
    void createGdbVariableClassic(const WatchData &data);

    void runDebuggingHelperClassic(const WatchData &data, bool dumpChildren);
    void runDirectDebuggingHelperClassic(const WatchData &data, bool dumpChildren);
    bool hasDebuggingHelperForType(const QString &type) const;

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

    void updateLocals(const QVariant &cookie = QVariant());
        void updateLocalsClassic(const QVariant &cookie);
        void updateLocalsPython(const QByteArray &varList);
            void handleStackFramePython(const GdbResponse &response);

    void handleStackListLocalsClassic(const GdbResponse &response);
    void handleStackListLocalsPython(const GdbResponse &response);

    WatchData localVariable(const GdbMi &item,
                            const QStringList &uninitializedVariables,
                            QMap<QByteArray, int> *seen);
    void setLocals(const QList<GdbMi> &locals);
    void handleStackListArgumentsClassic(const GdbResponse &response);
    void setWatchDataType(WatchData &data, const GdbMi &mi);
    void setWatchDataDisplayedType(WatchData &data, const GdbMi &mi);

    QSet<QByteArray> m_processedNames;
    QMap<QString, QString> m_varToType;

private: ////////// Dumper Management //////////
    QString qtDumperLibraryName() const;
    bool checkDebuggingHelpers();
        bool checkDebuggingHelpersClassic();
    void setDebuggingHelperStateClassic(DebuggingHelperState);
    void tryLoadDebuggingHelpersClassic();
    void tryQueryDebuggingHelpersClassic();
    Q_SLOT void recheckDebuggingHelperAvailabilityClassic();
    void connectDebuggingHelperActions();
    void disconnectDebuggingHelperActions();
    Q_SLOT void setDebugDebuggingHelpersClassic(const QVariant &on);
    Q_SLOT void setUseDebuggingHelpers(const QVariant &on);

    DebuggingHelperState m_debuggingHelperState;
    QtDumperHelper m_dumperHelper;

private: ////////// Convenience Functions //////////

    QString errorMessage(QProcess::ProcessError error);
    QMessageBox *showMessageBox(int icon, const QString &title, const QString &text,
        int buttons = 0);
    void debugMessage(const QString &msg);
    QMainWindow *mainWindow() const;

    static QString m_toolTipExpression;
    static QPoint m_toolTipPos;
    static QByteArray tooltipINameForExpression(const QByteArray &exp);

    static void setWatchDataValue(WatchData &data, const GdbMi &mi,
        int encoding = 0);
    static void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
            int encoding = 0);
    static void setWatchDataChildCount(WatchData &data, const GdbMi &mi);
    static void setWatchDataValueEnabled(WatchData &data, const GdbMi &mi);
    static void setWatchDataValueEditable(WatchData &data, const GdbMi &mi);
    static void setWatchDataExpression(WatchData &data, const GdbMi &mi);
    static void setWatchDataAddress(WatchData &data, const GdbMi &mi);
    static void setWatchDataAddressHelper(WatchData &data, const QByteArray &addr);
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_OPERATORS_FOR_FLAGS(Debugger::Internal::GdbEngine::GdbCommandFlags)

#endif // DEBUGGER_GDBENGINE_H
