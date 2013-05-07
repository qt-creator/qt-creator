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
#include "disassemblerlines.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "sourceutils.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

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


namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// LldbEngine
//
///////////////////////////////////////////////////////////////////////

LldbEngine::LldbEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters)
{
    m_lastAgentId = 0;
    setObjectName(QLatin1String("LldbEngine"));
}

LldbEngine::~LldbEngine()
{}

void LldbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages languages)
{
    if (!(languages & CppLanguage))
        return;
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    if (state() == DebuggerNotReady) {
        showMessage(_("LLDB PROCESS NOT RUNNING, PLAIN CMD IGNORED: ") + command);
        return;
    }
    //runCommand(command.toUtf8());
    m_lldbProc.write(command.toUtf8() + '\n');
}

static int token = 1;

void LldbEngine::runCommand(const Command &command)
{
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    ++token;
    QByteArray cmd = "db {'cmd':'" + command.function + "',"
        + command.args + "'token':" + QByteArray::number(token) + "}\n";
    showMessage(QString::number(token) + _(cmd), LogInput);
    m_lldbProc.write(cmd);
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

    m_lldbBridge = startParameters().debuggerCommand;
    showMessage(_("STARTING LLDB ") + m_lldbBridge);

    connect(&m_lldbProc, SIGNAL(error(QProcess::ProcessError)),
        SLOT(handleLldbError(QProcess::ProcessError)));
    connect(&m_lldbProc, SIGNAL(finished(int,QProcess::ExitStatus)),
        SLOT(handleLldbFinished(int,QProcess::ExitStatus)));
    connect(&m_lldbProc, SIGNAL(readyReadStandardOutput()),
        SLOT(readLldbStandardOutput()));
    connect(&m_lldbProc, SIGNAL(readyReadStandardError()),
        SLOT(readLldbStandardError()));

    connect(this, SIGNAL(outputReady(QByteArray)),
        SLOT(handleResponse(QByteArray)), Qt::QueuedConnection);

    //m_lldbProc.start(m_lldb);
    m_lldbProc.start(_("/usr/bin/python"), QStringList() << _("-i") << m_lldbBridge);

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start lldb '%1': %2")
            .arg(m_lldbBridge, m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty())
            Core::ICore::showWarningWithOptions(tr("Adapter start failed"), msg);
    }
}

void LldbEngine::setupInferior()
{
    QString executable = QFileInfo(startParameters().executable).absoluteFilePath();
    runCommand(Command("setupInferior").arg("executable", executable));
}

void LldbEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    Command cmd("handleBreakpoints");
    if (attemptBreakpointSynchronizationHelper(&cmd)) {
        runEngine2();
    } else {
        cmd.arg("continuation", "runEngine2");
        runCommand(cmd);
    }
}

void LldbEngine::runEngine2()
{
    showStatusMessage(tr("Running requested..."), 5000);
    runCommand("runEngine");
}

void LldbEngine::interruptInferior()
{
    showStatusMessage(tr("Interrupt requested..."), 5000);
    runCommand("interrupt");
}

void LldbEngine::executeStep()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("step");
}

void LldbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("stepi");
}

void LldbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("finish");
}

void LldbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("next");
}

void LldbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("nexti");
}

void LldbEngine::continueInferior()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("continue");
}

void LldbEngine::handleResponse(const QByteArray &response)
{
    GdbMi all;
    all.fromStringMultiple(response);

    foreach (const GdbMi &item, all.children()) {
        const QByteArray name = item.name();
        if (name == "data")
            refreshLocals(item);
        else if (name == "stack")
            refreshStack(item);
        else if (name == "registers")
            refreshRegisters(item);
        else if (name == "threads")
            refreshThreads(item);
        else if (name == "typeinfo")
            refreshTypeInfo(item);
        else if (name == "state")
            refreshState(item);
        else if (name == "location")
            refreshLocation(item);
        else if (name == "modules")
            refreshModules(item);
        else if (name == "symbols")
            refreshSymbols(item);
        else if (name == "bkpts")
            refreshBreakpoints(item);
        else if (name == "disassembly")
            refreshDisassembly(item);
        else if (name == "memory")
            refreshMemory(item);
        else if (name == "continuation")
            runContinuation(item);
    }
}

