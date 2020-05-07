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

#include "lldbengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/terminal.h>

#include <debugger/breakhandler.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/disassemblerlines.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/threadshandler.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/qtcprocess.h>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QJsonArray>
#include <QRegularExpression>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

static int &currentToken()
{
    static int token = 0;
    return token;
}

///////////////////////////////////////////////////////////////////////
//
// LldbEngine
//
///////////////////////////////////////////////////////////////////////

LldbEngine::LldbEngine()
{
    setObjectName("LldbEngine");
    setDebuggerName("LLDB");

    connect(action(AutoDerefPointers), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(CreateFullBacktrace), &QAction::triggered,
            this, &LldbEngine::fetchFullBacktrace);
    connect(action(UseDebuggingHelpers), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(UseDynamicType), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(IntelFlavor), &SavedAction::valueChanged,
            this, &LldbEngine::updateAll);

    connect(&m_lldbProc, &QProcess::errorOccurred,
            this, &LldbEngine::handleLldbError);
    connect(&m_lldbProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LldbEngine::handleLldbFinished);
    connect(&m_lldbProc, &QProcess::readyReadStandardOutput,
            this, &LldbEngine::readLldbStandardOutput);
    connect(&m_lldbProc, &QProcess::readyReadStandardError,
            this, &LldbEngine::readLldbStandardError);

    connect(this, &LldbEngine::outputReady,
            this, &LldbEngine::handleResponse, Qt::QueuedConnection);
}

LldbEngine::~LldbEngine()
{
    m_lldbProc.disconnect();
}

void LldbEngine::executeDebuggerCommand(const QString &command)
{
    DebuggerCommand cmd("executeDebuggerCommand");
    cmd.arg("command", command);
    runCommand(cmd);
}

void LldbEngine::runCommand(const DebuggerCommand &command)
{
    if (m_lldbProc.state() != QProcess::Running) {
        // This can legally happen e.g. through a reloadModule()
        // triggered by changes in view visibility.
        showMessage(QString("NO LLDB PROCESS RUNNING, CMD IGNORED: %1 %2")
            .arg(command.function).arg(state()));
        return;
    }
    const int tok = ++currentToken();
    DebuggerCommand cmd = command;
    cmd.arg("token", tok);
    QString token = QString::number(tok);
    QString function = cmd.function + "(" + cmd.argsToPython() + ")";
    QString msg = token + function + '\n';
    if (cmd.flags == Silent)
        msg.replace(QRegularExpression("\"environment\":.[^]]*."), "<environment suppressed>");
    if (cmd.flags == NeedsFullStop) {
        cmd.flags &= ~NeedsFullStop;
        if (state() == InferiorRunOk) {
            showStatusMessage(tr("Stopping temporarily"), 1000);
            m_onStop.append(cmd, false);
            interruptInferior();
            return;
        }
    }
    showMessage(msg, LogInput);
    m_commandForToken[currentToken()] = cmd;
    m_lldbProc.write("script theDumper." + function.toUtf8() + "\n");
}

void LldbEngine::debugLastCommand()
{
    runCommand(m_lastDebuggableCommand);
}

void LldbEngine::handleAttachedToCore()
{
    QTC_ASSERT(state() == InferiorUnrunnable, qDebug() << state();return);
    showMessage("Attached to core.");
    reloadFullStack();
    reloadModules();
    updateLocals();
}

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    runCommand({"shutdownInferior"});
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    if (m_lldbProc.state() == QProcess::Running)
        m_lldbProc.terminate();
    else
        notifyEngineShutdownFinished();
}

void LldbEngine::abortDebuggerProcess()
{
    if (m_lldbProc.state() == QProcess::Running)
        m_lldbProc.kill();
    else
        notifyEngineShutdownFinished();
}

static QString adapterStartFailed()
{
    return LldbEngine::tr("Adapter start failed.");
}

void LldbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    const FilePath lldbCmd = runParameters().debugger.executable;

    showMessage("STARTING LLDB: " + lldbCmd.toUserOutput());
    Environment environment = runParameters().debugger.environment;
    environment.appendOrSet("PYTHONUNBUFFERED", "1");  // avoid flushing problem on macOS
    m_lldbProc.setEnvironment(environment);
    if (QFileInfo(runParameters().debugger.workingDirectory).isDir())
        m_lldbProc.setWorkingDirectory(runParameters().debugger.workingDirectory);

    m_lldbProc.setCommand(CommandLine(lldbCmd));
    m_lldbProc.start();

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start LLDB \"%1\": %2")
            .arg(lldbCmd.toUserOutput(), m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        if (!msg.isEmpty())
            ICore::showWarningWithOptions(adapterStartFailed(), msg);
        return;
    }
    m_lldbProc.waitForReadyRead(1000);

    showStatusMessage(tr("Setting up inferior..."));

    const QByteArray dumperSourcePath =
        ICore::resourcePath().toLocal8Bit() + "/debugger/";

    m_lldbProc.write("script sys.path.insert(1, '" + dumperSourcePath + "')\n");
    // This triggers reportState("enginesetupok") or "enginesetupfailed":
    m_lldbProc.write("script from lldbbridge import *\n");

    QString commands = nativeStartupCommands();
    if (!commands.isEmpty())
        m_lldbProc.write(commands.toLocal8Bit() + '\n');


    const QString path = stringSetting(ExtraDumperFile);
    if (!path.isEmpty() && QFileInfo(path).isReadable()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", path);
        runCommand(cmd);
    }

    commands = stringSetting(ExtraDumperCommands);
    if (!commands.isEmpty()) {
        DebuggerCommand cmd("executeDebuggerCommand");
        cmd.arg("command", commands);
        runCommand(cmd);
    }

    DebuggerCommand cmd1("loadDumpers");
    cmd1.callback = [this](const DebuggerResponse &response) {
        watchHandler()->addDumpers(response.data["dumpers"]);
    };
    runCommand(cmd1);

    const DebuggerRunParameters &rp = runParameters();

    const SourcePathMap sourcePathMap =
        DebuggerSourcePathMappingWidget::mergePlatformQtPath(rp,
                Internal::globalDebuggerOptions()->sourcePathMap);
    for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd();
         it != cend;
         ++it) {
        executeDebuggerCommand(
                    "settings append target.source-map " + it.key() + ' ' + expand(it.value()));
    }

    DebuggerCommand cmd2("setupInferior");
    cmd2.arg("executable", rp.inferior.executable.toString());
    cmd2.arg("breakonmain", rp.breakOnMain);
    cmd2.arg("useterminal", bool(terminal()));
    cmd2.arg("startmode", rp.startMode);
    cmd2.arg("nativemixed", isNativeMixedActive());
    cmd2.arg("workingdirectory", rp.inferior.workingDirectory);
    cmd2.arg("environment", rp.inferior.environment.toStringList());
    cmd2.arg("processargs", toHex(QtcProcess::splitArgs(rp.inferior.commandLineArguments).join(QChar(0))));
    cmd2.arg("platform", rp.platform);
    cmd2.arg("symbolfile", rp.symbolFile);

    if (terminal()) {
        const qint64 attachedPID = terminal()->applicationPid();
        const qint64 attachedMainThreadID = terminal()->applicationMainThreadId();
        const QString msg = (attachedMainThreadID != -1)
                ? QString::fromLatin1("Attaching to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID)
                : QString::fromLatin1("Attaching to %1").arg(attachedPID);
        showMessage(msg, LogMisc);
        cmd2.arg("attachpid", attachedPID);

    } else {

        cmd2.arg("startmode", rp.startMode);
        // it is better not to check the start mode on the python sid (as we would have to duplicate the
        // enum values), and thus we assume that if the rp.attachPID is valid we really have to attach
        QTC_CHECK(!rp.attachPID.isValid() || (rp.startMode == AttachCrashedExternal
                                              || rp.startMode == AttachExternal));
        cmd2.arg("attachpid", rp.attachPID.pid());
        cmd2.arg("sysroot", rp.deviceSymbolsRoot.isEmpty() ? rp.sysRoot.toString()
                                                           : rp.deviceSymbolsRoot);
        cmd2.arg("remotechannel", ((rp.startMode == AttachToRemoteProcess
                                   || rp.startMode == AttachToRemoteServer)
                                  ? rp.remoteChannel : QString()));
        cmd2.arg("platform", rp.platform);
        QTC_CHECK(!rp.continueAfterAttach || (rp.startMode == AttachToRemoteProcess
                                              || rp.startMode == AttachExternal
                                              || rp.startMode == AttachToRemoteServer));
        m_continueAtNextSpontaneousStop = false;
    }

    cmd2.callback = [this](const DebuggerResponse &response) {
        const bool success = response.data["success"].toInt();
        if (success) {
            BreakpointManager::claimBreakpointsForEngine(this);
            // Some extra roundtrip to make sure we end up behind all commands triggered
            // from claimBreakpointsForEngine().
            DebuggerCommand cmd3("executeRoundtrip");
            cmd3.callback = [this](const DebuggerResponse &) { notifyEngineSetupOk(); };
            runCommand(cmd3);
        } else {
            notifyEngineSetupFailed();
        }
    };

    cmd2.flags = Silent;
    runCommand(cmd2);
}

void LldbEngine::runEngine()
{
    const DebuggerRunParameters &rp = runParameters();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state(); return);
    showStatusMessage(tr("Running requested..."), 5000);
    DebuggerCommand cmd("runEngine");
    if (rp.startMode == AttachCore)
        cmd.arg("coreFile", rp.coreFile);
    runCommand(cmd);
}

void LldbEngine::interruptInferior()
{
    showStatusMessage(tr("Interrupt requested..."), 5000);
    runCommand({"interruptInferior"});
}

void LldbEngine::executeStepIn(bool byInstruction)
{
    notifyInferiorRunRequested();
    runCommand({QLatin1String(byInstruction ? "executeStepI" : "executeStep")});
}

void LldbEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    runCommand({"executeStepOut"});
}

void LldbEngine::executeStepOver(bool byInstruction)
{
    notifyInferiorRunRequested();
    runCommand({QLatin1String(byInstruction ? "executeNextI" : "executeNext")});
}

void LldbEngine::continueInferior()
{
    notifyInferiorRunRequested();
    DebuggerCommand cmd("continueInferior");
    cmd.callback = [this](const DebuggerResponse &response) {
        if (response.resultClass == ResultError)
            notifyEngineIll();
    };
    runCommand(cmd);
}

void LldbEngine::handleResponse(const QString &response)
{
    GdbMi all;
    all.fromStringMultiple(response);

    for (const GdbMi &item : all) {
        const QString name = item.name();
        if (name == "result") {
            QString msg = item["status"].data();
            if (!msg.isEmpty())
                msg[0] = msg.at(0).toUpper();
            showStatusMessage(msg);

            int token = item["token"].toInt();
            showMessage(QString("%1^").arg(token), LogOutput);
            if (m_commandForToken.contains(token)) {
                DebuggerCommand cmd = m_commandForToken.take(token);
                DebuggerResponse response;
                response.token = token;
                response.data = item;
                if (cmd.callback)
                    cmd.callback(response);
            }
        } else if (name == "state")
            handleStateNotification(all);
        else if (name == "location")
            handleLocationNotification(item);
        else if (name == "output")
            handleOutputNotification(item);
        else if (name == "pid")
            notifyInferiorPid(item.toProcessHandle());
        else if (name == "breakpointmodified")
            handleInterpreterBreakpointModified(item);
        else if (name == "bridgemessage")
            showMessage(item["msg"].data(), item["channel"].toInt());
    }
}

void LldbEngine::executeRunToLine(const ContextData &data)
{
    notifyInferiorRunRequested();
    DebuggerCommand cmd("executeRunToLocation");
    cmd.arg("file", data.fileName);
    cmd.arg("line", data.lineNumber);
    cmd.arg("address", data.address);
    runCommand(cmd);
}

void LldbEngine::executeRunToFunction(const QString &functionName)
{
    notifyInferiorRunRequested();
    DebuggerCommand cmd("executeRunToFunction");
    cmd.arg("function", functionName);
    runCommand(cmd);
}

void LldbEngine::executeJumpToLine(const ContextData &data)
{
    DebuggerCommand cmd("executeJumpToLocation");
    cmd.arg("file", data.fileName);
    cmd.arg("line", data.lineNumber);
    cmd.arg("address", data.address);
    runCommand(cmd);
}

