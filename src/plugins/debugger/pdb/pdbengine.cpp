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

#include "pdbengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/threaddata.h>

#include <debugger/breakhandler.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <utils/qtcassert.h>

#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVariant>

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
    //XSDEBUG("PdbEngine::executeDebuggerCommand:" << command);
    if (state() == DebuggerNotReady) {
        showMessage(_("PDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }
    QTC_ASSERT(m_proc.state() == QProcess::Running, notifyEngineIll());
    postDirectCommand(command.toLatin1());
}

void PdbEngine::postDirectCommand(const QByteArray &command)
{
    QTC_ASSERT(m_proc.state() == QProcess::Running, notifyEngineIll());
    showMessage(_(command), LogInput);
    m_proc.write(command + '\n');
}

void PdbEngine::runCommand(const DebuggerCommand &cmd)
{
    QTC_ASSERT(m_proc.state() == QProcess::Running, notifyEngineIll());
    QByteArray command = "qdebug('" + cmd.function + "',{" + cmd.args + "})";
    showMessage(_(command), LogInput);
    m_proc.write(command + '\n');
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

    QString python = pythonInterpreter();
    showMessage(_("STARTING PDB ") + python);

    connect(&m_proc, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
        this, &PdbEngine::handlePdbError);
    connect(&m_proc, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
        this, &PdbEngine::handlePdbFinished);
    connect(&m_proc, &QProcess::readyReadStandardOutput,
        this, &PdbEngine::readPdbStandardOutput);
    connect(&m_proc, &QProcess::readyReadStandardError,
        this, &PdbEngine::readPdbStandardError);

    m_proc.start(python, QStringList() << _("-i"));

    if (!m_proc.waitForStarted()) {
        const QString msg = tr("Unable to start pdb \"%1\": %2")
            .arg(pythonInterpreter(), m_proc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
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

    QString fileName = mainPythonFile();
    QFile scriptFile(fileName);
    if (!scriptFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        AsynchronousMessageBox::critical(tr("Python Error"),
            _("Cannot open script file %1:\n%2").
               arg(fileName, scriptFile.errorString()));
        notifyInferiorSetupFailed();
        return;
    }
    notifyInferiorSetupOk();
}

QString PdbEngine::mainPythonFile() const
{
    return QFileInfo(runParameters().processArgs).absoluteFilePath();
}

QString PdbEngine::pythonInterpreter() const
{
    return runParameters().executable;
}

void PdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    showStatusMessage(tr("Running requested..."), 5000);
    QByteArray bridge = ICore::resourcePath().toUtf8() + "/debugger/pdbbridge.py";
    QByteArray pdb = "/usr/bin/pdb";
    if (pythonInterpreter().endsWith(QLatin1Char('3')))
        pdb += '3';
    postDirectCommand("import sys");
    postDirectCommand("sys.argv.append('" + mainPythonFile().toLocal8Bit() + "')");
    postDirectCommand("exec(open('" + pdb + "').read())");
    postDirectCommand("exec(open('" + bridge + "').read())");
    attemptBreakpointSynchronization();
    notifyEngineRunAndInferiorStopOk();
    continueInferior();
}

void PdbEngine::interruptInferior()
{
    notifyInferiorStopOk();
}

void PdbEngine::executeStep()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("step");
}

void PdbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("step");
}

void PdbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("return");
}

void PdbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("next");
}

void PdbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postDirectCommand("next");
}

void PdbEngine::continueInferior()
{
    resetLocation();
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
    resetLocation();
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    int oldIndex = handler->currentIndex();

    //if (frameIndex == handler->stackSize()) {
    //    reloadFullStack();
    //    return;
    //}

    QTC_ASSERT(frameIndex < handler->stackSize(), return);

    if (oldIndex != frameIndex) {
        // Assuming the command always succeeds this saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        handler->setCurrentIndex(frameIndex);
        //postDirectCommand("-stack-select-frame " + QByteArray::number(frameIndex));
    }
    gotoLocation(handler->currentFrame());
}

void PdbEngine::selectThread(ThreadId threadId)
{
    Q_UNUSED(threadId)
}

bool PdbEngine::acceptsBreakpoint(Breakpoint bp) const
{
    const QString fileName = bp.fileName();
    return fileName.endsWith(QLatin1String(".py"));
}

