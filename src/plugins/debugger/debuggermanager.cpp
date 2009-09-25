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
#include "debuggerrunner.h"
#include "debuggerconstants.h"
#include "idebuggerengine.h"

#include "breakwindow.h"
#include "debuggeroutputwindow.h"
#include "moduleswindow.h"
#include "registerwindow.h"
#include "stackwindow.h"
#include "sourcefileswindow.h"
#include "threadswindow.h"
#include "watchwindow.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "stackframe.h"
#include "watchhandler.h"

#include "debuggerdialogs.h"
#ifdef Q_OS_WIN
#  include "shared/peutils.h"
#endif
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <projectexplorer/toolchain.h>

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

// Note: the Debugger process itself is referred to as 'Debugger',
// whereas the debugged process is referred to as 'Inferior'.
//
// 0 == DebuggerNotReady
//          |
//     EngineStarting
//          |
//     AdapterStarting --> AdapterStartFailed --> 0
//          |
//     AdapterStarted
//          |
//     InferiorStarting --> InferiorStartFailed --> 0
//          |          |
//          |          |
//     InferiorStopped |
//          |          |
//          | <--------
//          |  .------------------------------------.
//          |  v                                    |
//     InferiorRunningRequested                     |
//          |                                       |
//     InferiorRunning  --> 1 (normal exit)         |
//          |                                       |
//     InferiorStopping                             |
//          |                                       |
//     InferiorStopped --> 1                        |
//          |                                       |
//          `---------------------------------------'
//          
// 1 == InferiorShuttingDown  -> InferiorShutdownFailed
//          |
//     InferiorShutDown
//          |
//     AdapterShuttingDown  -> AdapterShutdownFailed --> 0
//          |
//          0
//
// Allowed actions:
//    [R] :  Run
//    [C] :  Continue
//    [N] :  Step, Next

