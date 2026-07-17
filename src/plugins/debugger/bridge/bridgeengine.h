// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QString>

namespace Debugger::Internal {

// A debugger engine that speaks a DAP-shaped protocol to Qt Creator's own
// Python bridge (gdbbridge.py) rather than to a foreign DAP adapter. It
// deliberately duplicates the relevant parts of the dap/ DapEngine instead of
// inheriting from it, so the experimental path can diverge freely without
// perturbing the DAP engine. The transport (dap/DapClient) is still reused.
//
// The control plane is DAP-shaped; variables use the native, dumper-aware
// qtc/fetchVariables request (the real Qt dumpers, iname identity, reusing
// updateLocalsView) rather than the DAP variablesReference model.

class DapClient;
class DebuggerCommand;
class DisassemblerAgent;
class GdbMi;
class MemoryAgent;
enum class DapResponseType;
enum class DapEventType;

class BridgeEngine : public DebuggerEngine
{
public:
    BridgeEngine();

private:
    void setupEngine() override;

    void executeStepIn(bool) override;
    void executeStepOut() override;
    void executeStepOver(bool) override;

    void shutdownInferior() override;
    void shutdownEngine() override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void activateFrame(int index) override;
    void selectThread(const Thread &thread) override;

    bool acceptsBreakpoint(const BreakpointParameters &bp) const override;
    void insertBreakpoint(const Breakpoint &bp) override;
    void updateBreakpoint(const Breakpoint &bp) override;
    void removeBreakpoint(const Breakpoint &bp) override;

    void executeDebuggerCommand(const QString &command) override;

    void loadSymbols(const Utils::FilePath &moduleName) override;
    void loadAllSymbols() override;
    void reloadModules() override;
    void reloadRegisters() override;
    void reloadSourceFiles() override {}
    void reloadFullStack() override;

    void doUpdateLocals(const UpdateParameters &params) override;
    void updateAll() override;

    void fetchMemory(MemoryAgent *agent, quint64 addr, quint64 length) override;
    void changeMemory(MemoryAgent *agent, quint64 addr, const QByteArray &data) override;
    void fetchDisassembler(DisassemblerAgent *agent) override;

    bool hasCapability(unsigned cap) const override;

    void runCommand(const DebuggerCommand &cmd);

    void refreshLocation(const GdbMi &reportedLocation);
    void refreshStack(const QJsonArray &stackFrames);
    void refreshModules(const GdbMi &modules);
    void refreshState(const GdbMi &reportedState);
    void refreshSymbols(const GdbMi &symbols);

    QString errorMessage(QProcess::ProcessError error) const;

    void claimInitialBreakpoints();

    void handleDapStarted();
    void handleDapInitialize();
    void handleDapEventInitialized();
    void handleDapConfigurationDone();

    void handleBkpt(const GdbMi &bkpt, const Breakpoint &bp);

    void handleDapDone();
    void readDapStandardError();

    void handleResponse(DapResponseType type, const QJsonObject &response);
    void handleStackTraceResponse(const QJsonObject &response);
    void handleThreadsResponse(const QJsonObject &response);
    void handleInsertBreakpointResponse(const QJsonObject &response);
    void handleUpdateBreakpointResponse(const QJsonObject &response);
    void handleRemoveBreakpointResponse(const QJsonObject &response);
    void handleFetchVariablesResponse(const QJsonObject &response);
    void handleFetchRegistersResponse(const QJsonObject &response);
    void handleReadMemoryResponse(const QJsonObject &response);
    void handleWriteMemoryResponse(const QJsonObject &response);
    void handleDisassembleResponse(const QJsonObject &response);

    void handleEvent(DapEventType type, const QJsonObject &event);
    void handleStoppedEvent(const QJsonObject &event);

    void connectDataGeneratorSignals();

    const QLoggingCategory &logCategory();

    DapClient *m_dapClient = nullptr;

    int m_currentThreadId = -1;
    int m_currentStackFrameId = -1;

    // Correlates async qtc/readMemory responses back to the requesting agent.
    QHash<int, QPointer<MemoryAgent>> m_memoryAgents;
    int m_nextMemoryToken = 0;

    // Correlates async qtc/disassemble responses back to the requesting agent.
    QHash<int, QPointer<DisassemblerAgent>> m_disassemblerAgents;
    int m_nextDisassemblerToken = 0;
};

DebuggerEngine *createBridgeEngine();

} // namespace Debugger::Internal
