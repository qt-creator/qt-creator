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

#include "lldbengine.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>
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

LldbEngine::LldbEngine(const DebuggerStartParameters &startParameters)
    : DebuggerEngine(startParameters), m_continueAtNextSpontaneousStop(false)
{
    m_lastAgentId = 0;
    setObjectName(QLatin1String("LldbEngine"));

    if (startParameters.useTerminal) {
        #ifdef Q_OS_WIN
            // Windows up to xp needs a workaround for attaching to freshly started processes. see proc_stub_win
            if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
                m_stubProc.setMode(ConsoleProcess::Suspend);
            else
                m_stubProc.setMode(ConsoleProcess::Debug);
        #else
            m_stubProc.setMode(ConsoleProcess::Debug);
            m_stubProc.setSettings(ICore::settings());
        #endif
    }

    connect(action(AutoDerefPointers), &SavedAction::valueChanged,
            this, &LldbEngine::updateLocals);
    connect(action(CreateFullBacktrace), &QAction::triggered,
            this, &LldbEngine::createFullBacktrace);
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

void LldbEngine::runCommand(const DebuggerCommand &command_)
{
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    const int tok = ++currentToken();
    DebuggerCommand command = command_;
    command.arg("token", tok);
    QByteArray token = QByteArray::number(tok);
    QByteArray cmd  = command.function + "({" + command.args + "})";
    showMessage(_(token + cmd + '\n'), LogInput);
    m_lldbProc.write("script theDumper." + cmd + "\n");
}

void LldbEngine::debugLastCommand()
{
    runCommand(m_lastDebuggableCommand);
}

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    runCommand(DebuggerCommand("shutdownInferior"));
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_lldbProc.kill();
    if (startParameters().useTerminal)
        m_stubProc.stop();
    notifyEngineShutdownOk();
}

void LldbEngine::abortDebugger()
{
    if (targetState() == DebuggerFinished) {
        // We already tried. Try harder.
        showMessage(_("ABORTING DEBUGGER. SECOND TIME."));
        m_lldbProc.kill();
    } else {
        // Be friendly the first time. This will change targetState().
        showMessage(_("ABORTING DEBUGGER. FIRST TIME."));
        quitDebugger();
    }
}

// FIXME: Merge with GdbEngine/QtcProcess
bool LldbEngine::prepareCommand()
{
    if (HostOsInfo::isWindowsHost()) {
        DebuggerStartParameters &sp = startParameters();
        QtcProcess::SplitError perr;
        sp.processArgs = QtcProcess::prepareArgs(sp.processArgs, &perr,
                                                 HostOsInfo::hostOs(),
                    &sp.environment, &sp.workingDirectory).toWindowsArgs();
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
    if (startParameters().useTerminal) {
        QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
        showMessage(_("TRYING TO START ADAPTER"));

    // Currently, adapters are not re-used
    //    // We leave the console open, so recycle it now.
    //    m_stubProc.blockSignals(true);
    //    m_stubProc.stop();
    //    m_stubProc.blockSignals(false);

        if (!prepareCommand()) {
            notifyEngineSetupFailed();
            return;
        }

        m_stubProc.setWorkingDirectory(startParameters().workingDirectory);
        // Set environment + dumper preload.
        m_stubProc.setEnvironment(startParameters().environment);

        connect(&m_stubProc, &ConsoleProcess::processError, this, &LldbEngine::stubError);
        connect(&m_stubProc, &ConsoleProcess::processStarted, this, &LldbEngine::stubStarted);
        connect(&m_stubProc, &ConsoleProcess::stubStopped, this, &LldbEngine::stubExited);
        // FIXME: Starting the stub implies starting the inferior. This is
        // fairly unclean as far as the state machine and error reporting go.

        if (!m_stubProc.start(startParameters().executable,
                             startParameters().processArgs)) {
            // Error message for user is delivered via a signal.
            //handleAdapterStartFailed(QString());
            notifyEngineSetupFailed();
            return;
        }

    } else {
        QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
        if (startParameters().remoteSetupNeeded)
            notifyEngineRequestRemoteSetup();
        else
            startLldb();
    }
}

void LldbEngine::startLldb()
{
    m_lldbCmd = startParameters().debuggerCommand;
    connect(&m_lldbProc, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
            this, &LldbEngine::handleLldbError);
    connect(&m_lldbProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &LldbEngine::handleLldbFinished);
    connect(&m_lldbProc, &QProcess::readyReadStandardOutput,
            this, &LldbEngine::readLldbStandardOutput);
    connect(&m_lldbProc, &QProcess::readyReadStandardError,
            this, &LldbEngine::readLldbStandardError);

    connect(this, &LldbEngine::outputReady,
            this, &LldbEngine::handleResponse, Qt::QueuedConnection);

    showMessage(_("STARTING LLDB: ") + m_lldbCmd);
    m_lldbProc.setEnvironment(startParameters().environment.toStringList());
    if (!startParameters().workingDirectory.isEmpty())
        m_lldbProc.setWorkingDirectory(startParameters().workingDirectory);

    m_lldbProc.start(m_lldbCmd);

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start LLDB \"%1\": %2")
            .arg(m_lldbCmd, m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
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
    showMessage(_("ADAPTER STARTED"));
    showStatusMessage(tr("Setting up inferior..."));

    const QByteArray dumperSourcePath =
        ICore::resourcePath().toLocal8Bit() + "/debugger/";

    m_lldbProc.write("script sys.path.insert(1, '" + dumperSourcePath + "')\n");
    m_lldbProc.write("script from lldbbridge import *\n");
    m_lldbProc.write("script print(dir())\n");
    m_lldbProc.write("script theDumper = Dumper()\n"); // This triggers reportState("enginesetupok")
}

void LldbEngine::setupInferior()
{
    const QString path = stringSetting(ExtraDumperFile);
    if (!path.isEmpty()) {
        DebuggerCommand cmd("addDumperModule");
        cmd.arg("path", path.toUtf8());
        runCommand(cmd);
    }

    const QString commands = stringSetting(ExtraDumperCommands);
    if (!commands.isEmpty()) {
        DebuggerCommand cmd("executeDebuggerCommand");
        cmd.arg("commands", commands.toUtf8());
        runCommand(cmd);
    }

    DebuggerCommand cmd1("loadDumpers");
    runCommand(cmd1);
}

// FIXME: splitting of setupInferior() necessary to support LLDB <= 310 - revert asap
void LldbEngine::setupInferiorStage2()
{
    const DebuggerStartParameters &sp = startParameters();

    QString executable;
    QtcProcess::Arguments args;
    QtcProcess::prepareCommand(QFileInfo(sp.executable).absoluteFilePath(),
                               sp.processArgs, &executable, &args);

    DebuggerCommand cmd("setupInferior");
    cmd.arg("executable", executable);
    cmd.arg("breakOnMain", sp.breakOnMain);
    cmd.arg("useTerminal", sp.useTerminal);
    cmd.arg("startMode", sp.startMode);

    cmd.beginList("bkpts");
    foreach (Breakpoint bp, breakHandler()->unclaimedBreakpoints()) {
        if (acceptsBreakpoint(bp)) {
            showMessage(_("TAKING OWNERSHIP OF BREAKPOINT %1 IN STATE %2")
                            .arg(bp.id().toString()).arg(bp.state()));
            bp.setEngine(this);
            bp.notifyBreakpointInsertProceeding();
            cmd.beginGroup();
            bp.addToCommand(&cmd);
            cmd.endGroup();
        } else {
            showMessage(_("BREAKPOINT %1 IN STATE %2 IS NOT ACCEPTABLE")
                .arg(bp.id().toString()).arg(bp.state()));
        }
    }
    cmd.endList();

    cmd.beginList("processArgs");
    foreach (const QString &arg, args.toUnixArgs())
        cmd.arg(arg.toUtf8().toHex());
    cmd.endList();

    if (sp.useTerminal) {
        QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
        const qint64 attachedPID = m_stubProc.applicationPID();
        const qint64 attachedMainThreadID = m_stubProc.applicationMainThreadID();
        const QString msg = (attachedMainThreadID != -1)
                ? QString::fromLatin1("Attaching to %1 (%2)").arg(attachedPID).arg(attachedMainThreadID)
                : QString::fromLatin1("Attaching to %1").arg(attachedPID);
        showMessage(msg, LogMisc);
        cmd.arg("attachPid", attachedPID);

    } else {

        cmd.arg("startMode", sp.startMode);
        // it is better not to check the start mode on the python sid (as we would have to duplicate the
        // enum values), and thus we assume that if the sp.attachPID is valid we really have to attach
        QTC_CHECK(sp.attachPID <= 0 || (sp.startMode == AttachCrashedExternal
                                    || sp.startMode == AttachExternal));
        cmd.arg("attachPid", sp.attachPID);
        cmd.arg("sysRoot", sp.deviceSymbolsRoot.isEmpty() ? sp.sysRoot : sp.deviceSymbolsRoot);
        cmd.arg("remoteChannel", ((sp.startMode == AttachToRemoteProcess
                                   || sp.startMode == AttachToRemoteServer)
                                  ? sp.remoteChannel : QString()));
        cmd.arg("platform", sp.platform);
        QTC_CHECK(!sp.continueAfterAttach || (sp.startMode == AttachToRemoteProcess
                                              || sp.startMode == AttachExternal
                                              || sp.startMode == AttachToRemoteServer));
        m_continueAtNextSpontaneousStop = false;
    }

    runCommand(cmd);
}

void LldbEngine::runEngine()
{
    const DebuggerStartParameters &sp = startParameters();
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state(); return);
    showStatusMessage(tr("Running requested..."), 5000);
    DebuggerCommand cmd("runEngine");
    if (sp.startMode == AttachCore) {
        cmd.arg("coreFile", sp.coreFile);
        cmd.arg("continuation", "updateAll");
    }
    runCommand(cmd);
}

void LldbEngine::interruptInferior()
{
    showStatusMessage(tr("Interrupt requested..."), 5000);
    runCommand("interruptInferior");
}

void LldbEngine::executeStep()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeStep");
}

void LldbEngine::executeStepI()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeStepI");
}

void LldbEngine::executeStepOut()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeStepOut");
}

void LldbEngine::executeNext()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeNext");
}

void LldbEngine::executeNextI()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("executeNextI");
}

void LldbEngine::continueInferior()
{
    resetLocation();
    notifyInferiorRunRequested();
    runCommand("continueInferior");
}

void LldbEngine::handleResponse(const QByteArray &response)
{
    GdbMi all;
    all.fromStringMultiple(response);

    foreach (const GdbMi &item, all.children()) {
        const QByteArray name = item.name();
        if (name == "all") {
            watchHandler()->notifyUpdateFinished();
            updateLocalsView(item);
        } else if (name == "dumpers") {
            watchHandler()->addDumpers(item);
            setupInferiorStage2();
        } else if (name == "stack")
            refreshStack(item);
        else if (name == "registers")
            refreshRegisters(item);
        else if (name == "threads")
            refreshThreads(item);
        else if (name == "current-thread")
            refreshCurrentThread(item);
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
        else if (name == "breakpoint-added")
            refreshAddedBreakpoint(item);
        else if (name == "breakpoint-changed")
            refreshChangedBreakpoint(item);
        else if (name == "breakpoint-removed")
            refreshRemovedBreakpoint(item);
        else if (name == "output")
            refreshOutput(item);
        else if (name == "disassembly")
            refreshDisassembly(item);
        else if (name == "memory")
            refreshMemory(item);
        else if (name == "full-backtrace")
            showFullBacktrace(item);
        else if (name == "continuation")
            handleContinuation(item);
        else if (name == "statusmessage") {
            QString msg = QString::fromUtf8(item.data());
            if (msg.size())
                msg[0] = msg.at(0).toUpper();
            showStatusMessage(msg);
        }
    }
}

void LldbEngine::handleContinuation(const GdbMi &data)
{
    if (data.data() == "updateLocals") {
        updateLocals();
    } else if (data.data() == "updateAll") {
        updateAll();
    } else {
        QTC_ASSERT(false, qDebug() << "Unknown continuation: " << data.data());
    }
}

void LldbEngine::showFullBacktrace(const GdbMi &data)
{
    Internal::openTextEditor(_("Backtrace $"),
        QString::fromUtf8(QByteArray::fromHex(data.data())));
}

