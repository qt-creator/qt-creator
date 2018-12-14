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

#include "debuggerengine.h"

#include "debuggerinternalconstants.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggericons.h"
#include "debuggerruncontrol.h"
#include "debuggertooltipmanager.h"

#include "analyzer/analyzermanager.h"
#include "breakhandler.h"
#include "disassembleragent.h"
#include "localsandexpressionswindow.h"
#include "logwindow.h"
#include "debuggermainwindow.h"
#include "enginemanager.h"
#include "memoryagent.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "sourcefileshandler.h"
#include "sourceutils.h"
#include "stackhandler.h"
#include "stackwindow.h"
#include "terminal.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "watchwindow.h"
#include "debugger/shared/peutils.h"
#include "console/console.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>

#include <utils/basetreeview.h>
#include <utils/fileinprojectfinder.h>
#include <utils/macroexpander.h>
#include <utils/processhandle.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/savedaction.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QHeaderView>
#include <QTextBlock>
#include <QTimer>
#include <QToolButton>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace Core;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

//#define WITH_BENCHMARK
#ifdef WITH_BENCHMARK
#include <valgrind/callgrind.h>
#endif

namespace Debugger {

QDebug operator<<(QDebug d, DebuggerState state)
{
    //return d << DebuggerEngine::stateName(state) << '(' << int(state) << ')';
    return d << DebuggerEngine::stateName(state);
}

QDebug operator<<(QDebug str, const DebuggerRunParameters &sp)
{
    QDebug nospace = str.nospace();
    nospace << "executable=" << sp.inferior.executable
            << " coreFile=" << sp.coreFile
            << " processArgs=" << sp.inferior.commandLineArguments
            << " inferior environment=<" << sp.inferior.environment.size() << " variables>"
            << " debugger environment=<" << sp.debugger.environment.size() << " variables>"
            << " workingDir=" << sp.inferior.workingDirectory
            << " attachPID=" << sp.attachPID.pid()
            << " remoteChannel=" << sp.remoteChannel
            << " abi=" << sp.toolChainAbi.toString() << '\n';
    return str;
}

namespace Internal {

static bool debuggerActionsEnabledHelper(DebuggerState state)
{
    switch (state) {
    case InferiorRunOk:
    case InferiorUnrunnable:
    case InferiorStopOk:
        return true;
    case InferiorStopRequested:
    case InferiorRunRequested:
    case InferiorRunFailed:
    case DebuggerNotReady:
    case EngineSetupRequested:
    case EngineSetupOk:
    case EngineSetupFailed:
    case EngineRunRequested:
    case EngineRunFailed:
    case InferiorStopFailed:
    case InferiorShutdownRequested:
    case InferiorShutdownFinished:
    case EngineShutdownRequested:
    case EngineShutdownFinished:
    case DebuggerFinished:
        return false;
    }
    return false;
}

Location::Location(const StackFrame &frame, bool marker)
{
    m_fileName = frame.file;
    m_lineNumber = frame.line;
    m_needsMarker = marker;
    m_functionName = frame.function;
    m_hasDebugInfo = frame.isUsable();
    m_address = frame.address;
    m_from = frame.module;
}


LocationMark::LocationMark(DebuggerEngine *engine, const FileName &file, int line)
    : TextMark(file, line, Constants::TEXT_MARK_CATEGORY_LOCATION), m_engine(engine)
{
    setPriority(TextMark::HighPriority);
    updateIcon();
}

void LocationMark::updateIcon()
{
    const Icon *icon = &Icons::WATCHPOINT;
    if (m_engine && EngineManager::currentEngine() == m_engine)
        icon = m_engine->isReverseDebugging() ? &Icons::REVERSE_LOCATION : &Icons::LOCATION;
    setIcon(icon->icon());
    updateMarker();
}

bool LocationMark::isDraggable() const
{
    return m_engine && m_engine->hasCapability(JumpToLineCapability);
}

void LocationMark::dragToLine(int line)
{
    if (m_engine) {
        if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
            ContextData location = getLocationContext(textEditor->textDocument(), line);
            if (location.isValid())
                m_engine->executeJumpToLine(location);
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
// MemoryAgentSet
//
//////////////////////////////////////////////////////////////////////

class MemoryAgentSet
{
public:
    ~MemoryAgentSet()
    {
        qDeleteAll(m_agents);
        m_agents.clear();
    }

    // Called by engine to create a new view.
    void createBinEditor(const MemoryViewSetupData &data, DebuggerEngine *engine)
    {
        auto agent = new MemoryAgent(data, engine);
        if (agent->isUsable()) {
            m_agents.append(agent);
        } else {
            delete agent;
            AsynchronousMessageBox::warning(
                        DebuggerEngine::tr("No Memory Viewer Available"),
                        DebuggerEngine::tr("The memory contents cannot be shown as no viewer plugin "
                                           "for binary data has been loaded."));
        }
    }

    // On stack frame completed and on request.
    void updateContents()
    {
        foreach (MemoryAgent *agent, m_agents) {
            if (agent)
                agent->updateContents();
        }
    }

    void handleDebuggerFinished()
    {
        foreach (MemoryAgent *agent, m_agents) {
            if (agent)
                agent->setFinished(); // Prevent triggering updates, etc.
        }
    }

private:
    QList<MemoryAgent *> m_agents;
};



//////////////////////////////////////////////////////////////////////
//
// DebuggerEnginePrivate
//
//////////////////////////////////////////////////////////////////////

class DebuggerEnginePrivate : public QObject
{
    Q_OBJECT

public:
    DebuggerEnginePrivate(DebuggerEngine *engine)
        : m_engine(engine),
          m_breakHandler(engine),
          m_modulesHandler(engine),
          m_registerHandler(engine),
          m_sourceFilesHandler(engine),
          m_stackHandler(engine),
          m_threadsHandler(engine),
          m_watchHandler(engine),
          m_disassemblerAgent(engine),
          m_toolTipManager(engine)
    {
        m_logWindow = new LogWindow(m_engine); // Needed before start()
        m_logWindow->setObjectName(QLatin1String(DOCKWIDGET_OUTPUT));
        m_debuggerName = DebuggerEngine::tr("Debugger");

        connect(action(EnableReverseDebugging), &SavedAction::valueChanged,
                this, [this] { updateState(true); });
        static int contextCount = 0;
        m_context = Context(Id("Debugger.Engine.").withSuffix(++contextCount));

        ActionManager::registerAction(&m_continueAction, Constants::CONTINUE, m_context);
        ActionManager::registerAction(&m_exitAction, Constants::STOP, m_context);
        ActionManager::registerAction(&m_interruptAction, Constants::INTERRUPT, m_context);
        ActionManager::registerAction(&m_abortAction, Constants::ABORT, m_context);
        ActionManager::registerAction(&m_stepOverAction, Constants::NEXT, m_context);
        ActionManager::registerAction(&m_stepIntoAction, Constants::STEP, m_context);
        ActionManager::registerAction(&m_stepOutAction, Constants::STEPOUT, m_context);
        ActionManager::registerAction(&m_runToLineAction, Constants::RUNTOLINE, m_context);
        ActionManager::registerAction(&m_runToSelectedFunctionAction, Constants::RUNTOSELECTEDFUNCTION, m_context);
        ActionManager::registerAction(&m_jumpToLineAction, Constants::JUMPTOLINE, m_context);
        ActionManager::registerAction(&m_returnFromFunctionAction, Constants::RETURNFROMFUNCTION, m_context);
        ActionManager::registerAction(&m_detachAction, Constants::DETACH, m_context);
        ActionManager::registerAction(&m_resetAction, Constants::RESET, m_context);
        ActionManager::registerAction(&m_watchAction, Constants::WATCH, m_context);
        ActionManager::registerAction(&m_operateByInstructionAction, Constants::OPERATE_BY_INSTRUCTION, m_context);
        ActionManager::registerAction(&m_openMemoryEditorAction, Constants::OPEN_MEMORY_EDITOR, m_context);
        ActionManager::registerAction(&m_frameUpAction, Constants::FRAME_UP, m_context);
        ActionManager::registerAction(&m_frameDownAction, Constants::FRAME_DOWN, m_context);
    }

    ~DebuggerEnginePrivate()
    {
        ActionManager::unregisterAction(&m_continueAction, Constants::CONTINUE);
        ActionManager::unregisterAction(&m_exitAction, Constants::STOP);
        ActionManager::unregisterAction(&m_interruptAction, Constants::INTERRUPT);
        ActionManager::unregisterAction(&m_abortAction, Constants::ABORT);
        ActionManager::unregisterAction(&m_stepOverAction, Constants::NEXT);
        ActionManager::unregisterAction(&m_stepIntoAction, Constants::STEP);
        ActionManager::unregisterAction(&m_stepOutAction, Constants::STEPOUT);
        ActionManager::unregisterAction(&m_runToLineAction, Constants::RUNTOLINE);
        ActionManager::unregisterAction(&m_runToSelectedFunctionAction, Constants::RUNTOSELECTEDFUNCTION);
        ActionManager::unregisterAction(&m_jumpToLineAction, Constants::JUMPTOLINE);
        ActionManager::unregisterAction(&m_returnFromFunctionAction, Constants::RETURNFROMFUNCTION);
        ActionManager::unregisterAction(&m_detachAction, Constants::DETACH);
        ActionManager::unregisterAction(&m_resetAction, Constants::RESET);
        ActionManager::unregisterAction(&m_watchAction, Constants::WATCH);
        ActionManager::unregisterAction(&m_operateByInstructionAction, Constants::OPERATE_BY_INSTRUCTION);
        ActionManager::unregisterAction(&m_openMemoryEditorAction, Constants::OPEN_MEMORY_EDITOR);
        ActionManager::unregisterAction(&m_frameUpAction, Constants::FRAME_UP);
        ActionManager::unregisterAction(&m_frameDownAction, Constants::FRAME_DOWN);
        destroyPerspective();

        delete m_logWindow;
        delete m_breakWindow;
        delete m_returnWindow;
        delete m_localsWindow;
        delete m_watchersWindow;
        delete m_inspectorWindow;
        delete m_registerWindow;
        delete m_modulesWindow;
        delete m_sourceFilesWindow;
        delete m_stackWindow;
        delete m_threadsWindow;

        delete m_breakView;
        delete m_returnView;
        delete m_localsView;
        delete m_watchersView;
        delete m_inspectorView;
        delete m_registerView;
        delete m_modulesView;
        delete m_sourceFilesView;
        delete m_stackView;
        delete m_threadsView;
    }

    void updateActionToolTips()
    {
        // update tooltips that are visible on the button in the mode selector
        const QString displayName = m_engine->displayName();
        m_continueAction.setToolTip(tr("Continue %1").arg(displayName));
        m_interruptAction.setToolTip(tr("Interrupt %1").arg(displayName));
    }

    void setupViews();

    void destroyPerspective()
    {
        if (!m_perspective)
            return;

        delete m_perspective;
        m_perspective = nullptr;

        EngineManager::unregisterEngine(m_engine);

        // Give up ownership on claimed breakpoints.
        m_breakHandler.releaseAllBreakpoints();
        m_toolTipManager.deregisterEngine();
        m_memoryAgents.handleDebuggerFinished();

        setBusyCursor(false);
    }

    void updateReturnViewHeader(int section, int, int newSize)
    {
        if (m_perspective && m_returnView && m_returnView->header())
            m_returnView->header()->resizeSection(section, newSize);
    }

    void doShutdownEngine()
    {
        m_engine->setState(EngineShutdownRequested);
        m_engine->startDying();
        m_engine->showMessage("CALL: SHUTDOWN ENGINE");
        m_engine->shutdownEngine();
    }

    void doShutdownInferior()
    {
        m_engine->setState(InferiorShutdownRequested);
        //QTC_ASSERT(isMasterEngine(), return);
        resetLocation();
        m_engine->showMessage("CALL: SHUTDOWN INFERIOR");
        m_engine->shutdownInferior();
    }

    void doFinishDebugger()
    {
        QTC_ASSERT(m_state == EngineShutdownFinished, qDebug() << m_state);
        resetLocation();
        m_progress.setProgressValue(1000);
        m_progress.reportFinished();
        m_modulesHandler.removeAll();
        m_stackHandler.removeAll();
        m_threadsHandler.removeAll();
        m_watchHandler.cleanup();
        m_engine->showMessage(tr("Debugger finished."), StatusBar);
        m_engine->setState(DebuggerFinished); // Also destroys views.
        if (boolSetting(SwitchModeOnExit))
            EngineManager::deactivateDebugMode();
    }

    void scheduleResetLocation()
    {
        m_stackHandler.scheduleResetLocation();
        m_watchHandler.scheduleResetLocation();
        m_disassemblerAgent.scheduleResetLocation();
        m_locationTimer.setSingleShot(true);
        m_locationTimer.start(80);
    }

    void resetLocation()
    {
        m_lookupRequests.clear();
        m_locationTimer.stop();
        m_locationMark.reset();
        m_stackHandler.resetLocation();
        m_watchHandler.resetLocation();
        m_disassemblerAgent.resetLocation();
        m_toolTipManager.resetLocation();
    }

public:
    void setInitialActionStates();
    void setBusyCursor(bool on);
    void cleanupViews();
    void updateState(bool alsoUpdateCompanion);
    void updateReverseActions();

    DebuggerEngine *m_engine = nullptr; // Not owned.
    QString m_runId;
    QPointer<RunConfiguration> m_runConfiguration;  // Not owned.
    QString m_debuggerName;
    Perspective *m_perspective = nullptr;
    DebuggerRunParameters m_runParameters;
    IDevice::ConstPtr m_device;

    QPointer<DebuggerEngine> m_companionEngine;
    bool m_isPrimaryEngine = true;

    // The current state.
    DebuggerState m_state = DebuggerNotReady;

//    Terminal m_terminal;
    ProcessHandle m_inferiorPid;

    BreakHandler m_breakHandler;
    ModulesHandler m_modulesHandler;
    RegisterHandler m_registerHandler;
    SourceFilesHandler m_sourceFilesHandler;
    StackHandler m_stackHandler;
    ThreadsHandler m_threadsHandler;
    WatchHandler m_watchHandler;
    QFutureInterface<void> m_progress;

    DisassemblerAgent m_disassemblerAgent;
    MemoryAgentSet m_memoryAgents;
    QScopedPointer<LocationMark> m_locationMark;
    QTimer m_locationTimer;

    Utils::FileInProjectFinder m_fileFinder;
    QString m_qtNamespace;

    // Safety net to avoid infinite lookups.
    QSet<QString> m_lookupRequests; // FIXME: Integrate properly.
    QPointer<QWidget> m_alertBox;

    QPointer<BaseTreeView> m_breakView;
    QPointer<BaseTreeView> m_returnView;
    QPointer<BaseTreeView> m_localsView;
    QPointer<BaseTreeView> m_watchersView;
    QPointer<WatchTreeView> m_inspectorView;
    QPointer<BaseTreeView> m_registerView;
    QPointer<BaseTreeView> m_modulesView;
    QPointer<BaseTreeView> m_sourceFilesView;
    QPointer<BaseTreeView> m_stackView;
    QPointer<BaseTreeView> m_threadsView;
    QPointer<QWidget> m_breakWindow;
    QPointer<QWidget> m_returnWindow;
    QPointer<QWidget> m_localsWindow;
    QPointer<QWidget> m_watchersWindow;
    QPointer<QWidget> m_inspectorWindow;
    QPointer<QWidget> m_registerWindow;
    QPointer<QWidget> m_modulesWindow;
    QPointer<QWidget> m_sourceFilesWindow;
    QPointer<QWidget> m_stackWindow;
    QPointer<QWidget> m_threadsWindow;
    QPointer<LogWindow> m_logWindow;
    QPointer<LocalsAndInspectorWindow> m_localsAndInspectorWindow;

    QPointer<QLabel> m_threadLabel;

    bool m_busy = false;
    bool m_isDying = false;

    QAction m_debugWithoutDeployAction;
    QAction m_attachToQmlPortAction;
    QAction m_attachToRemoteServerAction;
    QAction m_startRemoteCdbAction;
    QAction m_attachToCoreAction;
    QAction m_detachAction;
    OptionalAction m_continueAction{tr("Continue")};
    QAction m_exitAction{tr("Stop Debugger")}; // On application output button if "Stop" is possible
    OptionalAction m_interruptAction{tr("Interrupt")}; // On the fat debug button if "Pause" is possible
    QAction m_abortAction{tr("Abort Debugging")};
    QAction m_stepIntoAction{tr("Step Into")};
    QAction m_stepOutAction{tr("Step Out")};
    QAction m_runToLineAction{tr("Run to Line")}; // In the debug menu
    QAction m_runToSelectedFunctionAction{tr("Run to Selected Function")};
    QAction m_jumpToLineAction{tr("Jump to Line")};
    QAction m_frameUpAction{QCoreApplication::translate("Debugger::Internal::DebuggerPluginPrivate",
                                                        "Move to Calling Frame")};
    QAction m_frameDownAction{QCoreApplication::translate("Debugger::Internal::DebuggerPluginPrivate",
                                                          "Move to Called Frame")};
    QAction m_openMemoryEditorAction{QCoreApplication::translate("Debugger::Internal::DebuggerPluginPrivate",
                                                                 "Memory...")};

    // In the Debug menu.
    QAction m_returnFromFunctionAction{tr("Immediately Return From Inner Function")};
    QAction m_stepOverAction{tr("Step Over")};
    QAction m_watchAction{tr("Add Expression Evaluator")};
    QAction m_breakAction{tr("Toggle Breakpoint")};
    QAction m_resetAction{tr("Restart Debugging")};
    OptionalAction m_operateByInstructionAction{tr("Operate by Instruction")};
    QAction m_recordForReverseOperationAction{tr("Record Information to Allow Reversal of Direction")};
    OptionalAction m_operateInReverseDirectionAction{tr("Reverse Direction")};
    OptionalAction m_snapshotAction{tr("Take Snapshot of Process State")};

    QPointer<TerminalRunner> m_terminalRunner;
    DebuggerToolTipManager m_toolTipManager;
    Context m_context;
};

void DebuggerEnginePrivate::setupViews()
{
    const DebuggerRunParameters &rp = m_runParameters;

    QTC_CHECK(!m_perspective);

    m_perspective = new Perspective("Debugger.Perspective." + m_runId,
                                    m_engine->displayName(),
                                    Debugger::Constants::PRESET_PERSPECTIVE_ID,
                                    m_debuggerName);
    m_perspective->setShouldPersistChecker([this] {
        return EngineManager::isLastOf(m_debuggerName);
    });

    m_progress.setProgressRange(0, 1000);
    FutureProgress *fp = ProgressManager::addTask(m_progress.future(),
        tr("Launching Debugger"), "Debugger.Launcher");
    connect(fp, &FutureProgress::canceled, m_engine, &DebuggerEngine::quitDebugger);
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    m_progress.reportStarted();

    m_inferiorPid = rp.attachPID.isValid() ? rp.attachPID : ProcessHandle();
//    if (m_inferiorPid.isValid())
//        m_runControl->setApplicationProcessHandle(m_inferiorPid);

    m_operateByInstructionAction.setEnabled(true);
    m_operateByInstructionAction.setVisible(m_engine->hasCapability(DisassemblerCapability));
    m_operateByInstructionAction.setIcon(Debugger::Icons::SINGLE_INSTRUCTION_MODE.icon());
    m_operateByInstructionAction.setCheckable(true);
    m_operateByInstructionAction.setChecked(false);
    m_operateByInstructionAction.setToolTip("<p>" + tr("Switches the debugger to instruction-wise "
        "operation mode. In this mode, stepping operates on single "
        "instructions and the source location view also shows the "
        "disassembled instructions."));
    m_operateByInstructionAction.setIconVisibleInMenu(false);
    connect(&m_operateByInstructionAction, &QAction::triggered,
            m_engine, &DebuggerEngine::operateByInstructionTriggered);

    m_frameDownAction.setEnabled(true);
    connect(&m_frameDownAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleFrameDown);

    m_frameUpAction.setEnabled(true);
    connect(&m_frameUpAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleFrameUp);

    m_openMemoryEditorAction.setEnabled(true);
    m_openMemoryEditorAction.setVisible(m_engine->hasCapability(ShowMemoryCapability));
    connect(&m_openMemoryEditorAction, &QAction::triggered,
            m_engine, &DebuggerEngine::openMemoryEditor);

    QTC_ASSERT(m_state == DebuggerNotReady || m_state == DebuggerFinished, qDebug() << m_state);
    m_progress.setProgressValue(200);

//    m_terminal.setup();
//    if (m_terminal.isUsable()) {
//        connect(&m_terminal, &Terminal::stdOutReady, [this](const QString &msg) {
//            m_engine->showMessage(msg, Utils::StdOutFormatSameLine);
//        });
//        connect(&m_terminal, &Terminal::stdErrReady, [this](const QString &msg) {
//            m_engine->showMessage(msg, Utils::StdErrFormatSameLine);
//        });
//        connect(&m_terminal, &Terminal::error, [this](const QString &msg) {
//            m_engine->showMessage(msg, Utils::ErrorMessageFormat);
//        });
//    }

    connect(&m_locationTimer, &QTimer::timeout,
            this, &DebuggerEnginePrivate::resetLocation);

    QSettings *settings = ICore::settings();

    m_modulesView = new BaseTreeView;
    m_modulesView->setModel(m_modulesHandler.model());
    m_modulesView->setSortingEnabled(true);
    m_modulesView->setSettings(settings, "Debugger.ModulesView");
    connect(m_modulesView, &BaseTreeView::aboutToShow,
            m_engine, &DebuggerEngine::reloadModules,
            Qt::QueuedConnection);
    m_modulesWindow = addSearch(m_modulesView);
    m_modulesWindow->setObjectName(DOCKWIDGET_MODULES);
    m_modulesWindow->setWindowTitle(tr("&Modules"));

    m_registerView = new BaseTreeView;
    m_registerView->setModel(m_registerHandler.model());
    m_registerView->setRootIsDecorated(true);
    m_registerView->setSettings(settings, "Debugger.RegisterView");
    connect(m_registerView, &BaseTreeView::aboutToShow,
            m_engine, &DebuggerEngine::reloadRegisters,
            Qt::QueuedConnection);
    m_registerWindow = addSearch(m_registerView);
    m_registerWindow->setObjectName(DOCKWIDGET_REGISTER);
    m_registerWindow->setWindowTitle(tr("Reg&isters"));

    m_stackView = new BaseTreeView;
    m_stackView->setModel(m_stackHandler.model());
    m_stackView->setSettings(settings, "Debugger.StackView");
    m_stackView->setIconSize(QSize(10, 10));
    m_stackWindow = addSearch(m_stackView);
    m_stackWindow->setObjectName(DOCKWIDGET_STACK);
    m_stackWindow->setWindowTitle(tr("&Stack"));

    m_sourceFilesView = new BaseTreeView;
    m_sourceFilesView->setModel(m_sourceFilesHandler.model());
    m_sourceFilesView->setSortingEnabled(true);
    m_sourceFilesView->setSettings(settings, "Debugger.SourceFilesView");
    connect(m_sourceFilesView, &BaseTreeView::aboutToShow,
            m_engine, &DebuggerEngine::reloadSourceFiles,
            Qt::QueuedConnection);
    m_sourceFilesWindow = addSearch(m_sourceFilesView);
    m_sourceFilesWindow->setObjectName(DOCKWIDGET_SOURCE_FILES);
    m_sourceFilesWindow->setWindowTitle(tr("Source Files"));

    m_threadsView = new BaseTreeView;
    m_threadsView->setModel(m_threadsHandler.model());
    m_threadsView->setSortingEnabled(true);
    m_threadsView->setSettings(settings, "Debugger.ThreadsView");
    m_threadsView->setIconSize(QSize(10, 10));
    m_threadsWindow = addSearch(m_threadsView);
    m_threadsWindow->setObjectName(DOCKWIDGET_THREADS);
    m_threadsWindow->setWindowTitle(tr("&Threads"));

    m_returnView = new WatchTreeView{ReturnType};
    m_returnView->setModel(m_watchHandler.model());
    m_returnWindow = addSearch(m_returnView);
    m_returnWindow->setObjectName("CppDebugReturn");
    m_returnWindow->setWindowTitle(tr("Locals"));
    m_returnWindow->setVisible(false);

    m_localsView = new WatchTreeView{LocalsType};
    m_localsView->setModel(m_watchHandler.model());
    m_localsView->setSettings(settings, "Debugger.LocalsView");
    m_localsWindow = addSearch(m_localsView);
    m_localsWindow->setObjectName("CppDebugLocals");
    m_localsWindow->setWindowTitle(tr("Locals"));

    m_inspectorView = new WatchTreeView{InspectType};
    m_inspectorView->setModel(m_watchHandler.model());
    m_inspectorView->setSettings(settings, "Debugger.LocalsView"); // sic! same as locals view.
    m_inspectorWindow = addSearch(m_inspectorView);
    m_inspectorWindow->setObjectName("Inspector");
    m_inspectorWindow->setWindowTitle(tr("Locals"));

    m_watchersView = new WatchTreeView{WatchersType};
    m_watchersView->setModel(m_watchHandler.model());
    m_watchersView->setSettings(settings, "Debugger.WatchersView");
    m_watchersWindow = addSearch(m_watchersView);
    m_watchersWindow->setObjectName("CppDebugWatchers");
    m_watchersWindow->setWindowTitle(tr("&Expressions"));

    m_localsAndInspectorWindow = new LocalsAndInspectorWindow(
                m_localsWindow, m_inspectorWindow, m_returnWindow);
    m_localsAndInspectorWindow->setObjectName(DOCKWIDGET_LOCALS_AND_INSPECTOR);
    m_localsAndInspectorWindow->setWindowTitle(m_localsWindow->windowTitle());

    // Locals
    connect(m_localsView->header(), &QHeaderView::sectionResized,
            this, &DebuggerEnginePrivate::updateReturnViewHeader, Qt::QueuedConnection);

    m_breakView = new BaseTreeView;
    m_breakView->setIconSize(QSize(10, 10));
    m_breakView->setWindowIcon(Icons::BREAKPOINTS.icon());
    m_breakView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(action(UseAddressInBreakpointsView), &QAction::toggled,
            this, [this](bool on) { m_breakView->setColumnHidden(BreakpointAddressColumn, !on); });
    m_breakView->setSettings(settings, "Debugger.BreakWindow");
    m_breakView->setModel(m_breakHandler.model());
    m_breakView->setRootIsDecorated(true);
    m_breakWindow = addSearch(m_breakView);
    m_breakWindow->setObjectName(DOCKWIDGET_BREAK);
    m_breakWindow->setWindowTitle(tr("&Breakpoints"));

    m_perspective->useSubPerspectiveSwitcher(EngineManager::engineChooser());

    m_perspective->addToolBarAction(&m_continueAction);
    m_perspective->addToolBarAction(&m_interruptAction);

    m_perspective->addToolBarAction(&m_exitAction);
    m_perspective->addToolBarAction(&m_stepOverAction);
    m_perspective->addToolBarAction(&m_stepIntoAction);
    m_perspective->addToolBarAction(&m_stepOutAction);
    m_perspective->addToolBarAction(&m_resetAction);
    m_perspective->addToolBarAction(&m_operateByInstructionAction);

    connect(&m_detachAction, &QAction::triggered, m_engine, &DebuggerEngine::handleExecDetach);

    m_continueAction.setIcon(Icons::DEBUG_CONTINUE_SMALL_TOOLBAR.icon());
    connect(&m_continueAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecContinue);

    m_exitAction.setIcon(Icons::DEBUG_EXIT_SMALL_TOOLBAR.icon());
    connect(&m_exitAction, &QAction::triggered,
            m_engine, &DebuggerEngine::requestRunControlStop);

    m_interruptAction.setIcon(Icons::DEBUG_INTERRUPT_SMALL_TOOLBAR.icon());
    connect(&m_interruptAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecInterrupt);

    m_abortAction.setToolTip(tr("Aborts debugging and resets the debugger to the initial state."));
    connect(&m_abortAction, &QAction::triggered,
            m_engine, &DebuggerEngine::abortDebugger);

    m_resetAction.setToolTip(tr("Restarts the debugging session."));
    m_resetAction.setIcon(Icons::RESTART_TOOLBAR.icon());
    connect(&m_resetAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleReset);

    m_stepOverAction.setIcon(Icons::STEP_OVER_TOOLBAR.icon());
    connect(&m_stepOverAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecStepOver);

    m_stepIntoAction.setIcon(Icons::STEP_INTO_TOOLBAR.icon());
    connect(&m_stepIntoAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecStepIn);

    m_stepOutAction.setIcon(Icons::STEP_OUT_TOOLBAR.icon());
    connect(&m_stepOutAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecStepOut);

    connect(&m_runToLineAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecRunToLine);

    connect(&m_runToSelectedFunctionAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecRunToSelectedFunction);

    connect(&m_returnFromFunctionAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecReturn);

    connect(&m_jumpToLineAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleExecJumpToLine);

    connect(&m_watchAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleAddToWatchWindow);

    m_perspective->addToolBarAction(&m_recordForReverseOperationAction);
    connect(&m_recordForReverseOperationAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleRecordReverse);

    m_perspective->addToolBarAction(&m_operateInReverseDirectionAction);
    connect(&m_operateInReverseDirectionAction, &QAction::triggered,
            m_engine, &DebuggerEngine::handleReverseDirection);

    m_perspective->addToolBarAction(&m_snapshotAction);
    connect(&m_snapshotAction, &QAction::triggered,
            m_engine, &DebuggerEngine::createSnapshot);

    m_perspective->addToolbarSeparator();

    m_threadLabel = new QLabel(tr("Threads:"));
    m_perspective->addToolBarWidget(m_threadLabel);
    m_perspective->addToolBarWidget(m_threadsHandler.threadSwitcher());

    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, [this](const FontSettings &settings) {
        if (!boolSetting(FontSizeFollowsEditor))
            return;
        const qreal size = settings.fontZoom() * settings.fontSize() / 100.;
        QFont font = m_breakWindow->font();
        font.setPointSizeF(size);
        m_breakWindow->setFont(font);
        m_logWindow->setFont(font);
        m_localsWindow->setFont(font);
        m_modulesWindow->setFont(font);
        //m_consoleWindow->setFont(font);
        m_registerWindow->setFont(font);
        m_returnWindow->setFont(font);
        m_sourceFilesWindow->setFont(font);
        m_stackWindow->setFont(font);
        m_threadsWindow->setFont(font);
        m_watchersWindow->setFont(font);
        m_inspectorWindow->setFont(font);
    });

    m_perspective->addWindow(m_stackWindow, Perspective::SplitVertical, nullptr);
    m_perspective->addWindow(m_breakWindow, Perspective::SplitHorizontal, m_stackWindow);
    m_perspective->addWindow(m_threadsWindow, Perspective::AddToTab, m_breakWindow,false);
    m_perspective->addWindow(m_modulesWindow, Perspective::AddToTab, m_threadsWindow, false);
    m_perspective->addWindow(m_sourceFilesWindow, Perspective::AddToTab, m_modulesWindow, false);
    m_perspective->addWindow(m_localsAndInspectorWindow, Perspective::AddToTab, nullptr, true, Qt::RightDockWidgetArea);
    m_perspective->addWindow(m_watchersWindow, Perspective::SplitVertical, m_localsAndInspectorWindow, true, Qt::RightDockWidgetArea);
    m_perspective->addWindow(m_registerWindow, Perspective::AddToTab, m_localsAndInspectorWindow, false, Qt::RightDockWidgetArea);
    m_perspective->addWindow(m_logWindow, Perspective::AddToTab, nullptr, false, Qt::TopDockWidgetArea);

    m_perspective->select();
    m_watchHandler.loadSessionDataForEngine();
}

//////////////////////////////////////////////////////////////////////
//
// DebuggerEngine
//
//////////////////////////////////////////////////////////////////////

DebuggerEngine::DebuggerEngine()
  : d(new DebuggerEnginePrivate(this))
{
    updateState(false);
}

DebuggerEngine::~DebuggerEngine()
{
//    EngineManager::unregisterEngine(this);
    delete d;
}

void DebuggerEngine::setDebuggerName(const QString &name)
{
    d->m_debuggerName = name;
    d->updateActionToolTips();
}

QString DebuggerEngine::debuggerName() const
{
    return d->m_debuggerName;
}

QString DebuggerEngine::stateName(int s)
{
#    define SN(x) case x: return QLatin1String(#x);
    switch (s) {
        SN(DebuggerNotReady)
        SN(EngineSetupRequested)
        SN(EngineSetupOk)
        SN(EngineSetupFailed)
        SN(EngineRunFailed)
        SN(EngineRunRequested)
        SN(InferiorRunRequested)
        SN(InferiorRunOk)
        SN(InferiorRunFailed)
        SN(InferiorUnrunnable)
        SN(InferiorStopRequested)
        SN(InferiorStopOk)
        SN(InferiorStopFailed)
        SN(InferiorShutdownRequested)
        SN(InferiorShutdownFinished)
        SN(EngineShutdownRequested)
        SN(EngineShutdownFinished)
        SN(DebuggerFinished)
    }
    return QLatin1String("<unknown>");
#    undef SN
}

void DebuggerEngine::showStatusMessage(const QString &msg, int timeout) const
{
    showMessage(msg, StatusBar, timeout);
}

void DebuggerEngine::updateLocalsWindow(bool showReturn)
{
    d->m_returnWindow->setVisible(showReturn);
    d->m_localsView->resizeColumns();
}

bool DebuggerEngine::isRegistersWindowVisible() const
{
    return d->m_registerWindow->isVisible();
}

bool DebuggerEngine::isModulesWindowVisible() const
{
    return d->m_modulesWindow->isVisible();
}

void DebuggerEngine::frameUp()
{
    int currentIndex = stackHandler()->currentIndex();
    activateFrame(qMin(currentIndex + 1, stackHandler()->stackSize() - 1));
}

void DebuggerEngine::frameDown()
{
    int currentIndex = stackHandler()->currentIndex();
    activateFrame(qMax(currentIndex - 1, 0));
}

void DebuggerEngine::doUpdateLocals(const UpdateParameters &)
{
}

ModulesHandler *DebuggerEngine::modulesHandler() const
{
    return &d->m_modulesHandler;
}

RegisterHandler *DebuggerEngine::registerHandler() const
{
    return &d->m_registerHandler;
}

StackHandler *DebuggerEngine::stackHandler() const
{
    return &d->m_stackHandler;
}

ThreadsHandler *DebuggerEngine::threadsHandler() const
{
    return &d->m_threadsHandler;
}

WatchHandler *DebuggerEngine::watchHandler() const
{
    return &d->m_watchHandler;
}

SourceFilesHandler *DebuggerEngine::sourceFilesHandler() const
{
    return &d->m_sourceFilesHandler;
}

BreakHandler *DebuggerEngine::breakHandler() const
{
    return &d->m_breakHandler;
}

LogWindow *DebuggerEngine::logWindow() const
{
    return d->m_logWindow;
}

DisassemblerAgent *DebuggerEngine::disassemblerAgent() const
{
    return &d->m_disassemblerAgent;
}

void DebuggerEngine::fetchMemory(MemoryAgent *, quint64 addr, quint64 length)
{
    Q_UNUSED(addr);
    Q_UNUSED(length);
}

void DebuggerEngine::changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data)
{
    Q_UNUSED(addr);
    Q_UNUSED(data);
}

void DebuggerEngine::setRegisterValue(const QString &name, const QString &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

void DebuggerEngine::setRunParameters(const DebuggerRunParameters &runParameters)
{
    d->m_runParameters = runParameters;
    d->updateActionToolTips();
}

void DebuggerEngine::setRunId(const QString &id)
{
    d->m_runId = id;
}

void DebuggerEngine::setRunTool(DebuggerRunTool *runTool)
{
    RunControl *runControl = runTool->runControl();
    d->m_runConfiguration = runControl->runConfiguration();
    d->m_device = runControl->device();
    if (!d->m_device)
        d->m_device = d->m_runParameters.inferior.device;
    d->m_terminalRunner = runTool->terminalRunner();

    validateRunParameters(d->m_runParameters);

    d->setupViews();
}

void DebuggerEngine::start()
{
    EngineManager::registerEngine(this);
    d->m_watchHandler.resetWatchers();
    d->setInitialActionStates();
    setState(EngineSetupRequested);
    showMessage("CALL: SETUP ENGINE");
    setupEngine();
}

void DebuggerEngine::resetLocation()
{
    // Do it after some delay to avoid flicker.
    d->scheduleResetLocation();
}

void DebuggerEngine::gotoLocation(const Location &loc)
{
     d->resetLocation();

    if (loc.canBeDisassembled()
            && ((hasCapability(OperateByInstructionCapability) && operatesByInstruction())
                || !loc.hasDebugInfo()) )
    {
        d->m_disassemblerAgent.setLocation(loc);
        return;
    }

    if (loc.fileName().isEmpty()) {
        showMessage("CANNOT GO TO THIS LOCATION");
        return;
    }
    const QString file = QDir::cleanPath(loc.fileName());
    const int line = loc.lineNumber();
    bool newEditor = false;
    IEditor *editor = EditorManager::openEditor(
                file, Id(),
                EditorManager::IgnoreNavigationHistory | EditorManager::DoNotSwitchToDesignMode,
                &newEditor);
    QTC_ASSERT(editor, return); // Unreadable file?

    editor->gotoLine(line, 0, !boolSetting(StationaryEditorWhileStepping));

    if (newEditor)
        editor->document()->setProperty(Constants::OPENED_BY_DEBUGGER, true);

    if (loc.needsMarker()) {
        d->m_locationMark.reset(new LocationMark(this, FileName::fromString(file), line));
        d->m_locationMark->setToolTip(tr("Current debugger location of %1").arg(displayName()));
    }
}

void DebuggerEngine::gotoCurrentLocation()
{
    if (d->m_state == InferiorStopOk || d->m_state == InferiorUnrunnable) {
        int top = stackHandler()->currentIndex();
        if (top >= 0)
            gotoLocation(stackHandler()->currentFrame());
    }
}

const DebuggerRunParameters &DebuggerEngine::runParameters() const
{
    return d->m_runParameters;
}

IDevice::ConstPtr DebuggerEngine::device() const
{
    return d->m_device;
}

DebuggerEngine *DebuggerEngine::companionEngine() const
{
    return d->m_companionEngine;
}

DebuggerState DebuggerEngine::state() const
{
    return d->m_state;
}

void DebuggerEngine::abortDebugger()
{
    resetLocation();
    if (!d->m_isDying) {
        // Be friendly the first time. This will change targetState().
        showMessage("ABORTING DEBUGGER. FIRST TIME.");
        quitDebugger();
    } else {
        // We already tried. Try harder.
        showMessage("ABORTING DEBUGGER. SECOND TIME.");
        abortDebuggerProcess();
        emit requestRunControlFinish();
    }
}

void DebuggerEngine::updateUi(bool isCurrentEngine)
{
    updateState(false);
    if (isCurrentEngine) {
        gotoCurrentLocation();
    } else {
        d->m_locationMark.reset();
        d->m_disassemblerAgent.resetLocation();
    }
}

static bool isAllowedTransition(DebuggerState from, DebuggerState to)
{
    switch (from) {
    case DebuggerNotReady:
        return to == EngineSetupRequested;

    case EngineSetupRequested:
        return to == EngineSetupOk || to == EngineSetupFailed;
    case EngineSetupFailed:
        // In is the engine's task to go into a proper "Shutdown"
        // state before calling notifyEngineSetupFailed
        return to == DebuggerFinished;
    case EngineSetupOk:
        return to == EngineRunRequested || to == EngineShutdownRequested;

    case EngineRunRequested:
        return to == EngineRunFailed
            || to == InferiorRunRequested
            || to == InferiorRunOk
            || to == InferiorStopOk
            || to == InferiorUnrunnable;
    case EngineRunFailed:
        return to == EngineShutdownRequested;

    case InferiorRunRequested:
        return to == InferiorRunOk || to == InferiorRunFailed;
    case InferiorRunFailed:
        return to == InferiorStopOk;
    case InferiorRunOk:
        return to == InferiorStopRequested
            || to == InferiorStopOk             // A spontaneous stop.
            || to == InferiorShutdownFinished;  // A spontaneous exit.

    case InferiorStopRequested:
        return to == InferiorStopOk || to == InferiorStopFailed;
    case InferiorStopOk:
        return to == InferiorRunRequested || to == InferiorShutdownRequested
            || to == InferiorStopOk || to == InferiorShutdownFinished;
    case InferiorStopFailed:
        return to == EngineShutdownRequested;

    case InferiorUnrunnable:
        return to == InferiorShutdownRequested;
    case InferiorShutdownRequested:
        return to == InferiorShutdownFinished;
    case InferiorShutdownFinished:
        return to == EngineShutdownRequested;

    case EngineShutdownRequested:
        return to == EngineShutdownFinished;
    case EngineShutdownFinished:
        return to == DebuggerFinished;

    case DebuggerFinished:
        return to == EngineSetupRequested; // Happens on restart.
    }

    qDebug() << "UNKNOWN DEBUGGER STATE:" << from;
    return false;
}

void DebuggerEngine::notifyEngineSetupFailed()
{
    showMessage("NOTE: ENGINE SETUP FAILED");
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupFailed);
    if (d->m_isPrimaryEngine) {
        showMessage(tr("Debugging has failed."), NormalMessageFormat);
        d->m_progress.setProgressValue(900);
        d->m_progress.reportCanceled();
        d->m_progress.reportFinished();
    }

    setState(DebuggerFinished);
}

void DebuggerEngine::notifyEngineSetupOk()
{
//#ifdef WITH_BENCHMARK
//    CALLGRIND_START_INSTRUMENTATION;
//#endif
    showMessage("NOTE: ENGINE SETUP OK");
    d->m_progress.setProgressValue(250);
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupOk);
    // Slaves will get called setupSlaveInferior() below.
    setState(EngineRunRequested);
    showMessage("CALL: RUN ENGINE");
    d->m_progress.setProgressValue(300);
    runEngine();
}

void DebuggerEngine::notifyEngineRunOkAndInferiorUnrunnable()
{
    showMessage("NOTE: INFERIOR UNRUNNABLE");
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    showStatusMessage(tr("Loading finished."));
    setState(InferiorUnrunnable);
}

void DebuggerEngine::notifyEngineRunFailed()
{
    showMessage("NOTE: ENGINE RUN FAILED");
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    d->m_progress.setProgressValue(900);
    d->m_progress.reportCanceled();
    d->m_progress.reportFinished();
    showStatusMessage(tr("Run failed."));
    setState(EngineRunFailed);
    d->doShutdownEngine();
}

void DebuggerEngine::notifyEngineRunAndInferiorRunOk()
{
    showMessage("NOTE: ENGINE RUN AND INFERIOR RUN OK");
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    showStatusMessage(tr("Running."));
    setState(InferiorRunOk);
}

void DebuggerEngine::notifyEngineRunAndInferiorStopOk()
{
    showMessage("NOTE: ENGINE RUN AND INFERIOR STOP OK");
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    showStatusMessage(tr("Stopped."));
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorRunRequested()
{
    showMessage("NOTE: INFERIOR RUN REQUESTED");
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << this << state());
    showStatusMessage(tr("Run requested..."));
    setState(InferiorRunRequested);
}

void DebuggerEngine::notifyInferiorRunOk()
{
    if (state() == InferiorRunOk) {
        showMessage("NOTE: INFERIOR RUN OK - REPEATED.");
        return;
    }
    showMessage("NOTE: INFERIOR RUN OK");
    showStatusMessage(tr("Running."));
    // Transition from StopRequested can happen in remotegdbadapter.
    QTC_ASSERT(state() == InferiorRunRequested
        || state() == InferiorStopOk
        || state() == InferiorStopRequested, qDebug() << this << state());
    setState(InferiorRunOk);
}

void DebuggerEngine::notifyInferiorRunFailed()
{
    showMessage("NOTE: INFERIOR RUN FAILED");
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << this << state());
    setState(InferiorRunFailed);
    setState(InferiorStopOk);
    if (isDying())
        d->doShutdownInferior();
}

