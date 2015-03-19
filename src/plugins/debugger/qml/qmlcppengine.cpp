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

#include "qmlcppengine.h"
#include "qmlengine.h"

#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/breakhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threaddata.h>
#include <debugger/watchhandler.h>

#include <utils/qtcassert.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <cppeditor/cppeditorconstants.h>
#include <qmljs/consolemanagerinterface.h>

namespace Debugger {
namespace Internal {

enum { debug = 0 };

#define EDEBUG(s) do { if (debug) qDebug() << s; } while (0)

#define CHECK_STATE(s) \
    do { \
        if (state() != s) { \
            showMessage(QString::fromLatin1("UNEXPECTED STATE: %1  WANTED: %2 IN %3:%4") \
                .arg(state()).arg(s).arg(QLatin1String(__FILE__)).arg(__LINE__), LogError); \
            QTC_ASSERT(false, qDebug() << state() << s); \
        } \
    } while (0)

DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &sp,
                                   QString *errorMessage)
{
    QmlCppEngine *newEngine = new QmlCppEngine(sp, errorMessage);
    if (newEngine->cppEngine())
        return newEngine;
    delete newEngine;
    return 0;
}


////////////////////////////////////////////////////////////////////////
//
// QmlCppEngine
//
////////////////////////////////////////////////////////////////////////

QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp, QString *errorMessage)
    : DebuggerEngine(sp)
{
    setObjectName(QLatin1String("QmlCppEngine"));
    m_qmlEngine = new QmlEngine(sp, this);
    m_cppEngine = DebuggerRunControlFactory::createEngine(sp.cppEngineType, sp, errorMessage);
    if (!m_cppEngine) {
        *errorMessage = tr("The slave debugging engine required for combined QML/C++-Debugging could not be created: %1").arg(*errorMessage);
        return;
    }
    m_cppEngine->setMasterEngine(this);
    setActiveEngine(m_cppEngine);
}

QmlCppEngine::~QmlCppEngine()
{
    delete m_qmlEngine;
    delete m_cppEngine;
}

bool QmlCppEngine::canDisplayTooltip() const
{
    return m_cppEngine->canDisplayTooltip() || m_qmlEngine->canDisplayTooltip();
}

bool QmlCppEngine::setToolTipExpression(const DebuggerToolTipContext &ctx)
{
    bool success = false;
    if (ctx.isCppEditor)
        success = m_cppEngine->setToolTipExpression(ctx);
    else
        success = m_qmlEngine->setToolTipExpression(ctx);
    return success;
}

void QmlCppEngine::updateWatchItem(WatchItem *item)
{
    if (item->isInspect())
        m_qmlEngine->updateWatchItem(item);
    else
        m_activeEngine->updateWatchItem(item);
}

void QmlCppEngine::watchDataSelected(const QByteArray &iname)
{
    const WatchItem *item = watchHandler()->findItem(iname);
    if (item && item->isInspect())
        m_qmlEngine->watchDataSelected(iname);
}

void QmlCppEngine::watchPoint(const QPoint &point)
{
    m_cppEngine->watchPoint(point);
}

void QmlCppEngine::fetchMemory(MemoryAgent *ma, QObject *obj,
        quint64 addr, quint64 length)
{
    m_cppEngine->fetchMemory(ma, obj, addr, length);
}

void QmlCppEngine::fetchDisassembler(DisassemblerAgent *da)
{
    m_cppEngine->fetchDisassembler(da);
}

void QmlCppEngine::activateFrame(int index)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    m_activeEngine->activateFrame(index);

    stackHandler()->setCurrentIndex(index);
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

void QmlCppEngine::setRegisterValue(const QByteArray &name, const QString &value)
{
    m_cppEngine->setRegisterValue(name, value);
}


bool QmlCppEngine::hasCapability(unsigned cap) const
{
    // ### this could also be an OR of both engines' capabilities
    bool hasCap = m_cppEngine->hasCapability(cap);
    if (m_activeEngine != m_cppEngine) {
        //Some capabilities cannot be handled by QML Engine
        //Expand this list as and when required
        if (cap == AddWatcherWhileRunningCapability)
            hasCap = hasCap || m_qmlEngine->hasCapability(cap);
        if (cap == WatchWidgetsCapability ||
                cap == DisassemblerCapability ||
                cap == OperateByInstructionCapability ||
                cap == ReverseSteppingCapability)
            hasCap = hasCap && m_qmlEngine->hasCapability(cap);
    }
    return hasCap;
}

