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

#include "lldbengine.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerplugin.h"
#include "debuggerprotocol.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "sourceutils.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <utils/qtcassert.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVariant>

#include <QApplication>
#include <QMessageBox>
#include <QToolTip>

#include <stdio.h>


#define DEBUG_SCRIPT 1
#if DEBUG_SCRIPT
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s


#define CB(callback) &LldbEngine::callback, STRINGIFY(callback)

namespace Debugger {
namespace Internal {

static QByteArray quoteUnprintable(const QByteArray &ba)
{
    QByteArray res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const unsigned char c = ba.at(i);
        if (isprint(c)) {
            res += c;
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\x%02x", int(c));
            res += buf;
        }
    }
    return res;
}


///////////////////////////////////////////////////////////////////////
//
// LldbEngine
//
///////////////////////////////////////////////////////////////////////

LldbEngine::LldbEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    setObjectName(QLatin1String("LldbEngine"));
    m_pythonAttemptedToLoad = false;
}

LldbEngine::~LldbEngine()
{}

void LldbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (!(languages & CppLanguage))
        return;
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    //XSDEBUG("LldbEngine::executeDebuggerCommand:" << command);
    if (state() == DebuggerNotReady) {
        showMessage(_("LLDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    postCommand(command.toLatin1(), CB(handleExecuteDebuggerCommand));
}

void LldbEngine::handleExecuteDebuggerCommand(const LldbResponse &response)
{
    Q_UNUSED(response);
}

void LldbEngine::postDirectCommand(const QByteArray &command)
{
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    showMessage(_(command), LogInput);
    m_lldbProc.write(command + '\n');
}

void LldbEngine::postCommand(const QByteArray &command,
                 LldbCommandCallback callback,
                 const char *callbackName,
                 const QVariant &cookie)
{
    static int token = 0;
    ++token;
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    LldbCommand cmd;
    cmd.command = command;
    cmd.callback = callback;
    cmd.callbackName = callbackName;
    cmd.cookie = cookie;
    cmd.token = token;
    m_commands.enqueue(cmd);
    showMessage(QString::number(token) + _(cmd.command), LogInput);
    m_lldbProc.write(cmd.command + '\n');
}

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    notifyInferiorShutdownOk();
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_lldbProc.kill();
}

void LldbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    m_lldb = startParameters().debuggerCommand;
    showMessage(_("STARTING LLDB ") + m_lldb);

    connect(&m_lldbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleLldbError(QProcess::ProcessError)));
    connect(&m_lldbProc, SIGNAL(finished(int,QProcess::ExitStatus)),
        SLOT(handleLldbFinished(int,QProcess::ExitStatus)));
    connect(&m_lldbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readLldbStandardOutput()));
    connect(&m_lldbProc, SIGNAL(readyReadStandardError()),
        SLOT(readLldbStandardError()));

    connect(this, SIGNAL(outputReady(QByteArray)),
        SLOT(handleOutput2(QByteArray)), Qt::QueuedConnection);

    // We will stop immediately, so setup a proper callback.
    LldbCommand cmd;
    cmd.callback = &LldbEngine::handleFirstCommand;
    m_commands.enqueue(cmd);

    m_lldbProc.start(m_lldb);

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start lldb '%1': %2")
            .arg(m_lldb, m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty()) {
            const QString title = tr("Adapter start failed");
            Core::ICore::showWarningWithOptions(title, msg);
        }
        notifyEngineSetupFailed();
        return;
    }

    loadPythonDumpers();
    postCommand("setting set auto-confirm on");
    postCommand("setting set interpreter.prompt-on-quit off");

    // Default:
    // frame-format (string) = "frame #${frame.index}: ${frame.pc}
    // { ${module.file.basename}{`${function.name-with-args}${function.pc-offset}}}
    // { at ${line.file.basename}:${line.number}}\n"
    postCommand("settings set frame-format frame=\\{"
                "index='${frame.index}',"
                "pc='${frame.pc}',"
                "module='${module.file.basename}',"
                //"function='${function.name-with-args}',"
                "function='${function.name}',"
                "pcoffset='${function.pc-offset}',"
                "file='${line.file.basename}',"
                "line='${line.number}'"
                "\\},");


    // Default:
    // "thread #${thread.index}: tid = ${thread.id}{, ${frame.pc}}
    // { ${module.file.basename}{`${function.name-with-args}${function.pc-offset}}}
    // { at ${line.file.basename}:${line.number}}
    // {, stop reason = ${thread.stop-reason}}{\nReturn value: ${thread.return-value}}\n"
    postCommand("settings set thread-format thread=\\{"
                "index='${thread.index}',"
                "tid='${thread.id}',"
                "framepc='${frame.pc}',"
                "module='${module.file.basename}',"
                "function='${function.name}',"
                "pcoffset='${function.pc-offset}',"
                "stopreason='${thread.stop-reason}'"
                //"returnvalue='${thread.return-value}'"
                "\\},");

    notifyEngineSetupOk();
}

void LldbEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    QString fileName = QFileInfo(startParameters().executable).absoluteFilePath();
    postCommand("target create " + fileName.toUtf8(), CB(handleInferiorSetup));
}

void LldbEngine::handleInferiorSetup(const LldbResponse &response)
{
    Q_UNUSED(response);
    notifyInferiorSetupOk();
}

void LldbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    attemptBreakpointSynchronization();
    showStatusMessage(tr("Running requested..."), 5000);
    postCommand("process launch", CB(handleRunEngine));
}

void LldbEngine::handleRunEngine(const LldbResponse &response)
{
    Q_UNUSED(response);
    notifyEngineRunAndInferiorRunOk();
}

void LldbEngine::interruptInferior()
{
    showStatusMessage(tr("Interrupt requested..."), 5000);
    postCommand("process launch", CB(handleInferiorInterrupt));
}

void LldbEngine::handleInferiorInterrupt(const LldbResponse &response)
{
    Q_UNUSED(response);
}

void LldbEngine::executeStep()
{
    resetLocation();
    notifyInferiorRunRequested();
    postCommand("thread step-in", CB(handleContinue));
}

void LldbEngine::handleContinue(const LldbResponse &response)
{
    Q_UNUSED(response);
    notifyInferiorRunOk();
}

void LldbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    postCommand("thread step-inst", CB(handleContinue));
}

void LldbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    postCommand("thread step-out", CB(handleContinue));
}

void LldbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    postCommand("thread step-over", CB(handleContinue));
}

void LldbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
    postCommand("thread step-inst-over", CB(handleContinue));
}

void LldbEngine::continueInferior()
{
    resetLocation();
    notifyInferiorRunRequested();
    postCommand("process continue", CB(handleContinue));
}

void LldbEngine::executeRunToLine(const ContextData &data)
{
    Q_UNUSED(data)
    SDEBUG("FIXME:  LldbEngine::runToLineExec()");
}

void LldbEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  LldbEngine::runToFunctionExec()");
}

void LldbEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    XSDEBUG("FIXME:  LldbEngine::jumpToLineExec()");
}

void LldbEngine::activateFrame(int frameIndex)
{
    resetLocation();
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    postCommand("frame select " + QByteArray::number(frameIndex),
        CB(handleUpdateAll));
}

void LldbEngine::selectThread(ThreadId threadId)
{
    postCommand("thread select " + QByteArray::number(threadId.raw()),
        CB(handleUpdateAll));
}

bool LldbEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    return breakHandler()->breakpointData(id).isCppBreakpoint()
        && startParameters().startMode != AttachCore;
}

void LldbEngine::insertBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    QTC_CHECK(handler->state(id) == BreakpointInsertRequested);
    handler->notifyBreakpointInsertProceeding(id);

    QByteArray loc;
    if (handler->type(id) == BreakpointByFunction)
        loc = " --name " + handler->functionName(id).toLatin1();
    else
        loc = " --file " + handler->fileName(id).toLocal8Bit()
            + " --line " + QByteArray::number(handler->lineNumber(id));

    postCommand("break set " + loc, CB(handleBreakInsert), QVariant(id));
}

