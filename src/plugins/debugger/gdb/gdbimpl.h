// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <memory>

#include <QHash>
#include <QSet>
#include <QStringDecoder>

namespace Debugger::Internal {

enum class GdbImplTracepointCaptureType {
    Address, Caller, Callstack, FilePos, Function,
    Pid, ProcessName, Tick, Tid, ThreadName, Expression
};
struct GdbImplTracepointCaptureData
{
    GdbImplTracepointCaptureType type;
    QString expression;
    int start = 0;
    int end = 0;
};
struct GdbImplTracepointInfo
{
    QString message;
    QList<GdbImplTracepointCaptureData> captures;
};

class DEBUGGER_EXPORT GdbImplStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath dumperScriptsDir;
    QString mainFunctionName = "main";
    bool nativeMixedDebugging = false;
    bool isElfTarget = false;
};

class DEBUGGER_EXPORT GdbImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit GdbImpl(const GdbImplStartData &startData);
    ~GdbImpl() override;

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
    void fetchDisassemblyPointMixed(quint64 requestId, quint64 address,
                                    const QString &functionName);
    void fetchDisassemblyRangeMixed(quint64 requestId, quint64 address);
    void fetchDisassemblyRangePlain(quint64 requestId, quint64 address);
    bool reportDisassemblyIfUsable(quint64 requestId, quint64 address,
                                   const QString &consoleStreamOutput);
    QChar mixedDisasmFlag() const;

    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;

    void watchPoint(quint64 requestId, const QPoint &pnt) final;

    void createSnapshot(quint64 requestId) final;

    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;

    void insertBreakpointCommand(const BreakpointChangeRequest &request);
    void updateBreakpointCommand(const BreakpointChangeRequest &request);
    void handleWatchInsert(quint64 requestId, const DebuggerResponse &response);
    void handleInterpreterBreakpointInsert(quint64 requestId, const DebuggerResponse &response);
    void handleLocalAttach(const DebuggerResponse &response);
    void handleTerminalStubAttach(const DebuggerResponse &response, qint64 mainThreadId);
    void handleTargetRemote(const DebuggerResponse &response);
    void handleExtendedRemoteAttach(const DebuggerResponse &response);
    void continueAfterAttach();
    void handleShowVersion(const DebuggerResponse &response);

    void runRunRequestCommand(const QString &function, int flags = 0);

    void fetchRegisterValues(quint64 requestId);
    void handleModulesList(quint64 requestId, const DebuggerResponse &response);
    void handleModuleSymbols(quint64 requestId, const Utils::FilePath &modulePath,
                             const Utils::FilePath &tempFilePath, const DebuggerResponse &response);
    void requestModuleSections(quint64 requestId, const Utils::FilePath &modulePath,
                               bool useLegacyAllObjKeyword);
    void handleModuleSections(quint64 requestId, const Utils::FilePath &modulePath,
                              const DebuggerResponse &response,
                              bool isRetryWithLegacyKeyword);

    struct MemoryRequestCookie
    {
        std::shared_ptr<QByteArray> accumulator;
        std::shared_ptr<int> pendingRequests;
        quint64 requestId = 0;
        quint64 base = 0;
        quint64 offset = 0;
        quint64 length = 0;
    };
    void fetchMemoryHelper(const MemoryRequestCookie &cookie);
    void handleFetchMemory(const DebuggerResponse &response, const MemoryRequestCookie &cookie);

    void runCommand(const DebuggerCommand &command);
    void requestInferiorInterrupt();
    void runCommandNow(const DebuggerCommand &command);
    void handleOutputLine(const QString &line);
    void handleResultRecord(DebuggerResponse *response);

    GdbImplStartData m_startData;
    qint64 m_inferiorPid = -1;
    Utils::Process m_gdbProc;
    QString m_inbuffer;
    enum class AttachPhase { Idle, AwaitingConnect, Stopped, Continuing };
    AttachPhase m_attachPhase = AttachPhase::Idle;
    QString m_pendingConsoleStreamOutput;
    QString m_pendingLogStreamOutput;
    bool m_inNativeMixedStep = false;
    QStringDecoder m_outputDecoder{"UTF-8"};
    QHash<int, DebuggerCommand> m_commandForToken;
    bool m_interruptRequested = false;
    bool m_expectTerminalTrap = false;
    int m_gdbVersion = 0;
    bool m_inferiorRunning = false;
    bool m_runCommandPending = false;
    bool m_interruptOnceRunning = false;
    QList<DebuggerCommand> m_onStopCommands;
    bool m_onStopWantContinue = false;
    int m_lastToken = 0;

    bool m_dumpersReady = false;
    QList<DebuggerCommand> m_bufferedDumperCommands;

    DebuggerCommand m_lastDebuggableCommand;

    struct RegisterInfo
    {
        QString name;
        int size = 0;
        QString reportedType;
    };
    QHash<int, RegisterInfo> m_registerInfoByNumber;
    bool m_registerNamesListed = false;

    void handleTracepointInsert(quint64 requestId, const DebuggerResponse &response,
                                const QString &message,
                                const QList<GdbImplTracepointCaptureData> &captures);
    void handleTracepointHit(const GdbMi &data);
    QHash<QString, GdbImplTracepointInfo> m_tracepointsByNumber;

    void registerInternalBreakpointNumber(const QString &number);
    QSet<QString> m_internalBreakpointNumbers;
};
} // namespace Debugger::Internal
