/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DEBUGGER_CDBENGINE_H
#define DEBUGGER_CDBENGINE_H

#include "idebuggerengine.h"

namespace Debugger {
namespace Internal {

class DebuggerManager;
class CdbDebugEventCallback;
class CdbDebugOutput;
struct CdbDebugEnginePrivate;

class CdbDebugEngine : public IDebuggerEngine
{
    Q_OBJECT
public:
    CdbDebugEngine(DebuggerManager *parent);
    ~CdbDebugEngine();

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
    virtual void setUseCustomDumpers(bool on);

    virtual void reloadSourceFiles();

protected:
    void timerEvent(QTimerEvent*);

private:
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
