#include "qmlcppengine.h"
#include "qmlengine.h"
#include "debuggeruiswitcher.h"
#include "debuggerplugin.h"

#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QTimer>

namespace Debugger {

const int ConnectionWaitTimeMs = 5000;

namespace Internal {
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &);

DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &sp)
{
    return new QmlCppEngine(sp);
}
} // namespace Internal

struct QmlCppEnginePrivate {
    QmlCppEnginePrivate();

    QmlEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
    bool m_shutdownOk;
    bool m_shutdownDeferred;
    bool m_shutdownDone;
    bool m_isInitialStartup;
};

QmlCppEnginePrivate::QmlCppEnginePrivate() :
    m_qmlEngine(0),
    m_cppEngine(0),
    m_activeEngine(0),
    m_shutdownOk(true)
    , m_shutdownDeferred(false)
    , m_shutdownDone(false)
    , m_isInitialStartup(true)
{
}

QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp)
    : DebuggerEngine(sp), d(new QmlCppEnginePrivate)
{
    d->m_qmlEngine = qobject_cast<QmlEngine*>(Internal::createQmlEngine(sp));
    d->m_qmlEngine->setAttachToRunningExternalApp(true);

    if (startParameters().cppEngineType == GdbEngineType) {
        d->m_cppEngine = Internal::createGdbEngine(sp);
    } else {
        d->m_cppEngine = Internal::createCdbEngine(sp);
    }

    d->m_cppEngine->setRunInWrapperEngine(true);
    d->m_qmlEngine->setRunInWrapperEngine(true);

    d->m_activeEngine = d->m_cppEngine;
    connect(d->m_cppEngine, SIGNAL(stateChanged(DebuggerState)), SLOT(masterEngineStateChanged(DebuggerState)));
    connect(d->m_qmlEngine, SIGNAL(stateChanged(DebuggerState)), SLOT(slaveEngineStateChanged(DebuggerState)));

    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(editorChanged(Core::IEditor*)));
}

QmlCppEngine::~QmlCppEngine()
{
    delete d->m_qmlEngine;
    delete d->m_cppEngine;
    d->m_qmlEngine = 0;
    d->m_cppEngine = 0;
}

void QmlCppEngine::editorChanged(Core::IEditor *editor)
{
    // change the engine based on editor, but only if we're not currently in stopped state.
    if (state() != InferiorRunOk || !editor)
        return;

    if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        setActiveEngine(QmlLanguage);
    } else {
        setActiveEngine(CppLanguage);
    }
}

void QmlCppEngine::setActiveEngine(DebuggerLanguage language)
{
    DebuggerEngine *previousEngine = d->m_activeEngine;
    bool updateEngine = false;
    QString engineName;

    if (language == CppLanguage) {
        engineName = QLatin1String("C++");
        d->m_activeEngine = d->m_cppEngine;
        // don't update cpp engine - at least gdb will stop temporarily,
        // which is not nice when you're just switching files.
    } else if (language == QmlLanguage) {
        engineName = QLatin1String("QML");
        d->m_activeEngine = d->m_qmlEngine;
        updateEngine = true;
    }
    if (previousEngine != d->m_activeEngine) {
        showStatusMessage(tr("%1 debugger activated").arg(engineName));
        plugin()->displayDebugger(d->m_activeEngine, updateEngine);
    }
}

void QmlCppEngine::setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, int cursorPos)
{
    d->m_activeEngine->setToolTipExpression(mousePos, editor, cursorPos);
}

void QmlCppEngine::updateWatchData(const Internal::WatchData &data, const Internal::WatchUpdateFlags &flags)
{
    d->m_activeEngine->updateWatchData(data, flags);
}

void QmlCppEngine::watchPoint(const QPoint &point)
{
    d->m_cppEngine->watchPoint(point);
}

void QmlCppEngine::fetchMemory(Internal::MemoryViewAgent *mva, QObject *obj,
        quint64 addr, quint64 length)
{
    d->m_cppEngine->fetchMemory(mva, obj, addr, length);
}

void QmlCppEngine::fetchDisassembler(Internal::DisassemblerViewAgent *dva)
{
    d->m_cppEngine->fetchDisassembler(dva);
}

void QmlCppEngine::activateFrame(int index)
{
    d->m_cppEngine->activateFrame(index);
}

