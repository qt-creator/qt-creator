
#include "qmlcppengine.h"

#include "debuggercore.h"
#include "debuggerstartparameters.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

enum { debug = 0 };

#define EDEBUG(s) do { if (debug) qDebug() << s; } while (0)

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


////////////////////////////////////////////////////////////////////////
//
// QmlCppEnginePrivate
//
////////////////////////////////////////////////////////////////////////

class QmlCppEnginePrivate : public QObject
{
    Q_OBJECT

public:
    QmlCppEnginePrivate(QmlCppEngine *parent,
            const DebuggerStartParameters &sp);
    ~QmlCppEnginePrivate() {}

private slots:
    void cppStackChanged();
    void qmlStackChanged();

private:
    friend class QmlCppEngine;
    QmlCppEngine *q;
    DebuggerEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
    int m_stackBoundary;
};


QmlCppEnginePrivate::QmlCppEnginePrivate(QmlCppEngine *parent,
        const DebuggerStartParameters &sp)
  : q(parent)
{
    m_stackBoundary = 0;
    m_cppEngine = 0;
    m_activeEngine = 0;
    m_qmlEngine = createQmlEngine(sp, q);

    if (sp.cppEngineType == GdbEngineType) {
        m_cppEngine = createGdbEngine(sp, q);
    } else {
        QString errorMessage;
        m_cppEngine = createCdbEngine(sp, q, &errorMessage);
        if (!m_cppEngine) {
            qWarning("%s", qPrintable(errorMessage));
            return;
        }
    }

    m_activeEngine = m_cppEngine;

    connect(m_cppEngine->stackHandler()->model(), SIGNAL(modelReset()),
        SLOT(cppStackChanged()), Qt::QueuedConnection);
    connect(m_qmlEngine->stackHandler()->model(), SIGNAL(modelReset()),
        SLOT(qmlStackChanged()), Qt::QueuedConnection);
    connect(m_cppEngine, SIGNAL(stackFrameCompleted()), q, SIGNAL(stackFrameCompleted()));
    connect(m_qmlEngine, SIGNAL(stackFrameCompleted()), q, SIGNAL(stackFrameCompleted()));
}

void QmlCppEnginePrivate::cppStackChanged()
{
    const QLatin1String firstFunction("QScript::FunctionWrapper::proxyCall");
    StackFrames frames;
    foreach (const StackFrame &frame, m_cppEngine->stackHandler()->frames()) {
        if (frame.function.endsWith(firstFunction))
            break;
        frames.append(frame);
    }
    int level = frames.size();
    m_stackBoundary = level;
    foreach (StackFrame frame, m_qmlEngine->stackHandler()->frames()) {
        frame.level = level++;
        frames.append(frame);
    }
    q->stackHandler()->setFrames(frames);
}

void QmlCppEnginePrivate::qmlStackChanged()
{
    StackFrames frames = m_qmlEngine->stackHandler()->frames();
    q->stackHandler()->setFrames(frames);
    m_stackBoundary = frames.size();
}


////////////////////////////////////////////////////////////////////////
//
// QmlCppEngine
//
////////////////////////////////////////////////////////////////////////

QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp)
    : DebuggerEngine(sp), d(new QmlCppEnginePrivate(this, sp))
{
//    setStateDebugging(true);
//    d->m_cppEngine->setStateDebugging(true);
//    d->m_qmlEngine->setStateDebugging(true);
}

QmlCppEngine::~QmlCppEngine()
{
    delete d->m_qmlEngine;
    delete d->m_cppEngine;
}

void QmlCppEngine::setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    d->m_activeEngine->setToolTipExpression(mousePos, editor, ctx);
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
    if (index >= d->m_stackBoundary)
        d->m_qmlEngine->activateFrame(index - d->m_stackBoundary);
    else
        d->m_cppEngine->activateFrame(index);
    stackHandler()->setCurrentIndex(index);
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
    if (d->m_activeEngine == d->m_qmlEngine) {
        QTC_ASSERT(d->m_cppEngine->state() == InferiorRunOk, /**/);
        if (d->m_cppEngine->setupQmlStep(true))
            return; // Wait for callback to readyToExecuteQmlStep()
    } else {
        notifyInferiorRunRequested();
        d->m_cppEngine->executeStep();
    }
}

