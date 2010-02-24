/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "debuggermanager.h"

#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerrunner.h"
#include "debuggerconstants.h"
#include "idebuggerengine.h"
#include "debuggerstringutils.h"
#include "watchutils.h"
#include "debuggeruiswitcher.h"
#include "debuggermainwindow.h"

#include "breakwindow.h"
#include "debuggeroutputwindow.h"
#include "moduleswindow.h"
#include "registerwindow.h"
#include "snapshotwindow.h"
#include "stackwindow.h"
#include "sourcefileswindow.h"
#include "threadswindow.h"
#include "watchwindow.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "stackhandler.h"
#include "stackframe.h"
#include "watchhandler.h"

#include "debuggerdialogs.h"
#ifdef Q_OS_WIN
#  include "shared/peutils.h"
#endif

#include <coreplugin/minisplitter.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>
#include <projectexplorer/toolchain.h>
#include <cplusplus/CppDocument.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <texteditor/fontsettings.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QTime>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QErrorMessage>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QToolButton>
#include <QtGui/QToolTip>

#define DEBUG_STATE 1
#ifdef DEBUG_STATE
// use  Q_FUNC_INFO?
#   define STATE_DEBUG(s) \
    { QString msg; QTextStream ts(&msg); ts << s; \
      showDebuggerOutput(LogDebug, msg); }
#else
#   define STATE_DEBUG(s)
#endif

// Note: the Debugger process itself and any helper processes like
// gdbserver, the trk client etc are referred to as 'Adapter',
// whereas the debugged process is referred to as 'Inferior'.
//
//              0 == DebuggerNotReady
//                          |
//                    EngineStarting
//                          |
//                    AdapterStarting --> AdapterStartFailed --> 0
//                          |
//                    AdapterStarted ------------------------------------.
//                          |                                            v
//                   InferiorStarting ----> InferiorStartFailed -------->|
//                          |                                            |
//         (core)           |     (attach) (term) (remote)               |
//      .-----------------<-|->------------------.                       |
//      |                   v                    |                       |
//  InferiorUnrunnable      | (plain)            |                       |
//      |                   | (trk)              |                       |
//      |                   |                    |                       |
//      |    .--> InferiorRunningRequested       |                       |
//      |    |              |                    |                       |
//      |    |       InferiorRunning             |                       |
//      |    |              |                    |                       |
//      |    |       InferiorStopping            |                       |
//      |    |              |                    |                       |
//      |    '------ InferiorStopped <-----------'                       |
//      |                   |                                            v
//      |          InferiorShuttingDown  ->  InferiorShutdownFailed ---->|
//      |                   |                                            |
//      |            InferiorShutDown                                    |
//      |                   |                                            |
//      '-------->  EngineShuttingDown  <--------------------------------'
//                          |
//                          0
//
// Allowed actions:
//    [R] :  Run
//    [C] :  Continue
//    [N] :  Step, Next

namespace Debugger {
namespace Internal {

IDebuggerEngine *createGdbEngine(DebuggerManager *parent);
IDebuggerEngine *createScriptEngine(DebuggerManager *parent);

// The createWinEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

IDebuggerEngine *createWinEngine(DebuggerManager *, bool /* cmdLineEnabled */, QList<Core::IOptionsPage*> *)
#ifdef CDB_ENABLED
;
#else
{ return 0; }
#endif

} // namespace Internal

DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &p)
{
    QDebug nospace = str.nospace();
    const QString sep = QString(QLatin1Char(','));
    nospace << "executable=" << p.executable << " coreFile=" << p.coreFile
            << " processArgs=" << p.processArgs.join(sep)
            << " environment=<" << p.environment.size() << " variables>"
            << " workingDir=" << p.workingDir << " buildDir=" << p.buildDir
            << " attachPID=" << p.attachPID << " useTerminal=" << p.useTerminal
            << " remoteChannel=" << p.remoteChannel
            << " remoteArchitecture=" << p.remoteArchitecture
            << " symbolFileName=" << p.symbolFileName
            << " serverStartScript=" << p.serverStartScript
            << " toolchain=" << p.toolChainType << '\n';
    return str;
}

using namespace Constants;
using namespace Debugger::Internal;

static const QString tooltipIName = "tooltip";

const char *DebuggerManager::stateName(int s)
{
    #define SN(x) case x: return #x;
    switch (s) {
        SN(DebuggerNotReady)
        SN(EngineStarting)
        SN(AdapterStarting)
        SN(AdapterStarted)
        SN(AdapterStartFailed)
        SN(InferiorStarting)
        SN(InferiorStartFailed)
        SN(InferiorRunningRequested)
        SN(InferiorRunningRequested_Kill)
        SN(InferiorRunning)
        SN(InferiorUnrunnable)
        SN(InferiorStopping)
        SN(InferiorStopping_Kill)
        SN(InferiorStopped)
        SN(InferiorStopFailed)
        SN(InferiorShuttingDown)
        SN(InferiorShutDown)
        SN(InferiorShutdownFailed)
        SN(EngineShuttingDown)
    }
    return "<unknown>";
    #undef SN
}


///////////////////////////////////////////////////////////////////////
//
// DebuggerStartParameters
//
///////////////////////////////////////////////////////////////////////

DebuggerStartParameters::DebuggerStartParameters()
  : attachPID(-1),
    useTerminal(false),
    toolChainType(ProjectExplorer::ToolChain::UNKNOWN),
    startMode(NoStartMode)
{}

void DebuggerStartParameters::clear()
{
    *this = DebuggerStartParameters();
}


///////////////////////////////////////////////////////////////////////
//
// DebuggerManager
//
///////////////////////////////////////////////////////////////////////

static Debugger::Internal::IDebuggerEngine *gdbEngine = 0;
static Debugger::Internal::IDebuggerEngine *scriptEngine = 0;
static Debugger::Internal::IDebuggerEngine *winEngine = 0;

struct DebuggerManagerPrivate
{
    explicit DebuggerManagerPrivate(DebuggerManager *manager);

    static DebuggerManager *instance;

    QIcon m_stopIcon;
    QIcon m_interruptIcon;
    const QIcon m_locationMarkIcon;

    // FIXME: Remove engine-specific state
    DebuggerStartParametersPtr m_startParameters;
    qint64 m_inferiorPid;

    /// Views
    DebuggerMainWindow *m_mainWindow;
    QLabel *m_statusLabel;

    QDockWidget *m_breakDock;
    QDockWidget *m_modulesDock;
    QDockWidget *m_outputDock;
    QDockWidget *m_registerDock;
    QDockWidget *m_snapshotDock;
    QDockWidget *m_sourceFilesDock;
    QDockWidget *m_stackDock;
    QDockWidget *m_threadsDock;
    QDockWidget *m_watchDock;
    QList<QDockWidget *> m_dockWidgets;

    BreakHandler *m_breakHandler;
    ModulesHandler *m_modulesHandler;
    RegisterHandler *m_registerHandler;
    SnapshotHandler *m_snapshotHandler;
    StackHandler *m_stackHandler;
    ThreadsHandler *m_threadsHandler;
    WatchHandler *m_watchHandler;

    DebuggerManagerActions m_actions;

    QWidget *m_breakWindow;
    QWidget *m_localsWindow;
    QWidget *m_registerWindow;
    QWidget *m_modulesWindow;
    QWidget *m_snapshotWindow;
    SourceFilesWindow *m_sourceFilesWindow;
    QWidget *m_stackWindow;
    QWidget *m_threadsWindow;
    QWidget *m_watchersWindow;
    DebuggerOutputWindow *m_outputWindow;

    bool m_busy;
    QTimer *m_statusTimer;
    QString m_lastPermanentStatusMessage;
    DisassemblerViewAgent m_disassemblerViewAgent;

    IDebuggerEngine *m_engine;
    DebuggerState m_state;

    CPlusPlus::Snapshot m_codeModelSnapshot;
};

DebuggerManager *DebuggerManagerPrivate::instance = 0;

