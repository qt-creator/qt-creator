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

#ifndef DEBUGGER_CDBDEBUGEVENTCALLBACK_H
#define DEBUGGER_CDBDEBUGEVENTCALLBACK_H

#include <windows.h>
#include <inc/dbgeng.h>

namespace Debugger {
namespace Internal {

class CdbDebugEngine;

class CdbDebugEventCallback : public IDebugEventCallbacks
{
public:
    explicit CdbDebugEventCallback(CdbDebugEngine* dbg);

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugEventCallbacks.

    STDMETHOD(GetInterestMask)(
        THIS_
        __out PULONG mask
        );

    STDMETHOD(Breakpoint)(
        THIS_
        __in PDEBUG_BREAKPOINT Bp
        );

    STDMETHOD(Exception)(
        THIS_
        __in PEXCEPTION_RECORD64 Exception,
        __in ULONG FirstChance
        );

    STDMETHOD(CreateThread)(
        THIS_
        __in ULONG64 Handle,
        __in ULONG64 DataOffset,
        __in ULONG64 StartOffset
        );
    STDMETHOD(ExitThread)(
        THIS_
        __in ULONG ExitCode
        );

    STDMETHOD(CreateProcess)(
        THIS_
        __in ULONG64 ImageFileHandle,
        __in ULONG64 Handle,
        __in ULONG64 BaseOffset,
        __in ULONG ModuleSize,
        __in_opt PCSTR ModuleName,
        __in_opt PCSTR ImageName,
        __in ULONG CheckSum,
        __in ULONG TimeDateStamp,
        __in ULONG64 InitialThreadHandle,
        __in ULONG64 ThreadDataOffset,
        __in ULONG64 StartOffset
        );

    STDMETHOD(ExitProcess)(
        THIS_
        __in ULONG ExitCode
        );

    STDMETHOD(LoadModule)(
        THIS_
        __in ULONG64 ImageFileHandle,
        __in ULONG64 BaseOffset,
        __in ULONG ModuleSize,
        __in_opt PCSTR ModuleName,
        __in_opt PCSTR ImageName,
        __in ULONG CheckSum,
        __in ULONG TimeDateStamp
        );

    STDMETHOD(UnloadModule)(
        THIS_
        __in_opt PCSTR ImageBaseName,
        __in ULONG64 BaseOffset
        );

    STDMETHOD(SystemError)(
        THIS_
        __in ULONG Error,
        __in ULONG Level
        );

    STDMETHOD(SessionStatus)(
        THIS_
        __in ULONG Status
        );

    STDMETHOD(ChangeDebuggeeState)(
        THIS_
        __in ULONG Flags,
        __in ULONG64 Argument
        );

    STDMETHOD(ChangeEngineState)(
        THIS_
        __in ULONG Flags,
        __in ULONG64 Argument
        );

    STDMETHOD(ChangeSymbolState)(
        THIS_
        __in ULONG Flags,
        __in ULONG64 Argument
        );

private:
    CdbDebugEngine *m_pEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBDEBUGEVENTCALLBACK_H
