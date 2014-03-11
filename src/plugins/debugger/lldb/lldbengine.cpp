/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerdialogs.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/debuggerplugin.h>
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

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/qtcprocess.h>

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

using namespace Utils;

namespace Debugger {
namespace Internal {

static QByteArray tooltipIName(const QString &exp)
{
    return "tooltip." + exp.toLatin1().toHex();
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
    m_lastToken = 0;
    setObjectName(QLatin1String("LldbEngine"));

    if (startParameters.useTerminal) {
        #ifdef Q_OS_WIN
            // Windows up to xp needs a workaround for attaching to freshly started processes. see proc_stub_win
            if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
                m_stubProc.setMode(Utils::ConsoleProcess::Suspend);
            else
                m_stubProc.setMode(Utils::ConsoleProcess::Debug);
        #else
            m_stubProc.setMode(Utils::ConsoleProcess::Debug);
            m_stubProc.setSettings(Core::ICore::settings());
        #endif
    }

    connect(debuggerCore()->action(AutoDerefPointers), SIGNAL(valueChanged(QVariant)),
            SLOT(updateLocals()));
    connect(debuggerCore()->action(CreateFullBacktrace), SIGNAL(triggered()),
            SLOT(updateAll()));
    connect(debuggerCore()->action(UseDebuggingHelpers), SIGNAL(valueChanged(QVariant)),
            SLOT(updateLocals()));
    connect(debuggerCore()->action(UseDynamicType), SIGNAL(valueChanged(QVariant)),
            SLOT(updateLocals()));
    connect(debuggerCore()->action(IntelFlavor), SIGNAL(valueChanged(QVariant)),
            SLOT(updateAll()));
}

LldbEngine::~LldbEngine()
{
    m_stubProc.disconnect(); // Avoid spurious state transitions from late exiting stub
}


void LldbEngine::executeDebuggerCommand(const QString &command, DebuggerLanguages)
{
    runCommand(Command("executeDebuggerCommand").arg("command", command));
}

void LldbEngine::runCommand(const Command &command)
{
    QTC_ASSERT(m_lldbProc.state() == QProcess::Running, notifyEngineIll());
    ++m_lastToken;
    QByteArray token = QByteArray::number(m_lastToken);
    QByteArray cmd = "{\"cmd\":\"" + command.function + "\","
        + command.args + "\"token\":" + token + "}\n";
    showMessage(_(token + cmd), LogInput);
    m_lldbProc.write(cmd);
}

void LldbEngine::debugLastCommand()
{
    runCommand(m_lastDebuggableCommand);
}

void LldbEngine::shutdownInferior()
{
    QTC_ASSERT(state() == InferiorShutdownRequested, qDebug() << state());
    runCommand(Command("shutdownInferior"));
}

void LldbEngine::shutdownEngine()
{
    QTC_ASSERT(state() == EngineShutdownRequested, qDebug() << state());
    m_lldbProc.kill();
    if (startParameters().useTerminal)
        m_stubProc.stop();
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
                                                 Utils::HostOsInfo::hostOs(),
                    &sp.environment, &sp.workingDirectory).toWindowsArgs();
        if (perr != Utils::QtcProcess::SplitOk) {
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

        connect(&m_stubProc, SIGNAL(processError(QString)), SLOT(stubError(QString)));
        connect(&m_stubProc, SIGNAL(processStarted()), SLOT(stubStarted()));
        connect(&m_stubProc, SIGNAL(stubStopped()), SLOT(stubExited()));
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

    QStringList args;
    args.append(_("-i"));
    args.append(Core::ICore::resourcePath() + _("/debugger/lldbbridge.py"));
    args.append(m_lldbCmd);
    showMessage(_("STARTING LLDB ") + args.join(QLatin1String(" ")));
    m_lldbProc.setEnvironment(startParameters().environment.toStringList());
    if (!startParameters().workingDirectory.isEmpty())
        m_lldbProc.setWorkingDirectory(startParameters().workingDirectory);

    m_lldbProc.start(_("python"), args);

    if (!m_lldbProc.waitForStarted()) {
        const QString msg = tr("Unable to start LLDB \"%1\": %2")
            .arg(m_lldbCmd, m_lldbProc.errorString());
        notifyEngineSetupFailed();
        showMessage(_("ADAPTER START FAILED"));
        if (!msg.isEmpty())
            Core::ICore::showWarningWithOptions(tr("Adapter start failed."), msg);
    }
}

void LldbEngine::setupInferior()
{
    const DebuggerStartParameters &sp = startParameters();

    QString executable;
    Utils::QtcProcess::Arguments args;
    Utils::QtcProcess::prepareCommand(QFileInfo(sp.executable).absoluteFilePath(),
                                      sp.processArgs, &executable, &args);

    Command cmd("setupInferior");
    cmd.arg("executable", executable);
    cmd.arg("breakOnMain", sp.breakOnMain);
    cmd.arg("useTerminal", sp.useTerminal);

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
        cmd.arg("startMode", AttachExternal);
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
        if (name == "data")
            refreshLocals(item);
        else if (name == "stack")
            refreshStack(item);
        else if (name == "stack-position")
            refreshStackPosition(item);
        else if (name == "stack-top")
            refreshStackTop(item);
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
        else if (name == "continuation")
            runContinuation(item);
        else if (name == "statusmessage") {
            QString msg = QString::fromUtf8(item.data());
            if (msg.size())
                msg[0] = msg.at(0).toUpper();
            showStatusMessage(msg);
        }
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
    Command cmd("executeRunToLocation");
    cmd.arg("file", data.fileName);
    cmd.arg("line", data.lineNumber);
    cmd.arg("address", data.address);
    runCommand(cmd);
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
    Command cmd("executeJumpToLocation");
    cmd.arg("file", data.fileName);
    cmd.arg("line", data.lineNumber);
    cmd.arg("address", data.address);
    runCommand(cmd);
}

void LldbEngine::activateFrame(int frameIndex)
{
    resetLocation();
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    int limit = debuggerCore()->action(MaximalStackDepth)->value().toInt();
    int n = stackHandler()->stackSize();
    if (frameIndex == n)
        limit = n * 10 + 3;

    Command cmd("activateFrame");
    cmd.arg("index", frameIndex);
    cmd.arg("thread", threadsHandler()->currentThread().raw());
    cmd.arg("stacklimit", limit);
    runCommand(cmd);

    updateAll();
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
        const BreakpointState bpState = handler->state(id);
        switch (bpState) {
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
                    .arg("condition", handler->condition(id).toHex())
                    .arg("function", handler->functionName(id).toUtf8())
                    .arg("oneshot", handler->isOneShot(id))
                    .arg("enabled", handler->isEnabled(id))
                    .arg("file", handler->fileName(id).toUtf8())
                    .arg("line", handler->lineNumber(id))
                    .arg("address", handler->address(id))
                    .arg("expression", handler->expression(id))
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
                    .arg("condition", handler->condition(id).toHex())
                    .arg("function", handler->functionName(id).toUtf8())
                    .arg("oneshot", handler->isOneShot(id))
                    .arg("enabled", handler->isEnabled(id))
                    .arg("file", handler->fileName(id).toUtf8())
                    .arg("line", handler->lineNumber(id))
                    .arg("address", handler->address(id))
                    .arg("expression", handler->expression(id))
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
            QTC_ASSERT(false, qDebug() << "UNEXPECTED STATE" << bpState << "FOR BP " << id);
            break;
        default:
            QTC_ASSERT(false, qDebug() << "UNKNOWN STATE" << bpState << "FOR BP" << id);
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
    BreakpointResponseId rid = BreakpointResponseId(bkpt["lldbid"].data());
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    if (!id.isValid())
        id = handler->findBreakpointByResponseId(rid);
    BreakpointResponse response = handler->response(id);
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
    const int numChild = locations.children().size();
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
            handler->insertSubBreakpoint(id, sub);
        }
    } else if (numChild == 1) {
        const GdbMi location = locations.childAt(0);
        response.address = location["addr"].toAddress();
        response.functionName = location["func"].toUtf8();
    } else {
        QTC_CHECK(false);
    }
    handler->setResponse(id, response);
    if (added)
        handler->notifyBreakpointInsertOk(id);
    else
        handler->notifyBreakpointChangeOk(id);
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
    QTC_CHECK(breakHandler()->state(id) == BreakpointInsertProceeding);
    updateBreakpointData(bkpt, true);
}

void LldbEngine::refreshChangedBreakpoint(const GdbMi &bkpt)
{
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    QTC_CHECK(!id.isValid() || breakHandler()->state(id) == BreakpointChangeProceeding);
    updateBreakpointData(bkpt, false);
}

void LldbEngine::refreshRemovedBreakpoint(const GdbMi &bkpt)
{
    BreakHandler *handler = breakHandler();
    BreakpointModelId id = BreakpointModelId(bkpt["modelid"].data());
    QTC_CHECK(handler->state(id) == BreakpointRemoveProceeding);
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
    runCommand("listModules");
}

void LldbEngine::refreshModules(const GdbMi &modules)
{
    Modules mods;
    foreach (const GdbMi &item, modules.children()) {
        Module module;
        module.modulePath = item["file"].toUtf8();
        module.moduleName = item["name"].toUtf8();
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

void LldbEngine::showToolTip()
{
    if (m_toolTipContext.isNull())
        return;
    const QString expression = m_toolTipContext->expression;
    if (DebuggerToolTipManager::debug())
        qDebug() << "LldbEngine::showToolTip " << expression << m_toolTipContext->iname << (*m_toolTipContext);

    if (m_toolTipContext->iname.startsWith("tooltip")
        && (!debuggerCore()->boolSetting(UseToolTipsInMainEditor)
            || !watchHandler()->isValidToolTip(m_toolTipContext->iname))) {
        watchHandler()->removeData(m_toolTipContext->iname);
        return;
    }

    DebuggerToolTipWidget *tw = new DebuggerToolTipWidget;
    tw->setContext(*m_toolTipContext);
    tw->acquireEngine(this);
    DebuggerToolTipManager::showToolTip(m_toolTipContext->mousePosition, tw);
    // Prevent tooltip from re-occurring (classic GDB, QTCREATORBUG-4711).
    m_toolTipContext.reset();
}

void LldbEngine::resetLocation()
{
    m_toolTipContext.reset();
    DebuggerEngine::resetLocation();
}

bool LldbEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &contextIn)
{
    if (state() != InferiorStopOk || !isCppEditor(editor)) {
        //qDebug() << "SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED "
        // " OR NOT A CPPEDITOR";
        return false;
    }

    DebuggerToolTipContext context = contextIn;
    int line, column;
    QString exp = fixCppExpression(cppExpressionAt(editor, context.position, &line, &column, &context.function));
    if (exp.isEmpty())
        return false;
    // Prefer a filter on an existing local variable if it can be found.
    QByteArray iname;
    if (const WatchData *localVariable = watchHandler()->findCppLocalVariable(exp)) {
        exp = QLatin1String(localVariable->exp);
        iname = localVariable->iname;
    } else {
        iname = tooltipIName(exp);
    }

    if (DebuggerToolTipManager::debug())
        qDebug() << "GdbEngine::setToolTipExpression1 " << exp << iname << context;

    // Same expression: Display synchronously.
    if (!m_toolTipContext.isNull() && m_toolTipContext->expression == exp) {
        showToolTip();
        return true;
    }

    m_toolTipContext.reset(new DebuggerToolTipContext(context));
    m_toolTipContext->mousePosition = mousePos;
    m_toolTipContext->expression = exp;
    m_toolTipContext->iname = iname;
    // Local variable: Display synchronously.
    if (iname.startsWith("local")) {
        showToolTip();
        return true;
    }

    if (DebuggerToolTipManager::debug())
        qDebug() << "GdbEngine::setToolTipExpression2 " << exp << (*m_toolTipContext);

    UpdateParameters params;
    params.tryPartial = true;
    params.tooltipOnly = true;
    params.varList = iname;
    doUpdateLocals(params);

    return true;
}

void LldbEngine::updateAll()
{
    reloadRegisters();
    updateLocals();
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void LldbEngine::assignValueInDebugger(const Internal::WatchData *data,
    const QString &expression, const QVariant &value)
{
    Q_UNUSED(data);
    Command cmd("assignValue");
    cmd.arg("exp", expression.toLatin1().toHex());
    cmd.arg("value", value.toString().toLatin1().toHex());
    runCommand(cmd);
}

void LldbEngine::updateWatchData(const WatchData &data, const WatchUpdateFlags &flags)
{
    Q_UNUSED(data);
    Q_UNUSED(flags);
    updateLocals();
}

void LldbEngine::updateLocals()
{
    UpdateParameters params;
    doUpdateLocals(params);
}

void LldbEngine::doUpdateLocals(UpdateParameters params)
{
    WatchHandler *handler = watchHandler();

    Command cmd("updateData");
    cmd.arg("expanded", handler->expansionRequests());
    cmd.arg("typeformats", handler->typeFormatRequests());
    cmd.arg("formats", handler->individualFormatRequests());

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
    cmd.arg("passexceptions", alwaysVerbose);
    cmd.arg("fancy", debuggerCore()->boolSetting(UseDebuggingHelpers));
    cmd.arg("autoderef", debuggerCore()->boolSetting(AutoDerefPointers));
    cmd.arg("dyntype", debuggerCore()->boolSetting(UseDynamicType));
    cmd.arg("partial", params.tryPartial);
    cmd.arg("tooltiponly", params.tooltipOnly);

    cmd.beginList("watchers");
    // Watchers
    QHashIterator<QByteArray, int> it(WatchHandler::watcherNames());
    while (it.hasNext()) {
        it.next();
        cmd.beginGroup()
            .arg("iname", "watch." + QByteArray::number(it.value()))
            .arg("exp", it.key().toHex())
        .endGroup();
    }
    // Tooltip
    const StackFrame frame = stackHandler()->currentFrame();
    if (!frame.file.isEmpty()) {
        // Re-create tooltip items that are not filters on existing local variables in
        // the tooltip model.
        DebuggerToolTipContexts toolTips =
            DebuggerToolTipManager::treeWidgetExpressions(frame.file, objectName(), frame.function);

        const QString currentExpression =
             m_toolTipContext.isNull() ? QString() : m_toolTipContext->expression;
        if (!currentExpression.isEmpty()) {
            int currentIndex = -1;
            for (int i = 0; i < toolTips.size(); ++i) {
                if (toolTips.at(i).expression == currentExpression) {
                    currentIndex = i;
                    break;
                }
            }
            if (currentIndex < 0) {
                DebuggerToolTipContext context;
                context.expression = currentExpression;
                context.iname = tooltipIName(currentExpression);
                toolTips.push_back(context);
            }
        }
        foreach (const DebuggerToolTipContext &p, toolTips) {
            if (p.iname.startsWith("tooltip"))
                cmd.beginGroup()
                    .arg("iname", p.iname)
                    .arg("exp", p.expression.toLatin1().toHex())
                .endGroup();
        }
    }
    cmd.endList();

    //cmd.arg("resultvarname", m_resultVarName);

    m_lastDebuggableCommand = cmd;
    m_lastDebuggableCommand.args.replace("\"passexceptions\":0", "\"passexceptions\":1");

    runCommand(cmd);

    reloadRegisters();
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
        showMessageBox(QMessageBox::Critical, tr("LLDB I/O Error"),
                       errorMessage(error));
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
    showMessage(_(err), LogError);
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

void LldbEngine::refreshLocals(const GdbMi &vars)
{
    //const bool partial = response.cookie.toBool();
    WatchHandler *handler = watchHandler();
    handler->resetValueCache();
    QList<WatchData> list;

    //if (!partial) {
        list.append(*handler->findData("local"));
        list.append(*handler->findData("watch"));
        list.append(*handler->findData("tooltip"));
        list.append(*handler->findData("return"));
    //}

    foreach (const GdbMi &child, vars.children()) {
        WatchData dummy;
        dummy.iname = child["iname"].data();
        GdbMi wname = child["wname"];
        if (wname.isValid()) {
            // Happens (only) for watched expressions.
            dummy.exp = QByteArray::fromHex(wname.data());
            dummy.name = QString::fromUtf8(dummy.exp);
        } else {
            dummy.name = child["name"].toUtf8();
        }
        parseWatchData(handler->expandedINames(), dummy, child, &list);
    }
    handler->insertData(list);

    showToolTip();
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
        frame.usable = QFileInfo(frame.file).isReadable();
        frames.append(frame);
    }
    bool canExpand = stack["hasmore"].toInt();
    debuggerCore()->action(ExpandStack)->setEnabled(canExpand);
    handler->setFrames(frames, canExpand);
}

void LldbEngine::refreshStackPosition(const GdbMi &position)
{
    setStackPosition(position["id"].toInt());
}

void LldbEngine::refreshStackTop(const GdbMi &)
{
    setStackPosition(stackHandler()->firstUsableIndex());
}

void LldbEngine::setStackPosition(int index)
{
    StackHandler *handler = stackHandler();
    handler->setFrames(handler->frames());
    handler->setCurrentIndex(index);
    if (index >= 0 && index < handler->stackSize())
        gotoLocation(handler->frameAt(index));
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
    else if (newState == "stopped") {
        notifyInferiorSpontaneousStop();
        if (m_continueAtNextSpontaneousStop) {
            m_continueAtNextSpontaneousStop = false;
            continueInferior();
        } else {
            updateAll();
        }
    } else if (newState == "inferiorstopok")
        notifyInferiorStopOk();
    else if (newState == "inferiorstopfailed")
        notifyInferiorStopFailed();
    else if (newState == "enginesetupok")
        notifyEngineSetupOk();
    else if (newState == "enginesetupfailed")
        notifyEngineSetupFailed();
    else if (newState == "enginerunfailed")
        notifyEngineRunFailed();
    else if (newState == "inferiorsetupok")
        notifyInferiorSetupOk();
    else if (newState == "enginerunandinferiorrunok") {
        if (startParameters().continueAfterAttach)
            m_continueAtNextSpontaneousStop = true;
        notifyEngineRunAndInferiorRunOk();
    } else if (newState == "enginerunandinferiorstopok")
        notifyEngineRunAndInferiorStopOk();
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
    if (debuggerCore()->boolSetting(OperateByInstruction)) {
        Location loc(reportedLocation["addr"].toAddress());
        loc.setNeedsMarker(true);
        gotoLocation(loc);
    } else {
        QString file = reportedLocation["file"].toUtf8();
        int line = reportedLocation["line"].toInt();
        gotoLocation(Location(file, line));
    }
}

void LldbEngine::reloadRegisters()
{
    if (debuggerCore()->isDockVisible(QLatin1String(DOCKWIDGET_REGISTER)))
        runCommand("reportRegisters");
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
    Command cmd("disassemble");
    cmd.arg("cookie", id);
    cmd.arg("address", loc.address());
    cmd.arg("function", loc.functionName());
    cmd.arg("flavor", debuggerCore()->boolSetting(IntelFlavor) ? "intel" : "att");
    runCommand(cmd);
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
    Command cmd("fetchMemory");
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
    Command cmd("writeMemory");
    cmd.arg("address", addr);
    cmd.arg("data", data.toHex());
    cmd.arg("cookie", id);
    runCommand(cmd);
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

void LldbEngine::notifyEngineRemoteSetupDone(int portOrPid, int qmlPort)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    DebuggerEngine::notifyEngineRemoteSetupDone(portOrPid, qmlPort);

    if (qmlPort != -1)
        startParameters().qmlServerPort = qmlPort;
    if (portOrPid != -1) {
        if (startParameters().startMode == AttachExternal) {
            startParameters().attachPID = portOrPid;
        } else {
            QString &rc = startParameters().remoteChannel;
            const int sepIndex = rc.lastIndexOf(QLatin1Char(':'));
            if (sepIndex != -1)
                rc.replace(sepIndex + 1, rc.count() - sepIndex - 1,
                           QString::number(portOrPid));
        }
    }
    startLldb();
}

void LldbEngine::notifyEngineRemoteSetupFailed(const QString &reason)
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    DebuggerEngine::notifyEngineRemoteSetupFailed(reason);
    showMessage(_("ADAPTER START FAILED"));
    if (!reason.isEmpty()) {
        const QString title = tr("Adapter start failed");
        Core::ICore::showWarningWithOptions(title, reason);
    }
    notifyEngineSetupFailed();
}

///////////////////////////////////////////////////////////////////////
//
// Command
//
///////////////////////////////////////////////////////////////////////

const LldbEngine::Command &LldbEngine::Command::argHelper(const char *name, const QByteArray &data) const
{
    args.append('"');
    args.append(name);
    args.append("\":");
    args.append(data);
    args.append(",");
    return *this;
}

QByteArray LldbEngine::Command::toData(const QList<QByteArray> &value)
{
    QByteArray res;
    foreach (const QByteArray &item, value) {
        if (!res.isEmpty())
            res.append(',');
        res += item;
    }
    return '[' + res + ']';
}

QByteArray LldbEngine::Command::toData(const QHash<QByteArray, QByteArray> &value)
{
    QByteArray res;
    QHashIterator<QByteArray, QByteArray> it(value);
    while (it.hasNext()) {
        it.next();
        if (!res.isEmpty())
            res.append(',');
        res += '"' + it.key() + "\":" + it.value();
    }
    return '{' + res + '}';
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, int value) const
{
    return argHelper(name, QByteArray::number(value));
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, qlonglong value) const
{
    return argHelper(name, QByteArray::number(value));
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *name, qulonglong value) const
{
    return argHelper(name, QByteArray::number(value));
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
    args.append('"');
    args.append(name);
    args.append("\":\"");
    args.append(value);
    args.append("\",");
    return *this;
}

const LldbEngine::Command &LldbEngine::Command::arg(const char *value) const
{
    args.append("\"");
    args.append(value);
    args.append("\",");
    return *this;
}

const LldbEngine::Command &LldbEngine::Command::beginList(const char *name) const
{
    if (name) {
        args += '"';
        args += name;
        args += "\":";
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
        args += '"';
        args += name;
        args += "\":";
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

void LldbEngine::stubStarted()
{
    startLldb();
}

void LldbEngine::stubError(const QString &msg)
{
    showMessageBox(QMessageBox::Critical, tr("Debugger Error"), msg);
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