void LldbEngine::activateFrame(int frameIndex)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    if (handler->isSpecialFrame(frameIndex)) {
        fetchStack(handler->stackSize() * 10 + 3);
        return;
    }

    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoCurrentLocation();

    DebuggerCommand cmd("activateFrame");
    cmd.arg("index", frameIndex);
    if (Thread thread = threadsHandler()->currentThread())
        cmd.arg("thread", thread->id());
    runCommand(cmd);

    updateLocals();
    reloadRegisters();
}

void LldbEngine::selectThread(const Thread &thread)
{
    QTC_ASSERT(thread, return);
    DebuggerCommand cmd("selectThread");
    cmd.arg("id", thread->id());
    cmd.callback = [this](const DebuggerResponse &) {
        fetchStack(action(MaximalStackDepth)->value().toInt());
    };
    runCommand(cmd);
}

bool LldbEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case EngineSetupRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
        return true;
    default:
        return false;
    }
}

bool LldbEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    if (runParameters().startMode == AttachCore)
        return false;
    if (bp.isCppBreakpoint())
        return true;
    return isNativeMixedEnabled();
}

void LldbEngine::insertBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    DebuggerCommand cmd("insertBreakpoint");
    cmd.callback = [this, bp](const DebuggerResponse &response) {
        QTC_CHECK(bp && bp->state() == BreakpointInsertionProceeding);
        updateBreakpointData(bp, response.data, true);
    };
    bp->addToCommand(&cmd);
    notifyBreakpointInsertProceeding(bp);
    runCommand(cmd);
}

void LldbEngine::updateBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    DebuggerCommand cmd("changeBreakpoint");
    cmd.arg("lldbid", bp->responseId());
    cmd.callback = [this, bp](const DebuggerResponse &response) {
        QTC_CHECK(bp && bp->state() == BreakpointUpdateProceeding);
        updateBreakpointData(bp, response.data, false);
    };
    bp->addToCommand(&cmd);
    notifyBreakpointChangeProceeding(bp);
    runCommand(cmd);
}

void LldbEngine::enableSubBreakpoint(const SubBreakpoint &sbp, bool on)
{
    QTC_ASSERT(sbp, return);
    Breakpoint bp = sbp->breakpoint();
    QTC_ASSERT(bp, return);
    DebuggerCommand cmd("enableSubbreakpoint");
    cmd.arg("lldbid", bp->responseId());
    cmd.arg("locid", sbp->responseId);
    cmd.arg("enabled", on);
    cmd.callback = [bp, sbp](const DebuggerResponse &response) {
        QTC_ASSERT(sbp, return);
        QTC_ASSERT(bp, return);
        if (response.resultClass == ResultDone) {
            sbp->params.enabled = response.data["enabled"].toInt();
            bp->adjustMarker();
        }
    };
    runCommand(cmd);
}

void LldbEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    if (!bp->responseId().isEmpty()) {
        DebuggerCommand cmd("removeBreakpoint");
        cmd.arg("lldbid", bp->responseId());
        notifyBreakpointRemoveProceeding(bp);
        runCommand(cmd);

        // Pretend it succeeds without waiting for response. Feels better.
        // Otherwise, clicking in the gutter leaves the breakpoint visible
        // for quite some time, so the user assumes a mis-click and clicks
        // again, effectivly re-introducing the breakpoint.
        notifyBreakpointRemoveOk(bp);
    }
}

void LldbEngine::updateBreakpointData(const Breakpoint &bp, const GdbMi &bkpt, bool added)
{
    QTC_ASSERT(bp, return);
    QString rid = bkpt["lldbid"].data();
    QTC_ASSERT(bp, return);
    if (added)
        bp->setResponseId(rid);
    QTC_CHECK(bp->responseId() == rid);
    bp->setAddress(0);
    bp->setEnabled(bkpt["enabled"].toInt());
    bp->setIgnoreCount(bkpt["ignorecount"].toInt());
    bp->setCondition(fromHex(bkpt["condition"].data()));
    bp->setHitCount(bkpt["hitcount"].toInt());
    bp->setFileName(FilePath::fromUserInput(bkpt["file"].data()));
    bp->setLineNumber(bkpt["line"].toInt());

    GdbMi locations = bkpt["locations"];
    const int numChild = locations.childCount();
    if (numChild > 1) {
        for (const GdbMi &location : locations) {
            const QString locid = location["locid"].data();
            SubBreakpoint loc = bp->findOrCreateSubBreakpoint(locid);
            QTC_ASSERT(loc, continue);
            loc->params.type = bp->type();
            loc->params.address = location["addr"].toAddress();
            loc->params.functionName = location["function"].data();
            loc->params.fileName = FilePath::fromUserInput(location["file"].data());
            loc->params.lineNumber = location["line"].toInt();
            loc->displayName = QString("%1.%2").arg(bp->responseId()).arg(locid);
        }
        bp->setPending(false);
    } else if (numChild == 1) {
        const GdbMi location = locations.childAt(0);
        bp->setAddress(location["addr"].toAddress());
        bp->setFunctionName(location["function"].data());
        bp->setPending(false);
    } else {
        // This can happen for pending breakpoints.
        showMessage(QString("NO LOCATIONS (YET) FOR BP %1").arg(bp->parameters().toString()));
    }
    bp->adjustMarker();
    if (added)
        notifyBreakpointInsertOk(bp);
    else
        notifyBreakpointChangeOk(bp);
}

void LldbEngine::handleOutputNotification(const GdbMi &output)
{
    QString channel = output["channel"].data();
    QString data = fromHex(output["data"].data());
    LogChannel ch = AppStuff;
    if (channel == "stdout")
        ch = AppOutput;
    else if (channel == "stderr")
        ch = AppError;
    showMessage(data, ch);
}

void LldbEngine::handleInterpreterBreakpointModified(const GdbMi &bpItem)
{
    QTC_ASSERT(bpItem.childCount(), return);
    QString id = bpItem.childAt(0).m_data;

    Breakpoint bp = breakHandler()->findBreakpointByResponseId(id);
    if (!bp)        // FIXME adapt whole bp handling and turn into soft assert
        return;

    // this function got triggered by a lldb internal breakpoint event
    // avoid asserts regarding unexpected state transitions
    switch (bp->state()) {
    case BreakpointInsertionRequested:  // was a pending bp
        bp->gotoState(BreakpointInsertionProceeding, BreakpointInsertionRequested);
        break;
    case BreakpointInserted:            // was an inserted, gets updated now
        bp->gotoState(BreakpointUpdateRequested, BreakpointInserted);
        notifyBreakpointChangeProceeding(bp);
        break;
    default:
        break;
    }
    updateBreakpointData(bp, bpItem, false);
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
    DebuggerCommand cmd("fetchModules");
    cmd.callback = [this](const DebuggerResponse &response) {
        const GdbMi &modules = response.data["modules"];
        ModulesHandler *handler = modulesHandler();
        handler->beginUpdateAll();
        for (const GdbMi &item : modules) {
            Module module;
            module.modulePath = item["file"].data();
            module.moduleName = item["name"].data();
            module.symbolsRead = Module::UnknownReadState;
            module.startAddress = item["loaded_addr"].toAddress();
            module.endAddress = 0; // FIXME: End address not easily available.
            handler->updateModule(module);
        }
        handler->endUpdateAll();
    };
    runCommand(cmd);
}

