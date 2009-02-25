/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/


#include "cdbdebugoutput.h"
#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"

#include <windows.h>
#include <inc/dbgeng.h>

namespace Debugger {
namespace Internal {

CdbDebugOutput::CdbDebugOutput(CdbDebugEngine* engine)  :
    m_pEngine(engine)
{
}

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
    } else {
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
    m_pEngine->m_d->handleDebugOutput(text);
    return S_OK;
}

} // namespace Internal
} // namespace Debugger
