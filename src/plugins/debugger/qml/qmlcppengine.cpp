/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlcppengine.h"
#include "debuggerruncontrolfactory.h"
#include "debuggercore.h"
#include "debuggerstartparameters.h"
#include "stackhandler.h"
#include "qmlengine.h"
#include "watchdata.h"
#include "watchhandler.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <texteditor/itexteditor.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <cppeditor/cppeditorconstants.h>
#include <qmljs/consolemanagerinterface.h>

#include <QTimer>

namespace Debugger {
namespace Internal {

enum { debug = 0 };

#define EDEBUG(s) do { if (debug) qDebug() << s; } while (0)

const int ConnectionWaitTimeMs = 5000;

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
// QmlCppEnginePrivate
//
////////////////////////////////////////////////////////////////////////

class QmlCppEnginePrivate
{
public:
    QmlCppEnginePrivate() {}

    QmlEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
};


////////////////////////////////////////////////////////////////////////
//
// QmlCppEngine
//
////////////////////////////////////////////////////////////////////////

QmlCppEngine::QmlCppEngine(const DebuggerStartParameters &sp, QString *errorMessage)
    : DebuggerEngine(sp)
{
    setObjectName(QLatin1String("QmlCppEngine"));
    d = new QmlCppEnginePrivate;
    d->m_qmlEngine = new QmlEngine(sp, this);
    d->m_cppEngine = DebuggerRunControlFactory::createEngine(sp.firstSlaveEngineType, sp, errorMessage);
    d->m_cppEngine->setMasterEngine(this);
    if (!d->m_cppEngine) {
        *errorMessage = tr("The slave debugging engine required for combined QML/C++-Debugging could not be created: %1").arg(*errorMessage);
        return;
    }
    setActiveEngine(d->m_cppEngine);
}

QmlCppEngine::~QmlCppEngine()
{
    delete d->m_qmlEngine;
    delete d->m_cppEngine;
    delete d;
}

bool QmlCppEngine::canDisplayTooltip() const
{
    return d->m_cppEngine->canDisplayTooltip() || d->m_qmlEngine->canDisplayTooltip();
}

bool QmlCppEngine::setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    bool success = false;
    if (editor->id() == CppEditor::Constants::CPPEDITOR_ID)
        success = d->m_cppEngine->setToolTipExpression(mousePos, editor, ctx);
    else if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID)
        success = d->m_qmlEngine->setToolTipExpression(mousePos, editor, ctx);
    return success;
}

void QmlCppEngine::updateWatchData(const WatchData &data,
    const WatchUpdateFlags &flags)
{
    if (data.isInspect())
        d->m_qmlEngine->updateWatchData(data, flags);
    else
        d->m_activeEngine->updateWatchData(data, flags);
}

void QmlCppEngine::watchDataSelected(const QByteArray &iname)
{
    const WatchData *wd = watchHandler()->findData(iname);
    if (wd && wd->isInspect())
        d->m_qmlEngine->watchDataSelected(iname);
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
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    d->m_activeEngine->activateFrame(index);

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


bool QmlCppEngine::hasCapability(unsigned cap) const
{
    // ### this could also be an OR of both engines' capabilities
    bool hasCap = d->m_cppEngine->hasCapability(cap);
    if (d->m_activeEngine != d->m_cppEngine) {
        //Some capabilities cannot be handled by QML Engine
        //Expand this list as and when required
        if (cap == AddWatcherWhileRunningCapability)
            hasCap = hasCap || d->m_qmlEngine->hasCapability(cap);
        if (cap == WatchWidgetsCapability ||
                cap == DisassemblerCapability ||
                cap == OperateByInstructionCapability ||
                cap == ReverseSteppingCapability)
            hasCap = hasCap && d->m_qmlEngine->hasCapability(cap);
    }
    return hasCap;
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

    switch (d->m_qmlEngine->state()) {
    case InferiorRunOk:
    case InferiorRunRequested:
    case InferiorStopOk: // fall through
    case InferiorStopRequested:
        d->m_qmlEngine->attemptBreakpointSynchronization();
        break;
    default:
        break;
    }
}

bool QmlCppEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    return d->m_cppEngine->acceptsBreakpoint(id)
        || d->m_qmlEngine->acceptsBreakpoint(id);
}