void LldbEngine::runContinuation(const GdbMi &data)
{
    const QByteArray target = data.data();
    QMetaObject::invokeMethod(this, target, Qt::QueuedConnection);
}

void LldbEngine::executeRunToLine(const ContextData &data)
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand(Command("executeRunToLine")
        .arg("file", data.fileName).arg("line", data.address));
}

void LldbEngine::executeRunToFunction(const QString &functionName)
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand(Command("executeRunToFunction").arg("function", functionName));
}

void LldbEngine::executeJumpToLine(const ContextData &data)
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand(Command("executeJumpToLine")
        .arg("file", data.fileName).arg("line", data.address));
}

void LldbEngine::activateFrame(int frameIndex)
{
    resetLocation();
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    runCommand(Command("activateFrame").arg("index", frameIndex));
}

void LldbEngine::selectThread(ThreadId threadId)
{
    runCommand(Command("selectThread").arg("id", threadId.raw()));
}

bool LldbEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    return breakHandler()->breakpointData(id).isCppBreakpoint()
        && startParameters().startMode != AttachCore;
}

bool LldbEngine::attemptBreakpointSynchronizationHelper(Command *cmd)
{
    BreakHandler *handler = breakHandler();

    foreach (BreakpointModelId id, handler->unclaimedBreakpointIds()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(id)) {
            showMessage(_("TAKING OWNERSHIP OF BREAKPOINT %1 IN STATE %2")
                .arg(id.toString()).arg(handler->state(id)));
            handler->setEngine(id, this);
        } else {
            showMessage(_("BREAKPOINT %1 IN STATE %2 IS NOT ACCEPTABLE")
                .arg(id.toString()).arg(handler->state(id)));
        }
    }

    bool done = true;
    cmd->beginList("bkpts");
    foreach (BreakpointModelId id, handler->engineBreakpointIds(this)) {
        const BreakpointResponse &response = handler->response(id);
        switch (handler->state(id)) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_CHECK(false);
            break;
        case BreakpointInsertRequested:
            done = false;
            cmd->beginGroup()
                    .arg("operation", "add")
                    .arg("modelid", id.toByteArray())
                    .arg("type", handler->type(id))
                    .arg("ignorecount", handler->ignoreCount(id))
                    .arg("condition", handler->condition(id))
                    .arg("function", handler->functionName(id).toUtf8())
                    .arg("oneshot", handler->isOneShot(id))
                    .arg("enabled", handler->isEnabled(id))
                    .arg("file", handler->fileName(id).toUtf8())
                    .arg("line", handler->lineNumber(id))
                    .endGroup();
            handler->notifyBreakpointInsertProceeding(id);
            break;
        case BreakpointChangeRequested:
            done = false;
            cmd->beginGroup()
                    .arg("operation", "change")
                    .arg("modelid", id.toByteArray())
                    .arg("lldbid", response.id.toByteArray())
                    .arg("type", handler->type(id))
                    .arg("ignorecount", handler->ignoreCount(id))
                    .arg("condition", handler->condition(id))
                    .arg("function", handler->functionName(id).toUtf8())
                    .arg("oneshot", handler->isOneShot(id))
                    .arg("enabled", handler->isEnabled(id))
                    .arg("file", handler->fileName(id).toUtf8())
                    .arg("line", handler->lineNumber(id))
                    .endGroup();
            handler->notifyBreakpointChangeProceeding(id);
            break;
        case BreakpointRemoveRequested:
            done = false;
            cmd->beginGroup()
                    .arg("operation", "remove")
                    .arg("modelid", id.toByteArray())
                    .arg("lldbid", response.id.toByteArray())
                    .endGroup();
            handler->notifyBreakpointRemoveProceeding(id);
            break;
        case BreakpointChangeProceeding:
        case BreakpointInsertProceeding:
        case BreakpointRemoveProceeding:
        case BreakpointInserted:
        case BreakpointDead:
            QTC_CHECK(false);
            break;
        default:
            QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << id << state());
        }
    }
    cmd->endList();
    return done;
}

