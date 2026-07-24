// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

namespace Debugger::Internal {

class DEBUGGER_EXPORT PdbImplStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath dumperScriptsDir;
};

class DEBUGGER_EXPORT PdbImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit PdbImpl(const PdbImplStartData &startData);
    ~PdbImpl() override;

private:
    // What a "Breakpoint <nr> at <file>:<line>" line coming back from pdb is an answer to.
    enum class BreakpointReply {
        Insert,     // a fresh engine-side insertion, to be answered with breakpointEvent()
        Reinsert,   // re-created after a restart, the engine already knows this breakpoint
        Temporary,  // our own tbreak behind a RunToLine/RunToFunction, invisible to the engine
    };

    class PendingBreakpointReply
    {
    public:
        BreakpointReply kind = BreakpointReply::Insert;
        quint64 fenceToken = 0;
        BreakpointChangeRequest request;
    };

    // pdb renumbers its breakpoints from 1 in a fresh process, so the number the engine
    // learned on insertion is not necessarily the one to address after a restart.
    class ActiveBreakpoint
    {
    public:
        BreakpointChangeRequest request;
        QString pdbNumber;
    };

    class PendingStackReply
    {
    public:
        bool forJumpToLine = false;
        quint64 requestId = 0;
    };

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

    void handlePdbOutput(const QString &output);
    void handleOutputLine(const QString &line);
    void handleStackReply(const GdbMi &item);
    void handleBreakpointReply(const QString &line);
    void handleBreakpointFence(quint64 token);

    void startPdbProcess();
    void resetTransientState();
    void requestInterrupt();
    void insertBreakpoint(const BreakpointChangeRequest &request, BreakpointReply kind);
    QString pdbNumberFor(const QString &responseId) const;
    QString responseIdFor(const QString &pdbNumber) const;

    void runCommand(const DebuggerCommand &command);
    void postDirectCommand(const QString &command);

    PdbImplStartData m_startData;
    Utils::Process m_pdbProc;
    QString m_inbuffer;

    bool m_inferiorRunning = false;
    bool m_sawInitialLocation = false;
    bool m_interruptRequested = false;
    bool m_continueConfirmedRunning = false;
    bool m_interruptPending = false;
    bool m_inferiorExited = false;
    bool m_expectLocationOnly = false;

    int m_currentFrame = 0;

    quint64 m_pendingLocalsRequestId = 0;
    quint64 m_pendingModulesRequestId = 0;
    quint64 m_pendingModuleSymbolsRequestId = 0;

    DebuggerCommand m_lastDebuggableCommand;

    QList<PendingBreakpointReply> m_pendingBreakpointReplies;
    QList<PendingStackReply> m_pendingStackReplies;
    QList<ActiveBreakpoint> m_activeBreakpoints;
    quint64 m_lastFenceToken = 0;

    bool m_isResetRestart = false;
    bool m_shuttingDown = false;
};
} // namespace Debugger::Internal