void LldbEngine::executeRunToLine(const ContextData &data)
{
    resetLocation();
    notifyInferiorRunRequested();
    DebuggerCommand cmd("executeRunToLocation");
    cmd.arg("file", data.fileName);
    cmd.arg("line", data.lineNumber);
    cmd.arg("address", data.address);
    runCommand(cmd);
}

void LldbEngine::executeRunToFunction(const QString &functionName)
{
    resetLocation();
    notifyInferiorRunRequested();
    DebuggerCommand cmd("executeRunToFunction");
    cmd.arg("function", functionName);
    runCommand(cmd);
}

void LldbEngine::executeJumpToLine(const ContextData &data)
{
    resetLocation();
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

    const int n = handler->stackSize();
    if (frameIndex == n) {
        DebuggerCommand cmd("reportStack");
        cmd.arg("nativeMixed", isNativeMixedActive());
        cmd.arg("stacklimit", n * 10 + 3);
        runCommand(cmd);
        return;
    }

    QTC_ASSERT(frameIndex < handler->stackSize(), return);
    handler->setCurrentIndex(frameIndex);
    gotoLocation(handler->currentFrame());

    DebuggerCommand cmd("activateFrame");
    cmd.arg("index", frameIndex);
    cmd.arg("thread", threadsHandler()->currentThread().raw());
    cmd.arg("continuation", "updateLocals");
    runCommand(cmd);
}

void LldbEngine::selectThread(ThreadId threadId)
{
    DebuggerCommand cmd1("selectThread");
    cmd1.arg("id", threadId.raw());
    runCommand(cmd1);

    DebuggerCommand cmd("reportStack");
    cmd.arg("nativeMixed", isNativeMixedActive());
    cmd.arg("stacklimit", action(MaximalStackDepth)->value().toInt());
    cmd.arg("continuation", "updateLocals");
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
    if (startParameters().startMode == AttachCore)
        return false;
    // We handle QML breakpoint unless specifically disabled.
    if (isNativeMixedEnabled() && !(startParameters().languages & QmlLanguage))
        return true;
    return bp.parameters().isCppBreakpoint();
}

void LldbEngine::insertBreakpoint(Breakpoint bp)
{
    DebuggerCommand cmd("insertBreakpoint");
    bp.addToCommand(&cmd);
    bp.notifyBreakpointInsertProceeding();
    runCommand(cmd);
}

void LldbEngine::changeBreakpoint(Breakpoint bp)
{
    const BreakpointResponse &response = bp.response();
    DebuggerCommand cmd("changeBreakpoint");
    cmd.arg("lldbid", response.id.toByteArray());
    bp.addToCommand(&cmd);
    bp.notifyBreakpointChangeProceeding();
    runCommand(cmd);
}

void LldbEngine::removeBreakpoint(Breakpoint bp)
{
    const BreakpointResponse &response = bp.response();
    DebuggerCommand cmd("removeBreakpoint");
    cmd.arg("modelid", bp.id().toByteArray());
    cmd.arg("lldbid", response.id.toByteArray());
    bp.notifyBreakpointRemoveProceeding();
    runCommand(cmd);
}

void LldbEngine::updateBreakpointData(const GdbMi &bkpt, bool added)
{
    BreakHandler *handler = breakHandler();
    BreakpointResponseId rid = BreakpointResponseId(bkpt["lldbid"].data());
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    Breakpoint bp = handler->breakpointById(id);
    if (!bp.isValid())
        bp = handler->findBreakpointByResponseId(rid);
    BreakpointResponse response = bp.response();
    if (added)
        response.id = rid;
    QTC_CHECK(response.id == rid);
    response.address = 0;
    response.enabled = bkpt["enabled"].toInt();
    response.ignoreCount = bkpt["ignorecount"].toInt();
    response.condition = QByteArray::fromHex(bkpt["condition"].data());
    response.hitCount = bkpt["hitcount"].toInt();
    response.fileName = bkpt["file"].toUtf8();
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
            sub.functionName = location["func"].toUtf8();
            sub.fileName = location["file"].toUtf8();
            sub.lineNumber = location["line"].toInt();
            bp.insertSubBreakpoint(sub);
        }
    } else if (numChild == 1) {
        const GdbMi location = locations.childAt(0);
        response.address = location["addr"].toAddress();
        response.functionName = location["func"].toUtf8();
    } else {
        // This can happen for pending breakpoints.
        showMessage(_("NO LOCATIONS (YET) FOR BP %1").arg(response.toString()));
    }
    bp.setResponse(response);
    if (added)
        bp.notifyBreakpointInsertOk();
    else
        bp.notifyBreakpointChangeOk();
}

