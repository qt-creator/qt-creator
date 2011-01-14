#include "qmlcppengine.h"
#include "debuggerstartparameters.h"
#include "qmlengine.h"
#include "debuggermainwindow.h"
#include "debuggercore.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <utils/qtcassert.h>

#include <QtCore/QTimer>

namespace Debugger {
namespace Internal {

const int ConnectionWaitTimeMs = 5000;

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &,
    DebuggerEngine *masterEngine, QString *);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &,
    DebuggerEngine *masterEngine);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &,
    DebuggerEngine *masterEngine);

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
};

QmlCppEnginePrivate::QmlCppEnginePrivate()
  : m_qmlEngine(0),
    m_cppEngine(0),
    m_activeEngine(0)
{}


QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp)
    : DebuggerEngine(sp), d(new QmlCppEnginePrivate)
{
    d->m_qmlEngine = createQmlEngine(sp, this);

    if (startParameters().cppEngineType == GdbEngineType) {
        d->m_cppEngine = createGdbEngine(sp, this);
    } else {
        QString errorMessage;
        d->m_cppEngine = Debugger::Internal::createCdbEngine(sp, this, &errorMessage);
        if (!d->m_cppEngine) {
            qWarning("%s", qPrintable(errorMessage));
            return;
        }
    }

    d->m_activeEngine = d->m_cppEngine;

    if (1) {
        setStateDebugging(true);
        d->m_cppEngine->setStateDebugging(true);
        d->m_qmlEngine->setStateDebugging(true);
    }
}