void DebuggerEngine::notifyInferiorStopOk()
{
    showMessage("NOTE: INFERIOR STOP OK");
    // Ignore spurious notifications after we are set to die.
    if (isDying()) {
        showMessage("NOTE: ... WHILE DYING. ");
        // Forward state to "StopOk" if needed.
        if (state() == InferiorStopRequested
                || state() == InferiorRunRequested
                || state() == InferiorRunOk) {
            showMessage("NOTE: ... FORWARDING TO 'STOP OK'. ");
            setState(InferiorStopOk);
        }
        if (state() == InferiorStopOk || state() == InferiorStopFailed)
            d->doShutdownInferior();
        showMessage("NOTE: ... IGNORING STOP MESSAGE");
        return;
    }
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << this << state());
    showMessage(tr("Stopped."), StatusBar);
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorSpontaneousStop()
{
    showMessage("NOTE: INFERIOR SPONTANEOUS STOP");
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << this << state());
    EngineManager::activateEngine(this);
    showMessage(tr("Stopped."), StatusBar);
    setState(InferiorStopOk);
    if (boolSetting(RaiseOnInterrupt))
        ICore::raiseWindow(DebuggerMainWindow::instance());
}

void DebuggerEngine::notifyInferiorStopFailed()
{
    showMessage("NOTE: INFERIOR STOP FAILED");
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << this << state());
    setState(InferiorStopFailed);
    d->doShutdownEngine();
}

