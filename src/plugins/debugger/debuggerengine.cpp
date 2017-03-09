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
#include "debuggericons.h"
#include "debuggerruncontrol.h"
#include "debuggerstartparameters.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "disassembleragent.h"
#include "logwindow.h"
#include "memoryagent.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "sourcefileshandler.h"
#include "sourceutils.h"
#include "stackhandler.h"
#include "terminal.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "debugger/shared/peutils.h"
#include "console/console.h"

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
#include <utils/processhandle.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include <QDir>

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

// VariableManager Prefix
const char PrefixDebugExecutable[]  = "DebuggedExecutable";

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
            << " useTerminal=" << sp.useTerminal
            << " remoteChannel=" << sp.remoteChannel
            << " serverStartScript=" << sp.serverStartScript
            << " abi=" << sp.toolChainAbi.toString() << '\n';
    return str;
}

namespace Internal {

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


LocationMark::LocationMark(DebuggerEngine *engine, const QString &file, int line)
    : TextMark(file, line, Constants::TEXT_MARK_CATEGORY_LOCATION), m_engine(engine)
{
    setIcon(Icons::LOCATION.icon());
    setPriority(TextMark::HighPriority);
}

bool LocationMark::isDraggable() const
{
    return m_engine->hasCapability(JumpToLineCapability);
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

// transitions:
//   None->Requested
//   Requested->Succeeded
//   Requested->Failed
//   Requested->Cancelled
enum RemoteSetupState { RemoteSetupNone, RemoteSetupRequested,
                        RemoteSetupSucceeded, RemoteSetupFailed,
                        RemoteSetupCancelled };

class DebuggerEnginePrivate : public QObject
{
    Q_OBJECT

public:
    DebuggerEnginePrivate(DebuggerEngine *engine, const DebuggerRunParameters &sp)
      : m_engine(engine),
        m_runParameters(sp),
        m_modulesHandler(engine),
        m_registerHandler(engine),
        m_sourceFilesHandler(engine),
        m_stackHandler(engine),
        m_threadsHandler(engine),
        m_watchHandler(engine),
        m_disassemblerAgent(engine),
        m_isStateDebugging(false)
    {
        connect(&m_locationTimer, &QTimer::timeout,
                this, &DebuggerEnginePrivate::resetLocation);
        connect(action(IntelFlavor), &Utils::SavedAction::valueChanged,
                this, &DebuggerEnginePrivate::reloadDisassembly);
        connect(action(ShowMixed), &Utils::SavedAction::valueChanged,
                this, &DebuggerEnginePrivate::reloadDisassembly);
        connect(action(ShowOpcode), &Utils::SavedAction::valueChanged,
                this, &DebuggerEnginePrivate::reloadDisassembly);
        connect(action(ShowOffset), &Utils::SavedAction::valueChanged,
                this, &DebuggerEnginePrivate::reloadDisassembly);

        Utils::globalMacroExpander()->registerFileVariables(PrefixDebugExecutable,
            tr("Debugged executable"),
            [this] { return m_runParameters.inferior.executable; });
    }

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
        m_engine->showMessage("QUEUE: SETUP ENGINE");
        QTimer::singleShot(0, this, &DebuggerEnginePrivate::doSetupEngine);
    }

    void queueSetupInferior()
    {
        m_engine->setState(InferiorSetupRequested);
        m_engine->showMessage("QUEUE: SETUP INFERIOR");
        QTimer::singleShot(0, this, &DebuggerEnginePrivate::doSetupInferior);
    }

    void queueRunEngine()
    {
        m_engine->setState(EngineRunRequested);
        m_engine->showMessage("QUEUE: RUN ENGINE");
        QTimer::singleShot(0, this, &DebuggerEnginePrivate::doRunEngine);
    }

    void queueShutdownEngine()
    {
        m_engine->setState(EngineShutdownRequested);
        m_engine->showMessage("QUEUE: SHUTDOWN ENGINE");
        QTimer::singleShot(0, this, &DebuggerEnginePrivate::doShutdownEngine);
    }

    void queueShutdownInferior()
    {
        m_engine->setState(InferiorShutdownRequested);
        m_engine->showMessage("QUEUE: SHUTDOWN INFERIOR");
        QTimer::singleShot(0, this, &DebuggerEnginePrivate::doShutdownInferior);
    }

    void queueFinishDebugger()
    {
        QTC_ASSERT(state() == EngineShutdownOk
            || state() == EngineShutdownFailed, qDebug() << state());
        m_engine->setState(DebuggerFinished);
        resetLocation();
        if (isMasterEngine()) {
            m_engine->showMessage("QUEUE: FINISH DEBUGGER");
            QTimer::singleShot(0, this, &DebuggerEnginePrivate::doFinishDebugger);
        }
    }

    void raiseApplication()
    {
        QTC_ASSERT(runControl(), return);
        runControl()->bringApplicationToForeground();
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
        m_lookupRequests.clear();
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

    DebuggerEngine *m_engine = nullptr; // Not owned.
    DebuggerEngine *m_masterEngine = nullptr; // Not owned
    DebuggerRunControl *m_runControl = nullptr;  // Not owned.

    DebuggerRunParameters m_runParameters;

    // The current state.
    DebuggerState m_state = DebuggerNotReady;

    // The state we had before something unexpected happend.
    DebuggerState m_lastGoodState = DebuggerNotReady;

    // The state we are aiming for.
    DebuggerState m_targetState = DebuggerNotReady;

    // State of RemoteSetup signal/slots.
    RemoteSetupState m_remoteSetupState = RemoteSetupNone;

    Terminal m_terminal;
    ProcessHandle m_inferiorPid;

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

    bool m_isStateDebugging = false;

    Utils::FileInProjectFinder m_fileFinder;
    QString m_qtNamespace;

    // Safety net to avoid infinite lookups.
    QSet<QString> m_lookupRequests; // FIXME: Integrate properly.
    QPointer<QWidget> m_alertBox;
};


//////////////////////////////////////////////////////////////////////
//
// DebuggerEngine
//
//////////////////////////////////////////////////////////////////////

DebuggerEngine::DebuggerEngine(const DebuggerRunParameters &startParameters)
  : d(new DebuggerEnginePrivate(this, startParameters))
{}

DebuggerEngine::~DebuggerEngine()
{
    disconnect();
    delete d;
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
        SN(InferiorShutdownRequested)
        SN(InferiorShutdownOk)
        SN(InferiorShutdownFailed)
        SN(EngineShutdownRequested)
        SN(EngineShutdownOk)
        SN(EngineShutdownFailed)
        SN(DebuggerFinished)
    }
    return QLatin1String("<unknown>");
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

void DebuggerEngine::doUpdateLocals(const UpdateParameters &)
{
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

void DebuggerEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    if (d->m_masterEngine) {
        d->m_masterEngine->showMessage(msg, channel, timeout);
        return;
    }
    //if (msg.size() && msg.at(0).isUpper() && msg.at(1).isUpper())
    //    qDebug() << qPrintable(msg) << "IN STATE" << state();
    if (channel == ConsoleOutput)
        debuggerConsole()->printItem(ConsoleItem::DefaultType, msg);

    Internal::showMessage(msg, channel, timeout);
    switch (channel) {
    case AppOutput:
    case AppError:
    case AppStuff:
        if (d->m_runControl)
            d->m_runControl->handleApplicationOutput(msg, channel);
        else
            qWarning("Warning: %s (no active run control)", qPrintable(msg));
        break;
    default:
        break;
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

    d->m_inferiorPid = d->m_runParameters.attachPID.isValid()
        ? d->m_runParameters.attachPID : ProcessHandle();
    if (d->m_inferiorPid.isValid())
        d->m_runControl->setApplicationProcessHandle(d->m_inferiorPid);

    if (isNativeMixedActive())
        d->m_runParameters.inferior.environment.set("QV4_FORCE_INTERPRETER", "1");

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
        showMessage("CANNOT GO TO THIS LOCATION");
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

    if (loc.needsMarker())
        d->m_locationMark.reset(new LocationMark(this, file, line));
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
    d->m_progress.setProgressValue(1000);
    d->m_progress.reportFinished();
    modulesHandler()->removeAll();
    stackHandler()->removeAll();
    threadsHandler()->removeAll();
    watchHandler()->cleanup();
}

const DebuggerRunParameters &DebuggerEngine::runParameters() const
{
    return d->m_runParameters;
}

DebuggerRunParameters &DebuggerEngine::runParameters()
{
    return d->m_runParameters;
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
            || to == InferiorStopOk       // A spontaneous stop.
            || to == InferiorShutdownOk;  // A spontaneous exit.

    case InferiorStopRequested:
        return to == InferiorStopOk || to == InferiorStopFailed;
    case InferiorStopOk:
        return to == InferiorRunRequested || to == InferiorShutdownRequested
            || to == InferiorStopOk || to == InferiorShutdownOk;
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
    m_engine->showMessage("CALL: SETUP ENGINE");
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << m_engine << state());
    m_engine->validateExecutable(&m_runParameters);
    m_engine->setupEngine();
}

void DebuggerEngine::notifyEngineSetupFailed()
{
    showMessage("NOTE: ENGINE SETUP FAILED");
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
    showMessage("NOTE: ENGINE SETUP OK");
    QTC_ASSERT(d->remoteSetupState() == RemoteSetupNone
               || d->remoteSetupState() == RemoteSetupSucceeded,
               qDebug() << this << "remoteSetupState" << d->remoteSetupState());

    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    setState(EngineSetupOk);
    showMessage("QUEUE: SETUP INFERIOR");
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
    m_engine->showMessage("CALL: SETUP INFERIOR");
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << m_engine << state());
    m_progress.setProgressValue(250);
    m_engine->setupInferior();
}

void DebuggerEngine::notifyInferiorSetupFailed()
{
    showMessage("NOTE: INFERIOR SETUP FAILED");
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
    showMessage("NOTE: INFERIOR SETUP OK");
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
    m_engine->showMessage("CALL: RUN ENGINE");
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << m_engine << state());
    m_progress.setProgressValue(300);
    m_engine->runEngine();
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
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyEngineRequestRemoteSetup()
{
    showMessage("NOTE: REQUEST REMOTE SETUP");
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << this << state());
    QTC_ASSERT(d->remoteSetupState() == RemoteSetupNone, qDebug() << this
               << "remoteSetupState" << d->remoteSetupState());

    d->setRemoteSetupState(RemoteSetupRequested);
    emit requestRemoteSetup();
}

void DebuggerEngine::notifyEngineRemoteServerRunning(const QString &, int /*pid*/)
{
    showMessage("NOTE: REMOTE SERVER RUNNING IN MULTIMODE");
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
        showMessage(QString("NOTE: REMOTE SETUP DONE: GDB SERVER PORT: %1  QML PORT %2")
                    .arg(result.gdbServerPort.number()).arg(result.qmlServerPort.number()));

        if (d->remoteSetupState() != RemoteSetupCancelled)
            d->setRemoteSetupState(RemoteSetupSucceeded);

        if (result.gdbServerPort.isValid()) {
            QString &rc = d->m_runParameters.remoteChannel;
            const int sepIndex = rc.lastIndexOf(':');
            if (sepIndex != -1) {
                rc.replace(sepIndex + 1, rc.count() - sepIndex - 1,
                           QString::number(result.gdbServerPort.number()));
            }
        } else if (result.inferiorPid != InvalidPid && runParameters().startMode == AttachExternal) {
            // e.g. iOS Simulator
            runParameters().attachPID = ProcessHandle(result.inferiorPid);
        }

        if (result.qmlServerPort.isValid()) {
            d->m_runParameters.qmlServer.port = result.qmlServerPort;
            d->m_runParameters.inferior.commandLineArguments.replace("%qml_port%",
                            QString::number(result.qmlServerPort.number()));
        }

    } else {
        d->setRemoteSetupState(RemoteSetupFailed);
        showMessage("NOTE: REMOTE SETUP FAILED: " + result.reason);
    }
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
        d->queueShutdownInferior();
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
            d->queueShutdownInferior();
        showMessage("NOTE: ... IGNORING STOP MESSAGE");
        return;
    }
    QTC_ASSERT(state() == InferiorStopRequested, qDebug() << this << state());
    showStatusMessage(tr("Stopped."));
    setState(InferiorStopOk);
}

void DebuggerEngine::notifyInferiorSpontaneousStop()
{
    showMessage("NOTE: INFERIOR SPONTANEOUS STOP");
    QTC_ASSERT(state() == InferiorRunOk, qDebug() << this << state());
    showStatusMessage(tr("Stopped."));
    setState(InferiorStopOk);
    if (boolSetting(RaiseOnInterrupt))
        ICore::raiseWindow(Internal::mainWindow());
}

void DebuggerEngine::notifyInferiorStopFailed()
{
    showMessage("NOTE: INFERIOR STOP FAILED");
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
    m_engine->showMessage("CALL: INTERRUPT INFERIOR");
    m_engine->showStatusMessage(tr("Attempting to interrupt."));
    m_engine->interruptInferior();
}

void DebuggerEnginePrivate::doShutdownInferior()
{
    //QTC_ASSERT(isMasterEngine(), return);
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << m_engine << state());
    resetLocation();
    m_targetState = DebuggerFinished;
    m_engine->showMessage("CALL: SHUTDOWN INFERIOR");
    m_engine->shutdownInferior();
}

void DebuggerEngine::notifyInferiorShutdownOk()
{
    showMessage("INFERIOR SUCCESSFULLY SHUT DOWN");
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << this << state());
    d->m_lastGoodState = DebuggerNotReady; // A "neutral" value.
    setState(InferiorShutdownOk);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorShutdownFailed()
{
    showMessage("INFERIOR SHUTDOWN FAILED");
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << this << state());
    setState(InferiorShutdownFailed);
    if (isMasterEngine())
        d->queueShutdownEngine();
}