void QmlCppEngine::selectThread(ThreadId threadId)
{
    d->m_activeEngine->selectThread(threadId);
}

void QmlCppEngine::assignValueInDebugger(const WatchData *data,
    const QString &expr, const QVariant &value)
{
    if (data->isInspect())
        d->m_qmlEngine->assignValueInDebugger(data, expr, value);
    else
        d->m_activeEngine->assignValueInDebugger(data, expr, value);
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
    d->m_cppEngine->notifyInferiorIll();
}

void QmlCppEngine::detachDebugger()
{
    d->m_qmlEngine->detachDebugger();
    d->m_cppEngine->detachDebugger();
}

void QmlCppEngine::executeStep()
{
//    TODO: stepping from qml -> cpp requires more thought
//    if (d->m_activeEngine == d->m_qmlEngine) {
//        QTC_CHECK(d->m_cppEngine->state() == InferiorRunOk);
//        if (d->m_cppEngine->setupQmlStep(true))
//            return; // Wait for callback to readyToExecuteQmlStep()
//    } else {
//        notifyInferiorRunRequested();
//        d->m_cppEngine->executeStep();
//    }

    notifyInferiorRunRequested();
    d->m_activeEngine->executeStep();
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
    d->m_cppEngine->requestInterruptInferior();
}

void QmlCppEngine::requestInterruptInferior()
{
    EDEBUG("\nMASTER REQUEST INTERRUPT INFERIOR");
    DebuggerEngine::requestInterruptInferior();
}

void QmlCppEngine::executeRunToLine(const ContextData &data)
{
    d->m_activeEngine->executeRunToLine(data);
}

void QmlCppEngine::executeRunToFunction(const QString &functionName)
{
    d->m_activeEngine->executeRunToFunction(functionName);
}

void QmlCppEngine::executeJumpToLine(const ContextData &data)
{
    d->m_activeEngine->executeJumpToLine(data);
}

void QmlCppEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    d->m_qmlEngine->executeDebuggerCommand(command, languages);
    d->m_cppEngine->executeDebuggerCommand(command, languages);
}

/////////////////////////////////////////////////////////

void QmlCppEngine::setupEngine()
{
    EDEBUG("\nMASTER SETUP ENGINE");
    setActiveEngine(d->m_cppEngine);
    d->m_qmlEngine->setupSlaveEngine();
    d->m_cppEngine->setupSlaveEngine();

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
    d->m_cppEngine->shutdownInferior();
}

void QmlCppEngine::shutdownEngine()
{
    EDEBUG("\nMASTER SHUTDOWN ENGINE");
    d->m_cppEngine->shutdownSlaveEngine();
    QmlJS::ConsoleManagerInterface *consoleManager = QmlJS::ConsoleManagerInterface::instance();
    if (consoleManager)
        consoleManager->setScriptEvaluator(0);
}

void QmlCppEngine::quitDebugger()
{
    EDEBUG("\nMASTER QUIT DEBUGGER");
    d->m_cppEngine->quitDebugger();
}

