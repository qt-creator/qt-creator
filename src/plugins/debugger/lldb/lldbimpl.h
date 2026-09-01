// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <QHash>
#include <QTimer>

#include <chrono>
#include <optional>

namespace Debugger::Internal {

class DEBUGGER_EXPORT LldbImplStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath dumperScriptsDir;
    bool nativeMixedDebugging = false;
    bool breakOnMain = false;
    bool continueAfterAttach = false;
    bool intelDisassembly = false;
    // Where an attached device's symbols live, and the platform to select.
    QString deviceSymbolsRoot;
    QString deviceUuid;
    QString platform;
    QStringList startupCommands;
    QStringList postAttachCommands;
    // Where to look for the shared libraries the inferior loads.
    Utils::FilePaths solibSearchPath;
    int qtVersion = 0;
    QString qtNamespace;
    Utils::FilePath extraDumperFile;
    QString extraDumperCommands;
    std::chrono::seconds watchdogTimeout{0};
};

class DEBUGGER_EXPORT LldbImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit LldbImpl(const LldbImplStartData &startData);
    ~LldbImpl() override;

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
    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;

    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;

    void handleLldbOutput(const QString &output);
    void handleStateReport(const GdbMi &item);
    void handleTracepointHit(const GdbMi &item);
    void fetchLocationAfterStop(InferiorEvent event);

    void runCommand(const DebuggerCommand &command);
    void restartWatchdog();
    void reportInferiorExitIfComplete();

    LldbImplStartData m_startData;
    bool m_continueAtNextSpontaneousStop = false;
    Utils::Process m_lldbProc;
    QTimer m_watchdog;
    QString m_inbuffer;
    QHash<int, DebuggerCommand> m_commandForToken;
    int m_lastToken = 0;
    DebuggerCommand m_lastDebuggableCommand;
    bool m_inferiorExited = false;
    std::optional<int> m_inferiorExitCode;
    bool m_inferiorExitReported = false;
    void interruptInferior();

    bool m_inferiorRunning = false;
    bool m_interruptOnceRunning = false;
    bool m_resumeAfterAttachPending = false;

    bool m_detached = false;
    qint64 m_inferiorPid = -1;
};
} // namespace Debugger::Internal
