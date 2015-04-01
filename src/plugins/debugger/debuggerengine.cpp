/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerengine.h"

#include "debuggerinternalconstants.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerruncontrol.h"
#include "debuggerstringutils.h"
#include "debuggerstartparameters.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "disassembleragent.h"
#include "logwindow.h"
#include "memoryagent.h"
#include "moduleshandler.h"
#include "gdb/gdbengine.h" // REMOVE
#include "registerhandler.h"
#include "sourcefileshandler.h"
#include "stackhandler.h"
#include "terminal.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include <debugger/shared/peutils.h>

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

#include <utils/fileinprojectfinder.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <qmljs/consolemanagerinterface.h>

#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include <QDir>

using namespace Core;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace TextEditor;

enum { debug = 0 };

#define SDEBUG(s) if (!debug) {} else qDebug() << s;
#define XSDEBUG(s) qDebug() << s

//#define WITH_BENCHMARK
#ifdef WITH_BENCHMARK
#include <valgrind/callgrind.h>
#endif

///////////////////////////////////////////////////////////////////////
//
// DebuggerStartParameters
//
///////////////////////////////////////////////////////////////////////

// VariableManager Prefix
const char PrefixDebugExecutable[]  = "DebuggedExecutable";

namespace Debugger {

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
            << " serverStartScript=" << sp.serverStartScript
            << " abi=" << sp.toolChainAbi.toString() << '\n';
    return str;
}

namespace Internal {

Location::Location(const StackFrame &frame, bool marker)
{
    init();
    m_fileName = frame.file;
    m_lineNumber = frame.line;
    m_needsMarker = marker;
    m_functionName = frame.function;
    m_hasDebugInfo = frame.isUsable();
    m_address = frame.address;
    m_from = frame.from;
}


//////////////////////////////////////////////////////////////////////
//
// DebuggerEnginePrivate
//
//////////////////////////////////////////////////////////////////////

// transitions:
//   None->Requested
//   Requested->Succeeded
//   Requested->Failed
//   Requested->Cancelled
enum RemoteSetupState { RemoteSetupNone, RemoteSetupRequested,
                        RemoteSetupSucceeded, RemoteSetupFailed,
                        RemoteSetupCancelled };

struct TypeInfo
{
    TypeInfo(uint s = 0) : size(s) {}
    uint size;
};

class DebuggerEnginePrivate : public QObject
{
    Q_OBJECT

public:
    DebuggerEnginePrivate(DebuggerEngine *engine, const DebuggerStartParameters &sp)
      : m_engine(engine),
        m_masterEngine(0),
        m_runControl(0),
        m_startParameters(sp),
        m_state(DebuggerNotReady),
        m_lastGoodState(DebuggerNotReady),
        m_targetState(DebuggerNotReady),
        m_remoteSetupState(RemoteSetupNone),
        m_inferiorPid(0),
        m_modulesHandler(engine),
        m_registerHandler(),
        m_sourceFilesHandler(),
        m_stackHandler(),
        m_threadsHandler(),
        m_watchHandler(engine),
        m_disassemblerAgent(engine),
        m_memoryAgent(engine),
        m_isStateDebugging(false)
    {
        connect(&m_locationTimer, &QTimer::timeout,
                this, &DebuggerEnginePrivate::resetLocation);
        connect(action(IntelFlavor), &Utils::SavedAction::valueChanged,
                this, &DebuggerEnginePrivate::reloadDisassembly);
        connect(action(OperateNativeMixed), &QAction::triggered,
                engine, &DebuggerEngine::reloadFullStack);

        Utils::globalMacroExpander()->registerFileVariables(PrefixDebugExecutable,
            tr("Debugged executable"),
            [this]() { return m_startParameters.executable; });
    }

public slots:
    void doSetupEngine();
    void doSetupInferior();
    void doRunEngine();
    void doShutdownEngine();
    void doShutdownInferior();
    void doInterruptInferior();
    void doFinishDebugger();

    void reloadDisassembly()
    {
        m_disassemblerAgent.reload();
    }

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
        resetLocation();
        if (isMasterEngine()) {
            m_engine->showMessage(_("QUEUE: FINISH DEBUGGER"));
            QTimer::singleShot(0, this, SLOT(doFinishDebugger()));
        }
    }

    void raiseApplication()
    {
        QTC_ASSERT(runControl(), return);
        runControl()->bringApplicationToForeground(m_inferiorPid);
    }

    void scheduleResetLocation()
    {
        m_stackHandler.scheduleResetLocation();
        m_watchHandler.scheduleResetLocation();
        m_threadsHandler.scheduleResetLocation();
        m_disassemblerAgent.scheduleResetLocation();
        m_locationTimer.setSingleShot(true);
        m_locationTimer.start(80);
    }

