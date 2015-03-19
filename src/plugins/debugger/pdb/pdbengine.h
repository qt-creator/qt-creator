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
#include <QVariant>

namespace Debugger {
namespace Internal {
class DebuggerCommand;
class GdbMi;

/*
 * A debugger engine for Python using the pdb command line debugger.
 */

class PdbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    explicit PdbEngine(const DebuggerStartParameters &startParameters);

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

    bool setToolTipExpression(const DebuggerToolTipContext &);

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

    void assignValueInDebugger(WatchItem *item,
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
    void updateWatchItem(WatchItem *item);
    QString mainPythonFile() const;

    void runCommand(const DebuggerCommand &cmd);
    void postDirectCommand(const QByteArray &command);

    void refreshStack(const GdbMi &stack);
    void refreshLocals(const GdbMi &vars);
    void refreshModules(const GdbMi &modules);
    void refreshSymbols(const GdbMi &symbols);

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

    QByteArray m_inbuffer;
    QProcess m_pdbProc;
    QString m_pdb;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PDBENGINE_H
