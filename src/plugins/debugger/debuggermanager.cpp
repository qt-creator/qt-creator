/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "debuggermanager.h"

#include "assert.h"
#include "debuggerconstants.h"
#include "idebuggerengine.h"

#include "breakwindow.h"
#include "disassemblerwindow.h"
#include "debuggeroutputwindow.h"
#include "moduleswindow.h"
#include "registerwindow.h"
#include "stackwindow.h"
#include "threadswindow.h"
#include "watchwindow.h"

#include "ui_breakbyfunction.h"

#include "disassemblerhandler.h"
#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"

#include "startexternaldialog.h"
#include "attachexternaldialog.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QErrorMessage>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStatusBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QToolTip>

using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;

static const QString tooltipIName = "tooltip";

///////////////////////////////////////////////////////////////////////
//
// BreakByFunctionDialog
//
///////////////////////////////////////////////////////////////////////

class BreakByFunctionDialog : public QDialog, Ui::BreakByFunctionDialog
{
    Q_OBJECT

public:
    explicit BreakByFunctionDialog(QWidget *parent)
      : QDialog(parent)
    {
        setupUi(this);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    }
    QString functionName() const { return functionLineEdit->text(); }
};


///////////////////////////////////////////////////////////////////////
//
// DebuggerManager
//
///////////////////////////////////////////////////////////////////////

static IDebuggerEngine *gdbEngine = 0;
static IDebuggerEngine *winEngine = 0;
static IDebuggerEngine *scriptEngine = 0;

extern IDebuggerEngine *createGdbEngine(DebuggerManager *parent);
extern IDebuggerEngine *createWinEngine(DebuggerManager *) { return 0; }
extern IDebuggerEngine *createScriptEngine(DebuggerManager *parent);

DebuggerManager::DebuggerManager()
{
    init();
}

DebuggerManager::~DebuggerManager()
{
    delete gdbEngine;
    delete winEngine;
    delete scriptEngine;
}