    void resetLocation()
    {
        m_locationTimer.stop();
        m_locationMark.reset();
        m_stackHandler.resetLocation();
        m_watchHandler.resetLocation();
        m_threadsHandler.resetLocation();
        m_disassemblerAgent.resetLocation();
        DebuggerToolTipManager::resetLocation();
    }

public:
    DebuggerState state() const { return m_state; }
    RemoteSetupState remoteSetupState() const { return m_remoteSetupState; }
    bool isMasterEngine() const { return m_engine->isMasterEngine(); }
    DebuggerRunControl *runControl() const
        { return m_masterEngine ? m_masterEngine->runControl() : m_runControl; }
    void setRemoteSetupState(RemoteSetupState state);

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

    // State of RemoteSetup signal/slots.
    RemoteSetupState m_remoteSetupState;

    Terminal m_terminal;
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
    QScopedPointer<TextMark> m_locationMark;
    QTimer m_locationTimer;

    bool m_isStateDebugging;

    Utils::FileInProjectFinder m_fileFinder;
    QHash<QByteArray, TypeInfo> m_typeInfoCache;
    QByteArray m_qtNamespace;
};


//////////////////////////////////////////////////////////////////////
//
// DebuggerEngine
//
//////////////////////////////////////////////////////////////////////

DebuggerEngine::DebuggerEngine(const DebuggerStartParameters &startParameters)
  : d(new DebuggerEnginePrivate(this, startParameters))
{}

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

void DebuggerEngine::setTargetState(DebuggerState state)
{
    d->m_targetState = state;
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
    return d->m_masterEngine
        ? d->m_masterEngine->stackHandler()
        : &d->m_stackHandler;
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
   return modulesHandler()->model();
}

QAbstractItemModel *DebuggerEngine::registerModel() const
{
    return registerHandler()->model();
}

QAbstractItemModel *DebuggerEngine::stackModel() const
{
    return stackHandler()->model();
}

QAbstractItemModel *DebuggerEngine::threadsModel() const
{
    return threadsHandler()->model();
}

QAbstractItemModel *DebuggerEngine::watchModel() const
{
    return watchHandler()->model();
}

QAbstractItemModel *DebuggerEngine::sourceFilesModel() const
{
    return sourceFilesHandler()->model();
}

void DebuggerEngine::fetchMemory(MemoryAgent *, QObject *,
        quint64 addr, quint64 length)
{
    Q_UNUSED(addr);
    Q_UNUSED(length);
}

void DebuggerEngine::changeMemory(MemoryAgent *, QObject *,
        quint64 addr, const QByteArray &data)
{
    Q_UNUSED(addr);
    Q_UNUSED(data);
}

void DebuggerEngine::setRegisterValue(const QByteArray &name, const QString &value)
{
    Q_UNUSED(name);
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
    QmlJS::ConsoleManagerInterface *consoleManager = QmlJS::ConsoleManagerInterface::instance();
    if (channel == ConsoleOutput && consoleManager)
        consoleManager->printToConsolePane(QmlJS::ConsoleItem::UndefinedType, msg);

    Internal::showMessage(msg, channel, timeout);
    if (d->m_runControl) {
        switch (channel) {
            case AppOutput:
                d->m_runControl->appendMessage(msg, Utils::StdOutFormatSameLine);
                break;
            case AppError:
                d->m_runControl->appendMessage(msg, Utils::StdErrFormatSameLine);
                break;
            case AppStuff:
                d->m_runControl->appendMessage(msg, Utils::DebugFormat);
                break;
        }
    } else {
        qWarning("Warning: %s (no active run control)", qPrintable(msg));
    }
}

void DebuggerEngine::startDebugger(DebuggerRunControl *runControl)
{
    QTC_ASSERT(runControl, notifyEngineSetupFailed(); return);
    QTC_ASSERT(!d->m_runControl, notifyEngineSetupFailed(); return);

    d->m_progress.setProgressRange(0, 1000);
    FutureProgress *fp = ProgressManager::addTask(d->m_progress.future(),
        tr("Launching Debugger"), "Debugger.Launcher");
    connect(fp, &FutureProgress::canceled, this, &DebuggerEngine::quitDebugger);
    fp->setKeepOnFinish(FutureProgress::HideOnFinish);
    d->m_progress.reportStarted();

    d->m_runControl = runControl;

    d->m_inferiorPid = d->m_startParameters.attachPID > 0
        ? d->m_startParameters.attachPID : 0;
    if (d->m_inferiorPid)
        d->m_runControl->setApplicationProcessHandle(ProcessHandle(d->m_inferiorPid));

    if (!d->m_startParameters.environment.size())
        d->m_startParameters.environment = Utils::Environment();

    action(OperateByInstruction)->setEnabled(hasCapability(DisassemblerCapability));

    QTC_ASSERT(state() == DebuggerNotReady || state() == DebuggerFinished,
         qDebug() << state());
    d->m_lastGoodState = DebuggerNotReady;
    d->m_targetState = DebuggerNotReady;
    d->m_progress.setProgressValue(200);

    d->m_terminal.setup();
    if (d->m_terminal.isUsable()) {
        connect(&d->m_terminal, &Terminal::stdOutReady, [this, runControl](const QString &msg) {
            runControl->appendMessage(msg, Utils::StdOutFormatSameLine);
        });
        connect(&d->m_terminal, &Terminal::stdErrReady, [this, runControl](const QString &msg) {
            runControl->appendMessage(msg, Utils::StdErrFormatSameLine);
        });
        connect(&d->m_terminal, &Terminal::error, [this, runControl](const QString &msg) {
            runControl->appendMessage(msg, Utils::ErrorMessageFormat);
        });
    }

    d->queueSetupEngine();
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
            && ((hasCapability(OperateByInstructionCapability) && boolSetting(OperateByInstruction))
                || !loc.hasDebugInfo()) )
    {
        d->m_disassemblerAgent.setLocation(loc);
        return;
    }

    if (loc.fileName().isEmpty()) {
        showMessage(QLatin1String("CANNOT GO TO THIS LOCATION"));
        return;
    }
    const QString file = QDir::cleanPath(loc.fileName());
    const int line = loc.lineNumber();
    bool newEditor = false;
    IEditor *editor = EditorManager::openEditor(file, Id(),
                                                EditorManager::IgnoreNavigationHistory, &newEditor);
    QTC_ASSERT(editor, return); // Unreadable file?

    editor->gotoLine(line, 0, !boolSetting(StationaryEditorWhileStepping));

    if (newEditor)
        editor->document()->setProperty(Constants::OPENED_BY_DEBUGGER, true);

    if (loc.needsMarker()) {
        d->m_locationMark.reset(new TextMark(file, line));
        d->m_locationMark->setIcon(Internal::locationMarkIcon());
        d->m_locationMark->setPriority(TextMark::HighPriority);
    }

    //qDebug() << "MEMORY: " << d->m_memoryAgent.hasVisibleEditor();
}

// Called from RunControl.
void DebuggerEngine::handleStartFailed()
{
    showMessage(QLatin1String("HANDLE RUNCONTROL START FAILED"));
    d->m_runControl = 0;
    d->m_progress.setProgressValue(900);
    d->m_progress.reportCanceled();
    d->m_progress.reportFinished();
}

// Called from RunControl.
void DebuggerEngine::handleFinished()
{
    showMessage(QLatin1String("HANDLE RUNCONTROL FINISHED"));
    d->m_runControl = 0;
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    modulesHandler()->removeAll();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    watchHandler()->cleanup();
}

const DebuggerStartParameters &DebuggerEngine::startParameters() const
{
    return d->m_startParameters;
}

DebuggerStartParameters &DebuggerEngine::startParameters()
{
    return d->m_startParameters;
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
            || to == InferiorStopOk // A spontaneous stop.
            || to == InferiorExitOk;

    case InferiorStopRequested:
        return to == InferiorStopOk || to == InferiorStopFailed;
    case InferiorStopOk:
        return to == InferiorRunRequested || to == InferiorShutdownRequested
            || to == InferiorStopOk || to == InferiorExitOk;
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
    QTC_CHECK(state() == DebuggerNotReady);
    d->queueSetupEngine();
}

void DebuggerEnginePrivate::doSetupEngine()
{
    m_engine->showMessage(_("CALL: SETUP ENGINE"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << m_engine << state());
    m_engine->validateExecutable(&m_startParameters);
    m_engine->setupEngine();
}

void DebuggerEngine::notifyEngineSetupFailed()
{
    showMessage(_("NOTE: ENGINE SETUP FAILED"));
    QTC_ASSERT(d->remoteSetupState() == RemoteSetupNone
               || d->remoteSetupState() == RemoteSetupRequested
               || d->remoteSetupState() == RemoteSetupSucceeded,
               qDebug() << this << "remoteSetupState" << d->remoteSetupState());
    if (d->remoteSetupState() == RemoteSetupRequested)
        d->setRemoteSetupState(RemoteSetupCancelled);

    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupFailed);
    if (isMasterEngine() && runControl())
        runControl()->startFailed();
    setState(DebuggerFinished);
}

void DebuggerEngine::notifyEngineSetupOk()
{
    showMessage(_("NOTE: ENGINE SETUP OK"));
    QTC_ASSERT(d->remoteSetupState() == RemoteSetupNone
               || d->remoteSetupState() == RemoteSetupSucceeded,
               qDebug() << this << "remoteSetupState" << d->remoteSetupState());

    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupOk);
    showMessage(_("QUEUE: SETUP INFERIOR"));
    if (isMasterEngine())
        d->queueSetupInferior();
}

void DebuggerEngine::setupSlaveInferior()
{
    QTC_CHECK(state() == EngineSetupOk);
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
    showStatusMessage(tr("Setup failed."));
    setState(InferiorSetupFailed);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorSetupOk()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_START_INSTRUMENTATION;
#endif
    aboutToNotifyInferiorSetupOk();
    showMessage(_("NOTE: INFERIOR SETUP OK"));
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << this << state());
    setState(InferiorSetupOk);
    if (isMasterEngine())
        d->queueRunEngine();
}

void DebuggerEngine::runSlaveEngine()
{
    QTC_ASSERT(isSlaveEngine(), return);
    QTC_CHECK(state() == InferiorSetupOk);
    d->queueRunEngine();
}

void DebuggerEnginePrivate::doRunEngine()
{
    m_engine->showMessage(_("CALL: RUN ENGINE"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << m_engine << state());
    m_progress.setProgressValue(300);
    m_engine->runEngine();
}

void DebuggerEngine::notifyEngineRunOkAndInferiorUnrunnable()
{
    showMessage(_("NOTE: INFERIOR UNRUNNABLE"));
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    showStatusMessage(tr("Loading finished."));
    setState(InferiorUnrunnable);
}

void DebuggerEngine::notifyEngineRunFailed()
{
    showMessage(_("NOTE: ENGINE RUN FAILED"));
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    d->m_progress.setProgressValue(900);
    d->m_progress.reportCanceled();
    d->m_progress.reportFinished();
    showStatusMessage(tr("Run failed."));
    setState(EngineRunFailed);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyEngineRequestRemoteSetup()
{
    showMessage(_("NOTE: REQUEST REMOTE SETUP"));
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    QTC_ASSERT(d->remoteSetupState() == RemoteSetupNone, qDebug() << this
               << "remoteSetupState" << d->remoteSetupState());

    d->setRemoteSetupState(RemoteSetupRequested);
    emit requestRemoteSetup();
}

void DebuggerEngine::notifyEngineRemoteServerRunning(const QByteArray &, int /*pid*/)
{
    showMessage(_("NOTE: REMOTE SERVER RUNNING IN MULTIMODE"));
}

void DebuggerEngine::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    QTC_ASSERT(state() == EngineSetupRequested
               || state() == EngineSetupFailed
               || state() == DebuggerFinished, qDebug() << this << state());

    QTC_ASSERT(d->remoteSetupState() == RemoteSetupRequested
               || d->remoteSetupState() == RemoteSetupCancelled,
               qDebug() << this << "remoteSetupState" << d->remoteSetupState());

    if (result.success) {
        showMessage(_("NOTE: REMOTE SETUP DONE: GDB SERVER PORT: %1  QML PORT %2")
                    .arg(result.gdbServerPort).arg(result.qmlServerPort));

        if (d->remoteSetupState() != RemoteSetupCancelled)
            d->setRemoteSetupState(RemoteSetupSucceeded);

        if (result.gdbServerPort != InvalidPid) {
            QString &rc = d->m_startParameters.remoteChannel;
            const int sepIndex = rc.lastIndexOf(QLatin1Char(':'));
            if (sepIndex != -1) {
                rc.replace(sepIndex + 1, rc.count() - sepIndex - 1,
                           QString::number(result.gdbServerPort));
            }
        } else if (result.inferiorPid != InvalidPid && startParameters().startMode == AttachExternal) {
            // e.g. iOS Simulator
            startParameters().attachPID = result.inferiorPid;
        }

        if (result.qmlServerPort != InvalidPort) {
            d->m_startParameters.qmlServerPort = result.qmlServerPort;
            d->m_startParameters.processArgs.replace(_("%qml_port%"), QString::number(result.qmlServerPort));
        }

    } else {
        d->setRemoteSetupState(RemoteSetupFailed);
        showMessage(_("NOTE: REMOTE SETUP FAILED: ") + result.reason);
    }
}

void DebuggerEngine::notifyEngineRunAndInferiorRunOk()
{
    showMessage(_("NOTE: ENGINE RUN AND INFERIOR RUN OK"));
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    showStatusMessage(tr("Running."));
    setState(InferiorRunOk);
}

void DebuggerEngine::notifyEngineRunAndInferiorStopOk()
{
    showMessage(_("NOTE: ENGINE RUN AND INFERIOR STOP OK"));
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << this << state());
    showStatusMessage(tr("Stopped."));
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorRunRequested()
{
    showMessage(_("NOTE: INFERIOR RUN REQUESTED"));
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << this << state());
    showStatusMessage(tr("Run requested..."));
    setState(InferiorRunRequested);
}

void DebuggerEngine::notifyInferiorRunOk()
{
    if (state() == InferiorRunOk) {
        showMessage(_("NOTE: INFERIOR RUN OK - REPEATED."));
        return;
    }
    showMessage(_("NOTE: INFERIOR RUN OK"));
    showStatusMessage(tr("Running."));
    // Transition from StopRequested can happen in remotegdbadapter.
    QTC_ASSERT(state() == InferiorRunRequested
        || state() == InferiorStopOk
        || state() == InferiorStopRequested, qDebug() << this << state());
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
        if (state() == InferiorStopOk || state() == InferiorStopFailed)
            d->queueShutdownInferior();
        showMessage(_("NOTE: ... IGNORING STOP MESSAGE"));
        return;
    }
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << this << state());
    showStatusMessage(tr("Stopped."));
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorSpontaneousStop()
{
    showMessage(_("NOTE: INFERIOR SPONTANEOUS STOP"));
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << this << state());
    showStatusMessage(tr("Stopped."));
    setState(InferiorStopOk);
    if (boolSetting(RaiseOnInterrupt))
        ICore::raiseWindow(Internal::mainWindow());
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
    m_engine->showStatusMessage(tr("Attempting to interrupt."));
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
    QTC_CHECK(isAllowedTransition(state(),EngineShutdownRequested));
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
    if (isMasterEngine() && m_runControl)
        m_runControl->debuggingFinished();
}

void DebuggerEnginePrivate::setRemoteSetupState(RemoteSetupState state)
{
    bool allowedTransition = false;
    if (m_remoteSetupState == RemoteSetupNone) {
        if (state == RemoteSetupRequested)
            allowedTransition = true;
    }
    if (m_remoteSetupState == RemoteSetupRequested) {
        if (state == RemoteSetupCancelled
                || state == RemoteSetupSucceeded
                || state == RemoteSetupFailed)
            allowedTransition = true;
    }


    if (!allowedTransition)
        qDebug() << "*** UNEXPECTED REMOTE SETUP TRANSITION from"
                 << m_remoteSetupState << "to" << state;
    m_remoteSetupState = state;
}

void DebuggerEngine::notifyEngineIll()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
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
            notifyInferiorStopFailed();
            break;
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
#ifdef WITH_BENCHMARK
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
    showMessage(_("NOTE: ENGINE SPONTANEOUS SHUTDOWN"));
    setState(EngineShutdownOk, true);
    if (isMasterEngine())
        d->queueFinishDebugger();
}

void DebuggerEngine::notifyInferiorExited()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
    showMessage(_("NOTE: INFERIOR EXITED"));
    d->resetLocation();
    setState(InferiorExitOk);
    setState(InferiorShutdownOk);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyDebuggerProcessFinished(int exitCode,
    QProcess::ExitStatus exitStatus, const QString &backendName)
{
    showMessage(_("%1 PROCESS FINISHED, status %2, exit code %3")
                .arg(backendName).arg(exitStatus).arg(exitCode));

    switch (state()) {
    case DebuggerFinished:
        // Nothing to do.
        break;
    case EngineShutdownRequested:
        notifyEngineShutdownOk();
        break;
    case InferiorRunOk:
        // This could either be a real gdb/lldb crash or a quickly exited inferior
        // in the terminal adapter. In this case the stub proc will die soon,
        // too, so there's no need to act here.
        showMessage(_("The %1 process exited somewhat unexpectedly.").arg(backendName));
        notifyEngineSpontaneousShutdown();
        break;
    default: {
        notifyEngineIll(); // Initiate shutdown sequence
        const QString msg = exitStatus == QProcess::CrashExit ?
                tr("The %1 process terminated.") :
                tr("The %2 process terminated unexpectedly (exit code %1).").arg(exitCode);
        AsynchronousMessageBox::critical(tr("Unexpected %1 Exit").arg(backendName),
                                         msg.arg(backendName));
        break;
    }
    }
}

void DebuggerEngine::slaveEngineStateChanged(DebuggerEngine *slaveEngine,
        DebuggerState state)
{
    Q_UNUSED(slaveEngine);
    Q_UNUSED(state);
}

static inline QString msgStateChanged(DebuggerState oldState, DebuggerState newState,
                                      bool forced, bool master)
{
    QString result;
    QTextStream str(&result);
    str << "State changed";
    if (forced)
        str << " BY FORCE";
    str << " from " << DebuggerEngine::stateName(oldState) << '(' << oldState
        << ") to " << DebuggerEngine::stateName(newState) << '(' << newState << ')';
    if (master)
        str << " [master]";
    return result;
}

void DebuggerEngine::setState(DebuggerState state, bool forced)
{
    const QString msg = msgStateChanged(d->m_state, state, forced, isMasterEngine());
    if (isStateDebugging())
        qDebug("%s", qPrintable(msg));

    DebuggerState oldState = d->m_state;
    d->m_state = state;

    if (!forced && !isAllowedTransition(oldState, state))
        qDebug() << "*** UNEXPECTED STATE TRANSITION: " << this << msg;

    if (state == EngineRunRequested) {
        DebuggerToolTipManager::registerEngine(this);
    }

    if (state == DebuggerFinished) {
        // Give up ownership on claimed breakpoints.
        foreach (Breakpoint bp, breakHandler()->engineBreakpoints(this))
            bp.notifyBreakpointReleased();
        DebuggerToolTipManager::deregisterEngine(this);
    }

    showMessage(msg, LogDebug);
    updateViews();

    emit stateChanged(d->m_state);

    if (isSlaveEngine())
        masterEngine()->slaveEngineStateChanged(this, state);
}

void DebuggerEngine::updateViews()
{
    // The slave engines are not entitled to change the view. Their wishes
    // should be coordinated by their master engine.
    if (isMasterEngine())
        Internal::updateState(this);
}

bool DebuggerEngine::isSlaveEngine() const
{
    return d->m_masterEngine != 0;
}

bool DebuggerEngine::isMasterEngine() const
{
    return d->m_masterEngine == 0;
}

void DebuggerEngine::setMasterEngine(DebuggerEngine *masterEngine)
{
    d->m_masterEngine = masterEngine;
}

DebuggerEngine *DebuggerEngine::masterEngine() const
{
    return d->m_masterEngine;
}

QString DebuggerEngine::toFileInProject(const QUrl &fileUrl)
{
    // make sure file finder is properly initialized
    const DebuggerStartParameters &sp = startParameters();
    d->m_fileFinder.setProjectDirectory(sp.projectSourceDirectory);
    d->m_fileFinder.setProjectFiles(sp.projectSourceFiles);
    d->m_fileFinder.setSysroot(sp.sysRoot);

    return d->m_fileFinder.findFile(fileUrl);
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
    if (d->m_inferiorPid == pid)
        return;
    d->m_inferiorPid = pid;
    if (pid) {
        showMessage(tr("Taking notice of pid %1").arg(pid));
        if (d->m_startParameters.startMode == StartInternal
            || d->m_startParameters.startMode == StartExternal
            || d->m_startParameters.startMode == AttachExternal)
        QTimer::singleShot(0, d, SLOT(raiseApplication()));
    }
}

qint64 DebuggerEngine::inferiorPid() const
{
    return d->m_inferiorPid;
}

bool DebuggerEngine::isReverseDebugging() const
{
    return Internal::isReverseDebugging();
}

// Called by DebuggerRunControl.
void DebuggerEngine::quitDebugger()
{
    showMessage(_("QUIT DEBUGGER REQUESTED IN STATE %1").arg(state()));
    d->m_targetState = DebuggerFinished;
    switch (state()) {
    case InferiorStopOk:
    case InferiorStopFailed:
        d->queueShutdownInferior();
        break;
    case InferiorRunOk:
        d->doInterruptInferior();
        break;
    case EngineSetupRequested:
        notifyEngineSetupFailed();
        break;
    case EngineSetupOk:
        setState(InferiorSetupRequested);
        notifyInferiorSetupFailed();
        break;
    case EngineRunRequested:
        notifyEngineRunFailed();
        break;
    case EngineShutdownRequested:
        break;
    case EngineRunFailed:
    case DebuggerFinished:
    case InferiorExitOk:
    case InferiorShutdownOk:
        break;
    case InferiorSetupRequested:
        notifyInferiorSetupFailed();
        break;
    default:
        // FIXME: We should disable the actions connected to that.
        notifyInferiorIll();
        break;
    }
}

void DebuggerEngine::abortDebugger()
{
    // Overridden in e.g. GdbEngine.
    quitDebugger();
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

Terminal *DebuggerEngine::terminal() const
{
    return &d->m_terminal;
}

bool DebuggerEngine::setToolTipExpression(const DebuggerToolTipContext &)
{
    return false;
}

void DebuggerEngine::watchDataSelected(const QByteArray &)
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

bool DebuggerEngine::isSynchronous() const
{
    return false;
}

QByteArray DebuggerEngine::qtNamespace() const
{
    return d->m_qtNamespace;
}

void DebuggerEngine::setQtNamespace(const QByteArray &ns)
{
    d->m_qtNamespace = ns;
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
            QTC_CHECK(state == BreakpointNew);
            // Take ownership of the breakpoint.
            bp->engine = this;
        }
#endif

void DebuggerEngine::attemptBreakpointSynchronization()
{
    showMessage(_("ATTEMPT BREAKPOINT SYNCHRONIZATION"));
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    BreakHandler *handler = breakHandler();

    foreach (Breakpoint bp, handler->unclaimedBreakpoints()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(bp)) {
            showMessage(_("TAKING OWNERSHIP OF BREAKPOINT %1 IN STATE %2")
                .arg(bp.id().toString()).arg(bp.state()));
            bp.setEngine(this);
        } else {
            showMessage(_("BREAKPOINT %1 IN STATE %2 IS NOT ACCEPTABLE")
                .arg(bp.id().toString()).arg(bp.state()));
        }
    }

    bool done = true;
    foreach (Breakpoint bp, handler->engineBreakpoints(this)) {
        switch (bp.state()) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_CHECK(false);
            continue;
        case BreakpointInsertRequested:
            done = false;
            insertBreakpoint(bp);
            continue;
        case BreakpointChangeRequested:
            done = false;
            changeBreakpoint(bp);
            continue;
        case BreakpointRemoveRequested:
            done = false;
            removeBreakpoint(bp);
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
            QTC_CHECK(false);
            continue;
        }
        QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << bp.id() << state());
    }

    if (done) {
        showMessage(_("BREAKPOINTS ARE SYNCHRONIZED"));
        d->m_disassemblerAgent.updateBreakpointMarkers();
    } else {
        showMessage(_("BREAKPOINTS ARE NOT FULLY SYNCHRONIZED"));
    }
}

bool DebuggerEngine::acceptsBreakpoint(Breakpoint bp) const
{
    Q_UNUSED(bp);
    return false;
}

void DebuggerEngine::insertBreakpoint(Breakpoint bp)
{
    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointInsertRequested,
               qDebug() << bp.id() << this << state);
    QTC_CHECK(false);
}

void DebuggerEngine::removeBreakpoint(Breakpoint bp)
{
    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointRemoveRequested,
               qDebug() << bp.id() << this << state);
    QTC_CHECK(false);
}

void DebuggerEngine::changeBreakpoint(Breakpoint bp)
{
    BreakpointState state = bp.state();
    QTC_ASSERT(state == BreakpointChangeRequested,
               qDebug() << bp.id() << this << state);
    QTC_CHECK(false);
}

void DebuggerEngine::assignValueInDebugger(WatchItem *,
    const QString &, const QVariant &)
{
}

void DebuggerEngine::detachDebugger()
{
}

void DebuggerEngine::exitDebugger()
{
    QTC_ASSERT(d->m_state == InferiorStopOk || d->m_state == InferiorUnrunnable
        || d->m_state == InferiorRunOk, qDebug() << d->m_state);
    quitDebugger();
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

void DebuggerEngine::executeRunToLine(const ContextData &)
{
}

void DebuggerEngine::executeRunToFunction(const QString &)
{
}

void DebuggerEngine::executeJumpToLine(const ContextData &)
{
}

void DebuggerEngine::executeDebuggerCommand(const QString &, DebuggerLanguages)
{
    showStatusMessage(tr("This debugger cannot handle user input."));
}

BreakHandler *DebuggerEngine::breakHandler() const
{
    return Internal::breakHandler();
}

bool DebuggerEngine::isDying() const
{
    return targetState() == DebuggerFinished;
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

void DebuggerEngine::showStoppedBySignalMessageBox(QString meaning, QString name)
{
    if (name.isEmpty())
        name = QLatin1Char(' ') + tr("<Unknown>", "name") + QLatin1Char(' ');
    if (meaning.isEmpty())
        meaning = QLatin1Char(' ') + tr("<Unknown>", "meaning") + QLatin1Char(' ');
    const QString msg = tr("<p>The inferior stopped because it received a "
                           "signal from the operating system.<p>"
                           "<table><tr><td>Signal name : </td><td>%1</td></tr>"
                           "<tr><td>Signal meaning : </td><td>%2</td></tr></table>")
            .arg(name, meaning);
    AsynchronousMessageBox::information(tr("Signal Received"), msg);
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
    d->m_memoryAgent.createBinEditor(data);
}

void DebuggerEngine::updateMemoryViews()
{
    d->m_memoryAgent.updateContents();
}

void DebuggerEngine::openDisassemblerView(const Location &location)
{
    DisassemblerAgent *agent = new DisassemblerAgent(this);
    agent->setLocation(location);
}

bool DebuggerEngine::isStateDebugging() const
{
    return d->m_isStateDebugging;
}

void DebuggerEngine::setStateDebugging(bool on)
{
    d->m_isStateDebugging = on;
}

void DebuggerEngine::validateExecutable(DebuggerStartParameters *sp)
{
    if (sp->skipExecutableValidation)
        return;
    if (sp->languages == QmlLanguage)
        return;
    QString binary = sp->executable;
    if (binary.isEmpty())
        return;

    const bool warnOnRelease = boolSetting(WarnOnReleaseBuilds);
    QString detailedWarning;
    switch (sp->toolChainAbi.binaryFormat()) {
    case Abi::PEFormat: {
        if (!warnOnRelease || (sp->masterEngineType != CdbEngineType))
            return;
        if (!binary.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
            binary.append(QLatin1String(".exe"));
        QString errorMessage;
        QStringList rc;
        if (getPDBFiles(binary, &rc, &errorMessage) && !rc.isEmpty())
            return;
        if (!errorMessage.isEmpty()) {
            detailedWarning.append(QLatin1Char('\n'));
            detailedWarning.append(errorMessage);
        }
        break;
    }
    case Abi::ElfFormat: {

        Utils::ElfReader reader(binary);
        Utils::ElfData elfData = reader.readHeaders();
        QString error = reader.errorString();

        Internal::showMessage(_("EXAMINING ") + binary, LogDebug);
        QByteArray msg = "ELF SECTIONS: ";

        static QList<QByteArray> interesting;
        if (interesting.isEmpty()) {
            interesting.append(".debug_info");
            interesting.append(".debug_abbrev");
            interesting.append(".debug_line");
            interesting.append(".debug_str");
            interesting.append(".debug_loc");
            interesting.append(".debug_range");
            interesting.append(".gdb_index");
            interesting.append(".note.gnu.build-id");
            interesting.append(".gnu.hash");
            interesting.append(".gnu_debuglink");
        }

        QSet<QByteArray> seen;
        foreach (const Utils::ElfSectionHeader &header, elfData.sectionHeaders) {
            msg.append(header.name);
            msg.append(' ');
            if (interesting.contains(header.name))
                seen.insert(header.name);
        }
        Internal::showMessage(_(msg), LogDebug);

        if (!error.isEmpty()) {
            Internal::showMessage(_("ERROR WHILE READING ELF SECTIONS: ") + error,
                                        LogDebug);
            return;
        }

        if (elfData.sectionHeaders.isEmpty()) {
            Internal::showMessage(_("NO SECTION HEADERS FOUND. IS THIS AN EXECUTABLE?"),
                                        LogDebug);
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
                            sp->sourcePathMap.insert(string.left(index) + exp.cap(1), itExp->second);
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
            const QString found = seen.contains(name) ? tr("Found.") : tr("Not found.");
            detailedWarning.append(QLatin1Char('\n') + tr("Section %1: %2").arg(_(name)).arg(found));
        }
        break;
    }
    default:
        return;
    }
    if (warnOnRelease) {
        AsynchronousMessageBox::information(tr("Warning"),
                       tr("This does not seem to be a \"Debug\" build.\n"
                          "Setting breakpoints by file name and line number may fail.")
                       + QLatin1Char('\n') + detailedWarning);
    }
}

void DebuggerEngine::updateLocalsView(const GdbMi &all)
{
    WatchHandler *handler = watchHandler();

    const bool partial = all["partial"].toInt();

    const GdbMi typeInfo = all["typeinfo"];
    if (typeInfo.type() == GdbMi::List) {
        foreach (const GdbMi &s, typeInfo.children()) {
            const GdbMi name = s["name"];
            const GdbMi size = s["size"];
            if (name.isValid() && size.isValid())
                d->m_typeInfoCache.insert(QByteArray::fromHex(name.data()),
                                       TypeInfo(size.data().toUInt()));
        }
    }

    QSet<QByteArray> toDelete;
    if (!partial) {
        foreach (WatchItem *item, handler->model()->treeLevelItems<WatchItem *>(2))
            toDelete.insert(item->iname);
    }

    GdbMi data = all["data"];
    foreach (const GdbMi &child, data.children()) {
        WatchItem *item = new WatchItem(child);
        const TypeInfo ti = d->m_typeInfoCache.value(item->type);
        if (ti.size)
            item->size = ti.size;

        handler->insertItem(item);
        toDelete.remove(item->iname);
    }

    GdbMi ns = all["qtnamespace"];
    if (ns.isValid()) {
        setQtNamespace(ns.data());
        showMessage(_("FOUND NAMESPACED QT: " + ns.data()));
    }

    handler->purgeOutdatedItems(toDelete);

    static int count = 0;
    showMessage(_("<Rebuild Watchmodel %1 @ %2 >")
                .arg(++count).arg(LogWindow::logTimeStamp()), LogMiscInput);
    showStatusMessage(GdbEngine::tr("Finished retrieving data"), 400); // FIXME: String

    DebuggerToolTipManager::updateEngine(this);

    if (!partial)
        emit stackFrameCompleted();
}

} // namespace Internal
} // namespace Debugger

#include "debuggerengine.moc"
