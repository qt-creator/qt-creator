#include "qmlcppengine.h"
#include "qmlengine.h"
#include "debuggeruiswitcher.h"
#include "debuggerplugin.h"

#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>


namespace Debugger {
namespace Internal {

const int ConnectionWaitTimeMs = 5000;

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &);

DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &sp)
{
    return new QmlCppEngine(sp);
}

QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp)
    : DebuggerEngine(sp)
    , m_shutdownOk(true)
    , m_shutdownDeferred(false)
    , m_shutdownDone(false)
    , m_isInitialStartup(true)
{
    m_qmlEngine = qobject_cast<QmlEngine*>(createQmlEngine(sp));
    m_qmlEngine->setAttachToRunningExternalApp(true);

    if (startParameters().cppEngineType == GdbEngineType) {
        m_cppEngine = createGdbEngine(sp);
    } else {
        m_cppEngine = createCdbEngine(sp);
    }

    m_cppEngine->setRunInWrapperEngine(true);
    m_qmlEngine->setRunInWrapperEngine(true);

    m_activeEngine = m_cppEngine;
    connect(m_cppEngine, SIGNAL(stateChanged(DebuggerState)), SLOT(masterEngineStateChanged(DebuggerState)));
    connect(m_qmlEngine, SIGNAL(stateChanged(DebuggerState)), SLOT(slaveEngineStateChanged(DebuggerState)));

    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(editorChanged(Core::IEditor*)));
}

QmlCppEngine::~QmlCppEngine()
{
    delete m_qmlEngine;
    delete m_cppEngine;
    m_qmlEngine = 0;
    m_cppEngine = 0;
}

void QmlCppEngine::editorChanged(Core::IEditor *editor)
{
    // change the engine based on editor, but only if we're not currently in stopped state.
    if (state() != InferiorRunOk || !editor)
        return;

    if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        setActiveEngine(Lang_Qml);
    } else {
        setActiveEngine(Lang_Cpp);
    }
}

void QmlCppEngine::setActiveEngine(DebuggerLanguage language)
{
    DebuggerEngine *previousEngine = m_activeEngine;
    bool updateEngine = false;
    QString engineName;

    if (language == Lang_Cpp) {
        engineName = QLatin1String("C++");
        m_activeEngine = m_cppEngine;
        // don't update cpp engine - at least gdb will stop temporarily,
        // which is not nice when you're just switching files.
    } else if (language == Lang_Qml) {
        engineName = QLatin1String("QML");
        m_activeEngine = m_qmlEngine;
        updateEngine = true;
    }
    if (previousEngine != m_activeEngine) {
        showStatusMessage(tr("%1 debugger activated").arg(engineName));
        plugin()->displayDebugger(m_activeEngine, updateEngine);
    }
}

void QmlCppEngine::setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, int cursorPos)
{
    m_activeEngine->setToolTipExpression(mousePos, editor, cursorPos);
}

void QmlCppEngine::updateWatchData(const WatchData &data)
{
    m_activeEngine->updateWatchData(data);
}

void QmlCppEngine::watchPoint(const QPoint &point)
{
    m_cppEngine->watchPoint(point);
}

void QmlCppEngine::fetchMemory(MemoryViewAgent *mva, QObject *obj,
        quint64 addr, quint64 length)
{
    m_cppEngine->fetchMemory(mva, obj, addr, length);
}

void QmlCppEngine::fetchDisassembler(DisassemblerViewAgent *dva)
{
    m_cppEngine->fetchDisassembler(dva);
}

void QmlCppEngine::activateFrame(int index)
{
    m_cppEngine->activateFrame(index);
}

void QmlCppEngine::reloadModules()
{
    m_cppEngine->reloadModules();
}

void QmlCppEngine::examineModules()
{
    m_cppEngine->examineModules();
}

void QmlCppEngine::loadSymbols(const QString &moduleName)
{
    m_cppEngine->loadSymbols(moduleName);
}

void QmlCppEngine::loadAllSymbols()
{
    m_cppEngine->loadAllSymbols();
}

void QmlCppEngine::requestModuleSymbols(const QString &moduleName)
{
    m_cppEngine->requestModuleSymbols(moduleName);
}