void LldbEngine::requestModuleSymbols(const QString &moduleName)
{
    DebuggerCommand cmd("fetchSymbols");
    cmd.arg("module", moduleName);
    cmd.callback = [moduleName](const DebuggerResponse &response) {
        const GdbMi &symbols = response.data["symbols"];
        QString moduleName = response.data["module"].data();
        Symbols syms;
        for (const GdbMi &item : symbols) {
            Symbol symbol;
            symbol.address = item["address"].data();
            symbol.name = item["name"].data();
            symbol.state = item["state"].data();
            symbol.section = item["section"].data();
            symbol.demangled = item["demangled"].data();
            syms.append(symbol);
        }
        showModuleSymbols(moduleName, syms);
    };
    runCommand(cmd);
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

bool LldbEngine::canHandleToolTip(const DebuggerToolTipContext &context) const
{
   return state() == InferiorStopOk && context.isCppEditor;
}

void LldbEngine::updateAll()
{
    DebuggerCommand cmd("fetchThreads");
    cmd.callback = [this](const DebuggerResponse &response) {
        threadsHandler()->setThreads(response.data);
        fetchStack(action(MaximalStackDepth)->value().toInt());
        reloadRegisters();
    };
    runCommand(cmd);
}

void LldbEngine::reloadFullStack()
{
    fetchStack(-1);
}

void LldbEngine::fetchStack(int limit)
{
    DebuggerCommand cmd("fetchStack");
    cmd.arg("nativemixed", isNativeMixedActive());
    cmd.arg("stacklimit", limit);
    cmd.arg("context", stackHandler()->currentFrame().context);
    cmd.callback = [this](const DebuggerResponse &response) {
        const GdbMi &stack = response.data["stack"];
        const bool isFull = !stack["hasmore"].toInt();
        stackHandler()->setFramesAndCurrentIndex(stack["frames"], isFull);
        activateFrame(stackHandler()->currentIndex());
    };
    runCommand(cmd);
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void LldbEngine::assignValueInDebugger(WatchItem *item,
    const QString &expression, const QVariant &value)
{
    DebuggerCommand cmd("assignValue");
    cmd.arg("expr", toHex(expression));
    cmd.arg("value", toHex(value.toString()));
    cmd.arg("type", toHex(item->type));
    cmd.arg("simpleType", isIntOrFloatType(item->type));
    cmd.callback = [this](const DebuggerResponse &) { updateLocals(); };
    runCommand(cmd);
}

void LldbEngine::doUpdateLocals(const UpdateParameters &params)
{
    watchHandler()->notifyUpdateStarted(params);

    DebuggerCommand cmd("fetchVariables");
    watchHandler()->appendFormatRequests(&cmd);
    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

    const static bool alwaysVerbose = qEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE");
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", boolSetting(UseDebuggingHelpers));
    cmd.arg("autoderef", boolSetting(AutoDerefPointers));
    cmd.arg("dyntype", boolSetting(UseDynamicType));
    cmd.arg("partialvar", params.partialVariable);
    cmd.arg("qobjectnames", boolSetting(ShowQObjectNames));
    cmd.arg("timestamps", boolSetting(LogTimeStamps));

    StackFrame frame = stackHandler()->currentFrame();
    cmd.arg("context", frame.context);
    cmd.arg("nativemixed", isNativeMixedActive());

    cmd.arg("stringcutoff", action(MaximalStringLength)->value().toString());
    cmd.arg("displaystringlimit", action(DisplayStringLimit)->value().toString());

    //cmd.arg("resultvarname", m_resultVarName);
    cmd.arg("partialvar", params.partialVariable);

    m_lastDebuggableCommand = cmd;
    m_lastDebuggableCommand.arg("passexceptions", "1");

    cmd.callback = [this](const DebuggerResponse &response) {
        updateLocalsView(response.data);
        watchHandler()->notifyUpdateFinished();
        updateToolTips();
    };

    runCommand(cmd);
}

void LldbEngine::handleLldbError(QProcess::ProcessError error)
{
    showMessage(QString("LLDB PROCESS ERROR: %1").arg(error));
    switch (error) {
    case QProcess::Crashed:
        m_lldbProc.disconnect();
        notifyEngineShutdownFinished();
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        m_lldbProc.kill();
        AsynchronousMessageBox::critical(tr("LLDB I/O Error"), errorMessage(error));
        break;
    }
}

QString LldbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return tr("The LLDB process failed to start. Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(runParameters().debugger.executable.toUserOutput());
        case QProcess::Crashed:
            return tr("The LLDB process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return tr("An error occurred when attempting to write "
                "to the LLDB process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return tr("An error occurred when attempting to read from "
                "the Lldb process. For example, the process may not be running.");
        default:
            return tr("An unknown error in the LLDB process occurred.") + ' ';
    }
}

void LldbEngine::handleLldbFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    notifyDebuggerProcessFinished(exitCode, exitStatus, "LLDB");
}

void LldbEngine::readLldbStandardError()
{
    QString err = QString::fromUtf8(m_lldbProc.readAllStandardError());
    qDebug() << "\nLLDB STDERR UNEXPECTED: " << err;
    showMessage("Lldb stderr: " + err, LogError);
}

