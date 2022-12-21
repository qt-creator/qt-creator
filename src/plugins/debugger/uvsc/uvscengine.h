// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvscclient.h"

#include <debugger/debuggerengine.h>

namespace Debugger::Internal {

class UvscEngine final : public CppDebuggerEngine
{
public:
    explicit UvscEngine();

    void setupEngine() final;
    void runEngine();
    void shutdownInferior() final;
    void shutdownEngine() final;

    bool hasCapability(unsigned cap) const final;

    void setRegisterValue(const QString &name, const QString &value) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;

    void executeStepOver(bool byInstruction) final;
    void executeStepIn(bool byInstruction) final;
    void executeStepOut() final;

    void continueInferior() final;
    void interruptInferior() final;

    void assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value) final;
    void selectThread(const Thread &thread) final;

    void activateFrame(int index) final;

    bool acceptsBreakpoint(const BreakpointParameters &params) const final;
    void insertBreakpoint(const Breakpoint &bp) final;
    void removeBreakpoint(const Breakpoint &bp) final;
    void updateBreakpoint(const Breakpoint &bp) final;

    void fetchDisassembler(DisassemblerAgent *agent) final;

    void changeMemory(MemoryAgent *agent, quint64 address, const QByteArray &data) final;
    void fetchMemory(MemoryAgent *agent, quint64 address, quint64 length) final;

    void reloadRegisters() final;
    void reloadPeripheralRegisters() final;

    void reloadFullStack() final;

private:
    void handleProjectClosed();
    void handleUpdateLocation(quint64 address);

    void handleStartExecution();
    void handleStopExecution();

    void handleThreadInfo();
    void handleReloadStack(bool isFull);
    void handleReloadRegisters();
    void handleReloadPeripheralRegisters(const QList<quint64> &addresses);
    void handleUpdateLocals(bool partial);
    void handleInsertBreakpoint(const QString &exp, const Breakpoint &bp);
    void handleRemoveBreakpoint(const Breakpoint &bp);
    void handleChangeBreakpoint(const Breakpoint &bp);

    void handleSetupFailure(const QString &errorMessage);
    void handleShutdownFailure(const QString &errorMessage);
    void handleRunFailure(const QString &errorMessage);
    void handleExecutionFailure(const QString &errorMessage);
    void handleStoppingFailure(const QString &errorMessage);

    void handleFetchMemory(MemoryAgent *agent, quint64 address, const QByteArray &data);
    void handleChangeMemory(MemoryAgent *agent, quint64 address, const QByteArray &data);

    void doUpdateLocals(const UpdateParameters &params) final;
    void updateAll() final;

    bool configureProject(const DebuggerRunParameters &rp);

    Utils::FilePaths enabledSourceFiles() const;
    quint32 currentThreadId() const;
    quint32 currentFrameLevel() const;

    bool m_simulator = false;
    bool m_loadingRequired = false;
    bool m_inUpdateLocals = false;
    quint64 m_address = 0;
    UvscClient::Registers m_registers;
    std::unique_ptr<UvscClient> m_client;
};

} // Debugger::Internal
