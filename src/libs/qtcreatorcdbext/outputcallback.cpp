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

#include "outputcallback.h"
#include "stringutils.h"
#include "extensioncontext.h"
#include "base64.h"

#include <cstring>

/* \class OutputCallback

    OutputCallback catches DEBUG_OUTPUT_DEBUGGEE and reports it
    base64-encoded back to Qt Creator.
    \ingroup qtcreatorcdbext
 */

OutputCallback::OutputCallback(IDebugOutputCallbacksWide *wrapped) :
    m_wrapped(wrapped), m_recording(false)
{
}

OutputCallback::~OutputCallback() // must be present to avoid exit crashes
{
}

STDMETHODIMP OutputCallback::QueryInterface(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacksWide)))
    {
        *Interface = (IDebugOutputCallbacksWide*)this;
        AddRef();
        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) OutputCallback::AddRef(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG) OutputCallback::Release(THIS)
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP OutputCallback::Output(
        THIS_
        IN ULONG mask,
        IN PCWSTR text
        )
{

    if (m_recording)
        m_recorded.append(text);
    // Do not unconditionally output ourselves here, as this causes an endless
    // recursion. Suppress prompts (note that sequences of prompts may mess parsing up)
    if (!m_wrapped || mask == DEBUG_OUTPUT_PROMPT)
        return S_OK;
    // Wrap debuggee output in gdbmi such that creator recognizes it
    if (mask != DEBUG_OUTPUT_DEBUGGEE) {
        m_wrapped->Output(mask, text);
        return S_OK;
    }
    // Base encode as GDBMI is not really made for wide chars
    std::ostringstream str;
    base64Encode(str, reinterpret_cast<const unsigned char *>(text), sizeof(wchar_t) * std::wcslen(text));
    ExtensionContext::instance().reportLong('E', 0, "debuggee_output", str.str().c_str());
    return S_OK;
}

void OutputCallback::startRecording()
{
    m_recorded.clear();
    m_recording = true;
}

std::wstring OutputCallback::stopRecording()
{
    const std::wstring rc = m_recorded;
    m_recorded.clear();
    m_recording = false;
    return rc;
}
