// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"

/* OutputCallback catches DEBUG_OUTPUT_DEBUGGEE and reports it
 * hex-encoded back to Qt Creator. */
class OutputCallback : public IDebugOutputCallbacksWide
{
public:
    explicit OutputCallback(IDebugOutputCallbacksWide *wrapped);
    virtual ~OutputCallback();
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

    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
            THIS_
            IN ULONG mask,
            IN PCWSTR text
            );

    void startRecording();
    std::wstring stopRecording();

private:
    IDebugOutputCallbacksWide *m_wrapped;
    bool m_recording = false;
    std::wstring m_recorded;
};