void QmlCppEngine::reloadModules()
{
    d->m_cppEngine->reloadModules();
}

void QmlCppEngine::examineModules()
{
    d->m_cppEngine->examineModules();
}

void QmlCppEngine::loadSymbols(const QString &moduleName)
{
    d->m_cppEngine->loadSymbols(moduleName);
}

void QmlCppEngine::loadAllSymbols()
{
    d->m_cppEngine->loadAllSymbols();
}

void QmlCppEngine::requestModuleSymbols(const QString &moduleName)
{
    d->m_cppEngine->requestModuleSymbols(moduleName);
}

void QmlCppEngine::reloadRegisters()
{
    d->m_cppEngine->reloadRegisters();
}

void QmlCppEngine::reloadSourceFiles()
{
    d->m_cppEngine->reloadSourceFiles();
}

void QmlCppEngine::reloadFullStack()
{
    d->m_cppEngine->reloadFullStack();
}

void QmlCppEngine::setRegisterValue(int regnr, const QString &value)
{
    d->m_cppEngine->setRegisterValue(regnr, value);
}

unsigned QmlCppEngine::debuggerCapabilities() const
{
    // ### this could also be an OR of both engines' capabilities
    return d->m_cppEngine->debuggerCapabilities();
}

bool QmlCppEngine::isSynchronous() const
{
    return d->m_activeEngine->isSynchronous();
}

QByteArray QmlCppEngine::qtNamespace() const
{
    return d->m_cppEngine->qtNamespace();
}

void QmlCppEngine::createSnapshot()
{
    d->m_cppEngine->createSnapshot();
}

void QmlCppEngine::updateAll()
{
    d->m_activeEngine->updateAll();
}

void QmlCppEngine::attemptBreakpointSynchronization()
{
    d->m_cppEngine->attemptBreakpointSynchronization();
    static_cast<DebuggerEngine*>(d->m_qmlEngine)->attemptBreakpointSynchronization();
}

void QmlCppEngine::selectThread(int index)
{
    d->m_cppEngine->selectThread(index);
}

void QmlCppEngine::assignValueInDebugger(const Internal::WatchData *w, const QString &expr, const QVariant &value)
{
    d->m_activeEngine->assignValueInDebugger(w, expr, value);
}

QAbstractItemModel *QmlCppEngine::commandModel() const
{
    return d->m_activeEngine->commandModel();
}

QAbstractItemModel *QmlCppEngine::modulesModel() const
{
    return d->m_cppEngine->modulesModel();
}

QAbstractItemModel *QmlCppEngine::breakModel() const
{
    return d->m_activeEngine->breakModel();
}

QAbstractItemModel *QmlCppEngine::registerModel() const
{
    return d->m_cppEngine->registerModel();
}

QAbstractItemModel *QmlCppEngine::stackModel() const
{
    return d->m_activeEngine->stackModel();
}

QAbstractItemModel *QmlCppEngine::threadsModel() const
{
    return d->m_cppEngine->threadsModel();
}

QAbstractItemModel *QmlCppEngine::localsModel() const
{
    return d->m_activeEngine->localsModel();
}

QAbstractItemModel *QmlCppEngine::watchersModel() const
{
    return d->m_activeEngine->watchersModel();
}

QAbstractItemModel *QmlCppEngine::returnModel() const
{
    return d->m_cppEngine->returnModel();
}

QAbstractItemModel *QmlCppEngine::sourceFilesModel() const
{
    return d->m_cppEngine->sourceFilesModel();
}

void QmlCppEngine::detachDebugger()
{
    d->m_qmlEngine->detachDebugger();
    d->m_cppEngine->detachDebugger();
}

void QmlCppEngine::executeStep()
{
    d->m_activeEngine->executeStep();
}

void QmlCppEngine::executeStepOut()
{
    d->m_activeEngine->executeStepOut();
}

void QmlCppEngine::executeNext()
{
    d->m_activeEngine->executeNext();
}

void QmlCppEngine::executeStepI()
{
    d->m_activeEngine->executeStepI();
}

void QmlCppEngine::executeNextI()
{
    d->m_activeEngine->executeNextI();
}

void QmlCppEngine::executeReturn()
{
    d->m_activeEngine->executeReturn();
}

void QmlCppEngine::continueInferior()
{
    d->m_activeEngine->continueInferior();
}