void PdbEngine::insertBreakpoint(Breakpoint bp)
{
    QTC_CHECK(bp.state() == BreakpointInsertRequested);
    bp.notifyBreakpointInsertProceeding();

    QByteArray loc;
    if (bp.type() == BreakpointByFunction)
        loc = bp.functionName().toLatin1();
    else
        loc = bp.fileName().toLocal8Bit() + ':'
         + QByteArray::number(bp.lineNumber());

    postDirectCommand("break " + loc);
}

void PdbEngine::removeBreakpoint(Breakpoint bp)
{
    QTC_CHECK(bp.state() == BreakpointRemoveRequested);
    bp.notifyBreakpointRemoveProceeding();
    BreakpointResponse br = bp.response();
    showMessage(_("DELETING BP %1 IN %2").arg(br.id.toString()).arg(bp.fileName()));
    postDirectCommand("clear " + br.id.toByteArray());
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
    runCommand("listModules");
}

void PdbEngine::refreshModules(const GdbMi &modules)
{
    ModulesHandler *handler = modulesHandler();
    handler->beginUpdateAll();
    foreach (const GdbMi &item, modules.children()) {
        Module module;
        module.moduleName = _(item["name"].data());
        QString path = _(item["value"].data());
        int pos = path.indexOf(_("' from '"));
        if (pos != -1) {
            path = path.mid(pos + 8);
            if (path.size() >= 2)
                path.chop(2);
        } else if (path.startsWith(_("<module '"))
                && path.endsWith(_("' (built-in)>"))) {
            path = _("(builtin)");
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

void PdbEngine::refreshSymbols(const GdbMi &symbols)
{
    QString moduleName = symbols["module"].toUtf8();
    Symbols syms;
    foreach (const GdbMi &item, symbols["symbols"].children()) {
        Symbol symbol;
        symbol.name = item["name"].toUtf8();
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
    QByteArray exp = expression.toUtf8();
    postDirectCommand("global " + exp + ';' + exp + "=" + value.toString().toUtf8());
    updateLocals();
}

void PdbEngine::updateItem(const QByteArray &iname)
{
    Q_UNUSED(iname);
    updateAll();
}

void PdbEngine::handlePdbError(QProcess::ProcessError error)
{
    qDebug() << "HANDLE PDB ERROR";
    showMessage(_("HANDLE PDB ERROR"));
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
                .arg(pythonInterpreter());
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
    qDebug() << "PDB FINISHED";
    showMessage(_("PDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    notifyEngineSpontaneousShutdown();
}

void PdbEngine::readPdbStandardError()
{
    QByteArray err = m_proc.readAllStandardError();
    qDebug() << "\nPDB STDERR" << err;
    //qWarning() << "Unexpected pdb stderr:" << err;
    //showMessage(_("Unexpected pdb stderr: " + err));
    //handleOutput(err);
}

void PdbEngine::readPdbStandardOutput()
{
    QByteArray out = m_proc.readAllStandardOutput();
    qDebug() << "\nPDB STDOUT" << out;
    handleOutput(out);
}

void PdbEngine::handleOutput(const QByteArray &data)
{
    //qDebug() << "READ: " << data;
    m_inbuffer.append(data);
    qDebug() << "BUFFER FROM: '" << m_inbuffer << '\'';
    while (true) {
        int pos = m_inbuffer.indexOf("(Pdb)");
        if (pos == -1)
            pos = m_inbuffer.indexOf(">>>");
        if (pos == -1)
            break;
        QByteArray response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 6);
        handleOutput2(response);
    }
    qDebug() << "BUFFER LEFT: '" << m_inbuffer << '\'';
    //m_inbuffer.clear();
}

void PdbEngine::handleOutput2(const QByteArray &data)
{
    QByteArray lineContext;
    foreach (QByteArray line, data.split('\n')) {

        GdbMi item;
        item.fromString(line);

        showMessage(_("LINE: " + line));

        if (line.startsWith("stack={")) {
            refreshStack(item);
        } else if (line.startsWith("data={")) {
            refreshLocals(item);
        } else if (line.startsWith("modules=[")) {
            refreshModules(item);
        } else if (line.startsWith("symbols={")) {
            refreshSymbols(item);
        } else if (line.startsWith("Breakpoint")) {
            int pos1 = line.indexOf(" at ");
            QTC_ASSERT(pos1 != -1, continue);
            QByteArray bpnr = line.mid(11, pos1 - 11);
            int pos2 = line.lastIndexOf(':');
            QTC_ASSERT(pos2 != -1, continue);
            QByteArray fileName = line.mid(pos1 + 4, pos2 - pos1 - 4);
            QByteArray lineNumber = line.mid(pos2 + 1);
            BreakpointResponse br;
            br.id = BreakpointResponseId(bpnr);
            br.fileName = _(fileName);
            br.lineNumber = lineNumber.toInt();
            Breakpoint bp = breakHandler()->findBreakpointByFileAndLine(br.fileName, br.lineNumber, false);
            if (bp.isValid()) {
                bp.setResponse(br);
                QTC_CHECK(!bp.needsChange());
                bp.notifyBreakpointInsertOk();
            }
        } else {
            if (line.startsWith("> /")) {
                lineContext = line;
                int pos1 = line.indexOf('(');
                int pos2 = line.indexOf(')', pos1);
                if (pos1 != -1 && pos2 != -1) {
                    int lineNumber = line.mid(pos1 + 1, pos2 - pos1 - 1).toInt();
                    QByteArray fileName = line.mid(2, pos1 - 2);
                    qDebug() << " " << pos1 << pos2 << lineNumber << fileName
                             << line.mid(pos1 + 1, pos2 - pos1 - 1);
                    StackFrame frame;
                    frame.file = _(fileName);
                    frame.line = lineNumber;
                    if (state() == InferiorRunOk) {
                        showMessage(QString::fromLatin1("STOPPED AT: %1:%2").arg(frame.file).arg(frame.line));
                        gotoLocation(frame);
                        notifyInferiorSpontaneousStop();
                        updateAll();
                        continue;
                    }
                }
            }
            showMessage(_(" #### ... UNHANDLED"));
        }
    }
}

/*
void PdbEngine::handleResponse(const QByteArray &response0)
{
    QByteArray response = response0;
    qDebug() << "RESPONSE: '" << response << "'";
    if (response.startsWith("--Call--")) {
        qDebug() << "SKIPPING '--Call--' MARKER";
        response = response.mid(9);
    }
    if (response.startsWith("--Return--")) {
        qDebug() << "SKIPPING '--Return--' MARKER";
        response = response.mid(11);
    }
}
*/

void PdbEngine::refreshLocals(const GdbMi &vars)
{
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();

    foreach (const GdbMi &child, vars.children())
        handler->insertItem(new WatchItem(child));

    handler->notifyUpdateFinished();

    DebuggerToolTipManager::updateEngine(this);
}

void PdbEngine::refreshStack(const GdbMi &stack)
{
    StackHandler *handler = stackHandler();
    StackFrames frames;
    foreach (const GdbMi &item, stack["frames"].children()) {
        StackFrame frame;
        frame.level = item["level"].toInt();
        frame.file = item["file"].toUtf8();
        frame.function = item["func"].toUtf8();
        frame.from = item["func"].toUtf8();
        frame.line = item["line"].toInt();
        frame.address = item["addr"].toAddress();
        GdbMi usable = item["usable"];
        if (usable.isValid())
            frame.usable = usable.data().toInt();
        else
            frame.usable = QFileInfo(frame.file).isReadable();
        if (item["language"].data() == "js"
                || frame.file.endsWith(QLatin1String(".js"))
                || frame.file.endsWith(QLatin1String(".qml"))) {
            frame.language = QmlLanguage;
            frame.fixQmlFrame(runParameters());
        }
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
    runCommand("stackListFrames");
    updateLocals();
}

void PdbEngine::updateLocals()
{
    DebuggerCommand cmd("updateData");
    cmd.arg("nativeMixed", isNativeMixedActive());
    watchHandler()->appendFormatRequests(&cmd);

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", boolSetting(UseDebuggingHelpers));

    cmd.beginList("watchers");

    // Watchers
    QHashIterator<QByteArray, int> it(WatchHandler::watcherNames());
    while (it.hasNext()) {
        it.next();
        cmd.beginGroup();
        cmd.arg("iname", "watch." + QByteArray::number(it.value()));
        cmd.arg("exp", it.key().toHex());
        cmd.endGroup();
    }

    // Tooltips
    DebuggerToolTipContexts toolTips = DebuggerToolTipManager::pendingTooltips(this);
    foreach (const DebuggerToolTipContext &p, toolTips) {
        cmd.beginGroup();
        cmd.arg("iname", p.iname);
        cmd.arg("exp", p.expression.toLatin1().toHex());
        cmd.endGroup();
    }

    cmd.endList();

    //cmd.arg("resultvarname", m_resultVarName);
    //m_lastDebuggableCommand = cmd;
    //m_lastDebuggableCommand.args.replace("\"passexceptions\":0", "\"passexceptions\":1");

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