void DebuggerEngine::notifyInferiorIll()
{
    showMessage("NOTE: INFERIOR ILL");
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
    m_engine->showMessage("CALL: SHUTDOWN ENGINE");
    m_engine->shutdownEngine();
}

void DebuggerEngine::notifyEngineShutdownOk()
{
    showMessage("NOTE: ENGINE SHUTDOWN OK");
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << this << state());
    setState(EngineShutdownOk);
    d->queueFinishDebugger();
}

void DebuggerEngine::notifyEngineShutdownFailed()
{
    showMessage("NOTE: ENGINE SHUTDOWN FAILED");
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << this << state());
    setState(EngineShutdownFailed);
    d->queueFinishDebugger();
}

void DebuggerEnginePrivate::doFinishDebugger()
{
    m_engine->showMessage("NOTE: FINISH DEBUGGER");
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
    showMessage("NOTE: ENGINE ILL ******");
    d->m_targetState = DebuggerFinished;
    d->m_lastGoodState = d->m_state;
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
            showMessage("FORWARDING STATE TO InferiorShutdownFailed");
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
    showMessage("NOTE: ENGINE SPONTANEOUS SHUTDOWN");
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
    showMessage("NOTE: INFERIOR EXITED");
    d->resetLocation();
    setState(InferiorShutdownOk);
    if (isMasterEngine())
        d->queueShutdownEngine();
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
        notifyEngineShutdownOk();
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
        if (isMasterEngine())
            notifyEngineIll();
        else
            masterEngine()->notifyInferiorIll();
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
        d->m_memoryAgents.handleDebuggerFinished();
        prepareForRestart();
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

void DebuggerEngine::removeBreakpointMarker(const Breakpoint &bp)
{
    d->m_disassemblerAgent.removeBreakpointMarker(bp);
}

QString DebuggerEngine::expand(const QString &string) const
{
    return d->m_runParameters.macroExpander->expand(string);
}

QString DebuggerEngine::nativeStartupCommands() const
{
    return expand(QStringList({stringSetting(GdbStartupCommands),
                               runParameters().additionalStartupCommands}).join('\n'));
}

void DebuggerEngine::updateBreakpointMarker(const Breakpoint &bp)
{
    d->m_disassemblerAgent.updateBreakpointMarker(bp);
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

void DebuggerEngine::notifyInferiorPid(const ProcessHandle &pid)
{
    if (d->m_inferiorPid == pid)
        return;
    d->m_inferiorPid = pid;
    if (pid.isValid()) {
        runControl()->setApplicationProcessHandle(pid);
        showMessage(tr("Taking notice of pid %1").arg(pid.pid()));
        if (d->m_runParameters.startMode == StartInternal
            || d->m_runParameters.startMode == StartExternal
            || d->m_runParameters.startMode == AttachExternal)
        QTimer::singleShot(0, d, &DebuggerEnginePrivate::raiseApplication);
    }
}

qint64 DebuggerEngine::inferiorPid() const
{
    return d->m_inferiorPid.pid();
}

bool DebuggerEngine::isReverseDebugging() const
{
    return Internal::isReverseDebugging();
}

// Called by DebuggerRunControl.
void DebuggerEngine::quitDebugger()
{
    showMessage(QString("QUIT DEBUGGER REQUESTED IN STATE %1").arg(state()));
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
            showStatusMessage(tr("Could not find a widget."));
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

bool DebuggerEngine::isSynchronous() const
{
    return false;
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
    showMessage("ATTEMPT BREAKPOINT SYNCHRONIZATION");
    if (!stateAcceptsBreakpointChanges()) {
        showMessage("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE");
        return;
    }

    BreakHandler *handler = breakHandler();

    foreach (Breakpoint bp, handler->unclaimedBreakpoints()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(bp)) {
            showMessage(QString("TAKING OWNERSHIP OF BREAKPOINT %1 IN STATE %2")
                .arg(bp.id().toString()).arg(bp.state()));
            bp.setEngine(this);
        } else {
            showMessage(QString("BREAKPOINT %1 IN STATE %2 IS NOT ACCEPTABLE")
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
            // Can happen temporarily during Breakpoint destruction.
            // BreakpointItem::deleteThis() intentionally lets the event loop run,
            // during which an attemptBreakpointSynchronization() might kick in.
            continue;
        }
    }

    if (done)
        showMessage("BREAKPOINTS ARE SYNCHRONIZED");
    else
        showMessage("BREAKPOINTS ARE NOT FULLY SYNCHRONIZED");
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

bool DebuggerEngine::isStateDebugging() const
{
    return d->m_isStateDebugging;
}

void DebuggerEngine::setStateDebugging(bool on)
{
    d->m_isStateDebugging = on;
}

void DebuggerEngine::validateExecutable(DebuggerRunParameters *sp)
{
    if (sp->skipExecutableValidation)
        return;
    if (sp->languages == QmlLanguage)
        return;

    QString symbolFile = sp->symbolFile;
    if (symbolFile.isEmpty())
        symbolFile = sp->inferior.executable;
    if (symbolFile.isEmpty())
        return;

    const bool warnOnRelease = boolSetting(WarnOnReleaseBuilds);
    bool warnOnInappropriateDebugger = false;
    QString detailedWarning;
    switch (sp->toolChainAbi.binaryFormat()) {
    case Abi::PEFormat: {
        if (sp->masterEngineType != CdbEngineType) {
            detailedWarning = tr(
                        "The inferior is in the Portable Executable format.\n"
                        "Selecting CDB as debugger would improve the debugging "
                        "experience for this binary format.");
            return;
        } else if (warnOnRelease) {
            if (!symbolFile.endsWith(".exe", Qt::CaseInsensitive))
                symbolFile.append(".exe");
            QString errorMessage;
            QStringList rc;
            if (getPDBFiles(symbolFile, &rc, &errorMessage) && !rc.isEmpty())
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
        if (sp->masterEngineType == CdbEngineType) {
            detailedWarning = tr(
                        "The inferior is in the ELF format.\n"
                        "Selecting GDB or LLDB as debugger would improve the debugging "
                        "experience for this binary format.");
            return;
        }

        Utils::ElfReader reader(symbolFile);
        Utils::ElfData elfData = reader.readHeaders();
        QString error = reader.errorString();

        Internal::showMessage("EXAMINING " + symbolFile, LogDebug);
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
        foreach (const Utils::ElfSectionHeader &header, elfData.sectionHeaders) {
            msg.append(header.name);
            msg.append(' ');
            if (interesting.contains(header.name))
                seen.insert(header.name);
        }
        Internal::showMessage(QString::fromUtf8(msg), LogDebug);

        if (!error.isEmpty()) {
            Internal::showMessage("ERROR WHILE READING ELF SECTIONS: " + error, LogDebug);
            return;
        }

        if (elfData.sectionHeaders.isEmpty()) {
            Internal::showMessage("NO SECTION HEADERS FOUND. IS THIS AN EXECUTABLE?", LogDebug);
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
            detailedWarning.append('\n' + tr("Section %1: %2").arg(QString::fromUtf8(name)).arg(found));
        }
        break;
    }
    default:
        return;
    }
    if (warnOnInappropriateDebugger) {
        AsynchronousMessageBox::information(tr("Warning"),
                tr("The selected debugger may be inappropiate for the inferior.\n"
                   "Examining symbols and setting breakpoints by file name and line number "
                   "may fail.\n")
               + '\n' + detailedWarning);
    } else if (warnOnRelease) {
        AsynchronousMessageBox::information(tr("Warning"),
               tr("This does not seem to be a \"Debug\" build.\n"
                  "Setting breakpoints by file name and line number may fail.")
               + '\n' + detailedWarning);
    }
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
    showStatusMessage(tr("Finished retrieving data"), 400);

    DebuggerToolTipManager::updateEngine(this);

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

void DebuggerEngine::checkState(DebuggerState state, const char *file, int line)
{
    const DebuggerState current = d->m_state;
    if (current == state)
        return;

    QString msg = QString("UNEXPECTED STATE: %1  WANTED: %2 IN %3:%4")
                .arg(current).arg(state).arg(QLatin1String(file)).arg(line);

    showMessage(msg, LogError);
    qDebug("%s", qPrintable(msg));
}

bool DebuggerEngine::isNativeMixedEnabled() const
{
    return runParameters().nativeMixedEnabled && (runParameters().languages & QmlLanguage);
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

} // namespace Internal
} // namespace Debugger

#include "debuggerengine.moc"
