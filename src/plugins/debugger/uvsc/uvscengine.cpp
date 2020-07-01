/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "uvscengine.h"
#include "uvscutils.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggercore.h>
#include <debugger/disassembleragent.h>
#include <debugger/disassemblerlines.h>
#include <debugger/memoryagent.h>
#include <debugger/moduleshandler.h>
#include <debugger/peripheralregisterhandler.h>
#include <debugger/registerhandler.h>
#include <debugger/stackhandler.h>
#include <debugger/threadshandler.h>
#include <debugger/watchhandler.h>

#include <coreplugin/messagebox.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <utils/fileutils.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QTextStream>

#include <utility>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
namespace Internal {

constexpr int kRootStackFrameLevel = 1;

// This is a workaround to enable the enumeration of a locals
// in a main/root stack. We need to remove a file in a form
// of 'projectname.uvguix.username' which are creates by uVision.
// Yes, it is a magic!
static void allowRootLocals(const FilePath &projectFile)
{
    const QFileInfo fi(projectFile.toFileInfo());
    if (!fi.exists())
        return;
    const QString baseName = fi.baseName();
    const QDir dir(fi.dir());
    const QString filter = QStringLiteral("%1.uvguix.*").arg(baseName);
    const QFileInfoList trashFileInfos = dir.entryInfoList({filter}, QDir::Files);
    for (const QFileInfo &trashFileInfo : trashFileInfos) {
        QFile f(trashFileInfo.absoluteFilePath());
        f.remove();
    }
}

// Accessed by DebuggerRunTool.
DebuggerEngine *createUvscEngine()
{
    return new UvscEngine;
}

// UvscEngine

UvscEngine::UvscEngine()
{
    setObjectName("UvscEngine");
    setDebuggerName("UVSC");
}

void UvscEngine::setupEngine()
{
    showMessage("TRYING TO INITIALIZE UVSC");

    const DebuggerRunParameters &rp = runParameters();

    // Extract the TCP/IP port for running uVision server.
    const QUrl channel(rp.remoteChannel);
    const int port = channel.port();
    if (port <= 0) {
        handleSetupFailure(tr("Internal error: Invalid TCP/IP port specified %1.")
                           .arg(port));
        return;
    }

    // Check for valid uVision executable.
    if (rp.debugger.executable.isEmpty()) {
        handleSetupFailure(tr("Internal error: No uVision executable specified."));
        return;
    } else if (!rp.debugger.executable.exists()) {
        handleSetupFailure(tr("Internal error: The specified uVision executable does not exist."));
        return;
    }

    showMessage("UVSC: RESOLVING LIBRARY SYMBOLS...");
    m_client.reset(new UvscClient(rp.debugger.executable.parentDir().toString()));
    if (m_client->error() != UvscClient::NoError) {
        handleSetupFailure(tr("Internal error: Cannot resolve the library: %1.")
                           .arg(m_client->errorString()));
        return;
    } else {
        // Show the client API version.
        QString uvscVersion;
        QString uvsockVersion;
        m_client->version(uvscVersion, uvsockVersion);
        const QString msg = tr("UVSC Version: %1, UVSOCK Version: %2.")
                .arg(uvscVersion, uvsockVersion);
        showMessage(msg, LogMisc);

        // Conenct to the client signals.
        connect(m_client.get(), &UvscClient::errorOccurred,
                this, [this](UvscClient::UvscError error) {
            // Handle errors if required.
            Q_UNUSED(error)
            Q_UNUSED(this)
        });
        connect(m_client.get(), &UvscClient::executionStarted,
                this, &UvscEngine::handleStartExecution);
        connect(m_client.get(), &UvscClient::executionStopped,
                this, &UvscEngine::handleStopExecution);
        connect(m_client.get(), &UvscClient::projectClosed,
                this, &UvscEngine::handleProjectClosed);
        connect(m_client.get(), &UvscClient::locationUpdated,
                this, &UvscEngine::handleUpdateLocation);
    }

    showMessage("UVSC: CONNECTING SESSION...");
    if (!m_client->connectSession(port)) {
        handleSetupFailure(tr("Internal error: Cannot open the session: %1.")
                           .arg(m_client->errorString()));
        return;
    } else {
        showMessage("UVSC: SESSION OPENED.");
        // Show window (fot testing only).
        //m_client->showWindow();
    }

    if (!configureProject(rp))
        return;

    // Reload peripheral register description.
    peripheralRegisterHandler()->updateRegisterGroups();
}

void UvscEngine::runEngine()
{
    showMessage("UVSC: STARTING DEBUGGER...");
    if (!m_client->startSession(true)) {
        showStatusMessage(tr("Internal error: Failed to start the debugger: %1")
                          .arg(m_client->errorString()));
        notifyEngineRunFailed();
        return;
    } else {
        showMessage("UVSC: DEBUGGER STARTED");
        showMessage(tr("Application started."), StatusBar);
    }

    // Initial attempt to set breakpoints.
    showStatusMessage(tr("Setting breakpoints..."));
    showMessage(tr("Setting breakpoints..."));
    BreakpointManager::claimBreakpointsForEngine(this);

    showMessage("UVSC RUNNING SUCCESSFULLY.");
    notifyEngineRunAndInferiorStopOk();
}

void UvscEngine::shutdownInferior()
{
    showMessage("UVSC: STOPPING DEBUGGER...");
    if (!m_client->stopSession()) {
        AsynchronousMessageBox::critical(tr("Failed to Shut Down Application"),
                                         m_client->errorString());
    } else {
        showMessage("UVSC: DEBUGGER STOPPED");
    }

    notifyInferiorShutdownFinished();
}

void UvscEngine::shutdownEngine()
{
    showMessage("INITIATE UVSC SHUTDOWN");
    m_client->disconnectSession();
    notifyEngineShutdownFinished();
}

bool UvscEngine::hasCapability(unsigned cap) const
{
    return cap & (DisassemblerCapability
                  | RegisterCapability
                  | AddWatcherCapability
                  | WatchWidgetsCapability
                  | CreateFullBacktraceCapability
                  | OperateByInstructionCapability);
}

void UvscEngine::setRegisterValue(const QString &name, const QString &value)
{
    const auto registerBegin = m_registers.cbegin();
    const auto registerEnd = m_registers.cend();
    const auto registerIt = std::find_if(registerBegin, registerEnd,
                                         [name](const std::pair<int, Register> &reg) {
        return reg.second.name == name;
    });
    if (registerIt == registerEnd)
        return; // Register not found.
    if (!m_client->setRegisterValue(registerIt->first, value))
        return;
    reloadRegisters();
}

void UvscEngine::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    const QByteArray data = UvscUtils::encodeU32(value);
    if (!m_client->changeMemory(address, data))
        return;
    reloadPeripheralRegisters();
}