DebuggerManagerPrivate::DebuggerManagerPrivate(DebuggerManager *manager) :
   m_stopIcon(QLatin1String(":/debugger/images/debugger_stop_small.png")),
   m_interruptIcon(QLatin1String(":/debugger/images/debugger_interrupt_small.png")),
   m_locationMarkIcon(QLatin1String(":/debugger/images/location.svg")),
   m_startParameters(new DebuggerStartParameters),
   m_inferiorPid(0),
   m_disassemblerViewAgent(manager),
   m_engine(0)
{
    m_interruptIcon.addFile(":/debugger/images/debugger_interrupt.png");
    m_stopIcon.addFile(":/debugger/images/debugger_stop.png");
}

DebuggerManager::DebuggerManager()
  : d(new DebuggerManagerPrivate(this))
{
    DebuggerManagerPrivate::instance = this;
    init();
}

DebuggerManager::~DebuggerManager()
{
    #define doDelete(ptr) delete ptr; ptr = 0
    doDelete(gdbEngine);
    doDelete(scriptEngine);
    doDelete(winEngine);
    #undef doDelete
    DebuggerManagerPrivate::instance = 0;
    delete d;
}

DebuggerManager *DebuggerManager::instance()
{
    return DebuggerManagerPrivate::instance;
}

void DebuggerManager::init()
{
    d->m_state = DebuggerState(-1);
    d->m_busy = false;

    d->m_modulesHandler = 0;
    d->m_registerHandler = 0;

    d->m_statusLabel = new QLabel;
    d->m_statusLabel->setMinimumSize(QSize(30, 10));

    d->m_breakWindow = new BreakWindow(this);
    d->m_modulesWindow = new ModulesWindow(this);
    d->m_outputWindow = new DebuggerOutputWindow;
    d->m_registerWindow = new RegisterWindow(this);
    d->m_snapshotWindow = new SnapshotWindow(this);
    d->m_stackWindow = new StackWindow(this);
    d->m_sourceFilesWindow = new SourceFilesWindow;
    d->m_threadsWindow = new ThreadsWindow;
    d->m_localsWindow = new WatchWindow(WatchWindow::LocalsType, this);
    d->m_watchersWindow = new WatchWindow(WatchWindow::WatchersType, this);
    d->m_statusTimer = new QTimer(this);

    d->m_mainWindow = DebuggerUISwitcher::instance()->mainWindow();

    // Snapshots
    d->m_snapshotHandler = new SnapshotHandler;
    QAbstractItemView *snapshotView =
        qobject_cast<QAbstractItemView *>(d->m_snapshotWindow);
    snapshotView->setModel(d->m_snapshotHandler);

    // Stack
    d->m_stackHandler = new StackHandler;
    QAbstractItemView *stackView =
        qobject_cast<QAbstractItemView *>(d->m_stackWindow);
    stackView->setModel(d->m_stackHandler->stackModel());
    connect(theDebuggerAction(ExpandStack), SIGNAL(triggered()),
        this, SLOT(reloadFullStack()));
    connect(theDebuggerAction(MaximalStackDepth), SIGNAL(triggered()),
        this, SLOT(reloadFullStack()));

    // Threads
    d->m_threadsHandler = new ThreadsHandler;
    QAbstractItemView *threadsView =
        qobject_cast<QAbstractItemView *>(d->m_threadsWindow);
    threadsView->setModel(d->m_threadsHandler->threadsModel());
    connect(threadsView, SIGNAL(threadSelected(int)),
        this, SLOT(selectThread(int)));

    // Breakpoints
    d->m_breakHandler = new BreakHandler(this);
    QAbstractItemView *breakView =
        qobject_cast<QAbstractItemView *>(d->m_breakWindow);
    breakView->setModel(d->m_breakHandler->model());
    connect(breakView, SIGNAL(breakpointActivated(int)),
        d->m_breakHandler, SLOT(activateBreakpoint(int)));
    connect(breakView, SIGNAL(breakpointDeleted(int)),
        d->m_breakHandler, SLOT(removeBreakpoint(int)));
    connect(breakView, SIGNAL(breakpointSynchronizationRequested()),
        this, SLOT(attemptBreakpointSynchronization()));
    connect(breakView, SIGNAL(breakByFunctionRequested(QString)),
        this, SLOT(breakByFunction(QString)), Qt::QueuedConnection);
    connect(breakView, SIGNAL(breakByFunctionMainRequested()),
        this, SLOT(breakByFunctionMain()), Qt::QueuedConnection);

    // Modules
    QAbstractItemView *modulesView =
        qobject_cast<QAbstractItemView *>(d->m_modulesWindow);
    d->m_modulesHandler = new ModulesHandler;
    modulesView->setModel(d->m_modulesHandler->model());
    connect(modulesView, SIGNAL(reloadModulesRequested()),
        this, SLOT(reloadModules()));
    connect(modulesView, SIGNAL(loadSymbolsRequested(QString)),
        this, SLOT(loadSymbols(QString)));
    connect(modulesView, SIGNAL(loadAllSymbolsRequested()),
        this, SLOT(loadAllSymbols()));
    connect(modulesView, SIGNAL(fileOpenRequested(QString)),
        this, SLOT(fileOpen(QString)));
    connect(modulesView, SIGNAL(newDockRequested(QWidget*)),
        this, SLOT(createNewDock(QWidget*)));

    // Source Files
    //d->m_sourceFilesHandler = new SourceFilesHandler;
    QAbstractItemView *sourceFilesView =
        qobject_cast<QAbstractItemView *>(d->m_sourceFilesWindow);
    //sourceFileView->setModel(d->m_stackHandler->stackModel());
    connect(sourceFilesView, SIGNAL(reloadSourceFilesRequested()),
        this, SLOT(reloadSourceFiles()));
    connect(sourceFilesView, SIGNAL(fileOpenRequested(QString)),
        this, SLOT(fileOpen(QString)));

    // Registers
    QAbstractItemView *registerView =
        qobject_cast<QAbstractItemView *>(d->m_registerWindow);
    d->m_registerHandler = new RegisterHandler;
    registerView->setModel(d->m_registerHandler->model());

    // Locals
    d->m_watchHandler = new WatchHandler(this);
    QTreeView *localsView = qobject_cast<QTreeView *>(d->m_localsWindow);
    localsView->setModel(d->m_watchHandler->model(LocalsWatch));

    // Watchers
    QTreeView *watchersView = qobject_cast<QTreeView *>(d->m_watchersWindow);
    watchersView->setModel(d->m_watchHandler->model(WatchersWatch));
    connect(theDebuggerAction(AssignValue), SIGNAL(triggered()),
        this, SLOT(assignValueInDebugger()), Qt::QueuedConnection);

    // Log
    connect(this, SIGNAL(emitShowInput(int, QString)),
            d->m_outputWindow, SLOT(showInput(int, QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(emitShowOutput(int, QString)),
            d->m_outputWindow, SLOT(showOutput(int, QString)), Qt::QueuedConnection);

    // Tooltip
    //QTreeView *tooltipView = qobject_cast<QTreeView *>(d->m_tooltipWindow);
    //tooltipView->setModel(d->m_watchHandler->model(TooltipsWatch));
    qRegisterMetaType<WatchData>("WatchData");
    qRegisterMetaType<StackCookie>("StackCookie");

    d->m_actions.continueAction = new QAction(tr("Continue"), this);
    QIcon continueIcon = QIcon(":/debugger/images/debugger_continue_small.png");
    continueIcon.addFile(":/debugger/images/debugger_continue.png");
    d->m_actions.continueAction->setIcon(continueIcon);

    d->m_actions.stopAction = new QAction(tr("Interrupt"), this);
    d->m_actions.stopAction->setIcon(d->m_interruptIcon);

    d->m_actions.resetAction = new QAction(tr("Abort Debugging"), this);
    d->m_actions.resetAction->setToolTip(tr("Aborts debugging and "
        "resets the debugger to the initial state."));

    d->m_actions.nextAction = new QAction(tr("Step Over"), this);
    d->m_actions.nextAction->setIcon(QIcon(":/debugger/images/debugger_stepover_small.png"));

    d->m_actions.stepAction = new QAction(tr("Step Into"), this);
    d->m_actions.stepAction->setIcon(QIcon(":/debugger/images/debugger_stepinto_small.png"));

    d->m_actions.stepOutAction = new QAction(tr("Step Out"), this);
    d->m_actions.stepOutAction->setIcon(QIcon(":/debugger/images/debugger_stepout_small.png"));

    d->m_actions.runToLineAction1 = new QAction(tr("Run to Line"), this);
    d->m_actions.runToLineAction2 = new QAction(tr("Run to Line"), this);

    d->m_actions.runToFunctionAction =
        new QAction(tr("Run to Outermost Function"), this);

    d->m_actions.returnFromFunctionAction =
        new QAction(tr("Immediately Return From Inner Function"), this);

    d->m_actions.jumpToLineAction1 = new QAction(tr("Jump to Line"), this);
    d->m_actions.jumpToLineAction2 = new QAction(tr("Jump to Line"), this);

    d->m_actions.breakAction = new QAction(tr("Toggle Breakpoint"), this);

    d->m_actions.watchAction1 = new QAction(tr("Add to Watch Window"), this);
    d->m_actions.watchAction2 = new QAction(tr("Add to Watch Window"), this);

    d->m_actions.snapshotAction = new QAction(tr("Snapshot"), this);
    d->m_actions.snapshotAction->setIcon(QIcon(":/debugger/images/debugger_snapshot_small.png"));

    d->m_actions.reverseDirectionAction = new QAction(tr("Reverse Direction"), this);
    d->m_actions.reverseDirectionAction->setCheckable(true);
    d->m_actions.reverseDirectionAction->setChecked(false);

    connect(d->m_actions.continueAction, SIGNAL(triggered()),
        this, SLOT(continueExec()));
    connect(d->m_actions.stopAction, SIGNAL(triggered()),
        this, SLOT(interruptDebuggingRequest()));
    connect(d->m_actions.resetAction, SIGNAL(triggered()),
        this, SLOT(exitDebugger()));
    connect(d->m_actions.nextAction, SIGNAL(triggered()),
        this, SLOT(nextExec()));
    connect(d->m_actions.stepAction, SIGNAL(triggered()),
        this, SLOT(stepExec()));
    connect(d->m_actions.stepOutAction, SIGNAL(triggered()),
        this, SLOT(stepOutExec()));
    connect(d->m_actions.runToLineAction1, SIGNAL(triggered()),
        this, SLOT(runToLineExec()));
    connect(d->m_actions.runToLineAction2, SIGNAL(triggered()),
        this, SLOT(runToLineExec()));
    connect(d->m_actions.runToFunctionAction, SIGNAL(triggered()),
        this, SLOT(runToFunctionExec()));
    connect(d->m_actions.jumpToLineAction1, SIGNAL(triggered()),
        this, SLOT(jumpToLineExec()));
    connect(d->m_actions.jumpToLineAction2, SIGNAL(triggered()),
        this, SLOT(jumpToLineExec()));
    connect(d->m_actions.returnFromFunctionAction, SIGNAL(triggered()),
        this, SLOT(returnExec()));
    connect(d->m_actions.watchAction1, SIGNAL(triggered()),
        this, SLOT(addToWatchWindow()));
    connect(d->m_actions.watchAction2, SIGNAL(triggered()),
        this, SLOT(addToWatchWindow()));
    connect(d->m_actions.breakAction, SIGNAL(triggered()),
        this, SLOT(toggleBreakpoint()));
    connect(d->m_actions.snapshotAction, SIGNAL(triggered()),
        this, SLOT(makeSnapshot()));

    connect(d->m_statusTimer, SIGNAL(timeout()),
        this, SLOT(clearStatusMessage()));

    connect(theDebuggerAction(ExecuteCommand), SIGNAL(triggered()),
        this, SLOT(executeDebuggerCommand()));

    connect(theDebuggerAction(WatchPoint), SIGNAL(triggered()),
        this, SLOT(watchPoint()));

    connect(theDebuggerAction(OperateByInstruction), SIGNAL(triggered()),
        this, SLOT(operateByInstructionTriggered()));

    DebuggerUISwitcher *uiSwitcher = DebuggerUISwitcher::instance();
    d->m_breakDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_breakWindow);
    d->m_modulesDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_modulesWindow,
                                                                     Qt::TopDockWidgetArea, false);

    connect(d->m_modulesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadModules()), Qt::QueuedConnection);

    d->m_registerDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_registerWindow,
                                                                      Qt::TopDockWidgetArea, false);
    connect(d->m_registerDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadRegisters()), Qt::QueuedConnection);

    d->m_outputDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_outputWindow,
                                                                    Qt::TopDockWidgetArea, false);

    d->m_snapshotDock =  uiSwitcher->createDockWidget(LANG_CPP, d->m_snapshotWindow);
    d->m_stackDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_stackWindow);
    d->m_sourceFilesDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_sourceFilesWindow,
                                                                         Qt::TopDockWidgetArea, false);
    connect(d->m_sourceFilesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadSourceFiles()), Qt::QueuedConnection);

    d->m_threadsDock = uiSwitcher->createDockWidget(LANG_CPP, d->m_threadsWindow);

    QSplitter *localsAndWatchers = new Core::MiniSplitter(Qt::Vertical);
    localsAndWatchers->setWindowTitle(d->m_localsWindow->windowTitle());
    localsAndWatchers->addWidget(d->m_localsWindow);
    localsAndWatchers->addWidget(d->m_watchersWindow);
    //localsAndWatchers->addWidget(d->m_tooltipWindow);
    localsAndWatchers->setStretchFactor(0, 3);
    localsAndWatchers->setStretchFactor(1, 1);
    localsAndWatchers->setStretchFactor(2, 1);
    d->m_watchDock = DebuggerUISwitcher::instance()->createDockWidget(LANG_CPP, localsAndWatchers);
    d->m_dockWidgets << d->m_breakDock << d->m_modulesDock << d->m_registerDock
                     << d->m_outputDock << d->m_stackDock << d->m_sourceFilesDock
                     << d->m_threadsDock << d->m_watchDock;

    setState(DebuggerNotReady);
}