QmlCppEngine::~QmlCppEngine()
{
    delete d->m_qmlEngine;
    delete d->m_cppEngine;
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

void QmlCppEngine::fetchMemory(MemoryAgent *ma, QObject *obj,
        quint64 addr, quint64 length)
{
    d->m_cppEngine->fetchMemory(ma, obj, addr, length);
}

void QmlCppEngine::fetchDisassembler(DisassemblerAgent *da)
{
    d->m_cppEngine->fetchDisassembler(da);
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

void QmlCppEngine::detachDebugger()
{
    d->m_qmlEngine->detachDebugger();
    d->m_cppEngine->detachDebugger();
}

void QmlCppEngine::executeStep()
{
    notifyInferiorRunRequested();
    d->m_activeEngine->executeStep();
}

void QmlCppEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    d->m_activeEngine->executeStepOut();
}

void QmlCppEngine::executeNext()
{
    notifyInferiorRunRequested();
    d->m_activeEngine->executeNext();
}

void QmlCppEngine::executeStepI()
{
    notifyInferiorRunRequested();
    d->m_activeEngine->executeStepI();
}

void QmlCppEngine::executeNextI()
{
    notifyInferiorRunRequested();
    d->m_activeEngine->executeNextI();
}

void QmlCppEngine::executeReturn()
{
    notifyInferiorRunRequested();
    d->m_activeEngine->executeReturn();
}

void QmlCppEngine::continueInferior()
{
    qDebug() << "\nMASTER CONTINUE INFERIOR"
        << d->m_cppEngine->state() << d->m_qmlEngine->state();
    notifyInferiorRunRequested();
    if (d->m_cppEngine->state() == InferiorStopOk) {
        d->m_cppEngine->continueInferior();
    } else if (d->m_qmlEngine->state() == InferiorStopOk) {
        d->m_qmlEngine->continueInferior();
    } else {
        QTC_ASSERT(false, qDebug() << "MASTER CANNOT CONTINUE INFERIOR"
                << d->m_cppEngine->state() << d->m_qmlEngine->state());
        notifyEngineIll();
    }
}

void QmlCppEngine::interruptInferior()
{
    qDebug() << "\nMASTER INTERRUPT INFERIOR";
}

void QmlCppEngine::requestInterruptInferior()
{
    qDebug() << "\nMASTER REQUEST INTERUPT INFERIOR";
    DebuggerEngine::requestInterruptInferior();
    d->m_cppEngine->requestInterruptInferior();
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

void QmlCppEngine::setupEngine()
{
    qDebug() << "\nMASTER SETUP ENGINE";
    QTC_ASSERT(d->m_cppEngine->state() == DebuggerNotReady, /**/);
    QTC_ASSERT(d->m_qmlEngine->state() == DebuggerNotReady, /**/);
    d->m_qmlEngine->setSilentState(EngineSetupRequested);
    d->m_cppEngine->setSilentState(EngineSetupRequested);
    d->m_qmlEngine->setupEngine(); // Always succeeds.
    d->m_cppEngine->setupEngine(); // May fail.
}

void QmlCppEngine::notifyEngineRunAndInferiorRunOk()
{
    qDebug() << "\nMASTER NOTIFY ENGINE RUN AND INFERIOR RUN OK";
    DebuggerEngine::notifyEngineRunAndInferiorRunOk();
}

void QmlCppEngine::notifyInferiorRunOk()
{
    qDebug() << "\nMASTER NOTIFY INFERIOR RUN OK";
    DebuggerEngine::notifyInferiorRunOk();
}

void QmlCppEngine::notifyInferiorSpontaneousStop()
{
    qDebug() << "\nMASTER SPONTANEOUS STOP OK";
    DebuggerEngine::notifyInferiorSpontaneousStop();
}

void QmlCppEngine::notifyInferiorShutdownOk()
{
    qDebug() << "\nMASTER INFERIOR SHUTDOWN OK";
    DebuggerEngine::notifyInferiorShutdownOk();
}

void QmlCppEngine::setupInferior()
{
    qDebug() << "\nMASTER SETUP INFERIOR";
    QTC_ASSERT(d->m_cppEngine->state() == EngineSetupOk, /**/);
    QTC_ASSERT(d->m_qmlEngine->state() == EngineSetupOk, /**/);
    d->m_qmlEngine->setSilentState(InferiorSetupRequested);
    d->m_cppEngine->setSilentState(InferiorSetupRequested);
    d->m_cppEngine->setupInferior();
    d->m_qmlEngine->setupInferior();
}

void QmlCppEngine::runEngine()
{
    qDebug() << "\nMASTER RUN ENGINE";
    QTC_ASSERT(d->m_cppEngine->state() == InferiorSetupOk, /**/);
    QTC_ASSERT(d->m_qmlEngine->state() == InferiorSetupOk, /**/);
    d->m_qmlEngine->setSilentState(EngineRunRequested);
    d->m_cppEngine->setSilentState(EngineRunRequested);
    d->m_cppEngine->runEngine();
    d->m_qmlEngine->runEngine();
}

void QmlCppEngine::shutdownInferior()
{
    qDebug() << "\nMASTER SHUTDOWN INFERIOR";
    d->m_qmlEngine->quitDebugger();
}

void QmlCppEngine::shutdownEngine()
{
    qDebug() << "\nMASTER SHUTDOWN ENGINE";
    QTC_ASSERT(d->m_cppEngine->state() == InferiorShutdownOk, /**/);
    QTC_ASSERT(d->m_qmlEngine->state() == InferiorShutdownOk, /**/);
    d->m_qmlEngine->setSilentState(EngineShutdownRequested);
    d->m_cppEngine->setSilentState(EngineShutdownRequested);
    d->m_qmlEngine->shutdownEngine();
    d->m_cppEngine->shutdownEngine();
}

void QmlCppEngine::setState(DebuggerState newState, bool forced)
{
    qDebug() << "SET MASTER STATE: " << newState;
    qDebug() << "  CPP STATE: " << d->m_cppEngine->state();
    qDebug() << "  QML STATE: " << d->m_qmlEngine->state();
    DebuggerEngine::setState(newState, forced);
}

void QmlCppEngine::slaveEngineStateChanged
    (DebuggerEngine *slaveEngine, const DebuggerState newState)
{
    const bool isCpp = slaveEngine == d->m_cppEngine;
    //const bool isQml = slaveEngine == d->m_qmlEngine;
    DebuggerEngine *otherEngine = isCpp ? d->m_qmlEngine : d->m_cppEngine;

    qDebug() << "GOT SLAVE STATE: " << slaveEngine << newState;
    qDebug() << "  OTHER ENGINE: " << otherEngine << otherEngine->state();
    qDebug() << "  COMBINED ENGINE: " << this << state() << isDying();

    switch (newState) {

    case DebuggerNotReady:
    case InferiorUnrunnable:
        break;

    case EngineSetupRequested:
        break;

    case EngineSetupFailed:
        notifyEngineSetupFailed();
        break;

    case EngineSetupOk:
        if (otherEngine->state() == EngineSetupOk)
            notifyEngineSetupOk();
        else
            qDebug() << "... WAITING FOR OTHER ENGINE SETUP...";
        break;


    case InferiorSetupRequested:
        break;

    case InferiorSetupFailed:
        notifyInferiorSetupFailed();
        break;

    case InferiorSetupOk:
        if (otherEngine->state() == InferiorSetupOk)
            notifyInferiorSetupOk();
        else
            qDebug() << "... WAITING FOR OTHER INFERIOR SETUP...";
        break;


    case EngineRunRequested:
        break;

    case EngineRunFailed:
        notifyEngineRunFailed();
        break;

    case EngineRunOk:
        if (otherEngine->state() == EngineRunOk) {
            // This is conditionalized on isMasterEngine() in the
            // base class, so do it here manually.
            slaveEngine->setSilentState(InferiorRunOk);
            otherEngine->setSilentState(InferiorRunOk);
            notifyEngineRunAndInferiorRunOk();
        } else {
            qDebug() << "... WAITING FOR OTHER ENGINE RUN...";
        }
        break;


    case InferiorRunRequested:
        break;

    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;

    case InferiorRunOk:
        qDebug() << "PLANNED INFERIOR RUN";
        if (otherEngine->state() == InferiorRunOk)
            notifyInferiorRunOk();
        else
            qDebug() << " **** INFERIOR RUN NOT OK ****";
        break;


    case InferiorStopSpontaneous:
        notifyInferiorSpontaneousStop();
        slaveEngine->setSilentState(InferiorStopOk);
        if (slaveEngine != d->m_activeEngine) {
            QString engineName = slaveEngine == d->m_cppEngine
                ? QLatin1String("C++") : QLatin1String("QML");
            showStatusMessage(tr("%1 debugger activated").arg(engineName));
            d->m_activeEngine = slaveEngine;
        }
        break;

    case InferiorStopRequested:
        break;

    case InferiorStopFailed:
        notifyInferiorStopFailed();
        break;

    case InferiorStopOk:
        if (isDying()) {
            qDebug() << "... AN INFERIOR STOPPED DURING SHUTDOWN ";
        } else {
            if (otherEngine->state() == InferiorShutdownOk) {
                qDebug() << "... STOPP ";
            } else if (state() == InferiorStopRequested) {
                qDebug() << "... AN INFERIOR STOPPED EXPECTEDLY";
                notifyInferiorStopOk();
            } else {
                qDebug() << "... AN INFERIOR STOPPED UNEXPECTEDLY";
                notifyInferiorSpontaneousStop();
            }
        }
        break;


    case InferiorExitOk:
        slaveEngine->setSilentState(InferiorShutdownOk);
        if (otherEngine->state() == InferiorShutdownOk) {
            notifyInferiorExited();
        } else  {
            if (state() == InferiorRunOk)
                notifyInferiorSpontaneousStop();
            otherEngine->notifyInferiorExited();
        }
        break;

    case InferiorShutdownRequested:
        break;

    case InferiorShutdownFailed:
        notifyInferiorShutdownFailed();
        break;

    case InferiorShutdownOk:
        if (otherEngine->state() == InferiorShutdownOk)
            notifyInferiorShutdownOk();
        else if (otherEngine->state() == InferiorRunOk)
            otherEngine->quitDebugger();
        else if (otherEngine->state() == InferiorStopOk)
            otherEngine->quitDebugger();
        break;


    case EngineShutdownRequested:
        break;

    case EngineShutdownFailed:
        notifyEngineShutdownFailed();
        break;

    case EngineShutdownOk:
        if (otherEngine->state() == EngineShutdownOk)
            notifyEngineShutdownOk();
        else
            qDebug() << "... WAITING FOR OTHER ENGINE SHUTDOWN...";
        break;


    case DebuggerFinished:
         break;
    }
}

void QmlCppEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    //qDebug() << "MASETER REMOTE SETUP DONE";
    d->m_qmlEngine->handleRemoteSetupDone(gdbServerPort, qmlPort);
    d->m_cppEngine->handleRemoteSetupDone(gdbServerPort, qmlPort);
}

void QmlCppEngine::handleRemoteSetupFailed(const QString &message)
{
    //qDebug() << "MASETER REMOTE SETUP FAILED";
    d->m_qmlEngine->handleRemoteSetupFailed(message);
    d->m_cppEngine->handleRemoteSetupFailed(message);
}

DebuggerEngine *QmlCppEngine::cppEngine() const
{
    return d->m_cppEngine;
}

} // namespace Internal
} // namespace Debugger
