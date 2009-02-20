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

#include "cdbdebugeventcallback.h"
#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"
#include "debuggermanager.h"

#include <QtCore/QDebug>

namespace Debugger {
namespace Internal {

STDMETHODIMP CdbDebugEventCallback::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface)
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks)))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CdbDebugEventCallback::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) CdbDebugEventCallback::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP CdbDebugEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = DEBUG_EVENT_CREATE_PROCESS | DEBUG_EVENT_EXIT_PROCESS
            //| DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD
            | DEBUG_EVENT_BREAKPOINT
            | DEBUG_EVENT_EXCEPTION
            ;
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT Bp)
{
    qDebug() << "MSVCDebugEventCallback::Breakpoint";
    m_pEngine->m_d->handleBreakpointEvent(Bp);
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG FirstChance
    )
{
    qDebug() << "MSVCDebugEventCallback::Exception";
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::CreateThread(
    THIS_
    __in ULONG64 Handle,
    __in ULONG64 DataOffset,
    __in ULONG64 StartOffset
    )
{
    //Debugger::ThreadInfo ti;
    //ti.handle = Handle;
    //ti.dataOffset = DataOffset;
    //ti.startOffset = StartOffset;
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ExitThread(
    THIS_
    __in ULONG ExitCode
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::CreateProcess(
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
    )
{
    m_pEngine->m_d->m_hDebuggeeProcess = (HANDLE)Handle;
    m_pEngine->m_d->m_hDebuggeeThread = (HANDLE)InitialThreadHandle;
    //m_pEngine->qq->notifyStartupFinished();
    m_pEngine->m_d->qq->notifyInferiorRunning();

    ULONG currentThreadId;
    if (SUCCEEDED(m_pEngine->m_d->m_pDebugSystemObjects->GetThreadIdByHandle(InitialThreadHandle, &currentThreadId)))
        m_pEngine->m_d->m_currentThreadId = currentThreadId;
    else
        m_pEngine->m_d->m_currentThreadId = 0;

    m_pEngine->attemptBreakpointSynchronization();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ExitProcess(
    THIS_
    __in ULONG ExitCode
    )
{
    UNREFERENCED_PARAMETER(ExitCode);
    m_pEngine->m_d->m_hDebuggeeProcess = 0;
    m_pEngine->m_d->m_hDebuggeeThread = 0;
    m_pEngine->m_d->qq->notifyInferiorExited();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::LoadModule(
    THIS_
    __in ULONG64 ImageFileHandle,
    __in ULONG64 BaseOffset,
    __in ULONG ModuleSize,
    __in_opt PCSTR ModuleName,
    __in_opt PCSTR ImageName,
    __in ULONG CheckSum,
    __in ULONG TimeDateStamp
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::UnloadModule(
    THIS_
    __in_opt PCSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::SessionStatus(
    THIS_
    __in ULONG Status
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ChangeDebuggeeState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ChangeEngineState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ChangeSymbolState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

} // namespace Internal
} // namespace Debugger
