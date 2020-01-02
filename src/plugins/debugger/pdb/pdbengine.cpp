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

#include "pdbengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/threaddata.h>

#include <debugger/breakhandler.h>
#include <debugger/moduleshandler.h>
#include <debugger/procinterrupt.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVariant>
#include <QJsonArray>

using namespace Core;

namespace Debugger {
namespace Internal {

PdbEngine::PdbEngine()
{
    setObjectName("PdbEngine");
    setDebuggerName("PDB");
}

void PdbEngine::executeDebuggerCommand(const QString &command)
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    if (state() == DebuggerNotReady) {
        showMessage("PDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: " + command);
        return;
    }
    QTC_ASSERT(m_proc.state() == QProcess::Running, notifyEngineIll());
    postDirectCommand(command);
}

void PdbEngine::postDirectCommand(const QString &command)
{
    QTC_ASSERT(m_proc.state() == QProcess::Running, notifyEngineIll());
    showMessage(command, LogInput);
    m_proc.write(command.toUtf8() + '\n');
}

void PdbEngine::runCommand(const DebuggerCommand &cmd)
{
    if (state() == EngineSetupRequested) { // cmd has been triggered too early
        showMessage("IGNORED COMMAND: " + cmd.function);
        return;
    }
    QTC_ASSERT(m_proc.state() == QProcess::Running, notifyEngineIll());
    QString command = "qdebug('" + cmd.function + "'," + cmd.argsToPython() + ")";
    showMessage(command, LogInput);
    m_proc.write(command.toUtf8() + '\n');
}

void PdbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    notifyInferiorShutdownFinished();
}

void PdbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_proc.kill();
}

void PdbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_interpreter = runParameters().interpreter;
    QString bridge = ICore::resourcePath() + "/debugger/pdbbridge.py";

    connect(&m_proc, &QProcess::errorOccurred, this, &PdbEngine::handlePdbError);
    connect(&m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &PdbEngine::handlePdbFinished);
    connect(&m_proc, &QProcess::readyReadStandardOutput,
        this, &PdbEngine::readPdbStandardOutput);
    connect(&m_proc, &QProcess::readyReadStandardError,
        this, &PdbEngine::readPdbStandardError);

    QFile scriptFile(runParameters().mainScript);
    if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        AsynchronousMessageBox::critical(tr("Python Error"),
            QString("Cannot open script file %1:\n%2").
               arg(scriptFile.fileName(), scriptFile.errorString()));
        notifyEngineSetupFailed();
    }

    QStringList args = {bridge, scriptFile.fileName()};
    args.append(Utils::QtcProcess::splitArgs(runParameters().inferior.workingDirectory));
    showMessage("STARTING " + m_interpreter + ' ' + args.join(' '));
    m_proc.setEnvironment(runParameters().debugger.environment.toStringList());
    m_proc.start(m_interpreter, args);

    if (!m_proc.waitForStarted()) {
        const QString msg = tr("Unable to start pdb \"%1\": %2")
            .arg(m_interpreter, m_proc.errorString());
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        if (!msg.isEmpty())
            ICore::showWarningWithOptions(tr("Adapter start failed"), msg);
        notifyEngineSetupFailed();
        return;
    }
    notifyEngineSetupOk();
}

void PdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    showStatusMessage(tr("Running requested..."), 5000);
    BreakpointManager::claimBreakpointsForEngine(this);
    notifyEngineRunAndInferiorStopOk();
    updateAll();
}

void PdbEngine::interruptInferior()
{
    QString error;
    interruptProcess(m_proc.processId(), GdbEngineType, &error);
}

void PdbEngine::executeStepIn(bool)
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("step");
}

void PdbEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("return");
}

void PdbEngine::executeStepOver(bool)
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("next");
}

void PdbEngine::continueInferior()
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    // Callback will be triggered e.g. when breakpoint is hit.
    postDirectCommand("continue");
}

void PdbEngine::executeRunToLine(const ContextData &data)
{
    Q_UNUSED(data)
    QTC_CHECK("FIXME:  PdbEngine::runToLineExec()" && false);
}

void PdbEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    QTC_CHECK("FIXME:  PdbEngine::runToFunctionExec()" && false);
}

void PdbEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    QTC_CHECK("FIXME:  PdbEngine::jumpToLineExec()" && false);
}

void PdbEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoLocation(handler->currentFrame());
    updateLocals();
}

void PdbEngine::selectThread(const Thread &thread)
{
    Q_UNUSED(thread)
}

bool PdbEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    return bp.fileName.endsWith(".py");
}

void PdbEngine::insertBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointInsertionRequested);
    notifyBreakpointInsertProceeding(bp);

    QString loc;
    const BreakpointParameters &params = bp->requestedParameters();
    if (params.type  == BreakpointByFunction)
        loc = params.functionName;
    else
        loc = params.fileName.toString() + ':' + QString::number(params.lineNumber);

    postDirectCommand("break " + loc);
}