void QmlCppEngine::interruptInferior()
{
    d->m_activeEngine->interruptInferior();
}

void QmlCppEngine::requestInterruptInferior()
{
    d->m_activeEngine->requestInterruptInferior();
}

void QmlCppEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    d->m_activeEngine->executeRunToLine(fileName, lineNumber);
}

void QmlCppEngine::executeRunToFunction(const QString &functionName)
{
    d->m_activeEngine->executeRunToFunction(functionName);
}

void QmlCppEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    d->m_activeEngine->executeJumpToLine(fileName, lineNumber);
}

void QmlCppEngine::executeDebuggerCommand(const QString &command)
{
    d->m_activeEngine->executeDebuggerCommand(command);
}

void QmlCppEngine::frameUp()
{
    d->m_activeEngine->frameUp();
}

void QmlCppEngine::frameDown()
{
    d->m_activeEngine->frameDown();
}

void QmlCppEngine::notifyInferiorRunOk()
{
    DebuggerEngine::notifyInferiorRunOk();

    Core::EditorManager *em = Core::EditorManager::instance();
    editorChanged(em->currentEditor());
}

void QmlCppEngine::setupEngine()
{
    d->m_cppEngine->startDebugger(runControl());
}

void QmlCppEngine::setupInferior()
{
    // called through notifyEngineSetupOk
}

void QmlCppEngine::runEngine()
{
    // should never happen
    showMessage(QString(Q_FUNC_INFO), LogError);
}

void QmlCppEngine::shutdownInferior()
{
    // user wants to stop inferior: always use cpp engine for this.
    if (d->m_activeEngine == d->m_qmlEngine) {
        d->m_activeEngine = d->m_cppEngine;

        // we end up in this state after trying to shut down while debugging qml.
        // b/c qml does not shutdown by itself, restore previous state and continue.
        if (d->m_qmlEngine->state() == InferiorShutdownRequested) {
            d->m_qmlEngine->setState(InferiorStopOk, true);
        }

        if (d->m_qmlEngine->state() == InferiorStopOk) {
            d->m_qmlEngine->continueInferior();
        }
    }
    if (d->m_cppEngine->state() == InferiorRunOk) {
        // first interrupt c++ engine; when done, we can shutdown.
        d->m_shutdownDeferred = true;
        d->m_cppEngine->requestInterruptInferior();
    }
    if (!d->m_shutdownDeferred)
        d->m_cppEngine->shutdownInferior();
}

void QmlCppEngine::shutdownEngine()
{
    d->m_cppEngine->shutdownEngine();
    d->m_qmlEngine->shutdownEngineAsSlave();
    notifyEngineShutdownOk();
}

void QmlCppEngine::finishDebugger()
{
    if (!d->m_shutdownDone) {
        d->m_shutdownDone = true;
        if (d->m_shutdownOk) {
            notifyEngineShutdownOk();
        } else {
            notifyEngineShutdownFailed();
        }
    }
}

void QmlCppEngine::setupSlaveEngineOnTimer()
{
    QTimer::singleShot(ConnectionWaitTimeMs, this, SLOT(setupSlaveEngine()));
}

void QmlCppEngine::setupSlaveEngine()
{
    if (state() == InferiorRunRequested)
        d->m_qmlEngine->startDebugger(runControl());
}