void LldbEngine::handleBreakInsert(const LldbResponse &response)
{
    //qDebug() << "BP RESPONSE: " << response.data;
    // Breakpoint 1: where = simple_test_app`main + 62 at simple_test_app.cpp:6699,
    // address = 0x08061664
    BreakpointModelId id(response.cookie.toInt());
    BreakHandler *handler = breakHandler();
    QTC_ASSERT(response.data.startsWith("Breakpoint "), return);
    int pos1 = response.data.indexOf(':');
    QTC_ASSERT(pos1 != -1, return);
    QByteArray bpnr = response.data.mid(11, pos1 - 11);
    int pos2 = response.data.lastIndexOf(':');
    QByteArray file = response.data.mid(pos1 + 4, pos2 - pos1 - 4);
    QByteArray line = response.data.mid(pos2 + 1);
    BreakpointResponse br;
    br.id = BreakpointResponseId(bpnr);
    br.fileName = _(file);
    br.lineNumber = line.toInt();
    handler->setResponse(id, br);
    QTC_CHECK(!handler->needsChange(id));
    handler->notifyBreakpointInsertOk(id);
}

void LldbEngine::removeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    QTC_CHECK(handler->state(id) == BreakpointRemoveRequested);
    handler->notifyBreakpointRemoveProceeding(id);
    BreakpointResponse br = handler->response(id);
    showMessage(_("DELETING BP %1 IN %2").arg(br.id.toString())
        .arg(handler->fileName(id)));
    postCommand("break delete " + br.id.toByteArray());
    // Pretend it succeeds without waiting for response.
    handler->notifyBreakpointRemoveOk(id);
}

void LldbEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void LldbEngine::loadAllSymbols()
{
}

void LldbEngine::reloadModules()
{
    //postCommand("qdebug('listmodules')", CB(handleListModules));
}

void LldbEngine::handleListModules(const LldbResponse &response)
{
    GdbMi out;
    out.fromString(response.data.trimmed());
    Modules modules;
    foreach (const GdbMi &item, out.children()) {
        Module module;
        module.moduleName = _(item.findChild("name").data());
        QString path = _(item.findChild("value").data());
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
        modules.append(module);
    }
    modulesHandler()->setModules(modules);
}

void LldbEngine::requestModuleSymbols(const QString &moduleName)
{
    postCommand("target module list",
        CB(handleListSymbols), moduleName);
}

void LldbEngine::handleListSymbols(const LldbResponse &response)
{
    GdbMi out;
    out.fromString(response.data.trimmed());
    Symbols symbols;
    QString moduleName = response.cookie.toString();
    foreach (const GdbMi &item, out.children()) {
        Symbol symbol;
        symbol.name = _(item.findChild("name").data());
        symbols.append(symbol);
    }
   debuggerCore()->showModuleSymbols(moduleName, symbols);
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////


static WatchData m_toolTip;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

bool LldbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    Q_UNUSED(mousePos)
    Q_UNUSED(editor)

    if (state() != InferiorStopOk) {
        //SDEBUG("SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED");
        return false;
    }
    // Check mime type and get expression (borrowing some C++ - functions)
    const QString javaPythonMimeType =
        QLatin1String("application/javascript");
    if (!editor->document() || editor->document()->mimeType() != javaPythonMimeType)
        return false;

    int line;
    int column;
    QString exp = cppExpressionAt(editor, ctx.position, &line, &column);

/*
    if (m_toolTipCache.contains(exp)) {
        const WatchData & data = m_toolTipCache[exp];
        q->watchHandler()->removeChildren(data.iname);
        insertData(data);
        return;
    }
*/

    QToolTip::hideText();
    if (exp.isEmpty() || exp.startsWith(QLatin1Char('#')))  {
        QToolTip::hideText();
        return false;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos, tr("'%1' contains no identifier").arg(exp));
        return true;
    }

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"'))) {
        QToolTip::showText(m_toolTipPos, tr("String literal %1").arg(exp));
        return true;
    }

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return false;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            tr("Cowardly refusing to evaluate expression '%1' "
               "with potential side effects").arg(exp));
        return true;
    }

