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

#ifndef DEBUGGER_CDBENGINEPRIVATE_H
#define DEBUGGER_CDBENGINEPRIVATE_H

#include "cdbdebugeventcallback.h"
#include "cdbdebugoutput.h"

namespace Debugger {
namespace Internal {

class DebuggerManager;
class IDebuggerManagerAccessForEngines;

// Thin wrapper around the 'DBEng' debugger engine shared library
// which is loaded at runtime.

class DebuggerEngineLibrary {
public:
    DebuggerEngineLibrary();
    bool init(QString *errorMessage);

    inline HRESULT debugCreate(REFIID interfaceId, PVOID *interfaceHandle) const
        { return m_debugCreate(interfaceId, interfaceHandle); }

private:
    // The exported functions of the library
    typedef HRESULT (*DebugCreateFunction)(REFIID, PVOID *);

    DebugCreateFunction m_debugCreate;
};

struct CdbDebugEnginePrivate
{    
    explicit CdbDebugEnginePrivate(const DebuggerEngineLibrary &lib, DebuggerManager *parent,  CdbDebugEngine* engine);
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

