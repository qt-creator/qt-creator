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

#ifndef DEBUGGER_CDBOUTPUT_H
#define DEBUGGER_CDBOUTPUT_H

#include <windows.h>
#include <inc/dbgeng.h>

namespace Debugger {
namespace Internal {

class CdbDebugEngine;

class CdbDebugOutput : public IDebugOutputCallbacks
{
public:
    explicit CdbDebugOutput(CdbDebugEngine* engine);

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
        IN PCSTR text
        );

private:
    CdbDebugEngine* m_pEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBOUTPUT_H
