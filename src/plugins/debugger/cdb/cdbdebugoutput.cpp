#include <windows.h>
#include <inc/dbgeng.h>

#include "cdbdebugoutput.h"
#include "cdbdebugengine.h"

namespace Debugger {
namespace Internal {

STDMETHODIMP CdbDebugOutput::QueryInterface(
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

STDMETHODIMP_(ULONG) CdbDebugOutput::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) CdbDebugOutput::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP CdbDebugOutput::Output(
    THIS_
    IN ULONG mask,
    IN PCSTR text
    )
{
    UNREFERENCED_PARAMETER(mask);
    m_pEngine->handleDebugOutput(text);
    return S_OK;
}

} // namespace Internal
} // namespace Debugger