void DebuggerManager::init()
{
    m_status = -1;
    m_busy = false;

    m_attachedPID = 0;
    m_startMode = startInternal;

    m_disassemblerHandler = 0;
    m_modulesHandler = 0;
    m_registerHandler = 0;

    m_statusLabel = new QLabel;
    m_breakWindow = new BreakWindow;
    m_disassemblerWindow = new DisassemblerWindow;
    m_modulesWindow = new ModulesWindow;
    m_outputWindow = new DebuggerOutputWindow;
    m_registerWindow = new RegisterWindow;
    m_stackWindow = new StackWindow;
    m_threadsWindow = new ThreadsWindow;
    m_localsWindow = new WatchWindow(WatchWindow::LocalsType);
    m_watchersWindow = new WatchWindow(WatchWindow::WatchersType);
    //m_tooltipWindow = new WatchWindow(WatchWindow::TooltipType);
    //m_watchersWindow = new QTreeView;
    m_tooltipWindow = new QTreeView;
    m_statusTimer = new QTimer(this);

    m_mainWindow = new QMainWindow;
    m_mainWindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    m_mainWindow->setDocumentMode(true);

    // Stack
    m_stackHandler = new StackHandler;
    QAbstractItemView *stackView =
        qobject_cast<QAbstractItemView *>(m_stackWindow);
    stackView->setModel(m_stackHandler->stackModel());
    connect(stackView, SIGNAL(frameActivated(int)),
        this, SLOT(activateFrame(int)));

    // Threads
    m_threadsHandler = new ThreadsHandler;
    QAbstractItemView *threadsView =
        qobject_cast<QAbstractItemView *>(m_threadsWindow);
    threadsView->setModel(m_threadsHandler->threadsModel());
    connect(threadsView, SIGNAL(threadSelected(int)),
        this, SLOT(selectThread(int)));

    // Disassembler
    m_disassemblerHandler = new DisassemblerHandler;
    QAbstractItemView *disassemblerView =
        qobject_cast<QAbstractItemView *>(m_disassemblerWindow);
    disassemblerView->setModel(m_disassemblerHandler->model());

    // Breakpoints
    m_breakHandler = new BreakHandler;
    QAbstractItemView *breakView =
        qobject_cast<QAbstractItemView *>(m_breakWindow);
    breakView->setModel(m_breakHandler->model());
    connect(breakView, SIGNAL(breakPointActivated(int)),
        m_breakHandler, SLOT(activateBreakPoint(int)));
    connect(breakView, SIGNAL(breakPointDeleted(int)),
        m_breakHandler, SLOT(removeBreakpoint(int)));
    connect(m_breakHandler, SIGNAL(gotoLocation(QString,int,bool)),
        this, SLOT(gotoLocation(QString,int,bool)));
    connect(m_breakHandler, SIGNAL(sessionValueRequested(QString,QVariant*)),
        this, SIGNAL(sessionValueRequested(QString,QVariant*)));
    connect(m_breakHandler, SIGNAL(setSessionValueRequested(QString,QVariant)),
        this, SIGNAL(setSessionValueRequested(QString,QVariant)));

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


    // Registers 
    QAbstractItemView *registerView =
        qobject_cast<QAbstractItemView *>(m_registerWindow);
    m_registerHandler = new RegisterHandler;
    registerView->setModel(m_registerHandler->model());


    m_watchHandler = new WatchHandler;

    // Locals
    QTreeView *localsView = qobject_cast<QTreeView *>(m_localsWindow);
    localsView->setModel(m_watchHandler->model());
    connect(localsView, SIGNAL(requestExpandChildren(QModelIndex)),
        this, SLOT(expandChildren(QModelIndex)));
    connect(localsView, SIGNAL(requestCollapseChildren(QModelIndex)),
        this, SLOT(collapseChildren(QModelIndex)));
    connect(localsView, SIGNAL(requestAssignValue(QString,QString)),
        this, SLOT(assignValueInDebugger(QString,QString)));
    connect(localsView, SIGNAL(requestWatchExpression(QString)),
        this, SLOT(watchExpression(QString)));

    // Watchers 
    QTreeView *watchersView = qobject_cast<QTreeView *>(m_watchersWindow);
    watchersView->setModel(m_watchHandler->model());
    connect(watchersView, SIGNAL(requestAssignValue(QString,QString)),
        this, SLOT(assignValueInDebugger(QString,QString)));
    connect(watchersView, SIGNAL(requestExpandChildren(QModelIndex)),
        this, SLOT(expandChildren(QModelIndex)));
    connect(watchersView, SIGNAL(requestCollapseChildren(QModelIndex)),
        this, SLOT(collapseChildren(QModelIndex)));
    connect(watchersView, SIGNAL(requestWatchExpression(QString)),
        this, SLOT(watchExpression(QString)));
    connect(watchersView, SIGNAL(requestRemoveWatchExpression(QString)),
        this, SLOT(removeWatchExpression(QString)));
    connect(m_watchHandler, SIGNAL(sessionValueRequested(QString,QVariant*)),
        this, SIGNAL(sessionValueRequested(QString,QVariant*)));
    connect(m_watchHandler, SIGNAL(setSessionValueRequested(QString,QVariant)),
        this, SIGNAL(setSessionValueRequested(QString,QVariant)));

    // Tooltip
    QTreeView *tooltipView = qobject_cast<QTreeView *>(m_tooltipWindow);
    tooltipView->setModel(m_watchHandler->model());

    connect(m_watchHandler, SIGNAL(watchModelUpdateRequested()),
        this, SLOT(updateWatchModel()));

    m_startExternalAction = new QAction(this);
    m_startExternalAction->setText(tr("Start and Debug External Application..."));

    m_attachExternalAction = new QAction(this);
    m_attachExternalAction->setText(tr("Attach to Running External Application..."));

    m_continueAction = new QAction(this);
    m_continueAction->setText(tr("Continue"));
    m_continueAction->setIcon(QIcon(":/gdbdebugger/images/debugger_continue_small.png"));

    m_stopAction = new QAction(this);
    m_stopAction->setText(tr("Interrupt"));
    m_stopAction->setIcon(QIcon(":/gdbdebugger/images/debugger_interrupt_small.png"));

    m_resetAction = new QAction(this);
    m_resetAction->setText(tr("Reset Debugger"));

    m_nextAction = new QAction(this);
    m_nextAction->setText(tr("Step Over"));
    //m_nextAction->setShortcut(QKeySequence(tr("F6")));
    m_nextAction->setIcon(QIcon(":/gdbdebugger/images/debugger_stepover_small.png"));

    m_stepAction = new QAction(this);
    m_stepAction->setText(tr("Step Into"));
    //m_stepAction->setShortcut(QKeySequence(tr("F7")));
    m_stepAction->setIcon(QIcon(":/gdbdebugger/images/debugger_stepinto_small.png"));

    m_nextIAction = new QAction(this);
    m_nextIAction->setText(tr("Step Over Instruction"));
    //m_nextIAction->setShortcut(QKeySequence(tr("Shift+F6")));
    m_nextIAction->setIcon(QIcon(":/gdbdebugger/images/debugger_stepoverproc_small.png"));

    m_stepIAction = new QAction(this);
    m_stepIAction->setText(tr("Step One Instruction"));
    //m_stepIAction->setShortcut(QKeySequence(tr("Shift+F9")));
    m_stepIAction->setIcon(QIcon(":/gdbdebugger/images/debugger_steponeproc_small.png"));

    m_stepOutAction = new QAction(this);
    m_stepOutAction->setText(tr("Step Out"));
    //m_stepOutAction->setShortcut(QKeySequence(tr("Shift+F7")));
    m_stepOutAction->setIcon(QIcon(":/gdbdebugger/images/debugger_stepout_small.png"));

    m_runToLineAction = new QAction(this);
    m_runToLineAction->setText(tr("Run to Line"));

    m_runToFunctionAction = new QAction(this);
    m_runToFunctionAction->setText(tr("Run to Outermost Function"));

    m_jumpToLineAction = new QAction(this);
    m_jumpToLineAction->setText(tr("Jump to Line"));

    m_breakAction = new QAction(this);
    m_breakAction->setText(tr("Toggle Breakpoint"));

    m_breakByFunctionAction = new QAction(this);
    m_breakByFunctionAction->setText(tr("Set Breakpoint at Function..."));

    m_breakAtMainAction = new QAction(this);
    m_breakAtMainAction->setText(tr("Set Breakpoint at Function 'main'"));

    m_debugDumpersAction = new QAction(this);
    m_debugDumpersAction->setText(tr("Debug Custom Dumpers"));
    m_debugDumpersAction->setToolTip(tr("This is an internal tool to "
        "make debugging the Custom Data Dumper code easier. "
        "Using this action is in general not needed unless you "
        "want do debug Qt Creator itself."));
    m_debugDumpersAction->setCheckable(true);

    m_skipKnownFramesAction = new QAction(this);
    m_skipKnownFramesAction->setText(tr("Skip Known Frames When Stepping"));
    m_skipKnownFramesAction->setToolTip(tr("After checking this option"
        "'Step Into' combines in certain situations several steps, "
        "leading to 'less noisy' debugging. So will, e.g., the atomic "
        "reference counting code be skipped, and a single 'Step Into' "
        "for a signal emission will end up directly in the slot connected "
        "to it"));
    m_skipKnownFramesAction->setCheckable(true);

    m_useCustomDumpersAction = new QAction(this);
    m_useCustomDumpersAction->setText(tr("Use Custom Display for Qt Objects"));
    m_useCustomDumpersAction->setToolTip(tr("Checking this will make the debugger "
        "try to use code to format certain data (QObject, QString, ...) nicely. "));
    m_useCustomDumpersAction->setCheckable(true);
    m_useCustomDumpersAction->setChecked(true);

    m_useFastStartAction = new QAction(this);
    m_useFastStartAction->setText(tr("Fast Debugger Start"));
    m_useFastStartAction->setToolTip(tr("Checking this will make the debugger "
        "start fast by loading only very few debug symbols on start up. This "
        "might lead to situations where breakpoints can not be set properly. "
        "So uncheck this option if you experience breakpoint related problems."));
    m_useFastStartAction->setCheckable(true);
    m_useFastStartAction->setChecked(true);

    m_useToolTipsAction = new QAction(this);
    m_useToolTipsAction->setText(tr("Use Tooltips While Debugging"));
    m_useToolTipsAction->setToolTip(tr("Checking this will make enable "
        "tooltips for variable values during debugging. Since this can slow "
        "down debugging and does not provide reliable information as it does "
        "not use scope information, it is switched off by default."));
    m_useToolTipsAction->setCheckable(true);
    m_useToolTipsAction->setChecked(false);

    // FIXME
    m_useFastStartAction->setChecked(false);
    m_useFastStartAction->setEnabled(false);

    m_dumpLogAction = new QAction(this);
    m_dumpLogAction->setText(tr("Dump Log File for Debugging Purposes"));

    m_watchAction = new QAction(this);
    m_watchAction->setText(tr("Add to Watch Window"));

    // For usuage hints oin focus{In,Out}
    //connect(m_outputWindow, SIGNAL(statusMessageRequested(QString,int)),
    //    this, SLOT(showStatusMessage(QString,int)));

    connect(m_continueAction, SIGNAL(triggered()),
        this, SLOT(continueExec()));

    connect(m_startExternalAction, SIGNAL(triggered()),
        this, SLOT(startExternalApplication()));
    connect(m_attachExternalAction, SIGNAL(triggered()),
        this, SLOT(attachExternalApplication()));

    connect(m_stopAction, SIGNAL(triggered()),
        this, SLOT(interruptDebuggingRequest()));
    connect(m_resetAction, SIGNAL(triggered()),
        this, SLOT(exitDebugger()));
    connect(m_nextAction, SIGNAL(triggered()),
        this, SLOT(nextExec()));
    connect(m_stepAction, SIGNAL(triggered()),
        this, SLOT(stepExec()));
    connect(m_nextIAction, SIGNAL(triggered()),
        this, SLOT(nextIExec()));
    connect(m_stepIAction, SIGNAL(triggered()),
        this, SLOT(stepIExec()));
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
    connect(m_breakByFunctionAction, SIGNAL(triggered()),
        this, SLOT(breakByFunction()));
    connect(m_breakAtMainAction, SIGNAL(triggered()),
        this, SLOT(breakAtMain()));

    connect(m_useFastStartAction, SIGNAL(triggered()),
        this, SLOT(saveSessionData()));
    connect(m_useCustomDumpersAction, SIGNAL(triggered()),
        this, SLOT(saveSessionData()));
    connect(m_skipKnownFramesAction, SIGNAL(triggered()),
        this, SLOT(saveSessionData()));
    connect(m_dumpLogAction, SIGNAL(triggered()),
        this, SLOT(dumpLog()));
    connect(m_statusTimer, SIGNAL(timeout()),
        this, SLOT(clearStatusMessage()));

    connect(m_outputWindow, SIGNAL(commandExecutionRequested(QString)),
        this, SLOT(executeDebuggerCommand(QString)));


    m_breakDock = createDockForWidget(m_breakWindow);

    m_disassemblerDock = createDockForWidget(m_disassemblerWindow);
    connect(m_disassemblerDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadDisassembler()), Qt::QueuedConnection);

    m_modulesDock = createDockForWidget(m_modulesWindow);
    connect(m_modulesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadModules()), Qt::QueuedConnection);

    m_registerDock = createDockForWidget(m_registerWindow);
    connect(m_registerDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadRegisters()), Qt::QueuedConnection);

    m_outputDock = createDockForWidget(m_outputWindow);

    m_stackDock = createDockForWidget(m_stackWindow);

    m_threadsDock = createDockForWidget(m_threadsWindow);

    setStatus(DebuggerProcessNotReady);
    gdbEngine = createGdbEngine(this);
    winEngine = createWinEngine(this);
    scriptEngine = createScriptEngine(this);
    setDebuggerType(GdbDebugger);
}

