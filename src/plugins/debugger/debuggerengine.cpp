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

#include "debuggerengine.h"

#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerrunner.h"
#include "debuggeroutputwindow.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "sourcefileshandler.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <utils/savedaction.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QAbstractItemView>
#include <QtGui/QStandardItemModel>
#include <QtGui/QAction>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QTextDocument>
#include <QtGui/QTreeWidget>


using namespace ProjectExplorer;
using namespace Debugger;
using namespace Debugger::Internal;


///////////////////////////////////////////////////////////////////////
//
// DebuggerStartParameters
//
///////////////////////////////////////////////////////////////////////

DebuggerStartParameters::DebuggerStartParameters()
  : attachPID(-1),
    useTerminal(false),
    breakAtMain(false),
    toolChainType(ToolChain::UNKNOWN),
    startMode(NoStartMode)
{}

void DebuggerStartParameters::clear()
{
    *this = DebuggerStartParameters();
}


namespace Debugger {

QDebug operator<<(QDebug d, DebuggerState state)
{
    return d << DebuggerEngine::stateName(state) << '(' << int(state) << ')';
}

QDebug operator<<(QDebug str, const DebuggerStartParameters &sp)
{
    QDebug nospace = str.nospace();
    const QString sep = QString(QLatin1Char(','));
    nospace << "executable=" << sp.executable
            << " coreFile=" << sp.coreFile
            << " processArgs=" << sp.processArgs.join(sep)
            << " environment=<" << sp.environment.size() << " variables>"
            << " workingDir=" << sp.workingDirectory
            << " attachPID=" << sp.attachPID
            << " useTerminal=" << sp.useTerminal
            << " remoteChannel=" << sp.remoteChannel
            << " remoteArchitecture=" << sp.remoteArchitecture
            << " symbolFileName=" << sp.symbolFileName
            << " serverStartScript=" << sp.serverStartScript
            << " toolchain=" << sp.toolChainType << '\n';
    return str;
}


namespace Internal {

const char *DebuggerEngine::stateName(int s)
{
#    define SN(x) case x: return #x;
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
#    undef SN
}

//////////////////////////////////////////////////////////////////////
//
// CommandHandler
//
//////////////////////////////////////////////////////////////////////

class CommandHandler : public QStandardItemModel
{
public:
    CommandHandler(DebuggerEngine *engine) : m_engine(engine) {}
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QAbstractItemModel *model() { return this; }

private:
    DebuggerEngine *m_engine;
};

bool CommandHandler::setData(const QModelIndex &, const QVariant &value, int role)
{
    //qDebug() << "COMMAND: " << role << value;

    switch (role) {
        case RequestLoadSessionDataRole:
            m_engine->loadSessionData();
            return true;

        case RequestSaveSessionDataRole:
            m_engine->saveSessionData();
            return true;

        case RequestReloadSourceFilesRole:
            m_engine->reloadSourceFiles();
            return true;

        case RequestReloadModulesRole:
            m_engine->reloadModules();
            return true;

        case RequestExecContinueRole:
            m_engine->continueInferior();
            return true;

        case RequestExecInterruptRole:
            m_engine->interruptInferior();
            return true;

        case RequestExecResetRole:
            //m_engine->exec();
            return true;

        case RequestExecStepRole:
            m_engine->executeStepX();
            return true;

        case RequestExecStepOutRole:
            m_engine->executeStepOutX();
            return true;

        case RequestExecNextRole:
            m_engine->executeStepNextX();
            return true;

        case RequestExecRunToLineRole:
            //m_engine->executeRunToLine();
            QTC_ASSERT(false, /* FIXME ABC */);
            return true;

        case RequestExecRunToFunctionRole:
            //m_engine->executeRunToFunction();
            QTC_ASSERT(false, /* FIXME ABC */);
            return true;

        case RequestExecReturnFromFunctionRole:
            m_engine->executeReturnX();
            return true;

        case RequestExecJumpToLineRole:
            //m_engine->executeJumpToLine();
            QTC_ASSERT(false, /* FIXME ABC */);
            return true;

        case RequestExecWatchRole:
            //m_engine->exec();
            QTC_ASSERT(false, /* FIXME ABC */);
            return true;

        case RequestExecExitRole:
            m_engine->exitDebugger();
            return true;

        case RequestExecSnapshotRole:
            m_engine->makeSnapshot();
            return true;

        case RequestExecFrameDownRole:
            m_engine->frameDown();
            return true;

        case RequestExecFrameUpRole:
            m_engine->frameUp();
            return true;

        case RequestOperatedByInstructionTriggeredRole:
            m_engine->gotoLocation(m_engine->stackHandler()->currentFrame(), true);
            return true;

        case RequestExecuteCommandRole:
            m_engine->executeDebuggerCommand(value.toString());
            return true;
    }

    return false;
}


//////////////////////////////////////////////////////////////////////
//
// DebuggerEnginePrivate
//
//////////////////////////////////////////////////////////////////////

class DebuggerEnginePrivate
{
public:
    DebuggerEnginePrivate(DebuggerEngine *engine, const DebuggerStartParameters &sp)
      : m_engine(engine),
        m_runControl(0),
        m_startParameters(sp),
        m_state(DebuggerNotReady),
        m_breakHandler(engine),
        m_commandHandler(engine),
        m_modulesHandler(engine),
        m_registerHandler(engine),
        m_snapshotHandler(engine),
        m_sourceFilesHandler(engine),
        m_stackHandler(engine),
        m_threadsHandler(engine),
        m_watchHandler(engine),
        m_disassemblerViewAgent(engine)
    {}

public:
    DebuggerEngine *m_engine; // Not owned.
    DebuggerRunControl *m_runControl;  // Not owned.

