/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLCPPENGINE_H
#define QMLCPPENGINE_H

#include <debugger/debuggerengine.h>

namespace Debugger {
namespace Internal {

class QmlEngine;

class QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    QmlCppEngine(const DebuggerRunParameters &sp, QStringList *errors);
    ~QmlCppEngine();

    bool canDisplayTooltip() const override;
    bool canHandleToolTip(const DebuggerToolTipContext &) const override;
    void updateItem(const QByteArray &iname) override;
    void expandItem(const QByteArray &iname) override;
    void selectWatchData(const QByteArray &iname) override;

    void watchPoint(const QPoint &) override;
    void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length) override;
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

    void setRegisterValue(const QByteArray &name, const QString &value) override;
    bool hasCapability(unsigned cap) const override;

    bool isSynchronous() const override;
    QByteArray qtNamespace() const override;

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

    void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const override;
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
    void notifyEngineRemoteServerRunning(const QByteArray &, int pid) override;

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

#endif // QMLCPPENGINE_H