void QmlCppEngine::abortDebugger()
{
    EDEBUG("\nMASTER ABORT DEBUGGER");
    d->m_cppEngine->abortDebugger();
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
    DebuggerEngine *otherEngine = (slaveEngine == d->m_cppEngine)
         ? d->m_qmlEngine : d->m_cppEngine;

    QTC_CHECK(otherEngine != slaveEngine);

    if (debug) {
        EDEBUG("GOT SLAVE STATE: " << slaveEngine << newState);
        EDEBUG("  OTHER ENGINE: " << otherEngine << otherEngine->state());
        EDEBUG("  COMBINED ENGINE: " << this << state() << isDying());
    }

    // Idea is to follow the state of the cpp engine, except where we are stepping in QML.
    // That is, when the QmlEngine moves between InferiorStopOk, and InferiorRunOk, InferiorStopOk ...
    //
    // Accordingly, the 'active engine' is the cpp engine until the qml engine enters the
    // InferiorStopOk state. The cpp engine becomes the active one again as soon as it itself enters
    // the InferiorStopOk state.

    if (slaveEngine == d->m_cppEngine) {
        switch (newState) {
        case DebuggerNotReady: {
            // Can this ever happen?
            break;
        }
        case EngineSetupRequested: {
            // set by queueSetupEngine()
            QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
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
            // set by queueSetupInferior()
            QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
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
            notifyInferiorUnrunnable();
            break;
        }
        case InferiorRunRequested: {
            // might be set already by notifyInferiorRunRequested()
            QTC_ASSERT(state() == InferiorRunRequested
                       || state() == InferiorStopOk, qDebug() << state());
            if (state() != InferiorRunRequested)
                notifyInferiorRunRequested();
            break;
        }
        case InferiorRunOk: {
            QTC_ASSERT(state() == EngineRunRequested
                       || state() == InferiorRunRequested, qDebug() << state());
            if (state() == EngineRunRequested)
                notifyEngineRunAndInferiorRunOk();
            else if (state() == InferiorRunRequested)
                notifyInferiorRunOk();

            if (qmlEngine()->state() == InferiorStopOk) {
                // track qml engine again
                setState(InferiorStopRequested);
                notifyInferiorStopOk();
                setActiveEngine(d->m_qmlEngine);
            }
            break;
        }
        case InferiorRunFailed: {
            qmlEngine()->quitDebugger();
            notifyInferiorRunFailed();
            break;
        }
        case InferiorStopRequested: {
            if (d->m_activeEngine == cppEngine()) {
                // might be set by doInterruptInferior()
                QTC_ASSERT(state() == InferiorStopRequested
                           || state() == InferiorRunOk, qDebug() << state());
                if (state() == InferiorRunOk)
                    setState(InferiorStopRequested);
            } else {
                // we're debugging qml, but got an interrupt, or abort
                QTC_ASSERT(state() == InferiorRunOk
                          || state() == InferiorStopOk
                           || state() == InferiorRunRequested, qDebug() << state());

                if (state() == InferiorRunOk) {
                    setState(InferiorStopRequested);
                } else if (state() == InferiorStopOk) {
                    notifyInferiorRunRequested();
                    notifyInferiorRunOk();
                    setState(InferiorStopRequested);
                } else if (state() == InferiorRunRequested) {
                    notifyInferiorRunOk();
                    setState(InferiorStopRequested);
                }
                // now track cpp engine
                setActiveEngine(d->m_cppEngine);
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
                setActiveEngine(d->m_cppEngine);

                if (state() == InferiorStopRequested)
                    setState(InferiorStopOk);
                // otherwise we're probably inside notifyInferiorStopOk already
            } else {
                if (d->m_activeEngine != cppEngine()) {
                    showStatusMessage(tr("C++ debugger activated"));
                    setActiveEngine(d->m_cppEngine);
                }

                QTC_ASSERT(state() == InferiorStopRequested
                           || state() == InferiorRunRequested
                           || state() == EngineRunRequested
                           || state() == InferiorRunOk
                           || state() == InferiorStopOk, qDebug() << state());
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
                    break;
                }
            }
            break;
        }
        case InferiorStopFailed: {
            QTC_ASSERT(state() == InferiorStopRequested, qDebug() << state());
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
            // might be set by queueShutdownInferior() already
            QTC_ASSERT(state() == InferiorShutdownRequested
                       || state() == InferiorStopOk, qDebug() << state());
            if (state() == InferiorStopOk)
                setState(InferiorShutdownRequested);
            qmlEngine()->quitDebugger();
            break;
        }
        case InferiorShutdownFailed: {
            QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
            notifyInferiorShutdownFailed();
            break;
        }
        case InferiorShutdownOk: {
            if (state() == InferiorShutdownRequested)
                notifyInferiorShutdownOk();
            else {
                // we got InferiorExitOk before, but ignored it ...
                notifyInferiorExited();
            }
            break;
        }
        case EngineShutdownRequested: {
            // set by queueShutdownEngine()
            QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
            break;
        }
        case EngineShutdownFailed: {
            QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
            notifyEngineShutdownFailed();
            break;
        }
        case EngineShutdownOk: {
            QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
            notifyEngineShutdownOk();
            break;
        }
        case DebuggerFinished: {
            // set by queueFinishDebugger()
            QTC_ASSERT(state() == DebuggerFinished, qDebug() << state());
            break;
        }
        }
    } else {
        // QML engine state change
        if (newState == InferiorStopOk) {
            if (isDying()) {
                EDEBUG("... QML ENGINE STOPPED DURING SHUTDOWN ");

                // Just to make sure, we're shutting down anyway ...
                setActiveEngine(d->m_cppEngine);

                if (state() == InferiorStopRequested)
                    notifyInferiorStopOk();
                // otherwise we're probably inside notifyInferiorStopOk already
            } else {
                if (d->m_activeEngine != qmlEngine()) {
                    showStatusMessage(tr("QML debugger activated"));
                    setActiveEngine(d->m_qmlEngine);
                }

                QTC_ASSERT(state() == InferiorRunOk
                           || state() == InferiorStopRequested
                           || state() == InferiorShutdownRequested, qDebug() << state());

                if (state() == InferiorRunOk)
                    notifyInferiorSpontaneousStop();
                else if (state() == InferiorStopRequested)
                    notifyInferiorStopOk();
            }

        } else if (newState == InferiorRunOk) {
            if (d->m_activeEngine == qmlEngine()) {
                QTC_ASSERT(state() == InferiorRunRequested, qDebug() << state());
                notifyInferiorRunOk();
            }
        } else if (newState == EngineRunFailed) {
            if (d->m_cppEngine->targetState() != DebuggerFinished)
                d->m_cppEngine->quitDebugger();
        }
    }
}

