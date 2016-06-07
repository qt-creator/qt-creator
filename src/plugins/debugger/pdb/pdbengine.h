/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <debugger/debuggerengine.h>

#include <QProcess>
#include <QVariant>

namespace Debugger {
namespace Internal {
class DebuggerCommand;
class GdbMi;

/*
 * A debugger engine for Python using the pdb command line debugger.
 */

class PdbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit PdbEngine(const DebuggerRunParameters &runParameters);

private:
    // DebuggerEngine implementation
    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override;
    void executeNextI() override;

    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void activateFrame(int index) override;
    void selectThread(ThreadId threadId) override;

    bool acceptsBreakpoint(Breakpoint bp) const override;
    void insertBreakpoint(Breakpoint bp) override;
    void removeBreakpoint(Breakpoint bp) override;

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;

    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override {}
    void reloadSourceFiles() override {}
    void reloadFullStack() override {}

    bool supportsThreads() const { return true; }
    bool isSynchronous() const override { return true; }
    void updateItem(const QString &iname) override;

    void runCommand(const DebuggerCommand &cmd) override;
    void postDirectCommand(const QString &command);

    void refreshLocation(const GdbMi &reportedLocation);
    void refreshStack(const GdbMi &stack);
    void refreshLocals(const GdbMi &vars);
    void refreshModules(const GdbMi &modules);
    void refreshState(const GdbMi &reportedState);
    void refreshSymbols(const GdbMi &symbols);

    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const override;

    void handlePdbFinished(int, QProcess::ExitStatus status);
    void handlePdbError(QProcess::ProcessError error);
    void readPdbStandardOutput();
    void readPdbStandardError();
    void handleOutput2(const QString &data);
    void handleResponse(const QString &ba);
    void handleOutput(const QString &data);
    void updateAll() override;
    void updateLocals() override;

    QString m_inbuffer;
    QProcess m_proc;
    QString m_interpreter;
};

} // namespace Internal
} // namespace Debugger
