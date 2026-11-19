// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvscimpl.h"

#include "uvscutils.h"

#include "../debuggertr.h"

#include <utils/qtcassert.h>

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QUrl>

using namespace Utils;

namespace Debugger::Internal {

constexpr int kRootStackFrameLevel = 1;

static GdbMi entry(const QString &name, const QString &data)
{
    return UvscUtils::buildEntry(name, data, GdbMi::Const);
}

// Enumerating the locals of the root frame works only without the
// 'projectname.uvguix.username' files uVision keeps next to the project.
static void allowRootLocals(const FilePath &projectFile)
{
    if (!projectFile.exists())
        return;
    const FilePaths trash = projectFile.parentDir().dirEntries(
        FileFilter({projectFile.completeBaseName() + ".uvguix.*"}, DirFilterFlag::Files));
    for (const FilePath &file : trash)
        file.removeFile();
}

static DisassemblerLines parseDisassembly(const QByteArray &data)
{
    DisassemblerLines result;
    QTextStream in(data);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith("0x")) {
            // An instruction, like "0x08000210 9101      STR      r1,[sp,#0x04]".
            const int oneSpaceIndex = line.indexOf(' ');
            if (oneSpaceIndex < 0)
                continue;
            const int sixSpaceIndex = line.indexOf("      ", oneSpaceIndex);
            if (sixSpaceIndex < 0)
                continue;
            DisassemblerLine dline;
            dline.address = line.left(oneSpaceIndex).toULongLong(nullptr, 0);
            dline.bytes = line.mid(oneSpaceIndex + 1, sixSpaceIndex - oneSpaceIndex - 1);
            dline.data = line.mid(sixSpaceIndex + 6);
            result.appendLine(dline);
        } else {
            // A source line, like "    25:     struct foo foo = {0}; ", or a comment.
            const int colonIndex = line.indexOf(':');
            if (colonIndex < 0) {
                result.appendComment(line);
            } else {
                DisassemblerLine dline;
                dline.lineNumber = line.left(colonIndex).trimmed().toInt();
                dline.data = line.mid(colonIndex + 1);
                result.appendLine(dline);
            }
        }
    }
    return result;
}

static bool acceptsBreakpoint(const AcceptsBreakpointQuery &query)
{
    return !query.isVirtualMachineBreakpoint() && query.isCppBreakpoint()
           && query.type == BreakpointByFileAndLine;
}

static DebuggerEngineSetupData uvscImplSetupData()
{
    DebuggerEngineSetupData data;
    data.acceptsBreakpoint = &acceptsBreakpoint;
    data.capabilities = DisassemblerCapability
                      | RegisterCapability
                      | AddWatcherCapability
                      | CreateFullBacktraceCapability
                      | OperateByInstructionCapability
                      | ShowMemoryCapability;
    return data;
}

UvscImpl::UvscImpl(const UvscImplStartData &startData)
    : DebuggerEngineInterface(uvscImplSetupData())
    , m_startData(startData)
{}

UvscImpl::~UvscImpl() = default;

void UvscImpl::failSetup(const QString &message)
{
    emit startFailed(Tr::tr("Failed to initialize the UVSC."), message, Key("UvscSetupFailed"));
    emit inferiorEvent(InferiorEvent::EngineSetupFailed);
}

