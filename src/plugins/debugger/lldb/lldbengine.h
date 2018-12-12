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

#ifndef DEBUGGER_LLDBENGINE
#define DEBUGGER_LLDBENGINE

#include <debugger/debuggerengine.h>
#include <debugger/disassembleragent.h>
#include <debugger/memoryagent.h>
#include <debugger/watchhandler.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggerprotocol.h>

#include <utils/consoleprocess.h>
#include <utils/qtcprocess.h>

#include <QPointer>
#include <QProcess>
#include <QMap>
#include <QVariant>

namespace Debugger {
namespace Internal {

/* A debugger engine interfacing the LLDB debugger
 * using its Python interface.
 */

class LldbEngine : public CppDebuggerEngine
{
    Q_OBJECT

public:
    LldbEngine();
    ~LldbEngine() override;

signals:
    void outputReady(const QString &data);

private:
    void executeStepIn(bool byInstruction) override;
    void executeStepOut() override;
    void executeStepOver(bool byInstruction) override;

    void setupEngine() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;
    void abortDebuggerProcess() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void activateFrame(int index) override;
    void selectThread(const Thread &thread) override;
    void fetchFullBacktrace();

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const override;
    bool acceptsBreakpoint(const BreakpointParameters &bp) const override;
    void insertBreakpoint(const Breakpoint &bp) override;
    void removeBreakpoint(const Breakpoint &bp) override;
    void updateBreakpoint(const Breakpoint &bp) override;
    void enableSubBreakpoint(const SubBreakpoint &sbp, bool on) override;

    void assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command) override;

    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override;
    void reloadSourceFiles() override {}
    void reloadFullStack() override;
    void reloadDebuggingHelpers() override;
    void fetchDisassembler(Internal::DisassemblerAgent *) override;

    void setRegisterValue(const QString &name, const QString &value) override;

    void fetchMemory(MemoryAgent *, quint64 addr, quint64 length) override;
    void changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data) override;

    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const override;

    void handleLldbFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleLldbError(QProcess::ProcessError error);
    void readLldbStandardOutput();
    void readLldbStandardError();

    void handleStateNotification(const GdbMi &item);
    void handleLocationNotification(const GdbMi &location);
    void handleOutputNotification(const GdbMi &output);

    void handleResponse(const QString &data);
    void updateAll() override;
    void doUpdateLocals(const UpdateParameters &params) override;
    void updateBreakpointData(const Breakpoint &bp, const GdbMi &bkpt, bool added);
    void fetchStack(int limit);

    void runCommand(const DebuggerCommand &cmd) override;
    void debugLastCommand() override;
    void handleAttachedToCore();

private:
    DebuggerCommand m_lastDebuggableCommand;

    QString m_inbuffer;
    QString m_scriptFileName;
    Utils::QtcProcess m_lldbProc;

    // FIXME: Make generic.
    int m_lastAgentId = 0;
    int m_continueAtNextSpontaneousStop = false;
    QMap<QPointer<DisassemblerAgent>, int> m_disassemblerAgents;

    QHash<int, DebuggerCommand> m_commandForToken;
    DebuggerCommandSequence m_onStop;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE
