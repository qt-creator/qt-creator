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

#include "qmlcppengine.h"
#include "qmlengine.h"

#include <debugger/debuggercore.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/breakhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threaddata.h>
#include <debugger/watchhandler.h>
#include <debugger/console/console.h>

#include <utils/qtcassert.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <cppeditor/cppeditorconstants.h>

namespace Debugger {
namespace Internal {

enum { debug = 0 };

#define EDEBUG(s) do { if (debug) qDebug() << s; } while (0)

#define CHECK_STATE(s) do { checkState(s, __FILE__, __LINE__); } while (0)

DebuggerEngine *createQmlCppEngine(DebuggerEngine *cppEngine)
{
    return new QmlCppEngine(cppEngine);
}


////////////////////////////////////////////////////////////////////////
//
// QmlCppEngine
//
////////////////////////////////////////////////////////////////////////

QmlCppEngine::QmlCppEngine(DebuggerEngine *cppEngine)
{
    setObjectName("QmlCppEngine");
    m_qmlEngine = new QmlEngine;
    m_qmlEngine->setMasterEngine(this);
    m_cppEngine = cppEngine;
    m_cppEngine->setMasterEngine(this);
    m_activeEngine = m_cppEngine;
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

bool QmlCppEngine::canHandleToolTip(const DebuggerToolTipContext &ctx) const
{
    bool success = false;
    if (ctx.isCppEditor)
        success = m_cppEngine->canHandleToolTip(ctx);
    else
        success = m_qmlEngine->canHandleToolTip(ctx);
    return success;
}

void QmlCppEngine::updateItem(const QString &iname)
{
    if (iname.startsWith("inspect."))
        m_qmlEngine->updateItem(iname);
    else
        m_activeEngine->updateItem(iname);
}

void QmlCppEngine::expandItem(const QString &iname)
{
    if (iname.startsWith("inspect."))
        m_qmlEngine->expandItem(iname);
    else
        m_activeEngine->expandItem(iname);
}

void QmlCppEngine::selectWatchData(const QString &iname)
{
    if (iname.startsWith("inspect."))
        m_qmlEngine->selectWatchData(iname);
}

void QmlCppEngine::watchPoint(const QPoint &point)
{
    m_cppEngine->watchPoint(point);
}

void QmlCppEngine::fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length)
{
    m_cppEngine->fetchMemory(agent, addr, length);
}

void QmlCppEngine::changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data)
{
    m_cppEngine->changeMemory(agent, addr, data);
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

void QmlCppEngine::setRegisterValue(const QString &name, const QString &value)
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

void QmlCppEngine::doUpdateLocals(const UpdateParameters &up)
{
    m_activeEngine->doUpdateLocals(up);
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
    m_activeEngine->setState(InferiorStopRequested);
    m_activeEngine->interruptInferior();
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
    m_activeEngine = m_cppEngine;
    m_qmlEngine->setupSlaveEngine();
    m_cppEngine->setupSlaveEngine();
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
    m_qmlEngine->shutdownSlaveEngine();
    m_cppEngine->shutdownSlaveEngine();
}

void QmlCppEngine::quitDebugger()
{
    EDEBUG("\nMASTER QUIT DEBUGGER");
    m_cppEngine->quitDebugger();
}

void QmlCppEngine::abortDebuggerProcess()
{
    EDEBUG("\nMASTER ABORT DEBUGGER");
    m_cppEngine->abortDebuggerProcess();
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
         ? m_qmlEngine.data() : m_cppEngine.data();

    QTC_ASSERT(otherEngine, return);
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
            m_qmlEngine->quitDebugger();
            notifyEngineSetupFailed();
            break;
        }
        case EngineSetupOk: {
            notifyEngineSetupOk();
            break;
        }
        case EngineRunRequested: {
            // set by queueRunEngine()
            break;
        }
        case EngineRunFailed: {
            m_qmlEngine->quitDebugger();
            notifyEngineRunFailed();
            break;
        }
        case InferiorUnrunnable: {
            m_qmlEngine->quitDebugger();
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
            if (state() == EngineRunRequested) {
                notifyEngineRunAndInferiorRunOk();
            } else if (state() == InferiorRunRequested) {
                notifyInferiorRunOk();
            } else if (state() == InferiorStopOk) {
                notifyInferiorRunRequested();
                notifyInferiorRunOk();
            } else {
                QTC_ASSERT(false, qDebug() << state());
            }

            if (m_qmlEngine->state() == InferiorStopOk) {
                // track qml engine again
                setState(InferiorStopRequested);
                notifyInferiorStopOk();
                setActiveEngine(m_qmlEngine);
            }
            break;
        }
        case InferiorRunFailed: {
            m_qmlEngine->quitDebugger();
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
                    if (!isDying()) {
                        notifyInferiorRunRequested();
                        notifyInferiorRunOk();
                        setState(InferiorStopRequested);
                    }
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
                m_activeEngine = m_cppEngine;

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
                    break;
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
        case InferiorShutdownRequested: {
            if (state() == InferiorStopOk) {
                setState(InferiorShutdownRequested);
            } else {
                // might be set by queueShutdownInferior() already
                CHECK_STATE(InferiorShutdownRequested);
            }
            m_qmlEngine->quitDebugger();
            break;
        }
        case InferiorShutdownFinished: {
            if (state() == InferiorShutdownRequested) {
                notifyInferiorShutdownFinished();
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
        case EngineShutdownFinished: {
            CHECK_STATE(EngineShutdownRequested);
            notifyEngineShutdownFinished();
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
                if (m_activeEngine != m_qmlEngine) {
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
            if (m_activeEngine == m_qmlEngine) {
                CHECK_STATE(InferiorRunRequested);
                notifyInferiorRunOk();
            }
        }
    }
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

void QmlCppEngine::setRunTool(DebuggerRunTool *runTool)
{
    DebuggerEngine::setRunTool(runTool);
    m_qmlEngine->setRunTool(runTool);
    m_cppEngine->setRunTool(runTool);
}

void QmlCppEngine::setActiveEngine(DebuggerEngine *engine)
{
    m_activeEngine = engine;
    updateViews();
}

void QmlCppEngine::loadAdditionalQmlStack()
{
    if (m_cppEngine)
        m_cppEngine->loadAdditionalQmlStack();
}

} // namespace Internal
} // namespace Debugger
