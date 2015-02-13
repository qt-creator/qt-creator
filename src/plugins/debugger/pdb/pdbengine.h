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

#ifndef DEBUGGER_PDBENGINE_H
#define DEBUGGER_PDBENGINE_H

#include <debugger/debuggerengine.h>

#include <QProcess>
#include <QQueue>
#include <QVariant>

namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

/* A debugger engine for Python using the pdb command line debugger.
 */

class PdbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit PdbEngine(const DebuggerStartParameters &startParameters);
    ~PdbEngine();

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
    void shutdownInferior();
    void shutdownEngine();

    bool setToolTipExpression(TextEditor::TextEditorWidget *editorWidget,
        const DebuggerToolTipContext &);

    void continueInferior();
    void interruptInferior();

    void executeRunToLine(const ContextData &data);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const ContextData &data);

    void activateFrame(int index);
    void selectThread(ThreadId threadId);

    bool acceptsBreakpoint(Breakpoint bp) const;
    void insertBreakpoint(Breakpoint bp);
    void removeBreakpoint(Breakpoint bp);

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
    QString mainPythonFile() const;

signals:
    void outputReady(const QByteArray &data);

private:
    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const;

    void handlePdbFinished(int, QProcess::ExitStatus status);
    void handlePdbError(QProcess::ProcessError error);
    void readPdbStandardOutput();
    void readPdbStandardError();
    void handleOutput2(const QByteArray &data);
    void handleResponse(const QByteArray &ba);
    void handleOutput(const QByteArray &data);
    void updateAll();
    void updateLocals();
    void handleUpdateAll(const DebuggerResponse &response);
    void handleFirstCommand(const DebuggerResponse &response);
    void handleExecuteDebuggerCommand(const DebuggerResponse &response);

    struct PdbCommand
    {
        PdbCommand() : callback(0) {}

        DebuggerCommand::Callback callback;
        QByteArray command;
    };

    void handleStop(const DebuggerResponse &response);
    void handleBacktrace(const DebuggerResponse &response);
    void handleListLocals(const DebuggerResponse &response);
    void handleListModules(const DebuggerResponse &response);
    void handleListSymbols(const DebuggerResponse &response, const QString &moduleName);
    void handleBreakInsert(const DebuggerResponse &response, Breakpoint bp);

    void handleChildren(const WatchData &data0, const GdbMi &item,
        QList<WatchData> *list);
    void postCommand(const QByteArray &command,
                     DebuggerCommand::Callback callback = 0);
    void postDirectCommand(const QByteArray &command);

    QQueue<PdbCommand> m_commands;

    QByteArray m_inbuffer;
    QString m_scriptFileName;
    QProcess m_pdbProc;
    QString m_pdb;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PDBENGINE_H
