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
#include "breakhandler.h"
#include "threadshandler.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runconfiguration.h>
#include <texteditor/textmark.h>
#include <utils/fileutils.h>

#include <QProcess>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
QT_END_NAMESPACE

namespace Core { class IOptionsPage; }

namespace Utils {
class MacroExpander;
class Perspective;
} // Utils

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
class BreakpointParameters;
class LocationMark;
class LogWindow;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SourceFilesHandler;
class WatchHandler;
class WatchTreeView;
class DebuggerToolTipContext;
class DebuggerToolTipManager;
class MemoryViewSetupData;
class TerminalRunner;

class DebuggerRunParameters
{
public:
    DebuggerStartMode startMode = NoStartMode;
    DebuggerCloseMode closeMode = KillAtClose;

    ProjectExplorer::Runnable inferior;
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
    Utils::FileNameList additionalSearchDirectories;

    // Used by iOS.
    QString platform;
    QString deviceSymbolsRoot;
    bool continueAfterAttach = false;
    Utils::FileName sysRoot;

    // Used by general core file debugging. Public access requested in QTCREATORBUG-17158.
    QString coreFile;

    // Macro-expanded and passed to debugger startup.
    QString additionalStartupCommands;

    DebuggerEngineType cppEngineType = NoEngineType;

    bool isQmlDebugging = false;
    bool breakOnMain = false;
    bool multiProcess = false; // Whether to set detach-on-fork off.
    bool useTerminal = false;

    ProjectExplorer::Runnable debugger;
    QString overrideStartScript; // Used in attach to core and remote debugging
    QString startMessage; // First status message shown.
    QString debugInfoLocation; // Gdb "set-debug-file-directory".
    QStringList debugSourceLocation; // Gdb "directory"
    QString qtPackageSourceLocation;
    bool isSnapshot = false; // Set if created internally.
    ProjectExplorer::Abi toolChainAbi;

    Utils::FileName projectSourceDirectory;
    Utils::FileNameList projectSourceFiles;

    // Used by Script debugging
    QString interpreter;
    QString mainScript;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    bool nativeMixedEnabled = false;

    bool isCppDebugging() const;
    bool isNativeMixedDebugging() const;

    Utils::MacroExpander *macroExpander = nullptr;

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
    Location() = default;
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

class DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    DebuggerEngine();
    ~DebuggerEngine() override;

    void setRunTool(DebuggerRunTool *runTool);
    void setRunParameters(const DebuggerRunParameters &runParameters);

    void setRunId(const QString &id);
    QString runId() const;