void QmlCppEngine::reloadRegisters()
{
    m_cppEngine->reloadRegisters();
}

void QmlCppEngine::reloadSourceFiles()
{
    m_cppEngine->reloadSourceFiles();
}

void QmlCppEngine::reloadFullStack()
{
    m_cppEngine->reloadFullStack();
}

void QmlCppEngine::setRegisterValue(int regnr, const QString &value)
{
    m_cppEngine->setRegisterValue(regnr, value);
}

unsigned QmlCppEngine::debuggerCapabilities() const
{
    // ### this could also be an OR of both engines' capabilities
    return m_cppEngine->debuggerCapabilities();
}

bool QmlCppEngine::isSynchroneous() const
{
    return m_activeEngine->isSynchroneous();
}

QString QmlCppEngine::qtNamespace() const
{
    return m_cppEngine->qtNamespace();
}

void QmlCppEngine::createSnapshot()
{
    m_cppEngine->createSnapshot();
}

void QmlCppEngine::updateAll()
{
    m_activeEngine->updateAll();
}

void QmlCppEngine::attemptBreakpointSynchronization()
{
    m_cppEngine->attemptBreakpointSynchronization();
    static_cast<DebuggerEngine*>(m_qmlEngine)->attemptBreakpointSynchronization();
}

void QmlCppEngine::selectThread(int index)
{
    m_cppEngine->selectThread(index);
}

void QmlCppEngine::assignValueInDebugger(const QString &expr, const QString &value)
{
    m_activeEngine->assignValueInDebugger(expr, value);
}

QAbstractItemModel *QmlCppEngine::commandModel() const
{
    return m_activeEngine->commandModel();
}

QAbstractItemModel *QmlCppEngine::modulesModel() const
{
    return m_cppEngine->modulesModel();
}

QAbstractItemModel *QmlCppEngine::breakModel() const
{
    return m_activeEngine->breakModel();
}

QAbstractItemModel *QmlCppEngine::registerModel() const
{
    return m_cppEngine->registerModel();
}

QAbstractItemModel *QmlCppEngine::stackModel() const
{
    return m_activeEngine->stackModel();
}

QAbstractItemModel *QmlCppEngine::threadsModel() const
{
    return m_cppEngine->threadsModel();
}

QAbstractItemModel *QmlCppEngine::localsModel() const
{
    return m_activeEngine->localsModel();
}

QAbstractItemModel *QmlCppEngine::watchersModel() const
{
    return m_activeEngine->watchersModel();
}

QAbstractItemModel *QmlCppEngine::returnModel() const
{
    return m_cppEngine->returnModel();
}

QAbstractItemModel *QmlCppEngine::sourceFilesModel() const
{
    return m_cppEngine->sourceFilesModel();
}

void QmlCppEngine::detachDebugger()
{
    m_qmlEngine->detachDebugger();
    m_cppEngine->detachDebugger();
}

void QmlCppEngine::executeStep()
{
    m_activeEngine->executeStep();
}

void QmlCppEngine::executeStepOut()
{
    m_activeEngine->executeStepOut();
}

void QmlCppEngine::executeNext()
{
    m_activeEngine->executeNext();
}

void QmlCppEngine::executeStepI()
{
    m_activeEngine->executeStepI();
}

void QmlCppEngine::executeNextI()
{
    m_activeEngine->executeNextI();
}

void QmlCppEngine::executeReturn()
{
    m_activeEngine->executeReturn();
}

void QmlCppEngine::continueInferior()
{
    m_activeEngine->continueInferior();
}

void QmlCppEngine::interruptInferior()
{
    m_activeEngine->interruptInferior();
}

void QmlCppEngine::requestInterruptInferior()
{
    m_activeEngine->requestInterruptInferior();
}

void QmlCppEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    m_activeEngine->executeRunToLine(fileName, lineNumber);
}

void QmlCppEngine::executeRunToFunction(const QString &functionName)
{
    m_activeEngine->executeRunToFunction(functionName);
}

void QmlCppEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    m_activeEngine->executeJumpToLine(fileName, lineNumber);
}

void QmlCppEngine::executeDebuggerCommand(const QString &command)
{
    m_activeEngine->executeDebuggerCommand(command);
}

void QmlCppEngine::frameUp()
{
    m_activeEngine->frameUp();
}

void QmlCppEngine::frameDown()
{
    m_activeEngine->frameDown();
}

void QmlCppEngine::notifyInferiorRunOk()
{
    DebuggerEngine::notifyInferiorRunOk();

    Core::EditorManager *em = Core::EditorManager::instance();
    editorChanged(em->currentEditor());
}

void QmlCppEngine::setupEngine()
{
    m_cppEngine->startDebugger(runControl());
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
    if (m_activeEngine == m_qmlEngine) {
        m_activeEngine = m_cppEngine;

        // we end up in this state after trying to shut down while debugging qml.
        // b/c qml does not shutdown by itself, restore previous state and continue.
        if (m_qmlEngine->state() == InferiorShutdownRequested) {
            m_qmlEngine->setState(InferiorStopOk, true);
        }

        if (m_qmlEngine->state() == InferiorStopOk) {
            m_qmlEngine->continueInferior();
        }
    }
    if (m_cppEngine->state() == InferiorRunOk) {
        // first interrupt c++ engine; when done, we can shutdown.
        m_shutdownDeferred = true;
        m_cppEngine->requestInterruptInferior();
    }
    if (!m_shutdownDeferred)
        m_cppEngine->shutdownInferior();
}

void QmlCppEngine::shutdownEngine()
{
    m_cppEngine->shutdownEngine();
    m_qmlEngine->shutdownEngineAsSlave();
    notifyEngineShutdownOk();
}

void QmlCppEngine::finishDebugger()
{
    if (!m_shutdownDone) {
        m_shutdownDone = true;
        if (m_shutdownOk) {
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
        m_qmlEngine->startDebugger(runControl());
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
        if (m_qmlEngine->state() == DebuggerNotReady) {
            if (m_isInitialStartup) {
                m_isInitialStartup = false;
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
            m_qmlEngine->pauseConnection();
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
            if (m_activeEngine == m_qmlEngine) {
                setActiveEngine(Lang_Cpp);
            }
            setState(newState);
        } else if (state() == InferiorRunRequested) {
            setState(newState, true);
        }
        if (m_shutdownDeferred) {
            m_activeEngine = m_cppEngine;
            m_shutdownDeferred = false;
            shutdownInferior();
        }
        break;

    case InferiorStopFailed:
        setState(newState);
        if (m_shutdownDeferred) {
            m_activeEngine = m_cppEngine;
            m_shutdownDeferred = false;
            shutdownInferior();
        }
        break;

    // here, we shut down the qml engine first.
    // but due to everything being asyncronous, we cannot guarantee
    // that it is shut down completely before gdb engine is shut down.
    case InferiorShutdownRequested:
        if (m_activeEngine == m_qmlEngine) {
            m_activeEngine = m_cppEngine;
        }

        m_qmlEngine->shutdownInferiorAsSlave();
        setState(newState);
        break;

    case InferiorShutdownOk:
        setState(newState);
        m_qmlEngine->shutdownEngineAsSlave();
        break;

    case InferiorShutdownFailed:
        setState(newState);
        m_qmlEngine->shutdownEngineAsSlave();
        break;

    case EngineShutdownRequested:
        setState(newState);
        break;

    case EngineShutdownOk:
        finishDebugger();
        break;

    case EngineShutdownFailed:
        m_shutdownOk = false;
        finishDebugger();
        break;

    default:
        break;
    }
}

void QmlCppEngine::slaveEngineStateChanged(const DebuggerState &newState)
{
    //qDebug() << "  qml engine changed to" << newState;

    if (m_activeEngine == m_qmlEngine) {
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
            setActiveEngine(Lang_Qml);
            setState(InferiorStopOk);
        }
        break;

    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;

    case EngineShutdownFailed:
        m_shutdownOk = false;
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
        if (m_cppEngine->state() == InferiorRunOk) {
            // occurs when user presses stop button from debugger UI.
            shutdownInferior();
        }
        break;

    default:
        break;
    }
}

} // namespace Internal
} // namespace Debugger
