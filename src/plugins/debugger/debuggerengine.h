// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerprotocol.h"
#include "breakhandler.h"
#include "threadshandler.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/devicesupport/idevicefwd.h>

#include <texteditor/textmark.h>

#include <utils/filepath.h>
#include <utils/outputformat.h>
#include <utils/processhandle.h>
#include <utils/processinterface.h>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
QT_END_NAMESPACE

namespace Core { class IOptionsPage; }

namespace ProjectExplorer { class RunControl; }

namespace Utils {
class MacroExpander;
class Perspective;
class ProcessResultData;
class Result;
} // Utils

namespace Debugger {

class DebuggerRunTool;

enum DebuggerState
{
    DebuggerNotReady,          // Debugger not started

    EngineSetupRequested,      // Engine starts
    EngineSetupFailed,

    EngineRunRequested,
    EngineRunFailed,

    InferiorUnrunnable,        // Used in the core dump adapter

    InferiorRunRequested,      // Debuggee requested to run
    InferiorRunOk,             // Debuggee running
    InferiorRunFailed,         // Debuggee not running

    InferiorStopRequested,     // Debuggee running, stop requested
    InferiorStopOk,            // Debuggee stopped
    InferiorStopFailed,        // Debuggee not stopped, will kill debugger

    InferiorShutdownRequested,
    InferiorShutdownFinished,

    EngineShutdownRequested,
    EngineShutdownFinished,

    DebuggerFinished
};

DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

class DEBUGGER_EXPORT DebuggerRunParameters
{
public:
    static DebuggerRunParameters fromRunControl(ProjectExplorer::RunControl *runControl);

    static void setBreakOnMainNextTime();

    Utils::Result fixupParameters(ProjectExplorer::RunControl *runControl);

    void setStartMode(DebuggerStartMode startMode);
    DebuggerStartMode startMode() const { return m_startMode; }
    bool isLocalAttachEngine() const { return m_startMode == AttachToLocalProcess; }

    void setCloseMode(DebuggerCloseMode closeMode) { m_closeMode = closeMode; }
    DebuggerCloseMode closeMode() const { return m_closeMode; }

    void setInferior(const Utils::ProcessRunData &runnable) { m_inferior = runnable; }
    void setInferiorExecutable(const Utils::FilePath &executable) {
        m_inferior.command.setExecutable(executable);
    }
    void setInferiorEnvironment(const Utils::Environment &env) { m_inferior.environment = env; }
    Utils::ProcessRunData inferior() const { return m_inferior; }

    void setDisplayName(const QString &name) { m_displayName = name; }
    QString displayName() const { return m_displayName; }

    void setAttachPid(Utils::ProcessHandle pid) { m_attachPid = pid; }
    void setAttachPid(qint64 pid) { m_attachPid = Utils::ProcessHandle(pid); }
    Utils::ProcessHandle attachPid() const { return m_attachPid; }

    void setSolibSearchPath(const Utils::FilePaths &list) { m_solibSearchPath = list; }
    void addSolibSearchDir(const QString &str);
    Utils::FilePaths solibSearchPath() const { return m_solibSearchPath; }

    void setQmlServer(const QUrl &qmlServer) { m_qmlServer = qmlServer; }
    QUrl qmlServer() const { return m_qmlServer; }

    bool isQmlDebugging() const { return m_isQmlDebugging; }
    void setQmlDebugging(bool on) { m_isQmlDebugging = on; }

    void setRemoteChannel(const QString &channel) { m_remoteChannel = channel; }
    void setRemoteChannel(const QUrl &url) {
        m_remoteChannel = QString("%1:%2").arg(url.host()).arg(url.port());
    }
    void setRemoteChannel(const QString &host, int port) {
        m_remoteChannel = QString("%1:%2").arg(host).arg(port);
    }
    QString remoteChannel() const { return m_remoteChannel; }

    void setUseExtendedRemote(bool on) { m_useExtendedRemote = on; }
    bool useExtendedRemote() const { return m_useExtendedRemote; }

    void setSymbolFile(const Utils::FilePath &symbolFile) { m_symbolFile = symbolFile; }
    Utils::FilePath symbolFile() const { return m_symbolFile; }

    void insertSourcePath(const QString &key, const QString &value) {
        m_sourcePathMap.insert(key, value);
    }
    QMap<QString, QString> sourcePathMap() const { return m_sourcePathMap; }

    void setCommandsForReset(const QString &commands) { m_commandsForReset = commands; }
    QString commandsForReset() const { return m_commandsForReset; }

    void setUseContinueInsteadOfRun(bool on) { m_useContinueInsteadOfRun = on; }
    bool useContinueInsteadOfRun() const { return m_useContinueInsteadOfRun; }

    void setCommandsAfterConnect(const QString &commands) { m_commandsAfterConnect = commands; }
    QString commandsAfterConnect() const { return m_commandsAfterConnect; }

    void addExpectedSignal(const QString &signal) { m_expectedSignals.append(signal); }
    QStringList expectedSignals() const { return m_expectedSignals; }

