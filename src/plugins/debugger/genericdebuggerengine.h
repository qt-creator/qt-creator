// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerengine.h"
#include "debuggerengineinterface.h"

#include <functional>
#include <memory>

#include <QHash>
#include <QMultiMap>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QSet>

namespace Debugger::Internal {

class ConsoleItem;

class GenericDebuggerEngine final : public DebuggerEngine
{
public:
    explicit GenericDebuggerEngine(const QString &debuggerTypeName, DebuggerEngineInterface *backend);
    ~GenericDebuggerEngine() override;

private:
    void setupEngine() final;
    void setState(DebuggerState state, bool forced = false) final;
    void updateConsoleContext();
    bool answersConsoleInput() const;
    void askToRetryConnection(const QString &message);
    void shutdownInferior() final;
    void shutdownEngine() final;

    bool hasCapability(unsigned cap) const final;
    bool acceptsBreakpoint(const BreakpointParameters &bp) const final;
    void insertBreakpoint(const Breakpoint &bp) final;
    void removeBreakpoint(const Breakpoint &bp) final;
    void updateBreakpoint(const Breakpoint &bp) final;
    void enableSubBreakpoint(const SubBreakpoint &sbp, bool enabled) final;
    void selectThread(const Thread &thread) final;
    void activateFrame(int index) final;
    void reloadModules() final;
    void requestModuleSymbols(const Utils::FilePath &moduleName) final;
    void requestModuleSections(const Utils::FilePath &moduleName) final;
    void reloadFullStack() final;
    void loadAdditionalQmlStack() final;
    void loadSymbolsForStack() final;
    void loadSymbols(const Utils::FilePath &moduleName) final;
    void reloadRegisters() final;
    void reloadPeripheralRegisters() final;
    void reloadSourceFiles() final;
    void loadAllSymbols() final;
    void reloadDebuggingHelpers() final;
    void setRegisterValue(const QString &name, const QString &value) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void assignValueInDebugger(WatchItem *item, const QString &expression,
                               const QVariant &value) final;
    std::optional<bool> registersFollowSelectedFrame() const final
    {
        return m_backend->registersFollowSelectedFrame();
    }
    bool canReadMemory() const final;
    void readMemory(quint64 addr, quint64 length, const MemoryReadCallback &callback) final;
    void changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data) final;
    void fetchDisassembler(DisassemblerAgent *agent) final;
    void watchPoint(const QPoint &pnt) final;
    void createSnapshot() final;

    void continueInferior() final;
    void interruptInferior() final;
    void executeStepOver(bool byInstruction) final;
    void executeStepIn(bool byInstruction) final;
    void executeStepOut() final;
    void executeReturn() final;
    void executeRunToLine(const ContextData &data) final;
    void executeRunToFunction(const QString &functionName) final;
    void executeJumpToLine(const ContextData &data) final;
    void executeRecordReverse(bool record) final;
    void debugLastCommand() final;
    void detachDebugger() final;
    void resetInferior() final;
    void abortDebuggerProcess() final;
    void quitDebugger() final;
    bool companionPreventsActions() const final;
    void executeDebuggerCommand(const QString &command) final;
    void handleDebuggerCommandDone();
    void doUpdateLocals(const UpdateParameters &params) final;
    void updateAll() final;
    bool showsStoppedState() const;
    void expandItem(const QString &iname) final;
    void refreshInspectorTree();
    void insertInspectorItems(const GdbMi &items);
    void clearInspectorTree();
    void selectWatchData(const QString &iname) final;
    void setInspectorToolsAvailable(bool available);
    void handleInspectorObjectSelected(const QString &iname, const Utils::FilePath &file,
                                       int line);
    ConsoleItem *toConsoleItem(const GdbMi &data);
    std::function<void(ConsoleItem *)> consoleFetch(qint64 handle);
    void appendConsoleChildren(ConsoleItem *item, const GdbMi &children);
    void handleConsoleItemFetched(quint64 requestId, const GdbMi &data);