void UvscImpl::start()
{
    const int port = QUrl(m_startData.remoteChannel).port();
    if (port <= 0) {
        failSetup(Tr::tr("Internal error: Invalid TCP/IP port specified %1.").arg(port));
        return;
    }

    const FilePath &uVision = m_startData.uVisionExecutable;
    if (uVision.isEmpty()) {
        failSetup(Tr::tr("Internal error: No uVision executable specified."));
        return;
    }
    if (!uVision.exists()) {
        failSetup(Tr::tr("Internal error: The specified uVision executable does not exist."));
        return;
    }

    m_client = m_startData.createClient
                   ? m_startData.createClient(uVision.parentDir())
                   : std::make_unique<UvscClient>(QDir(uVision.parentDir().toFSPathString()));
    QTC_ASSERT(m_client, failSetup({}); return);
    if (m_client->error() != UvscClientInterface::NoError) {
        failSetup(Tr::tr("Internal error: Cannot resolve the library: %1.")
                      .arg(m_client->errorString()));
        return;
    }

    QString uvscVersion;
    QString uvsockVersion;
    m_client->version(uvscVersion, uvsockVersion);
    emit message(Tr::tr("UVSC Version: %1, UVSOCK Version: %2.").arg(uvscVersion, uvsockVersion),
                 LogMisc);

    connect(m_client.get(), &UvscClientInterface::executionStarted,
            this, &UvscImpl::handleExecutionStarted);
    connect(m_client.get(), &UvscClientInterface::executionStopped,
            this, &UvscImpl::handleExecutionStopped);
    connect(m_client.get(), &UvscClientInterface::projectClosed,
            this, &UvscImpl::handleProjectClosed);
    connect(m_client.get(), &UvscClientInterface::locationUpdated,
            this, [this](quint64 address) { m_address = address; });

    if (!m_client->connectSession(port)) {
        failSetup(Tr::tr("Internal error: Cannot open the session: %1.")
                      .arg(m_client->errorString()));
        return;
    }

    const FilePath &projectFile = m_startData.projectFile;
    if (!m_startData.optionsFile.exists()) {
        failSetup(Tr::tr("Internal error: The specified uVision project options file does not "
                         "exist."));
        return;
    }
    if (!projectFile.exists()) {
        failSetup(Tr::tr("Internal error: The specified uVision project file does not exist."));
        return;
    }
    if (!m_client->openProject(projectFile)) {
        failSetup(Tr::tr("Internal error: Unable to open the uVision project %1: %2.")
                      .arg(projectFile.toUserOutput(), m_client->errorString()));
        return;
    }
    if (!m_client->setProjectDebugTarget(m_startData.simulator)) {
        failSetup(Tr::tr("Internal error: Unable to set the uVision debug target: %1.")
                      .arg(m_client->errorString()));
        return;
    }

    // uVision takes the output file relative to the project.
    const FilePath &inferior = m_startData.inferiorExecutable;
    const FilePath targetPath = inferior.relativeChildPath(projectFile.parentDir());
    if (!inferior.exists()) {
        failSetup(Tr::tr("Internal error: The specified output file does not exist."));
        return;
    }
    if (!m_client->setProjectOutputTarget(targetPath)) {
        failSetup(Tr::tr("Internal error: Unable to set the uVision output file %1: %2.")
                      .arg(targetPath.toUserOutput(), m_client->errorString()));
        return;
    }

    // Closing the project flushes the changes to the uVision project files.
    // The setup goes on once uVision reports it closed.
    m_loadingRequired = true;
    m_client->closeProject();
}

void UvscImpl::handleProjectClosed()
{
    if (!std::exchange(m_loadingRequired, false))
        return;

    const FilePath &projectFile = m_startData.projectFile;
    allowRootLocals(projectFile);

    if (!m_client->openProject(projectFile)) {
        failSetup(Tr::tr("Internal error: Unable to open the uVision project %1: %2.")
                      .arg(projectFile.toUserOutput(), m_client->errorString()));
        return;
    }

    // The engine claims the breakpoints right on EngineSetupOk, and uVision
    // takes them only from a running session.
    if (!m_client->startSession(true)) {
        failSetup(Tr::tr("Internal error: Failed to start the debugger: %1")
                      .arg(m_client->errorString()));
        return;
    }
    emit message(Tr::tr("Application started."), StatusBar);

    fetchThreads();
    emit inferiorEvent(InferiorEvent::EngineSetupOk);
    emit inferiorEvent(InferiorEvent::RunAndInferiorStopOk);
}

void UvscImpl::shutdownInferior(ShutdownMode)
{
    if (m_client && !m_client->stopSession()) {
        emit message(Tr::tr("Failed to shut down the application: %1")
                         .arg(m_client->errorString()), LogError);
    }
    emit inferiorEvent(InferiorEvent::ShutdownFinished);
}