void LldbEngine::attemptBreakpointSynchronization()
{
    showMessage(_("ATTEMPT BREAKPOINT SYNCHRONIZATION"));
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    Command cmd("handleBreakpoints");
    if (!attemptBreakpointSynchronizationHelper(&cmd)) {
        showMessage(_("BREAKPOINTS ARE NOT FULLY SYNCHRONIZED"));
        runCommand(cmd);
    } else {
        showMessage(_("BREAKPOINTS ARE SYNCHRONIZED"));
    }
}

void LldbEngine::updateBreakpointData(const GdbMi &bkpt, bool added)
{
    BreakHandler *handler = breakHandler();
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    BreakpointResponse response = handler->response(id);
    BreakpointResponseId rid = BreakpointResponseId(bkpt["lldbid"].data());
    if (added)
        response.id = rid;
    QTC_CHECK(response.id == rid);
    response.address = 0;
    response.enabled = bkpt["enabled"].data().toInt();
    response.ignoreCount = bkpt["ignorecount"].data().toInt();
    response.condition = bkpt["condition"].data();
    response.hitCount = bkpt["hitcount"].data().toInt();

    if (added) {
        // Added.
        GdbMi locations = bkpt["locations"];
        const int numChild = locations.children().size();
        if (numChild > 1) {
            foreach (const GdbMi &location, locations.children()) {
                const int locid = location["locid"].data().toUShort();
                BreakpointResponse sub;
                sub.id = BreakpointResponseId(rid.majorPart(), locid);
                sub.type = response.type;
                sub.address = location["addr"].toAddress();
                sub.functionName = QString::fromUtf8(location["func"].data());
                handler->insertSubBreakpoint(id, sub);
            }
        } else if (numChild == 1) {
            const GdbMi location = locations.childAt(0);
            response.address = location["addr"].toAddress();
            response.functionName = QString::fromUtf8(location["func"].data());
        } else {
            QTC_CHECK(false);
        }
        handler->setResponse(id, response);
        handler->notifyBreakpointInsertOk(id);
    } else {
        // Changed.
        handler->setResponse(id, response);
        handler->notifyBreakpointChangeOk(id);
    }
}

void LldbEngine::refreshDisassembly(const GdbMi &data)
{
    DisassemblerLines result;

    int cookie = data["cookie"].data().toInt();
    QPointer<DisassemblerAgent> agent = m_disassemblerAgents.key(cookie);
    if (!agent.isNull()) {
        foreach (const GdbMi &line, data["lines"].children()) {
            DisassemblerLine dl;
            QByteArray address = line["address"].data();
            dl.address = address.toULongLong(0, 0);
            dl.data = _(line["inst"].data());
            dl.function = _(line["func-name"].data());
            dl.offset = line["offset"].data().toUInt();
            result.appendLine(dl);
        }
        agent->setContents(result);
    }
}

void LldbEngine::refreshMemory(const GdbMi &data)
{
    int cookie = data["cookie"].data().toInt();
    qulonglong addr = data["address"].data().toInt();
    QPointer<MemoryAgent> agent = m_memoryAgents.key(cookie);
    if (!agent.isNull()) {
        QPointer<QObject> token = m_memoryAgentTokens.value(cookie);
        QTC_ASSERT(!token.isNull(), return);
        QByteArray ba = QByteArray::fromHex(data["contents"].data());
        agent->addLazyData(token.data(), addr, ba);
    }
}

void LldbEngine::refreshBreakpoints(const GdbMi &bkpts)
{
    BreakHandler *handler = breakHandler();
    foreach (const GdbMi &bkpt, bkpts.children()) {
        QByteArray op = bkpt["operation"].data();
        if (op == "added") {
            BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
            QTC_CHECK(handler->state(id) == BreakpointInsertProceeding);
            updateBreakpointData(bkpt, true);
        } else if (op == "changed") {
            BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
            QTC_CHECK(handler->state(id) == BreakpointChangeProceeding);
            updateBreakpointData(bkpt, false);
        } else if (op == "removed") {
            BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
            QTC_CHECK(handler->state(id) == BreakpointRemoveProceeding);
            handler->notifyBreakpointRemoveOk(id);
        }
    }
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
    runCommand("listModules");
}