#if 0
    //if (status() != InferiorStopOk)
    //    return;

    // FIXME: 'exp' can contain illegal characters
    m_toolTip = WatchData();
    m_toolTip.exp = exp;
    m_toolTip.name = exp;
    m_toolTip.iname = tooltipIName;
    insertData(m_toolTip);
#endif
    return false;
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void LldbEngine::assignValueInDebugger(const Internal::WatchData *, const QString &expression, const QVariant &value)
{
    Q_UNUSED(expression);
    Q_UNUSED(value);
    SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value.toString()));
#if 0
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value.toString());
    updateLocals();
#endif
}


void LldbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(data);
    Q_UNUSED(flags);
    updateAll();
}

void LldbEngine::handleLldbError(QProcess::ProcessError error)
{
    qDebug() << "HANDLE LLDB ERROR";
    showMessage(_("HANDLE LLDB ERROR"));
    switch (error) {
    case QProcess::Crashed:
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        m_lldbProc.kill();
        showMessageBox(QMessageBox::Critical, tr("Lldb I/O Error"),
                       errorMessage(error));
        break;
    }
}

QString LldbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The Lldb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(m_lldb);
        case QProcess::Crashed:
            return tr("The Lldb process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the Lldb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Lldb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the Lldb process occurred. ");
    }
}

void LldbEngine::handleLldbFinished(int code, QProcess::ExitStatus type)
{
    qDebug() << "LLDB FINISHED";
    showMessage(_("LLDB PROCESS FINISHED, status %1, code %2").arg(type).arg(code));
    notifyEngineSpontaneousShutdown();
}

void LldbEngine::readLldbStandardError()
{
    QByteArray err = m_lldbProc.readAllStandardError();
    qDebug() << "\nLLDB STDERR" << err;
    //qWarning() << "Unexpected lldb stderr:" << err;
    showMessage(_("Lldb stderr: " + err));
    //handleOutput(err);
}

static bool isEatable(char c)
{
    return c == 10 || c == 13;
}

void LldbEngine::readLldbStandardOutput()
{
    QByteArray out = m_lldbProc.readAllStandardOutput();
    showMessage(_("Lldb stdout: " + out));
    qDebug("\nLLDB RAW STDOUT: '%s'", quoteUnprintable(out).constData());
    // Remove embedded backspace characters
    int j = 1;
    const int n = out.size();
    for (int i = 1; i < n; ++i, ++j) {
        const char c = out.at(i);
        if (i != j)
            out[j] = c;
        if (c == 8)
            j -= 2;
        else if (isEatable(c))
            --j;
    }
    if (j != n)
        out.resize(j);

    //qDebug("\n\nLLDB STDOUT: '%s'", quoteUnprintable(out).constData());
    if (out.isEmpty())
        return;

    if (out.startsWith(8)) {
        if (!m_inbuffer.isEmpty())
            m_inbuffer.chop(1);
        m_inbuffer.append(out.mid(1));
    } else if (isEatable(out.at(0))) {
        m_inbuffer.append(out.mid(1));
    } else {
        m_inbuffer.append(out);
    }

    qDebug("\nBUFFER FROM: '%s'", quoteUnprintable(m_inbuffer).constData());
    while (true) {
        int pos = m_inbuffer.indexOf("(lldb) ");
        if (pos == -1)
            break;
        QByteArray response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 7);
        qDebug("\nBUFFER RECOGNIZED: '%s'", quoteUnprintable(response).constData());
        emit outputReady(response);
    }
    qDebug("\nBUFFER LEFT: '%s'", quoteUnprintable(m_inbuffer).constData());

    if (m_inbuffer.isEmpty())
        return;

    // Could be a 'stopped' message left.
    // "<Esc>[KProcess 5564 stopped"
    // "* thread #1: tid = 0x15bc, 0x0804a17d untitled11`main(argc=1,
    // argv=0xbfffeff4) + 61 at main.cpp:52, stop reason = breakpoint 1.1 ..."
    const int pos = m_inbuffer.indexOf("\x1b[KProcess");
    if (pos != -1) {
        int pid = 0;
        const char *format = "\x1b[KProcess %d stopped";
        if (::sscanf(m_inbuffer.constData() + pos, format, &pid) == 1) {
            notifyInferiorSpontaneousStop();
            m_inbuffer.clear();
            updateAll();
        }
    }
}

