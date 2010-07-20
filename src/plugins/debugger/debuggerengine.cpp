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
#include "watchutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/environment.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <texteditor/itexteditor.h>

#include <utils/savedaction.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QAbstractItemView>
#include <QtGui/QStandardItemModel>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QTreeWidget>


using namespace Core;
using namespace Debugger;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace TextEditor;

//#define DEBUG_STATE 1
#if DEBUG_STATE
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

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
    //return d << DebuggerEngine::stateName(state) << '(' << int(state) << ')';
    return d << DebuggerEngine::stateName(state);
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
        SN(EngineSetupRequested)
        SN(EngineSetupOk)
        SN(EngineSetupFailed)
        SN(EngineRunFailed)
        SN(InferiorSetupRequested)
        SN(InferiorSetupFailed)
        SN(EngineRunRequested)
        SN(InferiorRunRequested)
        SN(InferiorRunOk)
        SN(InferiorRunFailed)
        SN(InferiorUnrunnable)
        SN(InferiorStopRequested)
        SN(InferiorStopOk)
        SN(InferiorStopFailed)
        SN(InferiorShutdownRequested)
        SN(InferiorShutdownOk)
        SN(InferiorShutdownFailed)
        SN(EngineShutdownRequested)
        SN(EngineShutdownOk)
        SN(EngineShutdownFailed)
        SN(DebuggerFinished)
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
    explicit CommandHandler(DebuggerEngine *engine) : m_engine(engine) {}
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QAbstractItemModel *model() { return this; }

private:
    QPointer<DebuggerEngine> m_engine;
};

bool CommandHandler::setData(const QModelIndex &, const QVariant &value, int role)
{
    QTC_ASSERT(m_engine, qDebug() << value << role; return false);
    m_engine->handleCommand(role, value);
    return true;
}


//////////////////////////////////////////////////////////////////////
//
// DebuggerEnginePrivate
//
//////////////////////////////////////////////////////////////////////

class DebuggerEnginePrivate : public QObject
{
    Q_OBJECT

public:
    DebuggerEnginePrivate(DebuggerEngine *engine, const DebuggerStartParameters &sp)
      : m_engine(engine),
        m_runControl(0),
        m_isActive(false),
        m_startParameters(sp),
        m_state(DebuggerNotReady),
        m_lastGoodState(DebuggerNotReady),
        m_breakHandler(engine),
        m_commandHandler(engine),
        m_modulesHandler(engine),
        m_registerHandler(engine),
        m_sourceFilesHandler(engine),
        m_stackHandler(engine),
        m_threadsHandler(engine),
        m_watchHandler(engine),
        m_disassemblerViewAgent(engine)
    {}

public slots:
    void breakpointSetRemoveMarginActionTriggered();
    void breakpointEnableDisableMarginActionTriggered();
    void handleContextMenuRequest(const QVariant &parameters);

    void doSetupInferior();
    void doRunEngine();
    void doShutdownEngine();
    void doShutdownInferior();
    void doInterruptInferior();
    void doFinishDebugger();

    void queueRunEngine() {
        m_engine->setState(EngineRunRequested);
        m_engine->showMessage(_("QUEUE: RUN ENGINE"));
        QTimer::singleShot(0, this, SLOT(doRunEngine()));
    }

    void queueShutdownEngine() {
        m_engine->setState(EngineShutdownRequested);
        m_engine->showMessage(_("QUEUE: SHUTDOWN ENGINE"));
        QTimer::singleShot(0, this, SLOT(doShutdownEngine()));
    }

    void queueShutdownInferior() {
        m_engine->setState(InferiorShutdownRequested);
        m_engine->showMessage(_("QUEUE: SHUTDOWN INFERIOR"));
        QTimer::singleShot(0, this, SLOT(doShutdownInferior()));
    }

    void queueFinishDebugger() {
        m_engine->setState(DebuggerFinished, true);
        m_engine->showMessage(_("QUEUE: SHUTDOWN INFERIOR"));
        QTimer::singleShot(0, this, SLOT(doFinishDebugger()));
    }

    void raiseApplication() {
        m_runControl->bringApplicationToForeground(m_inferiorPid);
    }


public:
    DebuggerState state() const { return m_state; }