void DebuggerEnginePrivate::setInitialActionStates()
{
    m_returnWindow->setVisible(false);
    setBusyCursor(false);

    m_recordForReverseOperationAction.setCheckable(true);
    m_recordForReverseOperationAction.setChecked(false);
    m_recordForReverseOperationAction.setIcon(Icons::RECORD_OFF.icon());
    m_recordForReverseOperationAction.setToolTip(QString("<html><head/><body><p>%1</p><p>"
                                                         "<b>%2</b>%3</p></body></html>").arg(
                         tr("Record information to enable stepping backwards."),
                         tr("Note: "),
                         tr("This feature is very slow and unstable on the GDB side. "
                            "It exhibits unpredictable behavior when going backwards over system "
                            "calls and is very likely to destroy your debugging session.")));

    m_operateInReverseDirectionAction.setCheckable(true);
    m_operateInReverseDirectionAction.setChecked(false);
    m_operateInReverseDirectionAction.setIcon(Icons::DIRECTION_FORWARD.icon());

    m_snapshotAction.setIcon(Utils::Icons::SNAPSHOT_TOOLBAR.icon());

    m_attachToQmlPortAction.setEnabled(true);
    m_attachToCoreAction.setEnabled(true);
    m_attachToRemoteServerAction.setEnabled(true);
    m_detachAction.setEnabled(false);

    m_watchAction.setEnabled(true);
    m_breakAction.setEnabled(false);
    m_snapshotAction.setEnabled(false);
    m_operateByInstructionAction.setEnabled(false);

    m_exitAction.setEnabled(false);
    m_abortAction.setEnabled(false);
    m_resetAction.setEnabled(false);

    m_interruptAction.setEnabled(false);
    m_continueAction.setEnabled(false);

    m_stepIntoAction.setEnabled(true);
    m_stepOutAction.setEnabled(false);
    m_runToLineAction.setEnabled(false);
    m_runToLineAction.setVisible(false);
    m_runToSelectedFunctionAction.setEnabled(true);
    m_returnFromFunctionAction.setEnabled(false);
    m_jumpToLineAction.setEnabled(false);
    m_jumpToLineAction.setVisible(false);
    m_stepOverAction.setEnabled(true);

    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(false);

    m_threadLabel->setEnabled(false);
}