    void handleBreakpointEvent(quint64 requestId, BreakpointOp op, bool ok, const GdbMi &data);
    void applyBkptData(const GdbMi &bkpt, const Breakpoint &bp);
    void applyInterpreterBkptData(const GdbMi &bkpt, const Breakpoint &bp);
    BreakpointParameters parametersForDebugger(const Breakpoint &bp) const;
    struct KeptBreakpoint
    {
        Breakpoint bp;
        BreakpointParameters params;
        int modelId = 0;
    };
    void sendBreakpointRemoval(const Breakpoint &bp, const QString &responseId,
                               const BreakpointParameters &params, int modelId);
    void finishDeferredRemoval(const GdbMi &bkpt, const KeptBreakpoint &removal);
    void handleBreakpointModified(const GdbMi &data);
    void handleSignalReceived(const QString &name, const QString &meaning);
    void handleStopReasonReported(const QString &reason);
    void showBreakpointStopMessage();
    void askAboutQtSymbols(const Utils::FilePath &module);
    void reportMissingQtSymbols(const QString &module);
    void handleNotResponding(std::chrono::seconds waited, const QStringList &pendingCommands,
                             NotRespondingCause cause);
    quint64 reloadStack(int depthLimit);
    void reloadThreads();
    Utils::FilePath cleanupFullName(const QString &fileName);
    Utils::FilePath modulePathOnDebuggerDevice(const QString &reported) const;

    const std::unique_ptr<DebuggerEngineInterface> m_backend;
    QHash<QString, QString> m_qtVersionOfModule;
    QMultiMap<QString, Utils::FilePath> m_baseNameToFullName;
    quint64 m_nextBreakpointRequestId = 1;
    quint64 m_nextRefreshRequestId = 1;
    // The console entries being expanded. The index tells whether one is still there.
    struct PendingConsoleFetch
    {
        QPersistentModelIndex index;
        ConsoleItem *item = nullptr;
    };
    QHash<quint64, PendingConsoleFetch> m_pendingConsoleFetches;
    // The frame limit of each stack fetch not answered yet, -1 for none.
    QHash<quint64, int> m_stackDepthLimits;
    // The stack fetches whose answer leaves the selected frame as it is, with its level.
    QHash<quint64, QString> m_frameKeepingStackRequests;
    bool m_commandMovedSelection = false;
    QString m_selectedFrameLevel;
    // The frame limit of the stack shown. Rows are fewer with collapsed machinery.
    int m_shownStackDepthLimit = 0;
    quint64 m_nextMemoryRequestId = 1;
    quint64 m_nextDisassemblyRequestId = 1;
    quint64 m_nextSnapshotRequestId = 1;
    quint64 m_nextFullBacktraceRequestId = 1;
    quint64 m_nextWatchPointRequestId = 1;
    QHash<quint64, Breakpoint> m_pendingBreakpoints;
    // The model ids of interpreter breakpoints it gave up on, which no later
    // report is going to give a number.
    QSet<int> m_refusedInterpreterBreakpoints;
    // The model ids of deferred interpreter breakpoints changed before it
    // numbered them, which the report that gives the number updates.
    QSet<int> m_owedInterpreterUpdates;
    // Interpreter breakpoints removed before it numbered them, by model id.
    QHash<int, KeptBreakpoint> m_deferredRemovals;
    // Interpreter breakpoints changed while an insertion of theirs is in
    // flight, by model id. A removal in that state never reaches the engine,
    // so the answer to the insertion takes it over from what is kept here.
    QHash<int, KeptBreakpoint> m_heldInterpreterUpdates;
    // The insertions that carry the change of a refused interpreter breakpoint.
    QSet<quint64> m_reinsertions;
    struct PendingSubToggle
    {
        SubBreakpoint sbp;
        bool enabled = false;
        bool confirmed = false;
    };
    QHash<quint64, PendingSubToggle> m_pendingSubBreakpoints;
    QHash<quint64, MemoryReadCallback> m_pendingMemoryReads;
    QHash<quint64, QPointer<DisassemblerAgent>> m_pendingDisassemblyRequests;
    bool m_notRespondingPending = false;
    bool m_attachStopUnread = false;
    QString m_breakpointStopMessage;
    QString m_breakpointStopThreadId;
    bool m_stopThreadsPending = true;
    QAction *m_selectToolAction = nullptr;
    bool m_inspectorToolsRegistered = false;
    bool m_consoleInSelectedObject = false;

    friend class DebuggerUnitTests;
};
} // namespace Debugger::Internal
