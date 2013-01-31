/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLGDBENGINE_H
#define QMLGDBENGINE_H

#include "debuggerengine.h"

namespace Debugger {
namespace Internal {

class QmlCppEnginePrivate;

class QmlCppEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    QmlCppEngine(const DebuggerStartParameters &sp, QString *errorMessage);
    ~QmlCppEngine();

    bool canDisplayTooltip() const;
    bool setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor * editor, const DebuggerToolTipContext &);
    void updateWatchData(const WatchData &data,
        const WatchUpdateFlags &flags);
    void watchDataSelected(const QByteArray &iname);

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

    void setRegisterValue(int regnr, const QString &value);
    bool hasCapability(unsigned cap) const;

    bool isSynchronous() const;
    QByteArray qtNamespace() const;

    void createSnapshot();
    void updateAll();

    void attemptBreakpointSynchronization();
    bool acceptsBreakpoint(BreakpointModelId id) const;
    void selectThread(ThreadId threadId);

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);

    DebuggerEngine *cppEngine() const;
    DebuggerEngine *qmlEngine() const;

    void notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort);
    void notifyEngineRemoteSetupFailed(const QString &message);

    void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    void resetLocation();

    void notifyInferiorIll();

protected:
    void detachDebugger();
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

private:
    void engineStateChanged(DebuggerState newState);
    void setState(DebuggerState newState, bool forced = false);
    void slaveEngineStateChanged(DebuggerEngine *slaveEngine, DebuggerState state);

    void readyToExecuteQmlStep();
    void setActiveEngine(DebuggerEngine *engine);

private:
    QmlCppEnginePrivate *d;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLGDBENGINE_H