void DebuggerEnginePrivate::updateState(bool alsoUpdateCompanion)
{
    if (!m_perspective)
        return;

    const DebuggerState state = m_state;
    const bool companionPreventsAction = m_engine->companionPreventsActions();

    // Fixme: hint tr("Debugger is Busy");
    // Exactly one of m_interuptAction and m_continueAction should be
    // visible, possibly disabled.
    if (state == DebuggerNotReady) {
        // Happens when companion starts, otherwise this should not happen.
        QTC_CHECK(m_companionEngine);
        m_interruptAction.setVisible(true);
        m_interruptAction.setEnabled(false);
        m_continueAction.setVisible(false);
        m_continueAction.setEnabled(false);
        m_stepOverAction.setEnabled(true);
        m_stepIntoAction.setEnabled(true);
        m_stepOutAction.setEnabled(false);
        m_exitAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(true);
    } else if (state == InferiorStopOk) {
        // F5 continues, Shift-F5 kills. It is "continuable".
        m_interruptAction.setVisible(false);
        m_interruptAction.setEnabled(false);
        m_continueAction.setVisible(true);
        m_continueAction.setEnabled(!companionPreventsAction);
        m_stepOverAction.setEnabled(!companionPreventsAction);
        m_stepIntoAction.setEnabled(!companionPreventsAction);
        m_stepOutAction.setEnabled(!companionPreventsAction);
        m_exitAction.setEnabled(true);
        m_debugWithoutDeployAction.setEnabled(false);
        m_localsAndInspectorWindow->setShowLocals(true);
    } else if (state == InferiorRunOk) {
        // Shift-F5 interrupts. It is also "interruptible".
        m_interruptAction.setVisible(true);
        m_interruptAction.setEnabled(!companionPreventsAction);
        m_continueAction.setVisible(false);
        m_continueAction.setEnabled(false);
        m_stepOverAction.setEnabled(false);
        m_stepIntoAction.setEnabled(false);
        m_stepOutAction.setEnabled(false);
        m_exitAction.setEnabled(true);
        m_debugWithoutDeployAction.setEnabled(false);
        m_localsAndInspectorWindow->setShowLocals(false);
    } else if (state == DebuggerFinished) {
        const bool canRun = ProjectExplorerPlugin::canRunStartupProject(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        // We don't want to do anything anymore.
        m_interruptAction.setVisible(true);
        m_interruptAction.setEnabled(false);
        m_continueAction.setVisible(false);
        m_continueAction.setEnabled(false);
        m_stepOverAction.setEnabled(false);
        m_stepIntoAction.setEnabled(false);
        m_stepOutAction.setEnabled(false);
        m_exitAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(canRun);
        setBusyCursor(false);
        cleanupViews();
    } else if (state == InferiorUnrunnable) {
        // We don't want to do anything anymore.
        m_interruptAction.setVisible(true);
        m_interruptAction.setEnabled(false);
        m_continueAction.setVisible(false);
        m_continueAction.setEnabled(false);
        m_stepOverAction.setEnabled(false);
        m_stepIntoAction.setEnabled(false);
        m_stepOutAction.setEnabled(false);
        m_exitAction.setEnabled(true);
        m_debugWithoutDeployAction.setEnabled(false);
        // show locals in core dumps
        m_localsAndInspectorWindow->setShowLocals(true);
    } else {
        // Everything else is "undisturbable".
        m_interruptAction.setVisible(true);
        m_interruptAction.setEnabled(false);
        m_continueAction.setVisible(false);
        m_continueAction.setEnabled(false);
        m_stepOverAction.setEnabled(false);
        m_stepIntoAction.setEnabled(false);
        m_stepOutAction.setEnabled(false);
        m_exitAction.setEnabled(false);
        m_debugWithoutDeployAction.setEnabled(false);
    }

    m_attachToQmlPortAction.setEnabled(true);
    m_attachToCoreAction.setEnabled(true);
    m_attachToRemoteServerAction.setEnabled(true);

    const bool threadsEnabled = state == InferiorStopOk || state == InferiorUnrunnable;
    m_threadsHandler.threadSwitcher()->setEnabled(threadsEnabled);
    m_threadLabel->setEnabled(threadsEnabled);

    const bool isCore = m_engine->runParameters().startMode == AttachCore;
    const bool stopped = state == InferiorStopOk;
    const bool detachable = stopped && !isCore;
    m_detachAction.setEnabled(detachable);

    if (stopped)
        QApplication::alert(ICore::mainWindow(), 3000);

    updateReverseActions();

    const bool canSnapshot = m_engine->hasCapability(SnapshotCapability);
    m_snapshotAction.setVisible(canSnapshot);
    m_snapshotAction.setEnabled(stopped && !isCore);

    m_watchAction.setEnabled(true);
    m_breakAction.setEnabled(true);

    const bool canOperateByInstruction = m_engine->hasCapability(OperateByInstructionCapability);
    m_operateByInstructionAction.setVisible(canOperateByInstruction);
    m_operateByInstructionAction.setEnabled(canOperateByInstruction && (stopped || isCore));

    m_abortAction.setEnabled(state != DebuggerNotReady
                                      && state != DebuggerFinished);
    m_resetAction.setEnabled((stopped || state == DebuggerNotReady)
                              && m_engine->hasCapability(ResetInferiorCapability));

    m_stepIntoAction.setEnabled(stopped || state == DebuggerNotReady);
    m_stepIntoAction.setToolTip(QString());

    m_stepOverAction.setEnabled(stopped || state == DebuggerNotReady);
    m_stepOverAction.setToolTip(QString());

    m_stepOutAction.setEnabled(stopped);

    const bool canRunToLine = m_engine->hasCapability(RunToLineCapability);
    m_runToLineAction.setVisible(canRunToLine);
    m_runToLineAction.setEnabled(stopped && canRunToLine);

    m_runToSelectedFunctionAction.setEnabled(stopped);

    const bool canReturnFromFunction = m_engine->hasCapability(ReturnFromFunctionCapability);
    m_returnFromFunctionAction.setVisible(canReturnFromFunction);
    m_returnFromFunctionAction.setEnabled(stopped && canReturnFromFunction);

    const bool canJump = m_engine->hasCapability(JumpToLineCapability);
    m_jumpToLineAction.setVisible(canJump);
    m_jumpToLineAction.setEnabled(stopped && canJump);

    const bool actionsEnabled = m_engine->debuggerActionsEnabled();
    const bool canDeref = actionsEnabled && m_engine->hasCapability(AutoDerefPointersCapability);
    action(AutoDerefPointers)->setEnabled(canDeref);
    action(AutoDerefPointers)->setEnabled(true);
    action(ExpandStack)->setEnabled(actionsEnabled);

    const bool notbusy = state == InferiorStopOk
        || state == DebuggerNotReady
        || state == DebuggerFinished
        || state == InferiorUnrunnable;
    setBusyCursor(!notbusy);

    if (alsoUpdateCompanion && m_companionEngine)
        m_companionEngine->updateState(false);
}

void DebuggerEnginePrivate::updateReverseActions()
{
    const bool stopped = m_state == InferiorStopOk;
    const bool reverseEnabled = boolSetting(EnableReverseDebugging);
    const bool canReverse = reverseEnabled && m_engine->hasCapability(ReverseSteppingCapability);
    const bool doesRecord = m_recordForReverseOperationAction.isChecked();

    m_recordForReverseOperationAction.setVisible(canReverse);
    m_recordForReverseOperationAction.setEnabled(canReverse && stopped);
    m_recordForReverseOperationAction.setIcon(doesRecord
                                              ? Icons::RECORD_ON.icon()
                                              : Icons::RECORD_OFF.icon());

    m_operateInReverseDirectionAction.setVisible(canReverse);
    m_operateInReverseDirectionAction.setEnabled(canReverse && stopped && doesRecord);
    m_operateInReverseDirectionAction.setIcon(Icons::DIRECTION_BACKWARD.icon());
    m_operateInReverseDirectionAction.setText(DebuggerEngine::tr("Operate in Reverse Direction"));
}

void DebuggerEnginePrivate::cleanupViews()
{
    const bool closeSource = boolSetting(CloseSourceBuffersOnExit);
    const bool closeMemory = boolSetting(CloseMemoryBuffersOnExit);

    QList<IDocument *> toClose;
    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        const bool isMemory = document->property(Constants::OPENED_WITH_DISASSEMBLY).toBool();
        if (document->property(Constants::OPENED_BY_DEBUGGER).toBool()) {
            bool keepIt = true;
            if (document->isModified())
                keepIt = true;
            else if (document->filePath().toString().contains("qeventdispatcher"))
                keepIt = false;
            else if (isMemory)
                keepIt = !closeMemory;
            else
                keepIt = !closeSource;

            if (keepIt)
                document->setProperty(Constants::OPENED_BY_DEBUGGER, false);
            else
                toClose.append(document);
        }
    }
    EditorManager::closeDocuments(toClose);
}