void UvscImpl::shutdownEngine()
{
    if (m_client)
        m_client->disconnectSession();
    emit inferiorEvent(InferiorEvent::EngineShutdownFinished);
}

void UvscImpl::requestRun(const std::function<bool()> &command)
{
    if (m_running || m_runRequested)
        return;
    m_runRequested = true;
    emit inferiorEvent(InferiorEvent::RunRequested);
    if (command())
        return;
    m_runRequested = false;
    emit message(Tr::tr("Cannot continue debugged process: %1").arg(m_client->errorString()),
                 LogError);
    emit inferiorEvent(InferiorEvent::RunFailed);
}

void UvscImpl::execute(const ExecutionRequest &request)
{
    QTC_ASSERT(m_client, return);
    // uVision does not step over or into from the root frame, so that takes
    // a single instruction.
    const bool byInstruction = request.flag || m_frameLevel == kRootStackFrameLevel;
    switch (request.command) {
    case ExecutionCommand::Continue:
        requestRun([this] { return m_client->startExecution(); });
        return;
    case ExecutionCommand::StepOver:
        requestRun([this, byInstruction] {
            return byInstruction ? m_client->executeStepInstruction()
                                 : m_client->executeStepOver();
        });
        return;
    case ExecutionCommand::StepIn:
        requestRun([this, byInstruction] {
            return byInstruction ? m_client->executeStepInstruction()
                                 : m_client->executeStepIn();
        });
        return;
    case ExecutionCommand::StepOut:
        requestRun([this] { return m_client->executeStepOut(); });
        return;
    case ExecutionCommand::Interrupt:
        m_stopRequested = true;
        if (!m_client->stopExecution()) {
            m_stopRequested = false;
            emit message(Tr::tr("Cannot stop debugged process: %1")
                             .arg(m_client->errorString()), LogError);
            emit inferiorEvent(InferiorEvent::StopFailed);
        }
        return;
    case ExecutionCommand::Abort:
        return;
    default:
        emit message(Tr::tr("The UVSC debugger does not support this operation."), LogWarning);
        return;
    }
}

void UvscImpl::handleExecutionStarted()
{
    if (!std::exchange(m_runRequested, false))
        emit inferiorEvent(InferiorEvent::RunRequested);
    m_running = true;
    emit inferiorEvent(InferiorEvent::RunOk);
}

void UvscImpl::handleExecutionStopped()
{
    const bool wasRunning = std::exchange(m_running, false);
    const bool runRequested = std::exchange(m_runRequested, false);
    const bool stopRequested = std::exchange(m_stopRequested, false);
    if (!wasRunning && !runRequested)
        return;

    // The stack the engine asks for right after the stop is that of the
    // current task.
    fetchThreads();
    if (stopRequested && wasRunning) {
        emit inferiorEvent(InferiorEvent::StopOk);
        return;
    }
    if (runRequested)
        emit inferiorEvent(InferiorEvent::RunOk);
    emit inferiorEvent(InferiorEvent::SpontaneousStop);
}

void UvscImpl::fetchThreads(GdbMi *threads)
{
    GdbMi data;
    if (!m_client->fetchThreads(true, data))
        return;
    const QString current = QString::number(m_threadId);
    bool found = false;
    for (const GdbMi &thread : data["threads"]) {
        if (thread["id"].data() == current)
            found = true;
    }
    if (!found && data["threads"].childCount() > 0)
        m_threadId = data["threads"].childAt(0)["id"].data().toUInt();
    data.addChild(entry("current-thread-id", QString::number(m_threadId)));
    if (threads)
        *threads = data;
}