    void setUseCtrlCStub(bool on) { m_useCtrlCStub = on; }
    bool useCtrlCStub() const { return m_useCtrlCStub; }

    void setUseTargetAsync(bool on) { m_useTargetAsync = on; }
    bool useTargetAsync() const { return m_useTargetAsync; }

    void addSearchDirectory(const Utils::FilePath &dir) { m_additionalSearchDirectories.append(dir); }
    Utils::FilePaths additionalSearchDirectories() const { return m_additionalSearchDirectories; }

    void setLldbPlatform(const QString &platform) { m_lldbPlatform = platform; }
    QString lldbPlatform() const { return m_lldbPlatform; }

    void setDeviceSymbolsRoot(const QString &deviceSymbolsRoot) {
        m_deviceSymbolsRoot = deviceSymbolsRoot;
    }
    QString deviceSymbolsRoot() const { return m_deviceSymbolsRoot; }

    void setContinueAfterAttach(bool on) { m_continueAfterAttach = on; }
    bool continueAfterAttach() const { return m_continueAfterAttach; }

    void setSysRoot(const Utils::FilePath &sysRoot) { m_sysRoot = sysRoot; }
    Utils::FilePath sysRoot() const { return m_sysRoot; }

    void setDeviceUuid(const QString &uuid) { m_deviceUuid = uuid; }
    QString deviceUuid() const { return m_deviceUuid; }

    void setCoreFilePath(const Utils::FilePath &coreFile) { m_coreFile = coreFile; }
    Utils::FilePath coreFile() const { return m_coreFile; }

    void setSnapshot(bool isSnapshot) { m_isSnapshot = isSnapshot; }
    bool isSnapshot() const { return m_isSnapshot; }

    QString additionalStartupCommands() const { return m_additionalStartupCommands; }

    DebuggerEngineType cppEngineType = NoEngineType;
    QString version;

    bool isPythonDebugging = false;
    bool breakOnMain = false;
    bool multiProcess = false; // Whether to set detach-on-fork off.
    bool useTerminal = false;
    bool runAsRoot = false;

    Utils::ProcessRunData debugger;
    Utils::FilePath overrideStartScript; // Used in attach to core and remote debugging
    QString startMessage; // First status message shown.
    Utils::FilePath debugInfoLocation; // Gdb "set-debug-file-directory".
    QStringList debugSourceLocation; // Gdb "directory"
    Utils::FilePath qtSourceLocation;
    ProjectExplorer::Abi toolChainAbi;

    Utils::FilePath projectSourceDirectory;
    Utils::FilePaths projectSourceFiles;

    // Terminal
    qint64 applicationPid = 0;
    qint64 applicationMainThreadId = 0;

    // Used by Script debugging
    Utils::FilePath interpreter;
    Utils::FilePath mainScript;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    bool nativeMixedEnabled = false;

    bool isCppDebugging() const;
    bool isNativeMixedDebugging() const;

    const Utils::MacroExpander *macroExpander = nullptr;

    std::optional<int> exitCode = {};

    // For Debugger testing.
    int testCase = 0;

    QStringList validationErrors;

    int qtVersion = 0;
    QString qtNamespace;

    // Common debugger constants.
    Utils::FilePath peripheralDescriptionFile;

    // UVSC-specific debugger constants.
    Utils::FilePath uVisionProjectFilePath;
    Utils::FilePath uVisionOptionsFilePath;
    bool uVisionSimulator = false;

private:
    DebuggerStartMode m_startMode = NoStartMode;
    DebuggerCloseMode m_closeMode = KillAtClose;

    Utils::ProcessRunData m_inferior;

    QString m_displayName; // Used in the Snapshots view.

    Utils::ProcessHandle m_attachPid;

    Utils::FilePaths m_solibSearchPath;

    QUrl m_qmlServer; // Used by Qml debugging.
    bool m_isQmlDebugging = false;

    QString m_remoteChannel; // Used by general remote debugging.
    bool m_useExtendedRemote = false; // Whether to use GDB's target extended-remote or not.
    Utils::FilePath m_symbolFile;

    QMap<QString, QString> m_sourcePathMap; // Used by Mer plugin (3rd party)

    QString m_commandsForReset; // Used by baremetal plugin. Commands used for resetting the inferior
    bool m_useContinueInsteadOfRun = false; // If connected to a hw debugger run is not possible but continue is used
    QString m_commandsAfterConnect; // Additional commands to post after connection to debug target

    QStringList m_expectedSignals; // Used by Valgrind

    bool m_useCtrlCStub = false; // For QNX debugging.

    bool m_useTargetAsync = false;

    Utils::FilePaths m_additionalSearchDirectories;

    QString m_lldbPlatform;

    QString m_deviceSymbolsRoot;
    bool m_continueAfterAttach = false;
    Utils::FilePath m_sysRoot;

    QString m_deviceUuid; // iOS 17+

    // Used by general core file debugging. Public access requested in QTCREATORBUG-17158.
    Utils::FilePath m_coreFile;
    bool m_isSnapshot = false; // Set if created internally.

    // Macro-expanded and passed to debugger startup.
    QString m_additionalStartupCommands;
};

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
class PeripheralRegisterHandler;
class Section;
class SourceFilesHandler;
class StackFrame;
class StackHandler;
class Symbol;
class WatchHandler;
class WatchTreeView;
class DebuggerToolTipContext;
class DebuggerToolTipManager;
class MemoryViewSetupData;

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
    bool qmlFocusOnFrame = true; // QTCREATORBUG-29874
};

class Location
{
public:
    Location() = default;
    Location(quint64 address) { m_address = address; }
    Location(const Utils::FilePath &file) { m_fileName = file; }
    Location(const Utils::FilePath &file, int line, bool marker = true)
        { m_textPosition = {line, -1}; m_fileName = file; m_needsMarker = marker; }
    Location(const Utils::FilePath &file, const Utils::Text::Position &pos, bool marker = true)
        { m_textPosition = pos; m_fileName = file; m_needsMarker = marker; }
    Location(const StackFrame &frame, bool marker = true);
    Utils::FilePath fileName() const { return m_fileName; }
    QString functionName() const { return m_functionName; }
    QString from() const { return m_from; }
    Utils::Text::Position textPosition() const { return m_textPosition; }
    void setNeedsRaise(bool on) { m_needsRaise = on; }
    void setNeedsMarker(bool on) { m_needsMarker = on; }
    void setFileName(const Utils::FilePath &fileName) { m_fileName = fileName; }
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
    Utils::Text::Position m_textPosition;
    Utils::FilePath m_fileName;
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
    void addCompanionEngine(DebuggerEngine *engine);
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
    virtual void reexpandItems(
        const QSet<QString> &inames); // Called when items in tree need to be reexpanded.
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
    virtual void loadSymbols(const Utils::FilePath &moduleName);
    virtual void loadSymbolsForStack();
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const Utils::FilePath &moduleName);
    virtual void requestModuleSections(const Utils::FilePath &moduleName);

    virtual void reloadRegisters();
    virtual void reloadPeripheralRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();
    virtual void loadAdditionalQmlStack();
    virtual void reloadDebuggingHelpers();

    virtual void setRegisterValue(const QString &name, const QString &value);
    virtual void setPeripheralRegisterValue(quint64 address, quint64 value);
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
    PeripheralRegisterHandler *peripheralRegisterHandler() const;
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

    void notifyExitCode(int code);
    void notifyInferiorPid(const Utils::ProcessHandle &pid);
    qint64 inferiorPid() const;

    bool isReverseDebugging() const;
    void handleBeginOfRecordingReached();
    void handleRecordingFailed();
    void handleRecordReverse(bool);
    void handleReverseDirection(bool);

    // Convenience
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1) const;
    void showStatusMessage(const QString &msg, int timeout = -1) const;

    virtual void resetLocation();
    virtual void gotoLocation(const Internal::Location &location);
    void gotoCurrentLocation();
    virtual void quitDebugger(); // called when pressing the stop button
    void abortDebugger();
    void updateUi(bool isCurrentEngine);

    bool isPrimaryEngine() const;

    virtual bool canDisplayTooltip() const;

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
    void updateState();
    QString formatStartParameters() const;
    WatchTreeView *inspectorView();
    void updateLocalsWindow(bool showReturn);
    void raiseWatchersWindow();
    QString debuggerName() const;
    QString debuggerType() const;

    bool isRegistersWindowVisible() const;
    bool isPeripheralRegistersWindowVisible() const;
    bool isModulesWindowVisible() const;

    void openMemoryEditor();

    static void showModuleSymbols(const Utils::FilePath &moduleName, const QList<Symbol> &symbols);
    static void showModuleSections(const Utils::FilePath &moduleName, const QList<Section> &sections);

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
    void setDebuggerType(const QString &type);
    void notifyDebuggerProcessFinished(const Utils::ProcessResultData &resultData,
                                       const QString &backendName);

    virtual void setState(DebuggerState state, bool forced = false);

    void notifyInferiorShutdownFinished();

    void notifyEngineSpontaneousShutdown();
    void notifyEngineShutdownFinished();

    void notifyEngineIll();

    virtual void setupEngine() = 0;
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

    bool usesTerminal() const;
    qint64 applicationPid() const;
    qint64 applicationMainThreadId() const;
    void interruptTerminal() const;
    void kickoffTerminalProcess() const;

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

    ProjectExplorer::IDeviceConstPtr device() const;
    QList<DebuggerEngine *> companionEngines() const;

private:
    friend class DebuggerPluginPrivate;
    friend class DebuggerEnginePrivate;
    friend class LocationMark;
    friend class PeripheralRegisterHandler;
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
    LocationMark(DebuggerEngine *engine, const Utils::FilePath &file, int line);
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
