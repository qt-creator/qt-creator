// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lldbengine.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggertr.h>
#include <debugger/disassemblerlines.h>
#include <debugger/moduleshandler.h>
#include <debugger/registerhandler.h>
#include <debugger/sourceutils.h>
#include <debugger/stackhandler.h>
#include <debugger/terminal.h>
#include <debugger/threadshandler.h>
#include <debugger/watchhandler.h>
#include <debugger/watchutils.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QJsonArray>
#include <QRegularExpression>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

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
    m_lldbProc.setUseCtrlCStub(true);
    m_lldbProc.setProcessMode(ProcessMode::Writer);

    setObjectName("LldbEngine");
    setDebuggerName("LLDB");

    DebuggerSettings &ds = settings();
    connect(&ds.autoDerefPointers, &BaseAspect::changed, this, &LldbEngine::updateLocals);
    connect(ds.createFullBacktrace.action(), &QAction::triggered,
            this, &LldbEngine::fetchFullBacktrace);
    connect(&ds.useDebuggingHelpers, &BaseAspect::changed, this, &LldbEngine::updateLocals);
    connect(&ds.useDynamicType, &BaseAspect::changed, this, &LldbEngine::updateLocals);
    connect(&ds.intelFlavor, &BaseAspect::changed, this, &LldbEngine::updateAll);

    connect(&m_lldbProc, &Process::started, this, &LldbEngine::handleLldbStarted);
    connect(&m_lldbProc, &Process::done, this, &LldbEngine::handleLldbDone);
    connect(&m_lldbProc, &Process::readyReadStandardOutput,
            this, &LldbEngine::readLldbStandardOutput);
    connect(&m_lldbProc, &Process::readyReadStandardError,
            this, &LldbEngine::readLldbStandardError);

    connect(this, &LldbEngine::outputReady,
            this, &LldbEngine::handleResponse, Qt::QueuedConnection);
}

void LldbEngine::executeDebuggerCommand(const QString &command)
{
    DebuggerCommand cmd("executeDebuggerCommand");
    cmd.arg("command", command);
    runCommand(cmd);
}

void LldbEngine::runCommand(const DebuggerCommand &command)
{
    if (!m_lldbProc.isRunning()) {
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
            showStatusMessage(Tr::tr("Stopping temporarily"), 1000);
            m_onStop.append(cmd, false);
            interruptInferior();
            return;
        }
    }
    showMessage(msg, LogInput);
    m_commandForToken[currentToken()] = cmd;
    executeCommand("script theDumper." + function);
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

void LldbEngine::executeCommand(const QString &command)
{
    // For some reason, sometimes LLDB misses the first character of the next command on Windows
    // if passing only 1 LF.
    m_lldbProc.write(command + "\n\n");
}

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    runCommand({"shutdownInferior"});
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    abortDebuggerProcess();
}

void LldbEngine::abortDebuggerProcess()
{
    if (m_lldbProc.isRunning())
        m_lldbProc.stop();
    else
        notifyEngineShutdownFinished();
}

static QString adapterStartFailed()
{
    return Tr::tr("Adapter start failed.");
}

void LldbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());

    const FilePath lldbCmd = runParameters().debugger.command.executable();

    showMessage("STARTING LLDB: " + lldbCmd.toUserOutput());
    Environment environment = runParameters().debugger.environment;
    environment.appendOrSet("QT_CREATOR_LLDB_PROCESS", "1");
    environment.appendOrSet("PYTHONUNBUFFERED", "1");  // avoid flushing problem on macOS
    DebuggerItem::addAndroidLldbPythonEnv(lldbCmd, environment);

    if (lldbCmd.osType() == OsTypeLinux) {
        // LLDB 14 installation on Ubuntu 22.04 is broken:
        // https://bugs.launchpad.net/ubuntu/+source/llvm-defaults/+bug/1972855
        // Brush over it:
        Process lldbPythonPathFinder;
        lldbPythonPathFinder.setCommand({lldbCmd, {"-P"}});
        lldbPythonPathFinder.start();
        lldbPythonPathFinder.waitForFinished();
        QString lldbPythonPath = lldbPythonPathFinder.cleanedStdOut();
        if (lldbPythonPath.endsWith('\n'))
            lldbPythonPath.chop(1);
        if (lldbPythonPath == "/usr/lib/local/lib/python3.10/dist-packages")
            environment.appendOrSet("PYTHONPATH", "/usr/lib/llvm-14/lib/python3.10/dist-packages");
    }

    m_lldbProc.setEnvironment(environment);

    if (runParameters().debugger.workingDirectory.isDir())
        m_lldbProc.setWorkingDirectory(runParameters().debugger.workingDirectory);

    if (HostOsInfo::isRunningUnderRosetta())
        m_lldbProc.setCommand(CommandLine("/usr/bin/arch", {"-arm64", lldbCmd.toString()}));
    else
        m_lldbProc.setCommand(CommandLine(lldbCmd));

    m_lldbProc.start();
}

void LldbEngine::handleLldbStarted()
{
    m_lldbProc.waitForReadyRead(1000);

    showStatusMessage(Tr::tr("Setting up inferior..."));

    const DebuggerRunParameters &rp = runParameters();

    QString dumperPath = ICore::resourcePath("debugger").path();
    executeCommand("script sys.path.insert(1, '" + dumperPath + "')");
    // This triggers reportState("enginesetupok") or "enginesetupfailed":
    executeCommand("script from lldbbridge import *");

    QString commands = nativeStartupCommands();
    if (!commands.isEmpty())
        executeCommand(commands);

    const FilePath path = settings().extraDumperFile();
    if (!path.isEmpty() && path.isReadableFile()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", path.path());
        runCommand(cmd);
    }

    commands = settings().extraDumperCommands();
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

    const SourcePathMap sourcePathMap = mergePlatformQtPath(rp, settings().sourcePathMap());
    for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd();
         it != cend;
         ++it) {
        executeDebuggerCommand(
                    "settings append target.source-map " + it.key() + ' ' + expand(it.value()));
    }

    for (const FilePath &path : rp.solibSearchPath)
        executeDebuggerCommand("settings append target.exec-search-paths " + path.toString());

    DebuggerCommand cmd2("setupInferior");
    cmd2.arg("executable", rp.inferior.command.executable().path());
    cmd2.arg("breakonmain", rp.breakOnMain);
    cmd2.arg("useterminal", bool(terminal()));
    cmd2.arg("startmode", rp.startMode);
    cmd2.arg("nativemixed", isNativeMixedActive());
    cmd2.arg("workingdirectory", rp.inferior.workingDirectory.path());
    QStringList environment = rp.inferior.environment.toStringList();
    // Prevent lldb from automatically setting OS_ACTIVITY_DT_MODE to mirror
    // NSLog to stderr, as that will also mirror os_log, which we pick up in
    // AppleUnifiedLogger::preventsStderrLogging(), and end up disabling Qt's
    // default stderr logger. We prefer Qt's own stderr logging if we can.
    environment << "IDE_DISABLED_OS_ACTIVITY_DT_MODE=1";
    cmd2.arg("environment", environment);
    cmd2.arg("processargs", toHex(ProcessArgs::splitArgs(rp.inferior.command.arguments(),
                                                         HostOsInfo::hostOs()).join(QChar(0))));
    cmd2.arg("platform", rp.platform);
    cmd2.arg("symbolfile", rp.symbolFile.path());

    if (terminal()) {
        const qint64 attachedPID = terminal()->applicationPid();
        const qint64 attachedMainThreadID = terminal()->applicationMainThreadId();
        const QString msg = (attachedMainThreadID != -1)
                ? QString::fromLatin1("Attaching to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID)
                : QString::fromLatin1("Attaching to %1").arg(attachedPID);
        showMessage(msg, LogMisc);
        cmd2.arg("startmode", DebuggerStartMode::AttachToLocalProcess);
        cmd2.arg("attachpid", attachedPID);
    } else {
        cmd2.arg("startmode", rp.startMode);
        // it is better not to check the start mode on the python sid (as we would have to duplicate the
        // enum values), and thus we assume that if the rp.attachPID is valid we really have to attach
        QTC_CHECK(rp.attachPID.isValid() && (rp.startMode == AttachToRemoteProcess
                                             || rp.startMode == AttachToLocalProcess
                                             || rp.startMode == AttachToRemoteServer));
        cmd2.arg("attachpid", rp.attachPID.pid());
        cmd2.arg("sysroot", rp.deviceSymbolsRoot.isEmpty() ? rp.sysRoot.toString()
                                                           : rp.deviceSymbolsRoot);
        cmd2.arg("remotechannel", ((rp.startMode == AttachToRemoteProcess
                                   || rp.startMode == AttachToRemoteServer)
                                  ? rp.remoteChannel : QString()));
        QTC_CHECK(!rp.continueAfterAttach || (rp.startMode == AttachToRemoteProcess
                                              || rp.startMode == AttachToLocalProcess
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
            cmd3.callback = [this](const DebuggerResponse &) {
                notifyEngineSetupOk();
                runEngine();
            };
            runCommand(cmd3);
        } else {
            notifyEngineSetupFailed();
        }
    };

    cmd2.flags = Silent;
    runCommand(cmd2);

    DebuggerCommand cmd0("setFallbackQtVersion");
    cmd0.arg("version", rp.fallbackQtVersion);
    runCommand(cmd0);
}

void LldbEngine::runEngine()
{
    const DebuggerRunParameters &rp = runParameters();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state(); return);
    showStatusMessage(Tr::tr("Running requested..."), 5000);
    DebuggerCommand cmd("runEngine");
    if (rp.startMode == AttachToCore)
        cmd.arg("coreFile", rp.coreFile.path());
    runCommand(cmd);
}

void LldbEngine::interruptInferior()
{
    showStatusMessage(Tr::tr("Interrupt requested..."), 5000);
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
    cmd.arg("file", data.fileName.path());
    cmd.arg("line", data.textPosition.line);
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
    cmd.arg("file", data.fileName.path());
    cmd.arg("line", data.textPosition.line);
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
        fetchStack(settings().maximalStackDepth());
    };
    runCommand(cmd);
}

bool LldbEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    if (runParameters().startMode == AttachToCore)
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
    bp->setTextPosition({bkpt["line"].toInt(), -1});

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
            loc->params.textPosition.line = location["line"].toInt();
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

void LldbEngine::loadSymbols(const FilePath &moduleName)
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
        const FilePath inferior = runParameters().inferior.command.executable();
        const GdbMi &modules = response.data["modules"];
        ModulesHandler *handler = modulesHandler();
        handler->beginUpdateAll();
        for (const GdbMi &item : modules) {
            Module module;
            module.modulePath = inferior.withNewPath(item["file"].data());
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

void LldbEngine::requestModuleSymbols(const FilePath &moduleName)
{
    DebuggerCommand cmd("fetchSymbols");
    cmd.arg("module", moduleName.path());
    cmd.callback = [moduleName](const DebuggerResponse &response) {
        const GdbMi &symbols = response.data["symbols"];
        const QString module = response.data["module"].data();
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
        showModuleSymbols(moduleName.withNewPath(module), syms);
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
        fetchStack(settings().maximalStackDepth());
        reloadRegisters();
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

    const bool alwaysVerbose = qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PYTHON_VERBOSE");
    const DebuggerSettings &s = settings();
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", s.useDebuggingHelpers());
    cmd.arg("autoderef", s.autoDerefPointers());
    cmd.arg("dyntype", s.useDynamicType());
    cmd.arg("partialvar", params.partialVariable);
    cmd.arg("qobjectnames", s.showQObjectNames());
    cmd.arg("timestamps", s.logTimeStamps());

    StackFrame frame = stackHandler()->currentFrame();
    cmd.arg("context", frame.context);
    cmd.arg("nativemixed", isNativeMixedActive());

    cmd.arg("stringcutoff", s.maximalStringLength());
    cmd.arg("displaystringlimit", s.displayStringLimit());

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

void LldbEngine::handleLldbDone()
{
    if (m_lldbProc.result() == ProcessResult::StartFailed) {
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        ICore::showWarningWithOptions(adapterStartFailed(), Tr::tr("Unable to start LLDB \"%1\": %2")
                 .arg(runParameters().debugger.command.executable().toUserOutput(),
                 m_lldbProc.errorString()));
        return;
    }

    if (m_lldbProc.error() == QProcess::UnknownError) {
        notifyDebuggerProcessFinished(m_lldbProc.resultData(), "LLDB");
        return;
    }

    const QProcess::ProcessError error = m_lldbProc.error();
    showMessage(QString("LLDB PROCESS ERROR: %1").arg(error));
    switch (error) {
    case QProcess::Crashed:
        notifyEngineShutdownFinished();
        break; // will get a processExited() as well
    // impossible case QProcess::FailedToStart:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::Timedout:
    default:
        //setState(EngineShutdownRequested, true);
        AsynchronousMessageBox::critical(Tr::tr("LLDB I/O Error"), errorMessage(error));
        break;
    }
}

QString LldbEngine::errorMessage(QProcess::ProcessError error) const
{
    switch (error) {
        case QProcess::FailedToStart:
            return Tr::tr("The LLDB process failed to start. Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.")
                .arg(runParameters().debugger.command.executable().toUserOutput());
        case QProcess::Crashed:
            return Tr::tr("The LLDB process crashed some time after starting "
                "successfully.");
        case QProcess::Timedout:
            return Tr::tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
        case QProcess::WriteError:
            return Tr::tr("An error occurred when attempting to write "
                "to the LLDB process. For example, the process may not be running, "
                "or it may have closed its input channel.");
        case QProcess::ReadError:
            return Tr::tr("An error occurred when attempting to read from "
                "the Lldb process. For example, the process may not be running.");
        default:
            return Tr::tr("An unknown error in the LLDB process occurred.") + ' ';
    }
}

void LldbEngine::readLldbStandardError()
{
    QString err = QString::fromUtf8(m_lldbProc.readAllRawStandardError());
    qDebug() << "\nLLDB STDERR UNEXPECTED: " << err;
    showMessage("Lldb stderr: " + err, LogError);
}

void LldbEngine::readLldbStandardOutput()
{
    QByteArray outba = m_lldbProc.readAllRawStandardOutput();
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
        if (runParameters().startMode == AttachToCore)
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
    FilePath fileName = FilePath::fromUserInput(reportedLocation["file"].data());
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

void LldbEngine::loadAdditionalQmlStack()
{
    fetchStack(-1, true);
}

void LldbEngine::reloadFullStack()
{
    fetchStack(-1, false);
}

void LldbEngine::fetchStack(int limit, bool extraQml)
{
    DebuggerCommand cmd("fetchStack");
    cmd.arg("nativemixed", isNativeMixedActive());
    cmd.arg("stacklimit", limit);
    cmd.arg("context", stackHandler()->currentFrame().context);
    cmd.arg("extraqml", int(extraQml));
    cmd.callback = [this](const DebuggerResponse &response) {
        const GdbMi &stack = response.data["stack"];
        const bool isFull = !stack["hasmore"].toInt();
        stackHandler()->setFramesAndCurrentIndex(stack["frames"], isFull);
        activateFrame(stackHandler()->currentIndex());
    };
    runCommand(cmd);
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
    cmd.arg("flavor", settings().intelFlavor() ? "intel" : "att");
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
    cmd.callback = [](const DebuggerResponse &response) {
        Internal::openTextEditor("Backtrace $", fromHex(response.data["fulltrace"].data()));
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
        | MemoryAddressCapability
        | AdditionalQmlStackCapability))
        return true;

    if (runParameters().startMode == AttachToCore)
        return false;

    //return cap == SnapshotCapability;
    return false;
}

DebuggerEngine *createLldbEngine()
{
    return new LldbEngine;
}

} // Debugger::Internal
