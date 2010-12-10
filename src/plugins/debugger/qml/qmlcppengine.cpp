#include "qmlcppengine.h"
#include "qmlengine.h"
#include "debuggermainwindow.h"
#include "debuggercore.h"

#include <qmljseditor/qmljseditorconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QTimer>

namespace Debugger {
namespace Internal {

const int ConnectionWaitTimeMs = 5000;

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &, QString *);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &);

DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &sp)
{
    QmlCppEngine *newEngine = new QmlCppEngine(sp);
    if (newEngine->cppEngine())
        return newEngine;
    delete newEngine;
    return 0;
}

class QmlCppEnginePrivate
{
public:
    QmlCppEnginePrivate();
    ~QmlCppEnginePrivate() {}

    friend class QmlCppEngine;
private:
    DebuggerEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
    DebuggerState m_errorState;
};

QmlCppEnginePrivate::QmlCppEnginePrivate()
  : m_qmlEngine(0),
    m_cppEngine(0),
    m_activeEngine(0),
    m_errorState(InferiorRunOk)
{}


QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp)
    : DebuggerEngine(sp), d(new QmlCppEnginePrivate)
{
    d->m_qmlEngine = createQmlEngine(sp);

    if (startParameters().cppEngineType == GdbEngineType) {
        d->m_cppEngine = createGdbEngine(sp);
    } else {
        QString errorMessage;
        d->m_cppEngine = createCdbEngine(sp, &errorMessage);
        if (!d->m_cppEngine) {
            qWarning("%s", qPrintable(errorMessage));
            return;
        }
    }

    d->m_cppEngine->setSlaveEngine(true);
    d->m_qmlEngine->setSlaveEngine(true);

    d->m_activeEngine = d->m_cppEngine;
    connect(d->m_cppEngine, SIGNAL(stateChanged(DebuggerState)),
        SLOT(masterEngineStateChanged(DebuggerState)));
    connect(d->m_qmlEngine, SIGNAL(stateChanged(DebuggerState)),
        SLOT(slaveEngineStateChanged(DebuggerState)));

    connect(Core::EditorManager::instance(),
        SIGNAL(currentEditorChanged(Core::IEditor*)),
        SLOT(editorChanged(Core::IEditor*)));
}

QmlCppEngine::~QmlCppEngine()
{
    delete d->m_qmlEngine;
    delete d->m_cppEngine;
}

void QmlCppEngine::editorChanged(Core::IEditor *editor)
{
    // Change the engine based on editor, but only if we're not
    // currently in stopped state.
    if (state() != InferiorRunOk || !editor)
        return;

    if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID)
        setActiveEngine(QmlLanguage);
    else
        setActiveEngine(CppLanguage);
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
        debuggerCore()->displayDebugger(d->m_activeEngine, updateEngine);
    }
}

DebuggerLanguage QmlCppEngine::activeEngine() const
{
    if (d->m_activeEngine == d->m_cppEngine)
        return CppLanguage;
    if (d->m_activeEngine == d->m_qmlEngine)
        return QmlLanguage;
    return AnyLanguage;
}

void QmlCppEngine::setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, int cursorPos)
{
    d->m_activeEngine->setToolTipExpression(mousePos, editor, cursorPos);
}

void QmlCppEngine::updateWatchData(const WatchData &data,
    const WatchUpdateFlags &flags)
{
    d->m_activeEngine->updateWatchData(data, flags);
}

void QmlCppEngine::watchPoint(const QPoint &point)
{
    d->m_cppEngine->watchPoint(point);
}

void QmlCppEngine::fetchMemory(MemoryViewAgent *mva, QObject *obj,
        quint64 addr, quint64 length)
{
    d->m_cppEngine->fetchMemory(mva, obj, addr, length);
}

void QmlCppEngine::fetchDisassembler(DisassemblerViewAgent *dva)
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
    d->m_qmlEngine->attemptBreakpointSynchronization();
}

bool QmlCppEngine::acceptsBreakpoint(BreakpointId id) const
{
    return d->m_cppEngine->acceptsBreakpoint(id)
        || d->m_qmlEngine->acceptsBreakpoint(id);
}