void UvscImpl::refresh(const RefreshRequest &request)
{
    QTC_ASSERT(m_client, return);
    switch (request.kind) {
    case RefreshKind::FullStack: {
        GdbMi data;
        if (!m_client->fetchStackFrames(m_threadId, m_address, data)) {
            m_address = 0;
            emit message(Tr::tr("UVSC: Fetching the stack failed."), LogMisc);
            return;
        }
        emit refreshDataReceived(request.requestId, request.kind, data);
        return;
    }
    case RefreshKind::FullBacktrace: {
        GdbMi data;
        if (!m_client->fetchStackFrames(m_threadId, m_address, data))
            return;
        QString text;
        for (const GdbMi &frame : data["stack"]["frames"]) {
            text += QString("#%1  %2 in %3 at %4:%5\n")
                        .arg(frame["level"].data(), frame["address"].data(),
                             frame["function"].data(), frame["file"].data(),
                             frame["line"].data());
        }
        emit refreshDataReceived(request.requestId, request.kind, entry("backtrace", text));
        return;
    }
    case RefreshKind::Threads: {
        GdbMi data;
        fetchThreads(&data);
        if (data.isValid())
            emit refreshDataReceived(request.requestId, request.kind, data);
        return;
    }
    case RefreshKind::Registers:
        refreshRegisters(request.requestId);
        return;
    case RefreshKind::PeripheralRegisters:
        for (const quint64 address : request.addresses) {
            QByteArray value = UvscUtils::encodeU32(0);
            if (!m_client->fetchMemory(address, value)) {
                emit message(Tr::tr("UVSC: Fetching peripheral register failed."), LogMisc);
                continue;
            }
            GdbMi data = UvscUtils::buildEntry("", "", GdbMi::Tuple);
            data.addChild(entry("address", QString::number(address)));
            data.addChild(entry("value", QString::number(UvscUtils::decodeU32(value))));
            emit refreshDataReceived(request.requestId, request.kind, data);
        }
        return;
    case RefreshKind::Modules: {
        GdbMi module = UvscUtils::buildEntry("", "", GdbMi::Tuple);
        module.addChild(entry("modulepath", m_startData.inferiorExecutable.toFSPathString()));
        module.addChild(entry("modulename", "<executable>"));
        module.addChild(entry("startaddress", "0"));
        module.addChild(entry("endaddress", "0"));
        GdbMi data = UvscUtils::buildEntry("", "", GdbMi::List);
        data.addChild(module);
        emit refreshDataReceived(request.requestId, request.kind, data);
        return;
    }
    case RefreshKind::Locals:
        // Answering from within the update request trips the watch model.
        QMetaObject::invokeMethod(this, [this, request] { refreshLocals(request); },
                                  Qt::QueuedConnection);
        return;
    default:
        return;
    }
}

void UvscImpl::refreshLocals(const RefreshRequest &request)
{
    QStringList expandedLocalINames;
    QStringList expandedWatcherINames;
    for (const QString &iname : request.expandedINames) {
        if (iname.startsWith("local."))
            expandedLocalINames.append(iname);
        else if (iname.startsWith("watch."))
            expandedWatcherINames.append(iname);
    }
    expandedLocalINames.sort();
    expandedWatcherINames.sort();

    std::vector<std::pair<QString, QString>> rootWatchers;
    for (const QJsonValue &value : request.watchers) {
        const QJsonObject object = value.toObject();
        if (!object.isEmpty())
            rootWatchers.push_back({object.value("iname").toString(),
                                    object.value("exp").toString()});
    }

    GdbMi data = UvscUtils::buildEntry("data", "", GdbMi::List);
    if (!m_client->fetchLocals(expandedLocalINames, m_threadId, m_frameLevel, data))
        emit message(Tr::tr("UVSC: Locals enumeration failed."), LogMisc);
    if (!m_client->fetchWatchers(expandedWatcherINames, rootWatchers, data))
        emit message(Tr::tr("UVSC: Watchers enumeration failed."), LogMisc);

    GdbMi all = UvscUtils::buildResultTemplateEntry(!request.partialVariable.isEmpty());
    all.addChild(data);
    emit refreshDataReceived(request.requestId, RefreshKind::Locals, all);
}

void UvscImpl::refreshRegisters(quint64 requestId)
{
    m_registers.clear();
    if (!m_client->fetchRegisters(m_registers)) {
        emit message(Tr::tr("UVSC: Reading registers failed."), LogMisc);
        return;
    }
    GdbMi data = UvscUtils::buildEntry("", "", GdbMi::List);
    for (const auto &[index, reg] : m_registers) {
        GdbMi item = UvscUtils::buildEntry("", "", GdbMi::Tuple);
        item.addChild(entry("name", reg.name));
        item.addChild(entry("size", QString::number(reg.size)));
        item.addChild(entry("groups", reg.groups.join(',')));
        item.addChild(entry("description", reg.description));
        item.addChild(entry("value", reg.value.toString(reg.kind, reg.size, HexadecimalFormat)));
        data.addChild(item);
    }
    emit refreshDataReceived(requestId, RefreshKind::Registers, data);
}

void UvscImpl::insertBreakpoint(const BreakpointChangeRequest &request)
{
    const BreakpointParameters &params = request.params;
    QString exe = m_startData.inferiorExecutable.baseName();
    exe.replace('-', '_');
    const QString expression = "\\\\" + exe + "\\" + params.fileName.toUrlishString() + "\\"
                               + QString::number(params.textPosition.line);

    quint32 tickMark = 0;
    quint64 address = 0;
    quint32 line = -1;
    QString function;
    QString fileName;
    if (!m_client->createBreakpoint(expression, tickMark, address, line, function, fileName)) {
        emit message(Tr::tr("UVSC: Inserting breakpoint failed."), LogMisc);
        emit breakpointEvent(request.requestId, BreakpointOp::Insert, false);
        return;
    }
    const bool enabled = params.enabled || !m_client->disableBreakpoint(tickMark);
    m_breakpoints.insert(tickMark, {address, int(line), fileName, function, enabled});
    emit breakpointEvent(request.requestId, BreakpointOp::Insert, true,
                         breakpointData(tickMark));
}

GdbMi UvscImpl::breakpointData(quint32 tickMark) const
{
    const BreakpointRecord record = m_breakpoints.value(tickMark);
    GdbMi bkpt = UvscUtils::buildEntry("bkpt", "", GdbMi::Tuple);
    bkpt.addChild(entry("number", QString::number(tickMark)));
    bkpt.addChild(entry("addr", "0x" + QString::number(record.address, 16)));
    bkpt.addChild(entry("line", QString::number(record.line)));
    bkpt.addChild(entry("fullname", record.fileName));
    bkpt.addChild(entry("func", record.function));
    bkpt.addChild(entry("enabled", record.enabled ? "y" : "n"));
    GdbMi data = UvscUtils::buildEntry("", "", GdbMi::List);
    data.addChild(bkpt);
    return data;
}

void UvscImpl::changeBreakpoint(const BreakpointChangeRequest &request)
{
    QTC_ASSERT(m_client, return);
    const quint32 tickMark = request.responseId.toULong();
    switch (request.op) {
    case BreakpointOp::Insert:
        insertBreakpoint(request);
        return;
    case BreakpointOp::Remove: {
        const bool ok = m_client->deleteBreakpoint(tickMark);
        if (ok)
            m_breakpoints.remove(tickMark);
        else
            emit message(Tr::tr("UVSC: Removing breakpoint failed."), LogMisc);
        emit breakpointEvent(request.requestId, BreakpointOp::Remove, ok);
        return;
    }
    case BreakpointOp::Update: {
        const auto it = m_breakpoints.find(tickMark);
        if (it == m_breakpoints.end()) {
            emit breakpointEvent(request.requestId, BreakpointOp::Update, false);
            return;
        }
        const bool enabled = request.params.enabled;
        if (enabled != it->enabled) {
            const bool ok = enabled ? m_client->enableBreakpoint(tickMark)
                                    : m_client->disableBreakpoint(tickMark);
            if (!ok) {
                emit message(enabled ? Tr::tr("UVSC: Enabling breakpoint failed.")
                                     : Tr::tr("UVSC: Disabling breakpoint failed."), LogMisc);
                emit breakpointEvent(request.requestId, BreakpointOp::Update, false);
                return;
            }
            it->enabled = enabled;
        }
        emit breakpointEvent(request.requestId, BreakpointOp::Update, true,
                             breakpointData(tickMark));
        return;
    }
    case BreakpointOp::EnableSub:
        emit breakpointEvent(request.requestId, BreakpointOp::EnableSub, false);
        return;
    }
}

