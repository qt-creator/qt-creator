// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"
#include "extensioncontext.h"

class EventCallback : public IDebugEventCallbacks
{
public:
    explicit EventCallback(IDebugEventCallbacks *wrapped);
    virtual ~EventCallback() = default;
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

    // Call handleModuleLoad() when reimplementing this
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

    // Call handleModuleUnload() when reimplementing this
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
        IDebugEventCallbacks *m_wrapped;
};
