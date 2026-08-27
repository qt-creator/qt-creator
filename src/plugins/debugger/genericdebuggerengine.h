// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debuggerengine.h"
#include "debuggerengineinterface.h"

#include <memory>

#include <QHash>
#include <QMultiMap>
#include <QPointer>

namespace Debugger::Internal {

class GenericDebuggerEngine final : public DebuggerEngine
{
public:
    explicit GenericDebuggerEngine(const QString &debuggerTypeName, DebuggerEngineInterface *backend);
    ~GenericDebuggerEngine() override;

private:
    void setupEngine() final;
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
    void fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length) final;
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
    void executeDebuggerCommand(const QString &command) final;
    void doUpdateLocals(const UpdateParameters &params) final;
    void updateAll() final;
    void expandItem(const QString &iname) final;
    void refreshInspectorTree();

    void handleBreakpointEvent(quint64 requestId, BreakpointOp op, bool ok, const GdbMi &data);
    void applyBkptData(const GdbMi &bkpt, const Breakpoint &bp);
    void handleBreakpointModified(const GdbMi &data);
    void handleSignalReceived(const QString &name, const QString &meaning);
    void handleNotResponding(std::chrono::seconds waited, const QStringList &pendingCommands);
    void reloadStack(int depthLimit);
    void reloadThreads();
    Utils::FilePath cleanupFullName(const QString &fileName);

    const std::unique_ptr<DebuggerEngineInterface> m_backend;
    QMultiMap<QString, Utils::FilePath> m_baseNameToFullName;
    quint64 m_nextBreakpointRequestId = 1;
    quint64 m_nextRefreshRequestId = 1;
    quint64 m_nextMemoryRequestId = 1;
    quint64 m_nextDisassemblyRequestId = 1;
    quint64 m_nextSnapshotRequestId = 1;
    quint64 m_nextFullBacktraceRequestId = 1;
    quint64 m_nextWatchPointRequestId = 1;
    QHash<quint64, Breakpoint> m_pendingBreakpoints;
    QHash<quint64, QPointer<MemoryAgent>> m_pendingMemoryRequests;
    QHash<quint64, QPointer<DisassemblerAgent>> m_pendingDisassemblyRequests;
    bool m_notRespondingPending = false;
};
} // namespace Debugger::Internal