void LldbEngine::refreshDisassembly(const GdbMi &data)
{
    DisassemblerLines result;

    int cookie = data["cookie"].toInt();
    QPointer<DisassemblerAgent> agent = m_disassemblerAgents.key(cookie);
    if (!agent.isNull()) {
        foreach (const GdbMi &line, data["lines"].children()) {
            DisassemblerLine dl;
            dl.address = line["address"].toAddress();
            dl.data = line["inst"].toUtf8();
            dl.function = line["func-name"].toUtf8();
            dl.offset = line["offset"].toInt();
            QByteArray comment = line["comment"].data();
            if (!comment.isEmpty())
                dl.data += QString::fromUtf8(" # " + comment);
            result.appendLine(dl);
        }
        agent->setContents(result);
    }
}

void LldbEngine::refreshMemory(const GdbMi &data)
{
    int cookie = data["cookie"].toInt();
    qulonglong addr = data["address"].toAddress();
    QPointer<MemoryAgent> agent = m_memoryAgents.key(cookie);
    if (!agent.isNull()) {
        QPointer<QObject> token = m_memoryAgentTokens.value(cookie);
        QTC_ASSERT(!token.isNull(), return);
        QByteArray ba = QByteArray::fromHex(data["contents"].data());
        agent->addLazyData(token.data(), addr, ba);
    }
}

void LldbEngine::refreshOutput(const GdbMi &output)
{
    QByteArray channel = output["channel"].data();
    QByteArray data = QByteArray::fromHex(output["data"].data());
    LogChannel ch = AppStuff;
    if (channel == "stdout")
        ch = AppOutput;
    else if (channel == "stderr")
        ch = AppError;
    showMessage(QString::fromUtf8(data), ch);
}

void LldbEngine::refreshAddedBreakpoint(const GdbMi &bkpt)
{
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    Breakpoint bp = breakHandler()->breakpointById(id);
    QTC_CHECK(bp.state() == BreakpointInsertProceeding);
    updateBreakpointData(bkpt, true);
}

void LldbEngine::refreshChangedBreakpoint(const GdbMi &bkpt)
{
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    Breakpoint bp = breakHandler()->breakpointById(id);
    QTC_CHECK(!bp.isValid() || bp.state() == BreakpointChangeProceeding);
    updateBreakpointData(bkpt, false);
}

void LldbEngine::refreshRemovedBreakpoint(const GdbMi &bkpt)
{
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    Breakpoint bp = breakHandler()->breakpointById(id);
    QTC_CHECK(bp.state() == BreakpointRemoveProceeding);
    bp.notifyBreakpointRemoveOk();
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
    ModulesHandler *handler = modulesHandler();
    handler->beginUpdateAll();
    foreach (const GdbMi &item, modules.children()) {
        Module module;
        module.modulePath = item["file"].toUtf8();
        module.moduleName = item["name"].toUtf8();
        module.symbolsRead = Module::UnknownReadState;
        module.startAddress = item["loaded_addr"].toAddress();
        module.endAddress = 0; // FIXME: End address not easily available.
        handler->updateModule(module);
    }
    handler->endUpdateAll();
}

void LldbEngine::requestModuleSymbols(const QString &moduleName)
{
    DebuggerCommand cmd("listSymbols");
    cmd.arg("module", moduleName);
    runCommand(cmd);
}

void LldbEngine::refreshSymbols(const GdbMi &symbols)
{
    QString moduleName = symbols["module"].toUtf8();
    Symbols syms;
    foreach (const GdbMi &item, symbols["symbols"].children()) {
        Symbol symbol;
        symbol.address = item["address"].toUtf8();
        symbol.name = item["name"].toUtf8();
        symbol.state = item["state"].toUtf8();
        symbol.section = item["section"].toUtf8();
        symbol.demangled = item["demangled"].toUtf8();
        syms.append(symbol);
    }
   Internal::showModuleSymbols(moduleName, syms);
}


//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

bool LldbEngine::setToolTipExpression(const DebuggerToolTipContext &context)
{
    if (state() != InferiorStopOk || !context.isCppEditor) {
        //qDebug() << "SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED "
        // " OR NOT A CPPEDITOR";
        return false;
    }

    UpdateParameters params;
    params.partialVariable = context.iname;
    doUpdateLocals(params);

    return true;
}

void LldbEngine::updateAll()
{
    DebuggerCommand cmd1("reportThreads");
    runCommand(cmd1);

    DebuggerCommand cmd2("reportCurrentThread");
    runCommand(cmd2);

    DebuggerCommand cmd("reportStack");
    cmd.arg("nativeMixed", isNativeMixedActive());
    cmd.arg("stacklimit", action(MaximalStackDepth)->value().toInt());
    cmd.arg("continuation", "updateLocals");
    runCommand(cmd);
}

void LldbEngine::reloadFullStack()
{
    DebuggerCommand cmd("reportStack");
    cmd.arg("nativeMixed", isNativeMixedActive());
    cmd.arg("stacklimit", -1);
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
    cmd.arg("exp", expression.toLatin1().toHex());
    cmd.arg("value", value.toString().toLatin1().toHex());
    runCommand(cmd);
}

void LldbEngine::updateWatchItem(WatchItem *)
{
    updateLocals();
}

void LldbEngine::updateLocals()
{
    UpdateParameters params;
    doUpdateLocals(params);
}

void LldbEngine::doUpdateLocals(UpdateParameters params)
{
    if (stackHandler()->stackSize() == 0) {
        showMessage(_("SKIPPING LOCALS DUE TO EMPTY STACK"));
        return;
    }

    DebuggerCommand cmd("updateData");
    cmd.arg("nativeMixed", isNativeMixedActive());
    watchHandler()->appendFormatRequests(&cmd);

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", boolSetting(UseDebuggingHelpers));
    cmd.arg("autoderef", boolSetting(AutoDerefPointers));
    cmd.arg("dyntype", boolSetting(UseDynamicType));
    cmd.arg("partialVariable", params.partialVariable);

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

    m_lastDebuggableCommand = cmd;
    m_lastDebuggableCommand.args.replace("\"passexceptions\":0", "\"passexceptions\":1");

    watchHandler()->notifyUpdateStarted();
    runCommand(cmd);

    reloadRegisters();
}

void LldbEngine::handleLldbError(QProcess::ProcessError error)
{
    showMessage(_("LLDB PROCESS ERROR: %1").arg(error));
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
                .arg(m_lldbCmd);
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
            return tr("An unknown error in the LLDB process occurred.") + QLatin1Char(' ');
    }
}

void LldbEngine::handleLldbFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    notifyDebuggerProcessFinished(exitCode, exitStatus, QLatin1String("LLDB"));
}

void LldbEngine::readLldbStandardError()
{
    QByteArray err = m_lldbProc.readAllStandardError();
    qDebug() << "\nLLDB STDERR UNEXPECTED: " << err;
    showMessage(_("Lldb stderr: " + err), LogError);
}

void LldbEngine::readLldbStandardOutput()
{
    QByteArray out = m_lldbProc.readAllStandardOutput();
    showMessage(_(out));
    m_inbuffer.append(out);
    while (true) {
        int pos = m_inbuffer.indexOf("@\n");
        if (pos == -1)
            break;
        QByteArray response = m_inbuffer.left(pos).trimmed();
        m_inbuffer = m_inbuffer.mid(pos + 2);
        if (response == "lldbstartupok")
            startLldbStage2();
        else
            emit outputReady(response);
    }
}

void LldbEngine::refreshStack(const GdbMi &stack)
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
    action(ExpandStack)->setEnabled(canExpand);
    handler->setFrames(frames, canExpand);
}