QList<Core::IOptionsPage*> DebuggerManager::initializeEngines(unsigned enabledTypeFlags)
{
    QList<Core::IOptionsPage*> rc;

    if (enabledTypeFlags & GdbEngineType) {
        gdbEngine = createGdbEngine(this);
        gdbEngine->addOptionPages(&rc);
    }

    winEngine = createWinEngine(this, (enabledTypeFlags & CdbEngineType), &rc);

    if (enabledTypeFlags & ScriptEngineType) {
        scriptEngine = createScriptEngine(this);
        scriptEngine->addOptionPages(&rc);
    }

    d->m_engine = 0;
    STATE_DEBUG(gdbEngine << winEngine << scriptEngine << rc.size());
    return rc;
}

DebuggerManagerActions DebuggerManager::debuggerManagerActions() const
{
    return d->m_actions;
}

QLabel *DebuggerManager::statusLabel() const
{
    return d->m_statusLabel;
}

IDebuggerEngine *DebuggerManager::currentEngine() const
{
    return d->m_engine;
}

ModulesHandler *DebuggerManager::modulesHandler() const
{
    return d->m_modulesHandler;
}

BreakHandler *DebuggerManager::breakHandler() const
{
    return d->m_breakHandler;
}

RegisterHandler *DebuggerManager::registerHandler() const
{
    return d->m_registerHandler;
}

StackHandler *DebuggerManager::stackHandler() const
{
    return d->m_stackHandler;
}

ThreadsHandler *DebuggerManager::threadsHandler() const
{
    return d->m_threadsHandler;
}

WatchHandler *DebuggerManager::watchHandler() const
{
    return d->m_watchHandler;
}

SnapshotHandler *DebuggerManager::snapshotHandler() const
{
    return d->m_snapshotHandler;
}

const CPlusPlus::Snapshot &DebuggerManager::cppCodeModelSnapshot() const
{
    if (d->m_codeModelSnapshot.isEmpty() && theDebuggerAction(UseCodeModel)->isChecked())
        d->m_codeModelSnapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
    return d->m_codeModelSnapshot;
}

void DebuggerManager::clearCppCodeModelSnapshot()
{
    d->m_codeModelSnapshot = CPlusPlus::Snapshot();
}

SourceFilesWindow *DebuggerManager::sourceFileWindow() const
{
    return d->m_sourceFilesWindow;
}

QWidget *DebuggerManager::threadsWindow() const
{
    return d->m_threadsWindow;
}

void DebuggerManager::createNewDock(QWidget *widget)
{
    QDockWidget *dockWidget = DebuggerUISwitcher::instance()->createDockWidget(LANG_CPP, widget);
    dockWidget->setWindowTitle(widget->windowTitle());
    dockWidget->setObjectName(widget->windowTitle());
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    //dockWidget->setWidget(widget);
    //d->m_mainWindow->addDockWidget(Qt::TopDockWidgetArea, dockWidget);
    dockWidget->show();
}

void DebuggerManager::setSimpleDockWidgetArrangement(const QString &activeLanguage)
{
    if (activeLanguage == LANG_CPP || !activeLanguage.length()) {
        d->m_mainWindow->setTrackingEnabled(false);
        QList<QDockWidget *> dockWidgets = d->m_mainWindow->dockWidgets();
        foreach (QDockWidget *dockWidget, dockWidgets) {
            if (d->m_dockWidgets.contains(dockWidget)) {
                dockWidget->setFloating(false);
                d->m_mainWindow->removeDockWidget(dockWidget);
            }
        }

        foreach (QDockWidget *dockWidget, dockWidgets) {
            if (d->m_dockWidgets.contains(dockWidget)) {
                if (dockWidget == d->m_outputDock)
                    d->m_mainWindow->addDockWidget(Qt::TopDockWidgetArea, dockWidget);
                else
                    d->m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
                dockWidget->show();
            }
        }

        d->m_mainWindow->tabifyDockWidget(d->m_watchDock, d->m_breakDock);
        d->m_mainWindow->tabifyDockWidget(d->m_watchDock, d->m_modulesDock);
        d->m_mainWindow->tabifyDockWidget(d->m_watchDock, d->m_registerDock);
        d->m_mainWindow->tabifyDockWidget(d->m_watchDock, d->m_threadsDock);
        d->m_mainWindow->tabifyDockWidget(d->m_watchDock, d->m_sourceFilesDock);
        d->m_mainWindow->tabifyDockWidget(d->m_watchDock, d->m_snapshotDock);
        // They following views are rarely used in ordinary debugging. Hiding them
        // saves cycles since the corresponding information won't be retrieved.
        d->m_sourceFilesDock->hide();
        d->m_registerDock->hide();
        d->m_modulesDock->hide();
        d->m_outputDock->hide();
        d->m_mainWindow->setTrackingEnabled(true);
    }
}

QAbstractItemModel *DebuggerManager::threadsModel()
{
    return qobject_cast<ThreadsWindow*>(d->m_threadsWindow)->model();
}

void DebuggerManager::clearStatusMessage()
{
    d->m_statusLabel->setText(d->m_lastPermanentStatusMessage);
}

void DebuggerManager::showStatusMessage(const QString &msg, int timeout)
{
    Q_UNUSED(timeout)
    showDebuggerOutput(LogStatus, msg);
    d->m_statusLabel->setText(QLatin1String("   ") + msg);
    if (timeout > 0) {
        d->m_statusTimer->setSingleShot(true);
        d->m_statusTimer->start(timeout);
    } else {
        d->m_lastPermanentStatusMessage = msg;
        d->m_statusTimer->stop();
    }
}

void DebuggerManager::notifyInferiorStopped()
{
    setState(InferiorStopped);
    showStatusMessage(tr("Stopped."), 5000);
}

void DebuggerManager::notifyInferiorRunning()
{
    setState(InferiorRunning);
    showStatusMessage(tr("Running..."), 5000);
}

void DebuggerManager::notifyInferiorExited()
{
    setState(DebuggerNotReady);
    showStatusMessage(tr("Exited."), 5000);
}

void DebuggerManager::notifyInferiorPidChanged(qint64 pid)
{
    STATE_DEBUG(d->m_inferiorPid << pid);

    if (d->m_inferiorPid != pid) {
        d->m_inferiorPid = pid;
        emit inferiorPidChanged(pid);
    }
}

void DebuggerManager::showApplicationOutput(const QString &str)
{
     emit applicationOutputAvailable(str);
}

void DebuggerManager::shutdown()
{
    STATE_DEBUG(d->m_engine);
    if (d->m_engine)
        d->m_engine->shutdown();
    d->m_engine = 0;

    #define doDelete(ptr) delete ptr; ptr = 0
    doDelete(scriptEngine);
    doDelete(gdbEngine);
    doDelete(winEngine);

    // Delete these manually before deleting the manager
    // (who will delete the models for most views)
    doDelete(d->m_breakWindow);
    doDelete(d->m_modulesWindow);
    doDelete(d->m_outputWindow);
    doDelete(d->m_registerWindow);
    doDelete(d->m_stackWindow);
    doDelete(d->m_sourceFilesWindow);
    doDelete(d->m_threadsWindow);
    //doDelete(d->m_tooltipWindow);
    doDelete(d->m_watchersWindow);
    doDelete(d->m_localsWindow);

    doDelete(d->m_breakHandler);
    doDelete(d->m_threadsHandler);
    doDelete(d->m_modulesHandler);
    doDelete(d->m_registerHandler);
    doDelete(d->m_snapshotHandler);
    doDelete(d->m_stackHandler);
    doDelete(d->m_watchHandler);
    #undef doDelete
}

void DebuggerManager::makeSnapshot()
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->makeSnapshot();
}

void DebuggerManager::activateSnapshot(int index)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->activateSnapshot(index);
}

void DebuggerManager::removeSnapshot(int index)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_snapshotHandler->removeSnapshot(index);
}

BreakpointData *DebuggerManager::findBreakpoint(const QString &fileName, int lineNumber)
{
    if (!d->m_breakHandler)
        return 0;
    int index = d->m_breakHandler->findBreakpoint(fileName, lineNumber);
    return index == -1 ? 0 : d->m_breakHandler->at(index);
}

void DebuggerManager::toggleBreakpoint()
{
    QString fileName;
    int lineNumber = -1;
    queryCurrentTextEditor(&fileName, &lineNumber, 0);
    if (lineNumber == -1)
        return;
    toggleBreakpoint(fileName, lineNumber);
}

void DebuggerManager::toggleBreakpoint(const QString &fileName, int lineNumber)
{
    STATE_DEBUG(fileName << lineNumber);
    QTC_ASSERT(d->m_breakHandler, return);
    if (state() != InferiorRunning
         && state() != InferiorStopped
         && state() != DebuggerNotReady) {
        showStatusMessage(tr("Changing breakpoint state requires either a "
            "fully running or fully stopped application."));
        return;
    }

    int index = d->m_breakHandler->findBreakpoint(fileName, lineNumber);
    if (index == -1)
        d->m_breakHandler->setBreakpoint(fileName, lineNumber);
    else
        d->m_breakHandler->removeBreakpoint(index);

    attemptBreakpointSynchronization();
}

void DebuggerManager::toggleBreakpointEnabled(const QString &fileName, int lineNumber)
{
    STATE_DEBUG(fileName << lineNumber);
    QTC_ASSERT(d->m_breakHandler, return);
    if (state() != InferiorRunning
         && state() != InferiorStopped
         && state() != DebuggerNotReady) {
        showStatusMessage(tr("Changing breakpoint state requires either a "
            "fully running or fully stopped application."));
        return;
    }

    d->m_breakHandler->toggleBreakpointEnabled(fileName, lineNumber);

    attemptBreakpointSynchronization();
}

void DebuggerManager::attemptBreakpointSynchronization()
{
    if (d->m_engine)
        d->m_engine->attemptBreakpointSynchronization();
}

void DebuggerManager::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    if (d->m_engine)
        d->m_engine->setToolTipExpression(mousePos, editor, cursorPos);
}

void DebuggerManager::updateWatchData(const Debugger::Internal::WatchData &data)
{
    if (d->m_engine)
        d->m_engine->updateWatchData(data);
}

static QString msgEngineNotAvailable(const char *engine)
{
    return DebuggerManager::tr("The application requires the debugger engine '%1', "
        "which is disabled.").arg(QLatin1String(engine));
}

static IDebuggerEngine *debuggerEngineForToolChain(ProjectExplorer::ToolChain::ToolChainType tc)
{
    IDebuggerEngine *rc = 0;
    switch (tc) {
    //case ProjectExplorer::ToolChain::LinuxICC:
    case ProjectExplorer::ToolChain::MinGW:
    case ProjectExplorer::ToolChain::GCC:
        rc = gdbEngine;
        break;
    case ProjectExplorer::ToolChain::MSVC:
    case ProjectExplorer::ToolChain::WINCE:
        rc = winEngine;
        break;
    case ProjectExplorer::ToolChain::WINSCW: // S60
    case ProjectExplorer::ToolChain::GCCE:
    case ProjectExplorer::ToolChain::RVCT_ARMV5:
    case ProjectExplorer::ToolChain::RVCT_ARMV6:
        rc = gdbEngine;
        break;
    case ProjectExplorer::ToolChain::OTHER:
    case ProjectExplorer::ToolChain::UNKNOWN:
    case ProjectExplorer::ToolChain::INVALID:
    default:
        break;
    }
    return rc;
}

// Figure out the debugger type of an executable. Analyze executable
// unless the toolchain provides a hint.
static IDebuggerEngine *determineDebuggerEngine(const QString &executable,
                                                int toolChainType,
                                                QString *errorMessage,
                                                QString *settingsIdHint)
{
    if (executable.endsWith(_(".js"))) {
        if (!scriptEngine) {
            *errorMessage = msgEngineNotAvailable("Script Engine");
            return 0;
        }
        return scriptEngine;
    }

/*
    if (executable.endsWith(_(".sym"))) {
        if (!gdbEngine) {
            *errorMessage = msgEngineNotAvailable("Gdb Engine");
            return 0;
        }
        return gdbEngine;
    }
*/

    if (IDebuggerEngine *tce = debuggerEngineForToolChain(
            static_cast<ProjectExplorer::ToolChain::ToolChainType>(toolChainType)))
        return tce;

#ifndef Q_OS_WIN
    Q_UNUSED(settingsIdHint)
    if (!gdbEngine) {
        *errorMessage = msgEngineNotAvailable("Gdb Engine");
        return 0;
    }

    return gdbEngine;
#else
    // A remote executable?
    if (!executable.endsWith(_(".exe")))
        return gdbEngine;
    // If a file has PDB files, it has been compiled by VS.
    QStringList pdbFiles;
    if (!getPDBFiles(executable, &pdbFiles, errorMessage))
        return 0;
    if (pdbFiles.empty())
        return gdbEngine;

    // We need the CDB debugger in order to be able to debug VS
    // executables
    if (!DebuggerManager::instance()->checkDebugConfiguration(ProjectExplorer::ToolChain::MSVC, errorMessage, 0 , settingsIdHint))
        return 0;
    return winEngine;
#endif
}

// Figure out the debugger type of a PID
static IDebuggerEngine *determineDebuggerEngine(int  /* pid */,
                                                int toolChainType,
                                                QString *errorMessage)
{
    if (IDebuggerEngine *tce = debuggerEngineForToolChain(
            static_cast<ProjectExplorer::ToolChain::ToolChainType>(toolChainType)))
        return tce;
#ifdef Q_OS_WIN
    // Preferably Windows debugger
    if (winEngine)
        return winEngine;
    if (gdbEngine)
        return gdbEngine;
    *errorMessage = msgEngineNotAvailable("Gdb Engine");
    return 0;
#else
    if (!gdbEngine) {
        *errorMessage = msgEngineNotAvailable("Gdb Engine");
        return 0;
    }

    return gdbEngine;
#endif
}

void DebuggerManager::startNewDebugger(const DebuggerStartParametersPtr &sp)
{
    if (d->m_state != DebuggerNotReady)
        return;
    d->m_startParameters = sp;
    d->m_inferiorPid = d->m_startParameters->attachPID > 0
        ? d->m_startParameters->attachPID : 0;
    const QString toolChainName = ProjectExplorer::ToolChain::toolChainName(static_cast<ProjectExplorer::ToolChain::ToolChainType>(d->m_startParameters->toolChainType));

    emit debugModeRequested();
    showDebuggerOutput(LogStatus,
        tr("Starting debugger for tool chain '%1'...").arg(toolChainName));
    showDebuggerOutput(LogDebug, DebuggerSettings::instance()->dump());

    QString errorMessage;
    QString settingsIdHint;
    switch (d->m_startParameters->startMode) {
    case AttachExternal:
    case AttachCrashedExternal:
        d->m_engine = determineDebuggerEngine(d->m_startParameters->attachPID,
            d->m_startParameters->toolChainType, &errorMessage);
        break;
    default:
        d->m_engine = determineDebuggerEngine(d->m_startParameters->executable,
            d->m_startParameters->toolChainType, &errorMessage, &settingsIdHint);
        break;
    }

    if (!d->m_engine) {
        emit debuggingFinished();
        // Create Message box with possibility to go to settings
        const QString msg = tr("Cannot debug '%1' (tool chain: '%2'): %3").
            arg(d->m_startParameters->executable, toolChainName, errorMessage);
        Core::ICore::instance()->showWarningWithOptions(tr("Warning"), msg, QString(),
            QLatin1String(DEBUGGER_SETTINGS_CATEGORY), settingsIdHint);
        return;
    }

    STATE_DEBUG(d->m_startParameters->executable << d->m_engine);
    setBusyCursor(false);
    setState(EngineStarting);
    connect(d->m_engine, SIGNAL(startFailed()), this, SLOT(startFailed()));
    d->m_engine->startDebugger(d->m_startParameters);

    const unsigned engineCapabilities = d->m_engine->debuggerCapabilities();
    theDebuggerAction(OperateByInstruction)
        ->setEnabled(engineCapabilities & DisassemblerCapability);
    d->m_actions.reverseDirectionAction
        ->setEnabled(engineCapabilities & ReverseSteppingCapability);
}

void DebuggerManager::startFailed()
{
    disconnect(d->m_engine, SIGNAL(startFailed()), this, SLOT(startFailed()));
    setState(DebuggerNotReady);
    emit debuggingFinished();
}

void DebuggerManager::cleanupViews()
{
    resetLocation();
    breakHandler()->setAllPending();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    modulesHandler()->removeAll();
    watchHandler()->cleanup();
    registerHandler()->removeAll();
    d->m_sourceFilesWindow->removeAll();
    d->m_disassemblerViewAgent.cleanup();
}

void DebuggerManager::exitDebugger()
{
    // The engine will finally call setState(DebuggerNotReady) which
    // in turn will handle the cleanup.
    if (d->m_engine && state() != DebuggerNotReady)
        d->m_engine->exitDebugger();
    d->m_codeModelSnapshot = CPlusPlus::Snapshot();
}

DebuggerStartParametersPtr DebuggerManager::startParameters() const
{
    return d->m_startParameters;
}

qint64 DebuggerManager::inferiorPid() const
{
    return d->m_inferiorPid;
}

void DebuggerManager::assignValueInDebugger()
{
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        QString str = action->data().toString();
        int i = str.indexOf('=');
        if (i != -1)
            assignValueInDebugger(str.left(i), str.mid(i + 1));
    }
}

void DebuggerManager::assignValueInDebugger(const QString &expr, const QString &value)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->assignValueInDebugger(expr, value);
}