    DebuggerEngine *m_engine; // Not owned.
    DebuggerRunControl *m_runControl;  // Not owned.
    bool m_isActive;

    DebuggerStartParameters m_startParameters;

    // The current state.
    DebuggerState m_state;

    // The state we had before something unexpected happend.
    DebuggerState m_lastGoodState;

    // The state we are aiming for.
    DebuggerState m_targetState;

    qint64 m_inferiorPid;

    BreakHandler m_breakHandler;
    CommandHandler m_commandHandler;
    ModulesHandler m_modulesHandler;
    RegisterHandler m_registerHandler;
    SourceFilesHandler m_sourceFilesHandler;
    StackHandler m_stackHandler;
    ThreadsHandler m_threadsHandler;
    WatchHandler m_watchHandler;
    DisassemblerViewAgent m_disassemblerViewAgent;
};

void DebuggerEnginePrivate::breakpointSetRemoveMarginActionTriggered()
{
    QAction *act = qobject_cast<QAction *>(sender());
    QTC_ASSERT(act, return);
    QList<QVariant> list = act->data().toList();
    QTC_ASSERT(list.size() == 2, return);
    const QString fileName = list.at(0).toString();
    const int lineNumber = list.at(1).toInt();
    m_breakHandler.toggleBreakpoint(fileName, lineNumber);
}

void DebuggerEnginePrivate::breakpointEnableDisableMarginActionTriggered()
{
    QAction *act = qobject_cast<QAction *>(sender());
    QTC_ASSERT(act, return);
    QList<QVariant> list = act->data().toList();
    QTC_ASSERT(list.size() == 2, return);
    const QString fileName = list.at(0).toString();
    const int lineNumber = list.at(1).toInt();
    m_breakHandler.toggleBreakpointEnabled(fileName, lineNumber);
}

void DebuggerEnginePrivate::handleContextMenuRequest(const QVariant &parameters)
{
    const QList<QVariant> list = parameters.toList();
    QTC_ASSERT(list.size() == 3, return);
    TextEditor::ITextEditor *editor =
        (TextEditor::ITextEditor *)(list.at(0).value<quint64>());
    int lineNumber = list.at(1).toInt();
    QMenu *menu = (QMenu *)(list.at(2).value<quint64>());

    BreakpointData *data = 0;
    QString position;
    QString fileName;
    if (editor->property("DisassemblerView").toBool()) {
        fileName = editor->file()->fileName();
        QString line = editor->contents()
            .section('\n', lineNumber - 1, lineNumber - 1);
        position = _("*") + fileName;
        BreakpointData needle;
        needle.bpAddress = line.left(line.indexOf(QLatin1Char(' '))).toLatin1();
        needle.bpLineNumber = "-1";
        data = m_breakHandler.findSimilarBreakpoint(&needle);
    } else {
        fileName = editor->file()->fileName();
        position = fileName + QString(":%1").arg(lineNumber);
        data = m_breakHandler.findBreakpoint(fileName, lineNumber);
    }

    QList<QVariant> args;
    args.append(fileName);
    args.append(lineNumber);

    if (data) {
        // existing breakpoint
        QAction *act = new QAction(tr("Remove Breakpoint"), menu);
        act->setData(args);
        connect(act, SIGNAL(triggered()),
            this, SLOT(breakpointSetRemoveMarginActionTriggered()));
        menu->addAction(act);

        QAction *act2;
        if (data->enabled)
            act2 = new QAction(tr("Disable Breakpoint"), menu);
        else
            act2 = new QAction(tr("Enable Breakpoint"), menu);
        act2->setData(args);
        connect(act2, SIGNAL(triggered()),
            this, SLOT(breakpointEnableDisableMarginActionTriggered()));
        menu->addAction(act2);
    } else {
        // non-existing
        QAction *act = new QAction(tr("Set Breakpoint"), menu);
        act->setData(args);
        connect(act, SIGNAL(triggered()),
            this, SLOT(breakpointSetRemoveMarginActionTriggered()));
        menu->addAction(act);
    }
}

