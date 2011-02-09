/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerengine.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerplugin.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltip.h"
#include "debuggerstartparameters.h"

#include "memoryagent.h"
#include "disassembleragent.h"
#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "snapshothandler.h"
#include "sourcefileshandler.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"

#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchaintype.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetextmark.h>

#include <utils/savedaction.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QFutureInterface>

#include <QtGui/QMessageBox>

using namespace Core;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace TextEditor;

enum { debug = 0 };

#define SDEBUG(s) if (!debug) {} else qDebug() << s;
#define XSDEBUG(s) qDebug() << s


///////////////////////////////////////////////////////////////////////
//
// DebuggerStartParameters
//
///////////////////////////////////////////////////////////////////////

namespace Debugger {

Internal::Location::Location(const StackFrame &frame, bool marker)
{
    init();
    m_fileName = frame.file;
    m_lineNumber = frame.line;
    m_needsMarker = marker;
    m_functionName = frame.function;
    m_hasDebugInfo = frame.isUsable();
    m_address = frame.address;
}

QDebug operator<<(QDebug d, DebuggerState state)
{
    //return d << DebuggerEngine::stateName(state) << '(' << int(state) << ')';
    return d << DebuggerEngine::stateName(state);
}

QDebug operator<<(QDebug str, const DebuggerStartParameters &sp)
{
    QDebug nospace = str.nospace();
    nospace << "executable=" << sp.executable
            << " coreFile=" << sp.coreFile
            << " processArgs=" << sp.processArgs
            << " environment=<" << sp.environment.size() << " variables>"
            << " workingDir=" << sp.workingDirectory
            << " attachPID=" << sp.attachPID
            << " useTerminal=" << sp.useTerminal
            << " remoteChannel=" << sp.remoteChannel
            << " remoteArchitecture=" << sp.remoteArchitecture
            << " symbolFileName=" << sp.symbolFileName
            << " useServerStartScript=" << sp.useServerStartScript
            << " serverStartScript=" << sp.serverStartScript
            << " toolchain=" << sp.toolChainType << '\n';
    return str;
}


///////////////////////////////////////////////////////////////////////
//
// LocationMark
//
///////////////////////////////////////////////////////////////////////

// Used in "real" editors
class LocationMark : public TextEditor::BaseTextMark
{
public:
    LocationMark(const QString &fileName, int linenumber)
        : BaseTextMark(fileName, linenumber)
    {}