void DebuggerEnginePrivate::setBusyCursor(bool busy)
{
    //STATE_DEBUG("BUSY FROM: " << m_busy << " TO: " << busy);
    if (m_isDying)
        return;
    if (busy == m_busy)
        return;
    m_busy = busy;
    const QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_breakWindow->setCursor(cursor);
    //m_consoleWindow->setCursor(cursor);
    m_localsWindow->setCursor(cursor);
    m_modulesWindow->setCursor(cursor);
    m_logWindow->setCursor(cursor);
    m_registerWindow->setCursor(cursor);
    m_returnWindow->setCursor(cursor);
    m_sourceFilesWindow->setCursor(cursor);
    m_stackWindow->setCursor(cursor);
    m_threadsWindow->setCursor(cursor);
    m_watchersWindow->setCursor(cursor);
}

void DebuggerEngine::notifyInferiorShutdownFinished()
{
    showMessage("INFERIOR FINISHED SHUT DOWN");
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << this << state());
    setState(InferiorShutdownFinished);
    d->doShutdownEngine();
}

void DebuggerEngine::notifyInferiorIll()
{
    showMessage("NOTE: INFERIOR ILL");
    // This can be issued in almost any state. The inferior could still be
    // alive as some previous notifications might have been bogus.
    startDying();
    if (state() == InferiorRunRequested) {
        // We asked for running, but did not see a response.
        // Assume the inferior is dead.
        // FIXME: Use timeout?
        setState(InferiorRunFailed);
        setState(InferiorStopOk);
    }
    d->doShutdownInferior();
}

