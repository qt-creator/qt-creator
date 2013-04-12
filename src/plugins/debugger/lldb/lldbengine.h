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

#ifndef DEBUGGER_LLDBENGINE
#define DEBUGGER_LLDBENGINE

#include "debuggerengine.h"

#include <QProcess>
#include <QStack>
#include <QQueue>
#include <QVariant>


namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

/* A debugger engine for using the lldb command line debugger.
 */

class LldbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit LldbEngine(const DebuggerStartParameters &startParameters);
    ~LldbEngine();

private:
    // DebuggerEngine implementation
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void setupEngine();
    void setupInferior();
    void runEngine();
    void runEngine2();
    void shutdownInferior();
    void shutdownEngine();

    bool setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, const DebuggerToolTipContext &);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(ThreadId threadId);

    bool acceptsBreakpoint(BreakpointModelId id) const;
    void attemptBreakpointSynchronization();
    bool attemptBreakpointSynchronizationHelper(QByteArray *command);

    void assignValueInDebugger(const WatchData *data,
        const QString &expr, const QVariant &value);
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    void requestModuleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return true; }
    bool isSynchronous() const { return true; }
    void updateWatchData(const WatchData &data, const WatchUpdateFlags &flags);

    void performContinuation();

signals:
    void outputReady(const QByteArray &data);

private:
    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const;

    Q_SLOT void handleLldbFinished(int, QProcess::ExitStatus status);
    Q_SLOT void handleLldbError(QProcess::ProcessError error);
    Q_SLOT void readLldbStandardOutput();
    Q_SLOT void readLldbStandardError();
    Q_SLOT void handleResponse(const QByteArray &data);
    void refreshAll(const GdbMi &all);
    void refreshThreads(const GdbMi &threads);
    void refreshStack(const GdbMi &stack);
    void refreshLocals(const GdbMi &vars);
    void refreshTypeInfo(const GdbMi &typeInfo);
    void refreshState(const GdbMi &state);
    void refreshLocation(const GdbMi &location);
    void refreshModules(const GdbMi &modules);
    void refreshBreakpoints(const GdbMi &bkpts);

    void updateAll();

    typedef void (LldbEngine::*LldbCommandContinuation)();

    struct LldbCommand
    {
        LldbCommand() : token(0) {}
        QByteArray command;
        int token;
    };

    QByteArray currentOptions() const;
    void handleStop(const QByteArray &response);
    void handleListLocals(const QByteArray &response);
    void handleListModules(const QByteArray &response);
    void handleListSymbols(const QByteArray &response);
    void handleBreakpointsSynchronized(const QByteArray &response);
    void updateBreakpointData(const GdbMi &bkpt, bool added);
    void handleUpdateStack(const QByteArray &response);
    void handleUpdateThreads(const QByteArray &response);

    void handleChildren(const WatchData &data0, const GdbMi &item,
        QList<WatchData> *list);
    void runSimpleCommand(const QByteArray &command);
    void runCommand(const QByteArray &function,
        const QByteArray &extraArgs = QByteArray());
    GdbMi parseResultFromString(QByteArray out);

    QQueue<LldbCommand> m_commands;
    QStack<LldbCommandContinuation> m_continuations;

    QByteArray m_inbuffer;
    QString m_scriptFileName;
    QProcess m_lldbProc;
    QString m_lldb;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_LLDBENGINE