void QmlCppEngine::readyToExecuteQmlStep()
{
    notifyInferiorRunRequested();
    d->m_qmlEngine->executeStep();
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
    EDEBUG("\nMASTER CONTINUE INFERIOR"
        << d->m_cppEngine->state() << d->m_qmlEngine->state());
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
    EDEBUG("\nMASTER INTERRUPT INFERIOR");
}

void QmlCppEngine::requestInterruptInferior()
{
    EDEBUG("\nMASTER REQUEST INTERUPT INFERIOR");
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
    d->m_cppEngine->executeDebuggerCommand(command);
}

/////////////////////////////////////////////////////////

void QmlCppEngine::setupEngine()
{
    EDEBUG("\nMASTER SETUP ENGINE");
    d->m_activeEngine = d->m_cppEngine;
    d->m_stackBoundary = 0;
    d->m_qmlEngine->setupSlaveEngine();
    d->m_cppEngine->setupSlaveEngine();
}

void QmlCppEngine::notifyEngineRunAndInferiorRunOk()
{
    EDEBUG("\nMASTER NOTIFY ENGINE RUN AND INFERIOR RUN OK");
    DebuggerEngine::notifyEngineRunAndInferiorRunOk();
}

void QmlCppEngine::notifyInferiorRunOk()
{
    EDEBUG("\nMASTER NOTIFY INFERIOR RUN OK");
    DebuggerEngine::notifyInferiorRunOk();
}

void QmlCppEngine::notifyInferiorSpontaneousStop()
{
    EDEBUG("\nMASTER SPONTANEOUS STOP OK");
    DebuggerEngine::notifyInferiorSpontaneousStop();
}

void QmlCppEngine::notifyInferiorShutdownOk()
{
    EDEBUG("\nMASTER INFERIOR SHUTDOWN OK");
    DebuggerEngine::notifyInferiorShutdownOk();
}

void QmlCppEngine::setupInferior()
{
    EDEBUG("\nMASTER SETUP INFERIOR");
    d->m_qmlEngine->setupSlaveInferior();
    d->m_cppEngine->setupSlaveInferior();
}

void QmlCppEngine::runEngine()
{
    EDEBUG("\nMASTER RUN ENGINE");
    d->m_qmlEngine->runSlaveEngine();
    d->m_cppEngine->runSlaveEngine();
}

void QmlCppEngine::shutdownInferior()
{
    EDEBUG("\nMASTER SHUTDOWN INFERIOR");
    d->m_qmlEngine->quitDebugger();
}

void QmlCppEngine::shutdownEngine()
{
    EDEBUG("\nMASTER SHUTDOWN ENGINE");
    d->m_qmlEngine->shutdownSlaveEngine();
    d->m_cppEngine->shutdownSlaveEngine();
}

void QmlCppEngine::setState(DebuggerState newState, bool forced)
{
    EDEBUG("SET MASTER STATE: " << newState);
    EDEBUG("  CPP STATE: " << d->m_cppEngine->state());
    EDEBUG("  QML STATE: " << d->m_qmlEngine->state());
    DebuggerEngine::setState(newState, forced);
}

