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

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggeritem.h"
#include "debuggerprotocol.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runnables.h>
#include <texteditor/textmark.h>

#include <QObject>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Core { class IOptionsPage; }

namespace Utils { class MacroExpander; }

namespace Debugger {

class DebuggerRunTool;

DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

namespace Internal {

class DebuggerEnginePrivate;
class DebuggerPluginPrivate;
class DisassemblerAgent;
class MemoryAgent;
class WatchItem;
class BreakHandler;
class LocationMark;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SourceFilesHandler;
class ThreadsHandler;
class WatchHandler;
class Breakpoint;
class QmlCppEngine;
class DebuggerToolTipContext;
class MemoryViewSetupData;
class Terminal;
class TerminalRunner;
class ThreadId;

class DebuggerRunParameters
{
public:
    DebuggerStartMode startMode = NoStartMode;
    DebuggerCloseMode closeMode = KillAtClose;

    ProjectExplorer::StandardRunnable inferior;
    QString displayName; // Used in the Snapshots view.
    Utils::ProcessHandle attachPID;
    QStringList solibSearchPath;

    // Used by Qml debugging.
    QUrl qmlServer;

    // Used by general remote debugging.
    QString remoteChannel;
    bool useExtendedRemote = false; // Whether to use GDB's target extended-remote or not.
    QString symbolFile;

    // Used by Mer plugin (3rd party)
    QMap<QString, QString> sourcePathMap;

    // Used by baremetal plugin
    QString commandsForReset; // commands used for resetting the inferior
    bool useContinueInsteadOfRun = false; // if connected to a hw debugger run is not possible but continue is used
    QString commandsAfterConnect; // additional commands to post after connection to debug target

    // Used by Valgrind
    QStringList expectedSignals;

    // For QNX debugging
    bool useCtrlCStub = false;

    // Used by Android to avoid false positives on warnOnRelease
    bool skipExecutableValidation = false;
    bool useTargetAsync = false;
    QStringList additionalSearchDirectories;

    // Used by iOS.
    QString platform;
    QString deviceSymbolsRoot;
    bool continueAfterAttach = false;
    QString sysRoot;

    // Used by general core file debugging. Public access requested in QTCREATORBUG-17158.
    QString coreFile;

    // Macro-expanded and passed to debugger startup.
    QString additionalStartupCommands;

    DebuggerEngineType cppEngineType = NoEngineType;

    bool isCppDebugging = true;
    bool isQmlDebugging = false;
    bool breakOnMain = false;
    bool multiProcess = false; // Whether to set detach-on-fork off.

    ProjectExplorer::StandardRunnable debugger;
    QString overrideStartScript; // Used in attach to core and remote debugging
    QString startMessage; // First status message shown.
    QString debugInfoLocation; // Gdb "set-debug-file-directory".
    QStringList debugSourceLocation; // Gdb "directory"
    bool isSnapshot = false; // Set if created internally.
    ProjectExplorer::Abi toolChainAbi;

    QString projectSourceDirectory;
    QStringList projectSourceFiles;

    // Used by Script debugging
    QString interpreter;
    QString mainScript;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    bool nativeMixedEnabled = false;

    bool isNativeMixedDebugging() const;
    void validateExecutable();

    Utils::MacroExpander *macroExpander = 0;

    // For Debugger testing.
    int testCase = 0;

    QStringList validationErrors;
};

class UpdateParameters
{
public:
    UpdateParameters(const QString &partialVariable = QString()) :
        partialVariable(partialVariable) {}

    QStringList partialVariables() const
    {
        QStringList result;
        if (!partialVariable.isEmpty())
            result.append(partialVariable);
        return result;
    }

    QString partialVariable;
};

class Location
{
public:
    Location() {}
    Location(quint64 address) { m_address = address; }
    Location(const QString &file) { m_fileName = file; }
    Location(const QString &file, int line, bool marker = true)
        { m_lineNumber = line; m_fileName = file; m_needsMarker = marker; }
    Location(const StackFrame &frame, bool marker = true);
    QString fileName() const { return m_fileName; }
    QString functionName() const { return m_functionName; }
    QString from() const { return m_from; }
    int lineNumber() const { return m_lineNumber; }
    void setNeedsRaise(bool on) { m_needsRaise = on; }
    void setNeedsMarker(bool on) { m_needsMarker = on; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    void setUseAssembler(bool on) { m_hasDebugInfo = !on; }
    bool needsRaise() const { return m_needsRaise; }
    bool needsMarker() const { return m_needsMarker; }
    bool hasDebugInfo() const { return m_hasDebugInfo; }
    bool canBeDisassembled() const
        { return m_address != quint64(-1) || !m_functionName.isEmpty(); }
    quint64 address() const { return m_address; }

private:
    bool m_needsMarker = false;
    bool m_needsRaise = true;
    bool m_hasDebugInfo = true;
    int m_lineNumber = -1;
    QString m_fileName;
    QString m_functionName;
    QString m_from;
    quint64 m_address = 0;
};

enum LocationType { UnknownLocation, LocationByFile, LocationByAddress };

class ContextData
{
public:
    bool isValid() const { return type != UnknownLocation; }

public:
    LocationType type = UnknownLocation;
    QString fileName;
    int lineNumber = 0;
    quint64 address = 0;
};

class DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerEngine();
    virtual ~DebuggerEngine();