void LldbEngine::handleOutput2(const QByteArray &data)
{
    LldbResponse response;
    response.data = data;
    showMessage(_(data));
    QTC_ASSERT(!m_commands.isEmpty(), qDebug() << "RESPONSE: " << data; return);
    LldbCommand cmd = m_commands.dequeue();
    // FIXME: Find a way to tell LLDB to no echo input.
    if (response.data.startsWith(cmd.command))
        response.data = response.data.mid(cmd.command.size());

    response.cookie = cmd.cookie;
    qDebug("\nDEQUE: '%s' -> '%s'", cmd.command.constData(), cmd.callbackName);
    if (cmd.callback) {
        //qDebug() << "EXECUTING CALLBACK " << cmd.callbackName
        //    << " RESPONSE: " << response.data;
        (this->*cmd.callback)(response);
    } else {
        qDebug() << "NO CALLBACK FOR RESPONSE: " << response.data;
    }
}

void LldbEngine::handleFirstCommand(const LldbResponse &response)
{
    Q_UNUSED(response);
}

void LldbEngine::handleUpdateAll(const LldbResponse &response)
{
    Q_UNUSED(response);
    updateAll();
}

void LldbEngine::updateAll()
{
    //postCommand("bt", CB(handleBacktrace));
    updateLocals();
}

void LldbEngine::updateLocals()
{
    QByteArray watchers;
    //if (!m_toolTipExpression.isEmpty())
    //    watchers += m_toolTipExpression.toLatin1()
    //        + '#' + tooltipINameForExpression(m_toolTipExpression.toLatin1());

    WatchHandler *handler = watchHandler();
    QHash<QByteArray, int> watcherNames = handler->watcherNames();
    QHashIterator<QByteArray, int> it(watcherNames);
    while (it.hasNext()) {
        it.next();
        if (!watchers.isEmpty())
            watchers += "##";
        watchers += it.key() + "#watch." + QByteArray::number(it.value());
    }

    QByteArray options;
    if (debuggerCore()->boolSetting(UseDebuggingHelpers))
        options += "fancy,";
    if (debuggerCore()->boolSetting(AutoDerefPointers))
        options += "autoderef,";
    if (options.isEmpty())
        options += "defaults,";
    options.chop(1);

    postCommand("script bb('options:" + options + " "
        + "vars: "
        + "expanded:" + handler->expansionRequests() + " "
        + "typeformats:" + handler->typeFormatRequests() + " "
        + "formats:" + handler->individualFormatRequests() + " "
        + "watcher:" + watchers.toHex() + "')",
                CB(handleListLocals));
}

void LldbEngine::handleBacktrace(const LldbResponse &response)
{
    // Populate stack view.
    StackFrames stackFrames;
    int level = 0;
    int currentIndex = -1;
    foreach (const QByteArray &line, response.data.split('\n')) {
        //qDebug() << "  LINE: '" << line << "'";
        if (line.startsWith("> ") || line.startsWith("  ")) {
            int pos1 = line.indexOf('(');
            int pos2 = line.indexOf(')', pos1);
            if (pos1 != -1 && pos2 != -1) {
                int lineNumber = line.mid(pos1 + 1, pos2 - pos1 - 1).toInt();
                QByteArray fileName = line.mid(2, pos1 - 2);
                //qDebug() << " " << pos1 << pos2 << lineNumber << fileName
                //    << line.mid(pos1 + 1, pos2 - pos1 - 1);
                StackFrame frame;
                frame.file = _(fileName);
                frame.line = lineNumber;
                frame.function = _(line.mid(pos2 + 1));
                frame.usable = QFileInfo(frame.file).isReadable();
                if (frame.line > 0 && QFileInfo(frame.file).exists()) {
                    if (line.startsWith("> "))
                        currentIndex = level;
                    frame.level = level;
                    stackFrames.prepend(frame);
                    ++level;
                }
            }
        }
    }
    const int frameCount = stackFrames.size();
    for (int i = 0; i != frameCount; ++i)
        stackFrames[i].level = frameCount - stackFrames[i].level - 1;
    stackHandler()->setFrames(stackFrames);

    // Select current frame.
    if (currentIndex != -1) {
        currentIndex = frameCount - currentIndex - 1;
        stackHandler()->setCurrentIndex(currentIndex);
        gotoLocation(stackFrames.at(currentIndex));
    }

    updateLocals();
}