void QmlCppEngine::masterEngineStateChanged(const DebuggerState &newState)
{
    //qDebug() << "gdb state set to" << newState;

    switch(newState) {
    case EngineSetupRequested:
        // go through this state
        break;

    case EngineSetupFailed:
        notifyEngineSetupFailed();
        break;

    case EngineSetupOk:
        notifyEngineSetupOk();
        break;

    case InferiorSetupRequested:
        // go through this state
        break;

    case InferiorSetupFailed:
        notifyInferiorSetupFailed();
        break;

    case EngineRunRequested:
        setState(newState);
        break;

    case EngineRunFailed:
        notifyEngineRunFailed();
        break;

    case InferiorUnrunnable:
        setState(newState);
        break;

    case InferiorRunRequested:
        setState(newState);
        break;

    case InferiorRunOk:
        if (d->m_qmlEngine->state() == DebuggerNotReady) {
            if (d->m_isInitialStartup) {
                d->m_isInitialStartup = false;
                setupSlaveEngineOnTimer();
            } else {
                setupSlaveEngine();
            }
        } else {
            notifyInferiorRunOk();
        }
        break;

    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;

    case InferiorStopRequested:
        if (state() == InferiorRunRequested) {
            // if stopping on startup, move on to normal state
            // and go forward. Also, stop connection and continue later if needed.
            d->m_qmlEngine->pauseConnection();
            setState(EngineRunRequested, true);
            notifyEngineRunAndInferiorRunOk();
            setState(newState);
        } else {
            setState(newState);
        }
        break;

    case InferiorStopOk:
        // debugger can stop while qml connection is not made yet, so we can
        // end up in an illegal state transition here.
        if (state() == InferiorStopRequested
         || state() == InferiorRunFailed)
        {
            setState(newState);
        } else if (state() == InferiorRunOk) {
            // if we break on CPP side while running & engine is QML, switch.
            if (d->m_activeEngine == d->m_qmlEngine) {
                setActiveEngine(CppLanguage);
            }
            setState(newState);
        } else if (state() == InferiorRunRequested) {
            setState(newState, true);
        }
        if (d->m_shutdownDeferred) {
            d->m_activeEngine = d->m_cppEngine;
            d->m_shutdownDeferred = false;
            shutdownInferior();
        }
        break;

    case InferiorStopFailed:
        setState(newState);
        if (d->m_shutdownDeferred) {
            d->m_activeEngine = d->m_cppEngine;
            d->m_shutdownDeferred = false;
            shutdownInferior();
        }
        break;

    // here, we shut down the qml engine first.
    // but due to everything being asyncronous, we cannot guarantee
    // that it is shut down completely before gdb engine is shut down.
    case InferiorShutdownRequested:
        if (d->m_activeEngine == d->m_qmlEngine) {
            d->m_activeEngine = d->m_cppEngine;
        }

        d->m_qmlEngine->shutdownInferiorAsSlave();
        setState(newState);
        break;

    case InferiorShutdownOk:
        setState(newState);
        d->m_qmlEngine->shutdownEngineAsSlave();
        break;

    case InferiorShutdownFailed:
        setState(newState);
        d->m_qmlEngine->shutdownEngineAsSlave();
        break;

    case EngineShutdownRequested:
        setState(newState);
        break;

    case EngineShutdownOk:
        finishDebugger();
        break;

    case EngineShutdownFailed:
        d->m_shutdownOk = false;
        finishDebugger();
        break;

    default:
        break;
    }
}

void QmlCppEngine::slaveEngineStateChanged(const DebuggerState &newState)
{
    //qDebug() << "  qml engine changed to" << newState;

    if (d->m_activeEngine == d->m_qmlEngine) {
        handleSlaveEngineStateChangeAsActive(newState);
    } else {
        handleSlaveEngineStateChange(newState);
    }
}

void QmlCppEngine::handleSlaveEngineStateChange(const DebuggerState &newState)
{
    switch(newState) {
    case InferiorRunOk:
        if (state() == InferiorRunRequested) {
            // force state back so that the notification will succeed on init
            setState(EngineRunRequested, true);
            notifyEngineRunAndInferiorRunOk();
        } else {
            // we have to manually override state with current one, because
            // otherwise we'll have debugger controls in inconsistent state.
            setState(state(), true);
        }

        break;

    case InferiorStopOk:
        if (state() == InferiorRunOk) {
            // breakpoint was hit while running the app; change the active engine.
            setActiveEngine(QmlLanguage);
            setState(InferiorStopOk);
        }
        break;

    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;

    case EngineShutdownFailed:
        d->m_shutdownOk = false;
        break;

    default:
        // reset wrapper engine state to current state.
        setState(state(), true);
        break;
    }
}

void QmlCppEngine::handleSlaveEngineStateChangeAsActive(const DebuggerState &newState)
{
    switch(newState) {
    case InferiorRunRequested:
        setState(newState);
        break;

    case InferiorRunOk:
        setState(newState);
        break;

    case InferiorStopOk:
        setState(newState);
        break;

    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;

    case InferiorShutdownRequested:
        if (d->m_cppEngine->state() == InferiorRunOk) {
            // occurs when user presses stop button from debugger UI.
            shutdownInferior();
        }
        break;

    default:
        break;
    }
}

DebuggerEngine *QmlCppEngine::cppEngine() const
{
    return d->m_cppEngine;
}

} // namespace Debugger