namespace Debugger {
namespace Internal {

IDebuggerEngine *createGdbEngine(DebuggerManager *parent);
IDebuggerEngine *createScriptEngine(DebuggerManager *parent);
IDebuggerEngine *createTcfEngine(DebuggerManager *parent);

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


QDebug operator<<(QDebug str, const DebuggerStartParameters &p)
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
            << " serverStartScript=" << p.serverStartScript
            << " toolchain=" << p.toolChainType << '\n';
    return str;
}

using namespace Constants;

static const QString tooltipIName = "tooltip";

static const char *stateName(int s)
{
    #define SN(x) case x: return #x;
    switch (s) {
        SN(DebuggerNotReady)
        SN(EngineStarting)
        SN(AdapterStarting)
        SN(AdapterStarted)
        SN(AdapterStartFailed)
        SN(InferiorPreparing)
        SN(InferiorPrepared)
        SN(InferiorPreparationFailed)
        SN(InferiorStarting)
        SN(InferiorStartFailed)
        SN(InferiorRunningRequested)
        SN(InferiorRunning)
        SN(InferiorStopping)
        SN(InferiorStopped)
        SN(InferiorStopFailed)
        SN(InferiorShuttingDown)
        SN(InferiorShutDown)
        SN(InferiorShutdownFailed)
        SN(AdapterShuttingDown)
        SN(AdapterShutdownFailed)
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
    executable.clear();
    coreFile.clear();
    processArgs.clear();
    environment.clear();
    workingDir.clear();
    buildDir.clear();
    attachPID = -1;
    useTerminal = false;
    crashParameter.clear();
    remoteChannel.clear();
    remoteArchitecture.clear();
    serverStartScript.clear();
    toolChainType = ProjectExplorer::ToolChain::UNKNOWN;
    startMode = NoStartMode;
}


///////////////////////////////////////////////////////////////////////
//
// DebuggerManager
//
///////////////////////////////////////////////////////////////////////

static IDebuggerEngine *gdbEngine = 0;
static IDebuggerEngine *scriptEngine = 0;
static IDebuggerEngine *tcfEngine = 0;
static IDebuggerEngine *winEngine = 0;

DebuggerManager::DebuggerManager()
  : m_startParameters(new DebuggerStartParameters),
    m_inferiorPid(0)
{
    init();
}

DebuggerManager::~DebuggerManager()
{
    #define doDelete(ptr) delete ptr; ptr = 0
    doDelete(gdbEngine);
    doDelete(scriptEngine);
    doDelete(tcfEngine);
    doDelete(winEngine);
    #undef doDelete
}

void DebuggerManager::init()
{
    m_state = DebuggerState(-1);
    m_busy = false;

    m_modulesHandler = 0;
    m_registerHandler = 0;

    m_statusLabel = new QLabel;
    m_statusLabel->setMinimumSize(QSize(30, 10));

    m_breakWindow = new BreakWindow;
    m_modulesWindow = new ModulesWindow(this);
    m_outputWindow = new DebuggerOutputWindow;
    m_registerWindow = new RegisterWindow(this);
    m_stackWindow = new StackWindow(this);
    m_sourceFilesWindow = new SourceFilesWindow;
    m_threadsWindow = new ThreadsWindow;
    m_localsWindow = new WatchWindow(WatchWindow::LocalsType, this);
    m_watchersWindow = new WatchWindow(WatchWindow::WatchersType, this);
    //m_tooltipWindow = new WatchWindow(WatchWindow::TooltipType, this);
    m_statusTimer = new QTimer(this);

    m_mainWindow = new Core::Utils::FancyMainWindow;
    m_mainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    m_mainWindow->setDocumentMode(true);

    // Stack
    m_stackHandler = new StackHandler;
    QAbstractItemView *stackView =
        qobject_cast<QAbstractItemView *>(m_stackWindow);
    stackView->setModel(m_stackHandler->stackModel());
    connect(stackView, SIGNAL(frameActivated(int)),
        this, SLOT(activateFrame(int)));
    connect(theDebuggerAction(ExpandStack), SIGNAL(triggered()),
        this, SLOT(reloadFullStack()));
    connect(theDebuggerAction(MaximalStackDepth), SIGNAL(triggered()),
        this, SLOT(reloadFullStack()));

    // Threads
    m_threadsHandler = new ThreadsHandler;
    QAbstractItemView *threadsView =
        qobject_cast<QAbstractItemView *>(m_threadsWindow);
    threadsView->setModel(m_threadsHandler->threadsModel());
    connect(threadsView, SIGNAL(threadSelected(int)),
        this, SLOT(selectThread(int)));

    // Breakpoints
    m_breakHandler = new BreakHandler(this);
    QAbstractItemView *breakView =
        qobject_cast<QAbstractItemView *>(m_breakWindow);
    breakView->setModel(m_breakHandler->model());
    connect(breakView, SIGNAL(breakpointActivated(int)),
        m_breakHandler, SLOT(activateBreakpoint(int)));
    connect(breakView, SIGNAL(breakpointDeleted(int)),
        m_breakHandler, SLOT(removeBreakpoint(int)));
    connect(breakView, SIGNAL(breakpointSynchronizationRequested()),
        this, SLOT(attemptBreakpointSynchronization()));
    connect(breakView, SIGNAL(breakByFunctionRequested(QString)),
        this, SLOT(breakByFunction(QString)), Qt::QueuedConnection);
    connect(breakView, SIGNAL(breakByFunctionMainRequested()),
        this, SLOT(breakByFunctionMain()), Qt::QueuedConnection);

    // Modules
    QAbstractItemView *modulesView =
        qobject_cast<QAbstractItemView *>(m_modulesWindow);
    m_modulesHandler = new ModulesHandler;
    modulesView->setModel(m_modulesHandler->model());
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
    //m_sourceFilesHandler = new SourceFilesHandler;
    QAbstractItemView *sourceFilesView =
        qobject_cast<QAbstractItemView *>(m_sourceFilesWindow);
    //sourceFileView->setModel(m_stackHandler->stackModel());
    connect(sourceFilesView, SIGNAL(reloadSourceFilesRequested()),
        this, SLOT(reloadSourceFiles()));
    connect(sourceFilesView, SIGNAL(fileOpenRequested(QString)),
        this, SLOT(fileOpen(QString)));

    // Registers
    QAbstractItemView *registerView =
        qobject_cast<QAbstractItemView *>(m_registerWindow);
    m_registerHandler = new RegisterHandler;
    registerView->setModel(m_registerHandler->model());

    // Locals
    m_watchHandler = new WatchHandler;
    QTreeView *localsView = qobject_cast<QTreeView *>(m_localsWindow);
    localsView->setModel(m_watchHandler->model(LocalsWatch));

    // Watchers
    QTreeView *watchersView = qobject_cast<QTreeView *>(m_watchersWindow);
    watchersView->setModel(m_watchHandler->model(WatchersWatch));
    connect(m_watchHandler, SIGNAL(sessionValueRequested(QString,QVariant*)),
        this, SIGNAL(sessionValueRequested(QString,QVariant*)));
    connect(m_watchHandler, SIGNAL(setSessionValueRequested(QString,QVariant)),
        this, SIGNAL(setSessionValueRequested(QString,QVariant)));
    connect(theDebuggerAction(AssignValue), SIGNAL(triggered()),
        this, SLOT(assignValueInDebugger()), Qt::QueuedConnection);

    // Tooltip
    //QTreeView *tooltipView = qobject_cast<QTreeView *>(m_tooltipWindow);
    //tooltipView->setModel(m_watchHandler->model(TooltipsWatch));
    //qRegisterMetaType<WatchData>("Debugger::Internal::WatchData");
    qRegisterMetaType<WatchData>("WatchData");
    connect(m_watchHandler, SIGNAL(watchDataUpdateNeeded(WatchData)),
        this, SLOT(updateWatchData(WatchData)));

    m_continueAction = new QAction(this);
    m_continueAction->setText(tr("Continue"));
    m_continueAction->setIcon(QIcon(":/debugger/images/debugger_continue_small.png"));

    m_stopAction = new QAction(this);
    m_stopAction->setText(tr("Interrupt"));
    m_stopAction->setIcon(QIcon(":/debugger/images/debugger_interrupt_small.png"));

    m_resetAction = new QAction(this);
    m_resetAction->setText(tr("Reset Debugger"));

    m_nextAction = new QAction(this);
    m_nextAction->setText(tr("Step Over"));
    //m_nextAction->setShortcut(QKeySequence(tr("F6")));
    m_nextAction->setIcon(QIcon(":/debugger/images/debugger_stepover_small.png"));

    m_stepAction = new QAction(this);
    m_stepAction->setText(tr("Step Into"));
    //m_stepAction->setShortcut(QKeySequence(tr("F7")));
    m_stepAction->setIcon(QIcon(":/debugger/images/debugger_stepinto_small.png"));

    m_stepOutAction = new QAction(this);
    m_stepOutAction->setText(tr("Step Out"));
    //m_stepOutAction->setShortcut(QKeySequence(tr("Shift+F7")));
    m_stepOutAction->setIcon(QIcon(":/debugger/images/debugger_stepout_small.png"));

    m_runToLineAction = new QAction(this);
    m_runToLineAction->setText(tr("Run to Line"));

    m_runToFunctionAction = new QAction(this);
    m_runToFunctionAction->setText(tr("Run to Outermost Function"));

    m_jumpToLineAction = new QAction(this);
    m_jumpToLineAction->setText(tr("Jump to Line"));

    m_breakAction = new QAction(this);
    m_breakAction->setText(tr("Toggle Breakpoint"));

    m_watchAction = new QAction(this);
    m_watchAction->setText(tr("Add to Watch Window"));

    m_reverseDirectionAction = new QAction(this);
    m_reverseDirectionAction->setText(tr("Reverse Direction"));
    m_reverseDirectionAction->setCheckable(true);
    m_reverseDirectionAction->setChecked(false);
    //m_reverseDirectionAction->setIcon(QIcon(":/debugger/images/debugger_stepoverproc_small.png"));

    connect(m_continueAction, SIGNAL(triggered()),
        this, SLOT(continueExec()));
    connect(m_stopAction, SIGNAL(triggered()),
        this, SLOT(interruptDebuggingRequest()));
    connect(m_resetAction, SIGNAL(triggered()),
        this, SLOT(exitDebugger()));
    connect(m_nextAction, SIGNAL(triggered()),
        this, SLOT(nextExec()));
    connect(m_stepAction, SIGNAL(triggered()),
        this, SLOT(stepExec()));
    connect(m_stepOutAction, SIGNAL(triggered()),
        this, SLOT(stepOutExec()));
    connect(m_runToLineAction, SIGNAL(triggered()),
        this, SLOT(runToLineExec()));
    connect(m_runToFunctionAction, SIGNAL(triggered()),
        this, SLOT(runToFunctionExec()));
    connect(m_jumpToLineAction, SIGNAL(triggered()),
        this, SLOT(jumpToLineExec()));
    connect(m_watchAction, SIGNAL(triggered()),
        this, SLOT(addToWatchWindow()));
    connect(m_breakAction, SIGNAL(triggered()),
        this, SLOT(toggleBreakpoint()));

    connect(m_statusTimer, SIGNAL(timeout()),
        this, SLOT(clearStatusMessage()));

    connect(theDebuggerAction(ExecuteCommand), SIGNAL(triggered()),
        this, SLOT(executeDebuggerCommand()));

    connect(theDebuggerAction(WatchPoint), SIGNAL(triggered()),
        this, SLOT(watchPoint()));

    connect(theDebuggerAction(StepByInstruction), SIGNAL(triggered()),
        this, SLOT(stepByInstructionTriggered()));


    m_breakDock = m_mainWindow->addDockForWidget(m_breakWindow);

    m_modulesDock = m_mainWindow->addDockForWidget(m_modulesWindow);
    connect(m_modulesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadModules()), Qt::QueuedConnection);

    m_registerDock = m_mainWindow->addDockForWidget(m_registerWindow);
    connect(m_registerDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadRegisters()), Qt::QueuedConnection);

