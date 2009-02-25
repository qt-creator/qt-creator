/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "debuggermanager.h"

#include "debuggerconstants.h"
#include "idebuggerengine.h"

#include "breakwindow.h"
#include "disassemblerwindow.h"
#include "debuggeroutputwindow.h"
#include "moduleswindow.h"
#include "registerwindow.h"
#include "stackwindow.h"
#include "sourcefileswindow.h"
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

#include <utils/qtcassert.h>

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


DebuggerSettings::DebuggerSettings()
{
    m_autoRun = false;
    m_autoQuit = false;
    m_skipKnownFrames = false;
    m_debugDumpers = false;
    m_useToolTips = false;
    m_useCustomDumpers = true;
}

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
    m_startMode = StartInternal;

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
    m_sourceFilesWindow = new SourceFilesWindow;
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

    // Source Files
    //m_sourceFilesHandler = new SourceFilesHandler;
    QAbstractItemView *sourceFilesView =
        qobject_cast<QAbstractItemView *>(m_sourceFilesWindow);
    //sourceFileView->setModel(m_stackHandler->stackModel());
    connect(sourceFilesView, SIGNAL(reloadSourceFilesRequested()),
        this, SLOT(reloadSourceFiles()));

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

    m_sourceFilesDock = createDockForWidget(m_sourceFilesWindow);
    connect(m_sourceFilesDock->toggleViewAction(), SIGNAL(toggled(bool)),
        this, SLOT(reloadSourceFiles()), Qt::QueuedConnection);

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
    dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
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
    m_mainWindow->tabifyDockWidget(m_watchDock, m_sourceFilesDock);

    // They are rarely used even in ordinary debugging. Hiding them also saves
    // cycles since the corresponding information won't be retrieved.
    m_sourceFilesDock->hide();
    m_registerDock->hide();
    m_disassemblerDock->hide();
    m_modulesDock->hide();
    m_outputDock->hide();
}