void LldbEngine::refreshModules(const GdbMi &modules)
{
    Modules mods;
    foreach (const GdbMi &item, modules.children()) {
        Module module;
        module.modulePath = QString::fromUtf8(item["file"].data());
        module.moduleName = QString::fromUtf8(item["name"].data());
        module.symbolsRead = Module::UnknownReadState;
        module.startAddress = item["loaded_addr"].toAddress();
        module.endAddress = 0; // FIXME: End address not easily available.
        mods.append(module);
    }
    modulesHandler()->setModules(mods);
}

void LldbEngine::requestModuleSymbols(const QString &moduleName)
{
    runCommand(Command("listSymbols").arg("module", moduleName));
}

void LldbEngine::refreshSymbols(const GdbMi &symbols)
{
    QString moduleName = QString::fromUtf8(symbols["module"].data());
    Symbols syms;
    foreach (const GdbMi &item, symbols["symbols"].children()) {
        Symbol symbol;
        symbol.address = _(item["address"].data());
        symbol.name = _(item["name"].data());
        symbol.state = _(item["state"].data());
        symbol.section = _(item["section"].data());
        symbol.demangled = _(item["demangled"].data());
        syms.append(symbol);
    }
   debuggerCore()->showModuleSymbols(moduleName, syms);
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
    Q_UNUSED(mousePos);
    Q_UNUSED(editor);

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
    //SDEBUG("ASSIGNING: " << (expression + QLatin1Char('=') + value.toString()));
#if 0
    m_scriptEngine->evaluate(expression + QLatin1Char('=') + value.toString());
    updateLocals();
#endif
}


void LldbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(data);
    Q_UNUSED(flags);
    WatchHandler *handler = watchHandler();
    runCommand(Command("updateData").arg("expanded", handler->expansionRequests()));
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
                .arg(m_lldbBridge);
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

void LldbEngine::readLldbStandardOutput()
{
    QByteArray out = m_lldbProc.readAllStandardOutput();
    showMessage(_("Lldb stdout: " + out));
    m_inbuffer.append(out);
    while (true) {
        int pos = m_inbuffer.indexOf("@\n");
        if (pos == -1)
            break;
        QByteArray response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 2);
        emit outputReady(response);
    }
}

QByteArray LldbEngine::currentOptions() const
{
    QByteArray localsOptions;
    QByteArray stackOptions;
    QByteArray threadsOptions;

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

        localsOptions = "options:" + options + " "
            + "vars: "
            + "expanded:" + handler->expansionRequests() + " "
            + "typeformats:" + handler->typeFormatRequests() + " "
            + "formats:" + handler->individualFormatRequests() + " "
            + "watcher:" + watchers.toHex();
    }

    {
        int maxdepth = debuggerCore()->action(MaximalStackDepth)->value().toInt();
        ThreadId curthread = threadsHandler()->currentThread();
        stackOptions += "maxdepth:" + QByteArray::number(maxdepth);
        stackOptions += ",curthread:" + QByteArray::number(curthread.raw());
    }

    QByteArray result;
    result += "\"locals\":\"" + localsOptions + '"';
    result += ",\"stack\":\"" + stackOptions + '"';
    result += ",\"threads\":\"" + threadsOptions + '"';

    return result;
}

void LldbEngine::updateAll()
{
    runCommand("createReport");
}