void QmlCppEngine::notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    EDEBUG("MASTER REMOTE SETUP DONE");
    DebuggerEngine::notifyEngineRemoteSetupDone(gdbServerPort, qmlPort);

    cppEngine()->notifyEngineRemoteSetupDone(gdbServerPort, qmlPort);
    qmlEngine()->notifyEngineRemoteSetupDone(gdbServerPort, qmlPort);
}

void QmlCppEngine::notifyEngineRemoteSetupFailed(const QString &message)
{
    EDEBUG("MASTER REMOTE SETUP FAILED");
    DebuggerEngine::notifyEngineRemoteSetupFailed(message);

    cppEngine()->notifyEngineRemoteSetupFailed(message);
    qmlEngine()->notifyEngineRemoteSetupFailed(message);
}

void QmlCppEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    if (channel == AppOutput || channel == AppError || channel == AppStuff) {
        // message is from CppEngine, allow qml engine to process
        d->m_qmlEngine->filterApplicationMessage(msg, channel);
    }
    DebuggerEngine::showMessage(msg, channel, timeout);
}

void QmlCppEngine::resetLocation()
{
    if (d->m_qmlEngine)
        d->m_qmlEngine->resetLocation();
    if (d->m_cppEngine)
        d->m_cppEngine->resetLocation();

    DebuggerEngine::resetLocation();
}

DebuggerEngine *QmlCppEngine::cppEngine() const
{
    return d->m_cppEngine;
}

DebuggerEngine *QmlCppEngine::qmlEngine() const
{
    return d->m_qmlEngine;
}

void QmlCppEngine::setActiveEngine(DebuggerEngine *engine)
{
    d->m_activeEngine = engine;
    updateViews();
}

} // namespace Internal
} // namespace Debugger