    m_outputDock = m_mainWindow->addDockForWidget(m_outputWindow);

    m_stackDock = m_mainWindow->addDockForWidget(m_stackWindow);

    m_sourceFilesDock = m_mainWindow->addDockForWidget(m_sourceFilesWindow);
    connect(m_sourceFilesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadSourceFiles()), Qt::QueuedConnection);

    m_threadsDock = m_mainWindow->addDockForWidget(m_threadsWindow);

    QSplitter *localsAndWatchers = new QSplitter(Qt::Vertical, 0);
    localsAndWatchers->setWindowTitle(m_localsWindow->windowTitle());
    localsAndWatchers->addWidget(m_localsWindow);
    localsAndWatchers->addWidget(m_watchersWindow);
    //localsAndWatchers->addWidget(m_tooltipWindow);
    localsAndWatchers->setStretchFactor(0, 3);
    localsAndWatchers->setStretchFactor(1, 1);
    localsAndWatchers->setStretchFactor(2, 1);
    m_watchDock = m_mainWindow->addDockForWidget(localsAndWatchers);

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

    if (enabledTypeFlags & TcfEngineType) {
        tcfEngine = createTcfEngine(this);
        tcfEngine->addOptionPages(&rc);
    }

    m_engine = 0;
    STATE_DEBUG(gdbEngine << winEngine << scriptEngine << rc.size());
    return rc;
}

IDebuggerEngine *DebuggerManager::engine()
{
    return m_engine;
}

void DebuggerManager::createNewDock(QWidget *widget)
{
    QDockWidget *dockWidget = new QDockWidget(widget->windowTitle(), m_mainWindow);
    dockWidget->setObjectName(widget->windowTitle());
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    dockWidget->setWidget(widget);
    m_mainWindow->addDockWidget(Qt::TopDockWidgetArea, dockWidget);
    dockWidget->show();
}

void DebuggerManager::setSimpleDockWidgetArrangement()
{
    m_mainWindow->setTrackingEnabled(false);
    QList<QDockWidget *> dockWidgets = m_mainWindow->dockWidgets();
    foreach (QDockWidget *dockWidget, dockWidgets) {
        dockWidget->setFloating(false);
        m_mainWindow->removeDockWidget(dockWidget);
    }

    foreach (QDockWidget *dockWidget, dockWidgets) {
        m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
        dockWidget->show();
    }

    m_mainWindow->tabifyDockWidget(m_watchDock, m_breakDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_modulesDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_outputDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_registerDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_threadsDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_sourceFilesDock);

    // They following views are rarely used in ordinary debugging. Hiding them
    // saves cycles since the corresponding information won't be retrieved.
    m_sourceFilesDock->hide();
    m_registerDock->hide();
    m_modulesDock->hide();
    m_outputDock->hide();
    m_mainWindow->setTrackingEnabled(true);
}

QAbstractItemModel *DebuggerManager::threadsModel()
{
    return qobject_cast<ThreadsWindow*>(m_threadsWindow)->model();
}

void DebuggerManager::clearStatusMessage()
{
    m_statusLabel->setText(m_lastPermanentStatusMessage);
}

void DebuggerManager::showStatusMessage(const QString &msg, int timeout)
{
    Q_UNUSED(timeout)
    showDebuggerOutput(LogStatus, msg);
    m_statusLabel->setText(QLatin1String("   ") + msg);
    if (timeout > 0) {
        m_statusTimer->setSingleShot(true);
        m_statusTimer->start(timeout);
    } else {
        m_lastPermanentStatusMessage = msg;
        m_statusTimer->stop();
    }
}

void DebuggerManager::notifyInferiorStopped()
{
    resetLocation();
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
    STATE_DEBUG(m_inferiorPid << pid);

    if (m_inferiorPid != pid) {
        m_inferiorPid = pid;
        emit inferiorPidChanged(pid);
    }
}

void DebuggerManager::showApplicationOutput(const QString &str)
{
     emit applicationOutputAvailable(str);
}

void DebuggerManager::shutdown()
{
    STATE_DEBUG(m_engine);
    if (m_engine)
        m_engine->shutdown();
    m_engine = 0;

    #define doDelete(ptr) delete ptr; ptr = 0
    doDelete(scriptEngine);
    doDelete(gdbEngine);
    doDelete(winEngine);
    doDelete(tcfEngine);

    // Delete these manually before deleting the manager
    // (who will delete the models for most views)
    doDelete(m_breakWindow);
    doDelete(m_modulesWindow);
    doDelete(m_outputWindow);
    doDelete(m_registerWindow);
    doDelete(m_stackWindow);
    doDelete(m_sourceFilesWindow);
    doDelete(m_threadsWindow);
    //doDelete(m_tooltipWindow);
    doDelete(m_watchersWindow);
    doDelete(m_localsWindow);

    doDelete(m_breakHandler);
    doDelete(m_threadsHandler);
    doDelete(m_modulesHandler);
    doDelete(m_registerHandler);
    doDelete(m_stackHandler);
    doDelete(m_watchHandler);
    #undef doDelete
}

BreakpointData *DebuggerManager::findBreakpoint(const QString &fileName, int lineNumber)
{
    if (!m_breakHandler)
        return 0;
    int index = m_breakHandler->findBreakpoint(fileName, lineNumber);
    return index == -1 ? 0 : m_breakHandler->at(index);
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
    QTC_ASSERT(m_breakHandler, return);
    if (state() != InferiorRunning
         && state() != InferiorStopped
         && state() != DebuggerNotReady) {
        showStatusMessage(tr("Changing breakpoint state requires either a "
            "fully running or fully stopped application."));
        return;
    }

    int index = m_breakHandler->findBreakpoint(fileName, lineNumber);
    if (index == -1)
        m_breakHandler->setBreakpoint(fileName, lineNumber);
    else
        m_breakHandler->removeBreakpoint(index);

    attemptBreakpointSynchronization();
}

void DebuggerManager::toggleBreakpointEnabled(const QString &fileName, int lineNumber)
{
    STATE_DEBUG(fileName << lineNumber);
    QTC_ASSERT(m_breakHandler, return);
    if (state() != InferiorRunning
         && state() != InferiorStopped
         && state() != DebuggerNotReady) {
        showStatusMessage(tr("Changing breakpoint state requires either a "
            "fully running or fully stopped application."));
        return;
    }

    m_breakHandler->toggleBreakpointEnabled(fileName, lineNumber);

    attemptBreakpointSynchronization();
}

void DebuggerManager::attemptBreakpointSynchronization()
{
    if (m_engine)
        m_engine->attemptBreakpointSynchronization();
}

void DebuggerManager::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    if (m_engine)
        m_engine->setToolTipExpression(mousePos, editor, cursorPos);
}

void DebuggerManager::updateWatchData(const WatchData &data)
{
    if (m_engine)
        m_engine->updateWatchData(data);
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
    case ProjectExplorer::ToolChain::LinuxICC:
    case ProjectExplorer::ToolChain::MinGW:
    case ProjectExplorer::ToolChain::GCC:
        rc = gdbEngine;
        break;
    case ProjectExplorer::ToolChain::MSVC:
    case ProjectExplorer::ToolChain::WINCE:
        rc = winEngine;
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
    if (!winEngine) {
        *errorMessage = DebuggerManager::tr("Debugging VS executables is currently not enabled.");
        *settingsIdHint = QLatin1String("Cdb");
        return 0;
    }
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
    m_startParameters = sp;
    m_inferiorPid = m_startParameters->attachPID > 0
        ? m_startParameters->attachPID : 0;
    const QString toolChainName = ProjectExplorer::ToolChain::toolChainName(static_cast<ProjectExplorer::ToolChain::ToolChainType>(m_startParameters->toolChainType));

    emit debugModeRequested();
    showDebuggerOutput(LogStatus,
        tr("Starting debugger for tool chain '%1'...").arg(toolChainName));
    showDebuggerOutput(LogDebug, DebuggerSettings::instance()->dump());

    QString errorMessage;
    QString settingsIdHint;
    switch (m_startParameters->startMode) {
    case AttachExternal:
    case AttachCrashedExternal:
        m_engine = determineDebuggerEngine(m_startParameters->attachPID,
            m_startParameters->toolChainType, &errorMessage);
        break;
    case AttachTcf:
        m_engine = tcfEngine;
        break;
    default:
        m_engine = determineDebuggerEngine(m_startParameters->executable, 
            m_startParameters->toolChainType, &errorMessage, &settingsIdHint);
        break;
    }

    if (!m_engine) {
        emit debuggingFinished();
        // Create Message box with possibility to go to settings
        QAbstractButton *settingsButton = 0;
        QMessageBox msgBox(QMessageBox::Warning, tr("Warning"),
            tr("Cannot debug '%1' (tool chain: '%2'): %3").
            arg(m_startParameters->executable, toolChainName, errorMessage),
            QMessageBox::Ok);
        if (!settingsIdHint.isEmpty())
            settingsButton = msgBox.addButton(tr("Settings..."), QMessageBox::AcceptRole);
        msgBox.exec();
        if (msgBox.clickedButton() == settingsButton)
            Core::ICore::instance()->showOptionsDialog(
                _(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY), settingsIdHint);
        return;
    }

    STATE_DEBUG(m_startParameters->executable << m_engine);
    setBusyCursor(false);
    setState(EngineStarting);
    connect(m_engine, SIGNAL(startFailed()), this, SLOT(startFailed()));
    m_engine->startDebugger(m_startParameters);
}

void DebuggerManager::startFailed()
{
    disconnect(m_engine, SIGNAL(startFailed()), this, SLOT(startFailed()));
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
    m_sourceFilesWindow->removeAll();
}

void DebuggerManager::exitDebugger()
{
    STATE_DEBUG("");
    if (m_engine)
        m_engine->exitDebugger();
    cleanupViews();
    setState(DebuggerNotReady);
}

DebuggerStartParametersPtr DebuggerManager::startParameters() const
{
    return m_startParameters;
}

qint64 DebuggerManager::inferiorPid() const
{
    return m_inferiorPid;
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
    QTC_ASSERT(m_engine, return);
    m_engine->assignValueInDebugger(expr, value);
}

void DebuggerManager::activateFrame(int index)
{
    QTC_ASSERT(m_engine, return);
    m_engine->activateFrame(index);
}

void DebuggerManager::selectThread(int index)
{
    QTC_ASSERT(m_engine, return);
    m_engine->selectThread(index);
}

void DebuggerManager::loadAllSymbols()
{
    QTC_ASSERT(m_engine, return);
    m_engine->loadAllSymbols();
}

void DebuggerManager::loadSymbols(const QString &module)
{
    QTC_ASSERT(m_engine, return);
    m_engine->loadSymbols(module);
}

QList<Symbol> DebuggerManager::moduleSymbols(const QString &moduleName)
{
    QTC_ASSERT(m_engine, return QList<Symbol>());
    return m_engine->moduleSymbols(moduleName);
}

void DebuggerManager::stepExec()
{
    QTC_ASSERT(m_engine, return);
    resetLocation();
    if (theDebuggerBoolSetting(StepByInstruction))
        m_engine->stepIExec();
    else
        m_engine->stepExec();
}

void DebuggerManager::stepOutExec()
{
    QTC_ASSERT(m_engine, return);
    resetLocation();
    m_engine->stepOutExec();
}

void DebuggerManager::nextExec()
{
    QTC_ASSERT(m_engine, return);
    resetLocation();
    if (theDebuggerBoolSetting(StepByInstruction))
        m_engine->nextIExec();
    else
        m_engine->nextExec();
}

void DebuggerManager::watchPoint()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        if (m_engine)
            m_engine->watchPoint(action->data().toPoint());
}

void DebuggerManager::executeDebuggerCommand()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        executeDebuggerCommand(action->data().toString());
}

void DebuggerManager::executeDebuggerCommand(const QString &command)
{
    STATE_DEBUG(command);
    QTC_ASSERT(m_engine, return);
    m_engine->executeDebuggerCommand(command);
}

void DebuggerManager::sessionLoaded()
{
    cleanupViews();
    setState(DebuggerNotReady);
    setBusyCursor(false);
    loadSessionData();
}

void DebuggerManager::aboutToUnloadSession()
{
    cleanupViews();
    if (m_engine)
        m_engine->shutdown();
    setState(DebuggerNotReady);
    setBusyCursor(false);
}

void DebuggerManager::aboutToSaveSession()
{
    saveSessionData();
}

void DebuggerManager::loadSessionData()
{
    m_breakHandler->loadSessionData();
    m_watchHandler->loadSessionData();
}

void DebuggerManager::saveSessionData()
{
    m_breakHandler->saveSessionData();
    m_watchHandler->saveSessionData();
}

void DebuggerManager::dumpLog()
{
    QString fileName = QFileDialog::getSaveFileName(mainWindow(),
        tr("Save Debugger Log"), QDir::tempPath());
    if (fileName.isEmpty())
        return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return;
    QTextStream ts(&file);
    ts << m_outputWindow->inputContents();
    ts << "\n\n=======================================\n\n";
    ts << m_outputWindow->combinedContents();
}

void DebuggerManager::addToWatchWindow()
{
    // requires a selection, but that's the only case we want...
    QObject *ob = 0;
    queryCurrentTextEditor(0, 0, &ob);
    QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(ob);
    if (!editor)
        return;
    QTextCursor tc = editor->textCursor();
    theDebuggerAction(WatchExpression)->setValue(tc.selectedText());
}

void DebuggerManager::setBreakpoint(const QString &fileName, int lineNumber)
{
    STATE_DEBUG(Q_FUNC_INFO << fileName << lineNumber);
    QTC_ASSERT(m_breakHandler, return);
    m_breakHandler->setBreakpoint(fileName, lineNumber);
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
    QTC_ASSERT(m_breakHandler, return);
    m_breakHandler->breakByFunction(functionName);
    attemptBreakpointSynchronization();
}

void DebuggerManager::setBusyCursor(bool busy)
{
    //STATE_DEBUG("BUSY FROM: " << m_busy << " TO: " << m_busy);
    if (busy == m_busy)
        return;
    m_busy = busy;

    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_breakWindow->setCursor(cursor);
    m_localsWindow->setCursor(cursor);
    m_modulesWindow->setCursor(cursor);
    m_outputWindow->setCursor(cursor);
    m_registerWindow->setCursor(cursor);
    m_stackWindow->setCursor(cursor);
    m_sourceFilesWindow->setCursor(cursor);
    m_threadsWindow->setCursor(cursor);
    //m_tooltipWindow->setCursor(cursor);
    m_watchersWindow->setCursor(cursor);
}

void DebuggerManager::queryCurrentTextEditor(QString *fileName, int *lineNumber,
    QObject **object)
{
    emit currentTextEditorRequested(fileName, lineNumber, object);
}

void DebuggerManager::continueExec()
{
    if (m_engine)
        m_engine->continueInferior();
}

void DebuggerManager::detachDebugger()
{
    if (m_engine)
        m_engine->detachDebugger();
}

void DebuggerManager::interruptDebuggingRequest()
{
    STATE_DEBUG(state());
    if (!m_engine)
        return;
    bool interruptIsExit = (state() != InferiorRunning);
    if (interruptIsExit) {
        exitDebugger();
    } else {
        m_engine->interruptInferior();
    }
}

void DebuggerManager::runToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (m_engine && !fileName.isEmpty()) {
        STATE_DEBUG(fileName << lineNumber);
        m_engine->runToLineExec(fileName, lineNumber);
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

    if (m_engine && !functionName.isEmpty())
        m_engine->runToFunctionExec(functionName);
}

void DebuggerManager::jumpToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (m_engine && !fileName.isEmpty()) {
        STATE_DEBUG(fileName << lineNumber);
        m_engine->jumpToLineExec(fileName, lineNumber);
    }
}

void DebuggerManager::resetLocation()
{
    // connected to the plugin
    emit resetLocationRequested();
}

void DebuggerManager::gotoLocation(const StackFrame &frame, bool setMarker)
{
    // connected to the plugin
    emit gotoLocationRequested(frame, setMarker);
}

void DebuggerManager::fileOpen(const QString &fileName)
{
    // connected to the plugin
    StackFrame frame;
    frame.file = fileName;
    frame.line = -1;
    emit gotoLocationRequested(frame, false);
}

void DebuggerManager::stepByInstructionTriggered()
{
    QTC_ASSERT(m_stackHandler, return);
    StackFrame frame = m_stackHandler->currentFrame();
    gotoLocation(frame, true);
}


//////////////////////////////////////////////////////////////////////
//
// Source files specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadSourceFiles()
{
    if (m_engine && m_sourceFilesDock && m_sourceFilesDock->isVisible())
        m_engine->reloadSourceFiles();
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
    if (m_engine && m_modulesDock && m_modulesDock->isVisible())
        m_engine->reloadModules();
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
    if (m_outputWindow)
        m_outputWindow->showOutput(channel, msg);
    else 
        qDebug() << "OUTPUT: " << channel << msg;
}

void DebuggerManager::showDebuggerInput(int channel, const QString &msg)
{
    if (m_outputWindow)
        m_outputWindow->showInput(channel, msg);
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
    if (m_engine && m_registerDock && m_registerDock->isVisible())
        m_engine->reloadRegisters();
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
    return m_startParameters->dumperLibrary;
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
    return m_startParameters->dumperLibraryLocations;
}

void DebuggerManager::showQtDumperLibraryWarning(const QString &details)
{
    QMessageBox dialog(mainWindow());
    QPushButton *qtPref = dialog.addButton(tr("Open Qt preferences"),
        QMessageBox::ActionRole);
    QPushButton *helperOff = dialog.addButton(tr("Turn helper usage off"),
        QMessageBox::ActionRole);
    QPushButton *justContinue = dialog.addButton(tr("Continue anyway"),
        QMessageBox::AcceptRole);
    dialog.setDefaultButton(justContinue);
    dialog.setWindowTitle(tr("Debugging helper missing"));
    dialog.setText(tr("The debugger did not find the debugging helper library."));
    dialog.setInformativeText(tr(
        "The debugging helper is used to nicely format the values of some Qt "
        "and Standard Library data types. "
        "It must be compiled for each Qt version which "
        "you can do in the Qt preferences page by selecting "
        "a Qt installation and clicking on 'Rebuild' for the debugging "
        "helper."));
    if (!details.isEmpty())
        dialog.setDetailedText(details);
    dialog.exec();
    if (dialog.clickedButton() == qtPref) {
        Core::ICore::instance()->showOptionsDialog(_("Qt4"), _("Qt Versions"));
    } else if (dialog.clickedButton() == helperOff) {
        theDebuggerAction(UseDebuggingHelpers)->setValue(qVariantFromValue(false), false);
    }
}

void DebuggerManager::reloadFullStack()
{
    if (m_engine)
        m_engine->reloadFullStack();
}

void DebuggerManager::setRegisterValue(int nr, const QString &value)
{
    if (m_engine)
        m_engine->setRegisterValue(nr, value);
}

bool DebuggerManager::isReverseDebugging() const
{
    return m_reverseDirectionAction->isChecked();
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

void DebuggerManager::showMessageBox(int icon,
    const QString &title, const QString &text)
{
    QMessageBox *mb = new QMessageBox(QMessageBox::Icon(icon),
        title, text, QMessageBox::NoButton, mainWindow());
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->show();
}

DebuggerState DebuggerManager::state() const
{
    return m_state;
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
        return to == InferiorPreparing;
    case AdapterStartFailed:
        return to == DebuggerNotReady;

    case InferiorPreparing:
        return to == InferiorPrepared || to == InferiorPreparationFailed;
    case InferiorPrepared:
        return to == InferiorStarting;
    case InferiorPreparationFailed:
        return to == DebuggerNotReady;

    case InferiorStarting:
        return to == InferiorRunningRequested || to == InferiorStopped
            || to == InferiorStartFailed;
    case InferiorStartFailed:
        return to == DebuggerNotReady;

    case InferiorRunningRequested:
        return to == InferiorRunning;
    case InferiorRunning:
        return to == InferiorStopping || to == InferiorShuttingDown;

    case InferiorStopping:
        return to == InferiorStopped || to == InferiorStopFailed;
    case InferiorStopped:
        return to == InferiorRunningRequested || to == InferiorShuttingDown;
    case InferiorStopFailed:
        return to == DebuggerNotReady;

    case InferiorShuttingDown:
        return to == InferiorShutDown || to == InferiorShutdownFailed;
    case InferiorShutDown:
        return to == AdapterShuttingDown;

    case AdapterShuttingDown:
        return to == DebuggerNotReady;

    default:
        qDebug() << "UNKNOWN STATE: " << from;
    }
    return false;
}

void DebuggerManager::setState(DebuggerState state)
{
    //STATE_DEBUG("STATUS CHANGE: FROM " << stateName(m_state)
    //        << " TO " << stateName(state));

    QString msg = _("State changed from %1(%2) to %3(%4).")
        .arg(stateName(m_state)).arg(m_state).arg(stateName(state)).arg(state);
    qDebug() << msg;
    if (!isAllowedTransition(m_state, state))
        qDebug() << "UNEXPECTED STATE TRANSITION: " << msg;

    showDebuggerOutput(LogDebug, msg);

    resetLocation();
    if (state == m_state)
        return;

    m_state = state;

    if (m_state == InferiorStopped)
        resetLocation();

    if (m_state == DebuggerNotReady) {
        setBusyCursor(false);
        emit debuggingFinished();
    }

    const bool started = state == InferiorRunning
        || state == InferiorRunningRequested
        || state == InferiorStopping
        || state == InferiorStopped;

    const bool running = state == InferiorRunning;

    const bool ready = state == InferiorStopped
            && m_startParameters->startMode != AttachCore;

    if (ready)
        QApplication::alert(mainWindow(), 3000);

    m_watchAction->setEnabled(ready);
    m_breakAction->setEnabled(true);

    bool interruptIsExit = !running;
    if (interruptIsExit) {
        m_stopAction->setIcon(QIcon(":/debugger/images/debugger_stop_small.png"));
        m_stopAction->setText(tr("Stop Debugger"));
    } else {
        m_stopAction->setIcon(QIcon(":/debugger/images/debugger_interrupt_small.png"));
        m_stopAction->setText(tr("Interrupt"));
    }

    m_stopAction->setEnabled(started);
    m_resetAction->setEnabled(true);

    m_stepAction->setEnabled(ready);
    m_stepOutAction->setEnabled(ready);
    m_runToLineAction->setEnabled(ready);
    m_runToFunctionAction->setEnabled(ready);
    m_jumpToLineAction->setEnabled(ready);
    m_nextAction->setEnabled(ready);
    //showStatusMessage(QString("started: %1, running: %2")
    // .arg(started).arg(running));
    emit stateChanged(m_state);
    const bool notbusy = ready || state == DebuggerNotReady;
    setBusyCursor(!notbusy);
}

QDebug operator<<(QDebug d, DebuggerState state)
{
    return d << stateName(state) << '(' << int(state) << ')';
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

void IDebuggerEngine::setState(DebuggerState state)
{
    m_manager->setState(state);
}

//////////////////////////////////////////////////////////////////////
//
// Testing
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::runTest(const QString &fileName)
{
    m_startParameters->executable = fileName;
    m_startParameters->processArgs = QStringList() << "--run-debuggee";
    m_startParameters->workingDir.clear();
    //startNewDebugger(StartInternal);
}

} // namespace Internal
} // namespace Debugger

