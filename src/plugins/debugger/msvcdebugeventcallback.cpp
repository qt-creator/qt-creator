#include "msvcdebugeventcallback.h"
#include "msvcdebugengine.h"
#include "debuggermanager.h"

#include <QDebug>

namespace Debugger {
namespace Internal {

STDMETHODIMP
MSVCDebugEventCallback::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
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

STDMETHODIMP_(ULONG)
MSVCDebugEventCallback::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
MSVCDebugEventCallback::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP MSVCDebugEventCallback::GetInterestMask(
    THIS_
    __out PULONG mask
    )
{
    *mask = DEBUG_EVENT_CREATE_PROCESS | DEBUG_EVENT_EXIT_PROCESS
            //| DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD
            | DEBUG_EVENT_BREAKPOINT
            | DEBUG_EVENT_EXCEPTION
            ;
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::Breakpoint(
    THIS_
    __in PDEBUG_BREAKPOINT Bp
    )
{
    qDebug() << "MSVCDebugEventCallback::Breakpoint";
    m_pEngine->handleBreakpointEvent(Bp);
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG FirstChance
    )
{
    qDebug() << "MSVCDebugEventCallback::Exception";
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::CreateThread(
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

STDMETHODIMP MSVCDebugEventCallback::ExitThread(
    THIS_
    __in ULONG ExitCode
    )
{
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::CreateProcess(
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
    m_pEngine->m_hDebuggeeProcess = (HANDLE)Handle;
    m_pEngine->m_hDebuggeeThread = (HANDLE)InitialThreadHandle;
    m_pEngine->qq->notifyStartupFinished();
    m_pEngine->qq->notifyInferiorRunning();

    ULONG currentThreadId;
    if (SUCCEEDED(m_pEngine->m_pDebugSystemObjects->GetThreadIdByHandle(InitialThreadHandle, &currentThreadId)))
        m_pEngine->m_currentThreadId = currentThreadId;
    else
        m_pEngine->m_currentThreadId = 0;

    m_pEngine->attemptBreakpointSynchronization();
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::ExitProcess(
    THIS_
    __in ULONG ExitCode
    )
{
    UNREFERENCED_PARAMETER(ExitCode);
    m_pEngine->m_hDebuggeeProcess = 0;
    m_pEngine->m_hDebuggeeThread = 0;
    m_pEngine->qq->notifyInferiorExited();
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::LoadModule(
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

STDMETHODIMP MSVCDebugEventCallback::UnloadModule(
    THIS_
    __in_opt PCSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::SessionStatus(
    THIS_
    __in ULONG Status
    )
{
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::ChangeDebuggeeState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::ChangeEngineState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP MSVCDebugEventCallback::ChangeSymbolState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

} // namespace Internal
} // namespace Debugger
