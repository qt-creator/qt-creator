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

#include <debugger/breakhandler.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <utils/qtcassert.h>

#include <texteditor/texteditor.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVariant>

#include <QApplication>
#include <QToolTip>


#define DEBUG_SCRIPT 1
#if DEBUG_SCRIPT
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s


#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PdbEngine
//
///////////////////////////////////////////////////////////////////////

PdbEngine::PdbEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("PdbEngine"));
}

PdbEngine::~PdbEngine()
{}

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
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    postCommand(command.toLatin1(), CB(handleExecuteDebuggerCommand));
}

void PdbEngine::handleExecuteDebuggerCommand(const DebuggerResponse &response)
{
    Q_UNUSED(response);
}

void PdbEngine::postDirectCommand(const QByteArray &command)
{
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    showMessage(_(command), LogInput);
    m_pdbProc.write(command + '\n');
}

void PdbEngine::postCommand(const QByteArray &command, DebuggerCommand::Callback callback)
{
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    DebuggerCommand cmd;
    cmd.function = command;
    cmd.callback = callback;
    m_commands.enqueue(cmd);
    qDebug() << "ENQUEUE: " << cmd.function;
    showMessage(_(cmd.function), LogInput);
    m_pdbProc.write(cmd.function + '\n');
}

void PdbEngine::runCommand(const DebuggerCommand &cmd)
{
    QTC_ASSERT(m_pdbProc.state() == QProcess::Running, notifyEngineIll());
    QByteArray command = "theDumper." + cmd.function + "({" + cmd.args + "})";
    showMessage(_(command), LogInput);
    m_pdbProc.write(command + '\n');
}

void PdbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    notifyInferiorShutdownOk();
}

void PdbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_pdbProc.kill();
}

void PdbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_pdb = _("python");
    showMessage(_("STARTING PDB ") + m_pdb);

    connect(&m_pdbProc, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
        this, &PdbEngine::handlePdbError);
    connect(&m_pdbProc, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
        this, &PdbEngine::handlePdbFinished);
    connect(&m_pdbProc, &QProcess::readyReadStandardOutput,
        this, &PdbEngine::readPdbStandardOutput);
    connect(&m_pdbProc, &QProcess::readyReadStandardError,
        this, &PdbEngine::readPdbStandardError);

    connect(this, &PdbEngine::outputReady,
        this, &PdbEngine::handleOutput2, Qt::QueuedConnection);

    // We will stop immediately, so setup a proper callback.
    DebuggerCommand cmd;
    cmd.callback = CB(handleFirstCommand);
    m_commands.enqueue(cmd);

    m_pdbProc.start(m_pdb, QStringList() << _("-i"));

    if (!m_pdbProc.waitForStarted()) {
        const QString msg = tr("Unable to start pdb \"%1\": %2")
            .arg(m_pdb, m_pdbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty()) {
            const QString title = tr("Adapter start failed");
            Core::ICore::showWarningWithOptions(title, msg);
        }
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
        Core::AsynchronousMessageBox::critical(tr("Python Error"),
            _("Cannot open script file %1:\n%2").
               arg(fileName, scriptFile.errorString()));
        notifyInferiorSetupFailed();
        return;
    }
    notifyInferiorSetupOk();
}

QString PdbEngine::mainPythonFile() const
{
    return QFileInfo(startParameters().processArgs).absoluteFilePath();
}

void PdbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    showStatusMessage(tr("Running requested..."), 5000);
    const QByteArray dumperSourcePath =
        Core::ICore::resourcePath().toLocal8Bit() + "/debugger/";
    postDirectCommand("import sys");
    postDirectCommand("sys.argv.append('" + mainPythonFile().toLocal8Bit() + "')");
    postDirectCommand("execfile('/usr/bin/pdb')");
    postDirectCommand("execfile('" + dumperSourcePath + "pdbbridge.py')");
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
    postCommand("step", CB(handleUpdateAll));
}

void PdbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("step", CB(handleUpdateAll));
}

void PdbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("finish", CB(handleUpdateAll));
}

void PdbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("next", CB(handleUpdateAll));
}

void PdbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("next", CB(handleUpdateAll));
}