void LldbEngine::handleListLocals(const LldbResponse &response)
{
    //qDebug() << " LOCALS: '" << response.data << "'";
    QByteArray out = response.data.trimmed();

//    GdbMi all;
//    all.fromStringMultiple(out);
//    //qDebug() << "ALL: " << all.toString();

//    //GdbMi data = all.findChild("data");
//    QList<WatchData> list;
//    WatchHandler *handler = watchHandler();
//    foreach (const GdbMi &child, all.children()) {
//        WatchData dummy;
//        dummy.iname = child.findChild("iname").data();
//        dummy.name = _(child.findChild("name").data());
//        //qDebug() << "CHILD: " << child.toString();
//        parseWatchData(handler->expandedINames(), dummy, child, &list);
//    }
//    handler->insertData(list);

    const bool partial = response.cookie.toBool();
    int pos = out.indexOf("data=");
    if (pos != 0) {
        showMessage(_("DISCARDING JUNK AT BEGIN OF RESPONSE: "
            + out.left(pos)));
        out = out.mid(pos);
    }
    GdbMi all;
    all.fromStringMultiple(out);
    GdbMi data = all.findChild("data");

    WatchHandler *handler = watchHandler();
    QList<WatchData> list;

    if (!partial) {
        list.append(*handler->findData("local"));
        list.append(*handler->findData("watch"));
        list.append(*handler->findData("return"));
    }

    foreach (const GdbMi &child, data.children()) {
        WatchData dummy;
        dummy.iname = child.findChild("iname").data();
        GdbMi wname = child.findChild("wname");
        if (wname.isValid()) {
            // Happens (only) for watched expressions. They are encoded as
            // base64 encoded 8 bit data, without quotes
            dummy.name = decodeData(wname.data(), Base64Encoded8Bit);
            dummy.exp = dummy.name.toUtf8();
        } else {
            dummy.name = _(child.findChild("name").data());
        }
        parseWatchData(handler->expandedINames(), dummy, child, &list);
    }
//    const GdbMi typeInfo = all.findChild("typeinfo");
//    if (typeInfo.type() == GdbMi::List) {
//        foreach (const GdbMi &s, typeInfo.children()) {
//            const GdbMi name = s.findChild("name");
//            const GdbMi size = s.findChild("size");
//            if (name.isValid() && size.isValid())
//                m_typeInfoCache.insert(QByteArray::fromBase64(name.data()),
//                                       TypeInfo(size.data().toUInt()));
//        }
//    }
//    for (int i = 0; i != list.size(); ++i) {
//        const TypeInfo ti = m_typeInfoCache.value(list.at(i).type);
//        if (ti.size)
//            list[i].size = ti.size;
//    }

    handler->insertData(list);

    //rebuildWatchModel();
    if (!partial)
        emit stackFrameCompleted();
}

void LldbEngine::loadPythonDumpers()
{
    if (m_pythonAttemptedToLoad)
        return;
    m_pythonAttemptedToLoad = true;

    const QByteArray dumperSourcePath =
        Core::ICore::resourcePath().toLocal8Bit() + "/dumper/";

    postCommand("script execfile('" + dumperSourcePath + "bridge.py')");
    postCommand("script execfile('" + dumperSourcePath + "dumper.py')");
    postCommand("script execfile('" + dumperSourcePath + "qttypes.py')");
    postCommand("script bbsetup('')");
}

bool LldbEngine::hasCapability(unsigned cap) const
{
    return cap & (ReloadModuleCapability|BreakConditionCapability);
}

DebuggerEngine *createLldbEngine(const DebuggerStartParameters &startParameters)
{
    return new LldbEngine(startParameters);
}


} // namespace Internal
} // namespace Debugger
