#include "windbgeventcallback.h"
#include "debugger.h"

STDMETHODIMP
WinDbgEventCallback::QueryInterface(
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
WinDbgEventCallback::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
WinDbgEventCallback::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP WinDbgEventCallback::GetInterestMask(
    THIS_
    __out PULONG Mask
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::Breakpoint(
    THIS_
    __in PDEBUG_BREAKPOINT Bp
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Exception,
    __in ULONG FirstChance
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::CreateThread(
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

STDMETHODIMP WinDbgEventCallback::ExitThread(
    THIS_
    __in ULONG ExitCode
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::CreateProcess(
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
    m_pDebugger->m_hDebuggeeProcess = (HANDLE)Handle;
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::ExitProcess(
    THIS_
    __in ULONG ExitCode
    )
{
    m_pDebugger->m_hDebuggeeProcess = 0;
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::LoadModule(
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

STDMETHODIMP WinDbgEventCallback::UnloadModule(
    THIS_
    __in_opt PCSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::SessionStatus(
    THIS_
    __in ULONG Status
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::ChangeDebuggeeState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::ChangeEngineState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}

STDMETHODIMP WinDbgEventCallback::ChangeSymbolState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return S_OK;
}