    QIcon icon() const { return debuggerCore()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
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
    DebuggerEnginePrivate(DebuggerEngine *engine,
            DebuggerEngine *masterEngine,
            const DebuggerStartParameters &sp)
      : m_engine(engine),
        m_masterEngine(masterEngine),
        m_runControl(0),
        m_startParameters(sp),
        m_state(DebuggerNotReady),
        m_lastGoodState(DebuggerNotReady),
        m_targetState(DebuggerNotReady),
        m_modulesHandler(),
        m_registerHandler(),
        m_sourceFilesHandler(),
        m_stackHandler(),
        m_threadsHandler(),
        m_watchHandler(engine),
        m_disassemblerAgent(engine),
        m_memoryAgent(engine),
        m_isStateDebugging(false)
    {
        connect(&m_locationTimer, SIGNAL(timeout()), SLOT(resetLocation()));
    }

    ~DebuggerEnginePrivate() {}

public slots:
    void doSetupEngine();
    void doSetupInferior();
    void doRunEngine();
    void doShutdownEngine();
    void doShutdownInferior();
    void doInterruptInferior();
    void doFinishDebugger();

    void queueSetupEngine()
    {
        m_engine->setState(EngineSetupRequested);
        m_engine->showMessage(_("QUEUE: SETUP ENGINE"));
        QTimer::singleShot(0, this, SLOT(doSetupEngine()));
    }

    void queueSetupInferior()
    {
        m_engine->setState(InferiorSetupRequested);
        m_engine->showMessage(_("QUEUE: SETUP INFERIOR"));
        QTimer::singleShot(0, this, SLOT(doSetupInferior()));
    }

    void queueRunEngine()
    {
        m_engine->setState(EngineRunRequested);
        m_engine->showMessage(_("QUEUE: RUN ENGINE"));
        QTimer::singleShot(0, this, SLOT(doRunEngine()));
    }

    void queueShutdownEngine()
    {
        m_engine->setState(EngineShutdownRequested);
        m_engine->showMessage(_("QUEUE: SHUTDOWN ENGINE"));
        QTimer::singleShot(0, this, SLOT(doShutdownEngine()));
    }

    void queueShutdownInferior()
    {
        m_engine->setState(InferiorShutdownRequested);
        m_engine->showMessage(_("QUEUE: SHUTDOWN INFERIOR"));
        QTimer::singleShot(0, this, SLOT(doShutdownInferior()));
    }

    void queueFinishDebugger()
    {
        QTC_ASSERT(state() == EngineShutdownOk
            || state() == EngineShutdownFailed, qDebug() << state());
        m_engine->setState(DebuggerFinished);
        m_engine->showMessage(_("QUEUE: FINISH DEBUGGER"));
        QTimer::singleShot(0, this, SLOT(doFinishDebugger()));
    }

    void raiseApplication()
    {
        QTC_ASSERT(runControl(), return);
        runControl()->bringApplicationToForeground(m_inferiorPid);
    }

    void scheduleResetLocation()
    {
        m_stackHandler.scheduleResetLocation();
        m_locationTimer.setSingleShot(true);
        m_locationTimer.start(80);
    }

    void resetLocation()
    {
        m_locationTimer.stop();
        m_locationMark.reset();
        m_stackHandler.resetLocation();
        m_disassemblerAgent.resetLocation();
    }

public:
    DebuggerState state() const { return m_state; }
    bool isMasterEngine() const { return m_engine->isMasterEngine(); }
    DebuggerRunControl *runControl() const
        { return m_masterEngine ? m_masterEngine->runControl() : m_runControl; }

    DebuggerEngine *m_engine; // Not owned.
    DebuggerEngine *m_masterEngine; // Not owned
    DebuggerRunControl *m_runControl;  // Not owned.

    DebuggerStartParameters m_startParameters;

    // The current state.
    DebuggerState m_state;

    // The state we had before something unexpected happend.
    DebuggerState m_lastGoodState;

    // The state we are aiming for.
    DebuggerState m_targetState;

    qint64 m_inferiorPid;

    ModulesHandler m_modulesHandler;
    RegisterHandler m_registerHandler;
    SourceFilesHandler m_sourceFilesHandler;
    StackHandler m_stackHandler;
    ThreadsHandler m_threadsHandler;
    WatchHandler m_watchHandler;
    QFutureInterface<void> m_progress;

    DisassemblerAgent m_disassemblerAgent;
    MemoryAgent m_memoryAgent;
    QScopedPointer<TextEditor::BaseTextMark> m_locationMark;
    QTimer m_locationTimer;

    bool m_isStateDebugging;
};


//////////////////////////////////////////////////////////////////////
//
// DebuggerEngine
//
//////////////////////////////////////////////////////////////////////

DebuggerEngine::DebuggerEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *parentEngine)
  : d(new DebuggerEnginePrivate(this, parentEngine, startParameters))
{
}

DebuggerEngine::~DebuggerEngine()
{
    disconnect();
    delete d;
}

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
        SN(InferiorSetupOk)
        SN(EngineRunRequested)
        SN(InferiorRunRequested)
        SN(InferiorRunOk)
        SN(InferiorRunFailed)
        SN(InferiorUnrunnable)
        SN(InferiorStopRequested)
        SN(InferiorStopOk)
        SN(InferiorStopFailed)
        SN(InferiorExitOk)
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

void DebuggerEngine::showStatusMessage(const QString &msg, int timeout) const
{
    showMessage(msg, StatusBar, timeout);
}

void DebuggerEngine::removeTooltip()
{
    watchHandler()->removeTooltip();
    hideDebuggerToolTip();
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
    return d->m_masterEngine
        ? d->m_masterEngine->modulesHandler()
        : &d->m_modulesHandler;
}

RegisterHandler *DebuggerEngine::registerHandler() const
{
    return d->m_masterEngine
        ? d->m_masterEngine->registerHandler()
        : &d->m_registerHandler;
}

StackHandler *DebuggerEngine::stackHandler() const
{
    //return d->m_masterEngine
    //    ? d->m_masterEngine->stackHandler()
    //    : &d->m_stackHandler;
    return &d->m_stackHandler;
}

ThreadsHandler *DebuggerEngine::threadsHandler() const
{
    return d->m_masterEngine
        ? d->m_masterEngine->threadsHandler()
        : &d->m_threadsHandler;
}

WatchHandler *DebuggerEngine::watchHandler() const
{
    return d->m_masterEngine
        ? d->m_masterEngine->watchHandler()
        : &d->m_watchHandler;
}

SourceFilesHandler *DebuggerEngine::sourceFilesHandler() const
{
    return d->m_masterEngine
        ? d->m_masterEngine->sourceFilesHandler()
        : &d->m_sourceFilesHandler;
}

QAbstractItemModel *DebuggerEngine::modulesModel() const
{
    QAbstractItemModel *model = modulesHandler()->model();
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("ModulesModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::registerModel() const
{
    QAbstractItemModel *model = registerHandler()->model();
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("RegisterModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::stackModel() const
{
    QAbstractItemModel *model = stackHandler()->model();
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("StackModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::threadsModel() const
{
    QAbstractItemModel *model = threadsHandler()->model();
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("ThreadsModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::localsModel() const
{
    QAbstractItemModel *model = watchHandler()->model(LocalsWatch);
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("LocalsModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::watchersModel() const
{
    QAbstractItemModel *model = watchHandler()->model(WatchersWatch);
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("WatchersModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::returnModel() const
{
    QAbstractItemModel *model = watchHandler()->model(ReturnWatch);
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("ReturnModel"));
    return model;
}

QAbstractItemModel *DebuggerEngine::sourceFilesModel() const
{
    QAbstractItemModel *model = sourceFilesHandler()->model();
    if (model->objectName().isEmpty()) // Make debugging easier.
        model->setObjectName(objectName() + QLatin1String("SourceFilesModel"));
    return model;
}

void DebuggerEngine::fetchMemory(MemoryAgent *, QObject *,
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
    if (d->m_masterEngine) {
        d->m_masterEngine->showMessage(msg, channel, timeout);
        return;
    }
    //if (msg.size() && msg.at(0).isUpper() && msg.at(1).isUpper())
    //    qDebug() << qPrintable(msg) << "IN STATE" << state();
    debuggerCore()->showMessage(msg, channel, timeout);
    if (d->m_runControl) {
        d->m_runControl->showMessage(msg, channel);
    } else {
        qWarning("Warning: %s (no active run control)", qPrintable(msg));
    }
}

void DebuggerEngine::startDebugger(DebuggerRunControl *runControl)
{
    QTC_ASSERT(runControl, notifyEngineSetupFailed(); return);
    QTC_ASSERT(!d->m_runControl, notifyEngineSetupFailed(); return);

    d->m_progress.setProgressRange(0, 1000);
    Core::FutureProgress *fp = Core::ICore::instance()->progressManager()
        ->addTask(d->m_progress.future(),
        tr("Launching"), _("Debugger.Launcher"));
    fp->setKeepOnFinish(Core::FutureProgress::DontKeepOnFinish);
    d->m_progress.reportStarted();

    d->m_runControl = runControl;

    d->m_inferiorPid = d->m_startParameters.attachPID > 0
        ? d->m_startParameters.attachPID : 0;

    if (!d->m_startParameters.environment.size())
        d->m_startParameters.environment = Utils::Environment();

    const unsigned engineCapabilities = debuggerCapabilities();
    debuggerCore()->action(OperateByInstruction)
        ->setEnabled(engineCapabilities & DisassemblerCapability);

    QTC_ASSERT(state() == DebuggerNotReady || state() == DebuggerFinished,
         qDebug() << state());
    d->m_lastGoodState = DebuggerNotReady;
    d->m_targetState = DebuggerNotReady;
    d->m_progress.setProgressValue(200);
    d->queueSetupEngine();
}

void DebuggerEngine::resetLocation()
{
    // Do it after some delay to avoid flicker.
    d->scheduleResetLocation();
}

void DebuggerEngine::gotoLocation(const Location &loc)
{
    if (debuggerCore()->boolSetting(OperateByInstruction) || !loc.hasDebugInfo()) {
        d->m_disassemblerAgent.setTryMixed(true);
        d->m_disassemblerAgent.setLocation(loc);
        return;
    }
    // CDB might hit on breakpoints while shutting down.
    //if (m_shuttingDown)
    //    return;

    d->resetLocation();

    const QString file = loc.fileName();
    const int line = loc.lineNumber();
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> editors = editorManager->editorsForFileName(file);
    IEditor *editor = 0;
    if (editors.isEmpty()) {
        editor = editorManager->openEditor(file, QString(),
            EditorManager::IgnoreNavigationHistory);
        if (editor) {
            editors.append(editor);
            editor->setProperty(Constants::OPENED_BY_DEBUGGER, true);
        }
    } else {
        editor = editors.back();
    }
    ITextEditor *texteditor = qobject_cast<ITextEditor *>(editor);
    if (texteditor)
        texteditor->gotoLine(line, 0);

    if (loc.needsMarker())
        d->m_locationMark.reset(new LocationMark(file, line));

    // FIXME: Breaks with split views.
    if (!d->m_memoryAgent.hasVisibleEditor() || loc.needsRaise())
        editorManager->activateEditor(editor);
    //qDebug() << "MEMORY: " << d->m_memoryAgent.hasVisibleEditor();
}

// Called from RunControl.
void DebuggerEngine::handleStartFailed()
{
    showMessage("HANDLE RUNCONTROL START FAILED");
    d->m_runControl = 0;
    d->m_progress.setProgressValue(900);
    d->m_progress.reportCanceled();
    d->m_progress.reportFinished();
}

// Called from RunControl.
void DebuggerEngine::handleFinished()
{
    showMessage("HANDLE RUNCONTROL FINISHED");
    d->m_runControl = 0;
    modulesHandler()->removeAll();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    watchHandler()->cleanup();
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
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
    return debuggerCore()->boolSetting(UseDebuggingHelpers);
}

QStringList DebuggerEngine::qtDumperLibraryLocations() const
{
    if (debuggerCore()->action(UseCustomDebuggingHelperLocation)->value().toBool()) {
        const QString customLocation =
            debuggerCore()->action(CustomDebuggingHelperLocation)->value().toString();
        const QString location =
            tr("%1 (explicitly set in the Debugger Options)").arg(customLocation);
        return QStringList(location);
    }
    return d->m_startParameters.dumperLibraryLocations;
}

void DebuggerEngine::showQtDumperLibraryWarning(const QString &details)
{
    debuggerCore()->showQtDumperLibraryWarning(details);
}

QString DebuggerEngine::qtDumperLibraryName() const
{
    if (debuggerCore()->action(UseCustomDebuggingHelperLocation)->value().toBool())
        return debuggerCore()->action(CustomDebuggingHelperLocation)->value().toString();
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
        // In is the engine's task to go into a proper "Shutdown"
        // state before calling notifyEngineSetupFailed
        return to == DebuggerFinished;
    case EngineSetupOk:
        return to == InferiorSetupRequested || to == EngineShutdownRequested;

    case InferiorSetupRequested:
        return to == InferiorSetupOk || to == InferiorSetupFailed;
    case InferiorSetupFailed:
        return to == EngineShutdownRequested;
    case InferiorSetupOk:
        return to == EngineRunRequested;

    case EngineRunRequested:
        return to == EngineRunFailed
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
            || to == InferiorStopOk // A spontaneous stop.
            || to == InferiorExitOk;

    case InferiorStopRequested:
        return to == InferiorStopOk || to == InferiorStopFailed;
    case InferiorStopOk:
        return to == InferiorRunRequested || to == InferiorShutdownRequested
            || to == InferiorStopOk || InferiorExitOk;
    case InferiorStopFailed:
        return to == EngineShutdownRequested;

    case InferiorExitOk:
        return to == InferiorShutdownOk;

    case InferiorUnrunnable:
        return to == InferiorShutdownRequested;
    case InferiorShutdownRequested:
        return to == InferiorShutdownOk || to == InferiorShutdownFailed;
    case InferiorShutdownOk:
        return to == EngineShutdownRequested;
    case InferiorShutdownFailed:
        return to == EngineShutdownRequested;

    case EngineShutdownRequested:
        return to == EngineShutdownOk || to == EngineShutdownFailed;
    case EngineShutdownOk:
        return to == DebuggerFinished;
    case EngineShutdownFailed:
        return to == DebuggerFinished;

    case DebuggerFinished:
        return to == EngineSetupRequested; // Happens on restart.
    }

    qDebug() << "UNKNOWN DEBUGGER STATE:" << from;
    return false;
}

void DebuggerEngine::setupSlaveEngine()
{
    QTC_ASSERT(state() == DebuggerNotReady, /**/);
    d->queueSetupEngine();
}

void DebuggerEnginePrivate::doSetupEngine()
{
    m_engine->showMessage(_("CALL: SETUP ENGINE"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << m_engine << state());
    m_engine->setupEngine();
}

void DebuggerEngine::notifyEngineSetupFailed()
{
    showMessage(_("NOTE: ENGINE SETUP FAILED"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupFailed);
    if (isMasterEngine() && runControl())
        runControl()->startFailed();
    setState(DebuggerFinished);
}

void DebuggerEngine::notifyEngineSetupOk()
{
    showMessage(_("NOTE: ENGINE SETUP OK"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupOk);
    showMessage(_("QUEUE: SETUP INFERIOR"));
    if (isMasterEngine())
        d->queueSetupInferior();
}

void DebuggerEngine::setupSlaveInferior()
{
    QTC_ASSERT(state() == EngineSetupOk, /**/);
    d->queueSetupInferior();
}

void DebuggerEnginePrivate::doSetupInferior()
{
    m_engine->showMessage(_("CALL: SETUP INFERIOR"));
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << m_engine << state());
    m_progress.setProgressValue(250);
    m_engine->setupInferior();
}

void DebuggerEngine::notifyInferiorSetupFailed()
{
    showMessage(_("NOTE: INFERIOR SETUP FAILED"));
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << this << state());
    setState(InferiorSetupFailed);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorSetupOk()
{
    showMessage(_("NOTE: INFERIOR SETUP OK"));
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << this << state());
    setState(InferiorSetupOk);
    if (isMasterEngine())
        d->queueRunEngine();
}

void DebuggerEngine::runSlaveEngine()
{
    QTC_ASSERT(isSlaveEngine(), return);
    QTC_ASSERT(state() == InferiorSetupOk, /**/);
    d->queueRunEngine();
}

void DebuggerEnginePrivate::doRunEngine()
{
    m_engine->showMessage(_("CALL: RUN ENGINE"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << m_engine << state());
    m_progress.setProgressValue(300);
    m_engine->runEngine();
}

void DebuggerEngine::notifyInferiorUnrunnable()
{
    showMessage(_("NOTE: INFERIOR UNRUNNABLE"));
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    setState(InferiorUnrunnable);
}

void DebuggerEngine::notifyEngineRunFailed()
{
    showMessage(_("NOTE: ENGINE RUN FAILED"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    d->m_progress.setProgressValue(900);
    d->m_progress.reportCanceled();
    d->m_progress.reportFinished();
    setState(EngineRunFailed);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyEngineRunAndInferiorRunOk()
{
    showMessage(_("NOTE: ENGINE RUN AND INFERIOR RUN OK"));
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    setState(InferiorRunOk);
}

void DebuggerEngine::notifyEngineRunAndInferiorStopOk()
{
    showMessage(_("NOTE: ENGINE RUN AND INFERIOR STOP OK"));
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorRunRequested()
{
    showMessage(_("NOTE: INFERIOR RUN REQUESTED"));
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << this << state());
    setState(InferiorRunRequested);
}

void DebuggerEngine::notifyInferiorRunOk()
{
    showMessage(_("NOTE: INFERIOR RUN OK"));
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << this << state());
    setState(InferiorRunOk);
}

void DebuggerEngine::notifyInferiorRunFailed()
{
    showMessage(_("NOTE: INFERIOR RUN FAILED"));
    QTC_ASSERT(state() == InferiorRunRequested, qDebug() << this << state());
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
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << this << state());
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorSpontaneousStop()
{
    showMessage(_("NOTE: INFERIOR SPONTANEOUES STOP"));
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << this << state());
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorStopFailed()
{
    showMessage(_("NOTE: INFERIOR STOP FAILED"));
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << this << state());
    setState(InferiorStopFailed);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEnginePrivate::doInterruptInferior()
{
    //QTC_ASSERT(isMasterEngine(), return);
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << m_engine << state());
    m_engine->setState(InferiorStopRequested);
    m_engine->showMessage(_("CALL: INTERRUPT INFERIOR"));
    m_engine->interruptInferior();
}

void DebuggerEnginePrivate::doShutdownInferior()
{
    //QTC_ASSERT(isMasterEngine(), return);
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << m_engine << state());
    resetLocation();
    m_targetState = DebuggerFinished;
    m_engine->showMessage(_("CALL: SHUTDOWN INFERIOR"));
    m_engine->shutdownInferior();
}

void DebuggerEngine::notifyInferiorShutdownOk()
{
    showMessage(_("INFERIOR SUCCESSFULLY SHUT DOWN"));
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << this << state());
    d->m_lastGoodState = DebuggerNotReady; // A "neutral" value.
    setState(InferiorShutdownOk);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorShutdownFailed()
{
    showMessage(_("INFERIOR SHUTDOWN FAILED"));
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << this << state());
    setState(InferiorShutdownFailed);
    if (isMasterEngine())
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

void DebuggerEngine::shutdownSlaveEngine()
{
    QTC_ASSERT(isAllowedTransition(state(),EngineShutdownRequested), /**/);
    setState(EngineShutdownRequested);
    shutdownEngine();
}

void DebuggerEnginePrivate::doShutdownEngine()
{
    QTC_ASSERT(isMasterEngine(), qDebug() << m_engine; return);
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << m_engine << state());
    m_targetState = DebuggerFinished;
    m_engine->showMessage(_("CALL: SHUTDOWN ENGINE"));
    m_engine->shutdownEngine();
}

void DebuggerEngine::notifyEngineShutdownOk()
{
    showMessage(_("NOTE: ENGINE SHUTDOWN OK"));
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << this << state());
    setState(EngineShutdownOk);
    d->queueFinishDebugger();
}

void DebuggerEngine::notifyEngineShutdownFailed()
{
    showMessage(_("NOTE: ENGINE SHUTDOWN FAILED"));
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << this << state());
    setState(EngineShutdownFailed);
    d->queueFinishDebugger();
}

void DebuggerEnginePrivate::doFinishDebugger()
{
    m_engine->showMessage(_("NOTE: FINISH DEBUGGER"));
    QTC_ASSERT(state() == DebuggerFinished, qDebug() << m_engine << state());
    resetLocation();
    if (isMasterEngine() && m_runControl)
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
            // The engine does not look overly ill right now, so attempt to
            // properly interrupt at least once. If that fails, we are on the
            // shutdown path due to d->m_targetState anyways.
            setState(InferiorStopRequested, true);
            showMessage(_("ATTEMPT TO INTERRUPT INFERIOR"));
            interruptInferior();
            break;
        case InferiorStopRequested:
        case InferiorStopOk:
            showMessage(_("FORWARDING STATE TO InferiorShutdownFailed"));
            setState(InferiorShutdownFailed, true);
            if (isMasterEngine())
                d->queueShutdownEngine();
            break;
        default:
            if (isMasterEngine())
                d->queueShutdownEngine();
            break;
    }
}

void DebuggerEngine::notifyEngineSpontaneousShutdown()
{
    showMessage(_("NOTE: ENGINE SPONTANEOUS SHUTDOWN"));
    setState(EngineShutdownOk, true);
    if (isMasterEngine())
        d->queueFinishDebugger();
}

void DebuggerEngine::notifyInferiorExited()
{
    showMessage(_("NOTE: INFERIOR EXITED"));
    d->resetLocation();
    setState(InferiorExitOk);
    setState(InferiorShutdownOk);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::slaveEngineStateChanged(DebuggerEngine *slaveEngine,
        DebuggerState state)
{
    Q_UNUSED(slaveEngine);
    Q_UNUSED(state);
}

void DebuggerEngine::setState(DebuggerState state, bool forced)
{
    if (isStateDebugging()) {
        qDebug() << "STATUS CHANGE: " << this
            << " FROM " << stateName(d->m_state) << " TO " << stateName(state)
            << isMasterEngine();
    }

    DebuggerState oldState = d->m_state;
    d->m_state = state;

    QString msg = _("State changed%5 from %1(%2) to %3(%4).")
     .arg(stateName(oldState)).arg(oldState).arg(stateName(state)).arg(state)
     .arg(forced ? " BY FORCE" : "");
    if (!forced && !isAllowedTransition(oldState, state))
        qDebug() << "*** UNEXPECTED STATE TRANSITION: " << this << msg;

    if (state == DebuggerFinished) {
        // Give up ownership on claimed breakpoints.
        BreakHandler *handler = breakHandler();
        foreach (BreakpointId id, handler->engineBreakpointIds(this))
            handler->notifyBreakpointReleased(id);
    }

    const bool running = d->m_state == InferiorRunOk;
    if (running)
        threadsHandler()->notifyRunning();

    showMessage(msg, LogDebug);
    updateViews();

    if (isMasterEngine())
        emit stateChanged(d->m_state);

    if (isSlaveEngine())
        masterEngine()->slaveEngineStateChanged(this, state);
}

void DebuggerEngine::updateViews()
{
    // The slave engines are not entitled to change the view. Their wishes
    // should be coordinated by their master engine.
    if (isMasterEngine())
        debuggerCore()->updateState(this);
}

bool DebuggerEngine::isSlaveEngine() const
{
    return d->m_masterEngine != 0;
}

bool DebuggerEngine::isMasterEngine() const
{
    return d->m_masterEngine == 0;
}

DebuggerEngine *DebuggerEngine::masterEngine() const
{
    return d->m_masterEngine;
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
    case InferiorSetupOk:
    case DebuggerNotReady:
    case EngineSetupRequested:
    case EngineSetupOk:
    case EngineSetupFailed:
    case EngineRunRequested:
    case EngineRunFailed:
    case InferiorSetupFailed:
    case InferiorStopFailed:
    case InferiorExitOk:
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

bool DebuggerEngine::isReverseDebugging() const
{
    return debuggerCore()->isReverseDebugging();
}

// Called by DebuggerRunControl.
void DebuggerEngine::quitDebugger()
{
    showMessage("QUIT DEBUGGER REQUESTED");
    d->m_targetState = DebuggerFinished;
    switch (state()) {
    case InferiorStopOk:
    case InferiorStopFailed:
        d->queueShutdownInferior();
        break;
    case InferiorRunOk:
        d->doInterruptInferior();
        break;
    default:
        // FIXME: We should disable the actions connected to that.
        notifyInferiorIll();
        break;
    }
}

void DebuggerEngine::requestInterruptInferior()
{
    d->doInterruptInferior();
}

void DebuggerEngine::progressPing()
{
    int progress = qMin(d->m_progress.progressValue() + 2, 800);
    d->m_progress.setProgressValue(progress);
}

DebuggerRunControl *DebuggerEngine::runControl() const
{
    return d->runControl();
}

void DebuggerEngine::setToolTipExpression
    (const QPoint &, TextEditor::ITextEditor *, int)
{
}

void DebuggerEngine::updateWatchData(const WatchData &, const WatchUpdateFlags &)
{
}

void DebuggerEngine::watchPoint(const QPoint &)
{
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

void DebuggerEngine::reloadRegisters()
{
}

void DebuggerEngine::reloadSourceFiles()
{
}

void DebuggerEngine::reloadFullStack()
{
}

void DebuggerEngine::addOptionPages(QList<Core::IOptionsPage*> *) const
{
}

unsigned DebuggerEngine::debuggerCapabilities() const
{
    return 0;
}

bool DebuggerEngine::isSynchronous() const
{
    return false;
}

QByteArray DebuggerEngine::qtNamespace() const
{
    return QByteArray();
}

void DebuggerEngine::createSnapshot()
{
}

void DebuggerEngine::updateAll()
{
}

#if 0
        // FIXME: Remove explicit use of BreakpointData
        if (!bp->engine && acceptsBreakpoint(id)) {
            QTC_ASSERT(state == BreakpointNew, /**/);
            // Take ownership of the breakpoint.
            bp->engine = this;
        }
#endif

void DebuggerEngine::attemptBreakpointSynchronization()
{
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    BreakHandler *handler = breakHandler();

    foreach (BreakpointId id, handler->unclaimedBreakpointIds()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(id))
            handler->setEngine(id, this);
    }

    bool done = true;
    foreach (BreakpointId id, handler->engineBreakpointIds(this)) {
        switch (handler->state(id)) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_ASSERT(false, /**/);
            continue;
        case BreakpointInsertRequested:
            done = false;
            insertBreakpoint(id);
            continue;
        case BreakpointChangeRequested:
            done = false;
            changeBreakpoint(id);
            continue;
        case BreakpointRemoveRequested:
            done = false;
            removeBreakpoint(id);
            continue;
        case BreakpointChangeProceeding:
        case BreakpointInsertProceeding:
        case BreakpointRemoveProceeding:
            done = false;
            //qDebug() << "BREAKPOINT " << id << " STILL IN PROGRESS, STATE"
            //    << handler->state(id);
            continue;
        case BreakpointInserted:
            //qDebug() << "BREAKPOINT " << id << " IS GOOD";
            continue;
        case BreakpointDead:
            // Should not only be visible inside BreakpointHandler.
            QTC_ASSERT(false, /**/);
            continue;
        }
        QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << id << state());
    }

    if (done)
        d->m_disassemblerAgent.updateBreakpointMarkers();
}

void DebuggerEngine::insertBreakpoint(BreakpointId id)
{
    BreakpointState state = breakHandler()->state(id);
    QTC_ASSERT(state == BreakpointInsertRequested, qDebug() << id << this << state);
    QTC_ASSERT(false, /**/);
}

void DebuggerEngine::removeBreakpoint(BreakpointId id)
{
    BreakpointState state = breakHandler()->state(id);
    QTC_ASSERT(state == BreakpointRemoveRequested, qDebug() << id << this << state);
    QTC_ASSERT(false, /**/);
}

void DebuggerEngine::changeBreakpoint(BreakpointId id)
{
    BreakpointState state = breakHandler()->state(id);
    QTC_ASSERT(state == BreakpointChangeRequested, qDebug() << id << this << state);
    QTC_ASSERT(false, /**/);
}

void DebuggerEngine::selectThread(int)
{
}

void DebuggerEngine::assignValueInDebugger(const WatchData *,
    const QString &, const QVariant &)
{
}

void DebuggerEngine::detachDebugger()
{
}

void DebuggerEngine::exitDebugger()
{
    QTC_ASSERT(d->m_state == InferiorStopOk, qDebug() << d->m_state);
    d->queueShutdownInferior();
}

void DebuggerEngine::executeStep()
{
}

void DebuggerEngine::executeStepOut()
{
}

void DebuggerEngine::executeNext()
{
}

void DebuggerEngine::executeStepI()
{
}

void DebuggerEngine::executeNextI()
{
}

void DebuggerEngine::executeReturn()
{
}

void DebuggerEngine::continueInferior()
{
}

void DebuggerEngine::interruptInferior()
{
}

void DebuggerEngine::executeRunToLine(const QString &, int)
{
}

void DebuggerEngine::executeRunToFunction(const QString &)
{
}

void DebuggerEngine::executeJumpToLine(const QString &, int)
{
}

void DebuggerEngine::executeDebuggerCommand(const QString &)
{
}

BreakHandler *DebuggerEngine::breakHandler() const
{
    return debuggerCore()->breakHandler();
}

bool DebuggerEngine::isDying() const
{
    return targetState() == DebuggerFinished;
}

QString DebuggerEngine::msgWatchpointTriggered(BreakpointId id,
    const int number, quint64 address)
{
    return id
        ? tr("Watchpoint %1 (%2) at 0x%3 triggered.")
            .arg(id).arg(number).arg(address, 0, 16)
        : tr("Internal watchpoint %1 at 0x%2 triggered.")
            .arg(number).arg(address, 0, 16);
}

QString DebuggerEngine::msgWatchpointTriggered(BreakpointId id,
    const int number, quint64 address, const QString &threadId)
{
    return id
        ? tr("Watchpoint %1 (%2) at 0x%3 in thread %4 triggered.")
            .arg(id).arg(number).arg(address, 0, 16).arg(threadId)
        : tr("Internal watchpoint %1 at 0x%2 in thread %3 triggered.")
            .arg(id).arg(number).arg(address, 0, 16).arg(threadId);
}

QString DebuggerEngine::msgBreakpointTriggered(BreakpointId id,
        const int number, const QString &threadId)
{
    return id
        ? tr("Stopped at breakpoint %1 (%2) in thread %3.")
            .arg(id).arg(number).arg(threadId)
        : tr("Stopped at internal breakpoint %1 in thread %2.")
            .arg(number).arg(threadId);
}

QString DebuggerEngine::msgStopped(const QString &reason)
{
    return reason.isEmpty() ? tr("Stopped.") : tr("Stopped: \"%1\"").arg(reason);
}

QString DebuggerEngine::msgStoppedBySignal(const QString &meaning,
    const QString &name)
{
    return tr("Stopped: %1 by signal %2.").arg(meaning, name);
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

void DebuggerEngine::showStoppedBySignalMessageBox(QString meaning, QString name)
{
    if (name.isEmpty())
        name = tr(" <Unknown> ", "name");
    if (meaning.isEmpty())
        meaning = tr(" <Unknown> ", "meaning");
    const QString msg = tr("<p>The inferior stopped because it received a "
                           "signal from the Operating System.<p>"
                           "<table><tr><td>Signal name : </td><td>%1</td></tr>"
                           "<tr><td>Signal meaning : </td><td>%2</td></tr></table>")
            .arg(name, meaning);
    showMessageBox(QMessageBox::Information, tr("Signal received"), msg);
}

void DebuggerEngine::showStoppedByExceptionMessageBox(const QString &description)
{
    const QString msg =
        tr("<p>The inferior stopped because it triggered an exception.<p>%1").
                         arg(description);
    showMessageBox(QMessageBox::Information, tr("Exception Triggered"), msg);
}

bool DebuggerEngine::isCppBreakpoint(const BreakpointParameters &p)
{
    // Qml is currently only file
    if (p.type != BreakpointByFileAndLine)
        return true;
    return !p.fileName.endsWith(QLatin1String(".qml"), Qt::CaseInsensitive)
            && !p.fileName.endsWith(QLatin1String(".js"), Qt::CaseInsensitive);
}

void DebuggerEngine::openMemoryView(quint64 address)
{
    d->m_memoryAgent.createBinEditor(address);
}

void DebuggerEngine::updateMemoryViews()
{
    d->m_memoryAgent.updateContents();
}

void DebuggerEngine::openDisassemblerView(const Location &location)
{
    DisassemblerAgent *agent = new DisassemblerAgent(this);
    agent->setTryMixed(true);
    agent->setLocation(location);
}

void DebuggerEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    Q_UNUSED(gdbServerPort);
    Q_UNUSED(qmlPort);
}

void DebuggerEngine::handleRemoteSetupFailed(const QString &message)
{
    Q_UNUSED(message);
}

bool DebuggerEngine::isStateDebugging() const
{
    return d->m_isStateDebugging;
}

void DebuggerEngine::setStateDebugging(bool on)
{
    d->m_isStateDebugging = on;
}

} // namespace Debugger

#include "debuggerengine.moc"