void QmlCppEngine::slaveEngineStateChanged
    (DebuggerEngine *slaveEngine, const DebuggerState newState)
{
    DebuggerEngine *otherEngine = slaveEngine == d->m_cppEngine
         ? d->m_qmlEngine : d->m_cppEngine;

    EDEBUG("GOT SLAVE STATE: " << slaveEngine << newState);
    EDEBUG("  OTHER ENGINE: " << otherEngine << otherEngine->state());
    EDEBUG("  COMBINED ENGINE: " << this << state() << isDying());

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
            EDEBUG("... WAITING FOR OTHER ENGINE SETUP...");
        break;


    case InferiorSetupRequested:
        break;

    case InferiorSetupFailed:
        if (otherEngine->state() == InferiorRunOk)
            otherEngine->quitDebugger();
        else
            notifyInferiorSetupFailed();
        break;

    case InferiorSetupOk:
        if (otherEngine->state() == InferiorSetupOk)
            notifyInferiorSetupOk();
        else
            EDEBUG("... WAITING FOR OTHER INFERIOR SETUP...");
        break;


    case EngineRunRequested:
        break;

    case EngineRunFailed:
        if (otherEngine->state() == InferiorRunOk)
            otherEngine->quitDebugger();
        else
            notifyEngineRunFailed();
        break;


    case InferiorRunRequested:
        break;

    case InferiorRunFailed:
        notifyInferiorRunFailed();
        break;

    case InferiorRunOk:
        if (state() == EngineRunRequested) {
            if (otherEngine->state() == InferiorRunOk)
                notifyEngineRunAndInferiorRunOk();
            else if (otherEngine->state() == InferiorStopOk)
                notifyEngineRunAndInferiorStopOk();
            else
                EDEBUG("... WAITING FOR OTHER INFERIOR RUN");
        } else {
            if (otherEngine->state() == InferiorRunOk) {
                EDEBUG("PLANNED INFERIOR RUN");
                notifyInferiorRunOk();
            } else if (otherEngine->state() == InferiorStopOk) {
                EDEBUG("PLANNED SINGLE INFERIOR RUN");
            } else {
                EDEBUG(" **** INFERIOR RUN NOT OK ****");
            }
        }
        break;


    case InferiorStopRequested:
        break;

    case InferiorStopFailed:
        notifyInferiorStopFailed();
        break;

    case InferiorStopOk:
        if (isDying()) {
            EDEBUG("... AN INFERIOR STOPPED DURING SHUTDOWN ");
        } else {
            if (slaveEngine != d->m_activeEngine) {
                QString engineName = slaveEngine == d->m_cppEngine
                    ? QLatin1String("C++") : QLatin1String("QML");
                showStatusMessage(tr("%1 debugger activated").arg(engineName));
                d->m_activeEngine = slaveEngine;
            }
            if (otherEngine->state() == InferiorStopOk) {
                EDEBUG("... BOTH STOPPED ");
            } else if (otherEngine->state() == InferiorShutdownOk) {
                EDEBUG("... STOPP ");
            } else if (state() == InferiorStopRequested) {
                EDEBUG("... AN INFERIOR STOPPED EXPECTEDLY");
                notifyInferiorStopOk();
            } else if (state() == EngineRunRequested) {
                EDEBUG("... AN INFERIOR FAILED STARTUP, OTHER STOPPED EXPECTEDLY");
            } else {
                EDEBUG("... AN INFERIOR STOPPED SPONTANEOUSLY");
                notifyInferiorSpontaneousStop();
            }
        }
        break;


    case InferiorExitOk:
        break;

    case InferiorShutdownRequested:
        break;

    case InferiorShutdownFailed:
        notifyInferiorShutdownFailed();
        break;

    case InferiorShutdownOk:
        if (otherEngine->state() == InferiorShutdownOk) {
            if (state() == InferiorRunOk)
                notifyInferiorExited();
            else
                notifyInferiorShutdownOk();
        } else if (otherEngine->state() == InferiorRunOk) {
            otherEngine->quitDebugger();
        } else if (otherEngine->state() == InferiorStopOk) {
            otherEngine->quitDebugger();
        } else if (otherEngine->state() == EngineRunFailed) {
            EDEBUG("... INFERIOR STOPPED, OTHER ENGINE FAILED");
            notifyEngineRunFailed();
        } else if (otherEngine->state() == InferiorSetupFailed) {
            EDEBUG("... INFERIOR STOPPED, OTHER INFERIOR FAILED");
            notifyInferiorSetupFailed();
        }
        break;


    case EngineShutdownRequested:
        break;

    case EngineShutdownFailed:
        notifyEngineShutdownFailed();
        break;

    case EngineShutdownOk:
        if (otherEngine->state() == EngineShutdownOk)
            ; // Wait for DebuggerFinished.
        else
            EDEBUG("... WAITING FOR OTHER ENGINE SHUTDOWN...");
        break;


    case DebuggerFinished:
        if (otherEngine->state() == DebuggerFinished)
            notifyEngineShutdownOk();
        else
            EDEBUG("... WAITING FOR OTHER DEBUGGER TO FINISH...");
        break;
    }
}

void QmlCppEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    EDEBUG("MASTER REMOTE SETUP DONE");
    d->m_qmlEngine->handleRemoteSetupDone(gdbServerPort, qmlPort);
    d->m_cppEngine->handleRemoteSetupDone(gdbServerPort, qmlPort);
}

void QmlCppEngine::handleRemoteSetupFailed(const QString &message)
{
    EDEBUG("MASTER REMOTE SETUP FAILED");
    d->m_qmlEngine->handleRemoteSetupFailed(message);
    d->m_cppEngine->handleRemoteSetupFailed(message);
}

DebuggerEngine *QmlCppEngine::cppEngine() const
{
    return d->m_cppEngine;
}

} // namespace Internal
} // namespace Debugger

#include "qmlcppengine.moc"
