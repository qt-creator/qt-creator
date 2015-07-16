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

    bool canDisplayTooltip() const;
    bool canHandleToolTip(const DebuggerToolTipContext &) const;
    void updateItem(const QByteArray &iname);
    void expandItem(const QByteArray &iname);
    void selectWatchData(const QByteArray &iname);

    void watchPoint(const QPoint &);
    void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length);
    void fetchDisassembler(DisassemblerAgent *);
    void activateFrame(int index);

    void reloadModules();
    void examineModules();
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);

    void reloadRegisters();
    void reloadSourceFiles();
    void reloadFullStack();

    void setRegisterValue(const QByteArray &name, const QString &value);
    bool hasCapability(unsigned cap) const;

    bool isSynchronous() const;
    QByteArray qtNamespace() const;

    void createSnapshot();
    void updateAll();

    void attemptBreakpointSynchronization();
    bool acceptsBreakpoint(Breakpoint bp) const;
    void selectThread(ThreadId threadId);

    void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);

    DebuggerEngine *cppEngine() { return m_cppEngine; }
    DebuggerEngine *qmlEngine() const;

    void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);

    void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    void resetLocation();

    void notifyInferiorIll();

protected:
    void detachDebugger();
    void reloadDebuggingHelpers();
    void debugLastCommand();
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();
    void executeReturn();
    void continueInferior();
    void interruptInferior();
    void requestInterruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    void setupEngine();
    void setupInferior();
    void runEngine();
    void shutdownInferior();
    void shutdownEngine();
    void quitDebugger();
    void abortDebugger();

    void notifyInferiorRunOk();
    void notifyInferiorSpontaneousStop();
    void notifyEngineRunAndInferiorRunOk();
    void notifyInferiorShutdownOk();

    void notifyInferiorSetupOk();
    void notifyEngineRemoteServerRunning(const QByteArray &, int pid);

private:
    void engineStateChanged(DebuggerState newState);
    void setState(DebuggerState newState, bool forced = false);
    void slaveEngineStateChanged(DebuggerEngine *slaveEngine, DebuggerState state);

    void setActiveEngine(DebuggerEngine *engine);

private:
    QmlEngine *m_qmlEngine;
    DebuggerEngine *m_cppEngine;
    DebuggerEngine *m_activeEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLCPPENGINE_H