void UvscEngine::executeStepOver(bool byInstruction)
{
    notifyInferiorRunRequested();
    const quint32 frameLevel = currentFrameLevel();
    // UVSC does not support the 'step-over' from the main
    // stack frame (seems, it is UVSC bug), so, falling back
    // to the 'step-by-instruction' in this case.
    const bool result = (frameLevel == kRootStackFrameLevel || byInstruction)
            ? m_client->executeStepInstruction() : m_client->executeStepOver();
    if (!result)
        handleExecutionFailure(m_client->errorString());
}

void UvscEngine::executeStepIn(bool byInstruction)
{
    notifyInferiorRunRequested();
    const quint32 frameLevel = currentFrameLevel();
    // UVSC does not support the 'step-over' from the main
    // stack frame (seems, it is UVSC bug), so, falling back
    // to the 'step-by-instruction' in this case.
    const bool result = (frameLevel == kRootStackFrameLevel || byInstruction)
            ? m_client->executeStepInstruction() : m_client->executeStepIn();
    if (!result)
        handleExecutionFailure(m_client->errorString());
}

void UvscEngine::executeStepOut()
{
    notifyInferiorRunRequested();
    if (!m_client->executeStepOut())
        handleExecutionFailure(m_client->errorString());
}

void UvscEngine::continueInferior()
{
    if (state() != InferiorStopOk)
        return;

    notifyInferiorRunRequested();
    showStatusMessage(tr("Running requested..."), 5000);

    if (!m_client->startExecution()) {
        showMessage(tr("UVSC: Starting execution failed."), LogMisc);
        handleExecutionFailure(m_client->errorString());
    }
}

void UvscEngine::interruptInferior()
{
    if (state() != InferiorStopRequested)
        return;

    if (!m_client->stopExecution()) {
        showMessage(tr("UVSC: Stopping execution failed."), LogMisc);
        handleStoppingFailure(m_client->errorString());
    }
}