void LldbEngine::refreshLocals(const GdbMi &vars)
{
    //const bool partial = response.cookie.toBool();
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();
    QList<WatchData> list;

    //if (!partial) {
        list.append(*handler->findData("local"));
        list.append(*handler->findData("watch"));
        list.append(*handler->findData("return"));
    //}

    foreach (const GdbMi &child, vars.children()) {
        WatchData dummy;
        dummy.iname = child["iname"].data();
        GdbMi wname = child["wname"];
        if (wname.isValid()) {
            // Happens (only) for watched expressions. They are encoded as
            // base64 encoded 8 bit data, without quotes
            dummy.name = decodeData(wname.data(), Base64Encoded8Bit);
            dummy.exp = dummy.name.toUtf8();
        } else {
            dummy.name = _(child["name"].data());
        }
        parseWatchData(handler->expandedINames(), dummy, child, &list);
    }
    handler->insertData(list);
 }

void LldbEngine::refreshStack(const GdbMi &stack)
{
    //if (!partial)
    //    emit stackFrameCompleted();
    StackHandler *handler = stackHandler();
    StackFrames frames;
    foreach (const GdbMi &item, stack["frames"].children()) {
        StackFrame frame;
        frame.level = item["level"].data().toInt();
        frame.file = QString::fromLatin1(item["file"].data());
        frame.function = QString::fromLatin1(item["func"].data());
        frame.from = QString::fromLatin1(item["func"].data());
        frame.line = item["line"].data().toInt();
        frame.address = item["addr"].toAddress();
        frame.usable = QFileInfo(frame.file).isReadable();
        frames.append(frame);
    }
    bool canExpand = stack["hasmore"].data().toInt();
    debuggerCore()->action(ExpandStack)->setEnabled(canExpand);
    handler->setFrames(frames);
}

void LldbEngine::refreshRegisters(const GdbMi &registers)
{
    RegisterHandler *handler = registerHandler();
    Registers regs;
    foreach (const GdbMi &item, registers.children()) {
        Register reg;
        reg.name = item["name"].data();
        reg.value = item["value"].data();
        //reg.type = item["type"].data();
        regs.append(reg);
    }
    //handler->setRegisters(registers);
    handler->setAndMarkRegisters(regs);
}

void LldbEngine::refreshThreads(const GdbMi &threads)
{
    ThreadsHandler *handler = threadsHandler();
    handler->updateThreads(threads);
    if (!handler->currentThread().isValid()) {
        ThreadId other = handler->threadAt(0);
        if (other.isValid())
            selectThread(other);
    }
    updateViews(); // Adjust Threads combobox.
}

void LldbEngine::refreshTypeInfo(const GdbMi &typeInfo)
{
    if (typeInfo.type() == GdbMi::List) {
//        foreach (const GdbMi &s, typeInfo.children()) {
//            const GdbMi name = s["name"];
//            const GdbMi size = s["size"];
//            if (name.isValid() && size.isValid())
//                m_typeInfoCache.insert(QByteArray::fromBase64(name.data()),
//                                       TypeInfo(size.data().toUInt()));
//        }
    }
//    for (int i = 0; i != list.size(); ++i) {
//        const TypeInfo ti = m_typeInfoCache.value(list.at(i).type);
//        if (ti.size)
//            list[i].size = ti.size;
//    }
}

void LldbEngine::refreshState(const GdbMi &reportedState)
{
    QByteArray newState = reportedState.data();
    if (newState == "running")
        notifyInferiorRunOk();
    else if (newState == "inferiorrunfailed")
        notifyInferiorRunFailed();
    else if (newState == "stopped")
        notifyInferiorSpontaneousStop();
    else if (newState == "inferiorstopok")
        notifyInferiorStopOk();
    else if (newState == "inferiorstopfailed")
        notifyInferiorStopFailed();
    else if (newState == "enginesetupok")
        notifyEngineSetupOk();
    else if (newState == "enginesetupfailed")
        notifyEngineSetupFailed();
    else if (newState == "stopped")
        notifyInferiorSpontaneousStop();
    else if (newState == "inferiorsetupok")
        notifyInferiorSetupOk();
    else if (newState == "enginerunok")
        notifyEngineRunAndInferiorRunOk();
}

void LldbEngine::refreshLocation(const GdbMi &reportedLocation)
{
    QByteArray file = reportedLocation["file"].data();
    int line = reportedLocation["line"].data().toInt();
    gotoLocation(Location(QString::fromUtf8(file), line));
}

void LldbEngine::reloadRegisters()
{
    runCommand("reloadRegisters");
}

void LldbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    QPointer<DisassemblerAgent> p(agent);
    int id = m_disassemblerAgents.value(p, -1);
    if (id == -1) {
        id = ++m_lastAgentId;
        m_disassemblerAgents.insert(p, id);
    }
    runCommand(Command("disassemble").arg("cookie", id));
}


void LldbEngine::fetchMemory(MemoryAgent *agent, QObject *editorToken,
        quint64 addr, quint64 length)
{
    int id = m_memoryAgents.value(agent, -1);
    if (id == -1) {
        id = ++m_lastAgentId;
        m_memoryAgents.insert(agent, id);
        m_memoryAgentTokens.insert(id, editorToken);
    }
    runCommand(Command("readMemory")
               .arg("address", addr)
               .arg("length", length)
               .arg("cookie", id));
}

void LldbEngine::changeMemory(MemoryAgent *agent, QObject *editorToken,
        quint64 addr, const QByteArray &data)
{
    int id = m_memoryAgents.value(agent, -1);
    if (id == -1) {
        id = ++m_lastAgentId;
        m_memoryAgents.insert(agent, id);
        m_memoryAgentTokens.insert(id, editorToken);
    }
    runCommand(Command("writeMemory")
               .arg("address", addr)
               .arg("data", data.toHex())
               .arg("cookie", id));
}

void LldbEngine::setRegisterValue(int regnr, const QString &value)
{
    Register reg = registerHandler()->registers().at(regnr);
    runCommand(Command("setRegister").arg("name", reg.name).arg("value", value));
}


bool LldbEngine::hasCapability(unsigned cap) const
{
    if (cap & (ReverseSteppingCapability
        | AutoDerefPointersCapability
        | DisassemblerCapability
        | RegisterCapability
        | ShowMemoryCapability
        | JumpToLineCapability
        | ReloadModuleCapability
        | ReloadModuleSymbolsCapability
        | BreakOnThrowAndCatchCapability
        | BreakConditionCapability
        | TracePointCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointByAddressCapability
        | WatchpointByExpressionCapability
        | AddWatcherCapability
        | WatchWidgetsCapability
        | ShowModuleSymbolsCapability
        | ShowModuleSectionsCapability
        | CatchCapability
        | OperateByInstructionCapability
        | RunToLineCapability
        | WatchComplexExpressionsCapability
        | MemoryAddressCapability))
        return true;

    if (startParameters().startMode == AttachCore)
        return false;

    //return cap == SnapshotCapability;
    return false;
}

DebuggerEngine *createLldbEngine(const DebuggerStartParameters &startParameters)
{
    return new LldbEngine(startParameters);
}


///////////////////////////////////////////////////////////////////////
//
// Command
//
///////////////////////////////////////////////////////////////////////

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, int value) const
{
    args.append('\'');
    args.append(name);
    args.append("':");
    args.append(QByteArray::number(value));
    args.append(',');
    return *this;
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, const QString &value) const
{
    return arg(name, value.toUtf8().data());
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, const QByteArray &value) const
{
    return arg(name, value.data());
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, const char *value) const
{
    args.append('\'');
    args.append(name);
    args.append("':'");
    args.append(value);
    args.append("',");
    return *this;
}

const LldbEngine::Command &LldbEngine::Command::beginList(const char *name) const
{
    if (name) {
        args += '\'';
        args += name;
        args += "':";
    }
    args += '[';
    return *this;
}

void LldbEngine::Command::endList() const
{
    if (args.endsWith(','))
        args.chop(1);
    args += "],";
}

const LldbEngine::Command &LldbEngine::Command::beginGroup(const char *name) const
{
    if (name) {
        args += '\'';
        args += name;
        args += "':";
    }
    args += '{';
    return *this;
}

void LldbEngine::Command::endGroup() const
{
    if (args.endsWith(','))
        args.chop(1);
    args += "},";
}

} // namespace Internal
} // namespace Debugger
