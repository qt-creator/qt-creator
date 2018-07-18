/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

// Define KDEXT_64BIT to make all wdbgexts APIs recognize 64 bit addresses
// It is recommended for extensions to use 64 bit headers from wdbgexts so
// the extensions could support 64 bit targets.

#include <string>
#include <sstream>

#include <windows.h>
#define KDEXT_64BIT
#pragma warning( disable : 4838  )
#include <wdbgexts.h>
#pragma warning( default : 4838  )
#include <dbgeng.h>

typedef IDebugControl3 CIDebugControl;
typedef IDebugSymbols3 CIDebugSymbols;
typedef IDebugSymbolGroup2 CIDebugSymbolGroup;
typedef IDebugClient5 CIDebugClient;
typedef IDebugSystemObjects CIDebugSystemObjects;
typedef IDebugDataSpaces4 CIDebugDataSpaces;
typedef IDebugAdvanced2 CIDebugAdvanced;
typedef IDebugRegisters2 CIDebugRegisters;

// Utility messages
std::string winErrorMessage(unsigned long error);
std::string winErrorMessage();
std::string msgDebugEngineComResult(HRESULT hr);
std::string msgDebugEngineComFailed(const char *func, HRESULT hr);

// Debug helper for anything streamable as in 'DebugPrint() << object'
// Derives from std::ostringstream and print out everything accumulated in destructor.
struct DebugPrint : public std::ostringstream {
    DebugPrint() {}
    ~DebugPrint() {
        dprintf("DEBUG: %s\n", str().c_str());
    }
};

struct Bench
{
    Bench(const std::string &what) : m_initialTickCount(GetTickCount()), m_what(what) {}
    ~Bench()
    {
        DebugPrint() << m_what << " took "
                     << GetTickCount() - m_initialTickCount << "ms to execute." << std::endl;
    }
    const DWORD m_initialTickCount;
    const std::string m_what;
};

ULONG currentThreadId(IDebugSystemObjects *sysObjects);
ULONG currentThreadId(CIDebugClient *client);
ULONG currentProcessId(IDebugSystemObjects *sysObjects);
ULONG currentProcessId(CIDebugClient *client);
std::string moduleNameByOffset(CIDebugSymbols *symbols, ULONG64 offset);
std::string sourceFileNameByOffset(CIDebugSymbols *symbols, ULONG64 offset, PULONG lineNumber);

#ifdef QTC_TRACE
#  define QTC_TRACE_IN dprintf(">%s\n", __FUNCTION__);
#  define QTC_TRACE_OUT dprintf("<%s\n", __FUNCTION__);
#else
#  define QTC_TRACE_IN
#  define QTC_TRACE_OUT
#endif
