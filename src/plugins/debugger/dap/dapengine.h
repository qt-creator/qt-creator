// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <utils/process.h>

#include <QLoggingCategory>
#include <QVariant>

#include <queue>

namespace Debugger::Internal {

class DapClient;
class DebuggerCommand;
class IDataProvider;
class GdbMi;
enum class DapResponseType;
enum class DapEventType;
class DapEngine;

class VariablesHandler {
public:
    VariablesHandler(DapEngine *dapEngine);

    struct VariableItem {
        QString iname;
        int variablesReference;
    };

    void addVariable(const QString &iname, int variablesReference);
    void handleNext();

    VariableItem currentItem() const { return m_currentVarItem; }
    int queueSize() const { return int(m_queue.size()); }

private:
    void startHandling();

    DapEngine *m_dapEngine;
    std::list<VariableItem> m_queue;
    VariableItem m_currentVarItem;
};

/*
 * A debugger engine for the debugger adapter protocol.
 */
class DapEngine : public DebuggerEngine
{
public:
    DapEngine();
    ~DapEngine() override = default;

    DapClient *dapClient() const { return m_dapClient; }
    int currentStackFrameId() const { return m_currentStackFrameId; }

protected:
    void executeStepIn(bool) override;
    void executeStepOut() override;
    void executeStepOver(bool) override;

    void shutdownInferior() override;
    void shutdownEngine() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

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
    void reloadRegisters() override {}
    void reloadSourceFiles() override {}
    void reloadFullStack() override;

    bool supportsThreads() const { return true; }
    void updateItem(const QString &iname) override;
    void reexpandItems(const QSet<QString> &inames) override;
    void doUpdateLocals(const UpdateParameters &params) override;
    void getVariableFromQueue();

    void runCommand(const DebuggerCommand &cmd) override;

    void refreshLocation(const GdbMi &reportedLocation);
    void refreshStack(const QJsonArray &stackFrames);
    void refreshLocals(const QJsonArray &variables);
    void refreshModules(const GdbMi &modules);
    void refreshState(const GdbMi &reportedState);
    void refreshSymbols(const GdbMi &symbols);

    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const override;

    void claimInitialBreakpoints();

    void handleDapStarted();
    virtual void handleDapInitialize();
    void handleDapEventInitialized();
    virtual void handleDapConfigurationDone();

    void dapRemoveBreakpoint(const Breakpoint &bp);
    void dapInsertBreakpoint(const Breakpoint &bp);

    void handleDapDone();
    void readDapStandardOutput();
    void readDapStandardError();

    void handleResponse(DapResponseType type, const QJsonObject &response);
    void handleStackTraceResponse(const QJsonObject &response);
    void handleScopesResponse(const QJsonObject &response);
    void handleThreadsResponse(const QJsonObject &response);
    void handleEvaluateResponse(const QJsonObject &response);

    void handleEvent(DapEventType type, const QJsonObject &event);
    void handleBreakpointEvent(const QJsonObject &event);
    void handleStoppedEvent(const QJsonObject &event);

    void updateAll() override;
    void updateLocals() override;
    void connectDataGeneratorSignals();

    QByteArray m_inbuffer;
    DapClient *m_dapClient = nullptr;

    int m_nextBreakpointId = 1;
    int m_currentThreadId = -1;
    int m_currentStackFrameId = -1;

    std::unique_ptr<VariablesHandler> m_variablesHandler;

    virtual const QLoggingCategory &logCategory()
    {
        static const QLoggingCategory logCategory = QLoggingCategory("qtc.dbg.dapengine",
                                                                     QtWarningMsg);
        return logCategory;
    }
};

} // Debugger::Internal
