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
#include <debugger/debuggerstartparameters.h>
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

PdbEngine::PdbEngine(const DebuggerRunParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("PdbEngine"));
}

void PdbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (!(languages & CppLanguage))
        return;
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
    notifyInferiorShutdownOk();
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
    QString bridge = ICore::resourcePath() + QLatin1String("/debugger/pdbbridge.py");

    connect(&m_proc, &QProcess::errorOccurred, this, &PdbEngine::handlePdbError);
    connect(&m_proc, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
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

    QStringList args = { bridge, scriptFile.fileName() };
    args.append(Utils::QtcProcess::splitArgs(runParameters().inferior.workingDirectory));
    showMessage("STARTING " + m_interpreter + QLatin1Char(' ') + args.join(QLatin1Char(' ')));
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

void PdbEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    notifyInferiorSetupOk();
}

void PdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    showStatusMessage(tr("Running requested..."), 5000);
    attemptBreakpointSynchronization();
    notifyEngineRunAndInferiorStopOk();
    updateAll();
}

void PdbEngine::interruptInferior()
{
    QString error;
    interruptProcess(m_proc.processId(), GdbEngineType, &error);
}

void PdbEngine::executeStep()
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("step");
}

void PdbEngine::executeStepI()
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

void PdbEngine::executeNext()
{
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("next");
}

void PdbEngine::executeNextI()
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

void PdbEngine::selectThread(ThreadId threadId)
{
    Q_UNUSED(threadId)
}

bool PdbEngine::acceptsBreakpoint(Breakpoint bp) const
{
    const QString fileName = bp.fileName();
    return fileName.endsWith(".py");
}

void PdbEngine::insertBreakpoint(Breakpoint bp)
{
    QTC_CHECK(bp.state() == BreakpointInsertRequested);
    bp.notifyBreakpointInsertProceeding();

    QString loc;
    if (bp.type() == BreakpointByFunction)
        loc = bp.functionName();
    else
        loc = bp.fileName() + ':' + QString::number(bp.lineNumber());

    postDirectCommand("break " + loc);
}

void PdbEngine::removeBreakpoint(Breakpoint bp)
{
    QTC_CHECK(bp.state() == BreakpointRemoveRequested);
    bp.notifyBreakpointRemoveProceeding();
    BreakpointResponse br = bp.response();
    showMessage(QString("DELETING BP %1 IN %2").arg(br.id.toString()).arg(bp.fileName()));
    postDirectCommand("clear " + br.id.toString());
    // Pretend it succeeds without waiting for response.
    bp.notifyBreakpointRemoveOk();
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
    foreach (const GdbMi &item, modules.children()) {
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
    foreach (const GdbMi &item, symbols["symbols"].children()) {
        Symbol symbol;
        symbol.name = item["name"].data();
        syms.append(symbol);
    }
    Internal::showModuleSymbols(moduleName, syms);
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
    Q_UNUSED(iname);
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
            return tr("An unknown error in the Pdb process occurred.") + QLatin1Char(' ');
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
    foreach (QString line, data.split('\n')) {

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
            int pos1 = line.indexOf(" at ");
            QTC_ASSERT(pos1 != -1, continue);
            QString bpnr = line.mid(11, pos1 - 11);
            int pos2 = line.lastIndexOf(':');
            QTC_ASSERT(pos2 != -1, continue);
            BreakpointResponse br;
            br.id = BreakpointResponseId(bpnr);
            br.fileName = line.mid(pos1 + 4, pos2 - pos1 - 4);
            br.lineNumber = line.mid(pos2 + 1).toInt();
            Breakpoint bp = breakHandler()->findBreakpointByFileAndLine(br.fileName, br.lineNumber, false);
            if (bp.isValid()) {
                bp.setResponse(br);
                QTC_CHECK(!bp.needsChange());
                bp.notifyBreakpointInsertOk();
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

    DebuggerToolTipManager::updateEngine(this);
}

void PdbEngine::refreshStack(const GdbMi &stack)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    foreach (const GdbMi &item, stack["frames"].children()) {
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

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
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

DebuggerEngine *createPdbEngine(const DebuggerRunParameters &startParameters)
{
    return new PdbEngine(startParameters);
}

} // namespace Internal
} // namespace Debugger