    const DebuggerRunParameters &runParameters() const;

    virtual void setRunTool(DebuggerRunTool *runTool);
    DebuggerRunTool *runTool() const;

    void start();

    enum {
        // Remove need to qualify each use.
        NeedsTemporaryStop = DebuggerCommand::NeedsTemporaryStop,
        NeedsFullStop = DebuggerCommand::NeedsFullStop,
        Discardable = DebuggerCommand::Discardable,
        ConsoleCommand = DebuggerCommand::ConsoleCommand,
        NeedsFlush = DebuggerCommand::NeedsFlush,
        ExitRequest = DebuggerCommand::ExitRequest,
        RunRequest = DebuggerCommand::RunRequest,
        LosesChild = DebuggerCommand::LosesChild,
        RebuildBreakpointModel = DebuggerCommand::RebuildBreakpointModel,
        InUpdateLocals = DebuggerCommand::InUpdateLocals,
        NativeCommand = DebuggerCommand::NativeCommand,
        Silent = DebuggerCommand::Silent
    };

    virtual bool canHandleToolTip(const DebuggerToolTipContext &) const;
    virtual void expandItem(const QString &iname); // Called when item in tree gets expanded.
    virtual void updateItem(const QString &iname); // Called for fresh watch items.
    void updateWatchData(const QString &iname); // FIXME: Merge with above.
    virtual void selectWatchData(const QString &iname);

    virtual void prepareForRestart() {}

    virtual void watchPoint(const QPoint &pnt);
    virtual void runCommand(const DebuggerCommand &cmd);
    virtual void openMemoryView(const MemoryViewSetupData &data);
    virtual void fetchMemory(MemoryAgent *, quint64 addr, quint64 length);
    virtual void changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data);
    virtual void updateMemoryViews();
    virtual void openDisassemblerView(const Internal::Location &location);
    virtual void fetchDisassembler(Internal::DisassemblerAgent *);
    virtual void activateFrame(int index);