void UvscImpl::accessMemory(MemoryOp op, quint64 requestId, quint64 addr,
                            quint64 lengthOrSize, const QByteArray &data)
{
    QTC_ASSERT(m_client, return);
    if (op == MemoryOp::Change) {
        QTC_ASSERT(!data.isEmpty(), return);
        if (!m_client->changeMemory(addr, data)) {
            emit message(Tr::tr("UVSC: Changing memory at address 0x%1 failed.")
                             .arg(addr, 0, 16), LogMisc);
        }
        return;
    }

    MemoryReadResult result(addr, lengthOrSize);
    QByteArray bytes(int(lengthOrSize), 0);
    if (!m_client->fetchMemory(addr, bytes)) {
        emit message(Tr::tr("UVSC: Fetching memory at address 0x%1 failed.").arg(addr, 0, 16),
                     LogMisc);
        result.setUnreadable(0, lengthOrSize, m_client->errorString());
    } else {
        const quint64 read = std::min(quint64(bytes.size()), lengthOrSize);
        memcpy(result.data.data(), bytes.constData(), read);
        if (read < lengthOrSize)
            result.setUnreadable(read, lengthOrSize - read, "UVSC returned fewer bytes.");
    }
    emit memoryRead(requestId, result);
}

void UvscImpl::selectThread(const QString &threadId)
{
    bool ok = false;
    const quint32 id = threadId.toUInt(&ok);
    if (ok)
        m_threadId = id;
}

void UvscImpl::activateFrame(int index)
{
    m_frameLevel = index;
}

void UvscImpl::fetchDisassembly(quint64 requestId, quint64 address, const QString &,
                                const QString &, DisassemblyFlavor)
{
    QTC_ASSERT(m_client, return);
    QByteArray data;
    if (address && !m_client->disassemblyAddress(address, data))
        emit message(Tr::tr("UVSC: Disassembling by address failed."), LogMisc);
    emit disassemblyReceived(requestId, parseDisassembly(data));
}

void UvscImpl::executeDebuggerCommand(const QString &, const WatchItemData &)
{
    emit message(Tr::tr("The UVSC debugger does not take commands."), LogWarning);
}

void UvscImpl::assignValueInDebugger(const WatchItemData &item, const QString &,
                                     const QString &value)
{
    QTC_ASSERT(m_client, return);
    if (item.isLocal) {
        if (!m_client->setLocalValue(item.id, m_threadId, m_frameLevel, value))
            emit message(Tr::tr("UVSC: Setting local value failed."), LogMisc);
    } else if (item.isWatcher) {
        if (!m_client->setWatcherValue(item.id, value))
            emit message(Tr::tr("UVSC: Setting watcher value failed."), LogMisc);
    }
}

void UvscImpl::setRegisterValue(const QString &name, const QString &value)
{
    QTC_ASSERT(m_client, return);
    for (const auto &[index, reg] : m_registers) {
        if (reg.name == name) {
            if (!m_client->setRegisterValue(index, value))
                emit message(Tr::tr("UVSC: Setting register value failed."), LogMisc);
            return;
        }
    }
}

void UvscImpl::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    QTC_ASSERT(m_client, return);
    if (!m_client->changeMemory(address, UvscUtils::encodeU32(value)))
        emit message(Tr::tr("UVSC: Setting peripheral register value failed."), LogMisc);
}

void UvscImpl::watchPoint(quint64 requestId, const QPoint &)
{
    emit watchPointResolved(requestId, 0, {});
}

void UvscImpl::createSnapshot(quint64 requestId)
{
    emit snapshotCreated(requestId, false, {});
}

} // namespace Debugger::Internal
