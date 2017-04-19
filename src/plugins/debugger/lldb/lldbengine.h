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
#include <QQueue>
#include <QMap>
#include <QStack>
#include <QVariant>


namespace Debugger {
namespace Internal {

class GdbMi;

/* A debugger engine interfacing the LLDB debugger
 * using its Python interface.
 */

class LldbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit LldbEngine(const DebuggerRunParameters &runParameters);
    ~LldbEngine() override;

signals:
    void outputReady(const QString &data);

private:
    DebuggerEngine *cppEngine() override { return this; }

    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override;
    void executeNextI() override;

    void setupEngine() override;
    void startLldb();
    void startLldbStage2();
    void setupInferior() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;
    void abortDebugger() override;

    bool canHandleToolTip(const DebuggerToolTipContext &) const override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;

    void activateFrame(int index) override;
    void selectThread(ThreadId threadId) override;
    void fetchFullBacktrace();

    // This should be always the last call in a function.
    bool stateAcceptsBreakpointChanges() const override;
    bool acceptsBreakpoint(Breakpoint bp) const override;
    void insertBreakpoint(Breakpoint bp) override;
    void removeBreakpoint(Breakpoint bp) override;
    void changeBreakpoint(Breakpoint bp) override;

    void assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;

    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override;
    void reloadSourceFiles() override {}
    void reloadFullStack() override;
    void reloadDebuggingHelpers() override;
    void fetchDisassembler(Internal::DisassemblerAgent *) override;

    bool isSynchronous() const override { return true; }
    void setRegisterValue(const QString &name, const QString &value) override;

    void fetchMemory(MemoryAgent *, quint64 addr, quint64 length) override;
    void changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data) override;

    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const override;

    void handleLldbFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleLldbError(QProcess::ProcessError error);
    void readLldbStandardOutput();
    void readLldbStandardError();

    void handleStateNotification(const GdbMi &state);
    void handleLocationNotification(const GdbMi &location);
    void handleOutputNotification(const GdbMi &output);

    void handleResponse(const QString &data);
    void updateAll() override;
    void doUpdateLocals(const UpdateParameters &params) override;
    void updateBreakpointData(Breakpoint bp, const GdbMi &bkpt, bool added);
    void fetchStack(int limit);

    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result) override;

    void runCommand(const DebuggerCommand &cmd) override;
    void debugLastCommand() override;

private:
    DebuggerCommand m_lastDebuggableCommand;

    QString m_inbuffer;
    QString m_scriptFileName;
    Utils::QtcProcess m_lldbProc;

    // FIXME: Make generic.
    int m_lastAgentId;
    int m_continueAtNextSpontaneousStop;
    QMap<QPointer<DisassemblerAgent>, int> m_disassemblerAgents;

    QHash<int, DebuggerCommand> m_commandForToken;
    DebuggerCommandSequence m_onStop;

    // Console handling.
    void stubError(const QString &msg);
    void stubExited();
    void stubStarted();
    Utils::ConsoleProcess m_stubProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE
