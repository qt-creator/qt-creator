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
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggertooltipmanager.h>

#include <debugger/breakhandler.h>
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

LldbEngine::LldbEngine(const DebuggerRunParameters &startParameters)
    : DebuggerEngine(startParameters), m_continueAtNextSpontaneousStop(false)
{
    m_lastAgentId = 0;
    setObjectName(QLatin1String("LldbEngine"));

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
}

LldbEngine::~LldbEngine()
{
    m_stubProc.disconnect(); // Avoid spurious state transitions from late exiting stub
    m_lldbProc.disconnect();
}

void LldbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages)
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
            requestInterruptInferior();
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

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    runCommand({"shutdownInferior"});
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_lldbProc.kill();
    if (runParameters().useTerminal)
        m_stubProc.stop();
    notifyEngineShutdownOk();
}

void LldbEngine::abortDebugger()
{
    if (targetState() == DebuggerFinished) {
        // We already tried. Try harder.
        showMessage("ABORTING DEBUGGER. SECOND TIME.");
        m_lldbProc.kill();
    } else {
        // Be friendly the first time. This will change targetState().
        showMessage("ABORTING DEBUGGER. FIRST TIME.");
        quitDebugger();
    }
}

// FIXME: Merge with GdbEngine/QtcProcess
bool LldbEngine::prepareCommand()
{
    if (HostOsInfo::isWindowsHost()) {
        DebuggerRunParameters &rp = runParameters();
        QtcProcess::SplitError perr;
        rp.inferior.commandLineArguments
                = QtcProcess::prepareArgs(rp.inferior.commandLineArguments, &perr, HostOsInfo::hostOs(),
                                          nullptr, &rp.inferior.workingDirectory).toWindowsArgs();
        if (perr != QtcProcess::SplitOk) {
            // perr == BadQuoting is never returned on Windows
            // FIXME? QTCREATORBUG-2809
            notifyEngineSetupFailed();
            return false;
        }
    }
    return true;
}

void LldbEngine::setupEngine()
{
    // FIXME: We can't handle terminals yet.
    if (runParameters().useTerminal) {
        qWarning("Run in Terminal is not supported yet with the LLDB backend");
        showMessage(tr("Run in Terminal is not supported with the LLDB backend."), AppError);
        runParameters().useTerminal = false;
    }

    if (runParameters().useTerminal) {
        QTC_CHECK(false); // See above.
        if (HostOsInfo::isWindowsHost()) {
            // Windows up to xp needs a workaround for attaching to freshly started processes. see proc_stub_win
            if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
                m_stubProc.setMode(ConsoleProcess::Suspend);
            else
                m_stubProc.setMode(ConsoleProcess::Debug);
        } else {
            m_stubProc.setMode(ConsoleProcess::Debug);
            m_stubProc.setSettings(ICore::settings());
        }

        QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
        showMessage("TRYING TO START ADAPTER");

    // Currently, adapters are not re-used
    //    // We leave the console open, so recycle it now.
    //    m_stubProc.blockSignals(true);
    //    m_stubProc.stop();
    //    m_stubProc.blockSignals(false);

        if (!prepareCommand()) {
            notifyEngineSetupFailed();
            return;
        }

        m_stubProc.setWorkingDirectory(runParameters().inferior.workingDirectory);
        // Set environment + dumper preload.
        m_stubProc.setEnvironment(runParameters().stubEnvironment);

        connect(&m_stubProc, &ConsoleProcess::processError, this, &LldbEngine::stubError);
        connect(&m_stubProc, &ConsoleProcess::processStarted, this, &LldbEngine::stubStarted);
        connect(&m_stubProc, &ConsoleProcess::stubStopped, this, &LldbEngine::stubExited);
        // FIXME: Starting the stub implies starting the inferior. This is
        // fairly unclean as far as the state machine and error reporting go.

        if (!m_stubProc.start(runParameters().inferior.executable,
                             runParameters().inferior.commandLineArguments)) {
            // Error message for user is delivered via a signal.
            //handleAdapterStartFailed(QString());
            notifyEngineSetupFailed();
            return;
        }

    } else {
        QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
        if (runParameters().remoteSetupNeeded)
            notifyEngineRequestRemoteSetup();
        else
            startLldb();
    }
}

void LldbEngine::startLldb()
{
    QString lldbCmd = runParameters().debugger.executable;
    connect(&m_lldbProc, &QProcess::errorOccurred, this, &LldbEngine::handleLldbError);
    connect(&m_lldbProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &LldbEngine::handleLldbFinished);
    connect(&m_lldbProc, &QProcess::readyReadStandardOutput,
            this, &LldbEngine::readLldbStandardOutput);
    connect(&m_lldbProc, &QProcess::readyReadStandardError,
            this, &LldbEngine::readLldbStandardError);

    connect(this, &LldbEngine::outputReady,
            this, &LldbEngine::handleResponse, Qt::QueuedConnection);

    showMessage("STARTING LLDB: " + lldbCmd);
    m_lldbProc.setEnvironment(runParameters().debugger.environment);
    if (QFileInfo(runParameters().debugger.workingDirectory).isDir())
        m_lldbProc.setWorkingDirectory(runParameters().debugger.workingDirectory);

    m_lldbProc.setCommand(lldbCmd, QString());
    m_lldbProc.start();

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start LLDB \"%1\": %2")
            .arg(lldbCmd, m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage("ADAPTER START FAILED");
        if (!msg.isEmpty())
            ICore::showWarningWithOptions(tr("Adapter start failed."), msg);
        return;
    }
    m_lldbProc.waitForReadyRead(1000);
    m_lldbProc.write("sc print('@\\nlldbstartupok@\\n')\n");
}

// FIXME: splitting of startLldb() necessary to support LLDB <= 310 - revert asap
void LldbEngine::startLldbStage2()
{
    showMessage("ADAPTER STARTED");
    showStatusMessage(tr("Setting up inferior..."));

    const QByteArray dumperSourcePath =
        ICore::resourcePath().toLocal8Bit() + "/debugger/";

    m_lldbProc.write("script sys.path.insert(1, '" + dumperSourcePath + "')\n");
    m_lldbProc.write("script from lldbbridge import *\n");
    m_lldbProc.write("script print(dir())\n");
    m_lldbProc.write("script theDumper = Dumper()\n"); // This triggers reportState("enginesetupok")

    const QString commands = nativeStartupCommands();
    if (!commands.isEmpty())
        m_lldbProc.write(commands.toLocal8Bit() + '\n');
}

void LldbEngine::setupInferior()
{
    const QString path = stringSetting(ExtraDumperFile);
    if (!path.isEmpty() && QFileInfo(path).isReadable()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", path);
        runCommand(cmd);
    }

    const QString commands = stringSetting(ExtraDumperCommands);
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

    QString executable;
    QtcProcess::Arguments args;
    QtcProcess::prepareCommand(QFileInfo(rp.inferior.executable).absoluteFilePath(),
                               rp.inferior.commandLineArguments, &executable, &args);

    DebuggerCommand cmd2("setupInferior");
    cmd2.arg("executable", executable);
    cmd2.arg("breakonmain", rp.breakOnMain);
    cmd2.arg("useterminal", rp.useTerminal);
    cmd2.arg("startmode", rp.startMode);
    cmd2.arg("nativemixed", isNativeMixedActive());
    cmd2.arg("workingdirectory", rp.inferior.workingDirectory);
    cmd2.arg("environment", rp.inferior.environment.toStringList());
    cmd2.arg("processargs", args.toUnixArgs());

    if (rp.useTerminal) {
        QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
        const qint64 attachedPID = m_stubProc.applicationPID();
        const qint64 attachedMainThreadID = m_stubProc.applicationMainThreadID();
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
        cmd2.arg("sysroot", rp.deviceSymbolsRoot.isEmpty() ? rp.sysRoot : rp.deviceSymbolsRoot);
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
        bool success = response.data["success"].toInt();
        if (success) {
            foreach (Breakpoint bp, breakHandler()->unclaimedBreakpoints()) {
                if (acceptsBreakpoint(bp)) {
                    bp.setEngine(this);
                    insertBreakpoint(bp);
                } else {
                    showMessage(QString("BREAKPOINT %1 IN STATE %2 IS NOT ACCEPTABLE")
                                .arg(bp.id().toString()).arg(bp.state()));
                }
            }
            notifyInferiorSetupOk();
        } else {
            notifyInferiorSetupFailed();
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

void LldbEngine::executeStep()
{
    notifyInferiorRunRequested();
    runCommand({"executeStep"});
}

void LldbEngine::executeStepI()
{
    notifyInferiorRunRequested();
    runCommand({"executeStepI"});
}

void LldbEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    runCommand({"executeStepOut"});
}

void LldbEngine::executeNext()
{
    notifyInferiorRunRequested();
    runCommand({"executeNext"});
}

void LldbEngine::executeNextI()
{
    notifyInferiorRunRequested();
    runCommand({"executeNextI"});
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

    foreach (const GdbMi &item, all.children()) {
        const QString name = item.name();
        if (name == "result") {
            QString msg = item["status"].data();
            if (msg.size())
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
            handleStateNotification(item);
        else if (name == "location")
            handleLocationNotification(item);
        else if (name == "output")
            handleOutputNotification(item);
        else if (name == "pid")
            notifyInferiorPid(item.toProcessHandle());
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
    if (frameIndex == handler->stackSize()) {
        fetchStack(handler->stackSize() * 10 + 3);
        return;
    }

    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoLocation(handler->currentFrame());

    DebuggerCommand cmd("activateFrame");
    cmd.arg("index", frameIndex);
    cmd.arg("thread", threadsHandler()->currentThread().raw());
    runCommand(cmd);

    updateLocals();
    reloadRegisters();
}

void LldbEngine::selectThread(ThreadId threadId)
{
    DebuggerCommand cmd("selectThread");
    cmd.arg("id", threadId.raw());
    cmd.callback = [this](const DebuggerResponse &) {
        fetchStack(action(MaximalStackDepth)->value().toInt());
    };
    runCommand(cmd);
}

bool LldbEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorSetupRequested:
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorStopRequested:
    case InferiorStopOk:
        return true;
    default:
        return false;
    }
}

bool LldbEngine::acceptsBreakpoint(Breakpoint bp) const
{
    if (runParameters().startMode == AttachCore)
        return false;
    if (bp.parameters().isCppBreakpoint())
        return true;
    return isNativeMixedEnabled();
}

void LldbEngine::insertBreakpoint(Breakpoint bp)
{
    DebuggerCommand cmd("insertBreakpoint");
    cmd.callback = [this, bp](const DebuggerResponse &response) {
        QTC_CHECK(bp.state() == BreakpointInsertProceeding);
        updateBreakpointData(bp, response.data, true);
    };
    bp.addToCommand(&cmd);
    bp.notifyBreakpointInsertProceeding();
    runCommand(cmd);
}

void LldbEngine::changeBreakpoint(Breakpoint bp)
{
    const BreakpointResponse &response = bp.response();
    DebuggerCommand cmd("changeBreakpoint");
    cmd.arg("lldbid", response.id.toString());
    cmd.callback = [this, bp](const DebuggerResponse &response) {
        QTC_CHECK(!bp.isValid() || bp.state() == BreakpointChangeProceeding);
        updateBreakpointData(bp, response.data, false);
    };
    bp.addToCommand(&cmd);
    bp.notifyBreakpointChangeProceeding();
    runCommand(cmd);
}

void LldbEngine::removeBreakpoint(Breakpoint bp)
{
    const BreakpointResponse &response = bp.response();
    if (response.id.isValid()) {
        DebuggerCommand cmd("removeBreakpoint");
        cmd.arg("lldbid", response.id.toString());
        cmd.callback = [this, bp](const DebuggerResponse &) {
            QTC_CHECK(bp.state() == BreakpointRemoveProceeding);
            Breakpoint bp0 = bp;
            bp0.notifyBreakpointRemoveOk();
        };
        bp.notifyBreakpointRemoveProceeding();
        runCommand(cmd);
    }
}

void LldbEngine::updateBreakpointData(Breakpoint bp, const GdbMi &bkpt, bool added)
{
    BreakHandler *handler = breakHandler();
    BreakpointResponseId rid = BreakpointResponseId(bkpt["lldbid"].data());
    if (!bp.isValid())
        bp = handler->findBreakpointByResponseId(rid);
    BreakpointResponse response = bp.response();
    if (added)
        response.id = rid;
    QTC_CHECK(response.id == rid);
    response.address = 0;
    response.enabled = bkpt["enabled"].toInt();
    response.ignoreCount = bkpt["ignorecount"].toInt();
    response.condition = fromHex(bkpt["condition"].data());
    response.hitCount = bkpt["hitcount"].toInt();
    response.fileName = bkpt["file"].data();
    response.lineNumber = bkpt["line"].toInt();

    GdbMi locations = bkpt["locations"];
    const int numChild = int(locations.children().size());
    if (numChild > 1) {
        foreach (const GdbMi &location, locations.children()) {
            const int locid = location["locid"].toInt();
            BreakpointResponse sub;
            sub.id = BreakpointResponseId(rid.majorPart(), locid);
            sub.type = response.type;
            sub.address = location["addr"].toAddress();
            sub.functionName = location["func"].data();
            sub.fileName = location["file"].data();
            sub.lineNumber = location["line"].toInt();
            bp.insertSubBreakpoint(sub);
        }
        response.pending = false;
    } else if (numChild == 1) {
        const GdbMi location = locations.childAt(0);
        response.address = location["addr"].toAddress();
        response.functionName = location["func"].data();
        response.pending = false;
    } else {
        // This can happen for pending breakpoints.
        showMessage(QString("NO LOCATIONS (YET) FOR BP %1").arg(response.toString()));
    }
    bp.setResponse(response);
    if (added)
        bp.notifyBreakpointInsertOk();
    else
        bp.notifyBreakpointChangeOk();
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
        foreach (const GdbMi &item, modules.children()) {
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
    cmd.callback = [this, moduleName](const DebuggerResponse &response) {
        const GdbMi &symbols = response.data["symbols"];
        QString moduleName = response.data["module"].data();
        Symbols syms;
        foreach (const GdbMi &item, symbols.children()) {
            Symbol symbol;
            symbol.address = item["address"].data();
            symbol.name = item["name"].data();
            symbol.state = item["state"].data();
            symbol.section = item["section"].data();
            symbol.demangled = item["demangled"].data();
            syms.append(symbol);
        }
        Internal::showModuleSymbols(moduleName, syms);
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
        threadsHandler()->updateThreads(response.data);
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

void LldbEngine::assignValueInDebugger(WatchItem *,
    const QString &expression, const QVariant &value)
{
    DebuggerCommand cmd("assignValue");
    cmd.arg("exp", toHex(expression));
    cmd.arg("value", toHex(value.toString()));
    cmd.callback = [this](const DebuggerResponse &) { updateLocals(); };
    runCommand(cmd);
}

void LldbEngine::doUpdateLocals(const UpdateParameters &params)
{
    watchHandler()->notifyUpdateStarted(params);

    DebuggerCommand cmd("fetchVariables");
    watchHandler()->appendFormatRequests(&cmd);
    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", boolSetting(UseDebuggingHelpers));
    cmd.arg("autoderef", boolSetting(AutoDerefPointers));
    cmd.arg("dyntype", boolSetting(UseDynamicType));
    cmd.arg("partialvar", params.partialVariable);
    cmd.arg("qobjectnames", boolSetting(ShowQObjectNames));

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
    };

    runCommand(cmd);
}

void LldbEngine::handleLldbError(QProcess::ProcessError error)
{
    showMessage(QString("LLDB PROCESS ERROR: %1").arg(error));
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
                .arg(runParameters().debugger.executable);
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
    notifyDebuggerProcessFinished(exitCode, exitStatus, QLatin1String("LLDB"));
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
        if (response == "lldbstartupok")
            startLldbStage2();
        else
            emit outputReady(response);
    }
}

void LldbEngine::handleStateNotification(const GdbMi &reportedState)
{
    QString newState = reportedState.data();
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
    } else if (newState == "inferiorstopfailed")
        notifyInferiorStopFailed();
    else if (newState == "inferiorill")
        notifyInferiorIll();
    else if (newState == "enginesetupok")
        notifyEngineSetupOk();
    else if (newState == "enginesetupfailed")
        notifyEngineSetupFailed();
    else if (newState == "enginerunfailed")
        notifyEngineRunFailed();
    else if (newState == "enginerunandinferiorrunok") {
        if (runParameters().continueAfterAttach)
            m_continueAtNextSpontaneousStop = true;
        notifyEngineRunAndInferiorRunOk();
    } else if (newState == "enginerunandinferiorstopok") {
        notifyEngineRunAndInferiorStopOk();
        continueInferior();
    } else if (newState == "enginerunokandinferiorunrunnable")
        notifyEngineRunOkAndInferiorUnrunnable();
    else if (newState == "inferiorshutdownok")
        notifyInferiorShutdownOk();
    else if (newState == "inferiorshutdownfailed")
        notifyInferiorShutdownFailed();
    else if (newState == "engineshutdownok")
        notifyEngineShutdownOk();
    else if (newState == "engineshutdownfailed")
        notifyEngineShutdownFailed();
    else if (newState == "inferiorexited")
        notifyInferiorExited();
}

void LldbEngine::handleLocationNotification(const GdbMi &reportedLocation)
{
    qulonglong address = reportedLocation["address"].toAddress();
    QString fileName = reportedLocation["file"].data();
    QString function = reportedLocation["function"].data();
    int lineNumber = reportedLocation["line"].toInt();
    Location loc = Location(fileName, lineNumber);
    if (boolSetting(OperateByInstruction) || !QFileInfo::exists(fileName) || lineNumber <= 0) {
        loc = Location(address);
        loc.setNeedsMarker(true);
        loc.setUseAssembler(true);
    }

    // Quickly set the location marker.
    if (lineNumber > 0
            && QFileInfo::exists(fileName)
            && function != "::qt_qmlDebugMessageAvailable()")
        gotoLocation(Location(fileName, lineNumber));
}

void LldbEngine::reloadRegisters()
{
    if (!Internal::isRegistersWindowVisible())
        return;

    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    DebuggerCommand cmd("fetchRegisters");
    cmd.callback = [this](const DebuggerResponse &response) {
        RegisterHandler *handler = registerHandler();
        GdbMi regs = response.data["registers"];
        foreach (const GdbMi &item, regs.children()) {
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
            foreach (const GdbMi &line, response.data["lines"].children()) {
                DisassemblerLine dl;
                dl.address = line["address"].toAddress();
                //dl.data = line["data"].data();
                //dl.rawData = line["rawdata"].data();
                dl.data = line["rawdata"].data();
                if (!dl.data.isEmpty())
                    dl.data += QString(30 - dl.data.size(), QLatin1Char(' '));
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
    cmd.callback = [this, agent](const DebuggerResponse &response) {
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
    cmd.callback = [this](const DebuggerResponse &response) { Q_UNUSED(response); };
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

DebuggerEngine *createLldbEngine(const DebuggerRunParameters &startParameters)
{
    return new LldbEngine(startParameters);
}

void LldbEngine::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    DebuggerEngine::notifyEngineRemoteSetupFinished(result);

    if (result.success) {
        startLldb();
    } else {
        showMessage("ADAPTER START FAILED");
        if (!result.reason.isEmpty()) {
            const QString title = tr("Adapter start failed");
            ICore::showWarningWithOptions(title, result.reason);
        }
        notifyEngineSetupFailed();
        return;
    }
}

void LldbEngine::stubStarted()
{
    startLldb();
}

void LldbEngine::stubError(const QString &msg)
{
    AsynchronousMessageBox::critical(tr("Debugger Error"), msg);
}

void LldbEngine::stubExited()
{
    if (state() == EngineShutdownRequested || state() == DebuggerFinished) {
        showMessage("STUB EXITED EXPECTEDLY");
        return;
    }
    showMessage("STUB EXITED");
    notifyEngineIll();
}

} // namespace Internal
} // namespace Debugger
