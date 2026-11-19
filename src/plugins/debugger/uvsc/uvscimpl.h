// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"
#include "uvscclient.h"

#include <utils/filepath.h>

#include <functional>
#include <memory>

namespace Debugger::Internal {

class DEBUGGER_EXPORT UvscImplStartData
{
public:
    // uVision itself. The UVSC library sits next to it.
    Utils::FilePath uVisionExecutable;
    // Where uVision listens for the socket connection, as a URL with a port.
    QString remoteChannel;
    Utils::FilePath projectFile;
    Utils::FilePath optionsFile;
    bool simulator = false;
    Utils::FilePath inferiorExecutable;
    // Empty creates the real client for the directory uVision is in.
    std::function<std::unique_ptr<UvscClientInterface>(const Utils::FilePath &uvscDir)>
        createClient;
};

class DEBUGGER_EXPORT UvscImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit UvscImpl(const UvscImplStartData &startData);
    ~UvscImpl() override;

private:
    void start() final;
    void shutdownInferior(ShutdownMode mode) final;
    void shutdownEngine() final;

    void execute(const ExecutionRequest &request) final;
    void refresh(const RefreshRequest &request) final;
    void changeBreakpoint(const BreakpointChangeRequest &request) final;
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) final;
    void selectThread(const QString &threadId) final;
    void activateFrame(int index) final;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName,
                          const QString &module, DisassemblyFlavor flavor) final;
    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;
    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) final;
    void setRegisterValue(const QString &name, const QString &value) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;

    void failSetup(const QString &message);
    void handleProjectClosed();
    void handleExecutionStarted();
    void handleExecutionStopped();
    void requestRun(const std::function<bool()> &command);
    void fetchThreads(GdbMi *threads = nullptr);
    void refreshLocals(const RefreshRequest &request);
    void refreshRegisters(quint64 requestId);
    void insertBreakpoint(const BreakpointChangeRequest &request);
    GdbMi breakpointData(quint32 tickMark) const;

    class BreakpointRecord
    {
    public:
        quint64 address = 0;
        int line = -1;
        QString fileName;
        QString function;
        bool enabled = true;
    };

    UvscImplStartData m_startData;
    QHash<quint32, BreakpointRecord> m_breakpoints;
    std::unique_ptr<UvscClientInterface> m_client;
    UvscClientInterface::Registers m_registers;
    quint64 m_address = 0;
    quint32 m_threadId = 0;
    int m_frameLevel = 0;
    bool m_loadingRequired = false;
    bool m_running = false;
    bool m_runRequested = false;
    bool m_stopRequested = false;
};

} // namespace Debugger::Internal
