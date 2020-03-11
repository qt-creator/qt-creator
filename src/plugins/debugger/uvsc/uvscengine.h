/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "uvscclient.h"

#include <debugger/debuggerengine.h>

namespace Utils { class FilePath; }

namespace Debugger {
namespace Internal {

class UvscEngine final : public CppDebuggerEngine
{
    Q_OBJECT

public:
    explicit UvscEngine();

    void setupEngine() final;
    void runEngine() final;
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

    bool stateAcceptsBreakpointChanges() const final;
    bool acceptsBreakpoint(const BreakpointParameters &params) const final;

    void insertBreakpoint(const Breakpoint &bp) final;
    void removeBreakpoint(const Breakpoint &bp) final;
    void updateBreakpoint(const Breakpoint &bp) final;

    void fetchDisassembler(DisassemblerAgent *agent) final;

    void reloadRegisters() final;
    void reloadPeripheralRegisters() final;

    void reloadFullStack() final;

private slots:
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

private:
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

} // namespace Internal
} // namespace Debugger