void LldbEngine::readLldbStandardOutput()
{
    QByteArray outba = m_lldbProc.readAllStandardOutput();
    outba.replace("\r\n", "\n");
    QString out = QString::fromUtf8(outba);
    showMessage(out, LogOutput);
    m_inbuffer.append(out);
    while (true) {
        int pos = m_inbuffer.indexOf("@\n");
        if (pos == -1)
            break;
        QString response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 2);
        emit outputReady(response);
    }
}

void LldbEngine::handleStateNotification(const GdbMi &item)
{
    const QString newState = item["state"].data();
    if (newState == "running")
        notifyInferiorRunOk();
    else if (newState == "inferiorrunfailed")
        notifyInferiorRunFailed();
    else if (newState == "continueafternextstop")
        m_continueAtNextSpontaneousStop = true;
    else if (newState == "stopped") {
        notifyInferiorSpontaneousStop();
        if (m_onStop.isEmpty()) {
            if (m_continueAtNextSpontaneousStop) {
                m_continueAtNextSpontaneousStop = false;
                continueInferior();
            } else {
                updateAll();
            }
        } else {
            showMessage("HANDLING QUEUED COMMANDS AFTER TEMPORARY STOP", LogMisc);
            DebuggerCommandSequence seq = m_onStop;
            m_onStop = DebuggerCommandSequence();
            for (const DebuggerCommand &cmd : seq.commands())
                runCommand(cmd);
            if (seq.wantContinue())
                continueInferior();
        }
    } else if (newState == "inferiorstopok") {
        notifyInferiorStopOk();
        if (!isDying())
            updateAll();
    } else if (newState == "inferiorstopfailed")
        notifyInferiorStopFailed();
    else if (newState == "inferiorill")
        notifyInferiorIll();
    else if (newState == "enginesetupfailed") {
        Core::AsynchronousMessageBox::critical(adapterStartFailed(),
                                               item["error"].data());
        notifyEngineSetupFailed();
    } else if (newState == "enginerunfailed")
        notifyEngineRunFailed();
    else if (newState == "enginerunandinferiorrunok") {
        if (runParameters().continueAfterAttach)
            m_continueAtNextSpontaneousStop = true;
        notifyEngineRunAndInferiorRunOk();
    } else if (newState == "enginerunandinferiorstopok") {
        notifyEngineRunAndInferiorStopOk();
        continueInferior();
    } else if (newState == "enginerunokandinferiorunrunnable") {
        notifyEngineRunOkAndInferiorUnrunnable();
        if (runParameters().startMode == AttachCore)
            handleAttachedToCore();
    } else if (newState == "inferiorshutdownfinished")
        notifyInferiorShutdownFinished();
    else if (newState == "engineshutdownfinished")
        notifyEngineShutdownFinished();
    else if (newState == "inferiorexited")
        notifyInferiorExited();
}

void LldbEngine::handleLocationNotification(const GdbMi &reportedLocation)
{
    qulonglong address = reportedLocation["address"].toAddress();
    Utils::FilePath fileName = FilePath::fromUserInput(reportedLocation["file"].data());
    QString function = reportedLocation["function"].data();
    int lineNumber = reportedLocation["line"].toInt();
    Location loc = Location(fileName, lineNumber);
    if (operatesByInstruction() || !fileName.exists() || lineNumber <= 0) {
        loc = Location(address);
        loc.setNeedsMarker(true);
        loc.setUseAssembler(true);
    }

    // Quickly set the location marker.
    if (lineNumber > 0 && fileName.exists() && function != "::qt_qmlDebugMessageAvailable()")
        gotoLocation(Location(fileName, lineNumber));
}

void LldbEngine::reloadRegisters()
{
    if (!isRegistersWindowVisible())
        return;

    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    DebuggerCommand cmd("fetchRegisters");
    cmd.callback = [this](const DebuggerResponse &response) {
        RegisterHandler *handler = registerHandler();
        for (const GdbMi &item : response.data["registers"]) {
            Register reg;
            reg.name = item["name"].data();
            reg.value.fromString(item["value"].data(), HexadecimalFormat);
            reg.size = item["size"].data().toInt();
            reg.reportedType = item["type"].data();
            if (reg.reportedType.startsWith("unsigned"))
                reg.kind = IntegerRegister;
            handler->updateRegister(reg);
        }
        handler->commitUpdates();
    };
    runCommand(cmd);
}

