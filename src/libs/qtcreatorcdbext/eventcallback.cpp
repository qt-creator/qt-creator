/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "eventcallback.h"
#include "extensioncontext.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

static const char eventContextC[] = "event";
static const char moduleContextC[] = "module";

// Special exception codes (see dbgwinutils.cpp).
enum { winExceptionCppException = 0xe06d7363,
       winExceptionStartupCompleteTrap = 0x406d1388,
       winExceptionRpcServerUnavailable = 0x6ba,
       winExceptionRpcServerInvalid = 0x6a6,
       winExceptionDllNotFound = 0xc0000135,
       winExceptionDllEntryPointNoFound = 0xc0000139,
       winExceptionDllInitFailed = 0xc0000142,
       winExceptionMissingSystemFile = 0xc0000143,
       winExceptionAppInitFailed = 0xc0000143
};

EventCallback::EventCallback(IDebugEventCallbacks *wrapped) :
    m_wrapped(wrapped)
{
}

EventCallback::~EventCallback()
{
}

STDMETHODIMP EventCallback::QueryInterface(
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

STDMETHODIMP_(ULONG) EventCallback::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) EventCallback::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP EventCallback::GetInterestMask(THIS_ __out PULONG mask)
{
    if (m_wrapped)
        m_wrapped->GetInterestMask(mask);

    *mask |= DEBUG_EVENT_CREATE_PROCESS  | DEBUG_EVENT_EXIT_PROCESS
            | DEBUG_EVENT_BREAKPOINT
            | DEBUG_EVENT_EXCEPTION | DEBUG_EVENT_LOAD_MODULE | DEBUG_EVENT_UNLOAD_MODULE;
    return S_OK;
}

STDMETHODIMP EventCallback::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT b)
{
    // Breakpoint hit - Set the stop reason parameters on the extension context.
    typedef ExtensionContext::StopReasonMap StopReasonMap;
    typedef ExtensionContext::StopReasonMap::value_type StopReasonMapValue;

    ULONG id = 0;
    ULONG64 address = 0;
    b->GetId(&id);
    b->GetOffset(&address);

    StopReasonMap stopReason;
    stopReason.insert(StopReasonMapValue(std::string("breakpointId"), toString(id)));
    if (address)
        stopReason.insert(StopReasonMapValue(std::string("breakpointAddress"), toString(address)));
    ExtensionContext::instance().setStopReason(stopReason, "breakpoint");
    return m_wrapped ? m_wrapped->Breakpoint(b) : S_OK;
}

static inline ExtensionContext::StopReasonMap exceptionParameters(const EXCEPTION_RECORD64 &e,
                                                                  unsigned firstChance)
{
    typedef ExtensionContext::StopReasonMap StopReasonMap;
    typedef ExtensionContext::StopReasonMap::value_type StopReasonMapValue;
    // Fill exception record
    StopReasonMap parameters;
    parameters.insert(StopReasonMapValue(std::string("firstChance"), toString(firstChance)));
    parameters.insert(StopReasonMapValue(std::string("exceptionAddress"),
                                         toString(e.ExceptionAddress)));
    parameters.insert(StopReasonMapValue(std::string("exceptionCode"),
                                         toString(e.ExceptionCode)));
    parameters.insert(StopReasonMapValue(std::string("exceptionFlags"),
                                         toString(e.ExceptionFlags)));
    // Hard code some parameters (used for access violations)
    if (e.NumberParameters >= 1)
        parameters.insert(StopReasonMapValue(std::string("exceptionInformation0"),
                                             toString(e.ExceptionInformation[0])));
    if (e.NumberParameters >= 2)
        parameters.insert(StopReasonMapValue(std::string("exceptionInformation1"),
                                             toString(e.ExceptionInformation[1])));
    // Add top stack frame if possible
    StackFrame frame;
    std::string errorMessage;
    // If it is a C++ exception, get frame #2 (first outside MSVC runtime)
    const unsigned frameNumber = e.ExceptionCode == winExceptionCppException ? 2 : 0;
    if (getFrame(frameNumber, &frame, &errorMessage)) {
        if (!frame.fullPathName.empty()) {
            parameters.insert(StopReasonMapValue(std::string("exceptionFile"),
                                                 wStringToString(frame.fullPathName)));
            parameters.insert(StopReasonMapValue(std::string("exceptionLine"),
                                                 toString(frame.line)));
        }
        if (!frame.function.empty())
            parameters.insert(StopReasonMapValue(std::string("exceptionFunction"),
                                                 wStringToString(frame.function)));
    } // getCurrentFrame
    return parameters;
}