void DebuggerManager::setLocked(bool locked)
{
    const QDockWidget::DockWidgetFeatures features =
            (locked) ? QDockWidget::DockWidgetClosable :
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

void DebuggerManager::notifyInferiorStopRequested()
{
    setStatus(DebuggerInferiorStopRequested);
    showStatusMessage(tr("Stop requested..."), 5000);
}

void DebuggerManager::notifyInferiorStopped()
{
    resetLocation();
    setStatus(DebuggerInferiorStopped);
    showStatusMessage(tr("Stopped."), 5000);
}

void DebuggerManager::notifyInferiorRunningRequested()
{
    setStatus(DebuggerInferiorRunningRequested);
    showStatusMessage(tr("Running requested..."), 5000);
}

void DebuggerManager::notifyInferiorRunning()
{
    setStatus(DebuggerInferiorRunning);
    showStatusMessage(tr("Running..."), 5000);
}

void DebuggerManager::notifyInferiorExited()
{
    setStatus(DebuggerProcessNotReady);
    showStatusMessage(tr("Stopped."), 5000);
}

void DebuggerManager::notifyInferiorPidChanged(int pid)
{
    //QMessageBox::warning(0, "PID", "PID: " + QString::number(pid)); 
    //qDebug() << "PID: " << pid; 
    emit inferiorPidChanged(pid);
}

void DebuggerManager::showApplicationOutput(const QString &str)
{
     emit applicationOutputAvailable(str);
}

void DebuggerManager::shutdown()
{
    //qDebug() << "DEBUGGER_MANAGER SHUTDOWN START";
    if (m_engine) {
        //qDebug() << "SHUTTING DOWN ENGINE" << m_engine;
        m_engine->shutdown();
    }
    m_engine = 0;

    delete scriptEngine;
    scriptEngine = 0;
    delete gdbEngine;
    gdbEngine = 0;
    delete winEngine;
    winEngine = 0;

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
    QTC_ASSERT(m_engine, return);
    QTC_ASSERT(m_breakHandler, return);
    if (status() != DebuggerInferiorRunning
         && status() != DebuggerInferiorStopped 
         && status() != DebuggerProcessNotReady) {
        showStatusMessage(tr("Changing breakpoint state requires either a "
            "fully running or fully stopped application."));
        return;
    }

    int index = m_breakHandler->indexOf(fileName, lineNumber);
    if (index == -1)
        m_breakHandler->setBreakpoint(fileName, lineNumber);
    else
        m_breakHandler->removeBreakpoint(index);
    m_engine->attemptBreakpointSynchronization();
}

void DebuggerManager::setToolTipExpression(const QPoint &pos, const QString &exp)
{
    QTC_ASSERT(m_engine, return);
    m_engine->setToolTipExpression(pos, exp);
}

void DebuggerManager::updateWatchModel()
{
    QTC_ASSERT(m_engine, return);
    m_engine->updateWatchModel();
}

void DebuggerManager::expandChildren(const QModelIndex &idx)
{
    QTC_ASSERT(m_watchHandler, return);
    m_watchHandler->expandChildren(idx);
}

void DebuggerManager::collapseChildren(const QModelIndex &idx)
{
    QTC_ASSERT(m_watchHandler, return);
    m_watchHandler->collapseChildren(idx);
}

void DebuggerManager::removeWatchExpression(const QString &exp)
{
    QTC_ASSERT(m_watchHandler, return);
    m_watchHandler->removeWatchExpression(exp);
}

QVariant DebuggerManager::sessionValue(const QString &name)
{
    // this is answered by the plugin
    QVariant value;
    emit sessionValueRequested(name, &value);
    return value;
}

void DebuggerManager::querySessionValue(const QString &name, QVariant *value)
{
    // this is answered by the plugin
    emit sessionValueRequested(name, value);
}

void DebuggerManager::setSessionValue(const QString &name, const QVariant &value)
{
    // this is answered by the plugin
    emit setSessionValueRequested(name, value);
}

QVariant DebuggerManager::configValue(const QString &name)
{
    // this is answered by the plugin
    QVariant value;
    emit configValueRequested(name, &value);
    return value;
}

void DebuggerManager::queryConfigValue(const QString &name, QVariant *value)
{
    // this is answered by the plugin
    emit configValueRequested(name, value);
}

void DebuggerManager::setConfigValue(const QString &name, const QVariant &value)
{
    // this is answered by the plugin
    emit setConfigValueRequested(name, value);
}

void DebuggerManager::startExternalApplication()
{
    if (!startNewDebugger(StartExternal))
        emit debuggingFinished();
}

void DebuggerManager::attachExternalApplication()
{
    if (!startNewDebugger(AttachExternal))
        emit debuggingFinished();
}

bool DebuggerManager::startNewDebugger(StartMode mode)
{
    m_startMode = mode;
    // FIXME: Clean up

    if (startMode() == StartExternal) {
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
    } else if (startMode() == AttachExternal) {
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
    } else if (startMode() == StartInternal) {
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

    setStatus(DebuggerProcessStartingUp);
    if (!m_engine->startDebugger()) {
        setStatus(DebuggerProcessNotReady);
        return false;
    }

    m_busy = false;
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
    //qDebug() << "DebuggerManager::exitDebugger";
    if (m_engine)
        m_engine->exitDebugger();
    cleanupViews();
    setStatus(DebuggerProcessNotReady);
    setBusyCursor(false);
    emit debuggingFinished();
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

void DebuggerManager::stepExec()
{
    QTC_ASSERT(m_engine, return);
    resetLocation();
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
    m_engine->nextExec();
}

void DebuggerManager::stepIExec()
{
    QTC_ASSERT(m_engine, return);
    resetLocation();
    m_engine->stepIExec();
}

void DebuggerManager::nextIExec()
{
    QTC_ASSERT(m_engine, return);
    resetLocation();
    m_engine->nextIExec();
}

void DebuggerManager::executeDebuggerCommand(const QString &command)
{
    QTC_ASSERT(m_engine, return);
    m_engine->executeDebuggerCommand(command);
}

void DebuggerManager::sessionLoaded()
{
    cleanupViews();
    setStatus(DebuggerProcessNotReady);
    setBusyCursor(false);
    loadSessionData();
}

void DebuggerManager::aboutToSaveSession()
{
    saveSessionData();
}

void DebuggerManager::loadSessionData()
{
    QTC_ASSERT(m_engine, return);
    m_breakHandler->loadSessionData();
    m_watchHandler->loadSessionData();
    m_engine->loadSessionData();
}

void DebuggerManager::saveSessionData()
{
    QTC_ASSERT(m_engine, return);
    m_breakHandler->saveSessionData();
    m_watchHandler->saveSessionData();
    m_engine->saveSessionData();
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
    watchExpression(tc.selectedText());
}

void DebuggerManager::watchExpression(const QString &expression)
{
    QTC_ASSERT(m_watchHandler, return);
    m_watchHandler->watchExpression(expression);
}

void DebuggerManager::setBreakpoint(const QString &fileName, int lineNumber)
{
    QTC_ASSERT(m_breakHandler, return);
    QTC_ASSERT(m_engine, return);
    m_breakHandler->setBreakpoint(fileName, lineNumber);
    m_engine->attemptBreakpointSynchronization();
}

void DebuggerManager::breakByFunction(const QString &functionName)
{
    QTC_ASSERT(m_breakHandler, return);
    QTC_ASSERT(m_engine, return);
    m_breakHandler->breakByFunction(functionName);
    m_engine->attemptBreakpointSynchronization();
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
        || status == DebuggerInferiorStopped;

    const bool starting = status == DebuggerProcessStartingUp;
    const bool running = status == DebuggerInferiorRunning;
    const bool ready = status == DebuggerInferiorStopped;

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
    return m_settings.m_skipKnownFrames;
}

bool DebuggerManager::debugDumpers() const
{
    return m_settings.m_debugDumpers;
}

bool DebuggerManager::useCustomDumpers() const
{
    return m_settings.m_useCustomDumpers;
}

void DebuggerManager::setUseCustomDumpers(bool on)
{
    QTC_ASSERT(m_engine, return);
    m_settings.m_useCustomDumpers = on;
    m_engine->setUseCustomDumpers(on);
}

void DebuggerManager::setDebugDumpers(bool on)
{
    QTC_ASSERT(m_engine, return);
    m_settings.m_debugDumpers = on;
    m_engine->setDebugDumpers(on);
}

void DebuggerManager::setSkipKnownFrames(bool on)
{
    m_settings.m_skipKnownFrames = on;
}

void DebuggerManager::queryCurrentTextEditor(QString *fileName, int *lineNumber,
    QObject **object)
{
    emit currentTextEditorRequested(fileName, lineNumber, object);
}

void DebuggerManager::continueExec()
{
    m_engine->continueInferior();
}

void DebuggerManager::interruptDebuggingRequest()
{
    QTC_ASSERT(m_engine, return);
    //qDebug() << "INTERRUPTING AT" << status();
    bool interruptIsExit = (status() != DebuggerInferiorRunning);
    if (interruptIsExit)
        exitDebugger();
    else {
        setStatus(DebuggerInferiorStopRequested);
        m_engine->interruptInferior();
    }
}


void DebuggerManager::runToLineExec()
{
    QTC_ASSERT(m_engine, return);
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (!fileName.isEmpty())
        m_engine->runToLineExec(fileName, lineNumber);
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
        m_engine->runToFunctionExec(functionName);
}

void DebuggerManager::jumpToLineExec()
{
    QString fileName;
    int lineNumber = -1;
    emit currentTextEditorRequested(&fileName, &lineNumber, 0);
    if (!fileName.isEmpty())
        m_engine->jumpToLineExec(fileName, lineNumber);
}

void DebuggerManager::resetLocation()
{
    // connected to the plugin
    emit resetLocationRequested();
}

void DebuggerManager::gotoLocation(const QString &fileName, int line,
    bool setMarker)
{
    // connected to the plugin
    emit gotoLocationRequested(fileName, line, setMarker);
}


//////////////////////////////////////////////////////////////////////
//
// Disassembler specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadDisassembler()
{
    QTC_ASSERT(m_engine, return);
    if (!m_disassemblerDock || !m_disassemblerDock->isVisible())
        return;
    m_engine->reloadDisassembler();
}

void DebuggerManager::disassemblerDockToggled(bool on)
{
    if (on)
        reloadDisassembler();
}


//////////////////////////////////////////////////////////////////////
//
// Sourec files specific stuff
//
//////////////////////////////////////////////////////////////////////

void DebuggerManager::reloadSourceFiles()
{
    if (!m_sourceFilesDock || !m_sourceFilesDock->isVisible())
        return;
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
    if (!m_modulesDock || !m_modulesDock->isVisible())
        return;
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

void DebuggerManager::showDebuggerOutput(const QString &prefix, const QString &msg)
{
    QTC_ASSERT(m_outputWindow, return);
    m_outputWindow->showOutput(prefix, msg);
}

void DebuggerManager::showDebuggerInput(const QString &prefix, const QString &msg)
{
    QTC_ASSERT(m_outputWindow, return);
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
    m_engine->reloadRegisters();
}


#include "debuggermanager.moc"