void UvscEngine::assignValueInDebugger(WatchItem *item, const QString &expr,
                                       const QVariant &value)
{
    Q_UNUSED(expr)

    if (item->isLocal()) {
        const int taskId = currentThreadId();
        const int frameId = currentFrameLevel();
        if (!m_client->setLocalValue(item->id, taskId, frameId, value.toString()))
            showMessage(tr("UVSC: Setting local value failed."), LogMisc);
    } else if (item->isWatcher()) {
        if (!m_client->setWatcherValue(item->id, value.toString()))
            showMessage(tr("UVSC: Setting watcher value failed."), LogMisc);
    }

    updateLocals();
}

void UvscEngine::selectThread(const Thread &thread)
{
    Q_UNUSED(thread)

    // We don't support this feature, because we always have
    // only one main thread.
}

void UvscEngine::activateFrame(int index)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    if (handler->isSpecialFrame(index)) {
        reloadFullStack();
        return;
    }

    QTC_ASSERT(index < handler->stackSize(), return);
    handler->setCurrentIndex(index);

    gotoCurrentLocation();
    updateLocals();
    reloadRegisters();
    reloadPeripheralRegisters();
}

bool UvscEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorRunOk:
    case InferiorStopOk:
        return true;
    default:
        break;
    }
    return false;
}

bool UvscEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    if (bp.isCppBreakpoint()) {
        switch (bp.type) {
        case BreakpointByFileAndLine:
            return true;
        default:
            break;
        }
    }
    return false;
}

void UvscEngine::insertBreakpoint(const Breakpoint &bp)
{
    if (!bp || bp->state() != BreakpointInsertionRequested)
        return;

    notifyBreakpointInsertProceeding(bp);

    const BreakpointParameters &requested = bp->requestedParameters();
    QString expression;
    if (requested.type == BreakpointByFileAndLine) {
        // Add target executable name.
        const DebuggerRunParameters &rp = runParameters();
        QString exe = rp.inferior.executable.toFileInfo().baseName();
        exe.replace('-', '_');
        expression += "\\\\" + exe;
        // Add file name.
        expression += "\\" + requested.fileName.toString();
        // Add line number.
        expression += "\\" + QString::number(requested.lineNumber);
    }

    handleInsertBreakpoint(expression, bp);
}

void UvscEngine::removeBreakpoint(const Breakpoint &bp)
{
    if (!bp || bp->state() != BreakpointRemoveRequested || bp->responseId().isEmpty())
        return;

    notifyBreakpointRemoveProceeding(bp);
    handleRemoveBreakpoint(bp);
}

void UvscEngine::updateBreakpoint(const Breakpoint &bp)
{
    if (!bp || bp->state() != BreakpointUpdateRequested || bp->responseId().isEmpty())
        return;

    const BreakpointParameters &requested = bp->requestedParameters();
    if (requested.type == UnknownBreakpointType)
        return;

    notifyBreakpointChangeProceeding(bp);
    handleChangeBreakpoint(bp);
}

void UvscEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    QByteArray data;
    const Location location = agent->location();
    if (const quint64 address = location.address()) {
        if (!m_client->disassemblyAddress(address, data))
            showMessage(tr("UVSC: Disassembling by address failed."), LogMisc);
    }

    DisassemblerLines result;
    QTextStream in(data);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith("0x")) {
            // Instruction line, like:
            // "0x08000210 9101      STR      r1,[sp,#0x04]".
            const int oneSpaceIndex = line.indexOf(' ');
            if (oneSpaceIndex < 0)
                continue;
            const QString address = line.mid(0, oneSpaceIndex);
            const int sixSpaceIndex = line.indexOf("      ", oneSpaceIndex);
            if (sixSpaceIndex < 0)
                continue;
            const QString bytes = line.mid(oneSpaceIndex + 1, sixSpaceIndex - oneSpaceIndex - 1);
            const QString content = line.mid(sixSpaceIndex + 6);
            DisassemblerLine dline;
            dline.address = address.toULongLong(nullptr, 0);
            dline.bytes = bytes;
            dline.data = content;
            result.appendLine(dline);
        } else {
            // Comment or code line, like:
            // "    25:     struct foo foo = {0}; ".
            const int colonIndex = line.indexOf(':');
            if (colonIndex < 0) {
                result.appendComment(line);
            } else {
                const QString number = line.mid(0, colonIndex).trimmed();
                const QString content = line.mid(colonIndex + 1);
                DisassemblerLine dline;
                dline.lineNumber = number.toInt();
                dline.data = content;
                result.appendLine(dline);
            }
        }
    }

    if (result.coversAddress(agent->address())) {
        // We need to force cleanup a cache to make a location
        // marker work correctly in disassembly view.
        agent->cleanup();
        agent->setContents(result);
    }
}

