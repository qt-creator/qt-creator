// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QSet>
#include <QString>

#include <list>
#include <memory>

namespace Debugger::Internal {

// A debugger engine that speaks a DAP-shaped protocol to Qt Creator's own
// Python bridge (gdbbridge.py) rather than to a foreign DAP adapter. It
// deliberately duplicates the relevant parts of the dap/ DapEngine instead of
// inheriting from it, so the experimental path (native, dumper-aware
// breakpoints and variables) can diverge freely without perturbing the DAP
// engine. The transport (dap/DapClient) is still reused.

class BridgeEngine;
class DapClient;
class DebuggerCommand;
class GdbMi;
enum class DapResponseType;
enum class DapEventType;

// Serializes the on-demand fetching of variable subtrees. Copied from the
// DAP engine's VariablesHandler; will be replaced once the native iname-based
// variable payload lands.
class BridgeVariablesHandler
{
public:
    BridgeVariablesHandler(BridgeEngine *engine);

    struct VariableItem
    {
        QString iname;
        int variablesReference;
    };

    void addVariable(const QString &iname, int variablesReference);
    void handleNext();

    VariableItem currentItem() const { return m_currentVarItem; }
    int queueSize() const { return int(m_queue.size()); }

private:
    void startHandling();

    BridgeEngine *m_engine;
    std::list<VariableItem> m_queue;
    VariableItem m_currentVarItem;
};

class BridgeEngine : public DebuggerEngine
{
public:
    BridgeEngine();

    DapClient *dapClient() const { return m_dapClient; }
    int currentStackFrameId() const { return m_currentStackFrameId; }

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
    void reloadSourceFiles() override {}
    void reloadFullStack() override;

    void updateItem(const QString &iname) override;
    void reexpandItems(const QSet<QString> &inames) override;
    void doUpdateLocals(const UpdateParameters &params) override;

    void updateAll() override;
    void updateLocals() override;

    bool hasCapability(unsigned cap) const override;

    void runCommand(const DebuggerCommand &cmd);

    void refreshLocation(const GdbMi &reportedLocation);
    void refreshStack(const QJsonArray &stackFrames);
    void refreshLocals(const QJsonArray &variables);
    void refreshModules(const GdbMi &modules);
    void refreshState(const GdbMi &reportedState);
    void refreshSymbols(const GdbMi &symbols);

    QString errorMessage(QProcess::ProcessError error) const;

    void claimInitialBreakpoints();

    void handleDapStarted();
    void handleDapInitialize();
    void handleDapEventInitialized();
    void handleDapConfigurationDone();

    void dapRemoveBreakpoint(const Breakpoint &bp);
    void dapInsertBreakpoint(const Breakpoint &bp);
    void dapRemoveFunctionBreakpoint(const Breakpoint &bp);
    void dapInsertFunctionBreakpoint(const Breakpoint &bp);

    void handleDapDone();
    void readDapStandardError();

    void handleResponse(DapResponseType type, const QJsonObject &response);
    void handleStackTraceResponse(const QJsonObject &response);
    void handleScopesResponse(const QJsonObject &response);
    void handleThreadsResponse(const QJsonObject &response);
    void handleEvaluateResponse(const QJsonObject &response);
    void handleBreakpointResponse(const QJsonObject &response);

    void handleEvent(DapEventType type, const QJsonObject &event);
    void handleStoppedEvent(const QJsonObject &event);

    void connectDataGeneratorSignals();

    const QLoggingCategory &logCategory();

    DapClient *m_dapClient = nullptr;

    int m_currentThreadId = -1;
    int m_currentStackFrameId = -1;

    std::unique_ptr<BridgeVariablesHandler> m_variablesHandler;

    friend class BridgeVariablesHandler;
};

DebuggerEngine *createBridgeEngine();

} // namespace Debugger::Internal