    DebuggerStartParameters m_startParameters;
    DebuggerState m_state;

    qint64 m_inferiorPid;

    BreakHandler m_breakHandler;
    CommandHandler m_commandHandler;
    ModulesHandler m_modulesHandler;
    RegisterHandler m_registerHandler;
    SnapshotHandler m_snapshotHandler;
    SourceFilesHandler m_sourceFilesHandler;
    StackHandler m_stackHandler;
    ThreadsHandler m_threadsHandler;
    WatchHandler m_watchHandler;
    DisassemblerViewAgent m_disassemblerViewAgent;
};


//////////////////////////////////////////////////////////////////////
//
// DebuggerEngine
//
//////////////////////////////////////////////////////////////////////

DebuggerEngine::DebuggerEngine(const DebuggerStartParameters &startParameters)
  : d(new DebuggerEnginePrivate(this, startParameters))
{
    //loadSessionData();
}

DebuggerEngine::~DebuggerEngine()
{
    //saveSessionData();
}

/*
void DebuggerEngine::showStatusMessage(const QString &msg, int timeout)
{
    plugin()->showStatusMessage(msg, timeout);
}
*/

void DebuggerEngine::showModuleSymbols
    (const QString &moduleName, const Symbols &symbols)
{
    QTreeWidget *w = new QTreeWidget;
    w->setColumnCount(3);
    w->setRootIsDecorated(false);
    w->setAlternatingRowColors(true);
    w->setSortingEnabled(true);
    w->setHeaderLabels(QStringList() << tr("Symbol") << tr("Address") << tr("Code"));
    w->setWindowTitle(tr("Symbols in \"%1\"").arg(moduleName));
    foreach (const Symbol &s, symbols) {
        QTreeWidgetItem *it = new QTreeWidgetItem;
        it->setData(0, Qt::DisplayRole, s.name);
        it->setData(1, Qt::DisplayRole, s.address);
        it->setData(2, Qt::DisplayRole, s.state);
        w->addTopLevelItem(it);
    }
    plugin()->createNewDock(w);
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

ModulesHandler *DebuggerEngine::modulesHandler() const
{
    return &d->m_modulesHandler;
}

BreakHandler *DebuggerEngine::breakHandler() const
{
    return &d->m_breakHandler;
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

SnapshotHandler *DebuggerEngine::snapshotHandler() const
{
    return &d->m_snapshotHandler;
}

SourceFilesHandler *DebuggerEngine::sourceFilesHandler() const
{
    return &d->m_sourceFilesHandler;
}

QAbstractItemModel *DebuggerEngine::modulesModel() const
{
    return d->m_modulesHandler.model();
}

QAbstractItemModel *DebuggerEngine::breakModel() const
{
    return d->m_breakHandler.model();
}

QAbstractItemModel *DebuggerEngine::registerModel() const
{
    return d->m_registerHandler.model();
}

QAbstractItemModel *DebuggerEngine::stackModel() const
{
    return d->m_stackHandler.model();
}

QAbstractItemModel *DebuggerEngine::threadsModel() const
{
    return d->m_threadsHandler.model();
}

QAbstractItemModel *DebuggerEngine::localsModel() const
{
    return d->m_watchHandler.model(LocalsWatch);
}

QAbstractItemModel *DebuggerEngine::watchersModel() const
{
    return d->m_watchHandler.model(WatchersWatch);
}

QAbstractItemModel *DebuggerEngine::returnModel() const
{
    return d->m_watchHandler.model(ReturnWatch);
}

QAbstractItemModel *DebuggerEngine::snapshotModel() const
{
    return d->m_snapshotHandler.model();
}

QAbstractItemModel *DebuggerEngine::sourceFilesModel() const
{
    return d->m_sourceFilesHandler.model();
}

QAbstractItemModel *DebuggerEngine::commandModel() const
{
    return d->m_commandHandler.model();
}

void DebuggerEngine::fetchMemory(MemoryViewAgent *, QObject *,
        quint64 addr, quint64 length)
{
    Q_UNUSED(addr);
    Q_UNUSED(length);
}

void DebuggerEngine::setRegisterValue(int regnr, const QString &value)
{
    Q_UNUSED(regnr);
    Q_UNUSED(value);
}

void DebuggerEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    //qDebug() << channel << msg;
    d->m_runControl->showMessage(msg, channel);
    plugin()->showMessage(msg, channel, timeout);
}

void DebuggerEngine::startDebugger(DebuggerRunControl *runControl)
{
    QTC_ASSERT(runControl, startFailed(); return);
    QTC_ASSERT(!d->m_runControl, startFailed(); return);

    DebuggerEngine *sessionTemplate = plugin()->sessionTemplate();
    QTC_ASSERT(sessionTemplate, startFailed(); return);
    QTC_ASSERT(sessionTemplate != this, startFailed(); return);

    breakHandler()->initializeFromTemplate(sessionTemplate->breakHandler());

    d->m_runControl = runControl;

    QTC_ASSERT(state() == DebuggerNotReady, setState(DebuggerNotReady));

    d->m_inferiorPid = d->m_startParameters.attachPID > 0
        ? d->m_startParameters.attachPID : 0;

    if (d->m_startParameters.environment.empty())
        d->m_startParameters.environment = Environment().toStringList();

    if (d->m_startParameters.breakAtMain)
        breakByFunctionMain();

    const unsigned engineCapabilities = debuggerCapabilities();
    theDebuggerAction(OperateByInstruction)
        ->setEnabled(engineCapabilities & DisassemblerCapability);

    //loadSessionData();
    startDebugger();
}

void DebuggerEngine::breakByFunctionMain()
{
#ifdef Q_OS_WIN
    // FIXME: wrong on non-Qt based binaries
    emit breakByFunction("qMain");
#else
    emit breakByFunction("main");
#endif
}

void DebuggerEngine::breakByFunction(const QString &functionName)
{
    d->m_breakHandler.breakByFunction(functionName);
    attemptBreakpointSynchronization();
}

void DebuggerEngine::loadSessionData()
{
    d->m_breakHandler.loadSessionData();
    d->m_watchHandler.loadSessionData();
}

void DebuggerEngine::saveSessionData()
{
    d->m_breakHandler.saveSessionData();
    d->m_watchHandler.saveSessionData();
}

void DebuggerEngine::resetLocation()
{
    d->m_disassemblerViewAgent.resetLocation();
    d->m_stackHandler.setCurrentIndex(-1);
    plugin()->resetLocation();
}

void DebuggerEngine::gotoLocation(const QString &fileName, int lineNumber, bool setMarker)
{
    StackFrame frame;
    frame.file = fileName;
    frame.line = lineNumber;
    gotoLocation(frame, setMarker);
}

void DebuggerEngine::gotoLocation(const StackFrame &frame, bool setMarker)
{
    if (theDebuggerBoolSetting(OperateByInstruction) || !frame.isUsable()) {
        if (setMarker)
            plugin()->resetLocation();
        d->m_disassemblerViewAgent.setFrame(frame);
    } else {
        plugin()->gotoLocation(frame.file, frame.line, setMarker);
    }
}

void DebuggerEngine::executeStepX()
{
    resetLocation();
    if (theDebuggerBoolSetting(OperateByInstruction))
        executeStepI();
    else
        executeStep();
}

void DebuggerEngine::executeStepOutX()
{
    resetLocation();
    executeStepOut();
}

void DebuggerEngine::executeStepNextX()
{
    resetLocation();
    if (theDebuggerBoolSetting(OperateByInstruction))
        executeNextI();
    else
        executeNext();
}

void DebuggerEngine::executeReturnX()
{
    resetLocation();
    executeReturn();
}

void DebuggerEngine::executeWatchPointX()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        watchPoint(action->data().toPoint());
}

/*
void DebuggerEngine::executeDebuggerCommand()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        executeDebuggerCommand(action->data().toString());
}
void DebuggerManager::executeRunToLine()
{
    ITextEditor *textEditor = d->m_plugin->currentTextEditor();
    QTC_ASSERT(textEditor, return);
    QString fileName = textEditor->file()->fileName();
    int lineNumber = textEditor->currentLine();
    if (d->m_engine && !fileName.isEmpty()) {
        STATE_DEBUG(fileName << lineNumber);
        resetLocation();
        d->m_engine->executeRunToLine(fileName, lineNumber);
    }
}

void DebuggerManager::executeRunToFunction()
{
    ITextEditor *textEditor = d->m_plugin->currentTextEditor();
    QTC_ASSERT(textEditor, return);
    QString fileName = textEditor->file()->fileName();
    QPlainTextEdit *ed = qobject_cast<QPlainTextEdit*>(textEditor->widget());
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

    if (d->m_engine && !functionName.isEmpty()) {
        resetLocation();
        d->m_engine->executeRunToFunction(functionName);
    }
}

void DebuggerManager::executeJumpToLine()
{
    ITextEditor *textEditor = d->m_plugin->currentTextEditor();
    QTC_ASSERT(textEditor, return);
    QString fileName = textEditor->file()->fileName();
    int lineNumber = textEditor->currentLine();
    if (d->m_engine && !fileName.isEmpty()) {
        STATE_DEBUG(fileName << lineNumber);
        d->m_engine->executeJumpToLine(fileName, lineNumber);
    }
}
*/

void DebuggerEngine::cleanup()
{
/*
    FIXME ABS: Still needed?
    modulesHandler()->removeAll();
    breakHandler()->setAllPending();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    watchHandler()->cleanup();
*/
}

const DebuggerStartParameters &DebuggerEngine::startParameters() const
{
    return d->m_startParameters;
}

DebuggerStartParameters &DebuggerEngine::startParameters()
{
    return d->m_startParameters;
}


//////////////////////////////////////////////////////////////////////
//
// Dumpers. "Custom dumpers" are a library compiled against the current
// Qt containing functions to evaluate values of Qt classes
// (such as QString, taking pointers to their addresses).
// The library must be loaded into the debuggee.
//
//////////////////////////////////////////////////////////////////////

bool DebuggerEngine::qtDumperLibraryEnabled() const
{
    return theDebuggerBoolSetting(UseDebuggingHelpers);
}

QStringList DebuggerEngine::qtDumperLibraryLocations() const
{
    if (theDebuggerAction(UseCustomDebuggingHelperLocation)->value().toBool()) {
        const QString customLocation =
            theDebuggerAction(CustomDebuggingHelperLocation)->value().toString();
        const QString location =
            tr("%1 (explicitly set in the Debugger Options)").arg(customLocation);
        return QStringList(location);
    }
    return d->m_startParameters.dumperLibraryLocations;
}

void DebuggerEngine::showQtDumperLibraryWarning(const QString &details)
{
    //QMessageBox dialog(d->m_mainWindow); // FIXME
    QMessageBox dialog;
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
        Core::ICore::instance()->showOptionsDialog(
            _(Qt4ProjectManager::Constants::QT_SETTINGS_CATEGORY),
            _(Qt4ProjectManager::Constants::QTVERSION_SETTINGS_PAGE_ID));
    } else if (dialog.clickedButton() == helperOff) {
        theDebuggerAction(UseDebuggingHelpers)
            ->setValue(qVariantFromValue(false), false);
    }
}

QString DebuggerEngine::qtDumperLibraryName() const
{
    if (theDebuggerAction(UseCustomDebuggingHelperLocation)->value().toBool())
        return theDebuggerAction(CustomDebuggingHelperLocation)->value().toString();
    return startParameters().dumperLibrary;
}

DebuggerState DebuggerEngine::state() const
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

void DebuggerEngine::setState(DebuggerState state, bool forced)
{
    //qDebug() << "STATUS CHANGE: FROM " << stateName(d->m_state)
    //        << " TO " << stateName(state);

    QString msg = _("State changed from %1(%2) to %3(%4).")
     .arg(stateName(d->m_state)).arg(d->m_state).arg(stateName(state)).arg(state);
    //if (!((d->m_state == -1 && state == 0) || (d->m_state == 0 && state == 0)))
    //    qDebug() << msg;
    if (!forced && !isAllowedTransition(d->m_state, state))
        qDebug() << "UNEXPECTED STATE TRANSITION: " << msg;

    showMessage(msg, LogDebug);

    //resetLocation();
    if (state == d->m_state)
        return;

    d->m_state = state;

    plugin()->updateState(this);

    if (d->m_state == DebuggerNotReady) {
        saveSessionData();
        d->m_runControl->debuggingFinished();
    }
}

bool DebuggerEngine::debuggerActionsEnabled() const
{
    return debuggerActionsEnabled(d->m_state);
}

bool DebuggerEngine::debuggerActionsEnabled(DebuggerState state)
{
    switch (state) {
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

void DebuggerEngine::startFailed()
{
    // The concrete engines themselves are responsible for changing state.
    QTC_ASSERT(state() == DebuggerNotReady, setState(DebuggerNotReady));
    d->m_runControl->startFailed();
}

void DebuggerEngine::startSuccessful()
{
    d->m_runControl->startSuccessful();
}

void DebuggerEngine::notifyInferiorPid(qint64 pid)
{
    //STATE_DEBUG(d->m_inferiorPid << pid);
    if (d->m_inferiorPid == pid)
        return;
    d->m_inferiorPid = pid;
    QTimer::singleShot(0, this, SLOT(raiseApplication()));
}

qint64 DebuggerEngine::inferiorPid() const
{
    return d->m_inferiorPid;
}

DebuggerPlugin *DebuggerEngine::plugin() const
{
    return DebuggerPlugin::instance();
}

void DebuggerEngine::raiseApplication()
{
    d->m_runControl->bringApplicationToForeground(d->m_inferiorPid);
}

void DebuggerEngine::openFile(const QString &fileName, int lineNumber)
{
    plugin()->gotoLocation(fileName, lineNumber, false);
}

bool DebuggerEngine::isReverseDebugging() const
{
    return plugin()->isReverseDebugging();
}

} // namespace Internal
} // namespace Debugger