void PdbEngine::updateBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    const BreakpointState state = bp->state();
    if (QTC_GUARD(state == BreakpointUpdateRequested))
        notifyBreakpointChangeProceeding(bp);
    if (bp->responseId().isEmpty()) // FIXME postpone update somehow (QTimer::singleShot?)
        return;

    // FIXME figure out what needs to be changed (there might be more than enabled state)
    const BreakpointParameters &requested = bp->requestedParameters();
    if (requested.enabled != bp->isEnabled()) {
        if (bp->isEnabled())
            postDirectCommand("disable " + bp->responseId());
        else
            postDirectCommand("enable " + bp->responseId());
        bp->setEnabled(!bp->isEnabled());
    }
    // Pretend it succeeds without waiting for response.
    notifyBreakpointChangeOk(bp);
}

void PdbEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_CHECK(bp->state() == BreakpointRemoveRequested);
    notifyBreakpointRemoveProceeding(bp);
    if (bp->responseId().isEmpty()) {
        notifyBreakpointRemoveFailed(bp);
        return;
    }
    showMessage(QString("DELETING BP %1 IN %2")
                .arg(bp->responseId()).arg(bp->fileName().toUserOutput()));
    postDirectCommand("clear " + bp->responseId());
    // Pretend it succeeds without waiting for response.
    notifyBreakpointRemoveOk(bp);
}

void PdbEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void PdbEngine::loadAllSymbols()
{
}

void PdbEngine::reloadModules()
{
    runCommand({"listModules"});
}

void PdbEngine::refreshModules(const GdbMi &modules)
{
    ModulesHandler *handler = modulesHandler();
    handler->beginUpdateAll();
    for (const GdbMi &item : modules) {
        Module module;
        module.moduleName = item["name"].data();
        QString path = item["value"].data();
        int pos = path.indexOf("' from '");
        if (pos != -1) {
            path = path.mid(pos + 8);
            if (path.size() >= 2)
                path.chop(2);
        } else if (path.startsWith("<module '")
                && path.endsWith("' (built-in)>")) {
            path = "(builtin)";
        }
        module.modulePath = path;
        handler->updateModule(module);
    }
    handler->endUpdateAll();
}

void PdbEngine::requestModuleSymbols(const QString &moduleName)
{
    DebuggerCommand cmd("listSymbols");
    cmd.arg("module", moduleName);
    runCommand(cmd);
}

void PdbEngine::refreshState(const GdbMi &reportedState)
{
    QString newState = reportedState.data();
    if (newState == "stopped") {
        notifyInferiorSpontaneousStop();
        updateAll();
    } else if (newState == "inferiorexited") {
        notifyInferiorExited();
    }
}

void PdbEngine::refreshLocation(const GdbMi &reportedLocation)
{
    StackFrame frame;
    frame.file = reportedLocation["file"].data();
    frame.line = reportedLocation["line"].toInt();
    frame.usable = QFileInfo(frame.file).isReadable();
    if (state() == InferiorRunOk) {
        showMessage(QString("STOPPED AT: %1:%2").arg(frame.file).arg(frame.line));
        gotoLocation(frame);
        notifyInferiorSpontaneousStop();
        updateAll();
    }
}

void PdbEngine::refreshSymbols(const GdbMi &symbols)
{
    QString moduleName = symbols["module"].data();
    Symbols syms;
    for (const GdbMi &item : symbols["symbols"]) {
        Symbol symbol;
        symbol.name = item["name"].data();
        syms.append(symbol);
    }
    showModuleSymbols(moduleName, syms);
}

bool PdbEngine::canHandleToolTip(const DebuggerToolTipContext &) const
{
    return state() == InferiorStopOk;
}

void PdbEngine::assignValueInDebugger(WatchItem *, const QString &expression, const QVariant &value)
{
    //DebuggerCommand cmd("assignValue");
    //cmd.arg("expression", expression);
    //cmd.arg("value", value.toString());
    //runCommand(cmd);
    postDirectCommand("global " + expression + ';' + expression + "=" + value.toString());
    updateLocals();
}

void PdbEngine::updateItem(const QString &iname)
{
    Q_UNUSED(iname)
    updateAll();
}

void PdbEngine::handlePdbError(QProcess::ProcessError error)
{
    showMessage("HANDLE PDB ERROR");
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        m_proc.kill();
        AsynchronousMessageBox::critical(tr("Pdb I/O Error"), errorMessage(error));
        break;
    }
}

QString PdbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Pdb process failed to start. Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(m_interpreter);
        case QProcess::Crashed:
            return tr("The Pdb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Pdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Pdb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Pdb process occurred.") + ' ';
    }
}

