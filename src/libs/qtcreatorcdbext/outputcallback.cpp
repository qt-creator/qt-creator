/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "outputcallback.h"
#include "stringutils.h"
#include "extensioncontext.h"
#include "base64.h"

#include <cstring>

OutputCallback::OutputCallback(IDebugOutputCallbacksWide *wrapped) : m_wrapped(wrapped)
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
