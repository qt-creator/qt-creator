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

namespace Debugger {
namespace Internal {

class QmlEngine;

class QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    QmlCppEngine(const DebuggerRunParameters &sp, QStringList *errors);
    ~QmlCppEngine() override;

    bool canDisplayTooltip() const override;
    bool canHandleToolTip(const DebuggerToolTipContext &) const override;
    void updateItem(const QString &iname) override;
    void expandItem(const QString &iname) override;
    void selectWatchData(const QString &iname) override;

    void watchPoint(const QPoint &) override;
    void fetchMemory(MemoryAgent *, quint64 addr, quint64 length) override;
    void changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data) override;
    void fetchDisassembler(DisassemblerAgent *) override;
    void activateFrame(int index) override;

    void reloadModules() override;
    void examineModules() override;
    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;

    void reloadRegisters() override;
    void reloadSourceFiles() override;
    void reloadFullStack() override;

    void setRegisterValue(const QString &name, const QString &value) override;
    bool hasCapability(unsigned cap) const override;

    bool isSynchronous() const override;
    QString qtNamespace() const override;

    void createSnapshot() override;
    void updateAll() override;

    void attemptBreakpointSynchronization() override;
    bool acceptsBreakpoint(Breakpoint bp) const override;
    void selectThread(ThreadId threadId) override;

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value) override;

    DebuggerEngine *cppEngine() override { return m_cppEngine; }
    DebuggerEngine *qmlEngine() const;

    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result) override;
    void resetLocation() override;
    void notifyInferiorIll() override;

protected:
    void detachDebugger() override;
    void reloadDebuggingHelpers() override;
    void debugLastCommand() override;
    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override;
    void executeNextI() override;
    void executeReturn() override;
    void continueInferior() override;
    void interruptInferior() override;
    void requestInterruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;
    void doUpdateLocals(const UpdateParameters &up) override;

    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;
    void quitDebugger() override;
    void abortDebugger() override;

    void notifyInferiorRunOk() override;
    void notifyInferiorSpontaneousStop() override;
    void notifyEngineRunAndInferiorRunOk() override;
    void notifyInferiorShutdownOk() override;

    void notifyInferiorSetupOk() override;
    void notifyEngineRemoteServerRunning(const QString &, int pid) override;
    void loadAdditionalQmlStack() override;

private:
    void engineStateChanged(DebuggerState newState);
    void setState(DebuggerState newState, bool forced = false) override;
    void slaveEngineStateChanged(DebuggerEngine *slaveEngine, DebuggerState state) override;

    void setActiveEngine(DebuggerEngine *engine);

private:
    QmlEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
};

} // namespace Internal
} // namespace Debugger
