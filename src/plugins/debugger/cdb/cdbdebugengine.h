/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_CDBENGINE_H
#define DEBUGGER_CDBENGINE_H

#include "idebuggerengine.h"
#include "debuggermanager.h"

namespace Debugger {
namespace Internal {

class DebuggerManager;
class CdbDebugEventCallback;
class CdbDebugOutput;
struct CdbDebugEnginePrivate;

class CdbDebugEngine : public IDebuggerEngine
{
    Q_DISABLE_COPY(CdbDebugEngine)
    Q_OBJECT
    explicit CdbDebugEngine(DebuggerManager *parent);

public:
    ~CdbDebugEngine();

    // Factory function that returns 0 if the debug engine library cannot be found.
    static IDebuggerEngine *create(DebuggerManager *parent);

    virtual void shutdown();
    virtual void setToolTipExpression(const QPoint &pos, const QString &exp);
    virtual bool startDebugger();
    virtual void exitDebugger();
    virtual void updateWatchModel();

    virtual void stepExec();
    virtual void stepOutExec();
    virtual void nextExec();
    virtual void stepIExec();
    virtual void nextIExec();
    
    virtual void continueInferior();    
    virtual void interruptInferior();

    virtual void runToLineExec(const QString &fileName, int lineNumber);
    virtual void runToFunctionExec(const QString &functionName);
    virtual void jumpToLineExec(const QString &fileName, int lineNumber);
    virtual void assignValueInDebugger(const QString &expr, const QString &value);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void activateFrame(int index);
    virtual void selectThread(int index);

    virtual void attemptBreakpointSynchronization();

    virtual void loadSessionData();
    virtual void saveSessionData();

    virtual void reloadDisassembler();

    virtual void reloadModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();

    virtual void reloadRegisters();

    virtual void setDebugDumpers(bool on);
    virtual void recheckCustomDumperAvailability();

    virtual void reloadSourceFiles();

protected:
    void timerEvent(QTimerEvent*);

private slots:
    void slotConsoleStubStarted();
    void slotConsoleStubError(const QString &msg);
    void slotConsoleStubTerminated();

private:
    bool startAttachDebugger(qint64 pid, QString *errorMessage);
    bool startDebuggerWithExecutable(DebuggerStartMode sm, QString *errorMessage);
    void startWatchTimer();
    void killWatchTimer();

    CdbDebugEnginePrivate *m_d;

    friend struct CdbDebugEnginePrivate;
    friend class CdbDebugEventCallback;
    friend class CdbDebugOutput;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINE_H