bool QmlCppEngine::isSynchronous() const
{
    return m_activeEngine->isSynchronous();
}

QByteArray QmlCppEngine::qtNamespace() const
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

    switch (m_qmlEngine->state()) {
    case InferiorRunOk:
    case InferiorRunRequested:
    case InferiorStopOk: // fall through
    case InferiorStopRequested:
        m_qmlEngine->attemptBreakpointSynchronization();
        break;
    default:
        break;
    }
}

bool QmlCppEngine::acceptsBreakpoint(Breakpoint bp) const
{
    return m_cppEngine->acceptsBreakpoint(bp)
        || m_qmlEngine->acceptsBreakpoint(bp);
}

void QmlCppEngine::selectThread(ThreadId threadId)
{
    m_activeEngine->selectThread(threadId);
}

void QmlCppEngine::assignValueInDebugger(WatchItem *item,
    const QString &expr, const QVariant &value)
{
    if (item->isInspect())
        m_qmlEngine->assignValueInDebugger(item, expr, value);
    else
        m_activeEngine->assignValueInDebugger(item, expr, value);
}

void QmlCppEngine::notifyInferiorIll()
{
    //This will eventually shutdown the engine
    //Set final state to avoid quitDebugger() being called
    //after this call
    setTargetState(DebuggerFinished);

    //Call notifyInferiorIll of cpp engine
    //as qml engine will follow state transitions
    //of cpp engine
    m_cppEngine->notifyInferiorIll();
}

void QmlCppEngine::detachDebugger()
{
    m_qmlEngine->detachDebugger();
    m_cppEngine->detachDebugger();
}

void QmlCppEngine::executeStep()
{
    notifyInferiorRunRequested();
    m_activeEngine->executeStep();
}

void QmlCppEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    m_activeEngine->executeStepOut();
}

void QmlCppEngine::executeNext()
{
    notifyInferiorRunRequested();
    m_activeEngine->executeNext();
}

void QmlCppEngine::executeStepI()
{
    notifyInferiorRunRequested();
    m_activeEngine->executeStepI();
}

void QmlCppEngine::executeNextI()
{
    notifyInferiorRunRequested();
    m_activeEngine->executeNextI();
}

void QmlCppEngine::executeReturn()
{
    notifyInferiorRunRequested();
    m_activeEngine->executeReturn();
}

void QmlCppEngine::continueInferior()
{
    EDEBUG("\nMASTER CONTINUE INFERIOR"
        << state() << m_qmlEngine->state());
    notifyInferiorRunRequested();
    if (m_cppEngine->state() == InferiorStopOk) {
        m_cppEngine->continueInferior();
    } else if (m_qmlEngine->state() == InferiorStopOk) {
        m_qmlEngine->continueInferior();
    } else {
        QTC_ASSERT(false, qDebug() << "MASTER CANNOT CONTINUE INFERIOR"
                << m_cppEngine->state() << m_qmlEngine->state());
        notifyEngineIll();
    }
}

void QmlCppEngine::interruptInferior()
{
    EDEBUG("\nMASTER INTERRUPT INFERIOR");
    m_cppEngine->requestInterruptInferior();
}

void QmlCppEngine::requestInterruptInferior()
{
    EDEBUG("\nMASTER REQUEST INTERRUPT INFERIOR");
    DebuggerEngine::requestInterruptInferior();
}

void QmlCppEngine::executeRunToLine(const ContextData &data)
{
    m_activeEngine->executeRunToLine(data);
}

void QmlCppEngine::executeRunToFunction(const QString &functionName)
{
    m_activeEngine->executeRunToFunction(functionName);
}

void QmlCppEngine::executeJumpToLine(const ContextData &data)
{
    m_activeEngine->executeJumpToLine(data);
}

void QmlCppEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    m_qmlEngine->executeDebuggerCommand(command, languages);
    m_cppEngine->executeDebuggerCommand(command, languages);
}

/////////////////////////////////////////////////////////

void QmlCppEngine::setupEngine()
{
    EDEBUG("\nMASTER SETUP ENGINE");
    setActiveEngine(m_cppEngine);
    m_qmlEngine->setupSlaveEngine();
    m_cppEngine->setupSlaveEngine();

    if (startParameters().remoteSetupNeeded)
        notifyEngineRequestRemoteSetup();
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

void QmlCppEngine::notifyInferiorSetupOk()
{
    EDEBUG("\nMASTER INFERIOR SETUP OK");
    DebuggerEngine::notifyInferiorSetupOk();
}

void QmlCppEngine::notifyEngineRemoteServerRunning(const QByteArray &serverChannel, int pid)
{
    m_cppEngine->notifyEngineRemoteServerRunning(serverChannel, pid);
}

void QmlCppEngine::setupInferior()
{
    EDEBUG("\nMASTER SETUP INFERIOR");
    m_qmlEngine->setupSlaveInferior();
    m_cppEngine->setupSlaveInferior();
}

void QmlCppEngine::runEngine()
{
    EDEBUG("\nMASTER RUN ENGINE");
    m_qmlEngine->runSlaveEngine();
    m_cppEngine->runSlaveEngine();
}

void QmlCppEngine::shutdownInferior()
{
    EDEBUG("\nMASTER SHUTDOWN INFERIOR");
    m_cppEngine->shutdownInferior();
}

void QmlCppEngine::shutdownEngine()
{
    EDEBUG("\nMASTER SHUTDOWN ENGINE");
    m_cppEngine->shutdownSlaveEngine();
    QmlJS::ConsoleManagerInterface *consoleManager = QmlJS::ConsoleManagerInterface::instance();
    if (consoleManager)
        consoleManager->setScriptEvaluator(0);
}

void QmlCppEngine::quitDebugger()
{
    EDEBUG("\nMASTER QUIT DEBUGGER");
    m_cppEngine->quitDebugger();
}

void QmlCppEngine::abortDebugger()
{
    EDEBUG("\nMASTER ABORT DEBUGGER");
    m_cppEngine->abortDebugger();
}

void QmlCppEngine::setState(DebuggerState newState, bool forced)
{
    EDEBUG("SET MASTER STATE: " << newState);
    EDEBUG("  CPP STATE: " << m_cppEngine->state());
    EDEBUG("  QML STATE: " << m_qmlEngine->state());
    DebuggerEngine::setState(newState, forced);
}

void QmlCppEngine::slaveEngineStateChanged
    (DebuggerEngine *slaveEngine, const DebuggerState newState)
{
    DebuggerEngine *otherEngine = (slaveEngine == m_cppEngine)
         ? m_qmlEngine : m_cppEngine;

    QTC_CHECK(otherEngine != slaveEngine);

    if (debug) {
        EDEBUG("GOT SLAVE STATE: " << slaveEngine << newState);
        EDEBUG("  OTHER ENGINE: " << otherEngine << otherEngine->state());
        EDEBUG("  COMBINED ENGINE: " << this << state() << isDying());
    }

    if (state() == DebuggerFinished) {
        // We are done and don't care about slave state changes anymore.
        return;
    }

    // Idea is to follow the state of the cpp engine, except where we are stepping in QML.
    // That is, when the QmlEngine moves between InferiorStopOk, and InferiorRunOk, InferiorStopOk ...
    //
    // Accordingly, the 'active engine' is the cpp engine until the qml engine enters the
    // InferiorStopOk state. The cpp engine becomes the active one again as soon as it itself enters
    // the InferiorStopOk state.

    if (slaveEngine == m_cppEngine) {
        switch (newState) {
        case DebuggerNotReady: {
            // Can this ever happen?
            break;
        }
        case EngineSetupRequested: {
            // Set by queueSetupEngine()
            CHECK_STATE(EngineSetupRequested);
            break;
        }
        case EngineSetupFailed: {
            qmlEngine()->quitDebugger();
            notifyEngineSetupFailed();
            break;
        }
        case EngineSetupOk: {
            notifyEngineSetupOk();
            break;
        }
        case InferiorSetupRequested: {
            // Set by queueSetupInferior()
            CHECK_STATE(InferiorSetupRequested);
            break;
        }
        case InferiorSetupFailed: {
            qmlEngine()->quitDebugger();
            notifyInferiorSetupFailed();
            break;
        }
        case InferiorSetupOk: {
            notifyInferiorSetupOk();
            break;
        }
        case EngineRunRequested: {
            // set by queueRunEngine()
            break;
        }
        case EngineRunFailed: {
            qmlEngine()->quitDebugger();
            notifyEngineRunFailed();
            break;
        }
        case InferiorUnrunnable: {
            qmlEngine()->quitDebugger();
            notifyEngineRunOkAndInferiorUnrunnable();
            break;
        }
        case InferiorRunRequested: {
            // Might be set already by notifyInferiorRunRequested()
            if (state() != InferiorRunRequested) {
                CHECK_STATE(InferiorStopOk);
                notifyInferiorRunRequested();
            }
            break;
        }
        case InferiorRunOk: {
            if (state() == EngineRunRequested)
                notifyEngineRunAndInferiorRunOk();
            else if (state() == InferiorRunRequested)
                notifyInferiorRunOk();
            else
                QTC_ASSERT(false, qDebug() << state());

            if (qmlEngine()->state() == InferiorStopOk) {
                // track qml engine again
                setState(InferiorStopRequested);
                notifyInferiorStopOk();
                setActiveEngine(m_qmlEngine);
            }
            break;
        }
        case InferiorRunFailed: {
            qmlEngine()->quitDebugger();
            notifyInferiorRunFailed();
            break;
        }
        case InferiorStopRequested: {
            if (m_activeEngine == cppEngine()) {
                if (state() == InferiorRunOk) {
                    setState(InferiorStopRequested);
                } else {
                    // Might be set by doInterruptInferior()
                    CHECK_STATE(InferiorStopRequested);
                }
            } else {
                // We're debugging qml, but got an interrupt, or abort
                if (state() == InferiorRunOk) {
                    setState(InferiorStopRequested);
                } else if (state() == InferiorStopOk) {
                    notifyInferiorRunRequested();
                    notifyInferiorRunOk();
                    setState(InferiorStopRequested);
                } else if (state() == InferiorRunRequested) {
                    notifyInferiorRunOk();
                    setState(InferiorStopRequested);
                } else {
                    QTC_ASSERT(false, qDebug() << state());
                }
                // now track cpp engine
                setActiveEngine(m_cppEngine);
            }
            break;
        }
        case InferiorStopOk: {
            if (isDying()) {
                EDEBUG("... CPP ENGINE STOPPED DURING SHUTDOWN ");
                QTC_ASSERT(state() == InferiorStopRequested
                           || state() == InferiorRunOk
                           || state() == InferiorStopOk, qDebug() << state());

                // Just to make sure, we're shutting down anyway ...
                setActiveEngine(m_cppEngine);

                if (state() == InferiorStopRequested)
                    setState(InferiorStopOk);
                // otherwise we're probably inside notifyInferiorStopOk already
            } else {
                if (m_activeEngine != cppEngine()) {
                    showStatusMessage(tr("C++ debugger activated"));
                    setActiveEngine(m_cppEngine);
                }
                switch (state()) {
                case InferiorStopRequested:
                    EDEBUG("... CPP ENGINE STOPPED EXPECTEDLY");
                    notifyInferiorStopOk();
                    break;
                case EngineRunRequested:
                    EDEBUG("... CPP ENGINE STOPPED ON STARTUP");
                    notifyEngineRunAndInferiorStopOk();
                    break;
                case InferiorRunOk:
                    EDEBUG("... CPP ENGINE STOPPED SPONTANEOUSLY");
                    notifyInferiorSpontaneousStop();
                    break;
                case InferiorRunRequested:
                    // can happen if qml engine was active
                    notifyInferiorRunFailed();
                default:
                    CHECK_STATE(InferiorStopOk);
                    break;
                }
            }
            break;
        }
        case InferiorStopFailed: {
            CHECK_STATE(InferiorStopRequested);
            notifyInferiorStopFailed();
            break;
        }
        case InferiorExitOk: {
            // InferiorExitOk will be called through notifyInferiorExited
            // when InferiorShutDownOk is reached
            qmlEngine()->quitDebugger();
            break;
        }
        case InferiorShutdownRequested: {
            if (state() == InferiorStopOk) {
                setState(InferiorShutdownRequested);
            } else {
                // might be set by queueShutdownInferior() already
                CHECK_STATE(InferiorShutdownRequested);
            }
            qmlEngine()->quitDebugger();
            break;
        }
        case InferiorShutdownFailed: {
            CHECK_STATE(InferiorShutdownRequested);
            notifyInferiorShutdownFailed();
            break;
        }
        case InferiorShutdownOk: {
            if (state() == InferiorShutdownRequested) {
                notifyInferiorShutdownOk();
            } else {
                // we got InferiorExitOk before, but ignored it ...
                notifyInferiorExited();
            }
            break;
        }
        case EngineShutdownRequested: {
            // set by queueShutdownEngine()
            CHECK_STATE(EngineShutdownRequested);
            break;
        }
        case EngineShutdownFailed: {
            CHECK_STATE(EngineShutdownRequested);
            notifyEngineShutdownFailed();
            break;
        }
        case EngineShutdownOk: {
            CHECK_STATE(EngineShutdownRequested);
            notifyEngineShutdownOk();
            break;
        }
        case DebuggerFinished: {
            // set by queueFinishDebugger()
            CHECK_STATE(DebuggerFinished);
            break;
        }
        }
    } else {
        // QML engine state change
        if (newState == InferiorStopOk) {
            if (isDying()) {
                EDEBUG("... QML ENGINE STOPPED DURING SHUTDOWN ");

                // Just to make sure, we're shutting down anyway ...
                setActiveEngine(m_cppEngine);

                if (state() == InferiorStopRequested)
                    notifyInferiorStopOk();
                // otherwise we're probably inside notifyInferiorStopOk already
            } else {
                if (m_activeEngine != qmlEngine()) {
                    showStatusMessage(tr("QML debugger activated"));
                    setActiveEngine(m_qmlEngine);
                }

                if (state() == InferiorRunOk)
                    notifyInferiorSpontaneousStop();
                else if (state() == InferiorStopRequested)
                    notifyInferiorStopOk();
                else
                    CHECK_STATE(InferiorShutdownRequested);
            }

        } else if (newState == InferiorRunOk) {
            if (m_activeEngine == qmlEngine()) {
                CHECK_STATE(InferiorRunRequested);
                notifyInferiorRunOk();
            }
        }
    }
}

void QmlCppEngine::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    EDEBUG("MASTER REMOTE SETUP FINISHED");
    DebuggerEngine::notifyEngineRemoteSetupFinished(result);

    cppEngine()->notifyEngineRemoteSetupFinished(result);
    qmlEngine()->notifyEngineRemoteSetupFinished(result);
}

void QmlCppEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    if (channel == AppOutput || channel == AppError || channel == AppStuff) {
        // message is from CppEngine, allow qml engine to process
        m_qmlEngine->filterApplicationMessage(msg, channel);
    }
    DebuggerEngine::showMessage(msg, channel, timeout);
}

void QmlCppEngine::resetLocation()
{
    if (m_qmlEngine)
        m_qmlEngine->resetLocation();
    if (m_cppEngine)
        m_cppEngine->resetLocation();

    DebuggerEngine::resetLocation();
}

void QmlCppEngine::reloadDebuggingHelpers()
{
    if (m_cppEngine)
        m_cppEngine->reloadDebuggingHelpers();
}

void QmlCppEngine::debugLastCommand()
{
    if (m_cppEngine)
        m_cppEngine->debugLastCommand();
}

DebuggerEngine *QmlCppEngine::qmlEngine() const
{
    return m_qmlEngine;
}

void QmlCppEngine::setActiveEngine(DebuggerEngine *engine)
{
    m_activeEngine = engine;
    updateViews();
}

} // namespace Internal
} // namespace Debugger
