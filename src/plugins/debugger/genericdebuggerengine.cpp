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

#include "console/console.h"
#include "console/consoleitem.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppmodelmanager.h>
#include <qtsupport/qtversionmanager.h>
#include <projectexplorer/taskhub.h>

#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/widgets.h>

#include <QGuiApplication>
#include <QMessageBox>

#include <optional>

using namespace Utils;

namespace Debugger::Internal {

ConsoleItem *GenericDebuggerEngine::toConsoleItem(const GdbMi &data)
{
    const QString type = data["type"].data();
    const ConsoleItem::ItemType itemType = type == "debug"     ? ConsoleItem::DebugType
                                           : type == "warning" ? ConsoleItem::WarningType
                                           : type == "error"   ? ConsoleItem::ErrorType
                                                               : ConsoleItem::DefaultType;
    if (const GdbMi handle = data["handle"]; handle.isValid()) {
        return new ConsoleItem(itemType, data["text"].data(),
                               consoleFetch(handle.data().toLongLong()));
    }
    const GdbMi line = data["line"];
    auto item = new ConsoleItem(itemType, data["text"].data(), data["file"].data(),
                                line.isValid() ? line.toInt() : -1);
    appendConsoleChildren(item, data["children"]);
    return item;
}

std::function<void(ConsoleItem *)> GenericDebuggerEngine::consoleFetch(qint64 handle)
{
    return [engine = QPointer(this), handle](ConsoleItem *item) {
        // The console outlives the session that printed the entry, and the
        // next session makes it expandable again.
        if (!engine)
            return;
        // Refused silently: expanding a level fetches every child at once.
        if (!engine->answersConsoleInput()) {
            item->setFetch(engine->consoleFetch(handle));
            return;
        }
        RefreshRequest request;
        request.kind = RefreshKind::ConsoleItem;
        request.requestId = engine->m_nextRefreshRequestId++;
        request.handle = handle;
        engine->m_pendingConsoleFetches.insert(request.requestId,
                                               PendingConsoleFetch{item->index(), item});
        engine->m_backend->refresh(request);
    };
}

// Sorted before they are attached: sorting attached children would move
// rows the console view has cached indices for.
void GenericDebuggerEngine::appendConsoleChildren(ConsoleItem *item, const GdbMi &children)
{
    QList<ConsoleItem *> items;
    for (const GdbMi &child : children)
        items.append(toConsoleItem(child));
    if (settings().sortStructMembers()) {
        std::stable_sort(items.begin(), items.end(), [](ConsoleItem *a, ConsoleItem *b) {
            return a->text() < b->text();
        });
    }
    for (ConsoleItem *child : std::as_const(items))
        item->appendChild(child);
}

void GenericDebuggerEngine::handleConsoleItemFetched(quint64 requestId, const GdbMi &data)
{
    const PendingConsoleFetch fetch = m_pendingConsoleFetches.take(requestId);
    // The console may have been cleared while the answer was on its way.
    if (!fetch.index.isValid())
        return;
    appendConsoleChildren(fetch.item, data["children"]);
}

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
    setRunParametersValidationEnabled(m_backend->setupData().validatesRunParameters);

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
    connect(m_backend.get(), &DebuggerEngineInterface::consoleItemReported, this,
            [this](const GdbMi &item) { debuggerConsole()->printItem(toConsoleItem(item)); });
    connect(m_backend.get(), &DebuggerEngineInterface::consoleObjectContextReported, this,
            [this](bool inSelectedObject) {
        m_consoleInSelectedObject = inSelectedObject;
        updateConsoleContext();
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
            m_commandMovedSelection = true;
            m_selectedFrameLevel = data["frame"]["level"].data();
            // The reread reads the stack of the thread the backend has as its own.
            const Thread current = threadsHandler()->currentThread();
            if (!id.isEmpty() && (!current || current->id() != id)) {
                m_backend->selectThread(id);
                if (const Thread selected = threadsHandler()->threadForId(id))
                    threadsHandler()->setCurrentThread(selected);
            }
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
    connect(m_backend.get(), &DebuggerEngineInterface::recordingFailed,
            this, &GenericDebuggerEngine::handleRecordingFailed);
    connect(m_backend.get(), &DebuggerEngineInterface::debuggerCommandDone,
            this, &GenericDebuggerEngine::handleDebuggerCommandDone);
    connect(m_backend.get(), &DebuggerEngineInterface::notResponding,
            this, &GenericDebuggerEngine::handleNotResponding);
    connect(m_backend.get(), &DebuggerEngineInterface::startFailed,
            this, [this](const QString &title, const QString &message, const Key &settingsKey) {
        showMessage(message, LogError);
        CheckableMessageBox::information(title, message, settingsKey);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::connectionFailed,
            this, &GenericDebuggerEngine::askToRetryConnection);
    connect(m_backend.get(), &DebuggerEngineInterface::refreshFailed,
            this, [this](quint64 requestId, RefreshKind kind, const FilePath &path) {
        if (kind == RefreshKind::ConsoleItem) {
            m_pendingConsoleFetches.remove(requestId);
            return;
        }
        QTC_ASSERT(kind == RefreshKind::ModuleSymbols, return);
        AsynchronousMessageBox::critical(Tr::tr("Cannot Read Symbols"),
            Tr::tr("Cannot read symbols for module \"%1\".").arg(path.toUserOutput()));
    });
    connect(m_backend.get(), &DebuggerEngineInterface::inspectorTreeOutdated,
            this, &GenericDebuggerEngine::refreshInspectorTree);
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
            if (settings().showQmlObjectTree())
                insertInspectorItems(data["data"]);
            break;
        case RefreshKind::FullBacktrace:
            if (!data.data().isEmpty())
                openTextEditor("Backtrace$", data.data());
            break;
        case RefreshKind::FullStack: {
            const GdbMi frames = data["stack"]["frames"];
            // Only a stack fetched with a limit can be truncated, and only a
            // truncated one offers the user the rest of it.
            const GdbMi hasMore = data["stack"]["hasmore"];
            const int depthLimit = m_stackDepthLimits.take(requestId);
            m_shownStackDepthLimit = depthLimit;
            const bool isFull = !frames.isValid()
                                || (hasMore.isValid() ? !hasMore.toInt()
                                                      : depthLimit <= 0
                                                            || frames.childCount() < depthLimit);
            const QString keptLevel = m_frameKeepingStackRequests.take(requestId);
            stackHandler()->setFramesAndCurrentIndex(frames, isFull);
            if (!keptLevel.isEmpty()) {
                for (int row = 0, n = stackHandler()->stackSize(); row < n; ++row) {
                    if (stackHandler()->frameAt(row).level == keptLevel) {
                        stackHandler()->setCurrentIndex(row);
                        break;
                    }
                }
            }
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
                reg.description = item["description"].data();
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
                module.modulePath = modulePathOnDebuggerDevice(item["modulepath"].data());
                const GdbMi moduleName = item["modulename"];
                module.moduleName = moduleName.isValid() ? moduleName.data()
                                                         : module.modulePath.baseName();
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
            const FilePath modulePath = modulePathOnDebuggerDevice(data["modulepath"].data());
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
            const FilePath modulePath = modulePathOnDebuggerDevice(data["modulepath"].data());
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
            if (std::exchange(m_stopThreadsPending, false) && !m_breakpointStopMessage.isEmpty())
                showBreakpointStopMessage();
            break;
        case RefreshKind::DebuggingHelpers:
            watchHandler()->addDumpers(data["dumpers"]);
            break;
        case RefreshKind::ConsoleItem:
            handleConsoleItemFetched(requestId, data);
            break;
        default:
            break;
        }
    });
    connect(m_backend.get(), &DebuggerEngineInterface::memoryRead, this,
            [this](quint64 requestId, const MemoryReadResult &result) {
        if (const MemoryReadCallback callback = m_pendingMemoryReads.take(requestId))
            callback(result);
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
        if (!oldValue.isEmpty() && !newValue.isEmpty() && oldValue != newValue) {
            if (!message.isEmpty())
                message += ' ';
            message += Tr::tr("Value changed from %1 to %2.").arg(oldValue, newValue);
        }
        if (!message.isEmpty())
            showStatusMessage(message);
    });
    connect(m_backend.get(), &DebuggerEngineInterface::breakpointTriggered, this,
            [this](const QString &responseId, const QString &threadId) {
        if (const Breakpoint bp = breakHandler()->findBreakpointByResponseId(responseId)) {
            m_breakpointStopMessage = bp->msgBreakpointTriggered(threadId);
            m_breakpointStopThreadId = threadId;
            showBreakpointStopMessage();
        }
    });
    connect(m_backend.get(), &DebuggerEngineInterface::watchPointResolved, this,
            [this](quint64, quint64 address, const QString &expr) {
        if (address == 0)
            showMessage(Tr::tr("Cannot find a widget at this position."), StatusBar);
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
        // What a backend that was given up on during its setup still says.
        if (state() == DebuggerFinished)
            return;
        switch (event) {
        case InferiorEvent::RunOk:
            m_attachStopUnread = false;
            m_breakpointStopMessage.clear();
            m_breakpointStopThreadId.clear();
            m_stopThreadsPending = true;
            notifyInferiorRunOk();
            break;
        case InferiorEvent::RunFailed:
            notifyInferiorRunFailed();
            // The views were left unfilled on the promise of a resume that did
            // not happen, and the stop they skipped is the only one there is.
            if (std::exchange(m_attachStopUnread, false)) {
                reloadStack(settings().maximalStackDepth());
                reloadThreads();
            }
            break;
        case InferiorEvent::RunRequested: notifyInferiorRunRequested(); break;
        case InferiorEvent::StopOk:
            notifyInferiorStopOk();
            m_stopThreadsPending = true;
            reloadStack(settings().maximalStackDepth());
            reloadThreads();
            break;
        case InferiorEvent::StopFailed: notifyInferiorStopFailed(); break;
        case InferiorEvent::SpontaneousStop:
            notifyInferiorSpontaneousStop();
            m_stopThreadsPending = true;
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
            m_stopThreadsPending = true;
            reloadStack(settings().maximalStackDepth());
            reloadThreads();
            break;
        case InferiorEvent::RunAndInferiorStopOkAndResuming:
            notifyEngineRunAndInferiorStopOk();
            m_attachStopUnread = true;
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

    connect(m_backend.get(), &DebuggerEngineInterface::inspectorToolsAvailable,
            this, &GenericDebuggerEngine::setInspectorToolsAvailable);
    connect(m_backend.get(), &DebuggerEngineInterface::inspectorObjectSelected,
            this, &GenericDebuggerEngine::handleInspectorObjectSelected);
    connect(settings().showAppOnTop.action(), &QAction::triggered, this, [this](bool checked) {
        if (m_inspectorToolsRegistered)
            m_backend->setShowAppOnTop(checked);
    });

    DebuggerSettings &s = settings();
    m_backend->showInspectorTree(s.showQmlObjectTree());
    connect(&s.showQmlObjectTree, &BaseAspect::changed, this, [this] {
        const bool shown = settings().showQmlObjectTree();
        if (!shown)
            clearInspectorTree();
        m_backend->showInspectorTree(shown);
    });
    connect(&s.sortStructMembers, &BaseAspect::changed, this, [this] {
        // Only a backend that filled the inspector tree has one to sort.
        const WatchItem *root = watchHandler()->findItem("inspect");
        if (root && root->childCount() > 0)
            refreshInspectorTree();
    });
    for (BoolAspect *aspect : {&s.useDebuggingHelpers, &s.autoDerefPointers, &s.useDynamicType,
                               &s.allowInferiorCalls}) {
        connect(aspect, &BaseAspect::changed, this, [this] {
            if (showsStoppedState())
                updateLocals();
        });
    }
}

// m_backend is the first member, so it is destroyed last - after every member a
// slot of this class touches, and before ~QObject drops the connections to it.
// Nothing it emits on its way out may reach us any more.
GenericDebuggerEngine::~GenericDebuggerEngine()
{
    m_backend->disconnect();
    setInspectorToolsAvailable(false);
}

static const Core::Context &inspectorToolsContext()
{
    static const Core::Context context("Debugger.QmlInspector");
    return context;
}

void GenericDebuggerEngine::setInspectorToolsAvailable(bool available)
{
    if (available == m_inspectorToolsRegistered)
        return;
    m_inspectorToolsRegistered = available;
    QAction *showAppOnTopAction = settings().showAppOnTop.action();
    if (!available) {
        m_selectToolAction->setEnabled(false);
        showAppOnTopAction->setEnabled(false);
        Core::ActionManager::unregisterAction(m_selectToolAction, Constants::QML_SELECTTOOL);
        Core::ActionManager::unregisterAction(showAppOnTopAction, Constants::QML_SHOW_APP_ON_TOP);
        Core::ICore::removeAdditionalContext(inspectorToolsContext());
        return;
    }
    if (!m_selectToolAction) {
        m_selectToolAction = new QAction(this);
        m_selectToolAction->setObjectName("QML Select Action");
        m_selectToolAction->setCheckable(true);
        connect(m_selectToolAction, &QAction::triggered, this, [this](bool checked) {
            m_backend->setSelectTool(checked);
        });
    }
    showAppOnTopAction->setCheckable(true);
    Core::ICore::addAdditionalContext(inspectorToolsContext());
    Core::ActionManager::registerAction(m_selectToolAction, Constants::QML_SELECTTOOL,
                                        inspectorToolsContext());
    Core::ActionManager::registerAction(showAppOnTopAction, Constants::QML_SHOW_APP_ON_TOP,
                                        inspectorToolsContext());
    m_selectToolAction->setEnabled(state() == InferiorRunOk);
    showAppOnTopAction->setEnabled(state() == InferiorRunOk);
    if (showAppOnTopAction->isChecked())
        m_backend->setShowAppOnTop(true);
}

void GenericDebuggerEngine::selectWatchData(const QString &iname)
{
    if (iname.startsWith("inspect."))
        m_backend->selectInspectorItem(iname);
}

void GenericDebuggerEngine::handleInspectorObjectSelected(const QString &iname,
                                                          const FilePath &file, int line)
{
    if (file.isReadableFile())
        Core::EditorManager::openEditorAt({file, line});
    if (!iname.isEmpty())
        watchHandler()->setCurrentItem(iname);
}

void GenericDebuggerEngine::setupEngine()
{
    const FilePath script = runParameters().overrideStartScript();
    if (!script.isEmpty() && !script.isReadableFile()) {
        AsynchronousMessageBox::warning(
            Tr::tr("Cannot Find Debugger Initialization Script"),
            Tr::tr("The debugger settings point to a script file at \"%1\", "
                   "which is not accessible. If a script file is not needed, "
                   "consider clearing that entry to avoid this warning.")
                .arg(script.toUserOutput()));
    }
    if (m_backend->hasExtraCapability(DebuggerExtraCapability::ScriptConsole)) {
        debuggerConsole()->setScriptEvaluator(this, [this](const QString &expression) {
            if (!answersConsoleInput()) {
                debuggerConsole()->printItem(
                    ConsoleItem::ErrorType, Tr::tr("The debugger cannot evaluate expressions now."));
                return;
            }
            executeDebuggerCommand(expression);
        });
        connect(stackHandler(), &StackHandler::stackChanged,
                this, &GenericDebuggerEngine::updateConsoleContext);
        connect(stackHandler(), &StackHandler::currentIndexChanged,
                this, &GenericDebuggerEngine::updateConsoleContext);
        if (WatchTreeView *view = inspectorView()) {
            connect(view, &WatchTreeView::currentIndexChanged,
                    this, &GenericDebuggerEngine::updateConsoleContext);
        }
    }
    if (m_backend->setupData().needsRunningCompanion) {
        for (DebuggerEngine *companion : companionEngines()) {
            connect(companion, &DebuggerEngine::postMessageRequested,
                    this, [this](const QString &output) { m_backend->companionOutput(output); });
        }
    }
    m_backend->start();
}

void GenericDebuggerEngine::setState(DebuggerState state, bool forced)
{
    DebuggerEngine::setState(state, forced);
    updateConsoleContext();
    if (m_inspectorToolsRegistered) {
        m_selectToolAction->setEnabled(state == InferiorRunOk);
        settings().showAppOnTop.action()->setEnabled(state == InferiorRunOk);
    }
}

// Input the runtime cannot answer would only wake the watchdog.
bool GenericDebuggerEngine::answersConsoleInput() const
{
    return (state() == InferiorRunOk || state() == InferiorStopOk) && !companionPreventsActions();
}

// Names what the console evaluates its input in: the selected frame while
// stopped, the object selected in the inspector while running.
void GenericDebuggerEngine::updateConsoleContext()
{
    if (!m_backend->hasExtraCapability(DebuggerExtraCapability::ScriptConsole))
        return;

    QString context;
    switch (state()) {
    case InferiorStopOk:
        context = stackHandler()->currentFrame().function;
        break;
    case InferiorRunOk:
        if (WatchTreeView *view = inspectorView(); view && m_consoleInSelectedObject) {
            const WatchItem *item = watchHandler()->watchItem(view->currentIndex());
            // A property names the object it belongs to, at any depth.
            while (item && item->parent() && item->parent()->id == item->id)
                item = item->parent();
            if (item && item->id != WatchItem::InvalidId)
                context = item->name;
        }
        break;
    default:
        debuggerConsole()->setContext(QString());
        return;
    }
    debuggerConsole()->setContext(Tr::tr("Context:") + ' '
                                  + (context.isEmpty() ? Tr::tr("Global QML Context") : context));
}

void GenericDebuggerEngine::askToRetryConnection(const QString &message)
{
    showMessage(message, LogError);
    auto box = new QMessageBox(Core::ICore::dialogParent());
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setIcon(QMessageBox::Critical);
    box->setObjectName("RetryConnectionBox");
    box->setWindowTitle(QGuiApplication::applicationDisplayName());
    box->setText(message + QLatin1Char('\n') + Tr::tr("Do you want to retry?"));
    box->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Help);
    box->setDefaultButton(QMessageBox::Retry);
    connect(box, &QDialog::finished, this, [this](int result) {
        // The user can end the session while the question is open.
        if (state() != EngineSetupRequested)
            return;
        if (result == QMessageBox::Retry) {
            m_backend->retryConnection();
            return;
        }
        if (result == QMessageBox::Help)
            Core::HelpManager::showHelpUrl(
                "qthelp://org.qt-project.qtcreator/doc/creator-debugging-qml.html");
        notifyEngineSetupFailed();
    });
    box->show();
}

void GenericDebuggerEngine::shutdownInferior()
{
    m_backend->shutdownInferior(runParameters().closeMode() == DetachAtClose
                                ? ShutdownMode::Detach : ShutdownMode::Kill);
}

void GenericDebuggerEngine::shutdownEngine()
{
    debuggerConsole()->setScriptEvaluator(this, ScriptEvaluator());
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
    query.functionName = bp.functionName;
    query.expression = bp.expression;
    query.startMode = runParameters().startMode();
    query.isNativeMixedEnabled = isNativeMixedEnabled();
    query.isQmlCompiledToNative = runParameters().isQmlCompiledToNative();
    query.isJavaDebugging = runParameters().isJavaDebugging();
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
    m_owedInterpreterUpdates.remove(bp->modelId());
    if (bp->responseId().isEmpty()) {
        // The insertion is still in flight, or the interpreter deferred it,
        // and what answers either takes the removal over from there.
        // One the interpreter gave up on has no answer left to wait for.
        if (m_refusedInterpreterBreakpoints.remove(bp->modelId())) {
            notifyBreakpointRemoveProceeding(bp);
            notifyBreakpointRemoveOk(bp);
        } else if (!bp->requestedParameters().isCppBreakpoint()) {
            // Deleting the global breakpoint takes the model id and the
            // parameters away right after this, so keep what the report needs.
            m_deferredRemovals.insert(bp->modelId(),
                                      {bp, parametersForDebugger(bp), bp->modelId()});
        }
        return;
    }
    sendBreakpointRemoval(bp, bp->responseId(), parametersForDebugger(bp), bp->modelId());
}

void GenericDebuggerEngine::sendBreakpointRemoval(const Breakpoint &bp, const QString &responseId,
                                                  const BreakpointParameters &params, int modelId)
{
    notifyBreakpointRemoveProceeding(bp);
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Remove;
    request.requestId = m_nextBreakpointRequestId++;
    request.responseId = responseId;
    // The backend tells an interpreter breakpoint from a C++ one by the
    // parameters, and without them sends the number the interpreter handed
    // out to lldb's own numbering.
    request.params = params;
    request.modelId = modelId;
    m_pendingBreakpoints[request.requestId] = bp;
    m_backend->changeBreakpoint(request);
}

// The interpreter reports a breakpoint it deferred once it takes it, or gives
// up on it. Removed in the meantime, the breakpoint has waited for the number.
void GenericDebuggerEngine::finishDeferredRemoval(const GdbMi &bkpt, const KeptBreakpoint &removal)
{
    if (bkpt["pending"].toInt())
        return;
    m_deferredRemovals.remove(removal.modelId);
    const QString nr = bkpt["number"].data();
    if (nr.toInt() > 0) {
        sendBreakpointRemoval(removal.bp, nr, removal.params, removal.modelId);
        return;
    }
    notifyBreakpointRemoveProceeding(removal.bp);
    notifyBreakpointRemoveOk(removal.bp);
}

void GenericDebuggerEngine::updateBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    if (bp->responseId().isEmpty() && !bp->requestedParameters().isCppBreakpoint()) {
        // The interpreter holds nothing of a refused one, so the change sets it
        // anew. A backend changes an interpreter breakpoint by removing it
        // first, and the cdb one stops there when the removal fails.
        if (m_refusedInterpreterBreakpoints.remove(bp->modelId())) {
            notifyBreakpointChangeProceeding(bp);
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = m_nextBreakpointRequestId++;
            request.params = parametersForDebugger(bp);
            request.modelId = bp->modelId();
            m_pendingBreakpoints[request.requestId] = bp;
            m_heldInterpreterUpdates.insert(bp->modelId(),
                                            {bp, request.params, request.modelId});
            m_reinsertions.insert(request.requestId);
            m_backend->changeBreakpoint(request);
            return;
        }
        // The answer to an insertion in flight sends the update.
        if (std::find(m_pendingBreakpoints.cbegin(), m_pendingBreakpoints.cend(), bp)
                != m_pendingBreakpoints.cend()) {
            m_heldInterpreterUpdates.insert(bp->modelId(),
                                            {bp, parametersForDebugger(bp), bp->modelId()});
            return;
        }
        // The interpreter deferred it, and the report that gives it a number
        // sends the update. Until then the breakpoint must not sit in an update
        // state, since a removal in that state never reaches the engine.
        notifyBreakpointChangeProceeding(bp);
        notifyBreakpointChangeOk(bp);
        m_owedInterpreterUpdates.insert(bp->modelId());
        return;
    }
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
    // The state the backend has, as far as the answers so far tell.
    bool confirmed = !enabled;
    for (const PendingSubToggle &pending : std::as_const(m_pendingSubBreakpoints)) {
        if (pending.sbp == sbp)
            confirmed = pending.confirmed;
    }
    m_pendingSubBreakpoints[request.requestId] = {sbp, enabled, confirmed};
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
    const PendingSubToggle toggle = m_pendingSubBreakpoints.take(requestId);
    if (!bp) {
        showMessage(QString("GenericDebuggerEngine: breakpoint event for unknown request %1")
                        .arg(requestId));
        return;
    }

    switch (op) {
    case BreakpointOp::Insert: {
        std::optional<KeptBreakpoint> held;
        for (const KeptBreakpoint &candidate : std::as_const(m_heldInterpreterUpdates)) {
            if (candidate.bp == bp)
                held = candidate;
        }
        if (held)
            m_heldInterpreterUpdates.remove(held->modelId);
        const bool reinsertion = m_reinsertions.remove(requestId);
        if (bp->state() == BreakpointRemoveRequested && reinsertion && !ok) {
            notifyBreakpointRemoveProceeding(bp);
            notifyBreakpointRemoveOk(bp);
            break;
        }
        if (bp->state() == BreakpointRemoveRequested && ok) {
            const QString nr = data.childCount() > 0 ? data.childAt(0)["number"].data()
                                                     : QString();
            std::optional<KeptBreakpoint> removal;
            for (const KeptBreakpoint &candidate : std::as_const(m_deferredRemovals)) {
                if (candidate.bp == bp)
                    removal = candidate;
            }
            // Deleted while the update was held, and the global breakpoint
            // may have taken the model id and the parameters away since.
            if (!removal && held) {
                m_deferredRemovals.insert(held->modelId, *held);
                removal = held;
            }
            if (removal) {
                // A deferral is finished by the interpreter's later report.
                if (data.childCount() > 0)
                    finishDeferredRemoval(data.childAt(0), *removal);
                break;
            }
            if (!nr.isEmpty()) {
                sendBreakpointRemoval(bp, nr, parametersForDebugger(bp), bp->modelId());
                break;
            }
        }
        if (reinsertion) {
            if (ok) {
                for (const GdbMi &bkpt : data)
                    applyBkptData(bkpt, bp);
            } else {
                m_refusedInterpreterBreakpoints.insert(bp->modelId());
            }
            // Changed again while the insertion was in flight, and
            // updateBreakpoint() held that change back for this answer.
            if (bp->state() == BreakpointUpdateRequested) {
                updateBreakpoint(bp);
                break;
            }
            // Refused again, it stays to be changed or removed like before.
            notifyBreakpointChangeOk(bp);
            break;
        }
        if (ok) {
            for (const GdbMi &bkpt : data)
                applyBkptData(bkpt, bp);
            if (bp->state() == BreakpointUpdateRequested
                    && !bp->requestedParameters().isCppBreakpoint()) {
                // Changed while the insertion was in flight, and updateBreakpoint()
                // held the update back for want of a number.
                updateBreakpoint(bp);
                break;
            }
            notifyBreakpointInsertOk(bp);
        } else {
            notifyBreakpointInsertFailed(bp);
        }
        break;
    }
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
    case BreakpointOp::EnableSub: {
        // Another toggle of the same place, pending or answered since, decides
        // its state.
        const SubBreakpoint sbp = toggle.sbp;
        const bool confirmed = ok ? toggle.enabled : toggle.confirmed;
        bool later = false;
        for (auto it = m_pendingSubBreakpoints.begin(); it != m_pendingSubBreakpoints.end(); ++it) {
            if (it->sbp != sbp)
                continue;
            it->confirmed = confirmed;
            if (it.key() > requestId)
                later = true;
        }
        if (!later && sbp && sbp->params.enabled != confirmed) {
            sbp->params.enabled = confirmed;
            bp->update();
            bp->adjustMarker();
        }
        break;
    }
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
    // A deferred breakpoint carries a placeholder number, which is no handle.
    const QString nr = bkpt["number"].data();
    const bool deferred = bkpt["pending"].toInt();
    bp->setResponseId(deferred || nr.toInt() <= 0 ? QString() : nr);
    if (!deferred && nr.toInt() <= 0)
        m_refusedInterpreterBreakpoints.insert(bp->modelId());
    else
        m_refusedInterpreterBreakpoints.remove(bp->modelId());
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

    QSet<QString> listed;
    for (const GdbMi &location : bkpt["locations"]) {
        const QString subnr = location["number"].data();
        listed.insert(subnr);
        SubBreakpoint sub = bp->findOrCreateSubBreakpoint(subnr);
        QTC_ASSERT(sub, return);
        applyToSub(location, sub);
    }
    // No list means a single location or none.
    bp->keepOnlySubBreakpoints(listed);

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
        } else if (isInterpreterBkptData(bkpt)) {
            // The interpreter counts its breakpoints itself, so its number can
            // be one a C++ breakpoint already carries. Only the model id may
            // look one up, and a report that matches none is dropped.
            const auto removal = m_deferredRemovals.constFind(bkpt["modelid"].toInt());
            if (removal != m_deferredRemovals.cend()) {
                finishDeferredRemoval(bkpt, KeptBreakpoint(*removal));
                bp = {};
                continue;
            }
            bp = handler->findBreakpointByModelId(bkpt["modelid"].toInt());
            if (bp) {
                applyInterpreterBkptData(bkpt, bp);
                if (!bkpt["pending"].toInt() && m_owedInterpreterUpdates.remove(bp->modelId()))
                    handler->requestBreakpointUpdate(bp);
            }
        } else {
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

// Several threads can stop at a breakpoint at once, and the stop is shown in
// one of them. The threads of the stop and the breakpoint it is shown at are
// reported in either order, and the threads only count once they are the
// ones of this stop. Later reports of the threads during the same stop leave
// the status alone.
void GenericDebuggerEngine::showBreakpointStopMessage()
{
    int others = 0;
    if (!m_stopThreadsPending) {
        threadsHandler()->forItemsAtLevel<1>([this, &others](ThreadItem *thread) {
            if (thread->id() != m_breakpointStopThreadId && thread->isAtBreakpoint())
                ++others;
        });
    }
    if (others == 0) {
        showStatusMessage(m_breakpointStopMessage);
        return;
    }
    showStatusMessage(m_breakpointStopMessage + ' '
                      + Tr::tr("%n other threads stopped at a breakpoint as well.", nullptr,
                               others));
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
        // A step at a time: a runaway recursion has more frames than one
        // answer should carry.
        reloadStack(qMax(m_shownStackDepthLimit, handler->stackSize()) * 10 + 3);
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
    if (state() != InferiorRunOk && state() != InferiorStopOk && state() != InferiorUnrunnable)
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

FilePath GenericDebuggerEngine::modulePathOnDebuggerDevice(const QString &reported) const
{
    const FilePath path = FilePath::fromUserInput(reported);
    if (path.isEmpty() || !path.isLocal())
        return path;
    return runParameters().debugger().command.executable().withNewPath(path.path());
}

quint64 GenericDebuggerEngine::reloadStack(int depthLimit)
{
    RefreshRequest request;
    request.kind = RefreshKind::FullStack;
    request.requestId = m_nextRefreshRequestId++;
    request.stackDepthLimit = depthLimit;
    m_stackDepthLimits.insert(request.requestId, depthLimit);
    m_backend->refresh(request);
    return request.requestId;
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
    // Creating a QML stack while the QmlEngine is stopped results in a frozen inferior.
    for (DebuggerEngine *companion : companionEngines()) {
        if (companion && companion->objectName() == "QmlEngine"
                && companion->state() == InferiorStopOk) {
            showMessage(Tr::tr("Cannot create a QML stack trace while the QML debugger "
                               "is stopped."),
                        StatusBar);
            return;
        }
    }
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
    for (const quint64 address : addresses)
        request.sizes.append(peripheralRegisterHandler()->registerSize(address));
    m_backend->refresh(request);
}

void GenericDebuggerEngine::reloadSourceFiles()
{
    if (state() != InferiorRunOk && state() != InferiorStopOk && state() != InferiorUnrunnable)
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

bool GenericDebuggerEngine::showsStoppedState() const
{
    return (state() == InferiorStopOk && !m_attachStopUnread) || state() == InferiorUnrunnable;
}

// A backend answers its requests in order, so what is asked for after the
// load reads what the load brought.
void GenericDebuggerEngine::loadAllSymbols()
{
    RefreshRequest request;
    request.kind = RefreshKind::AllSymbols;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
    reloadModules();
    if (showsStoppedState()) {
        if (state() == InferiorUnrunnable || m_shownStackDepthLimit < 0)
            reloadFullStack();
        else
            reloadStack(qMax(m_shownStackDepthLimit, settings().maximalStackDepth()));
    }
}

void GenericDebuggerEngine::reloadDebuggingHelpers()
{
    RefreshRequest request;
    request.kind = RefreshKind::DebuggingHelpers;
    request.requestId = m_nextRefreshRequestId++;
    m_backend->refresh(request);
    if (showsStoppedState())
        updateLocals();
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

bool GenericDebuggerEngine::canReadMemory() const
{
    return hasCapability(ShowMemoryCapability);
}

void GenericDebuggerEngine::readMemory(quint64 addr, quint64 length,
                                       const MemoryReadCallback &callback)
{
    const quint64 requestId = m_nextMemoryRequestId++;
    m_pendingMemoryReads.insert(requestId, callback);
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
    m_backend->fetchDisassembly(requestId, agent->address(), agent->location().functionName(),
                                agent->location().from(),
                                settings().intelFlavor() ? DisassemblyFlavor::Intel
                                                         : DisassemblyFlavor::Att);
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

void GenericDebuggerEngine::executeRunToCallTarget(const RunToTarget &target)
{
    ExecutionRequest request;
    request.command = ExecutionCommand::RunToFunction;
    request.functionName = target.function;
    request.exactName = target.exact;
    m_backend->execute(request);
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

void GenericDebuggerEngine::quitDebugger()
{
    const bool settingUp = state() == EngineSetupRequested;
    DebuggerEngine::quitDebugger();
    // That gave up on the setup, while the backend can still be bringing up
    // what it needs, and a debuggee with it.
    if (settingUp)
        m_backend->execute({ExecutionCommand::Abort});
}

bool GenericDebuggerEngine::companionPreventsActions() const
{
    if (!m_backend->setupData().needsRunningCompanion)
        return DebuggerEngine::companionPreventsActions();
    const DebuggerEngine *companion = companionEngines().value(0);
    return companion && companion->state() != InferiorRunOk;
}

void GenericDebuggerEngine::executeDebuggerCommand(const QString &command)
{
    WatchItemData inspectorItem;
    if (WatchTreeView *view = inspectorView()) {
        const WatchItem *item = watchHandler()->watchItem(view->currentIndex());
        if (item && item->isInspect())
            inspectorItem = watchItemData(item);
    }
    m_commandMovedSelection = false;
    m_selectedFrameLevel.clear();
    m_backend->executeDebuggerCommand(command, inspectorItem);
}

// The command can change anything the views show, but not the frame the user
// selected. One that moved the selection of the debugger itself leaves the
// debugger on the frame it chose, and the views follow that one.
void GenericDebuggerEngine::handleDebuggerCommandDone()
{
    if (state() != InferiorStopOk)
        return;
    const int depth = m_shownStackDepthLimit < 0
            ? -1 : qMax(m_shownStackDepthLimit, settings().maximalStackDepth());
    const int row = stackHandler()->currentIndex();
    const QString level = m_commandMovedSelection
            ? m_selectedFrameLevel
            : row >= 0 ? stackHandler()->frameAt(row).level : QString();
    m_frameKeepingStackRequests.insert(reloadStack(depth), level);
    reloadThreads();
}

void GenericDebuggerEngine::expandItem(const QString &iname)
{
    if (!iname.startsWith("inspect.")) {
        DebuggerEngine::expandItem(iname);
        return;
    }
    refreshInspectorTree();
}

void GenericDebuggerEngine::insertInspectorItems(const GdbMi &items)
{
    WatchHandler *handler = watchHandler();
    handler->insertItems(items);
    if (settings().sortStructMembers()) {
        for (const GdbMi &item : items) {
            const QString iname = item["iname"].data();
            if (!iname.endsWith(".[properties]"))
                continue;
            if (WatchItem *properties = handler->findItem(iname)) {
                properties->sortChildren([](const WatchItem *a, const WatchItem *b) {
                    return a->name < b->name;
                });
            }
        }
    }
    handler->updateLocalsWindow();
    handler->reexpandItems();
}

void GenericDebuggerEngine::clearInspectorTree()
{
    WatchHandler *handler = watchHandler();
    if (WatchItem *root = handler->findItem("inspect")) {
        while (root->childCount() > 0)
            handler->removeItemByIName(root->childAt(0)->iname);
    }
    handler->updateLocalsWindow();
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
                             .sortStructMembers = s.sortStructMembers(),
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
