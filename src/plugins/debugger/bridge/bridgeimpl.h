// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/processinterface.h>

#include <QHash>
#include <QJsonObject>

#include <memory>

namespace Debugger::Internal {

class DapClient;
enum class DapEventType;
enum class DapResponseType;

class DEBUGGER_EXPORT BridgeImplHostRecipe
{
public:
    QStringList startupArguments;
    QString bridgeModule;
    QString serverCall;
};

DEBUGGER_EXPORT BridgeImplHostRecipe gdbHostRecipe(bool loadInitFile);

class DEBUGGER_EXPORT BridgeImplStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath dumperScriptsDir;
    BridgeImplHostRecipe hostRecipe;
    Utils::FilePaths extraDumperFiles;
    QStringList extraDumperCommands;
    Utils::FilePath sysroot;
    QList<QPair<QString, QString>> sourcePathMap;
    Utils::FilePaths sourceDirectories;
    bool nativeMixedDebugging = false;
    // Dumper context the interface's RefreshRequest does not carry.
    int qtVersion = 0;
    QString qtNamespace;
};

class DEBUGGER_EXPORT BridgeImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit BridgeImpl(const BridgeImplStartData &startData);
    ~BridgeImpl() override;

private:
    void start() final;
    void shutdownInferior(ShutdownMode mode) final;
    void shutdownEngine() final;

    void execute(const ExecutionRequest &request) final;
    void changeBreakpoint(const BreakpointChangeRequest &request) final;
    void refresh(const RefreshRequest &request) final;

    void selectThread(const QString &threadId) final;
    void activateFrame(int index) final;
    void setRegisterValue(const QString &name, const QString &value) final;
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) final;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;

    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                              const QString &value) final;

    void executeDebuggerCommand(const QString &command,
                               const WatchItemData &inspectorItem) final;

    void handleStarted();
    void handleFinished();
    void handleStandardError();
    void configureTarget();
    void handleResponse(DapResponseType type, const QJsonObject &response);
    void handleEvent(DapEventType type, const QJsonObject &event);
    void handleStopped(const QJsonObject &event);
    void handleStackTrace(const QJsonObject &response);
    void reportStop();
    void handleBreakpointResponse(BreakpointOp op, const QJsonObject &response);

    int postRequest(const QString &command, const QJsonObject &arguments = {});
    QJsonObject stepArguments(bool byInstruction) const;
    void fetchDisassemblyForTarget(quint64 requestId, quint64 address, const QString &target);
    void postLaunchOrAttach();
    void postBreakpointRequest(const QString &request, const BreakpointChangeRequest &change);

    const BridgeImplStartData m_startData;
    DapClient *m_client = nullptr;

    class DisassemblyRequest
    {
    public:
        quint64 requestId = 0;
        quint64 address = 0;
        QString target;
    };
    QHash<quint64, DisassemblyRequest> m_disassemblyRequests;
    quint64 m_nextDisassemblyToken = 0;

    // The locals fetch, kept for RepeatLastCommand.
    QString m_lastDebuggableCommand;
    QJsonObject m_lastDebuggableArguments;

    int m_currentThreadId = -1;
    int m_currentFrameId = -1;
    bool m_stopRequested = false;
    bool m_inferiorRunning = false;

    // The stop event carries no frame, so the location has to be asked for.
    // The answer is routed by the request's sequence number.
    class StackTraceRequest
    {
    public:
        bool reportsStop = false;
        quint64 refreshRequestId = 0;
    };
    QHash<int, StackTraceRequest> m_stackTraceRequests;

    quint64 m_pendingLocalsRequestId = 0;
    quint64 m_pendingModulesRequestId = 0;
    quint64 m_pendingSymbolsRequestId = 0;
    quint64 m_pendingRegistersRequestId = 0;
    // One memory request can end up as several reads: an unreadable range is
    // split until the readable part is known.
    class MemoryRequest
    {
    public:
        quint64 requestId = 0;
        quint64 base = 0;
        quint64 offset = 0;
        quint64 length = 0;
        std::shared_ptr<QByteArray> accumulator;
        std::shared_ptr<int> pending;
    };
    void fetchMemoryChunk(const MemoryRequest &request, quint64 offset, quint64 length);
    QHash<quint64, MemoryRequest> m_memoryRequests;
    quint64 m_nextMemoryToken = 0;

    // A peripheral register is a memory read whose answer goes to the register
    // handler rather than to a memory agent.
    class PeripheralRequest
    {
    public:
        quint64 requestId = 0;
        quint64 address = 0;
    };
    QHash<quint64, PeripheralRequest> m_peripheralRequests;
    quint64 m_nextPeripheralToken = 1000000;
    quint64 m_pendingModuleSymbolsRequestId = 0;

    QHash<int, quint64> m_breakpointRequestIds;
};

} // namespace Debugger::Internal
