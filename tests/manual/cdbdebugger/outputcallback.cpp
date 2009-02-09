#include <stdio.h>
#include <windows.h>
#include <dbgeng.h>

#include "outputcallback.h"

WinDbgOutputCallback g_outputCallbacks;

STDMETHODIMP
WinDbgOutputCallback::QueryInterface(
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
WinDbgOutputCallback::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
WinDbgOutputCallback::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
WinDbgOutputCallback::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    UNREFERENCED_PARAMETER(Mask);
    fputs(Text, stdout);
    return S_OK;
}