void PdbEngine::handlePdbFinished(int code, QProcess::ExitStatus type)
{
    showMessage(QString("PDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    notifyEngineSpontaneousShutdown();
}

void PdbEngine::readPdbStandardError()
{
    QString err = QString::fromUtf8(m_proc.readAllStandardError());
    //qWarning() << "Unexpected pdb stderr:" << err;
    showMessage("Unexpected pdb stderr: " + err);
    //handleOutput(err);
}

void PdbEngine::readPdbStandardOutput()
{
    QString out = QString::fromUtf8(m_proc.readAllStandardOutput());
    handleOutput(out);
}

void PdbEngine::handleOutput(const QString &data)
{
    m_inbuffer.append(data);
    while (true) {
        int pos = m_inbuffer.indexOf('\n');
        if (pos == -1)
            break;
        QString response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 1);
        handleOutput2(response);
    }
}

void PdbEngine::handleOutput2(const QString &data)
{
    const QStringList lines = data.split('\n');
    for (const QString &line : lines) {
        GdbMi item;
        item.fromString(line);

        showMessage(line, LogOutput);

        if (line.startsWith("stack={")) {
            refreshStack(item);
        } else if (line.startsWith("data={")) {
            refreshLocals(item);
        } else if (line.startsWith("modules=[")) {
            refreshModules(item);
        } else if (line.startsWith("symbols={")) {
            refreshSymbols(item);
        } else if (line.startsWith("location={")) {
            refreshLocation(item);
        } else if (line.startsWith("state=")) {
            refreshState(item);
        } else if (line.startsWith("Breakpoint")) {
            const int pos1 = line.indexOf(" at ");
            QTC_ASSERT(pos1 != -1, continue);
            const QString bpnr = line.mid(11, pos1 - 11);
            const int pos2 = line.lastIndexOf(':');
            QTC_ASSERT(pos2 != -1, continue);
            const Utils::FilePath fileName = Utils::FilePath::fromString(
                line.mid(pos1 + 4, pos2 - pos1 - 4));
            const int lineNumber = line.midRef(pos2 + 1).toInt();
            const Breakpoint bp = Utils::findOrDefault(breakHandler()->breakpoints(), [&](const Breakpoint &bp) {
                return bp->parameters().isLocatedAt(fileName, lineNumber, bp->markerFileName())
                    || bp->requestedParameters().isLocatedAt(fileName, lineNumber, bp->markerFileName());
            });
            QTC_ASSERT(bp, continue);
            bp->setResponseId(bpnr);
            bp->setFileName(fileName);
            bp->setLineNumber(lineNumber);
            bp->adjustMarker();
            bp->setPending(false);
            notifyBreakpointInsertOk(bp);
            if (bp->needsChange()) {
                bp->gotoState(BreakpointUpdateRequested, BreakpointInserted);
                updateBreakpoint(bp);
//            QTC_CHECK(!bp->needsChange());
            }
        }
    }
}

void PdbEngine::refreshLocals(const GdbMi &vars)
{
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();
    handler->insertItems(vars);
    handler->notifyUpdateFinished();

    updateToolTips();
}

void PdbEngine::refreshStack(const GdbMi &stack)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    for (const GdbMi &item : stack["frames"]) {
        StackFrame frame;
        frame.level = item["level"].data();
        frame.file = item["file"].data();
        frame.function = item["function"].data();
        frame.module = item["function"].data();
        frame.line = item["line"].toInt();
        frame.address = item["address"].toAddress();
        GdbMi usable = item["usable"];
        if (usable.isValid())
            frame.usable = usable.data().toInt();
        else
            frame.usable = QFileInfo(frame.file).isReadable();
        frames.append(frame);
    }
    bool canExpand = stack["hasmore"].toInt();
    //action(ExpandStack)->setEnabled(canExpand);
    handler->setFrames(frames, canExpand);

    int index = stackHandler()->firstUsableIndex();
    handler->setCurrentIndex(index);
    if (index >= 0 && index < handler->stackSize())
        gotoLocation(handler->frameAt(index));
}

void PdbEngine::updateAll()
{
    runCommand({"stackListFrames"});
    updateLocals();
}

void PdbEngine::updateLocals()
{
    DebuggerCommand cmd("updateData");
    cmd.arg("nativeMixed", isNativeMixedActive());
    watchHandler()->appendFormatRequests(&cmd);
    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

    const static bool alwaysVerbose = qEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE");
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", boolSetting(UseDebuggingHelpers));

    //cmd.arg("resultvarname", m_resultVarName);
    //m_lastDebuggableCommand = cmd;
    //m_lastDebuggableCommand.args.replace("\"passexceptions\":0", "\"passexceptions\":1");
    cmd.arg("frame", stackHandler()->currentIndex());

    watchHandler()->notifyUpdateStarted();
    runCommand(cmd);
}

bool PdbEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability
              | BreakConditionCapability
              | ShowModuleSymbolsCapability);
}

DebuggerEngine *createPdbEngine()
{
    return new PdbEngine;
}

} // namespace Internal
} // namespace Debugger