void DebuggerManager::setDebuggerType(DebuggerType type)
{
    switch (type) {
        case GdbDebugger:
            m_engine = gdbEngine;
            break;
        case ScriptDebugger:
            m_engine = scriptEngine;
            break;
        case WinDebugger:
            m_engine = winEngine;
            break;
    }
}

IDebuggerEngine *DebuggerManager::engine()
{
    return m_engine;
}

IDebuggerManagerAccessForEngines *DebuggerManager::engineInterface()
{
    return dynamic_cast<IDebuggerManagerAccessForEngines *>(this);
}

IDebuggerManagerAccessForDebugMode *DebuggerManager::debugModeInterface()
{
    return dynamic_cast<IDebuggerManagerAccessForDebugMode *>(this);
}

void DebuggerManager::createDockWidgets()
{
    QSplitter *localsAndWatchers = new QSplitter(Qt::Vertical, 0);
    localsAndWatchers->setWindowTitle(m_localsWindow->windowTitle());
    localsAndWatchers->addWidget(m_localsWindow);
    localsAndWatchers->addWidget(m_watchersWindow);
    localsAndWatchers->setStretchFactor(0, 3);
    localsAndWatchers->setStretchFactor(1, 1);
    m_watchDock = createDockForWidget(localsAndWatchers);
}

QDockWidget *DebuggerManager::createDockForWidget(QWidget *widget)
{
    QDockWidget *dockWidget = new QDockWidget(widget->windowTitle(), m_mainWindow);
    dockWidget->setObjectName(widget->windowTitle());
    //dockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    dockWidget->setAllowedAreas(Qt::AllDockWidgetAreas); // that space is needed.
    //dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
    dockWidget->setTitleBarWidget(new QWidget(dockWidget));
    dockWidget->setWidget(widget);
    connect(dockWidget->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(dockToggled(bool)), Qt::QueuedConnection);
    m_dockWidgets.append(dockWidget);
    return dockWidget;
}

void DebuggerManager::setSimpleDockWidgetArrangement()
{
    foreach (QDockWidget *dockWidget, m_dockWidgets)
        m_mainWindow->removeDockWidget(dockWidget);

    foreach (QDockWidget *dockWidget, m_dockWidgets) {
        m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
        dockWidget->show();
    }

    m_mainWindow->tabifyDockWidget(m_watchDock, m_breakDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_disassemblerDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_modulesDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_outputDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_registerDock);
    m_mainWindow->tabifyDockWidget(m_watchDock, m_threadsDock);

    // They are rarely used even in ordinary debugging. Hiding them also saves
    // cycles since the corresponding information won't be retrieved.
    m_registerDock->hide();
    m_disassemblerDock->hide();
    m_modulesDock->hide();
    m_outputDock->hide();
}

void DebuggerManager::setLocked(bool locked)
{
    const QDockWidget::DockWidgetFeatures features =
            (locked) ? QDockWidget::NoDockWidgetFeatures :
                       QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable;

    foreach (QDockWidget *dockWidget, m_dockWidgets) {
        QWidget *titleBarWidget = dockWidget->titleBarWidget();
        if (locked && !titleBarWidget)
            titleBarWidget = new QWidget(dockWidget);
        else if (!locked && titleBarWidget) {
            delete titleBarWidget;
            titleBarWidget = 0;
        }
        dockWidget->setTitleBarWidget(titleBarWidget);
        dockWidget->setFeatures(features);
    }
}

void DebuggerManager::dockToggled(bool on)
{
    QDockWidget *dw = qobject_cast<QDockWidget *>(sender()->parent());
    if (on && dw)
        dw->raise();
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
    //qDebug() << "STATUS: " << msg;
    showDebuggerOutput("status:", msg);
    m_statusLabel->setText("   " + msg);
    if (timeout > 0) {
        m_statusTimer->setSingleShot(true);
        m_statusTimer->start(timeout);
    } else {
        m_lastPermanentStatusMessage = msg;
        m_statusTimer->stop();
    }
}

void DebuggerManager::notifyStartupFinished()
{
    setStatus(DebuggerProcessReady);
    showStatusMessage(tr("Startup finished. Debugger ready."), -1);
    if (m_startMode == attachExternal) {
        // we continue the execution
        engine()->continueInferior();
    } else {
        engine()->runInferior();
    }
}

void DebuggerManager::notifyInferiorStopped()
{
    resetLocation();
    setStatus(DebuggerInferiorStopped);
    showStatusMessage(tr("Stopped."), 5000);
}

void DebuggerManager::notifyInferiorUpdateFinished()
{
    setStatus(DebuggerInferiorReady);
    showStatusMessage(tr("Stopped."), 5000);
}

void DebuggerManager::notifyInferiorRunningRequested()
{
    setStatus(DebuggerInferiorRunningRequested);
    showStatusMessage(tr("Running..."), 5000);
}

void DebuggerManager::notifyInferiorRunning()
{
    setStatus(DebuggerInferiorRunning);
    showStatusMessage(tr("Running..."), 5000);
}

void DebuggerManager::notifyInferiorExited()
{
    setStatus(DebuggerProcessReady);
    showStatusMessage(tr("Stopped."), 5000);
}

void DebuggerManager::notifyInferiorPidChanged(int pid)
{
    //QMessageBox::warning(0, "PID", "PID: " + QString::number(pid)); 
    //qDebug() << "PID: " << pid; 
    emit inferiorPidChanged(pid);
}

void DebuggerManager::showApplicationOutput(const QString &prefix, const QString &str)
{
     emit applicationOutputAvailable(prefix, str);
}

void DebuggerManager::shutdown()
{
    //qDebug() << "DEBUGGER_MANAGER SHUTDOWN START";
    engine()->shutdown();
    // Delete these manually before deleting the manager
    // (who will delete the models for most views)
    delete m_breakWindow;
    delete m_disassemblerWindow;
    delete m_modulesWindow;
    delete m_outputWindow;
    delete m_registerWindow;
    delete m_stackWindow;
    delete m_threadsWindow;
    delete m_tooltipWindow;
    delete m_watchersWindow;
    delete m_localsWindow;
    // These widgets are all in some layout which will take care of deletion.
    m_breakWindow = 0;
    m_disassemblerWindow = 0;
    m_modulesWindow = 0;
    m_outputWindow = 0;
    m_registerWindow = 0;
    m_stackWindow = 0;
    m_threadsWindow = 0;
    m_tooltipWindow = 0;
    m_watchersWindow = 0;
    m_localsWindow = 0;

    delete m_breakHandler;
    delete m_disassemblerHandler;
    delete m_threadsHandler;
    delete m_modulesHandler;
    delete m_registerHandler;
    delete m_stackHandler;
    delete m_watchHandler;
    m_breakHandler = 0;
    m_disassemblerHandler = 0;
    m_threadsHandler = 0;
    m_modulesHandler = 0;
    m_registerHandler = 0;
    m_stackHandler = 0;
    m_watchHandler = 0;
    //qDebug() << "DEBUGGER_MANAGER SHUTDOWN END";
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
    int index = m_breakHandler->indexOf(fileName, lineNumber);
    if (index == -1)
        breakHandler()->setBreakpoint(fileName, lineNumber);
    else
        breakHandler()->removeBreakpoint(index);
    engine()->attemptBreakpointSynchronization();
}

void DebuggerManager::setToolTipExpression(const QPoint &pos, const QString &exp)
{
    engine()->setToolTipExpression(pos, exp);
}

void DebuggerManager::updateWatchModel()
{
    engine()->updateWatchModel();
}

void DebuggerManager::expandChildren(const QModelIndex &idx)
{
    watchHandler()->expandChildren(idx);
}

void DebuggerManager::collapseChildren(const QModelIndex &idx)
{
    watchHandler()->collapseChildren(idx);
}

void DebuggerManager::removeWatchExpression(const QString &exp)
{
    watchHandler()->removeWatchExpression(exp);
}

QVariant DebuggerManager::sessionValue(const QString &name)
{
    QVariant value;
    emit sessionValueRequested(name, &value);
    return value;
}

void DebuggerManager::querySessionValue(const QString &name, QVariant *value)
{
    emit sessionValueRequested(name, value);
}

void DebuggerManager::setSessionValue(const QString &name, const QVariant &value)
{
    emit setSessionValueRequested(name, value);
}

QVariant DebuggerManager::configValue(const QString &name)
{
    QVariant value;
    emit configValueRequested(name, &value);
    return value;
}

void DebuggerManager::queryConfigValue(const QString &name, QVariant *value)
{
    emit configValueRequested(name, value);
}

void DebuggerManager::setConfigValue(const QString &name, const QVariant &value)
{
    emit setConfigValueRequested(name, value);
}

void DebuggerManager::startExternalApplication()
{
    if (!startNewDebugger(startExternal))
        emit debuggingFinished();
}

void DebuggerManager::attachExternalApplication()
{
    if (!startNewDebugger(attachExternal))
        emit debuggingFinished();
}

bool DebuggerManager::startNewDebugger(StartMode mode)
{
    m_startMode = mode;
    // FIXME: Clean up

    if (startMode() == startExternal) {
        StartExternalDialog dlg(mainWindow());
        dlg.setExecutableFile(
            configValue(QLatin1String("LastExternalExecutableFile")).toString());
        dlg.setExecutableArguments(
            configValue(QLatin1String("LastExternalExecutableArguments")).toString());
        if (dlg.exec() != QDialog::Accepted)
            return false;
        setConfigValue(QLatin1String("LastExternalExecutableFile"),
            dlg.executableFile());
        setConfigValue(QLatin1String("LastExternalExecutableArguments"),
            dlg.executableArguments());
        m_executable = dlg.executableFile();
        m_processArgs = dlg.executableArguments().split(' ');
        m_workingDir = QString();
        m_attachedPID = -1;
    } else if (startMode() == attachExternal) {
        AttachExternalDialog dlg(mainWindow());
        if (dlg.exec() != QDialog::Accepted)
            return false;
        m_executable = QString();
        m_processArgs = QStringList();
        m_workingDir = QString();
        m_attachedPID = dlg.attachPID();
        if (m_attachedPID == 0) {
            QMessageBox::warning(mainWindow(), tr("Warning"),
                tr("Cannot attach to PID 0"));
            return false;
        }
    } else if (startMode() == startInternal) {
        if (m_executable.isEmpty()) {
            QString startDirectory = m_executable;
            if (m_executable.isEmpty()) {
                QString fileName;
                emit currentTextEditorRequested(&fileName, 0, 0);
                if (!fileName.isEmpty()) {
                    const QFileInfo editorFile(fileName);
                    startDirectory = editorFile.dir().absolutePath();
                }
            }
            StartExternalDialog dlg(mainWindow());
            dlg.setExecutableFile(startDirectory);
            if (dlg.exec() != QDialog::Accepted)
                return false;
            m_executable = dlg.executableFile();
            m_processArgs = dlg.executableArguments().split(' ');
            m_workingDir = QString();
            m_attachedPID = 0;
        } else {
            //m_executable = QDir::convertSeparators(m_executable);
            //m_processArgs = sd.processArgs.join(QLatin1String(" "));
            m_attachedPID = 0;
        }
    }

    emit debugModeRequested();

    if (m_executable.endsWith(".js"))
        setDebuggerType(ScriptDebugger);
    else 
        setDebuggerType(GdbDebugger);

    if (!engine()->startDebugger())
        return false;

    m_busy = false;
    setStatus(DebuggerProcessStartingUp);
    return true;
}

void DebuggerManager::cleanupViews()
{
    resetLocation();
    breakHandler()->setAllPending();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    disassemblerHandler()->removeAll();
    modulesHandler()->removeAll();
    watchHandler()->cleanup();
}

void DebuggerManager::exitDebugger()
{
    engine()->exitDebugger();
    cleanupViews();
    setStatus(DebuggerProcessNotReady);
    setBusyCursor(false);
    emit debuggingFinished();
}

void DebuggerManager::assignValueInDebugger(const QString &expr, const QString &value)
{
    engine()->assignValueInDebugger(expr, value);
}

void DebuggerManager::activateFrame(int index)
{
    engine()->activateFrame(index);
}

void DebuggerManager::selectThread(int index)
{
    engine()->selectThread(index);
}

void DebuggerManager::loadAllSymbols()
{
    engine()->loadAllSymbols();
}

void DebuggerManager::loadSymbols(const QString &module)
{
    engine()->loadSymbols(module);
}

void DebuggerManager::stepExec()
{
    resetLocation();
    engine()->stepExec();
} 

void DebuggerManager::stepOutExec()
{
    resetLocation();
    engine()->stepOutExec();
}

void DebuggerManager::nextExec()
{
    resetLocation();
    engine()->nextExec();
}

void DebuggerManager::stepIExec()
{
    resetLocation();
    engine()->stepIExec();
}

void DebuggerManager::nextIExec()
{
    resetLocation();
    engine()->nextIExec();
}

void DebuggerManager::executeDebuggerCommand(const QString &command)
{
    engine()->executeDebuggerCommand(command);
}

void DebuggerManager::sessionLoaded()
{
    exitDebugger();
    loadSessionData();
}

void DebuggerManager::aboutToSaveSession()
{
    saveSessionData();
}

void DebuggerManager::loadSessionData()
{
    m_breakHandler->loadSessionData();
    m_watchHandler->loadSessionData();

    QVariant value;
    querySessionValue(QLatin1String("UseFastStart"), &value);
    m_useFastStartAction->setChecked(value.toBool());
    querySessionValue(QLatin1String("UseToolTips"), &value);
    m_useToolTipsAction->setChecked(value.toBool());
    querySessionValue(QLatin1String("UseCustomDumpers"), &value);
    m_useCustomDumpersAction->setChecked(!value.isValid() || value.toBool());
    querySessionValue(QLatin1String("SkipKnownFrames"), &value);
    m_skipKnownFramesAction->setChecked(value.toBool());
    engine()->loadSessionData();
}

void DebuggerManager::saveSessionData()
{
    m_breakHandler->saveSessionData();
    m_watchHandler->saveSessionData();

    setSessionValue(QLatin1String("UseFastStart"),
        m_useFastStartAction->isChecked());
    setSessionValue(QLatin1String("UseToolTips"),
        m_useToolTipsAction->isChecked());
    setSessionValue(QLatin1String("UseCustomDumpers"),
        m_useCustomDumpersAction->isChecked());
    setSessionValue(QLatin1String("SkipKnownFrames"),
        m_skipKnownFramesAction->isChecked());
    engine()->saveSessionData();
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

#if 0
// call after m_gdbProc exited.
void GdbEngine::procFinished()
{
    //qDebug() << "GDB PROCESS FINISHED";
    setStatus(DebuggerProcessNotReady);
    showStatusMessage(tr("Done"), 5000);
    q->m_breakHandler->procFinished();
    q->m_watchHandler->cleanup();
    m_stackHandler->m_stackFrames.clear();
    m_stackHandler->resetModel();
    m_threadsHandler->resetModel();
    if (q->m_modulesHandler)
        q->m_modulesHandler->procFinished();
    q->resetLocation();
    setStatus(DebuggerProcessNotReady);
    emit q->previousModeRequested();
    emit q->debuggingFinished();
    //exitDebugger();
    //showStatusMessage("Gdb killed");
    m_shortToFullName.clear();
    m_fullToShortName.clear();
    m_shared = 0;
    q->m_busy = false;
}
#endif

void DebuggerManager::addToWatchWindow()
{
    // requires a selection, but that's the only case we want...
    QObject *ob = 0;
    queryCurrentTextEditor(0, 0, &ob);
    QPlainTextEdit *editor = qobject_cast<QPlainTextEdit*>(ob);
    if (!editor)
        return;
    QTextCursor tc = editor->textCursor();
    watchExpression(tc.selectedText());
}

void DebuggerManager::watchExpression(const QString &expression)
{
    watchHandler()->watchExpression(expression);
}

void DebuggerManager::setBreakpoint(const QString &fileName, int lineNumber)
{
    breakHandler()->setBreakpoint(fileName, lineNumber);
    engine()->attemptBreakpointSynchronization();
}

void DebuggerManager::breakByFunction(const QString &functionName)
{
    breakHandler()->breakByFunction(functionName);
    engine()->attemptBreakpointSynchronization();
}

void DebuggerManager::breakByFunction()
{
    BreakByFunctionDialog dlg(m_mainWindow);
    if (dlg.exec()) 
        breakByFunction(dlg.functionName());
}

void DebuggerManager::breakAtMain()
{
#ifdef Q_OS_WIN
    breakByFunction("qMain");
#else
    breakByFunction("main");
#endif
}

void DebuggerManager::setStatus(int status)
{
    //qDebug() << "STATUS CHANGE: from" << m_status << "to" << status;

    if (status == m_status)
        return;

    m_status = status;

    const bool started = status == DebuggerInferiorRunning
        || status == DebuggerInferiorRunningRequested
        || status == DebuggerInferiorStopRequested
        || status == DebuggerInferiorStopped
        || status == DebuggerInferiorUpdating
        || status == DebuggerInferiorUpdateFinishing
        || status == DebuggerInferiorReady;

    const bool starting = status == DebuggerProcessStartingUp;
    const bool running = status == DebuggerInferiorRunning;
    const bool ready = status == DebuggerInferiorStopped
        || status == DebuggerInferiorReady
        || status == DebuggerProcessReady;

    m_startExternalAction->setEnabled(!started && !starting);
    m_attachExternalAction->setEnabled(!started && !starting);
    m_watchAction->setEnabled(ready);
    m_breakAction->setEnabled(true);

    bool interruptIsExit = !running;
    if (interruptIsExit) {
        m_stopAction->setIcon(QIcon(":/gdbdebugger/images/debugger_stop_small.png"));
        m_stopAction->setText(tr("Stop Debugger"));
    } else {
        m_stopAction->setIcon(QIcon(":/gdbdebugger/images/debugger_interrupt_small.png"));
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
    m_stepIAction->setEnabled(ready);
    m_nextIAction->setEnabled(ready);
    //showStatusMessage(QString("started: %1, running: %2").arg(started).arg(running));
    emit statusChanged(m_status);
    const bool notbusy = ready || status == DebuggerProcessNotReady;
    setBusyCursor(!notbusy);
}

void DebuggerManager::setBusyCursor(bool busy)
{
    if (busy == m_busy)
        return;
    //qDebug() << "BUSY: " << busy;
    m_busy = busy;

    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_breakWindow->setCursor(cursor);
    m_disassemblerWindow->setCursor(cursor);
    m_localsWindow->setCursor(cursor);
    m_modulesWindow->setCursor(cursor);
    m_outputWindow->setCursor(cursor);
    m_registerWindow->setCursor(cursor);
    m_stackWindow->setCursor(cursor);
    m_threadsWindow->setCursor(cursor);
    m_tooltipWindow->setCursor(cursor);
    m_watchersWindow->setCursor(cursor);
}

bool DebuggerManager::skipKnownFrames() const
{
    return m_skipKnownFramesAction->isChecked();
}

bool DebuggerManager::debugDumpers() const
{
    return m_debugDumpersAction->isChecked();
}

bool DebuggerManager::useCustomDumpers() const
{
    return m_useCustomDumpersAction->isChecked();
}

bool DebuggerManager::useFastStart() const
{
    return 0; // && m_useFastStartAction->isChecked();
}

void DebuggerManager::queryCurrentTextEditor(QString *fileName, int *lineNumber,
    QObject **object)
{
    emit currentTextEditorRequested(fileName, lineNumber, object);
}

void DebuggerManager::continueExec()
{
    engine()->continueInferior();
}

void DebuggerManager::interruptDebuggingRequest()
{
    //qDebug() << "INTERRUPTING AT" << status();
    bool interruptIsExit = (status() != DebuggerInferiorRunning);
    if (interruptIsExit)
        exitDebugger();
    else {
        setStatus(DebuggerInferiorStopRequested);
        engine()->interruptInferior();
    }
}


void DebuggerManager::runToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (!fileName.isEmpty())
        engine()->runToLineExec(fileName, lineNumber);
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
    //qDebug() << "RUN TO FUNCTION " << functionName;
    if (!functionName.isEmpty())
        engine()->runToFunctionExec(functionName);
}

void DebuggerManager::jumpToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (!fileName.isEmpty())
        engine()->jumpToLineExec(fileName, lineNumber);
}

void DebuggerManager::resetLocation()
{
    //m_watchHandler->removeMouseMoveCatcher(editor->widget());
    emit resetLocationRequested();
}

void DebuggerManager::gotoLocation(const QString &fileName, int line,
    bool setMarker)
{
    emit gotoLocationRequested(fileName, line, setMarker);
    //m_watchHandler->installMouseMoveCatcher(editor->widget());
}


//////////////////////////////////////////////////////////////////////
//
// Disassembler specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadDisassembler()
{
    if (!m_disassemblerDock || !m_disassemblerDock->isVisible())
        return;
    engine()->reloadDisassembler();
}

void DebuggerManager::disassemblerDockToggled(bool on)
{
    if (on)
        reloadDisassembler();
}


//////////////////////////////////////////////////////////////////////
//
// Modules specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadModules()
{
    if (!m_modulesDock || !m_modulesDock->isVisible())
        return;
    engine()->reloadModules();
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

void DebuggerManager::showDebuggerOutput(const QString &prefix, const QString &msg)
{
    m_outputWindow->showOutput(prefix, msg);
}

void DebuggerManager::showDebuggerInput(const QString &prefix, const QString &msg)
{
    m_outputWindow->showInput(prefix, msg);
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
    if (!m_registerDock || !m_registerDock->isVisible())
        return;
    engine()->reloadRegisters();
}


#include "debuggermanager.moc"