//////////////////////////////////////////////////////////////////////
//
// DebuggerEngine
//
//////////////////////////////////////////////////////////////////////

DebuggerEngine::DebuggerEngine(const DebuggerStartParameters &startParameters)
  : d(new DebuggerEnginePrivate(this, startParameters))
{
}

DebuggerEngine::~DebuggerEngine()
{
    disconnect();
    delete d;
}

void DebuggerEngine::showStatusMessage(const QString &msg, int timeout) const
{
    showMessage(msg, StatusBar, timeout);
}

void DebuggerEngine::handleCommand(int role, const QVariant &value)
{
    //qDebug() << "COMMAND: " << role << value;

    switch (role) {
        case RequestReloadSourceFilesRole:
            reloadSourceFiles();
            break;

        case RequestReloadModulesRole:
            reloadModules();
            break;

        case RequestReloadRegistersRole:
            reloadRegisters();
            break;

        case RequestExecDetachRole:
            detachDebugger();
            break;

        case RequestExecContinueRole:
            continueInferior();
            break;

        case RequestExecInterruptRole:
            requestInterruptInferior();
            break;

        case RequestExecResetRole:
            notifyEngineIll(); // FIXME: check
            break;

        case RequestExecStepRole:
            executeStepX();
            break;

        case RequestExecStepOutRole:
            executeStepOutX();
            break;

        case RequestExecNextRole:
            executeStepNextX();
            break;

        case RequestExecRunToLineRole:
            executeRunToLine();
            break;

        case RequestExecRunToFunctionRole:
            executeRunToFunction();
            break;

        case RequestExecReturnFromFunctionRole:
            executeReturnX();
            break;

        case RequestExecJumpToLineRole:
            executeJumpToLine();
            break;

        case RequestExecWatchRole:
            addToWatchWindow();
            break;

        case RequestExecExitRole:
            d->queueShutdownInferior();
            break;

        case RequestMakeSnapshotRole:
            makeSnapshot();
            break;

        case RequestActivationRole:
            setActive(value.toBool());
            break;

        case RequestExecFrameDownRole:
            frameDown();
            break;

        case RequestExecFrameUpRole:
            frameUp();
            break;

        case RequestOperatedByInstructionTriggeredRole:
            gotoLocation(stackHandler()->currentFrame(), true);
            break;

        case RequestExecuteCommandRole:
            executeDebuggerCommand(value.toString());
            break;

        case RequestToggleBreakpointRole: {
            QList<QVariant> list = value.toList();
            QTC_ASSERT(list.size() == 2, break);
            const QString fileName = list.at(0).toString();
            const int lineNumber = list.at(1).toInt();
            breakHandler()->toggleBreakpoint(fileName, lineNumber);
            break;
        }

        case RequestToolTipByExpressionRole: {
            QList<QVariant> list = value.toList();
            QTC_ASSERT(list.size() == 3, break);
            QPoint point = list.at(0).value<QPoint>();
            TextEditor::ITextEditor *editor = // Eeks.
                (TextEditor::ITextEditor *)(list.at(1).value<quint64>());
            int pos = list.at(2).toInt();
            setToolTipExpression(point, editor, pos);
            break;
        }

        case RequestContextMenuRole: {
            QList<QVariant> list = value.toList();
            QTC_ASSERT(list.size() == 3, break);
            d->handleContextMenuRequest(list);
            break;
        }
    }
}

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

//SnapshotHandler *DebuggerEngine::snapshotHandler() const
//{
//    return &d->m_snapshotHandler;
//}

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
    //if (msg.size() && msg.at(0).isUpper() && msg.at(1).isUpper())
    //    qDebug() << qPrintable(msg) << "IN STATE" << state();
    d->m_runControl->showMessage(msg, channel);
    plugin()->showMessage(msg, channel, timeout);
}

void DebuggerEngine::startDebugger(DebuggerRunControl *runControl)
{
    QTC_ASSERT(runControl, notifyEngineSetupFailed(); return);
    QTC_ASSERT(!d->m_runControl, notifyEngineSetupFailed(); return);

    DebuggerEngine *sessionTemplate = plugin()->sessionTemplate();
    QTC_ASSERT(sessionTemplate, notifyEngineSetupFailed(); return);
    QTC_ASSERT(sessionTemplate != this, notifyEngineSetupFailed(); return);

    breakHandler()->initializeFromTemplate(sessionTemplate->breakHandler());
    watchHandler()->initializeFromTemplate(sessionTemplate->watchHandler());

    d->m_runControl = runControl;

    QTC_ASSERT(state() == DebuggerNotReady, qDebug() << state());

    d->m_inferiorPid = d->m_startParameters.attachPID > 0
        ? d->m_startParameters.attachPID : 0;

    if (d->m_startParameters.environment.empty())
        d->m_startParameters.environment = Environment().toStringList();

    if (d->m_startParameters.breakAtMain)
        breakByFunctionMain();

    const unsigned engineCapabilities = debuggerCapabilities();
    theDebuggerAction(OperateByInstruction)
        ->setEnabled(engineCapabilities & DisassemblerCapability);

    setState(EngineSetupRequested);
    setupEngine();
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

static TextEditor::ITextEditor *currentTextEditor()
{
    EditorManager *editorManager = EditorManager::instance();
    if (!editorManager)
        return 0;
    Core::IEditor *editor = editorManager->currentEditor();
    return qobject_cast<ITextEditor*>(editor);
}

void DebuggerEngine::executeRunToLine()
{
    ITextEditor *textEditor = currentTextEditor();
    QTC_ASSERT(textEditor, return);
    QString fileName = textEditor->file()->fileName();
    if (fileName.isEmpty())
        return;
    int lineNumber = textEditor->currentLine();
    resetLocation();
    executeRunToLine(fileName, lineNumber);
}

void DebuggerEngine::executeRunToFunction()
{
    ITextEditor *textEditor = currentTextEditor();
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

    if (functionName.isEmpty())
        return;
    resetLocation();
    executeRunToFunction(functionName);
}

void DebuggerEngine::executeJumpToLine()
{
    ITextEditor *textEditor = currentTextEditor();
    QTC_ASSERT(textEditor, return);
    QString fileName = textEditor->file()->fileName();
    int lineNumber = textEditor->currentLine();
    if (fileName.isEmpty())
        return;
    executeJumpToLine(fileName, lineNumber);
}

void DebuggerEngine::addToWatchWindow()
{
    // Requires a selection, but that's the only case we want anyway.
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
    if (exp.isEmpty())
        return;
    watchHandler()->watchExpression(exp);
}

// Called from RunControl.
void DebuggerEngine::handleFinished()
{
    modulesHandler()->removeAll();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    watchHandler()->cleanup();

    DebuggerEngine *sessionTemplate = plugin()->sessionTemplate();
    QTC_ASSERT(sessionTemplate != this, /**/);
    breakHandler()->storeToTemplate(sessionTemplate->breakHandler());
    watchHandler()->storeToTemplate(sessionTemplate->watchHandler());
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

DebuggerState DebuggerEngine::lastGoodState() const
{
    return d->m_lastGoodState;
}

DebuggerState DebuggerEngine::targetState() const
{
    return d->m_targetState;
}

static bool isAllowedTransition(DebuggerState from, DebuggerState to)
{
    switch (from) {
    case DebuggerNotReady:
        return to == EngineSetupRequested;

    case EngineSetupRequested:
        return to == EngineSetupOk || to == EngineSetupFailed;
    case EngineSetupFailed:
        // FIXME: In therory it's the engine's task to go into a 
        // proper "Shutdown" state before calling notifyEngineSetupFailed
        //return to == DebuggerFinished;
        return to == EngineShutdownRequested;
    case EngineSetupOk:
        return to == InferiorSetupRequested || to == EngineShutdownRequested;

    case InferiorSetupRequested:
        return to == EngineRunRequested || to == InferiorSetupFailed;
    case InferiorSetupFailed:
        return to == EngineShutdownRequested;

    case EngineRunRequested:
        return to == InferiorRunRequested || to == InferiorStopRequested
            || to == InferiorUnrunnable || to == EngineRunFailed;

    case EngineRunFailed:
        return to == InferiorShutdownRequested;

    case InferiorRunRequested:
        return to == InferiorRunOk || to == InferiorRunFailed;
    case InferiorRunFailed:
        return to == InferiorStopOk;
    case InferiorRunOk:
        return to == InferiorStopRequested || to == InferiorStopOk;

    case InferiorStopRequested:
        return to == InferiorStopOk || to == InferiorStopFailed;
    case InferiorStopOk:
        return to == InferiorRunRequested || to == InferiorShutdownRequested
            || to == InferiorStopOk;
    case InferiorStopFailed:
        return to == EngineShutdownRequested;

    case InferiorUnrunnable:
        return to == InferiorShutdownRequested;
    case InferiorShutdownRequested:
        return to == InferiorShutdownOk || to == InferiorShutdownFailed;
    case InferiorShutdownOk:
        return to == EngineShutdownRequested;
    case InferiorShutdownFailed:
        return to == EngineShutdownRequested;

    case EngineShutdownRequested:
        return to == EngineShutdownOk;
    case EngineShutdownOk:
        return to == DebuggerFinished;
    case EngineShutdownFailed:
        return to == DebuggerFinished;

    case DebuggerFinished:
        return false;
    }

    qDebug() << "UNKNOWN STATE:" << from;
    return false;
}

void DebuggerEngine::notifyEngineSetupFailed()
{
    showMessage(_("NOTE: ENGINE SETUP FAILED"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    setState(EngineSetupFailed);
    d->m_runControl->startFailed();
    d->queueShutdownEngine();
}

void DebuggerEngine::notifyEngineSetupOk()
{
    showMessage(_("NOTE: ENGINE SETUP OK"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    setState(EngineSetupOk);
    d->m_runControl->startSuccessful();
    showMessage(_("QUEUE: SETUP INFERIOR"));
    QTimer::singleShot(0, d, SLOT(doSetupInferior()));
}

void DebuggerEnginePrivate::doSetupInferior()
{
    QTC_ASSERT(state() == EngineSetupOk, qDebug() << state());
    m_engine->setState(InferiorSetupRequested);
    m_engine->showMessage(_("CALL: SETUP INFERIOR"));
    m_engine->setupInferior();
}

void DebuggerEngine::notifyInferiorSetupFailed()
{
    showMessage(_("NOTE: INFERIOR SETUP FAILED"));
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    setState(InferiorSetupFailed);
    d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorSetupOk()
{
    showMessage(_("NOTE: INFERIOR SETUP OK"));
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    d->queueRunEngine();
}

void DebuggerEnginePrivate::doRunEngine()
{
    m_engine->showMessage(_("CALL: RUN ENGINE"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->runEngine();
}

void DebuggerEngine::notifyInferiorUnrunnable()
{
    showMessage(_("NOTE: INFERIOR UNRUNNABLE"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    setState(InferiorUnrunnable);
}

void DebuggerEngine::notifyEngineRunFailed()
{
    showMessage(_("NOTE: ENGINE RUN FAILED"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    setState(EngineRunFailed);
    d->queueShutdownInferior();
}

void DebuggerEngine::notifyEngineRunAndInferiorRunOk()
{
    showMessage(_("NOTE: ENGINE RUN AND INFERIOR RUN OK"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    setState(InferiorRunRequested);
    notifyInferiorRunOk();
}

void DebuggerEngine::notifyEngineRunAndInferiorStopOk()
{
    showMessage(_("NOTE: ENGINE RUN AND INFERIOR STOP OK"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    setState(InferiorStopRequested);
    notifyInferiorStopOk();
}

void DebuggerEngine::notifyInferiorRunRequested()
{
    showMessage(_("NOTE: INFERIOR RUN REQUESTED"));
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    setState(InferiorRunRequested);
}

void DebuggerEngine::notifyInferiorRunOk()
{
    showMessage(_("NOTE: INFERIOR RUN OK"));
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    setState(InferiorRunOk);
}

void DebuggerEngine::notifyInferiorRunFailed()
{
    showMessage(_("NOTE: INFERIOR RUN FAILED"));
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
    setState(InferiorRunFailed);
    setState(InferiorStopOk);
    if (isDying())
        d->queueShutdownInferior();
}

void DebuggerEngine::notifyInferiorStopOk()
{
    showMessage(_("NOTE: INFERIOR STOP OK"));
    // Ignore spurious notifications after we are set to die.
    if (isDying()) {
        showMessage(_("NOTE: ... WHILE DYING. "));
        // Forward state to "StopOk" if needed.
        if (state() == InferiorStopRequested
                || state() == InferiorRunRequested
                || state() == InferiorRunOk) {
            showMessage(_("NOTE: ... FORWARDING TO 'STOP OK'. "));
            setState(InferiorStopOk);
        }
        if (state() == InferiorStopOk || state() == InferiorStopFailed) {
            d->queueShutdownInferior();
        }
        showMessage(_("NOTE: ... IGNORING STOP MESSAGE"));
        return;
    }
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorSpontaneousStop()
{
    showMessage(_("NOTE: INFERIOR SPONTANEOUES STOP"));
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << state());
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorStopFailed()
{
    showMessage(_("NOTE: INFERIOR STOP FAILED"));
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
    setState(InferiorStopFailed);
    d->queueShutdownEngine();
}

void DebuggerEnginePrivate::doInterruptInferior()
{
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << state());
    m_engine->setState(InferiorStopRequested);
    m_engine->showMessage(_("CALL: INTERRUPT INFERIOR"));
    m_engine->interruptInferior();
}

void DebuggerEnginePrivate::doShutdownInferior()
{
    m_engine->resetLocation();
    m_targetState = DebuggerFinished;
    m_engine->showMessage(_("CALL: SHUTDOWN INFERIOR"));
    m_engine->shutdownInferior();
}

void DebuggerEngine::notifyInferiorShutdownOk()
{
    showMessage(_("INFERIOR SUCCESSFULLY SHUT DOWN"));
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    d->m_lastGoodState = DebuggerNotReady; // A "neutral" value.
    setState(InferiorShutdownOk);
    d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorShutdownFailed()
{
    showMessage(_("INFERIOR SHUTDOWN FAILED"));
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << this << state());
    setState(InferiorShutdownFailed);
    d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorIll()
{
    showMessage(_("NOTE: INFERIOR ILL"));
    // This can be issued in almost any state. The inferior could still be
    // alive as some previous notifications might have been bogus.
    d->m_targetState = DebuggerFinished;
    d->m_lastGoodState = d->m_state;
    if (state() == InferiorRunRequested) {
        // We asked for running, but did not see a response.
        // Assume the inferior is dead.
        // FIXME: Use timeout?
        setState(InferiorRunFailed);
        setState(InferiorStopOk);
    }
    d->queueShutdownInferior();
}

void DebuggerEnginePrivate::doShutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested
           || state() == InferiorShutdownOk, qDebug() << state());
    m_targetState = DebuggerFinished;
    m_engine->showMessage(_("CALL: SHUTDOWN ENGINE"));
    m_engine->shutdownEngine();
}

void DebuggerEngine::notifyEngineShutdownOk()
{
    showMessage(_("NOTE: ENGINE SHUTDOWN OK"));
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    setState(EngineShutdownOk);
    QTimer::singleShot(0, d, SLOT(doFinishDebugger()));
}

void DebuggerEngine::notifyEngineShutdownFailed()
{
    showMessage(_("NOTE: ENGINE SHUTDOWN FAILED"));
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    setState(EngineShutdownFailed);
    QTimer::singleShot(0, d, SLOT(doFinishDebugger()));
}

void DebuggerEnginePrivate::doFinishDebugger()
{
    m_engine->showMessage(_("NOTE: FINISH DEBUGGER"));
    QTC_ASSERT(state() == EngineShutdownOk
        || state() == EngineShutdownFailed, qDebug() << state());
    m_engine->resetLocation();
    m_engine->setState(DebuggerFinished);
    m_runControl->debuggingFinished();
}

void DebuggerEngine::notifyEngineIll()
{
    showMessage(_("NOTE: ENGINE ILL ******"));
    d->m_targetState = DebuggerFinished;
    d->m_lastGoodState = d->m_state;
    switch (state()) {
        case InferiorRunRequested:
        case InferiorRunOk:
        case InferiorStopRequested:
        case InferiorStopOk:
            qDebug() << "FORWARDING STATE TO " << InferiorShutdownFailed;
            setState(InferiorShutdownFailed, true);
            break;
        default:
            break;
    }
    d->queueShutdownEngine();
}

void DebuggerEngine::notifyEngineSpontaneousShutdown()
{
    showMessage(_("NOTE: ENGINE SPONTANEOUS SHUTDOWN"));
    d->queueFinishDebugger();
}

void DebuggerEngine::notifyInferiorExited()
{
    showMessage(_("NOTE: INFERIOR EXITED"));
    resetLocation();

    // This can be issued in almost any state. We assume, though,
    // that at this point of time the inferior is not running anymore,
    // even if stop notification were not issued or got lost.
    if (state() == InferiorRunOk) {
        setState(InferiorStopRequested);
        setState(InferiorStopOk);
    }
    setState(InferiorShutdownRequested);
    setState(InferiorShutdownOk);
    d->queueShutdownEngine();
}

void DebuggerEngine::setState(DebuggerState state, bool forced)
{
    //qDebug() << "STATUS CHANGE: FROM " << stateName(d->m_state)
    //        << " TO " << stateName(state);

    DebuggerState oldState = d->m_state;
    d->m_state = state;

    QString msg = _("State changed%5 from %1(%2) to %3(%4).")
     .arg(stateName(oldState)).arg(oldState).arg(stateName(state)).arg(state)
     .arg(forced ? " BY FORCE" : "");
    if (!forced && !isAllowedTransition(oldState, state))
        qDebug() << "UNEXPECTED STATE TRANSITION: " << msg;

    showMessage(msg, LogDebug);
    plugin()->updateState(this);
}

bool DebuggerEngine::debuggerActionsEnabled() const
{
    return debuggerActionsEnabled(d->m_state);
}

bool DebuggerEngine::debuggerActionsEnabled(DebuggerState state)
{
    switch (state) {
    case InferiorSetupRequested:
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
    case InferiorSetupFailed:
    case InferiorStopFailed:
    case InferiorShutdownRequested:
    case InferiorShutdownOk:
    case InferiorShutdownFailed:
    case EngineShutdownRequested:
    case EngineShutdownOk:
    case EngineShutdownFailed:
    case DebuggerFinished:
        return false;
    }
    return false;
}

void DebuggerEngine::notifyInferiorPid(qint64 pid)
{
    showMessage(tr("Taking notice of pid %1").arg(pid));
    if (d->m_inferiorPid == pid)
        return;
    d->m_inferiorPid = pid;
    QTimer::singleShot(0, d, SLOT(raiseApplication()));
}

qint64 DebuggerEngine::inferiorPid() const
{
    return d->m_inferiorPid;
}

DebuggerPlugin *DebuggerEngine::plugin() const
{
    return DebuggerPlugin::instance();
}

void DebuggerEngine::openFile(const QString &fileName, int lineNumber)
{
    plugin()->gotoLocation(fileName, lineNumber, false);
}

bool DebuggerEngine::isReverseDebugging() const
{
    return plugin()->isReverseDebugging();
}

bool DebuggerEngine::isActive() const
{
    return d->m_isActive && !isSessionEngine();
}

void DebuggerEngine::setActive(bool on)
{
    //qDebug() << "SETTING ACTIVE" << this << on;
    d->m_isActive = on;
    //breakHandler()->updateMarkers();
}

// called by DebuggerRunControl
void DebuggerEngine::quitDebugger()
{
    showMessage("QUIT DEBUGGER REQUESTED");
    d->m_targetState = DebuggerFinished;
    if (state() == InferiorStopOk) {
        d->queueShutdownInferior();
    } else if (state() == InferiorRunOk) {
        d->doInterruptInferior();
    } else {
        // FIXME: We should disable the actions connected to that
        notifyInferiorIll();
    }
}

void DebuggerEngine::requestInterruptInferior()
{
    d->doInterruptInferior();
}

} // namespace Internal
} // namespace Debugger

#include "debuggerengine.moc"