    const DebuggerRunParameters &runParameters() const;
    bool isStartupRunConfiguration() const;
    void setCompanionEngine(DebuggerEngine *engine);
    void setSecondaryEngine();

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
        InUpdateLocals = DebuggerCommand::InUpdateLocals,
        NativeCommand = DebuggerCommand::NativeCommand,
        Silent = DebuggerCommand::Silent
    };

    virtual bool canHandleToolTip(const DebuggerToolTipContext &) const;
    virtual void expandItem(const QString &iname); // Called when item in tree gets expanded.
    virtual void updateItem(const QString &iname); // Called for fresh watch items.
    void updateWatchData(const QString &iname); // FIXME: Merge with above.
    virtual void selectWatchData(const QString &iname);

    virtual void validateRunParameters(DebuggerRunParameters &) {}
    virtual void prepareForRestart() {}
    virtual void abortDebuggerProcess() {} // second attempt

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

    virtual QString qtNamespace() const;
    void setQtNamespace(const QString &ns);

    virtual void createSnapshot();
    virtual void updateAll();
    virtual void updateLocals();

    Core::Context debuggerContext() const;
    virtual Core::Context languageContext() const { return {}; }
    QString displayName() const;

    virtual bool stateAcceptsBreakpointChanges() const { return true; }
    virtual bool acceptsBreakpoint(const BreakpointParameters &bp) const = 0;
    virtual void insertBreakpoint(const Breakpoint &bp) = 0;
    virtual void removeBreakpoint(const Breakpoint &bp) = 0;
    virtual void updateBreakpoint(const Breakpoint &bp) = 0;
    virtual void enableSubBreakpoint(const SubBreakpoint &sbp, bool enabled);

    virtual bool acceptsDebuggerCommands() const { return true; }
    virtual void executeDebuggerCommand(const QString &command);

    virtual void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);
    virtual void selectThread(const Internal::Thread &thread) = 0;

    virtual void executeRecordReverse(bool) {}
    virtual void executeReverse(bool) {}

    ModulesHandler *modulesHandler() const;
    RegisterHandler *registerHandler() const;
    StackHandler *stackHandler() const;
    ThreadsHandler *threadsHandler() const;
    WatchHandler *watchHandler() const;
    SourceFilesHandler *sourceFilesHandler() const;
    BreakHandler *breakHandler() const;
    LogWindow *logWindow() const;
    DisassemblerAgent *disassemblerAgent() const;

    void progressPing();
    bool debuggerActionsEnabled() const;
    virtual bool companionPreventsActions() const;

    bool operatesByInstruction() const;
    virtual void operateByInstructionTriggered(bool on); // FIXME: Remove.

    DebuggerState state() const;
    bool isDying() const;

    static QString stateName(int s);

    void notifyInferiorPid(const Utils::ProcessHandle &pid);
    qint64 inferiorPid() const;

    bool isReverseDebugging() const;
    void handleBeginOfRecordingReached();
    void handleRecordingFailed();
    void handleRecordReverse(bool);
    void handleReverseDirection(bool);

    // Convenience
    Q_SLOT virtual void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    Q_SLOT void showStatusMessage(const QString &msg, int timeout = -1) const;

    virtual void resetLocation();
    virtual void gotoLocation(const Internal::Location &location);
    void gotoCurrentLocation();
    virtual void quitDebugger(); // called when pressing the stop button
    void abortDebugger();
    void updateUi(bool isCurrentEngine);

    bool isPrimaryEngine() const;

    virtual bool canDisplayTooltip() const;

    QString toFileInProject(const QUrl &fileUrl);

    QString expand(const QString &string) const;
    QString nativeStartupCommands() const;
    Utils::Perspective *perspective() const;
    void updateMarkers();

    void updateToolTips();
    DebuggerToolTipManager *toolTipManager();

signals:
    void engineStarted();
    void engineFinished();
    void requestRunControlFinish();
    void requestRunControlStop();
    void attachToCoreRequested(const QString &coreFile);
    void appendMessageRequested(const QString &msg,
                                Utils::OutputFormat format,
                                bool appendNewLine) const;

protected:
    void notifyEngineSetupOk();
    void notifyEngineSetupFailed();
    void notifyEngineRunFailed();

    void notifyEngineRunAndInferiorRunOk();
    void notifyEngineRunAndInferiorStopOk();
    void notifyEngineRunOkAndInferiorUnrunnable(); // Called by CoreAdapter.

    // Use notifyInferiorRunRequested() plus notifyInferiorRunOk() instead.
    // void notifyInferiorSpontaneousRun();

    void notifyInferiorRunRequested();
    void notifyInferiorRunOk();
    void notifyInferiorRunFailed();

    void notifyInferiorIll();
    void notifyInferiorExited();

    void notifyInferiorStopOk();
    void notifyInferiorSpontaneousStop();
    void notifyInferiorStopFailed();