void DebuggerEngine::notifyEngineShutdownFinished()
{
    showMessage("NOTE: ENGINE SHUTDOWN FINISHED");
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << this << state());
    setState(EngineShutdownFinished);
    d->doFinishDebugger();
}

void DebuggerEngine::notifyEngineIll()
{
//#ifdef WITH_BENCHMARK
//    CALLGRIND_STOP_INSTRUMENTATION;
//    CALLGRIND_DUMP_STATS;
//#endif
    showMessage("NOTE: ENGINE ILL ******");
    startDying();
    switch (state()) {
        case InferiorRunRequested:
        case InferiorRunOk:
            // The engine does not look overly ill right now, so attempt to
            // properly interrupt at least once. If that fails, we are on the
            // shutdown path due to d->m_targetState anyways.
            setState(InferiorStopRequested, true);
            showMessage("ATTEMPT TO INTERRUPT INFERIOR");
            interruptInferior();
            break;
        case InferiorStopRequested:
            notifyInferiorStopFailed();
            break;
        case InferiorStopOk:
            showMessage("FORWARDING STATE TO InferiorShutdownFinished");
            setState(InferiorShutdownFinished, true);
            d->doShutdownEngine();
            break;
        default:
            d->doShutdownEngine();
            break;
    }
}

void DebuggerEngine::notifyEngineSpontaneousShutdown()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
    showMessage("NOTE: ENGINE SPONTANEOUS SHUTDOWN");
    setState(EngineShutdownFinished, true);
    d->doFinishDebugger();
}

void DebuggerEngine::notifyInferiorExited()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
    showMessage("NOTE: INFERIOR EXITED");
    d->resetLocation();
    setState(InferiorShutdownFinished);
    d->doShutdownEngine();
}

void DebuggerEngine::updateState(bool alsoUpdateCompanion)
{
    d->updateState(alsoUpdateCompanion);
}

WatchTreeView *DebuggerEngine::inspectorView()
{
    return d->m_inspectorView;
}

void DebuggerEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    //qDebug() << "PLUGIN OUTPUT: " << channel << msg;
    QTC_ASSERT(d->m_logWindow, qDebug() << "MSG: " << msg; return);
    switch (channel) {
        case StatusBar:
            d->m_logWindow->showInput(LogMisc, msg);
            d->m_logWindow->showOutput(LogMisc, msg);
            Debugger::showStatusMessage(msg, timeout);
            break;
        case LogMiscInput:
            d->m_logWindow->showInput(LogMisc, msg);
            d->m_logWindow->showOutput(LogMisc, msg);
            break;
        case LogInput:
            d->m_logWindow->showInput(LogInput, msg);
            d->m_logWindow->showOutput(LogInput, msg);
            break;
        case LogError:
            d->m_logWindow->showInput(LogError, QLatin1String("ERROR: ") + msg);
            d->m_logWindow->showOutput(LogError, QLatin1String("ERROR: ") + msg);
            break;
        case AppOutput:
        case AppStuff:
            d->m_logWindow->showOutput(channel, msg);
            emit appendMessageRequested(msg, StdOutFormatSameLine, false);
            break;
        case AppError:
            d->m_logWindow->showOutput(channel, msg);
            emit appendMessageRequested(msg, StdErrFormatSameLine, false);
            break;
        default:
            d->m_logWindow->showOutput(channel, msg);
            break;
    }
}

void DebuggerEngine::notifyDebuggerProcessFinished(int exitCode,
    QProcess::ExitStatus exitStatus, const QString &backendName)
{
    showMessage(QString("%1 PROCESS FINISHED, status %2, exit code %3")
                .arg(backendName).arg(exitStatus).arg(exitCode));

    switch (state()) {
    case DebuggerFinished:
        // Nothing to do.
        break;
    case EngineShutdownRequested:
    case InferiorShutdownRequested:
        notifyEngineShutdownFinished();
        break;
    case InferiorRunOk:
        // This could either be a real gdb/lldb crash or a quickly exited inferior
        // in the terminal adapter. In this case the stub proc will die soon,
        // too, so there's no need to act here.
        showMessage(QString("The %1 process exited somewhat unexpectedly.").arg(backendName));
        notifyEngineSpontaneousShutdown();
        break;
    default: {
        // Initiate shutdown sequence
        notifyInferiorIll();
        const QString msg = exitStatus == QProcess::CrashExit ?
                tr("The %1 process terminated.") :
                tr("The %2 process terminated unexpectedly (exit code %1).").arg(exitCode);
        AsynchronousMessageBox::critical(tr("Unexpected %1 Exit").arg(backendName),
                                         msg.arg(backendName));
        break;
    }
    }
}

static QString msgStateChanged(DebuggerState oldState, DebuggerState newState, bool forced)
{
    QString result;
    QTextStream str(&result);
    str << "State changed";
    if (forced)
        str << " BY FORCE";
    str << " from " << DebuggerEngine::stateName(oldState) << '(' << oldState
        << ") to " << DebuggerEngine::stateName(newState) << '(' << newState << ')';
    return result;
}

void DebuggerEngine::setState(DebuggerState state, bool forced)
{
    const QString msg = msgStateChanged(d->m_state, state, forced);

    DebuggerState oldState = d->m_state;
    d->m_state = state;

    if (!forced && !isAllowedTransition(oldState, state))
        qDebug() << "*** UNEXPECTED STATE TRANSITION: " << this << msg;

    if (state == EngineRunRequested)
        emit engineStarted();

    showMessage(msg, LogDebug);

    d->updateState(true);

    if (oldState != d->m_state)
        emit EngineManager::instance()->engineStateChanged(this);

    if (state == DebuggerFinished) {
        d->destroyPerspective();
        emit engineFinished();
    }
}

bool DebuggerEngine::isPrimaryEngine() const
{
    return d->m_isPrimaryEngine;
}

bool DebuggerEngine::canDisplayTooltip() const
{
    return state() == InferiorStopOk;
}

QString DebuggerEngine::toFileInProject(const QUrl &fileUrl)
{
    // make sure file finder is properly initialized
    const DebuggerRunParameters &rp = runParameters();
    d->m_fileFinder.setProjectDirectory(rp.projectSourceDirectory);
    d->m_fileFinder.setProjectFiles(rp.projectSourceFiles);
    d->m_fileFinder.setAdditionalSearchDirectories(rp.additionalSearchDirectories);
    d->m_fileFinder.setSysroot(rp.sysRoot);

    return d->m_fileFinder.findFile(fileUrl);
}

QString DebuggerEngine::expand(const QString &string) const
{
    return runParameters().macroExpander->expand(string);
}

QString DebuggerEngine::nativeStartupCommands() const
{
    return expand(QStringList({stringSetting(GdbStartupCommands),
                               runParameters().additionalStartupCommands}).join('\n'));
}

Perspective *DebuggerEngine::perspective() const
{
    return d->m_perspective;
}

void DebuggerEngine::updateMarkers()
{
    if (d->m_locationMark)
        d->m_locationMark->updateIcon();

    d->m_disassemblerAgent.updateLocationMarker();
}

void DebuggerEngine::updateToolTips()
{
    d->m_toolTipManager.updateToolTips();
}

DebuggerToolTipManager *DebuggerEngine::toolTipManager()
{
    return &d->m_toolTipManager;
}

bool DebuggerEngine::operatesByInstruction() const
{
    return d->m_operateByInstructionAction.isChecked();
}

bool DebuggerEngine::debuggerActionsEnabled() const
{
    return debuggerActionsEnabledHelper(d->m_state);
}

void DebuggerEngine::operateByInstructionTriggered(bool on)
{
    // Go to source only if we have the file.
    //    if (DebuggerEngine *cppEngine = m_engine->cppEngine()) {
    d->m_stackHandler.resetModel();
    if (d->m_stackHandler.currentIndex() >= 0) {
        const StackFrame frame = d->m_stackHandler.currentFrame();
        if (on || frame.isUsable())
            gotoLocation(Location(frame, true));
    }
    //    }
}

bool DebuggerEngine::companionPreventsActions() const
{
    return false;
}

void DebuggerEngine::notifyInferiorPid(const ProcessHandle &pid)
{
    if (d->m_inferiorPid == pid)
        return;
    d->m_inferiorPid = pid;
    if (pid.isValid()) {
        showMessage(tr("Taking notice of pid %1").arg(pid.pid()));
        DebuggerStartMode sm = runParameters().startMode;
        if (sm == StartInternal || sm == StartExternal || sm == AttachExternal)
            d->m_inferiorPid.activate();
    }
}

qint64 DebuggerEngine::inferiorPid() const
{
    return d->m_inferiorPid.pid();
}

bool DebuggerEngine::isReverseDebugging() const
{
    return d->m_operateInReverseDirectionAction.isChecked();
}

void DebuggerEngine::handleBeginOfRecordingReached()
{
    showStatusMessage(tr("Reverse-execution history exhausted. Going forward again."));
    d->m_operateInReverseDirectionAction.setChecked(false);
    d->updateReverseActions();
}

void DebuggerEngine::handleRecordingFailed()
{
    showStatusMessage(tr("Reverse-execution recording failed."));
    d->m_operateInReverseDirectionAction.setChecked(false);
    d->m_recordForReverseOperationAction.setChecked(false);
    d->updateReverseActions();
    executeRecordReverse(false);
}

// Called by DebuggerRunControl.
void DebuggerEngine::quitDebugger()
{
    showMessage(QString("QUIT DEBUGGER REQUESTED IN STATE %1").arg(state()));
    startDying();
    switch (state()) {
    case InferiorStopOk:
    case InferiorStopFailed:
    case InferiorUnrunnable:
        d->doShutdownInferior();
        break;
    case InferiorRunOk:
        setState(InferiorStopRequested);
        showMessage(tr("Attempting to interrupt."), StatusBar);
        interruptInferior();
        break;
    case EngineSetupRequested:
        notifyEngineSetupFailed();
        break;
    case EngineSetupOk:
        notifyEngineSetupFailed();
        break;
    case EngineRunRequested:
        notifyEngineRunFailed();
        break;
    case EngineShutdownRequested:
    case InferiorShutdownRequested:
        break;
    case EngineRunFailed:
    case DebuggerFinished:
    case InferiorShutdownFinished:
        break;
    default:
        // FIXME: We should disable the actions connected to that.
        notifyInferiorIll();
        break;
    }
}

void DebuggerEngine::requestInterruptInferior()
{
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << this << state());
    setState(InferiorStopRequested);
    showMessage("CALL: INTERRUPT INFERIOR");
    showMessage(tr("Attempting to interrupt."), StatusBar);
    interruptInferior();
}

