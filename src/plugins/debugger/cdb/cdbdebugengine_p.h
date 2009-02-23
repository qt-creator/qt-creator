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

#ifndef DEBUGGER_CDBENGINEPRIVATE_H
#define DEBUGGER_CDBENGINEPRIVATE_H

#include "cdbdebugeventcallback.h"
#include "cdbdebugoutput.h"

namespace Debugger {
namespace Internal {

class DebuggerManager;
class IDebuggerManagerAccessForEngines;

struct CdbDebugEnginePrivate
{    
    explicit CdbDebugEnginePrivate(DebuggerManager *parent,  CdbDebugEngine* engine);
    ~CdbDebugEnginePrivate();

    bool isDebuggeeRunning() const { return m_watchTimer != -1; }
    void handleDebugEvent();
    void updateThreadList();
    void updateStackTrace();
    void handleDebugOutput(const char* szOutputString);
    void handleBreakpointEvent(PDEBUG_BREAKPOINT pBP);

    HANDLE                  m_hDebuggeeProcess;
    HANDLE                  m_hDebuggeeThread;
    int                     m_currentThreadId;
    bool                    m_bIgnoreNextDebugEvent;

    int                     m_watchTimer;
    IDebugClient5*          m_pDebugClient;
    IDebugControl4*         m_pDebugControl;
    IDebugSystemObjects4*   m_pDebugSystemObjects;
    IDebugSymbols3*         m_pDebugSymbols;
    IDebugRegisters2*       m_pDebugRegisters;
    CdbDebugEventCallback   m_debugEventCallBack;
    CdbDebugOutput          m_debugOutputCallBack;

    CdbDebugEngine* m_engine;
    DebuggerManager *m_debuggerManager;
    IDebuggerManagerAccessForEngines *m_debuggerManagerAccess;
};

enum { debugCDB = 0 };

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINEPRIVATE_H