    virtual void reloadModules();
    virtual void examineModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadSymbolsForStack();
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);
    virtual void requestModuleSections(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();
    virtual void loadAdditionalQmlStack();
    virtual void reloadDebuggingHelpers();

    virtual void setRegisterValue(const QString &name, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const;
    virtual bool hasCapability(unsigned cap) const = 0;
    virtual void debugLastCommand() {}

    virtual bool isSynchronous() const;
    virtual QString qtNamespace() const;
    void setQtNamespace(const QString &ns);

    virtual void createSnapshot();
    virtual void updateAll();
    virtual void updateLocals();

    virtual bool stateAcceptsBreakpointChanges() const { return true; }
    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(Breakpoint bp) const = 0;
    virtual void insertBreakpoint(Breakpoint bp);  // FIXME: make pure
    virtual void removeBreakpoint(Breakpoint bp);  // FIXME: make pure
    virtual void changeBreakpoint(Breakpoint bp);  // FIXME: make pure

    virtual bool acceptsDebuggerCommands() const { return true; }
    virtual void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    virtual void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);
    virtual void selectThread(Internal::ThreadId threadId) = 0;

    virtual Internal::ModulesHandler *modulesHandler() const;
    virtual Internal::RegisterHandler *registerHandler() const;
    virtual Internal::StackHandler *stackHandler() const;
    virtual Internal::ThreadsHandler *threadsHandler() const;
    virtual Internal::WatchHandler *watchHandler() const;
    virtual Internal::SourceFilesHandler *sourceFilesHandler() const;
    virtual Internal::BreakHandler *breakHandler() const;

    virtual QAbstractItemModel *modulesModel() const;
    virtual QAbstractItemModel *registerModel() const;
    virtual QAbstractItemModel *stackModel() const;
    virtual QAbstractItemModel *threadsModel() const;
    virtual QAbstractItemModel *watchModel() const;
    virtual QAbstractItemModel *sourceFilesModel() const;

    void progressPing();
    bool debuggerActionsEnabled() const;
    static bool debuggerActionsEnabled(DebuggerState state);

    DebuggerState state() const;
    bool isDying() const;

    static QString stateName(int s);

    void notifyInferiorPid(const Utils::ProcessHandle &pid);
    qint64 inferiorPid() const;
    bool isReverseDebugging() const;
    void handleCommand(int role, const QVariant &value);

    // Convenience
    Q_SLOT virtual void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    Q_SLOT void showStatusMessage(const QString &msg, int timeout = -1) const;

    virtual void resetLocation();
    virtual void gotoLocation(const Internal::Location &location);
    virtual void quitDebugger(); // called when pressing the stop button

    void abortDebugger(); // called from the debug menu action

    void updateViews();
    bool isSlaveEngine() const;
    bool isMasterEngine() const;
    DebuggerEngine *masterEngine() const;
    virtual DebuggerEngine *activeEngine() { return this; }
    virtual DebuggerEngine *cppEngine() { return 0; }

    virtual bool canDisplayTooltip() const;

    virtual void notifyInferiorIll();

    QString toFileInProject(const QUrl &fileUrl);
    void updateBreakpointMarker(const Breakpoint &bp);
    void removeBreakpointMarker(const Breakpoint &bp);

    QString expand(const QString &string) const;
    QString nativeStartupCommands() const;

protected:
    // The base notify*() function implementation should be sufficient
    // in most cases, but engines are free to override them to do some
    // engine specific cleanup like stopping timers etc.
    void notifyEngineSetupOk();
    void notifyEngineSetupFailed();
    void notifyEngineRunFailed();

    void notifyInferiorSetupOk();
    void notifyInferiorSetupFailed();

    void notifyEngineRunAndInferiorRunOk();
    void notifyEngineRunAndInferiorStopOk();
    void notifyEngineRunOkAndInferiorUnrunnable(); // Called by CoreAdapter.

    // Use notifyInferiorRunRequested() plus notifyInferiorRunOk() instead.
    // void notifyInferiorSpontaneousRun();

    void notifyInferiorRunRequested();
    void notifyInferiorRunOk();
    void notifyInferiorRunFailed();

    void notifyInferiorStopOk();
    void notifyInferiorSpontaneousStop();
    void notifyInferiorStopFailed();

    public: // FIXME: Remove, currently needed for Android.
    void notifyInferiorExited();

    protected:
    void notifyDebuggerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus,
                                       const QString &backendName);

    virtual void setState(DebuggerState state, bool forced = false);

    void notifyInferiorShutdownOk();
    void notifyInferiorShutdownFailed();

    void notifyEngineSpontaneousShutdown();
    void notifyEngineShutdownOk();
    void notifyEngineShutdownFailed();

    void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;
    virtual void resetInferior() {}

    virtual void detachDebugger();
    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();
    virtual void executeReturn();

    virtual void continueInferior();
    virtual void interruptInferior();
    void requestInterruptInferior();

    virtual void executeRunToLine(const Internal::ContextData &data);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const Internal::ContextData &data);

    virtual void frameUp();
    virtual void frameDown();

    virtual void abortDebuggerProcess() {} // second attempt

    virtual void doUpdateLocals(const UpdateParameters &params);

    void setMasterEngine(DebuggerEngine *masterEngine);

    TerminalRunner *terminal() const;

    static QString msgStopped(const QString &reason = QString());
    static QString msgStoppedBySignal(const QString &meaning, const QString &name);
    static QString msgStoppedByException(const QString &description,
        const QString &threadId);
    static QString msgInterrupted();
    bool showStoppedBySignalMessageBox(const QString meaning, QString name);
    void showStoppedByExceptionMessageBox(const QString &description);

    virtual void setupSlaveEngine();
    virtual void runSlaveEngine();
    virtual void shutdownSlaveEngine();

    virtual void slaveEngineStateChanged(DebuggerEngine *engine,
        DebuggerState state);

    void updateLocalsView(const GdbMi &all);
    void checkState(DebuggerState state, const char *file, int line);
    bool isNativeMixedEnabled() const;
    bool isNativeMixedActive() const;
    bool isNativeMixedActiveFrame() const;

private:
    // Wrapper engine needs access to state of its subengines.
    friend class QmlCppEngine;
    friend class DebuggerPluginPrivate;

    friend class DebuggerEnginePrivate;
    friend class LocationMark;
    DebuggerEnginePrivate *d;
};

class LocationMark : public TextEditor::TextMark
{
public:
    LocationMark(DebuggerEngine *engine, const QString &file, int line);
    void removedFromEditor() override { updateLineNumber(0); }

private:
    bool isDraggable() const override;
    void dragToLine(int line) override;

    QPointer<DebuggerEngine> m_engine;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::UpdateParameters)
Q_DECLARE_METATYPE(Debugger::Internal::ContextData)