public:
    void updateState(bool alsoUpdateCompanion);
    QString formatStartParameters() const;
    WatchTreeView *inspectorView();
    void updateLocalsWindow(bool showReturn);
    void raiseWatchersWindow();
    QString debuggerName() const;

    bool isRegistersWindowVisible() const;
    bool isModulesWindowVisible() const;

    void openMemoryEditor();

    void handleExecDetach();
    void handleExecContinue();
    void handleExecInterrupt();
    void handleUserStop();
    void handleAbort();
    void handleReset();
    void handleExecStepIn();
    void handleExecStepOver();
    void handleExecStepOut();
    void handleExecReturn();
    void handleExecJumpToLine();
    void handleExecRunToLine();
    void handleExecRunToSelectedFunction();
    void handleAddToWatchWindow();
    void handleFrameDown();
    void handleFrameUp();

    // Breakpoint state transitions
    void notifyBreakpointInsertProceeding(const Breakpoint &bp);
    void notifyBreakpointInsertOk(const Breakpoint &bp);
    void notifyBreakpointInsertFailed(const Breakpoint &bp);
    void notifyBreakpointChangeOk(const Breakpoint &bp);
    void notifyBreakpointChangeProceeding(const Breakpoint &bp);
    void notifyBreakpointChangeFailed(const Breakpoint &bp);
    void notifyBreakpointPending(const Breakpoint &bp);
    void notifyBreakpointRemoveProceeding(const Breakpoint &bp);
    void notifyBreakpointRemoveOk(const Breakpoint &bp);
    void notifyBreakpointRemoveFailed(const Breakpoint &bp);
    void notifyBreakpointNeedsReinsertion(const Breakpoint &bp);

protected:
    void setDebuggerName(const QString &name);
    void notifyDebuggerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus,
                                       const QString &backendName);

    virtual void setState(DebuggerState state, bool forced = false);

    void notifyInferiorShutdownFinished();

    void notifyEngineSpontaneousShutdown();
    void notifyEngineShutdownFinished();

    void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;
    virtual void resetInferior() {}

    virtual void detachDebugger() {}
    virtual void executeStepOver(bool /*byInstruction*/ = false) {}
    virtual void executeStepIn(bool /*byInstruction*/ = false) {}
    virtual void executeStepOut() {}
    virtual void executeReturn() {}

    virtual void continueInferior() {}
    virtual void interruptInferior() {}
    void requestInterruptInferior();

    virtual void executeRunToLine(const Internal::ContextData &) {}
    virtual void executeRunToFunction(const QString &) {}
    virtual void executeJumpToLine(const Internal::ContextData &) {}

    virtual void frameUp();
    virtual void frameDown();

    virtual void doUpdateLocals(const UpdateParameters &params);

    TerminalRunner *terminal() const;

    static QString msgStopped(const QString &reason = QString());
    static QString msgStoppedBySignal(const QString &meaning, const QString &name);
    static QString msgStoppedByException(const QString &description,
        const QString &threadId);
    static QString msgInterrupted();
    bool showStoppedBySignalMessageBox(const QString meaning, QString name);
    void showStoppedByExceptionMessageBox(const QString &description);

    void updateLocalsView(const GdbMi &all);
    void checkState(DebuggerState state, const char *file, int line);
    bool isNativeMixedEnabled() const;
    bool isNativeMixedActive() const;
    bool isNativeMixedActiveFrame() const;
    void startDying() const;

protected:
    ProjectExplorer::IDevice::ConstPtr device() const;
    DebuggerEngine *companionEngine() const;

private:
    friend class DebuggerPluginPrivate;
    friend class DebuggerEnginePrivate;
    friend class LocationMark;
    DebuggerEnginePrivate *d;
};

class CppDebuggerEngine : public DebuggerEngine
{
public:
    CppDebuggerEngine() {}
    ~CppDebuggerEngine() override {}

    void validateRunParameters(DebuggerRunParameters &rp) override;
    Core::Context languageContext() const override;
};

class LocationMark : public TextEditor::TextMark
{
public:
    LocationMark(DebuggerEngine *engine, const Utils::FileName &file, int line);
    void removedFromEditor() override { updateLineNumber(0); }

    void updateIcon();

private:
    bool isDraggable() const override;
    void dragToLine(int line) override;

    QPointer<DebuggerEngine> m_engine;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::UpdateParameters)
Q_DECLARE_METATYPE(Debugger::Internal::ContextData)