void DebuggerEngine::progressPing()
{
    int progress = qMin(d->m_progress.progressValue() + 2, 800);
    d->m_progress.setProgressValue(progress);
}

bool DebuggerEngine::isStartupRunConfiguration() const
{
    return d->m_runConfiguration == RunConfiguration::startupRunConfiguration();
}

void DebuggerEngine::setCompanionEngine(DebuggerEngine *engine)
{
    d->m_companionEngine = engine;
}

void DebuggerEngine::setSecondaryEngine()
{
    d->m_isPrimaryEngine = false;
}

TerminalRunner *DebuggerEngine::terminal() const
{
    return d->m_terminalRunner;
}

void DebuggerEngine::selectWatchData(const QString &)
{
}

void DebuggerEngine::watchPoint(const QPoint &pnt)
{
    DebuggerCommand cmd("watchPoint", NeedsFullStop);
    cmd.arg("x", pnt.x());
    cmd.arg("y", pnt.y());
    cmd.callback = [this](const DebuggerResponse &response) {
        qulonglong addr = response.data["selected"].toAddress();
        if (addr == 0)
            showMessage(tr("Could not find a widget."), StatusBar);
        // Add the watcher entry nevertheless, as that's the place where
        // the user expects visual feedback.
        watchHandler()->watchExpression(response.data["expr"].data(), QString(), true);
    };
    runCommand(cmd);
}

void DebuggerEngine::runCommand(const DebuggerCommand &)
{
    // Overridden in the engines that use the interface.
    QTC_CHECK(false);
}

void DebuggerEngine::fetchDisassembler(DisassemblerAgent *)
{
}

void DebuggerEngine::activateFrame(int)
{
}

void DebuggerEngine::reloadModules()
{
}

void DebuggerEngine::examineModules()
{
}

void DebuggerEngine::loadSymbols(const QString &)
{
}

void DebuggerEngine::loadAllSymbols()
{
}

void DebuggerEngine::loadSymbolsForStack()
{
}

void DebuggerEngine::requestModuleSymbols(const QString &)
{
}

void DebuggerEngine::requestModuleSections(const QString &)
{
}

void DebuggerEngine::reloadRegisters()
{
}

void DebuggerEngine::reloadSourceFiles()
{
}

void DebuggerEngine::reloadFullStack()
{
}

void DebuggerEngine::loadAdditionalQmlStack()
{
}

void DebuggerEngine::reloadDebuggingHelpers()
{
}

void DebuggerEngine::addOptionPages(QList<IOptionsPage*> *) const
{
}

QString DebuggerEngine::qtNamespace() const
{
    return d->m_qtNamespace;
}

void DebuggerEngine::setQtNamespace(const QString &ns)
{
    d->m_qtNamespace = ns;
}

void DebuggerEngine::createSnapshot()
{
}

void DebuggerEngine::updateLocals()
{
    watchHandler()->resetValueCache();
    doUpdateLocals(UpdateParameters());
}

Context DebuggerEngine::debuggerContext() const
{
    return d->m_context;
}

void DebuggerEngine::updateAll()
{
}

QString DebuggerEngine::displayName() const
{
    //: e.g. LLDB for "myproject", shows up i
    return tr("%1 for \"%2\"").arg(d->m_debuggerName, runParameters().displayName);
}

void DebuggerEngine::insertBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    BreakpointState state = bp->state();
    QTC_ASSERT(state == BreakpointInsertionRequested,
               qDebug() << bp->modelId() << this << state);
    QTC_CHECK(false);
}

void DebuggerEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    BreakpointState state = bp->state();
    QTC_ASSERT(state == BreakpointRemoveRequested,
               qDebug() << bp->responseId() << this << state);
    QTC_CHECK(false);
}

void DebuggerEngine::updateBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    BreakpointState state = bp->state();
    QTC_ASSERT(state == BreakpointUpdateRequested,
               qDebug() << bp->responseId() << this << state);
    QTC_CHECK(false);
}

void DebuggerEngine::enableSubBreakpoint(const SubBreakpoint &sbp, bool)
{
    QTC_ASSERT(sbp, return);
    QTC_CHECK(false);
}

void DebuggerEngine::assignValueInDebugger(WatchItem *,
    const QString &, const QVariant &)
{
}

void DebuggerEngine::handleRecordReverse(bool record)
{
    executeRecordReverse(record);
    d->updateReverseActions();
}

void DebuggerEngine::handleReverseDirection(bool reverse)
{
    executeReverse(reverse);
    updateMarkers();
    d->updateReverseActions();
}

void DebuggerEngine::executeDebuggerCommand(const QString &)
{
    showMessage(tr("This debugger cannot handle user input."), StatusBar);
}

bool DebuggerEngine::isDying() const
{
    return d->m_isDying;
}

QString DebuggerEngine::msgStopped(const QString &reason)
{
    return reason.isEmpty() ? tr("Stopped.") : tr("Stopped: \"%1\".").arg(reason);
}

QString DebuggerEngine::msgStoppedBySignal(const QString &meaning,
    const QString &name)
{
    return tr("Stopped: %1 (Signal %2).").arg(meaning, name);
}

QString DebuggerEngine::msgStoppedByException(const QString &description,
    const QString &threadId)
{
    return tr("Stopped in thread %1 by: %2.").arg(threadId, description);
}

QString DebuggerEngine::msgInterrupted()
{
    return tr("Interrupted.");
}

bool DebuggerEngine::showStoppedBySignalMessageBox(QString meaning, QString name)
{
    if (d->m_alertBox)
        return false;

    if (name.isEmpty())
        name = ' ' + tr("<Unknown>", "name") + ' ';
    if (meaning.isEmpty())
        meaning = ' ' + tr("<Unknown>", "meaning") + ' ';
    const QString msg = tr("<p>The inferior stopped because it received a "
                           "signal from the operating system.<p>"
                           "<table><tr><td>Signal name : </td><td>%1</td></tr>"
                           "<tr><td>Signal meaning : </td><td>%2</td></tr></table>")
            .arg(name, meaning);

    d->m_alertBox = AsynchronousMessageBox::information(tr("Signal Received"), msg);
    return true;
}

void DebuggerEngine::showStoppedByExceptionMessageBox(const QString &description)
{
    const QString msg =
        tr("<p>The inferior stopped because it triggered an exception.<p>%1").
                         arg(description);
    AsynchronousMessageBox::information(tr("Exception Triggered"), msg);
}

void DebuggerEngine::openMemoryView(const MemoryViewSetupData &data)
{
    d->m_memoryAgents.createBinEditor(data, this);
}

void DebuggerEngine::updateMemoryViews()
{
    d->m_memoryAgents.updateContents();
}

void DebuggerEngine::openDisassemblerView(const Location &location)
{
    DisassemblerAgent *agent = new DisassemblerAgent(this);
    agent->setLocation(location);
}

void DebuggerEngine::raiseWatchersWindow()
{
    if (d->m_watchersView) {
        if (auto dock = qobject_cast<QDockWidget *>(d->m_watchersView->parentWidget())) {
            if (QAction *act = dock->toggleViewAction()) {
                if (!act->isChecked())
                    QTimer::singleShot(1, act, [act] { act->trigger(); });
                dock->raise();
            }
        }
    }
}

void DebuggerEngine::openMemoryEditor()
{
    AddressDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;
    MemoryViewSetupData data;
    data.startAddress = dialog.address();
    openMemoryView(data);
}

void DebuggerEngine::updateLocalsView(const GdbMi &all)
{
    WatchHandler *handler = watchHandler();

    const GdbMi typeInfo = all["typeinfo"];
    handler->recordTypeInfo(typeInfo);

    const GdbMi data = all["data"];
    handler->insertItems(data);

    const GdbMi ns = all["qtnamespace"];
    if (ns.isValid()) {
        setQtNamespace(ns.data());
        showMessage("FOUND NAMESPACED QT: " + ns.data());
    }

    static int count = 0;
    showMessage(QString("<Rebuild Watchmodel %1 @ %2 >")
                .arg(++count).arg(LogWindow::logTimeStamp()), LogMiscInput);
    showMessage(tr("Finished retrieving data."), 400, StatusBar);

    d->m_toolTipManager.updateToolTips();

    const bool partial = all["partial"].toInt();
    if (!partial)
        updateMemoryViews();
}

bool DebuggerEngine::canHandleToolTip(const DebuggerToolTipContext &context) const
{
    return state() == InferiorStopOk && context.isCppEditor;
}

void DebuggerEngine::updateItem(const QString &iname)
{
    if (d->m_lookupRequests.contains(iname)) {
        showMessage(QString("IGNORING REPEATED REQUEST TO EXPAND " + iname));
        WatchHandler *handler = watchHandler();
        WatchItem *item = handler->findItem(iname);
        QTC_CHECK(item);
        WatchModelBase *model = handler->model();
        QTC_CHECK(model);
        if (item && !model->hasChildren(model->indexForItem(item))) {
            handler->notifyUpdateStarted(UpdateParameters(iname));
            item->setValue(decodeData({}, "notaccessible"));
            item->setHasChildren(false);
            item->outdated = false;
            item->update();
            handler->notifyUpdateFinished();
            return;
        }
        // We could legitimately end up here after expanding + closing + re-expaning an item.
    }
    d->m_lookupRequests.insert(iname);

    UpdateParameters params;
    params.partialVariable = iname;
    doUpdateLocals(params);
}

void DebuggerEngine::updateWatchData(const QString &iname)
{
    // This is used in cases where re-evaluation is ok for the same iname
    // e.g. when changing the expression in a watcher.
    UpdateParameters params;
    params.partialVariable = iname;
    doUpdateLocals(params);
}

void DebuggerEngine::expandItem(const QString &iname)
{
    updateItem(iname);
}

void DebuggerEngine::handleExecDetach()
{
    resetLocation();
    detachDebugger();
}

void DebuggerEngine::handleExecContinue()
{
    resetLocation();
    continueInferior();
}

void DebuggerEngine::handleExecInterrupt()
{
    resetLocation();
    requestInterruptInferior();
}

void DebuggerEngine::handleReset()
{
    resetLocation();
    resetInferior();
}

void DebuggerEngine::handleExecStepIn()
{
    resetLocation();
    executeStepIn(operatesByInstruction());
}

void DebuggerEngine::handleExecStepOver()
{
    resetLocation();
    executeStepOver(operatesByInstruction());
}

void DebuggerEngine::handleExecStepOut()
{
    resetLocation();
    executeStepOut();
}

void DebuggerEngine::handleExecReturn()
{
    resetLocation();
    executeReturn();
}

void DebuggerEngine::handleExecJumpToLine()
{
    resetLocation();
    if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
        ContextData location = getLocationContext(textEditor->textDocument(),
                                                  textEditor->currentLine());
        if (location.isValid())
            executeJumpToLine(location);
    }
}

void DebuggerEngine::handleExecRunToLine()
{
    resetLocation();
    if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
        ContextData location = getLocationContext(textEditor->textDocument(),
                                                  textEditor->currentLine());
        if (location.isValid())
            executeRunToLine(location);
    }
}

void DebuggerEngine::handleExecRunToSelectedFunction()
{
    BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    QTC_ASSERT(textEditor, return);
    QTextCursor cursor = textEditor->textCursor();
    QString functionName = cursor.selectedText();
    if (functionName.isEmpty()) {
        const QTextBlock block = cursor.block();
        const QString line = block.text();
        foreach (const QString &str, line.trimmed().split(QLatin1Char('('))) {
            QString a;
            for (int i = str.size(); --i >= 0; ) {
                if (!str.at(i).isLetterOrNumber())
                    break;
                a = str.at(i) + a;
            }
            if (!a.isEmpty()) {
                functionName = a;
                break;
            }
        }
    }

    if (functionName.isEmpty()) {
        showMessage(tr("No function selected."), StatusBar);
    } else {
        showMessage(tr("Running to function \"%1\".").arg(functionName), StatusBar);
        resetLocation();
        executeRunToFunction(functionName);
    }
}

