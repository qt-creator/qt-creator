// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericdebuggerengine.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggertr.h"
#include "debuggerinternalconstants.h"
#include "disassembleragent.h"
#include "memoryagent.h"
#include "moduleshandler.h"
#include "peripheralregisterhandler.h"
#include "registerhandler.h"
#include "sourcefileshandler.h"
#include "sourceutils.h"
#include "stackhandler.h"
#include "threadshandler.h"
#include "watchhandler.h"
#include "watchwindow.h"

#include <cppeditor/cppmodelmanager.h>
#include <qtsupport/qtversionmanager.h>
#include <projectexplorer/taskhub.h>

#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/widgets.h>

#include <QMessageBox>

using namespace Utils;

namespace Debugger::Internal {

static QString stopSignal(const ProjectExplorer::Abi &abi)
{
    return abi.os() == ProjectExplorer::Abi::WindowsOS ? QStringLiteral("SIGTRAP")
                                                        : QStringLiteral("SIGINT");
}

GenericDebuggerEngine::GenericDebuggerEngine(const QString &debuggerTypeName,
                                             DebuggerEngineInterface *backend)
    : m_backend(backend)
{
    setObjectName("GenericDebuggerEngine");
    setDebuggerName(debuggerTypeName);
    setToolTipHandling(m_backend->setupData().toolTipHandling);

    connect(m_backend.get(), &DebuggerEngineInterface::message, this,
            [this](const QString &text, int channel, int timeout) {
        showMessage(text, channel, timeout);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::progressMessage, this,
            [this](const QString &text) {
        showStatusMessage(text, 1000);
        progressPing();
    });
    connect(m_backend.get(), &DebuggerEngineInterface::inferiorDone, this,
            [this](const InferiorResultData &resultData) {
        if (resultData.exitStatus != InferiorExitStatus::Detached) {
            notifyExitCode(resultData.exitCode);
            if (resultData.exitStatus == InferiorExitStatus::Crash) {
                showStatusMessage(resultData.signalName.isEmpty()
                                      ? Tr::tr("Application exited after receiving a signal.")
                                      : Tr::tr("Application exited after receiving signal %1")
                                            .arg(resultData.signalName));
            } else if (resultData.exitCode != 0) {
                showStatusMessage(
                    Tr::tr("Application exited with exit code %1").arg(resultData.exitCode));
            } else {
                showStatusMessage(Tr::tr("Application exited normally."));
            }
        }
        if (resultData.terminatedByRuntime) {
            AsynchronousMessageBox::information(
                Tr::tr("Terminated by the C++ Runtime"),
                Tr::tr("The application was terminated by the C++ runtime, in most cases "
                       "because of an uncaught exception. The runtime reported the reason "
                       "in the application output.\n\n"
                       "There is no location to show, as the termination did not produce a "
                       "signal or exception the debugger stops on. Add a breakpoint at "
                       "\"throw\" to stop where an exception is thrown."));
        }
        notifyInferiorExited();
    });
    connect(m_backend.get(), &DebuggerEngineInterface::inferiorPidKnown,
            this, &DebuggerEngine::notifyInferiorPid);
    connect(m_backend.get(), &DebuggerEngineInterface::interruptTerminalRequested,
            this, &DebuggerEngine::interruptTerminalRequested);
    connect(m_backend.get(), &DebuggerEngineInterface::kickoffTerminalProcessRequested,
            this, &DebuggerEngine::kickoffTerminalProcessRequested);
    connect(m_backend.get(), &DebuggerEngineInterface::engineProcessFinished,
            this, &GenericDebuggerEngine::notifyDebuggerProcessFinished);
    connect(m_backend.get(), &DebuggerEngineInterface::breakpointEvent,
            this, &GenericDebuggerEngine::handleBreakpointEvent);
    connect(m_backend.get(), &DebuggerEngineInterface::locationChanged, this,
            [this](const FilePath &fileName, int lineNumber) {
        if (operatesByInstruction())
            return;
        const FilePath cleanFileName = cleanupFullName(fileName.path());
        const FilePath file = cleanFileName.isEmpty() ? fileName : cleanFileName;
        // Debug information names the file as the machine the binary was built on
        // saw it, so a library built elsewhere points nowhere here. Opening that
        // puts a modal complaint in front of a stop the user did not ask about.
        if (!file.isReadableFile())
            return;
        gotoLocation(Location(file, lineNumber));
    });
    connect(m_backend.get(), &DebuggerEngineInterface::threadEvent, this,
            [this](ThreadEvent event, const GdbMi &data) {
        const QString id = data["id"].data();
        if (event == ThreadEvent::Running) {
            threadsHandler()->notifyRunning(data["thread-id"].data());
            return;
        }
        if (event == ThreadEvent::Stopped) {
            threadsHandler()->notifyStopped(id);
            return;
        }
        if (event == ThreadEvent::Selected) {
            showStatusMessage(Tr::tr("Thread %1 selected.").arg(id), 1000);
            return;
        }
        if (event == ThreadEvent::GroupCreated) {
            showStatusMessage(Tr::tr("Thread group %1 created.").arg(id), 1000);
            threadsHandler()->notifyGroupCreated(id, data["pid"].data());
            return;
        }
        if (event == ThreadEvent::GroupExited) {
            showStatusMessage(Tr::tr("Thread group %1 exited.").arg(id), 1000);
            threadsHandler()->notifyGroupExited(id);
            return;
        }
        if (event == ThreadEvent::Exited) {
            const QString groupId = data["group-id"].data();
            showStatusMessage(groupId.isEmpty()
                                  ? Tr::tr("Thread %1 exited.").arg(id)
                                  : Tr::tr("Thread %1 in group %2 exited.").arg(id, groupId),
                              1000);
            threadsHandler()->removeThread(id);
            return;
        }
        showStatusMessage(Tr::tr("Thread %1 created.").arg(id), 1000);
        ThreadData thread;
        thread.id = id;
        thread.groupId = data["group-id"].data();
        threadsHandler()->updateThread(thread);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::exceptionReported, this,
            [this](const ExceptionReport &report) {
        showStatusMessage(report.description);
        if (report.isCppException)
            showMessage(report.description + '\n', AppOutput);
        using namespace ProjectExplorer;
        TaskHub::addTask(Task(report.fatal ? Task::Error : Task::Warning,
                              Tr::tr("Debugger encountered an exception: %1")
                                  .arg(report.withoutLocation),
                              report.file, report.line,
                              Constants::TASK_CATEGORY_DEBUGGER_RUNTIME));
    });
    connect(m_backend.get(), &DebuggerEngineInterface::libraryEvent, this,
            [this](LibraryEvent event, const GdbMi &data) {
        const QString id = data["id"].data();
        const FilePath modulePath = runParameters().inferior().command.executable()
                                        .withNewPath(data["target-name"].data());
        if (event == LibraryEvent::Loaded) {
            Module module;
            module.hostPath = FilePath::fromUserInput(data["host-name"].data());
            module.modulePath = modulePath;
            module.moduleName = module.hostPath.baseName();
            modulesHandler()->updateModule(module);
            askAboutQtSymbols(module.hostPath);
        } else {
            modulesHandler()->removeModule(modulePath);
        }
        progressPing();
        if (!id.isEmpty()) {
            showStatusMessage(event == LibraryEvent::Loaded ? Tr::tr("Library %1 loaded.").arg(id)
                                                             : Tr::tr("Library %1 unloaded.").arg(id),
                              1000);
        }
    });
    connect(m_backend.get(), &DebuggerEngineInterface::breakpointModified,
            this, &GenericDebuggerEngine::handleBreakpointModified);
    connect(m_backend.get(), &DebuggerEngineInterface::signalReceived,
            this, &GenericDebuggerEngine::handleSignalReceived);
    connect(m_backend.get(), &DebuggerEngineInterface::stopReasonReported,
            this, &GenericDebuggerEngine::handleStopReasonReported);
    connect(m_backend.get(), &DebuggerEngineInterface::notResponding,
            this, &GenericDebuggerEngine::handleNotResponding);
    connect(m_backend.get(), &DebuggerEngineInterface::startFailed,
            this, [this](const QString &title, const QString &message, const Key &settingsKey) {
        showMessage(message, LogError);
        CheckableMessageBox::information(title, message, settingsKey);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::refreshDataReceived, this,
            [this](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        switch (kind) {
        case RefreshKind::Locals:
            updateLocalsView(data);
            watchHandler()->notifyUpdateFinished();
            updateToolTips();
            break;
        case RefreshKind::ModuleSymbolState:
            if (data["private"].data() == "0")
                reportMissingQtSymbols(data["module"].data());
            break;
        case RefreshKind::InspectorTree:
            watchHandler()->insertItems(data["data"]);
            watchHandler()->updateLocalsWindow();
            watchHandler()->reexpandItems();
            break;
        case RefreshKind::FullBacktrace:
            if (!data.data().isEmpty())
                openTextEditor("Backtrace$", data.data());
            break;
        case RefreshKind::FullStack: {
            const GdbMi frames = data["stack"]["frames"];
            // Only a stack fetched with a limit can be truncated, and only a
            // truncated one offers the user the rest of it.
            const bool isFull = requestId == m_fullStackRequestId || !frames.isValid();
            stackHandler()->setFramesAndCurrentIndex(frames, isFull);
            const int index = stackHandler()->currentIndex();
            if (index >= 0)
                activateFrame(index);
            break;
        }
        case RefreshKind::Registers: {
            RegisterHandler *handler = registerHandler();
            for (const GdbMi &item : data) {
                Register reg;
                reg.name = item["name"].data();
                reg.size = item["size"].toInt();
                reg.reportedType = item["type"].data();
                reg.groups = item["groups"].data().split(',', Qt::SkipEmptyParts);
                reg.value.fromString(item["value"].data(), HexadecimalFormat);
                handler->updateRegister(reg);
            }
            handler->commitUpdates();
            break;
        }
        case RefreshKind::Modules: {
            ModulesHandler *handler = modulesHandler();
            handler->beginUpdateAll();
            for (const GdbMi &item : data) {
                Module module;
                module.modulePath = FilePath::fromUserInput(item["modulepath"].data());
                module.moduleName = module.modulePath.baseName();
                module.startAddress = item["startaddress"].data().toULongLong();
                module.endAddress = item["endaddress"].data().toULongLong();
                if (const GdbMi symbolsRead = item["symbolsread"]; symbolsRead.isValid()) {
                    module.symbolsRead = symbolsRead.data() == "Yes"
                                         ? Module::ReadOk : Module::ReadFailed;
                }
                handler->updateModule(module);
            }
            handler->endUpdateAll();
            break;
        }
        case RefreshKind::PeripheralRegisters:
            peripheralRegisterHandler()->updateRegister(data["address"].data().toULongLong(),
                                                        data["value"].data().toULongLong());
            break;
        case RefreshKind::SourceFiles: {
            QMap<QString, FilePath> sourceFiles;
            for (const GdbMi &item : data) {
                const GdbMi fullName = item["fullname"];
                sourceFiles[item["file"].data()] =
                    fullName.isValid() ? cleanupFullName(fullName.data()) : FilePath();
            }
            sourceFilesHandler()->setSourceFiles(sourceFiles);
            break;
        }
        case RefreshKind::ModuleSymbols: {
            const FilePath modulePath = FilePath::fromUserInput(data["modulepath"].data());
            Symbols symbols;
            for (const GdbMi &item : data["symbols"]) {
                Symbol symbol;
                symbol.state = item["state"].data();
                symbol.address = item["address"].data();
                symbol.name = item["name"].data();
                symbol.section = item["section"].data();
                symbol.demangled = item["demangled"].data();
                symbols.push_back(symbol);
            }
            showModuleSymbols(modulePath, symbols);
            break;
        }
        case RefreshKind::ModuleSections: {
            const FilePath modulePath = FilePath::fromUserInput(data["modulepath"].data());
            Sections sections;
            for (const GdbMi &item : data["sections"]) {
                Section section;
                section.from = item["from"].data();
                section.to = item["to"].data();
                section.address = item["address"].data();
                section.name = item["name"].data();
                section.flags = item["flags"].data();
                sections.push_back(section);
            }
            showModuleSections(modulePath, sections);
            break;
        }
        case RefreshKind::Threads:
            threadsHandler()->setThreads(data);
            break;
        case RefreshKind::DebuggingHelpers:
            watchHandler()->addDumpers(data["dumpers"]);
            break;
        default:
            break;
        }
    });
    connect(m_backend.get(), &DebuggerEngineInterface::memoryDataReceived, this,
            [this](quint64 requestId, quint64 address, const QByteArray &data) {
        if (MemoryAgent *agent = m_pendingMemoryRequests.take(requestId))
            agent->addData(address, data);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::disassemblyReceived, this,
            [this](quint64 requestId, const DisassemblerLines &lines) {
        if (DisassemblerAgent *agent = m_pendingDisassemblyRequests.take(requestId))
            agent->setContents(lines);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::watchpointTriggered, this,
            [this](const QString &responseId, const QString &expression,
                   const QString &oldValue, const QString &newValue) {
        QString message;
        if (const Breakpoint bp = breakHandler()->findBreakpointByResponseId(responseId)) {
            if (bp->type() == WatchpointAtExpression)
                message = bp->msgWatchpointByExpressionTriggered(bp->expression());
            else if (bp->type() == WatchpointAtAddress)
                message = bp->msgWatchpointByAddressTriggered(expression.mid(1)
                                                                  .toULongLong(nullptr, 0));
        }
        if (!oldValue.isEmpty() && !newValue.isEmpty()) {
            if (!message.isEmpty())
                message += ' ';
            message += Tr::tr("Value changed from %1 to %2.").arg(oldValue, newValue);
        }
        if (!message.isEmpty())
            showStatusMessage(message);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::breakpointTriggered, this,
            [this](const QString &responseId, const QString &threadId) {
        if (const Breakpoint bp = breakHandler()->findBreakpointByResponseId(responseId))
            showStatusMessage(bp->msgBreakpointTriggered(threadId));
    });
    connect(m_backend.get(), &DebuggerEngineInterface::watchPointResolved, this,
            [this](quint64, quint64 address, const QString &expr) {
        if (address == 0)
            showMessage(Tr::tr("Could not find a widget."), StatusBar);
        watchHandler()->watchExpression(expr, QString(), true);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::snapshotCreated, this,
            [this](quint64, bool ok, const FilePath &coreFile) {
        if (ok)
            emit attachToCoreRequested(coreFile);
        else
            AsynchronousMessageBox::critical(Tr::tr("Snapshot Creation Error"),
                                              Tr::tr("Cannot create snapshot."));
    });
    connect(settings().createFullBacktrace.action(), &QAction::triggered, this, [this] {
        m_backend->refresh({m_nextFullBacktraceRequestId++, RefreshKind::FullBacktrace});
    });
    connect(m_backend.get(), &DebuggerEngineInterface::inferiorEvent, this,
            [this](InferiorEvent event) {
        switch (event) {
        case InferiorEvent::RunOk: notifyInferiorRunOk(); break;
        case InferiorEvent::RunFailed: notifyInferiorRunFailed(); break;
        case InferiorEvent::RunRequested: notifyInferiorRunRequested(); break;
        case InferiorEvent::StopOk:
            notifyInferiorStopOk();
            reloadStack(settings().maximalStackDepth());
            reloadThreads();
            break;
        case InferiorEvent::StopFailed: notifyInferiorStopFailed(); break;
        case InferiorEvent::SpontaneousStop:
            notifyInferiorSpontaneousStop();
            reloadStack(settings().maximalStackDepth());
            reloadThreads();
            break;
        case InferiorEvent::InferiorIll: notifyInferiorIll(); break;
        case InferiorEvent::ShutdownFinished: notifyInferiorShutdownFinished(); break;
        case InferiorEvent::EngineSetupOk:
            notifyEngineSetupOk();
            peripheralRegisterHandler()->updateRegisterGroups();
            if (runParameters().startMode() != AttachToCore)
                BreakpointManager::claimBreakpointsForEngine(this);
            break;
        case InferiorEvent::EngineSetupFailed: notifyEngineSetupFailed(); break;
        case InferiorEvent::EngineRunFailed: notifyEngineRunFailed(); break;
        case InferiorEvent::EngineIll: notifyEngineIll(); break;
        case InferiorEvent::EngineShutdownFinished: notifyEngineShutdownFinished(); break;
        case InferiorEvent::EngineSpontaneousShutdown: notifyEngineSpontaneousShutdown(); break;
        case InferiorEvent::RunAndInferiorStopOk:
            notifyEngineRunAndInferiorStopOk();
            reloadStack(settings().maximalStackDepth());
            reloadThreads();
            break;
        case InferiorEvent::RunAndInferiorRunOk: notifyEngineRunAndInferiorRunOk(); break;
        case InferiorEvent::RunOkAndInferiorUnrunnable:
            notifyEngineRunOkAndInferiorUnrunnable();
            reloadFullStack();
            reloadModules();
            updateLocals();
            break;
        }
    });
}

// m_backend is the first member, so it is destroyed last - after every member a
// slot of this class touches, and before ~QObject drops the connections to it.
// Nothing it emits on its way out may reach us any more.
GenericDebuggerEngine::~GenericDebuggerEngine()
{
    m_backend->disconnect();
}

void GenericDebuggerEngine::setupEngine()
{
    m_backend->start();
}

void GenericDebuggerEngine::shutdownInferior()
{
    m_backend->shutdownInferior(runParameters().closeMode() == DetachAtClose
                                ? ShutdownMode::Detach : ShutdownMode::Kill);
}

void GenericDebuggerEngine::shutdownEngine()
{
    m_backend->shutdownEngine();
}

bool GenericDebuggerEngine::hasCapability(unsigned cap) const
{
    return m_backend->hasCapability(cap, runParameters().startMode());
}

bool GenericDebuggerEngine::acceptsBreakpoint(const BreakpointParameters &bp) const
{
    const auto &accepts = m_backend->setupData().acceptsBreakpoint;
    if (!accepts)
        return true;
    AcceptsBreakpointQuery query;
    query.type = bp.type;
    query.fileName = bp.fileName;
    query.startMode = runParameters().startMode();
    query.isNativeMixedEnabled = isNativeMixedEnabled();
    return accepts(query);
}

// The file a breakpoint names is spelled the way the debug information has it,
// which is what BreakpointItem::addToCommand() does for the old engines.
BreakpointParameters GenericDebuggerEngine::parametersForDebugger(const Breakpoint &bp) const
{
    BreakpointParameters params = bp->requestedParameters();
    if (params.pathUsage != BreakpointUseShortPath && params.fileName.isLocal()
            && settings().resolveBreakpointSymlinks()) {
        // lldb, unlike gdb, does not resolve a symbolic link in a breakpoint
        // path itself (QTCREATORBUG-17554).
        params.fileName = params.fileName.resolveSymlinks();
    }
    params.fileName = runParameters().buildDirectory().withNewMappedPath(params.fileName);
    return params;
}

void GenericDebuggerEngine::insertBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    notifyBreakpointInsertProceeding(bp);
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = m_nextBreakpointRequestId++;
    request.params = parametersForDebugger(bp);
    request.modelId = bp->modelId();
    m_pendingBreakpoints[request.requestId] = bp;
    m_backend->changeBreakpoint(request);
}

void GenericDebuggerEngine::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    if (bp->responseId().isEmpty()) {
        // The insertion is still in flight, and what answers it takes the
        // removal over from there.
        return;
    }
    notifyBreakpointRemoveProceeding(bp);
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Remove;
    request.requestId = m_nextBreakpointRequestId++;
    request.responseId = bp->responseId();
    request.params = parametersForDebugger(bp);
    request.modelId = bp->modelId();
    m_pendingBreakpoints[request.requestId] = bp;
    m_backend->changeBreakpoint(request);
}

void GenericDebuggerEngine::updateBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    notifyBreakpointChangeProceeding(bp);
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Update;
    request.requestId = m_nextBreakpointRequestId++;
    request.responseId = bp->responseId();
    request.params = parametersForDebugger(bp);
    request.modelId = bp->modelId();
    m_pendingBreakpoints[request.requestId] = bp;
    m_backend->changeBreakpoint(request);
}

void GenericDebuggerEngine::enableSubBreakpoint(const SubBreakpoint &sbp, bool enabled)
{
    QTC_ASSERT(sbp, return);
    Breakpoint bp = sbp->breakpoint();
    QTC_ASSERT(bp, return);
    BreakpointChangeRequest request;
    request.op = BreakpointOp::EnableSub;
    request.requestId = m_nextBreakpointRequestId++;
    request.subResponseId = sbp->responseId;
    request.enabled = enabled;
    m_pendingBreakpoints[request.requestId] = bp;
    m_backend->changeBreakpoint(request);
}

void GenericDebuggerEngine::handleBreakpointEvent(quint64 requestId, BreakpointOp op, bool ok,
                                                  const GdbMi &data)
{
    if (requestId == 0) {
        const QString responseId = data["number"].data();
        if (responseId.isEmpty())
            return;
        if (op == BreakpointOp::Remove) {
            breakHandler()->removeAlienBreakpoint(responseId);
            return;
        }
        BreakpointParameters params;
        params.type = BreakpointByFileAndLine;
        params.updateFromGdbOutput(data, runParameters());
        breakHandler()->handleAlienBreakpoint(responseId, params);
        return;
    }

    const Breakpoint bp = m_pendingBreakpoints.take(requestId);
    if (!bp) {
        showMessage(QString("GenericDebuggerEngine: breakpoint event for unknown request %1")
                        .arg(requestId));
        return;
    }

    switch (op) {
    case BreakpointOp::Insert:
        if (bp->state() == BreakpointRemoveRequested && ok && data.childCount() > 0) {
            const QString nr = data.childAt(0)["number"].data();
            if (!nr.isEmpty()) {
                notifyBreakpointRemoveProceeding(bp);
                BreakpointChangeRequest removeRequest;
                removeRequest.op = BreakpointOp::Remove;
                removeRequest.requestId = m_nextBreakpointRequestId++;
                removeRequest.responseId = nr;
                // The backend tells an interpreter breakpoint from a C++ one
                // by the parameters, and without them sends the number the
                // interpreter handed out to lldb's own numbering.
                removeRequest.params = parametersForDebugger(bp);
                removeRequest.modelId = bp->modelId();
                m_pendingBreakpoints[removeRequest.requestId] = bp;
                m_backend->changeBreakpoint(removeRequest);
                break;
            }
        }
        if (ok) {
            for (const GdbMi &bkpt : data)
                applyBkptData(bkpt, bp);
            notifyBreakpointInsertOk(bp);
        } else {
            notifyBreakpointInsertFailed(bp);
        }
        break;
    case BreakpointOp::Remove:
        if (ok)
            notifyBreakpointRemoveOk(bp);
        else
            notifyBreakpointRemoveFailed(bp);
        break;
    case BreakpointOp::Update:
        if (ok) {
            // A backend that cannot change a breakpoint puts a new one in its
            // place, and the model addresses it by number from here on.
            for (const GdbMi &bkpt : data)
                applyBkptData(bkpt, bp);
            notifyBreakpointChangeOk(bp);
        } else {
            notifyBreakpointChangeFailed(bp);
        }
        break;
    case BreakpointOp::EnableSub:
        break;
    }
}

// A report the interpreter made spells its fields the service's way - "1"
// where gdb writes "y" - so reading it as gdb output turns an enabled
// breakpoint into a disabled one. Only such a report carries the model id its
// request went out with, which is what tells the two apart.
static bool isInterpreterBkptData(const GdbMi &bkpt)
{
    return bkpt["modelid"].toInt() != 0;
}

void GenericDebuggerEngine::applyInterpreterBkptData(const GdbMi &bkpt, const Breakpoint &bp)
{
    // The number the interpreter assigned is the only handle the backend takes
    // for removing or changing it later.
    bp->setResponseId(bkpt["number"].data());
    bp->setEnabled(bkpt["enabled"].toInt());
    bp->setCondition(bkpt["condition"].data());
    bp->setIgnoreCount(bkpt["ignorecount"].toInt());
    bp->setTextPosition({bkpt["line"].toInt(), -1});
    bp->setPending(false);
}

void GenericDebuggerEngine::applyBkptData(const GdbMi &bkpt, const Breakpoint &bp)
{
    if (isInterpreterBkptData(bkpt)) {
        applyInterpreterBkptData(bkpt, bp);
        return;
    }

    // A pseudo tracepoint prints a message rather than stopping, and each of
    // its locations does, so the message travels to them as well.
    const bool isPseudoTracepoint = bp->isTracepoint() && settings().usePseudoTracepoints();
    const auto applyToSub = [this, &bp, isPseudoTracepoint](const GdbMi &data,
                                                            const SubBreakpoint &sub) {
        sub->params.updateFromGdbOutput(data, runParameters());
        sub->params.type = bp->type();
        if (isPseudoTracepoint) {
            sub->params.tracepoint = true;
            sub->params.message = bp->message();
        }
    };

    const QString nr = bkpt["number"].data();
    if (nr.contains('.')) {
        SubBreakpoint sub = bp->findOrCreateSubBreakpoint(nr);
        QTC_ASSERT(sub, return);
        applyToSub(bkpt, sub);
        return;
    }

    const GdbMi locations = bkpt["locations"];
    if (locations.isValid()) {
        for (const GdbMi &location : locations) {
            const QString subnr = location["number"].data();
            SubBreakpoint sub = bp->findOrCreateSubBreakpoint(subnr);
            QTC_ASSERT(sub, return);
            applyToSub(location, sub);
        }
    }

    bp->setResponseId(nr);
    bp->updateFromGdbOutput(bkpt, runParameters());
    if (isPseudoTracepoint)
        bp->setMessage(bp->requestedParameters().message);
}

void GenericDebuggerEngine::handleBreakpointModified(const GdbMi &data)
{
    BreakHandler *handler = breakHandler();
    Breakpoint bp;
    for (const GdbMi &bkpt : data) {
        const QString nr = bkpt["number"].data();
        if (nr.contains('.')) {
            QTC_ASSERT(bp, continue);
            SubBreakpoint sub = bp->findOrCreateSubBreakpoint(nr);
            sub->params.updateFromGdbOutput(bkpt, runParameters());
            sub->params.type = bp->type();
            if (bp->isTracepoint()) {
                sub->params.tracepoint = true;
                sub->params.message = bp->message();
            }
        } else {
            if (isInterpreterBkptData(bkpt)) {
                bp = handler->findBreakpointByModelId(bkpt["modelid"].toInt());
                if (bp) {
                    applyInterpreterBkptData(bkpt, bp);
                    continue;
                }
            }
            bp = handler->findBreakpointByResponseId(nr);
            if (bp)
                bp->updateFromGdbOutput(bkpt, runParameters());
        }
    }
    if (bp)
        bp->adjustMarker();
}

void GenericDebuggerEngine::handleNotResponding(std::chrono::seconds waited,
                                                const QStringList &pendingCommands,
                                                NotRespondingCause cause)
{
    showMessage(QString("TIMED OUT WAITING FOR A REPLY. COMMANDS STILL IN PROGRESS: %1")
                    .arg(pendingCommands.join(", ")));
    if (m_notRespondingPending)
        return;
    m_notRespondingPending = true;
    const bool fetching = cause == NotRespondingCause::FetchingDebugInfo;
    CheckableMessageBox::question_async(
        fetching ? Tr::tr("Downloading Debug Information")
                 : Tr::tr("Debugger Not Responding"),
        fetching ? Tr::tr("The debugger is downloading debug information, which can take a "
                          "long time on a slow connection, and none has arrived for the last "
                          "%n seconds. The server may also be unreachable.<br/>You can keep "
                          "waiting or stop debugging.", nullptr, int(waited.count()))
                 : Tr::tr("The debugger process has not responded to a command within %n "
                          "seconds. This could mean it is stuck in an endless loop or taking "
                          "longer than expected to perform the operation.<br/>You can choose "
                          "between waiting longer or aborting debugging.",
                          nullptr, int(waited.count())),
        {}, this,
        [this](QMessageBox::StandardButton button) {
            m_notRespondingPending = false;
            if (button != QMessageBox::Ok)
                return;
            // An unresponsive debugger will not act on a graceful shutdown either.
            showMessage("KILLING THE DEBUGGER AS REQUESTED BY THE USER");
            m_backend->execute({ExecutionCommand::Abort});
        },
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel, QMessageBox::NoButton,
        {{QMessageBox::Ok, Tr::tr("Stop Debugging")},
         {QMessageBox::Cancel, Tr::tr("Give the Debugger More Time")}});
}

void GenericDebuggerEngine::askAboutQtSymbols(const FilePath &module)
{
    if (!m_backend->hasExtraCapability(DebuggerExtraCapability::ModuleSymbolState))
        return;
    static const QRegularExpression debugQtCore("(Qt\\dCored)\\.dll");
    const QRegularExpressionMatch match = debugQtCore.match(module.fileName());
    if (!match.hasMatch())
        return;
    const FilePath modulePath = module.parentDir();
    const QtSupport::QtVersion *version = QtSupport::QtVersionManager::version(
        [modulePath](const QtSupport::QtVersion *version) {
            return version->detectionSource().isAutoDetected()
                   && version->binPath() == modulePath;
        });
    if (!version)
        return;
    const QString moduleName = match.captured(1);
    if (modulePath.pathAppended(moduleName + ".pdb").exists())
        return;
    m_qtVersionOfModule.insert(moduleName, version->displayName());
    RefreshRequest request;
    request.kind = RefreshKind::ModuleSymbolState;
    request.requestId = m_nextRefreshRequestId++;
    request.path = FilePath::fromString(moduleName);
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reportMissingQtSymbols(const QString &module)
{
    const QString qtName = m_qtVersionOfModule.take(module);
    if (qtName.isEmpty())
        return;
    showMessage("Missing Qt Debug Information Files package for " + qtName, LogMisc);
    CheckableMessageBox::information(Tr::tr("Missing Qt Debug Information"),
        Tr::tr("The installed %1 is missing debug information files.\n"
               "Locals and Expression might not be able to display all Qt types in a "
               "human readable format.\n\n"
               "Install the \"Qt Debug Information Files\" Package from the "
               "Maintenance Tool for this Qt installation to get all relevant "
               "symbols for the debugger.").arg(qtName),
        Key("CdbQtSdkPdbHint"));
}

void GenericDebuggerEngine::handleStopReasonReported(const QString &reason)
{
    showStatusMessage(msgStopped(reason));
}

void GenericDebuggerEngine::handleSignalReceived(const QString &name, const QString &meaning)
{
    if (name == stopSignal(runParameters().toolChainAbi()) || runParameters().expectedSignals().contains(name)) {
        if (!name.isEmpty() && !meaning.isEmpty())
            showStatusMessage(msgStoppedBySignal(meaning, name));
        return;
    }
    if (name.isEmpty()) {
        if (settings().useMessageBoxForSignals())
            showStoppedByExceptionMessageBox(meaning);
        showStatusMessage(meaning.isEmpty() ? msgStopped() : meaning);
        return;
    }
    if (settings().useMessageBoxForSignals() && !showStoppedBySignalMessageBox(meaning, name))
        return;
    showStatusMessage(msgStoppedBySignal(meaning, name));
}

void GenericDebuggerEngine::selectThread(const Thread &thread)
{
    QTC_ASSERT(thread, return);
    m_backend->selectThread(thread->id());
    reloadStack(settings().maximalStackDepth());
}

void GenericDebuggerEngine::activateFrame(int index)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    StackHandler *handler = stackHandler();
    if (handler->isSpecialFrame(index)) {
        reloadFullStack();
        return;
    }

    QTC_ASSERT(index >= 0 && index < handler->stackSize(), return);
    handler->setCurrentIndex(index);
    gotoCurrentLocation();

    const StackFrame &frame = handler->frameAt(index);
    if (frame.language != QmlLanguage) {
        bool ok = false;
        const int level = frame.level.toInt(&ok);
        m_backend->activateFrame(ok ? level : index);
    }

    updateLocals();
    reloadRegisters();
    reloadPeripheralRegisters();
}

void GenericDebuggerEngine::updateAll()
{
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
    reloadStack(settings().maximalStackDepth());
    reloadThreads();
    reloadRegisters();
    reloadPeripheralRegisters();
    updateLocals();
}

void GenericDebuggerEngine::reloadModules()
{
    if (state() != InferiorRunOk && state() != InferiorStopOk)
        return;

    RefreshRequest request;
    request.kind = RefreshKind::Modules;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::requestModuleSymbols(const FilePath &moduleName)
{
    RefreshRequest request;
    request.kind = RefreshKind::ModuleSymbols;
    request.requestId = m_nextRefreshRequestId++;
    request.path = moduleName;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::requestModuleSections(const FilePath &moduleName)
{
    RefreshRequest request;
    request.kind = RefreshKind::ModuleSections;
    request.requestId = m_nextRefreshRequestId++;
    request.path = moduleName;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reloadStack(int depthLimit)
{
    RefreshRequest request;
    request.kind = RefreshKind::FullStack;
    request.requestId = m_nextRefreshRequestId++;
    request.stackDepthLimit = depthLimit;
    if (depthLimit <= 0)
        m_fullStackRequestId = request.requestId;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reloadFullStack()
{
    reloadStack(-1);
}

void GenericDebuggerEngine::reloadThreads()
{
    RefreshRequest request;
    request.kind = RefreshKind::Threads;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::loadAdditionalQmlStack()
{
    RefreshRequest request;
    request.kind = RefreshKind::QmlStack;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::loadSymbolsForStack()
{
    bool needUpdate = false;
    const Modules modules = modulesHandler()->modules();
    stackHandler()->forItemsAtLevel<2>([this, &modules, &needUpdate](StackFrameItem *frameItem) {
        const StackFrame &frame = frameItem->frame;
        if (frame.function == "??") {
            for (const Module &module : modules) {
                if (module.startAddress <= frame.address && frame.address < module.endAddress) {
                    RefreshRequest request;
                    request.kind = RefreshKind::StackSymbols;
                    request.requestId = m_nextRefreshRequestId++;
                    request.path = module.modulePath;
                    m_backend->refresh(request);
                    needUpdate = true;
                }
            }
        }
    });
    if (needUpdate) {
        reloadStack(settings().maximalStackDepth());
        updateLocals();
    }
}

void GenericDebuggerEngine::loadSymbols(const FilePath &moduleName)
{
    RefreshRequest request;
    request.kind = RefreshKind::StackSymbols;
    request.requestId = m_nextRefreshRequestId++;
    request.path = moduleName;
    m_backend->refresh(request);
    reloadModules();
    reloadStack(settings().maximalStackDepth());
    updateLocals();
}

void GenericDebuggerEngine::reloadRegisters()
{
    if (!isRegistersWindowVisible())
        return;
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    RefreshRequest request;
    request.kind = RefreshKind::Registers;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reloadPeripheralRegisters()
{
    if (!isPeripheralRegistersWindowVisible())
        return;

    const QList<quint64> addresses = peripheralRegisterHandler()->activeRegisters();
    if (addresses.isEmpty())
        return;

    RefreshRequest request;
    request.kind = RefreshKind::PeripheralRegisters;
    request.requestId = m_nextRefreshRequestId++;
    request.addresses = addresses;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reloadSourceFiles()
{
    if (state() != InferiorRunOk && state() != InferiorStopOk)
        return;

    RefreshRequest request;
    request.kind = RefreshKind::SourceFiles;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

FilePath GenericDebuggerEngine::cleanupFullName(const QString &fileName)
{
    FilePath cleanFilePath =
        runParameters().projectSourceDirectory().withNewPath(fileName).cleanPath();

    if (HostOsInfo::isWindowsHost() && fileName.isEmpty())
        return {};

    if (cleanFilePath.isReadableFile())
        return cleanFilePath;

    const FilePath onDebuggerDevice = runParameters().findOnDebuggerDevice(fileName);
    if (!onDebuggerDevice.isEmpty())
        return onDebuggerDevice;

    if (!settings().autoEnrichParameters())
        return cleanFilePath;

    const FilePath sysroot = runParameters().sysRoot();
    if (!sysroot.isEmpty() && fileName.startsWith('/')) {
        cleanFilePath = sysroot.pathAppended(fileName.mid(1));
        if (cleanFilePath.isReadableFile())
            return cleanFilePath;
    }
    if (m_baseNameToFullName.isEmpty()) {
        const FilePath filePath = sysroot.pathAppended("/usr/src/debug");
        if (filePath.isDir()) {
            filePath.iterateDirectory(
                [this](const FilePath &filePath) {
                    const QString name = filePath.fileName();
                    if (!name.startsWith('.'))
                        m_baseNameToFullName.insert(name, filePath);
                    return IterationPolicy::Continue;
                },
                FileFilter{{"*"}, DirFilterFlag::NoFilter, DirIteratorFlag::Subdirectories});
        }
    }

    const QString base = FilePath::fromUserInput(fileName).fileName();
    const auto jt = m_baseNameToFullName.constFind(base);
    if (jt != m_baseNameToFullName.constEnd() && jt.key() == base) {
        return jt.value();
    }

    return {};
}

void GenericDebuggerEngine::loadAllSymbols()
{
    RefreshRequest request;
    request.kind = RefreshKind::AllSymbols;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reloadDebuggingHelpers()
{
    RefreshRequest request;
    request.kind = RefreshKind::DebuggingHelpers;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
}

void GenericDebuggerEngine::setRegisterValue(const QString &name, const QString &value)
{
    m_backend->setRegisterValue(name, value);
    reloadRegisters();
}

void GenericDebuggerEngine::setPeripheralRegisterValue(quint64 address, quint64 value)
{
    m_backend->setPeripheralRegisterValue(address, value);
    reloadPeripheralRegisters();
}

static WatchItemData watchItemData(const WatchItem *item)
{
    WatchItemData data;
    data.id = item->id;
    data.iname = item->iname;
    data.type = item->type;
    data.isLocal = item->isLocal();
    data.isWatcher = item->isWatcher();
    data.isInspect = item->isInspect();
    return data;
}

void GenericDebuggerEngine::assignValueInDebugger(WatchItem *item, const QString &expression,
                                                  const QVariant &value)
{
    QTC_ASSERT(item, return);
    m_backend->assignValueInDebugger(watchItemData(item), expression, value.toString());
    updateLocals();
}

void GenericDebuggerEngine::fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length)
{
    const quint64 requestId = m_nextMemoryRequestId++;
    m_pendingMemoryRequests[requestId] = agent;
    m_backend->accessMemory(MemoryOp::Fetch, requestId, addr, length);
}

void GenericDebuggerEngine::changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data)
{
    m_backend->accessMemory(MemoryOp::Change, 0, addr, 0, data);
    updateLocals();
}

void GenericDebuggerEngine::fetchDisassembler(DisassemblerAgent *agent)
{
    const quint64 requestId = m_nextDisassemblyRequestId++;
    m_pendingDisassemblyRequests[requestId] = agent;
    m_backend->fetchDisassembly(requestId, agent->address(), agent->location().functionName());
}

void GenericDebuggerEngine::watchPoint(const QPoint &pnt)
{
    m_backend->watchPoint(m_nextWatchPointRequestId++, pnt);
}

void GenericDebuggerEngine::createSnapshot()
{
    m_backend->createSnapshot(m_nextSnapshotRequestId++);
}

static bool currentFrameIsQml(StackHandler *handler)
{
    return handler->stackSize() > 0 && handler->currentFrame().language == QmlLanguage;
}

void GenericDebuggerEngine::continueInferior()
{
    ExecutionRequest request;
    request.command = ExecutionCommand::Continue;
    request.currentFrameIsQml = currentFrameIsQml(stackHandler());
    request.reverse = isReverseDebugging();
    m_backend->execute(request);
}

void GenericDebuggerEngine::interruptInferior()
{
    m_backend->execute({ExecutionCommand::Interrupt});
}

void GenericDebuggerEngine::executeStepOver(bool byInstruction)
{
    ExecutionRequest request;
    request.command = ExecutionCommand::StepOver;
    request.flag = byInstruction;
    request.currentFrameIsQml = currentFrameIsQml(stackHandler());
    request.reverse = isReverseDebugging();
    m_backend->execute(request);
}

void GenericDebuggerEngine::executeStepIn(bool byInstruction)
{
    ExecutionRequest request;
    request.command = ExecutionCommand::StepIn;
    request.flag = byInstruction;
    request.currentFrameIsQml = currentFrameIsQml(stackHandler());
    request.reverse = isReverseDebugging();
    m_backend->execute(request);
}

void GenericDebuggerEngine::executeStepOut()
{
    ExecutionRequest request;
    request.command = ExecutionCommand::StepOut;
    request.currentFrameIsQml = currentFrameIsQml(stackHandler());
    request.reverse = isReverseDebugging();
    m_backend->execute(request);
}

void GenericDebuggerEngine::executeReturn()
{
    m_backend->execute({ExecutionCommand::Return});
}

void GenericDebuggerEngine::executeRunToLine(const ContextData &data)
{
    m_backend->execute({ExecutionCommand::RunToLine, false, data});
}

void GenericDebuggerEngine::executeRunToFunction(const QString &functionName)
{
    m_backend->execute({ExecutionCommand::RunToFunction, false, {}, functionName});
}

void GenericDebuggerEngine::executeJumpToLine(const ContextData &data)
{
    m_backend->execute({ExecutionCommand::JumpToLine, false, data});
}

void GenericDebuggerEngine::executeRecordReverse(bool record)
{
    m_backend->execute({ExecutionCommand::RecordReverse, record});
}

void GenericDebuggerEngine::debugLastCommand()
{
    m_backend->execute({ExecutionCommand::RepeatLastCommand});
}

void GenericDebuggerEngine::detachDebugger()
{
    m_backend->execute({ExecutionCommand::Detach});
}

void GenericDebuggerEngine::resetInferior()
{
    m_backend->execute({ExecutionCommand::ResetInferior});
}

void GenericDebuggerEngine::abortDebuggerProcess()
{
    m_backend->execute({ExecutionCommand::Abort});
}

void GenericDebuggerEngine::executeDebuggerCommand(const QString &command)
{
    WatchItemData inspectorItem;
    if (WatchTreeView *view = inspectorView()) {
        const WatchItem *item = watchHandler()->watchItem(view->currentIndex());
        if (item && item->isInspect())
            inspectorItem = watchItemData(item);
    }
    m_backend->executeDebuggerCommand(command, inspectorItem);
}

void GenericDebuggerEngine::expandItem(const QString &iname)
{
    if (!iname.startsWith("inspect.")) {
        DebuggerEngine::expandItem(iname);
        return;
    }
    refreshInspectorTree();
}

void GenericDebuggerEngine::refreshInspectorTree()
{
    if (!settings().showQmlObjectTree())
        return;

    RefreshRequest request;
    request.kind = RefreshKind::InspectorTree;
    request.requestId = m_nextRefreshRequestId++;
    request.expandedINames = watchHandler()->expandedINames();
    m_backend->refresh(request);
}

void GenericDebuggerEngine::doUpdateLocals(const UpdateParameters &params)
{
    watchHandler()->notifyUpdateStarted(params);

    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.requestId = m_nextRefreshRequestId++;
    request.partialVariable = params.partialVariable;
    request.context = stackHandler()->currentFrame().context;
    DebuggerCommand watchersCmd;
    watchHandler()->appendWatchersAndTooltipRequests(&watchersCmd);
    request.watchers = watchersCmd.args.toObject().value("watchers").toArray();
    request.expandedINames = watchHandler()->expandedINames();
    DebuggerCommand formatsCmd;
    watchHandler()->appendFormatRequests(&formatsCmd);
    const QJsonObject formatArgs = formatsCmd.args.toObject();
    request.expandedItems = formatArgs.value("expanded").toObject();
    request.typeFormats = formatArgs.value("typeformats").toObject();
    request.individualFormats = formatArgs.value("formats").toObject();
    request.formatTypes = formatArgs.value("formattypes").toObject();
    request.allowInferiorCalls = settings().allowInferiorCalls();
    request.autoDerefPointers = settings().autoDerefPointers();
    const DebuggerSettings &s = settings();
    request.dumperOptions = {.useDebuggingHelpers = s.useDebuggingHelpers(),
                             .useDynamicType = s.useDynamicType(),
                             .showQObjectNames = s.showQObjectNames(),
                             .logTimeStamps = s.logTimeStamps(),
                             .maximalStringLength = int(s.maximalStringLength()),
                             .displayStringLimit = int(s.displayStringLimit())};
    if (s.useCodeModel()) {
        const StackFrame frame = stackHandler()->currentFrame();
        request.uninitializedVariables = getUninitializedVariables(
            CppEditor::CppModelManager::snapshot(), frame.function, frame.file, frame.line);
    }
    m_backend->refresh(request);
}
} // namespace Debugger::Internal
