// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerengine.h>

#include <utils/process.h>

#include <QVariant>

#include <queue>

namespace Debugger::Internal {

class DebuggerCommand;
class GdbMi;

/*
 * A debugger engine for the debugger adapter protocol.
 */

class IDataProvider : public QObject
{
    Q_OBJECT
public:
    virtual void start() = 0;
    virtual bool isRunning() const = 0;
    virtual void writeRaw(const QByteArray &input) = 0;
    virtual void kill() = 0;
    virtual QByteArray readAllStandardOutput() = 0;
    virtual QString readAllStandardError() = 0;
    virtual int exitCode() const = 0;
    virtual QString executable() const = 0;

    virtual QProcess::ExitStatus exitStatus() const = 0;
    virtual QProcess::ProcessError error() const = 0;
    virtual Utils::ProcessResult result() const = 0;
    virtual QString exitMessage() const = 0;

signals:
    void started();
    void done();
    void readyReadStandardOutput();
    void readyReadStandardError();
};

class DapEngine : public DebuggerEngine
{
public:
    DapEngine();

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

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command) override;

    void loadSymbols(const Utils::FilePath &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const Utils::FilePath &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override {}
    void reloadSourceFiles() override {}
    void reloadFullStack() override {}

    bool supportsThreads() const { return true; }
    void updateItem(const QString &iname) override;

    void runCommand(const DebuggerCommand &cmd) override;
    void postDirectCommand(const QJsonObject &ob);

    void refreshLocation(const GdbMi &reportedLocation);
    void refreshStack(const QJsonArray &stackFrames);
    void refreshLocals(const QJsonArray &variables);
    void refreshModules(const GdbMi &modules);
    void refreshState(const GdbMi &reportedState);
    void refreshSymbols(const GdbMi &symbols);

    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const override;

    void claimInitialBreakpoints();

    virtual void handleDapStarted();
    void handleDapLaunch();
    void handleDapConfigurationDone();

    void dapStackTrace();
    void dapScopes(int frameId);
    void threads();
    void dapVariables(int variablesReference);

    void handleDapDone();
    void readDapStandardOutput();
    void readDapStandardError();

    void handleOutput(const QJsonDocument &data);

    void handleResponse(const QJsonObject &response);
    void handleStackTraceResponse(const QJsonObject &response);
    void handleScopesResponse(const QJsonObject &response);
    void handleThreadsResponse(const QJsonObject &response);

    void handleEvent(const QJsonObject &event);
    void handleBreakpointEvent(const QJsonObject &event);
    void handleStoppedEvent(const QJsonObject &event);

    void updateAll() override;
    void updateLocals() override;
    void connectDataGeneratorSignals();

    QByteArray m_inbuffer;
    std::unique_ptr<IDataProvider> m_dataGenerator = nullptr;

    int m_nextBreakpointId = 1;
    int m_currentThreadId = -1;

    std::queue<std::pair<int, WatchItem *>> m_variablesReferenceQueue;
    WatchItem *m_currentWatchItem = nullptr;
    WatchItem *m_rootWatchItem = nullptr;
};

} // Debugger::Internal
