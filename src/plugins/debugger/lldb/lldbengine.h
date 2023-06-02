// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DEBUGGER_LLDBENGINE
#define DEBUGGER_LLDBENGINE

#include <debugger/debuggerengine.h>
#include <debugger/disassembleragent.h>
#include <debugger/memoryagent.h>
#include <debugger/watchhandler.h>
#include <debugger/debuggertooltipmanager.h>
#include <debugger/debuggerprotocol.h>

#include <utils/process.h>

#include <QPointer>
#include <QProcess>
#include <QMap>
#include <QVariant>

namespace Debugger::Internal {

/* A debugger engine interfacing the LLDB debugger
 * using its Python interface.
 */

class LldbEngine : public CppDebuggerEngine
{
    Q_OBJECT

public:
    LldbEngine();

signals:
    void outputReady(const QString &data);

private:
    void executeStepIn(bool byInstruction) override;
    void executeStepOut() override;
    void executeStepOver(bool byInstruction) override;

    void setupEngine() override;
    void runEngine();
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
    bool acceptsBreakpoint(const BreakpointParameters &bp) const override;
    void insertBreakpoint(const Breakpoint &bp) override;
    void removeBreakpoint(const Breakpoint &bp) override;
    void updateBreakpoint(const Breakpoint &bp) override;
    void enableSubBreakpoint(const SubBreakpoint &sbp, bool on) override;

    void assignValueInDebugger(WatchItem *item, const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command) override;

    void loadSymbols(const Utils::FilePath &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const Utils::FilePath &moduleName) override;
    void reloadModules() override;
    void reloadRegisters() override;
    void reloadSourceFiles() override {}
    void reloadFullStack() override;
    void reloadDebuggingHelpers() override;
    void loadAdditionalQmlStack() override;
    void fetchDisassembler(Internal::DisassemblerAgent *) override;

    void setRegisterValue(const QString &name, const QString &value) override;

    void fetchMemory(MemoryAgent *, quint64 addr, quint64 length) override;
    void changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data) override;

    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const override;

    void handleLldbStarted();
    void handleLldbDone();
    void readLldbStandardOutput();
    void readLldbStandardError();

    void handleStateNotification(const GdbMi &item);
    void handleLocationNotification(const GdbMi &location);
    void handleOutputNotification(const GdbMi &output);
    void handleInterpreterBreakpointModified(const GdbMi &item);

    void handleResponse(const QString &data);
    void updateAll() override;
    void doUpdateLocals(const UpdateParameters &params) override;
    void updateBreakpointData(const Breakpoint &bp, const GdbMi &bkpt, bool added);
    void fetchStack(int limit, bool alsoQml = false);

    void runCommand(const DebuggerCommand &cmd) override;
    void debugLastCommand() override;
    void handleAttachedToCore();
    void executeCommand(const QString &command);

private:
    DebuggerCommand m_lastDebuggableCommand;

    QString m_inbuffer;
    QString m_scriptFileName;
    Utils::Process m_lldbProc;

    // FIXME: Make generic.
    int m_lastAgentId = 0;
    int m_continueAtNextSpontaneousStop = false;
    QMap<QPointer<DisassemblerAgent>, int> m_disassemblerAgents;

    QHash<int, DebuggerCommand> m_commandForToken;
    DebuggerCommandSequence m_onStop;
};

} // Debugger::Internal

#endif // DEBUGGER_LLDBENGINE
