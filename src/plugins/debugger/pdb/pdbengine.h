/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_PDBENGINE_H
#define DEBUGGER_PDBENGINE_H

#include "idebuggerengine.h"

#include <QtCore/QProcess>
#include <QtCore/QQueue>
#include <QtCore/QVariant>


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

class PdbEngine : public IDebuggerEngine
{
    Q_OBJECT

public:
    PdbEngine(DebuggerManager *manager);
    ~PdbEngine();

private:
    // IDebuggerEngine implementation
    void executeStep();
    void executeStepOut();
    void executeNext();
    void executeStepI();
    void executeNextI();

    void shutdown();
    void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);
    void startDebugger(const DebuggerStartParametersPtr &sp);

    void exitDebugger();

    void continueInferior();
    Q_SLOT void runInferior();
    void interruptInferior();

    void executeRunToLine(const QString &fileName, int lineNumber);
    void executeRunToFunction(const QString &functionName);
    void executeJumpToLine(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    void attemptBreakpointSynchronization();

    void assignValueInDebugger(const QString &expr, const QString &value);
    void executeDebuggerCommand(const QString & command);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    virtual QList<Symbol> moduleSymbols(const QString &moduleName);
    void reloadModules();
    void reloadRegisters() {}
    void reloadSourceFiles() {}
    void reloadFullStack() {}

    bool supportsThreads() const { return true; }
    bool isSynchroneous() const { return true; }
    void updateWatchData(const WatchData &data);

private:
    void debugMessage(const QString &msg);
    QString errorMessage(QProcess::ProcessError error) const;

    Q_SLOT void handlePdbFinished(int, QProcess::ExitStatus status);
    Q_SLOT void handlePdbError(QProcess::ProcessError error);
    Q_SLOT void readPdbStandardOutput();
    Q_SLOT void readPdbStandardError();
    void handleResponse(const QByteArray &ba);
    void updateAll();
    void handleUpdateAll(const PdbResponse &response);

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
    void handleLoadDumper(const PdbResponse &response);
    void handleBreakInsert(const PdbResponse &response);

    void handleChildren(const WatchData &data0, const GdbMi &item,
        QList<WatchData> *list);
    void postCommand(const QByteArray &command,
                     //GdbCommandFlags flags = 0,
                     PdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());

    QQueue<PdbCommand> m_commands;

    QByteArray m_inbuffer; 
    QString m_scriptFileName;
    QProcess m_pdbProc;
    QString m_pdb;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PDBENGINE_H