void UvscEngine::reloadRegisters()
{
    if (!isRegistersWindowVisible())
        return;
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;
    handleReloadRegisters();
}

void UvscEngine::reloadPeripheralRegisters()
{
    if (!isPeripheralRegistersWindowVisible())
        return;

    const QList<quint64> addresses = peripheralRegisterHandler()->activeRegisters();
    if (addresses.isEmpty())
        return; // Nothing to update.
    handleReloadPeripheralRegisters(addresses);
}

void UvscEngine::reloadFullStack()
{
    resetLocation();
    handleReloadStack(true);
}

void UvscEngine::doUpdateLocals(const UpdateParameters &params)
{
    if (m_inUpdateLocals)
        return;
    m_inUpdateLocals = true;

    watchHandler()->notifyUpdateStarted(params);

    const bool partial = !params.partialVariable.isEmpty();
    // This is a workaround to avoid a strange QVector index assertion
    // inside of the watch model.
    QMetaObject::invokeMethod(this, "handleUpdateLocals", Qt::QueuedConnection,
                              Q_ARG(bool, partial));
}

void UvscEngine::updateAll()
{
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);

    handleThreadInfo();
    reloadRegisters();
    reloadPeripheralRegisters();
    updateLocals();
}

bool UvscEngine::configureProject(const DebuggerRunParameters &rp)
{
    // Fetch patchs for the generated uVision project files.
    const FilePath optionsPath = FilePath::fromString(rp.inferior.extraData.value(
                    Constants::kUVisionOptionsFilePath).toString());
    const FilePath projectPath = FilePath::fromString(rp.inferior.extraData.value(
                    Constants::kUVisionProjectFilePath).toString());

    showMessage("UVSC: LOADING PROJECT...");
    if (!optionsPath.exists()) {
        handleSetupFailure(tr("Internal error: The specified uVision project options file does not exist."));
        return false;
    } else if (!projectPath.exists()) {
        handleSetupFailure(tr("Internal error: The specified uVision project file does not exist."));
        return false;
    } else if (!m_client->openProject(projectPath)) {
        handleSetupFailure(tr("Internal error: Unable to open the uVision project %1: %2.")
                           .arg(projectPath.toString(), m_client->errorString()));
        return false;
    } else {
        showMessage("UVSC: PROJECT LOADED");
    }

    showMessage("UVSC: SETTING PROJECT DEBUG TARGET...");
    m_simulator = rp.inferior.extraData.value(Constants::kUVisionSimulator).toBool();
    if (!m_client->setProjectDebugTarget(m_simulator)) {
        handleSetupFailure(tr("Internal error: Unable to set the uVision debug target: %1.")
                           .arg(m_client->errorString()));
        return false;
    } else {
        showMessage("UVSC: PROJECT DEBUG TARGET SET");
    }

    // We need to use the relative output target path.
    showMessage("UVSC: SETTING PROJECT OUTPUT TARGET...");
    const FilePath targetPath = rp.inferior.executable.relativeChildPath(
                projectPath.parentDir());
    if (!rp.inferior.executable.exists()) {
        handleSetupFailure(tr("Internal error: The specified output file does not exist."));
        return false;
    } else if (!m_client->setProjectOutputTarget(targetPath)) {
        handleSetupFailure(tr("Internal error: Unable to set the uVision output file %1: %2.")
                           .arg(targetPath.toString(), m_client->errorString()));
        return false;
    } else {
        showMessage("UVSC: PROJECT OUTPUT TARGET SET");
    }

    // Close the project to flush all changes to the
    // specific uVision project files.
    m_loadingRequired = true;
    m_client->closeProject();
    return true;
}

quint32 UvscEngine::currentThreadId() const
{
    const Thread thread = threadsHandler()->currentThread();
    return thread ? thread->id().toUInt() : -1;
}

quint32 UvscEngine::currentFrameLevel() const
{
    const StackFrame frame = stackHandler()->currentFrame();
    return frame.level.toUInt();
}