void PdbEngine::continueInferior()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    // Callback will be triggered e.g. when breakpoint is hit.
    postCommand("continue", CB(handleUpdateAll));
}

void PdbEngine::executeRunToLine(const ContextData &data)
{
    Q_UNUSED(data)
    SDEBUG("FIXME:  PdbEngine::runToLineExec()");
}

void PdbEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  PdbEngine::runToFunctionExec()");
}

void PdbEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    XSDEBUG("FIXME:  PdbEngine::jumpToLineExec()");
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
        //postCommand("-stack-select-frame " + QByteArray::number(frameIndex),
        //    CB(handleStackSelectFrame));
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

    postCommand("break " + loc, [this, bp](const DebuggerResponse &r) { handleBreakInsert(r, bp); });
}

void PdbEngine::handleBreakInsert(const DebuggerResponse &response, Breakpoint bp)
{
    //qDebug() << "BP RESPONSE: " << response.data;
    // "Breakpoint 1 at /pdb/math.py:10"
    QTC_ASSERT(response.logStreamOutput.startsWith("Breakpoint "), return);
    int pos1 = response.logStreamOutput.indexOf(" at ");
    QTC_ASSERT(pos1 != -1, return);
    QByteArray bpnr = response.logStreamOutput.mid(11, pos1 - 11);
    int pos2 = response.logStreamOutput.lastIndexOf(':');
    QByteArray file = response.logStreamOutput.mid(pos1 + 4, pos2 - pos1 - 4);
    QByteArray line = response.logStreamOutput.mid(pos2 + 1);
    BreakpointResponse br;
    br.id = BreakpointResponseId(bpnr);
    br.fileName = _(file);
    br.lineNumber = line.toInt();
    if (!bp.isValid())
        bp = breakHandler()->findBreakpointByFileAndLine(br.fileName, br.lineNumber, false);
    bp.setResponse(br);
    QTC_CHECK(!bp.needsChange());
    bp.notifyBreakpointInsertOk();
}

void PdbEngine::removeBreakpoint(Breakpoint bp)
{
    QTC_CHECK(bp.state() == BreakpointRemoveRequested);
    bp.notifyBreakpointRemoveProceeding();
    BreakpointResponse br = bp.response();
    showMessage(_("DELETING BP %1 IN %2").arg(br.id.toString()).arg(bp.fileName()));
    postCommand("clear " + br.id.toByteArray());
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
    //postCommand("qdebug('listmodules')", CB(handleListModules));
}

void PdbEngine::handleListModules(const DebuggerResponse &response)
{
    GdbMi out;
    out.fromString(response.logStreamOutput.trimmed());
    ModulesHandler *handler = modulesHandler();
    handler->beginUpdateAll();
    foreach (const GdbMi &item, out.children()) {
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
    postCommand("qdebug('listsymbols','" + moduleName.toLatin1() + "')",
                [this, moduleName](const DebuggerResponse &r) { handleListSymbols(r, moduleName); });
}

void PdbEngine::handleListSymbols(const DebuggerResponse &response, const QString &moduleName)
{
    GdbMi out;
    out.fromString(response.logStreamOutput.trimmed());
    Symbols symbols;
    foreach (const GdbMi &item, out.children()) {
        Symbol symbol;
        symbol.name = _(item["name"].data());
        symbols.append(symbol);
    }
    Internal::showModuleSymbols(moduleName, symbols);
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////


bool PdbEngine::setToolTipExpression(TextEditor::TextEditorWidget *,
    const DebuggerToolTipContext &ctx)
{
    if (state() != InferiorStopOk)
        return false;

    DebuggerCommand cmd("evaluateTooltip");
    ctx.appendFormatRequest(&cmd);
    runCommand(cmd);
    return true;
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void PdbEngine::assignValueInDebugger(const Internal::WatchData *, const QString &expression, const QVariant &value)
{
    Q_UNUSED(expression);
    Q_UNUSED(value);
    SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value.toString()));
#if 0
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value.toString());
    updateLocals();
#endif
}


void PdbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(data);
    Q_UNUSED(flags);
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
        m_pdbProc.kill();
        Core::AsynchronousMessageBox::critical(tr("Pdb I/O Error"),
                                               errorMessage(error));
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
                .arg(m_pdb);
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
    QByteArray err = m_pdbProc.readAllStandardError();
    qDebug() << "\nPDB STDERR" << err;
    //qWarning() << "Unexpected pdb stderr:" << err;
    //showMessage(_("Unexpected pdb stderr: " + err));
    //handleOutput(err);
}

void PdbEngine::readPdbStandardOutput()
{
    QByteArray out = m_pdbProc.readAllStandardOutput();
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
        emit outputReady(response);
    }
    qDebug() << "BUFFER LEFT: '" << m_inbuffer << '\'';
    //m_inbuffer.clear();
}


void PdbEngine::handleOutput2(const QByteArray &data)
{
    QByteArray lineContext;
    foreach (QByteArray line, data.split('\n')) {
//        line = line.trimmed();

        DebuggerResponse response;
        response.logStreamOutput = line;
        response.data.fromString(line);
        showMessage(_("LINE: " + line));

        if (line.startsWith("stack={")) {
            refreshStack(response.data);
            continue;
        }
        if (line.startsWith("data={")) {
            refreshLocals(response.data);
            continue;
        }
        if (line.startsWith("Breakpoint")) {
            handleBreakInsert(response, Breakpoint());
            continue;
        }

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

        if (line.startsWith("-> ")) {
            // Current line
        }

        showMessage(_(" #### ... UNHANDLED"));
    }


//    DebuggerResponse response;
//    response.logStreamOutput = data;
//    showMessage(_(data));
//    QTC_ASSERT(!m_commands.isEmpty(), qDebug() << "RESPONSE: " << data; return);
//    DebuggerCommand cmd = m_commands.dequeue();
//    qDebug() << "DEQUE: " << cmd.function;
//    if (cmd.callback) {
//        //qDebug() << "EXECUTING CALLBACK " << cmd.callbackName
//        //    << " RESPONSE: " << response.data;
//        cmd.callback(response);
//    } else {
//        qDebug() << "NO CALLBACK FOR RESPONSE: " << response.logStreamOutput;
//    }
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
    //const bool partial = response.cookie.toBool();
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();

    QSet<QByteArray> toDelete;
    foreach (WatchItem *item, handler->model()->treeLevelItems<WatchItem *>(2))
        toDelete.insert(item->d.iname);

    foreach (const GdbMi &child, vars.children()) {
        WatchItem *item = new WatchItem(child);
        handler->insertItem(item);
        toDelete.remove(item->d.iname);
    }

    handler->purgeOutdatedItems(toDelete);

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
            frame.fixQmlFrame(startParameters());
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

void PdbEngine::handleFirstCommand(const DebuggerResponse &response)
{
    Q_UNUSED(response);
}

void PdbEngine::handleUpdateAll(const DebuggerResponse &response)
{
    Q_UNUSED(response);
    notifyInferiorSpontaneousStop();
    updateAll();
}

void PdbEngine::updateAll()
{
    postCommand("dir()");
    postCommand("theDumper.stackListFrames({})");
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
//    cmd.arg("partial", params.tryPartial);

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

    runCommand(cmd);
}

void PdbEngine::handleListLocals(const DebuggerResponse &response)
{
    //qDebug() << " LOCALS: '" << response.data << "'";
    QByteArray out = response.logStreamOutput.trimmed();

    GdbMi all;
    all.fromStringMultiple(out);
    //qDebug() << "ALL: " << all.toString();

    //GdbMi data = all.findChild("data");
    WatchHandler *handler = watchHandler();

    QSet<QByteArray> toDelete;
    foreach (WatchItem *item, handler->model()->treeLevelItems<WatchItem *>(2))
        toDelete.insert(item->d.iname);

    foreach (const GdbMi &child, all.children()) {
        WatchItem *item = new WatchItem(child);
        handler->insertItem(item);
        toDelete.remove(item->d.iname);
    }

    handler->purgeOutdatedItems(toDelete);
}

bool PdbEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability|BreakConditionCapability);
}

DebuggerEngine *createPdbEngine(const DebuggerStartParameters &startParameters)
{
    return new PdbEngine(startParameters);
}

} // namespace Internal
} // namespace Debugger
