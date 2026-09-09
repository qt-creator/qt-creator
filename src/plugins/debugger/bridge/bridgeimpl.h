// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../dap/dapstartdata.h"

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

class DEBUGGER_EXPORT BridgeImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit BridgeImpl(const DapStartData &startData);
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
    void runUserStartupCommands();
    void handleResponse(DapResponseType type, const QJsonObject &response);
    void handleEvent(DapEventType type, const QJsonObject &event);
    void interruptInferior();
    void interruptGdb();
    void handleResumeResponse(bool success);
    void handleStopped(const QJsonObject &event);
    void handleStackTrace(const QJsonObject &response);
    void reportStop();
    void handleBreakpointResponse(BreakpointOp op, const QJsonObject &response);
    void handleTracepointHit(const QJsonObject &body);

    int postRequest(const QString &command, const QJsonObject &arguments = {});
    void postWhenStopped(const QString &command, const QJsonObject &arguments,
                         const BreakpointChangeRequest &request);
    void failDeferredRequests();
    QJsonObject stepArguments(bool byInstruction) const;
    void fetchDisassemblyForTarget(quint64 requestId, quint64 address, const QString &target);
    void postLaunchOrAttach();
    void postBreakpointRequest(const QString &request, const BreakpointChangeRequest &change);

    const DapStartData m_startData;
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
    bool m_resumePending = false;
    bool m_interruptOnceRunning = false;
    bool m_detaching = false;
    bool m_stepRequested = false;
    bool m_shuttingDown = false;
    bool m_inferiorResumed = false;
    bool m_interruptOnceResumed = false;
    // Whether the next stop ends the setup rather than a run of its own.
    bool m_reportsSetupStop = false;

    // A request that arrived while the inferior was running: the bridge is
    // blocked in the resume then, so the request goes out on a stop forced for
    // it, and it is answered by hand if the inferior exits first.
    class DeferredRequest
    {
    public:
        QString command;
        QJsonObject arguments;
        quint64 requestId = 0;
        BreakpointOp op = BreakpointOp::Insert;
    };
    QList<DeferredRequest> m_deferredRequests;

    // The stop event carries no frame, so the location has to be asked for.
    // The answer is routed by the request's sequence number.
    class StackTraceRequest
    {
    public:
        bool reportsStop = false;
        quint64 refreshRequestId = 0;
        // Whether a step brought the inferior here, which is the only stop the
        // skip list has a say over.
        bool fromStep = false;
    };
    QHash<int, StackTraceRequest> m_stackTraceRequests;

    quint64 m_pendingLocalsRequestId = 0;
    quint64 m_pendingDumpersRequestId = 0;
    quint64 m_pendingModulesRequestId = 0;
    quint64 m_pendingSymbolsRequestId = 0;
    quint64 m_pendingSectionsRequestId = 0;
    quint64 m_pendingRegistersRequestId = 0;
    quint64 m_pendingBacktraceRequestId = 0;
    quint64 m_pendingThreadsRequestId = 0;
    quint64 m_pendingSourceFilesRequestId = 0;
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

    // The core file to be, kept until its request is answered.
    class SnapshotRequest
    {
    public:
        quint64 requestId = 0;
        Utils::FilePath filePath;
    };
    QHash<int, SnapshotRequest> m_snapshotRequests;

    QHash<int, quint64> m_breakpointRequestIds;

    struct Tracepoint
    {
        QString message;
        QList<TracepointCapture> captures;
    };
    QHash<int, Tracepoint> m_tracepoints;
};

} // namespace Debugger::Internal