void UvscEngine::handleProjectClosed()
{
    if (!m_loadingRequired)
        return;
    m_loadingRequired = false;

    const DebuggerRunParameters &rp = runParameters();
    const FilePath projectPath = FilePath::fromString(rp.inferior.extraData.value(
                    Constants::kUVisionProjectFilePath).toString());

    // This magic function removes specific files from the uVision
    // project directory. Without of this we can't enumerate the local
    // variables on a root/main stack (yes, it is a magic)!
    allowRootLocals(projectPath);

    // Re-open the project again.
    if (!m_client->openProject(projectPath)) {
        handleSetupFailure(tr("Internal error: Unable to open the uVision project %1: %2.")
                           .arg(projectPath.toString(), m_client->errorString()));
        return;
    }

    // Adding executable to modules list.
    Module module;
    module.startAddress = 0;
    module.endAddress = 0;
    module.modulePath = rp.inferior.executable.toString();
    module.moduleName = "<executable>";
    modulesHandler()->updateModule(module);

    showMessage("UVSC: ALL INITIALIZED SUCCESSFULLY.");

    notifyEngineSetupOk();
}

void UvscEngine::handleUpdateLocation(quint64 address)
{
    m_address = address;
}

void UvscEngine::handleStartExecution()
{
    if (state() != InferiorRunRequested)
        notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void UvscEngine::handleStopExecution()
{
    if (state() == InferiorRunOk) {
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorRunRequested) {
        notifyInferiorRunOk();
        notifyInferiorSpontaneousStop();
    } else if (state() == InferiorStopOk) {
        // That's expected.
    } else if (state() == InferiorStopRequested) {
        notifyInferiorStopOk();
    } else if (state() == EngineRunRequested) {
        // This is gdb 7+'s initial *stopped in response to attach that
        // appears before the ^done is seen for local setups.
        notifyEngineRunAndInferiorStopOk();
    } else {
        QTC_CHECK(false);
    }

    QTC_CHECK(state() == InferiorStopOk);
    handleThreadInfo();
}

void UvscEngine::handleThreadInfo()
{
    const bool showNames = true;
    GdbMi data;
    if (!m_client->fetchThreads(showNames, data))
        return;
    ThreadsHandler *handler = threadsHandler();
    handler->setThreads(data);
    updateState();
    handleReloadStack(false);
}

void UvscEngine::handleReloadStack(bool isFull)
{
    GdbMi data;
    const quint32 taskId = currentThreadId();
    if (!m_client->fetchStackFrames(taskId, m_address, data)) {
        m_address = 0;
        reloadRegisters();
        reloadPeripheralRegisters();
        return;
    }

    const GdbMi stack = data["stack"];
    const GdbMi frames = stack["frames"];
    if (!frames.isValid())
        isFull = true;

    stackHandler()->setFramesAndCurrentIndex(frames, isFull);
    activateFrame(stackHandler()->currentIndex());
}

void UvscEngine::handleReloadRegisters()
{
    m_registers.clear();
    if (!m_client->fetchRegisters(m_registers)) {
        showMessage(tr("UVSC: Reading registers failed."), LogMisc);
    } else {
        RegisterHandler *handler = registerHandler();
        for (const auto &reg : qAsConst(m_registers))
            handler->updateRegister(reg.second);
        handler->commitUpdates();
    }
}

void UvscEngine::handleReloadPeripheralRegisters(const QList<quint64> &addresses)
{
    for (const quint64 address : addresses) {
        QByteArray data = UvscUtils::encodeU32(0);
        if (!m_client->fetchMemory(address, data)) {
            showMessage(tr("UVSC: Fetching peripheral register failed."), LogMisc);
        } else {
            const quint32 value = UvscUtils::decodeU32(data);
            peripheralRegisterHandler()->updateRegister(address, value);
        }
    }
}

void UvscEngine::handleUpdateLocals(bool partial)
{
    m_inUpdateLocals = false;

    // Build result entry.
    GdbMi all = UvscUtils::buildResultTemplateEntry(partial);
    // Build data entry.
    GdbMi data = UvscUtils::buildEntry("data", "", GdbMi::List);

    const int taskId = currentThreadId();
    const int frameId = currentFrameLevel();

    DebuggerCommand cmd;
    watchHandler()->appendFormatRequests(&cmd);
    watchHandler()->appendWatchersAndTooltipRequests(&cmd);

    auto enumerateExpandedINames = [&cmd]() {
        QStringList inames;
        const QJsonArray array = cmd.args["expanded"].toArray();
        for (const QJsonValue &value : array)
            inames.push_back(value.toString());
        return inames;
    };

    auto enumerateRootWatchers = [&cmd]() {
        std::vector<std::pair<QString, QString>> inames;
        const QJsonArray array = cmd.args["watchers"].toArray();
        for (const QJsonValue &value : array) {
            if (!value.isObject())
                continue;
            const QJsonObject object = value.toObject();
            inames.push_back({object.value("iname").toString(),
                              object.value("exp").toString()});
        }
        return inames;
    };

    const QStringList expandedINames = enumerateExpandedINames();
    const std::vector<std::pair<QString, QString>> rootWatchers = enumerateRootWatchers();
    QStringList expandedLocalINames;
    QStringList expandedWatcherINames;
    for (const QString &iname : expandedINames) {
        if (iname.startsWith("local."))
            expandedLocalINames.push_back(iname);
        else if (iname.startsWith("watch."))
            expandedWatcherINames.push_back(iname);
    }

    if (!m_client->fetchLocals(expandedLocalINames, taskId, frameId, data))
        showMessage(tr("UVSC: Locals enumeration failed."), LogMisc);
    if (!m_client->fetchWatchers(expandedWatcherINames, rootWatchers, data))
        showMessage(tr("UVSC: Watchers enumeration failed."), LogMisc);

    all.addChild(data);

    updateLocalsView(all);
    watchHandler()->notifyUpdateFinished();
    updateToolTips();
}

void UvscEngine::handleInsertBreakpoint(const QString &exp, const Breakpoint &bp)
{
    quint32 tickMark = 0;
    quint64 address = 0;
    quint32 line = -1;
    QString function;
    QString fileName;
    if (!m_client->createBreakpoint(exp, tickMark, address, line, function, fileName)) {
        showMessage(tr("UVSC: Inserting breakpoint failed."), LogMisc);
        notifyBreakpointInsertFailed(bp);
    } else {
        bp->setPending(false);
        bp->setResponseId(QString::number(tickMark));
        bp->setAddress(address);
        bp->setLineNumber(line);
        bp->setFileName(FilePath::fromString(fileName));
        bp->setFunctionName(function);
        notifyBreakpointInsertOk(bp);
    }
}

void UvscEngine::handleRemoveBreakpoint(const Breakpoint &bp)
{
    const quint32 tickMark = bp->responseId().toULong();
    if (!m_client->deleteBreakpoint(tickMark)) {
        showMessage(tr("UVSC: Removing breakpoint failed."), LogMisc);
        notifyBreakpointRemoveFailed(bp);
    } else {
        notifyBreakpointRemoveOk(bp);
    }
}

void UvscEngine::handleChangeBreakpoint(const Breakpoint &bp)
{
    const quint32 tickMark = bp->responseId().toULong();
    const BreakpointParameters &requested = bp->requestedParameters();
    if (requested.enabled && !bp->isEnabled()) {
        if (!m_client->enableBreakpoint(tickMark)) {
            showMessage(tr("UVSC: Enabling breakpoint failed."), LogMisc);
            notifyBreakpointChangeFailed(bp);
            return;
        }
    } else if (!requested.enabled && bp->isEnabled()) {
        if (!m_client->disableBreakpoint(tickMark)) {
            showMessage(tr("UVSC: Disabling breakpoint failed."), LogMisc);
            notifyBreakpointChangeFailed(bp);
            return;
        }
    }

    notifyBreakpointChangeOk(bp);
}

void UvscEngine::handleSetupFailure(const QString &errorMessage)
{
    showMessage("UVSC INITIALIZATION FAILED");
    AsynchronousMessageBox::critical(tr("Failed to initialize the UVSC."), errorMessage);
    notifyEngineSetupFailed();
}

void UvscEngine::handleShutdownFailure(const QString &errorMessage)
{
    showMessage("UVSC SHUTDOWN FAILED");
    AsynchronousMessageBox::critical(tr("Failed to de-initialize the UVSC."), errorMessage);
}

void UvscEngine::handleRunFailure(const QString &errorMessage)
{
    showMessage("UVSC RUN FAILED");
    AsynchronousMessageBox::critical(tr("Failed to run the UVSC."), errorMessage);
    notifyEngineSetupFailed();
}

void UvscEngine::handleExecutionFailure(const QString &errorMessage)
{
    AsynchronousMessageBox::critical(tr("Execution Error"),
                                     tr("Cannot continue debugged process:\n") + errorMessage);
    notifyInferiorRunFailed();
}

void UvscEngine::handleStoppingFailure(const QString &errorMessage)
{
    AsynchronousMessageBox::critical(tr("Execution Error"),
                                     tr("Cannot stop debugged process:\n") + errorMessage);
    notifyInferiorStopFailed();
}

} // namespace Internal
} // namespace Debugger
