/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "eventcallback.h"
#include "extensioncontext.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

static const char eventContextC[] = "event";

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

typedef ExtensionContext::StopReasonMap StopReasonMap;

/*!
    \class EventCallback

    Event handler wrapping the original IDebugEventCallbacks
    to catch and store exceptions (report crashes as stop reasons).
    \ingroup qtcreatorcdbext
*/

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

// Fill a map with parameters to be reported for a breakpoint stop.
static StopReasonMap breakPointStopReasonParameters(PDEBUG_BREAKPOINT b)
{
    typedef StopReasonMap::value_type StopReasonMapValue;

    StopReasonMap rc;
    ULONG uId = 0;
    if (FAILED(b->GetId(&uId)))
        return rc;
    rc.insert(StopReasonMapValue(std::string("breakpointId"), toString(uId)));
    const std::pair<ULONG64, ULONG> memoryRange = breakPointMemoryRange(b);
    if (!memoryRange.first)
        return rc;
    rc.insert(StopReasonMapValue(std::string("breakpointAddress"), toString(memoryRange.first)));
    // Report the memory for data breakpoints allowing for watching for changed bits
    // on the client side.
    if (!memoryRange.second)
        return rc;
    // Try to grab a IDataSpace from somewhere to get the memory
    if (CIDebugClient *client = ExtensionContext::instance().hookedClient()) {
        IInterfacePointer<CIDebugDataSpaces> dataSpaces(client);
        if (dataSpaces) {
            const std::wstring memoryHex = memoryToHexW(dataSpaces.data(), memoryRange.first, memoryRange.second);
            if (!memoryHex.empty())
                rc.insert(StopReasonMapValue("memory", wStringToString(memoryHex)));
        } // dataSpaces
    } // client
    return rc;
}

STDMETHODIMP EventCallback::Breakpoint(THIS_ __in PDEBUG_BREAKPOINT b)
{
    // Breakpoint hit - Set the stop reason parameters on the extension context.
    ExtensionContext::instance().setStopReason(breakPointStopReasonParameters(b),
                                               ExtensionContext::breakPointStopReasonC);
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
    ExtensionContext::instance().report('E', 0, 0, "exception", "%s", str.str().c_str());
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
    ExtensionContext::instance().report('E', 0, 0, eventContextC, "Process exited (%lu)",
                                        ExitCode);
    const HRESULT hr = m_wrapped ? m_wrapped->ExitProcess(ExitCode) : S_OK;
    // Remotely debugged process exited, there is no session-inactive notification.
    // Note: We get deleted here, so, order is important.
    ExtensionContext::instance().unhookCallbacks();
    return hr;
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