void LldbEngine::reloadDebuggingHelpers()
{
    runCommand({"reloadDumpers"});
    updateAll();
}

void LldbEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    QPointer<DisassemblerAgent> p(agent);
    int id = m_disassemblerAgents.value(p, -1);
    if (id == -1) {
        id = ++m_lastAgentId;
        m_disassemblerAgents.insert(p, id);
    }
    const Location &loc = agent->location();
    DebuggerCommand cmd("fetchDisassembler");
    cmd.arg("address", loc.address());
    cmd.arg("function", loc.functionName());
    cmd.arg("flavor", boolSetting(IntelFlavor) ? "intel" : "att");
    cmd.callback = [this, id](const DebuggerResponse &response) {
        DisassemblerLines result;
        QPointer<DisassemblerAgent> agent = m_disassemblerAgents.key(id);
        if (!agent.isNull()) {
            for (const GdbMi &line : response.data["lines"]) {
                DisassemblerLine dl;
                dl.address = line["address"].toAddress();
                //dl.data = line["data"].data();
                //dl.rawData = line["rawdata"].data();
                dl.data = line["rawdata"].data();
                if (!dl.data.isEmpty())
                    dl.data += QString(30 - dl.data.size(), ' ');
                dl.data += fromHex(line["hexdata"].data());
                dl.data += line["data"].data();
                dl.offset = line["offset"].toInt();
                dl.lineNumber = line["line"].toInt();
                dl.fileName = line["file"].data();
                dl.function = line["function"].data();
                dl.hunk = line["hunk"].toInt();
                QString comment = fromHex(line["comment"].data());
                if (!comment.isEmpty())
                    dl.data += " # " + comment;
                result.appendLine(dl);
            }
            agent->setContents(result);
        }
    };
    runCommand(cmd);
}

void LldbEngine::fetchFullBacktrace()
{
    DebuggerCommand cmd("fetchFullBacktrace");
    cmd.callback = [](const DebuggerResponse &response)  {
        Internal::openTextEditor("Backtrace $", fromHex(response.data.data()));
    };
    runCommand(cmd);
}

void LldbEngine::fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length)
{
    DebuggerCommand cmd("fetchMemory");
    cmd.arg("address", addr);
    cmd.arg("length", length);
    cmd.callback = [agent](const DebuggerResponse &response) {
        qulonglong addr = response.data["address"].toAddress();
        QByteArray ba = QByteArray::fromHex(response.data["contents"].data().toUtf8());
        agent->addData(addr, ba);
    };
    runCommand(cmd);
}

void LldbEngine::changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data)
{
    Q_UNUSED(agent)
    DebuggerCommand cmd("writeMemory");
    cmd.arg("address", addr);
    cmd.arg("data", QString::fromUtf8(data.toHex()));
    cmd.callback = [](const DebuggerResponse &response) { Q_UNUSED(response) };
    runCommand(cmd);
}

void LldbEngine::setRegisterValue(const QString &name, const QString &value)
{
    DebuggerCommand cmd("setRegister");
    cmd.arg("name", name);
    cmd.arg("value", value);
    runCommand(cmd);
}

bool LldbEngine::hasCapability(unsigned cap) const
{
    if (cap & (0
        //| ReverseSteppingCapability
        | AutoDerefPointersCapability
        | DisassemblerCapability
        | RegisterCapability
        | ShowMemoryCapability
        | JumpToLineCapability
        | ReloadModuleCapability
        | ReloadModuleSymbolsCapability
        | BreakOnThrowAndCatchCapability
        | BreakConditionCapability
        | BreakIndividualLocationsCapability
        | TracePointCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointByAddressCapability
        | WatchpointByExpressionCapability
        | WatchWidgetsCapability
        | AddWatcherCapability
        | ShowModuleSymbolsCapability
        | ShowModuleSectionsCapability
        | CatchCapability
        | OperateByInstructionCapability
        | RunToLineCapability
        | WatchComplexExpressionsCapability
        | MemoryAddressCapability))
        return true;

    if (runParameters().startMode == AttachCore)
        return false;

    //return cap == SnapshotCapability;
    return false;
}

DebuggerEngine *createLldbEngine()
{
    return new LldbEngine;
}

} // namespace Internal
} // namespace Debugger
