/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbdebugeventcallback.h"
#include "cdbdebugengine.h"
#include "cdbexceptionutils.h"
#include "cdbdebugengine_p.h"
#include "debuggermanager.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

//     CdbDebugEventCallbackBase
CdbDebugEventCallbackBase::CdbDebugEventCallbackBase()
{
}

STDMETHODIMP CdbDebugEventCallbackBase::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface)
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks)))  {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CdbDebugEventCallbackBase::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) CdbDebugEventCallbackBase::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP CdbDebugEventCallbackBase::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT2)
{
    return S_OK;
}
STDMETHODIMP CdbDebugEventCallbackBase::Exception(
    THIS_
    __in PEXCEPTION_RECORD64,
    __in ULONG /* FirstChance */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::CreateThread(
    THIS_
    __in ULONG64 /* Handle */,
    __in ULONG64 /* DataOffset */,
    __in ULONG64 /* StartOffset */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::ExitThread(
    THIS_
    __in ULONG /* ExitCode */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::CreateProcess(
    THIS_
    __in ULONG64 /* ImageFileHandle */,
    __in ULONG64 /* Handle */,
    __in ULONG64 /* BaseOffset */,
    __in ULONG /* ModuleSize */,
    __in_opt PCWSTR /* ModuleName */,
    __in_opt PCWSTR /* ImageName */,
    __in ULONG /* CheckSum */,
    __in ULONG /* TimeDateStamp */,
    __in ULONG64 /* InitialThreadHandle */,
    __in ULONG64 /* ThreadDataOffset */,
    __in ULONG64 /* StartOffset */
    )
{    
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::ExitProcess(
    THIS_
    __in ULONG /* ExitCode */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::LoadModule(
    THIS_
    __in ULONG64 /* ImageFileHandle */,
    __in ULONG64 /* BaseOffset */,
    __in ULONG /* ModuleSize */,
    __in_opt PCWSTR /* ModuleName */,
    __in_opt PCWSTR /* ImageName */,
    __in ULONG /* CheckSum */,
    __in ULONG /* TimeDateStamp */
    )
{    
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::UnloadModule(
    THIS_
    __in_opt PCWSTR /* ImageBaseName */,
    __in ULONG64 /* BaseOffset */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::SystemError(
    THIS_
    __in ULONG /* Error */,
    __in ULONG /* Level */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::SessionStatus(
    THIS_
    __in ULONG /* Status */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::ChangeDebuggeeState(
    THIS_
    __in ULONG /* Flags */,
    __in ULONG64 /* Argument */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::ChangeEngineState(
    THIS_
    __in ULONG /* Flags */,
    __in ULONG64 /* Argument */
    )
{
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallbackBase::ChangeSymbolState(
    THIS_
    __in ULONG /* Flags */,
    __in ULONG64 /* Argument */
    )
{    
    return S_OK;
}

IDebugEventCallbacksWide *CdbDebugEventCallbackBase::getEventCallback(CIDebugClient *clnt)
{
    IDebugEventCallbacksWide *rc = 0;
    if (SUCCEEDED(clnt->GetEventCallbacksWide(&rc)))
        return rc;
    return 0;
}

// ---------- CdbDebugEventCallback

CdbDebugEventCallback::CdbDebugEventCallback(CdbDebugEngine* dbg) :
    m_pEngine(dbg)
{
}

STDMETHODIMP CdbDebugEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = DEBUG_EVENT_CREATE_PROCESS  | DEBUG_EVENT_EXIT_PROCESS
            | DEBUG_EVENT_LOAD_MODULE   | DEBUG_EVENT_UNLOAD_MODULE
            | DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD
            | DEBUG_EVENT_BREAKPOINT
            | DEBUG_EVENT_EXCEPTION;
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT2 Bp)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    m_pEngine->m_d->handleBreakpointEvent(Bp);
    return S_OK;
}


STDMETHODIMP CdbDebugEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG /* FirstChance */
    )
{
    QString msg;
    {
        QTextStream str(&msg);
        formatException(Exception, m_pEngine->m_d->m_dumper, str);
    }
    const bool fatal = isFatalException(Exception->ExceptionCode);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\nex=" << Exception->ExceptionCode << " fatal=" << fatal << msg;
    m_pEngine->manager()->showApplicationOutput(msg);
    if (fatal)
        m_pEngine->m_d->notifyCrashed();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::CreateThread(
    THIS_
    __in ULONG64 Handle,
    __in ULONG64 DataOffset,
    __in ULONG64 StartOffset
    )
{
    Q_UNUSED(Handle)
    Q_UNUSED(DataOffset)
    Q_UNUSED(StartOffset)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    m_pEngine->m_d->updateThreadList();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ExitThread(
    THIS_
    __in ULONG ExitCode
    )
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << ExitCode;
    // @TODO: It seems the terminated thread is still in the list...
    m_pEngine->m_d->updateThreadList();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::CreateProcess(
    THIS_
    __in ULONG64 ImageFileHandle,
    __in ULONG64 Handle,
    __in ULONG64 BaseOffset,
    __in ULONG ModuleSize,
    __in_opt PCWSTR ModuleName,
    __in_opt PCWSTR ImageName,
    __in ULONG CheckSum,
    __in ULONG TimeDateStamp,
    __in ULONG64 InitialThreadHandle,
    __in ULONG64 ThreadDataOffset,
    __in ULONG64 StartOffset
    )
{
    Q_UNUSED(ImageFileHandle)
    Q_UNUSED(BaseOffset)
    Q_UNUSED(ModuleSize)
    Q_UNUSED(ModuleName)
    Q_UNUSED(ImageName)
    Q_UNUSED(CheckSum)
    Q_UNUSED(TimeDateStamp)
    Q_UNUSED(ThreadDataOffset)
    Q_UNUSED(StartOffset)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << ModuleName;
    m_pEngine->m_d->processCreatedAttached(Handle, InitialThreadHandle);
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::ExitProcess(
    THIS_
    __in ULONG ExitCode
    )
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << ExitCode;
    m_pEngine->processTerminated(ExitCode);
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::LoadModule(
    THIS_
    __in ULONG64 ImageFileHandle,
    __in ULONG64 BaseOffset,
    __in ULONG ModuleSize,
    __in_opt PCWSTR ModuleName,
    __in_opt PCWSTR ImageName,
    __in ULONG CheckSum,
    __in ULONG TimeDateStamp
    )
{
    Q_UNUSED(ImageFileHandle)
    Q_UNUSED(BaseOffset)
    Q_UNUSED(ModuleName)
    Q_UNUSED(ModuleSize)
    Q_UNUSED(ImageName)
    Q_UNUSED(CheckSum)
    Q_UNUSED(TimeDateStamp)
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << ModuleName;
    m_pEngine->m_d->handleModuleLoad(QString::fromUtf16(reinterpret_cast<const ushort *>(ModuleName)));
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::UnloadModule(
    THIS_
    __in_opt PCWSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    Q_UNUSED(ImageBaseName)
    Q_UNUSED(BaseOffset)
    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO << ImageBaseName;
    m_pEngine->m_d->updateModules();
    return S_OK;
}

STDMETHODIMP CdbDebugEventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << Error << Level;
    return S_OK;
}

// -----------ExceptionLoggerEventCallback
CdbExceptionLoggerEventCallback::CdbExceptionLoggerEventCallback(int logChannel, DebuggerManager *manager) :
    m_logChannel(logChannel),
    m_manager(manager)
{
}

STDMETHODIMP CdbExceptionLoggerEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = DEBUG_EVENT_EXCEPTION;
    return S_OK;
}

STDMETHODIMP CdbExceptionLoggerEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG /* FirstChance */
    )
{
    m_exceptionCodes.push_back(Exception->ExceptionCode);
    m_exceptionMessages.push_back(QString());
    {
        QTextStream str(&m_exceptionMessages.back());
        formatException(Exception, str);
    }
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n' << m_exceptionMessages.back();
    m_manager->showDebuggerOutput(m_logChannel, m_exceptionMessages.back());
    return S_OK;
}

// -----------IgnoreDebugEventCallback
IgnoreDebugEventCallback::IgnoreDebugEventCallback()
{
}

STDMETHODIMP IgnoreDebugEventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    *mask = 0;
    return S_OK;
}

// --------- EventCallbackRedirector
EventCallbackRedirector::EventCallbackRedirector(CIDebugClient *client,  IDebugEventCallbacksWide *cb) :
        m_client(client),
        m_oldCb(CdbDebugEventCallbackBase::getEventCallback(client))
{
    client->SetEventCallbacksWide(cb);
}

EventCallbackRedirector::~EventCallbackRedirector()
{
    m_client->SetEventCallbacksWide(m_oldCb);
}

} // namespace Internal
} // namespace Debugger