STDMETHODIMP EventCallback::Exception(
    THIS_
    __in PEXCEPTION_RECORD64 Ex,
    __in ULONG FirstChance
    )
{
    // Report the exception as GBMI and set potential stop reason
    const ExtensionContext::StopReasonMap parameters =
            exceptionParameters(*Ex, FirstChance);

    std::ostringstream str;
    formatGdbmiHash(str, parameters);
    ExtensionContext::instance().setStopReason(parameters, "exception");
    ExtensionContext::instance().report('E', 0, "exception", "%s", str.str().c_str());
    return m_wrapped ? m_wrapped->Exception(Ex, FirstChance) : S_OK;
}

STDMETHODIMP EventCallback::CreateThread(
    THIS_
    __in ULONG64 Handle,
    __in ULONG64 DataOffset,
    __in ULONG64 StartOffset
    )
{
    return m_wrapped ? m_wrapped->CreateThread(Handle, DataOffset, StartOffset) : S_OK;
}

STDMETHODIMP EventCallback::ExitThread(
    THIS_
    __in ULONG  ExitCode
    )
{
    return m_wrapped ? m_wrapped->ExitThread(ExitCode) : S_OK;
}

STDMETHODIMP EventCallback::CreateProcess(
    THIS_
    __in ULONG64 ImageFileHandle,
    __in ULONG64 Handle,
    __in ULONG64 BaseOffset,
    __in ULONG    ModuleSize,
    __in_opt PCSTR ModuleName,
    __in_opt PCSTR ImageName,
    __in ULONG  CheckSum,
    __in ULONG  TimeDateStamp,
    __in ULONG64 InitialThreadHandle,
    __in ULONG64 ThreadDataOffset,
    __in ULONG64 StartOffset
    )
{
    return m_wrapped ? m_wrapped->CreateProcess(ImageFileHandle, Handle,
                                                 BaseOffset, ModuleSize, ModuleName,
                                                 ImageName, CheckSum, TimeDateStamp,
                                                 InitialThreadHandle, ThreadDataOffset,
                                                 StartOffset) : S_OK;
}

STDMETHODIMP EventCallback::ExitProcess(
    THIS_
    __in ULONG ExitCode
    )
{
    ExtensionContext::instance().report('E', 0, eventContextC, "Process exited (%lu)",
                                        ExitCode);

    dprintf("%s ExitProcess %u\n", creatorOutputPrefixC, ExitCode);
    return m_wrapped ? m_wrapped->ExitProcess(ExitCode) : S_OK;
}

STDMETHODIMP EventCallback::LoadModule(
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
    ExtensionContext::instance().report('E', 0, moduleContextC, "L:%s:%s:0x%llx:0x%llx\n",
                                        ModuleName, ImageName, BaseOffset, ModuleSize);

    return m_wrapped ? m_wrapped->LoadModule(ImageFileHandle, BaseOffset,
                                             ModuleSize, ModuleName, ImageName,
                                             CheckSum, TimeDateStamp) : S_OK;
}

STDMETHODIMP EventCallback::UnloadModule(
    THIS_
    __in_opt PCSTR ImageBaseName,
    __in ULONG64 BaseOffset
    )
{
    ExtensionContext::instance().report('U', 0, moduleContextC, "U:%s\n",
                                        ImageBaseName);

    return m_wrapped ? m_wrapped->UnloadModule(ImageBaseName, BaseOffset) : S_OK;
}

STDMETHODIMP EventCallback::SystemError(
    THIS_
    __in ULONG Error,
    __in ULONG Level
    )
{
    return m_wrapped ? m_wrapped->SystemError(Error, Level) : S_OK;
}

STDMETHODIMP EventCallback::SessionStatus(
    THIS_
    __in ULONG Status
    )
{
    return m_wrapped ? m_wrapped->SessionStatus(Status) : S_OK;
}

STDMETHODIMP EventCallback::ChangeDebuggeeState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return m_wrapped ? m_wrapped->ChangeDebuggeeState(Flags, Argument) : S_OK;
}

STDMETHODIMP EventCallback::ChangeEngineState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return m_wrapped ? m_wrapped->ChangeEngineState(Flags, Argument) : S_OK;
}

STDMETHODIMP EventCallback::ChangeSymbolState(
    THIS_
    __in ULONG Flags,
    __in ULONG64 Argument
    )
{
    return m_wrapped ? m_wrapped->ChangeSymbolState(Flags, Argument) : S_OK;
}