void QmlCppEngine::selectThread(int index)
{
    d->m_cppEngine->selectThread(index);
}

void QmlCppEngine::assignValueInDebugger(const WatchData *data,
    const QString &expr, const QVariant &value)
{
    d->m_activeEngine->assignValueInDebugger(data, expr, value);
}

QAbstractItemModel *QmlCppEngine::modulesModel() const
{
    return d->m_cppEngine->modulesModel();
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
    if (d->m_activeEngine->state() == InferiorStopOk) {
        d->m_activeEngine->continueInferior();
    } else {
        notifyInferiorRunRequested();
    }
}

void QmlCppEngine::interruptInferior()
{
    if (d->m_activeEngine->state() == InferiorRunOk) {
        d->m_activeEngine->requestInterruptInferior();
    } else {
        if (d->m_activeEngine->state() == InferiorStopOk && (!checkErrorState(InferiorStopFailed))) {
            notifyInferiorStopOk();
        }
    }
}

void QmlCppEngine::requestInterruptInferior()
{
    DebuggerEngine::requestInterruptInferior();

    if (d->m_activeEngine->state() == InferiorRunOk) {
        d->m_activeEngine->requestInterruptInferior();
    }
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

/////////////////////////////////////////////////////////

bool QmlCppEngine::checkErrorState(const DebuggerState stateToCheck)
{
    if (d->m_errorState != stateToCheck)
        return false;

    // reset state ( so that more than one error can accumulate over time )
    d->m_errorState = InferiorRunOk;
    switch (stateToCheck) {
    case InferiorRunOk:
        // nothing to do
        break;
    case EngineRunFailed:
        notifyEngineRunFailed();
        break;
    case EngineSetupFailed:
        notifyEngineSetupFailed();
        break;
    case EngineShutdownFailed:
        notifyEngineShutdownFailed();
        break;
    case InferiorSetupFailed:
        notifyInferiorSetupFailed();
        break;
    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;
    case InferiorUnrunnable:
        notifyInferiorUnrunnable();
        break;
    case InferiorStopFailed:
        notifyInferiorStopFailed();
        break;
    case InferiorShutdownFailed:
        notifyInferiorShutdownFailed();
        break;
    default:
        // unexpected
        break;
    }
    return true;
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
    if (!checkErrorState(InferiorSetupFailed)) {
        notifyInferiorSetupOk();
    }
}

void QmlCppEngine::runEngine()
{
    if (!checkErrorState(EngineRunFailed)) {
        if (d->m_errorState == InferiorRunOk) {
            switch (d->m_activeEngine->state()) {
            case InferiorRunOk:
                notifyEngineRunAndInferiorRunOk();
                break;
            case InferiorStopOk:
                notifyEngineRunAndInferiorStopOk();
                break;
            default: // not supported?
                notifyEngineRunFailed();
                break;
            }
        } else {
            notifyEngineRunFailed();
        }
    }
}

void QmlCppEngine::shutdownInferior()
{
    if (!checkErrorState(InferiorShutdownFailed)) {
        if (d->m_cppEngine->state() == InferiorStopOk) {
            d->m_cppEngine->quitDebugger();
        } else {
            notifyInferiorShutdownOk();
        }
    }
}

void QmlCppEngine::initEngineShutdown()
{
    if (d->m_qmlEngine->state() != DebuggerFinished) {
        d->m_qmlEngine->quitDebugger();
    } else
    if (d->m_cppEngine->state() != DebuggerFinished) {
        d->m_cppEngine->quitDebugger();
    } else
    if (state() == EngineSetupRequested) {
        if (!runControl() || d->m_errorState == EngineSetupFailed) {
            notifyEngineSetupFailed();
        } else {
            notifyEngineSetupOk();
        }
    } else
    if (state() == InferiorStopRequested) {
        checkErrorState(InferiorStopFailed);
    } else
    if (state() == InferiorShutdownRequested && !checkErrorState(InferiorShutdownFailed)) {
        notifyInferiorShutdownOk();
    } else
    if (state() != DebuggerFinished) {
        quitDebugger();
    }
}

void QmlCppEngine::shutdownEngine()
{
    if (!checkErrorState(EngineShutdownFailed)) {
        showStatusMessage(tr("Debugging finished"));
        notifyEngineShutdownOk();
    }
}

void QmlCppEngine::setupSlaveEngine()
{
    if (d->m_qmlEngine->state() == DebuggerNotReady)
        d->m_qmlEngine->startDebugger(runControl());
}

void QmlCppEngine::masterEngineStateChanged(const DebuggerState &newState)
{
    if (newState == InferiorStopOk) {
        setActiveEngine(CppLanguage);
    }
    engineStateChanged(newState);
}

void QmlCppEngine::slaveEngineStateChanged(const DebuggerState &newState)
{
    if (newState == InferiorStopOk) {
        setActiveEngine(QmlLanguage);
    }
    engineStateChanged(newState);
}


void QmlCppEngine::engineStateChanged(const DebuggerState &newState)
{
    switch (newState) {
    case InferiorRunOk:
        // startup?
        if (d->m_qmlEngine->state() == DebuggerNotReady) {
            setupSlaveEngine();
        } else
        if (d->m_cppEngine->state() == DebuggerNotReady) {
            setupEngine();
        } else
        if (state() == EngineSetupRequested) {
            notifyEngineSetupOk();
        } else
        // breakpoint?
        if (state() == InferiorStopOk) {
            continueInferior();
        } else
        if (state() == InferiorStopRequested) {
            checkErrorState(InferiorStopFailed);
        } else
        if (state() == InferiorRunRequested && (!checkErrorState(InferiorRunFailed)) && (!checkErrorState(InferiorUnrunnable))) {
            notifyInferiorRunOk();
        }
        break;

    case InferiorRunRequested:
        // follow the inferior
        if (state() == InferiorStopOk && checkErrorState(InferiorRunOk)) {
            continueInferior();
        }
        break;

    case InferiorStopRequested:
        // follow the inferior
        if (state() == InferiorRunOk && checkErrorState(InferiorRunOk)) {
            requestInterruptInferior();
        }
        break;

    case InferiorStopOk:
        // check breakpoints
        if (state() == InferiorRunRequested) {
            checkErrorState(InferiorRunFailed);
        } else
        if (checkErrorState(InferiorRunOk)) {
            if (state() == InferiorRunOk) {
                requestInterruptInferior();
            } else
            if (state() == InferiorStopRequested) {
                interruptInferior();
            }
        }
        break;

    case EngineRunFailed:
    case EngineSetupFailed:
    case EngineShutdownFailed:
    case InferiorSetupFailed:
    case InferiorRunFailed:
    case InferiorUnrunnable:
    case InferiorStopFailed:
    case InferiorShutdownFailed:
        if (d->m_errorState == InferiorRunOk) {
            d->m_errorState = newState;
        }
        break;

    case InferiorShutdownRequested:
        if (activeEngine() == QmlLanguage) {
            setActiveEngine(CppLanguage);
        }
        break;

    case EngineShutdownRequested:
        // we have to abort the setup before the sub-engines die
        // because we depend on an active runcontrol that will be shut down by the dying engine
        if (state() == EngineSetupRequested)
            notifyEngineSetupFailed();
        break;

    case DebuggerFinished:
        initEngineShutdown();
        break;

    default:
        break;
    }
}

void QmlCppEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    d->m_qmlEngine->handleRemoteSetupDone(gdbServerPort, qmlPort);
    d->m_cppEngine->handleRemoteSetupDone(gdbServerPort, qmlPort);
}

void QmlCppEngine::handleRemoteSetupFailed(const QString &message)
{
    d->m_qmlEngine->handleRemoteSetupFailed(message);
    d->m_cppEngine->handleRemoteSetupFailed(message);
}

DebuggerEngine *QmlCppEngine::cppEngine() const
{
    return d->m_cppEngine;
}

} // namespace Internal
} // namespace Debugger