void DebuggerEngine::handleAddToWatchWindow()
{
    // Requires a selection, but that's the only case we want anyway.
    BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor();
    if (!textEditor)
        return;
    QTextCursor tc = textEditor->textCursor();
    QString exp;
    if (tc.hasSelection()) {
        exp = tc.selectedText();
    } else {
        int line, column;
        exp = cppExpressionAt(textEditor->editorWidget(), tc.position(), &line, &column);
    }
    if (hasCapability(WatchComplexExpressionsCapability))
        exp = removeObviousSideEffects(exp);
    else
        exp = fixCppExpression(exp);
    exp = exp.trimmed();
    if (exp.isEmpty()) {
        // Happens e.g. when trying to evaluate 'char' or 'return'.
        AsynchronousMessageBox::warning(tr("Warning"),
                                        tr("Select a valid expression to evaluate."));
        return;
    }
    watchHandler()->watchVariable(exp);
}

void DebuggerEngine::handleFrameDown()
{
    frameDown();
}

void DebuggerEngine::handleFrameUp()
{
    frameUp();
}

void DebuggerEngine::checkState(DebuggerState state, const char *file, int line)
{
    const DebuggerState current = d->m_state;
    if (current == state)
        return;

    QString msg = QString("UNEXPECTED STATE: %1  WANTED: %2 IN %3:%4")
                .arg(stateName(current)).arg(stateName(state)).arg(QLatin1String(file)).arg(line);

    showMessage(msg, LogError);
    qDebug("%s", qPrintable(msg));
}

bool DebuggerEngine::isNativeMixedEnabled() const
{
    return d->m_runParameters.isNativeMixedDebugging();
}

bool DebuggerEngine::isNativeMixedActive() const
{
    return isNativeMixedEnabled(); //&& boolSetting(OperateNativeMixed);
}

bool DebuggerEngine::isNativeMixedActiveFrame() const
{
    if (!isNativeMixedActive())
        return false;
    if (stackHandler()->frames().isEmpty())
        return false;
    StackFrame frame = stackHandler()->frameAt(0);
    return frame.language == QmlLanguage;
}

void DebuggerEngine::startDying() const
{
    d->m_isDying = true;
    if (DebuggerEngine *other = d->m_companionEngine)
        other->d->m_isDying = true;
}

QString DebuggerEngine::runId() const
{
    return d->m_runId;
}

bool DebuggerRunParameters::isCppDebugging() const
{
    return cppEngineType == GdbEngineType
        || cppEngineType == LldbEngineType
        || cppEngineType == CdbEngineType;
}

bool DebuggerRunParameters::isNativeMixedDebugging() const
{
    return nativeMixedEnabled && isCppDebugging() && isQmlDebugging;
}

QString DebuggerEngine::formatStartParameters() const
{
    const DebuggerRunParameters &sp = d->m_runParameters;
    QString rc;
    QTextStream str(&rc);
    str << "Start parameters: '" << sp.displayName << "' mode: " << sp.startMode
        << "\nABI: " << sp.toolChainAbi.toString() << '\n';
    str << "Languages: ";
    if (sp.isCppDebugging())
        str << "c++ ";
    if (sp.isQmlDebugging)
        str << "qml";
    str << '\n';
    if (!sp.inferior.executable.isEmpty()) {
        str << "Executable: " << QDir::toNativeSeparators(sp.inferior.executable)
            << ' ' << sp.inferior.commandLineArguments;
        if (d->m_terminalRunner)
            str << " [terminal]";
        str << '\n';
        if (!sp.inferior.workingDirectory.isEmpty())
            str << "Directory: " << QDir::toNativeSeparators(sp.inferior.workingDirectory)
                << '\n';
    }
    QString cmd = sp.debugger.executable;
    if (!cmd.isEmpty())
        str << "Debugger: " << QDir::toNativeSeparators(cmd) << '\n';
    if (!sp.coreFile.isEmpty())
        str << "Core: " << QDir::toNativeSeparators(sp.coreFile) << '\n';
    if (sp.attachPID.isValid())
        str << "PID: " << sp.attachPID.pid() << ' ' << sp.crashParameter << '\n';
    if (!sp.projectSourceDirectory.isEmpty()) {
        str << "Project: " << sp.projectSourceDirectory.toUserOutput() << '\n';
        str << "Additional Search Directories:";
        for (const FileName &dir : sp.additionalSearchDirectories)
            str << ' ' << dir;
        str << '\n';
    }
    if (!sp.remoteChannel.isEmpty())
        str << "Remote: " << sp.remoteChannel << '\n';
    if (!sp.qmlServer.host().isEmpty())
        str << "QML server: " << sp.qmlServer.host() << ':' << sp.qmlServer.port() << '\n';
    str << "Sysroot: " << sp.sysRoot << '\n';
    str << "Debug Source Location: " << sp.debugSourceLocation.join(QLatin1Char(':')) << '\n';
    return rc;
}

// CppDebuggerEngine

Context CppDebuggerEngine::languageContext() const
{
    return Context(Constants::C_CPPDEBUGGER);
}

void CppDebuggerEngine::validateRunParameters(DebuggerRunParameters &rp)
{
    const bool warnOnRelease = boolSetting(WarnOnReleaseBuilds) && rp.toolChainAbi.osFlavor() != Abi::AndroidLinuxFlavor;
    bool warnOnInappropriateDebugger = false;
    QString detailedWarning;
    switch (rp.toolChainAbi.binaryFormat()) {
    case Abi::PEFormat: {
        QString preferredDebugger;
        if (rp.toolChainAbi.osFlavor() == Abi::WindowsMSysFlavor) {
            if (rp.cppEngineType == CdbEngineType)
                preferredDebugger = "GDB";
        } else if (rp.cppEngineType != CdbEngineType) {
            // osFlavor() is MSVC, so the recommended debugger is CDB
            preferredDebugger = "CDB";
        }
        if (!preferredDebugger.isEmpty()) {
            warnOnInappropriateDebugger = true;
            detailedWarning = DebuggerEngine::tr(
                        "The inferior is in the Portable Executable format.\n"
                        "Selecting %1 as debugger would improve the debugging "
                        "experience for this binary format.").arg(preferredDebugger);
            break;
        }
        if (warnOnRelease
                && rp.cppEngineType == CdbEngineType
                && rp.startMode != AttachToRemoteServer) {
            QTC_ASSERT(!rp.symbolFile.isEmpty(), return);
            if (!rp.symbolFile.endsWith(".exe", Qt::CaseInsensitive))
                rp.symbolFile.append(".exe");
            QString errorMessage;
            QStringList rc;
            if (getPDBFiles(rp.symbolFile, &rc, &errorMessage) && !rc.isEmpty())
                return;
            if (!errorMessage.isEmpty()) {
                detailedWarning.append('\n');
                detailedWarning.append(errorMessage);
            }
        } else {
            return;
        }
        break;
    }
    case Abi::ElfFormat: {
        if (rp.cppEngineType == CdbEngineType) {
            warnOnInappropriateDebugger = true;
            detailedWarning = DebuggerEngine::tr(
                        "The inferior is in the ELF format.\n"
                        "Selecting GDB or LLDB as debugger would improve the debugging "
                        "experience for this binary format.");
            break;
        }

        Utils::ElfReader reader(rp.symbolFile);
        const ElfData elfData = reader.readHeaders();
        const QString error = reader.errorString();

        showMessage("EXAMINING " + rp.symbolFile, LogDebug);
        QByteArray msg = "ELF SECTIONS: ";

        static const QList<QByteArray> interesting = {
            ".debug_info",
            ".debug_abbrev",
            ".debug_line",
            ".debug_str",
            ".debug_loc",
            ".debug_range",
            ".gdb_index",
            ".note.gnu.build-id",
            ".gnu.hash",
            ".gnu_debuglink"
        };

        QSet<QByteArray> seen;
        for (const ElfSectionHeader &header : elfData.sectionHeaders) {
            msg.append(header.name);
            msg.append(' ');
            if (interesting.contains(header.name))
                seen.insert(header.name);
        }
        showMessage(QString::fromUtf8(msg), LogDebug);

        if (!error.isEmpty()) {
            showMessage("ERROR WHILE READING ELF SECTIONS: " + error, LogDebug);
            return;
        }

        if (elfData.sectionHeaders.isEmpty()) {
            showMessage("NO SECTION HEADERS FOUND. IS THIS AN EXECUTABLE?", LogDebug);
            return;
        }

        // Note: .note.gnu.build-id also appears in regular release builds.
        // bool hasBuildId = elfData.indexOf(".note.gnu.build-id") >= 0;
        bool hasEmbeddedInfo = elfData.indexOf(".debug_info") >= 0;
        bool hasLink = elfData.indexOf(".gnu_debuglink") >= 0;
        if (hasEmbeddedInfo) {
            QSharedPointer<GlobalDebuggerOptions> options = Internal::globalDebuggerOptions();
            SourcePathRegExpMap globalRegExpSourceMap;
            globalRegExpSourceMap.reserve(options->sourcePathRegExpMap.size());
            foreach (auto entry, options->sourcePathRegExpMap) {
                const QString expanded = Utils::globalMacroExpander()->expand(entry.second);
                if (!expanded.isEmpty())
                    globalRegExpSourceMap.push_back(qMakePair(entry.first, expanded));
            }
            if (globalRegExpSourceMap.isEmpty())
                return;
            if (QSharedPointer<Utils::ElfMapper> mapper = reader.readSection(".debug_str")) {
                const char *str = mapper->start;
                const char *limit = str + mapper->fdlen;
                bool found = false;
                while (str < limit) {
                    const QString string = QString::fromUtf8(str);
                    for (auto itExp = globalRegExpSourceMap.begin(), itEnd = globalRegExpSourceMap.end();
                         itExp != itEnd;
                         ++itExp) {
                        QRegExp exp = itExp->first;
                        int index = exp.indexIn(string);
                        if (index != -1) {
                            rp.sourcePathMap.insert(string.left(index) + exp.cap(1), itExp->second);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;

                    const int len = int(strlen(str));
                    if (len == 0)
                        break;
                    str += len + 1;
                }
            }
        }
        if (hasEmbeddedInfo || hasLink)
            return;

        foreach (const QByteArray &name, interesting) {
            const QString found = seen.contains(name) ? DebuggerEngine::tr("Found.")
                                                      : DebuggerEngine::tr("Not found.");
            detailedWarning.append('\n' + DebuggerEngine::tr("Section %1: %2").arg(QString::fromUtf8(name)).arg(found));
        }
        break;
    }
    default:
        return;
    }
    if (warnOnInappropriateDebugger) {
        AsynchronousMessageBox::information(DebuggerEngine::tr("Warning"),
                DebuggerEngine::tr("The selected debugger may be inappropriate for the inferior.\n"
                   "Examining symbols and setting breakpoints by file name and line number "
                   "may fail.\n")
               + '\n' + detailedWarning);
    } else if (warnOnRelease) {
        AsynchronousMessageBox::information(DebuggerEngine::tr("Warning"),
               DebuggerEngine::tr("This does not seem to be a \"Debug\" build.\n"
                  "Setting breakpoints by file name and line number may fail.")
               + '\n' + detailedWarning);
    }
}

} // namespace Internal
} // namespace Debugger

#include "debuggerengine.moc"