void DebuggerManager::activateFrame(int index)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->activateFrame(index);
}

void DebuggerManager::selectThread(int index)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->selectThread(index);
}

void DebuggerManager::loadAllSymbols()
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->loadAllSymbols();
}

void DebuggerManager::loadSymbols(const QString &module)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->loadSymbols(module);
}

QList<Symbol> DebuggerManager::moduleSymbols(const QString &moduleName)
{
    QTC_ASSERT(d->m_engine, return QList<Symbol>());
    return d->m_engine->moduleSymbols(moduleName);
}

void DebuggerManager::stepExec()
{
    QTC_ASSERT(d->m_engine, return);
    resetLocation();
    if (theDebuggerBoolSetting(OperateByInstruction))
        d->m_engine->stepIExec();
    else
        d->m_engine->stepExec();
}

void DebuggerManager::stepOutExec()
{
    QTC_ASSERT(d->m_engine, return);
    resetLocation();
    d->m_engine->stepOutExec();
}

void DebuggerManager::nextExec()
{
    QTC_ASSERT(d->m_engine, return);
    resetLocation();
    if (theDebuggerBoolSetting(OperateByInstruction))
        d->m_engine->nextIExec();
    else
        d->m_engine->nextExec();
}

void DebuggerManager::returnExec()
{
    QTC_ASSERT(d->m_engine, return);
    resetLocation();
    d->m_engine->returnExec();
}

void DebuggerManager::watchPoint()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        if (d->m_engine)
            d->m_engine->watchPoint(action->data().toPoint());
}

void DebuggerManager::executeDebuggerCommand()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        executeDebuggerCommand(action->data().toString());
}

void DebuggerManager::executeDebuggerCommand(const QString &command)
{
    STATE_DEBUG(command);
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->executeDebuggerCommand(command);
}

void DebuggerManager::sessionLoaded()
{
    loadSessionData();
}

void DebuggerManager::aboutToUnloadSession()
{
    if (d->m_engine)
        d->m_engine->shutdown();
}

void DebuggerManager::aboutToSaveSession()
{
    saveSessionData();
}

void DebuggerManager::loadSessionData()
{
    d->m_breakHandler->loadSessionData();
    d->m_watchHandler->loadSessionData();
}

void DebuggerManager::saveSessionData()
{
    d->m_breakHandler->saveSessionData();
    d->m_watchHandler->saveSessionData();
}

void DebuggerManager::dumpLog()
{
    QString fileName = QFileDialog::getSaveFileName(d->m_mainWindow,
        tr("Save Debugger Log"), QDir::tempPath());
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QTextStream ts(&file);
    ts << d->m_outputWindow->inputContents();
    ts << "\n\n=======================================\n\n";
    ts << d->m_outputWindow->combinedContents();
}

void DebuggerManager::addToWatchWindow()
{
    using namespace Core;
    using namespace TextEditor;
    // requires a selection, but that's the only case we want...
    EditorManager *editorManager = EditorManager::instance();
    if (!editorManager)
        return;
    IEditor *editor = editorManager->currentEditor();
    if (!editor)
        return;
    ITextEditor *textEditor = qobject_cast<ITextEditor*>(editor);
    if (!textEditor)
        return;
    QTextCursor tc;
    QPlainTextEdit *ptEdit = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (ptEdit)
        tc = ptEdit->textCursor();
    QString exp;
    if (tc.hasSelection()) {
        exp = tc.selectedText();
    } else {
        int line, column;
        exp = cppExpressionAt(textEditor, tc.position(), &line, &column);
    }
    if (!exp.isEmpty())
        d->m_watchHandler->watchExpression(exp);
}

void DebuggerManager::setBreakpoint(const QString &fileName, int lineNumber)
{
    STATE_DEBUG(Q_FUNC_INFO << fileName << lineNumber);
    QTC_ASSERT(d->m_breakHandler, return);
    d->m_breakHandler->setBreakpoint(fileName, lineNumber);
    attemptBreakpointSynchronization();
}

void DebuggerManager::breakByFunctionMain()
{
#ifdef Q_OS_WIN
    // FIXME: wrong on non-Qt based binaries
    emit breakByFunction("qMain");
#else
    emit breakByFunction("main");
#endif
}

void DebuggerManager::breakByFunction(const QString &functionName)
{
    QTC_ASSERT(d->m_breakHandler, return);
    d->m_breakHandler->breakByFunction(functionName);
    attemptBreakpointSynchronization();
}

void DebuggerManager::setBusyCursor(bool busy)
{
    //STATE_DEBUG("BUSY FROM: " << d->m_busy << " TO: " << d->m_busy);
    if (busy == d->m_busy)
        return;
    d->m_busy = busy;

    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    d->m_breakWindow->setCursor(cursor);
    d->m_localsWindow->setCursor(cursor);
    d->m_modulesWindow->setCursor(cursor);
    d->m_outputWindow->setCursor(cursor);
    d->m_registerWindow->setCursor(cursor);
    d->m_stackWindow->setCursor(cursor);
    d->m_sourceFilesWindow->setCursor(cursor);
    d->m_threadsWindow->setCursor(cursor);
    //d->m_tooltipWindow->setCursor(cursor);
    d->m_watchersWindow->setCursor(cursor);
}

void DebuggerManager::queryCurrentTextEditor(QString *fileName, int *lineNumber,
    QObject **object)
{
    emit currentTextEditorRequested(fileName, lineNumber, object);
}

void DebuggerManager::continueExec()
{
    if (d->m_engine)
        d->m_engine->continueInferior();
}

void DebuggerManager::detachDebugger()
{
    if (d->m_engine)
        d->m_engine->detachDebugger();
}

void DebuggerManager::interruptDebuggingRequest()
{
    STATE_DEBUG(state());
    if (!d->m_engine)
        return;
    bool interruptIsExit = (state() != InferiorRunning);
    if (interruptIsExit) {
        exitDebugger();
    } else {
        d->m_engine->interruptInferior();
    }
}

void DebuggerManager::runToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (d->m_engine && !fileName.isEmpty()) {
        STATE_DEBUG(fileName << lineNumber);
        d->m_engine->runToLineExec(fileName, lineNumber);
    }
}

