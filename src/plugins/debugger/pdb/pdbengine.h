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

#ifndef DEBUGGER_PDBENGINE_H
#define DEBUGGER_PDBENGINE_H

#include "debuggerengine.h"

#include <QProcess>
#include <QQueue>
#include <QVariant>


namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

/* A debugger engine for Python using the pdb command line debugger.
 */

class PdbResponse
{
public:
    QByteArray data;
    QVariant cookie;
};

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
    void insertBreakpoint(BreakpointModelId id);
    void removeBreakpoint(BreakpointModelId id);

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

signals:
    void outputReady(const QByteArray &data);

private:
    QString errorMessage(QProcess::ProcessError error) const;
    bool hasCapability(unsigned cap) const;

    Q_SLOT void handlePdbFinished(int, QProcess::ExitStatus status);
    Q_SLOT void handlePdbError(QProcess::ProcessError error);
    Q_SLOT void readPdbStandardOutput();
    Q_SLOT void readPdbStandardError();
    Q_SLOT void handleOutput2(const QByteArray &data);
    void handleResponse(const QByteArray &ba);
    void handleOutput(const QByteArray &data);
    void updateAll();
    void updateLocals();
    void handleUpdateAll(const PdbResponse &response);
    void handleFirstCommand(const PdbResponse &response);
    void handleExecuteDebuggerCommand(const PdbResponse &response);

    typedef void (PdbEngine::*PdbCommandCallback)
        (const PdbResponse &response);

    struct PdbCommand
    {
        PdbCommand()
            : callback(0), callbackName(0)
        {}

        PdbCommandCallback callback;
        const char *callbackName;
        QByteArray command;
        QVariant cookie;
        //QTime postTime;
    };

    void handleStop(const PdbResponse &response);
    void handleBacktrace(const PdbResponse &response);
    void handleListLocals(const PdbResponse &response);
    void handleListModules(const PdbResponse &response);
    void handleListSymbols(const PdbResponse &response);
    void handleBreakInsert(const PdbResponse &response);

    void handleChildren(const WatchData &data0, const GdbMi &item,
        QList<WatchData> *list);
    void postCommand(const QByteArray &command,
                     //GdbCommandFlags flags = 0,
                     PdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());
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