void LldbEngine::refreshRegisters(const GdbMi &registers)
{
    RegisterHandler *handler = registerHandler();
    foreach (const GdbMi &item, registers.children()) {
        Register reg;
        reg.name = item["name"].data();
        reg.value = item["value"].data();
        reg.size = item["size"].data().toInt();
        reg.reportedType = item["type"].data();
        handler->updateRegister(reg);
    }
    handler->commitUpdates();
}

void LldbEngine::refreshThreads(const GdbMi &threads)
{
    ThreadsHandler *handler = threadsHandler();
    handler->updateThreads(threads);
    updateViews(); // Adjust Threads combobox.
}

void LldbEngine::refreshCurrentThread(const GdbMi &data)
{
    ThreadsHandler *handler = threadsHandler();
    ThreadId id(data["id"].toInt());
    handler->setCurrentThread(id);
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
    else if (newState == "stopped") {
        notifyInferiorSpontaneousStop();
        if (m_continueAtNextSpontaneousStop) {
            m_continueAtNextSpontaneousStop = false;
            continueInferior();
        } else {
            updateAll();
        }
    } else if (newState == "inferiorstopok") {
        notifyInferiorStopOk();
        updateAll();
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
    else if (newState == "inferiorsetupok")
        notifyInferiorSetupOk();
    else if (newState == "inferiorsetupfailed")
        notifyInferiorSetupFailed();
    else if (newState == "enginerunandinferiorrunok") {
        if (startParameters().continueAfterAttach)
            m_continueAtNextSpontaneousStop = true;
        notifyEngineRunAndInferiorRunOk();
    } else if (newState == "enginerunandinferiorstopok")
        notifyEngineRunAndInferiorStopOk();
    else if (newState == "enginerunokandinferiorunrunnable")
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

void LldbEngine::refreshLocation(const GdbMi &reportedLocation)
{
    qulonglong addr = reportedLocation["addr"].toAddress();
    QString file = reportedLocation["file"].toUtf8();
    int line = reportedLocation["line"].toInt();
    Location loc = Location(file, line);
    if (boolSetting(OperateByInstruction) || !QFileInfo::exists(file) || line <= 0) {
        loc = Location(addr);
        loc.setNeedsMarker(true);
        loc.setUseAssembler(true);
    }
    gotoLocation(loc);
}

void LldbEngine::reloadRegisters()
{
    if (Internal::isDockVisible(QLatin1String(DOCKWIDGET_REGISTER)))
        runCommand("reportRegisters");
}

void LldbEngine::reloadDebuggingHelpers()
{
    runCommand("reloadDumpers");
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
    DebuggerCommand cmd("disassemble");
    cmd.arg("cookie", id);
    cmd.arg("address", loc.address());
    cmd.arg("function", loc.functionName());
    cmd.arg("flavor", boolSetting(IntelFlavor) ? "intel" : "att");
    runCommand(cmd);
}

void LldbEngine::createFullBacktrace()
{
    runCommand("createFullBacktrace");
}

void LldbEngine::fetchMemory(MemoryAgent *agent, QObject *editorToken,
        quint64 addr, quint64 length)
{
    int id = m_memoryAgents.value(agent, -1);
    if (id == -1) {
        id = ++m_lastAgentId;
        m_memoryAgents.insert(agent, id);
    }
    m_memoryAgentTokens.insert(id, editorToken);
    DebuggerCommand cmd("fetchMemory");
    cmd.arg("address", addr);
    cmd.arg("length", length);
    cmd.arg("cookie", id);
    runCommand(cmd);
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
    DebuggerCommand cmd("writeMemory");
    cmd.arg("address", addr);
    cmd.arg("data", data.toHex());
    cmd.arg("cookie", id);
    runCommand(cmd);
}

void LldbEngine::setRegisterValue(const QByteArray &name, const QString &value)
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

void LldbEngine::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    DebuggerEngine::notifyEngineRemoteSetupFinished(result);

    if (result.success) {
        startLldb();
    } else {
        showMessage(_("ADAPTER START FAILED"));
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
        showMessage(_("STUB EXITED EXPECTEDLY"));
        return;
    }
    showMessage(_("STUB EXITED"));
    notifyEngineIll();
}

} // namespace Internal
} // namespace Debugger