void DebuggerManager::runToFunctionExec()
{
    QString fileName;
    int lineNumber = -1;
    QObject *object = 0;
    emit currentTextEditorRequested(&fileName, &lineNumber, &object);
    QPlainTextEdit *ed = qobject_cast<QPlainTextEdit*>(object);
    if (!ed)
        return;
    QTextCursor cursor = ed->textCursor();
    QString functionName = cursor.selectedText();
    if (functionName.isEmpty()) {
        const QTextBlock block = cursor.block();
        const QString line = block.text();
        foreach (const QString &str, line.trimmed().split('(')) {
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
    STATE_DEBUG(functionName);

    if (d->m_engine && !functionName.isEmpty())
        d->m_engine->runToFunctionExec(functionName);
}

void DebuggerManager::jumpToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (d->m_engine && !fileName.isEmpty()) {
        STATE_DEBUG(fileName << lineNumber);
        d->m_engine->jumpToLineExec(fileName, lineNumber);
    }
}

void DebuggerManager::resetLocation()
{
    d->m_disassemblerViewAgent.resetLocation();
    d->m_stackHandler->setCurrentIndex(-1);
    // Connected to the plugin.
    emit resetLocationRequested();
}

void DebuggerManager::gotoLocation(const Debugger::Internal::StackFrame &frame, bool setMarker)
{
    if (theDebuggerBoolSetting(OperateByInstruction) || !frame.isUsable()) {
        if (setMarker)
            emit resetLocationRequested();
        d->m_disassemblerViewAgent.setFrame(frame);
    } else {
        // Connected to the plugin.
        emit gotoLocationRequested(frame.file, frame.line, setMarker);
    }
}

void DebuggerManager::fileOpen(const QString &fileName)
{
    StackFrame frame;
    frame.file = fileName;
    frame.line = -1;
    gotoLocation(frame, false);
}

void DebuggerManager::operateByInstructionTriggered()
{
    QTC_ASSERT(d->m_stackHandler, return);
    StackFrame frame = d->m_stackHandler->currentFrame();
    gotoLocation(frame, true);
}


//////////////////////////////////////////////////////////////////////
//
// Source files specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadSourceFiles()
{
    if (d->m_engine && d->m_sourceFilesDock && d->m_sourceFilesDock->isVisible())
        d->m_engine->reloadSourceFiles();
}

void DebuggerManager::sourceFilesDockToggled(bool on)
{
    if (on)
        reloadSourceFiles();
}


//////////////////////////////////////////////////////////////////////
//
// Modules specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadModules()
{
    if (d->m_engine && d->m_modulesDock && d->m_modulesDock->isVisible())
        d->m_engine->reloadModules();
}

void DebuggerManager::modulesDockToggled(bool on)
{
    if (on)
        reloadModules();
}


//////////////////////////////////////////////////////////////////////
//
// Output specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::showDebuggerOutput(int channel, const QString &msg)
{
    if (d->m_outputWindow) {
        emit emitShowOutput(channel, msg);
        if (channel == LogError)
            ensureLogVisible();
    } else  {
        qDebug() << "OUTPUT: " << channel << msg;

    }
}

void DebuggerManager::showDebuggerInput(int channel, const QString &msg)
{
    if (d->m_outputWindow)
        emit emitShowInput(channel, msg);
    else
        qDebug() << "INPUT: " << channel << msg;
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::registerDockToggled(bool on)
{
    if (on)
        reloadRegisters();
}

void DebuggerManager::reloadRegisters()
{
    if (d->m_engine && d->m_registerDock && d->m_registerDock->isVisible())
        d->m_engine->reloadRegisters();
}

//////////////////////////////////////////////////////////////////////
//
// Dumpers. "Custom dumpers" are a library compiled against the current
// Qt containing functions to evaluate values of Qt classes
// (such as QString, taking pointers to their addresses).
// The library must be loaded into the debuggee.
//
//////////////////////////////////////////////////////////////////////

bool DebuggerManager::qtDumperLibraryEnabled() const
{
    return theDebuggerBoolSetting(UseDebuggingHelpers);
}

QString DebuggerManager::qtDumperLibraryName() const
{
    if (theDebuggerAction(UseCustomDebuggingHelperLocation)->value().toBool())
        return theDebuggerAction(CustomDebuggingHelperLocation)->value().toString();
    return d->m_startParameters->dumperLibrary;
}

QStringList DebuggerManager::qtDumperLibraryLocations() const
{
    if (theDebuggerAction(UseCustomDebuggingHelperLocation)->value().toBool()) {
        const QString customLocation =
            theDebuggerAction(CustomDebuggingHelperLocation)->value().toString();
        const QString location =
            tr("%1 (explicitly set in the Debugger Options)").arg(customLocation);
        return QStringList(location);
    }
    return d->m_startParameters->dumperLibraryLocations;
}

void DebuggerManager::showQtDumperLibraryWarning(const QString &details)
{
    QMessageBox dialog(d->m_mainWindow);
    QPushButton *qtPref = dialog.addButton(tr("Open Qt preferences"),
        QMessageBox::ActionRole);
    QPushButton *helperOff = dialog.addButton(tr("Turn off helper usage"),
        QMessageBox::ActionRole);
    QPushButton *justContinue = dialog.addButton(tr("Continue anyway"),
        QMessageBox::AcceptRole);
    dialog.setDefaultButton(justContinue);
    dialog.setWindowTitle(tr("Debugging helper missing"));
    dialog.setText(tr("The debugger could not load the debugging helper library."));
    dialog.setInformativeText(tr(
        "The debugging helper is used to nicely format the values of some Qt "
        "and Standard Library data types. "
        "It must be compiled for each used Qt version separately. "
        "This can be done in the Qt preferences page by selecting a Qt installation "
        "and clicking on 'Rebuild' in the 'Debugging Helper' row."));
    if (!details.isEmpty())
        dialog.setDetailedText(details);
    dialog.exec();
    if (dialog.clickedButton() == qtPref) {
        Core::ICore::instance()->showOptionsDialog(_(Qt4ProjectManager::Constants::QT_SETTINGS_CATEGORY),
                                                   _(Qt4ProjectManager::Constants::QTVERSION_SETTINGS_PAGE_ID));
    } else if (dialog.clickedButton() == helperOff) {
        theDebuggerAction(UseDebuggingHelpers)->setValue(qVariantFromValue(false), false);
    }
}

void DebuggerManager::reloadFullStack()
{
    if (d->m_engine)
        d->m_engine->reloadFullStack();
}

void DebuggerManager::setRegisterValue(int nr, const QString &value)
{
    if (d->m_engine)
        d->m_engine->setRegisterValue(nr, value);
}

bool DebuggerManager::isReverseDebugging() const
{
    return d->m_actions.reverseDirectionAction->isChecked();
}

QVariant DebuggerManager::sessionValue(const QString &name)
{
    // this is answered by the plugin
    QVariant value;
    emit sessionValueRequested(name, &value);
    return value;
}

void DebuggerManager::setSessionValue(const QString &name, const QVariant &value)
{
    emit setSessionValueRequested(name, value);
}

QMessageBox *DebuggerManager::showMessageBox(int icon, const QString &title,
    const QString &text, int buttons)
{
    QMessageBox *mb = new QMessageBox(QMessageBox::Icon(icon),
        title, text, QMessageBox::StandardButtons(buttons), d->m_mainWindow);
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
    return mb;
}

DebuggerState DebuggerManager::state() const
{
    return d->m_state;
}

static bool isAllowedTransition(int from, int to)
{
    switch (from) {
    case -1:
        return to == DebuggerNotReady;

    case DebuggerNotReady:
        return to == EngineStarting || to == DebuggerNotReady;

    case EngineStarting:
        return to == AdapterStarting || to == DebuggerNotReady;

    case AdapterStarting:
        return to == AdapterStarted || to == AdapterStartFailed;
    case AdapterStarted:
        return to == InferiorStarting || to == EngineShuttingDown;
    case AdapterStartFailed:
        return to == DebuggerNotReady;

    case InferiorStarting:
        return to == InferiorRunningRequested || to == InferiorStopped
            || to == InferiorStartFailed || to == InferiorUnrunnable;
    case InferiorStartFailed:
        return to == EngineShuttingDown;

    case InferiorRunningRequested:
        return to == InferiorRunning || to == InferiorStopped
            || to == InferiorRunningRequested_Kill;
    case InferiorRunningRequested_Kill:
        return to == InferiorRunning || to == InferiorStopped;
    case InferiorRunning:
        return to == InferiorStopping;

    case InferiorStopping:
        return to == InferiorStopped || to == InferiorStopFailed
            || to == InferiorStopping_Kill;
    case InferiorStopping_Kill:
        return to == InferiorStopped || to == InferiorStopFailed;
    case InferiorStopped:
        return to == InferiorRunningRequested || to == InferiorShuttingDown;
    case InferiorStopFailed:
        return to == EngineShuttingDown;

    case InferiorUnrunnable:
        return to == EngineShuttingDown;
    case InferiorShuttingDown:
        return to == InferiorShutDown || to == InferiorShutdownFailed;
    case InferiorShutDown:
        return to == EngineShuttingDown;
    case InferiorShutdownFailed:
        return to == EngineShuttingDown;

    case EngineShuttingDown:
        return to == DebuggerNotReady;
    }

    qDebug() << "UNKNOWN STATE:" << from;
    return false;
}

void DebuggerManager::setState(DebuggerState state, bool forced)
{
    //STATE_DEBUG("STATUS CHANGE: FROM " << stateName(d->m_state)
    //        << " TO " << stateName(state));

    QString msg = _("State changed from %1(%2) to %3(%4).")
        .arg(stateName(d->m_state)).arg(d->m_state).arg(stateName(state)).arg(state);
    //if (!((d->m_state == -1 && state == 0) || (d->m_state == 0 && state == 0)))
    //    qDebug() << msg;
    if (!forced && !isAllowedTransition(d->m_state, state))
        qDebug() << "UNEXPECTED STATE TRANSITION: " << msg;

    showDebuggerOutput(LogDebug, msg);

    //resetLocation();
    if (state == d->m_state)
        return;

    d->m_state = state;

    //if (d->m_state == InferiorStopped)
    //    resetLocation();

    if (d->m_state == DebuggerNotReady) {
        setBusyCursor(false);
        cleanupViews();
        emit debuggingFinished();
    }

    const bool stoppable = state == InferiorRunning
        || state == InferiorRunningRequested
        || state == InferiorStopping
        || state == InferiorStopped
        || state == InferiorUnrunnable;

    const bool running = state == InferiorRunning;
    if (running)
        threadsHandler()->notifyRunning();
    const bool stopped = state == InferiorStopped;

    if (stopped)
        QApplication::alert(d->m_mainWindow, 3000);

    const bool actionsEnabled = debuggerActionsEnabled();
    const unsigned engineCapabilities = debuggerCapabilities();
    d->m_actions.watchAction1->setEnabled(true);
    d->m_actions.watchAction2->setEnabled(true);
    d->m_actions.breakAction->setEnabled(true);
    d->m_actions.snapshotAction->
        setEnabled(stopped && (engineCapabilities & SnapshotCapability));

    const bool interruptIsExit = !running;
    if (interruptIsExit) {
        d->m_actions.stopAction->setIcon(d->m_stopIcon);
        d->m_actions.stopAction->setText(tr("Stop Debugger"));
    } else {
        d->m_actions.stopAction->setIcon(d->m_interruptIcon);
        d->m_actions.stopAction->setText(tr("Interrupt"));
    }

    d->m_actions.stopAction->setEnabled(stoppable);
    d->m_actions.resetAction->setEnabled(state != DebuggerNotReady);

    d->m_actions.stepAction->setEnabled(stopped);
    d->m_actions.stepOutAction->setEnabled(stopped);
    d->m_actions.runToLineAction1->setEnabled(stopped);
    d->m_actions.runToLineAction2->setEnabled(stopped);
    d->m_actions.runToFunctionAction->setEnabled(stopped);
    d->m_actions.returnFromFunctionAction->
        setEnabled(stopped && (engineCapabilities & ReturnFromFunctionCapability));

    const bool canJump = stopped && (engineCapabilities & JumpToLineCapability);
    d->m_actions.jumpToLineAction1->setEnabled(canJump);
    d->m_actions.jumpToLineAction2->setEnabled(canJump);

    d->m_actions.nextAction->setEnabled(stopped);

    theDebuggerAction(RecheckDebuggingHelpers)->setEnabled(actionsEnabled);
    const bool canDeref = actionsEnabled
        && (engineCapabilities & AutoDerefPointersCapability);
    theDebuggerAction(AutoDerefPointers)->setEnabled(canDeref);
    theDebuggerAction(ExpandStack)->setEnabled(actionsEnabled);
    theDebuggerAction(ExecuteCommand)->setEnabled(d->m_state != DebuggerNotReady);

    emit stateChanged(d->m_state);
    const bool notbusy = state == InferiorStopped
        || state == DebuggerNotReady
        || state == InferiorUnrunnable;
    setBusyCursor(!notbusy);
}

bool DebuggerManager::debuggerActionsEnabled() const
{
    if (!d->m_engine)
        return false;
    switch (state()) {
    case InferiorStarting:
    case InferiorRunningRequested:
    case InferiorRunning:
    case InferiorUnrunnable:
    case InferiorStopping:
    case InferiorStopped:
        return true;
    case DebuggerNotReady:
    case EngineStarting:
    case AdapterStarting:
    case AdapterStarted:
    case AdapterStartFailed:
    case InferiorStartFailed:
    case InferiorRunningRequested_Kill:
    case InferiorStopping_Kill:
    case InferiorStopFailed:
    case InferiorShuttingDown:
    case InferiorShutDown:
    case InferiorShutdownFailed:
    case EngineShuttingDown:
        break;
    }
    return false;
}

unsigned DebuggerManager::debuggerCapabilities() const
{
    return d->m_engine ? d->m_engine->debuggerCapabilities() : 0;
}

bool DebuggerManager::checkDebugConfiguration(int toolChain,
                                              QString *errorMessage,
                                              QString *settingsCategory /* = 0 */,
                                              QString *settingsPage /* = 0 */) const
{
    errorMessage->clear();
    if (settingsCategory)
        settingsCategory->clear();
    if (settingsPage)
        settingsPage->clear();
    bool success = true;
    switch(toolChain) {
    case ProjectExplorer::ToolChain::GCC:
    //case ProjectExplorer::ToolChain::LinuxICC:
    case ProjectExplorer::ToolChain::MinGW:
    case ProjectExplorer::ToolChain::WINCE: // S60
    case ProjectExplorer::ToolChain::WINSCW:
    case ProjectExplorer::ToolChain::GCCE:
    case ProjectExplorer::ToolChain::RVCT_ARMV5:
    case ProjectExplorer::ToolChain::RVCT_ARMV6:
        if (gdbEngine) {
            success = gdbEngine->checkConfiguration(toolChain, errorMessage, settingsPage);
        } else {
            success = false;
            *errorMessage = msgEngineNotAvailable("Gdb");
        }
        break;
    case ProjectExplorer::ToolChain::MSVC:
        if (winEngine) {
            success = winEngine->checkConfiguration(toolChain, errorMessage, settingsPage);
        } else {
            success = false;
            *errorMessage = msgEngineNotAvailable("Cdb");
            if (settingsPage)
                *settingsPage = QLatin1String("Cdb");
        }
        break;
    }
    if (!success && settingsCategory && settingsPage && !settingsPage->isEmpty())
        *settingsCategory = QLatin1String(DEBUGGER_SETTINGS_CATEGORY);
    return success;
}

void DebuggerManager::ensureLogVisible()
{
    QAction *action = d->m_outputDock->toggleViewAction();
    if (!action->isChecked())
        action->trigger();
}

QIcon DebuggerManager::locationMarkIcon() const
{
    return d->m_locationMarkIcon;
}

QDebug operator<<(QDebug d, DebuggerState state)
{
    return d << DebuggerManager::stateName(state) << '(' << int(state) << ')';
}

static void changeFontSize(QWidget *widget, int size)
{
    QFont font = widget->font();
    font.setPointSize(size);
    widget->setFont(font);
}

void DebuggerManager::fontSettingsChanged(const TextEditor::FontSettings &settings)
{
    int size = settings.fontZoom() * settings.fontSize() / 100;
    changeFontSize(d->m_localsWindow, size);
    changeFontSize(d->m_watchersWindow, size);
    changeFontSize(d->m_breakWindow, size);
    changeFontSize(d->m_modulesWindow, size);
    changeFontSize(d->m_outputWindow, size);
    changeFontSize(d->m_registerWindow, size);
    changeFontSize(d->m_stackWindow, size);
    changeFontSize(d->m_sourceFilesWindow, size);
    changeFontSize(d->m_threadsWindow, size);
}

//////////////////////////////////////////////////////////////////////
//
// AbstractDebuggerEngine
//
//////////////////////////////////////////////////////////////////////

void IDebuggerEngine::showStatusMessage(const QString &msg, int timeout)
{
    m_manager->showStatusMessage(msg, timeout);
}

DebuggerState IDebuggerEngine::state() const
{
    return m_manager->state();
}

void IDebuggerEngine::setState(DebuggerState state, bool forced)
{
    m_manager->setState(state, forced);
}

//////////////////////////////////////////////////////////////////////
//
// Testing
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::runTest(const QString &fileName)
{
    d->m_startParameters->executable = fileName;
    d->m_startParameters->processArgs = QStringList() << "--run-debuggee";
    d->m_startParameters->workingDir.clear();
    //startNewDebugger(StartInternal);
}

} // namespace Debugger

