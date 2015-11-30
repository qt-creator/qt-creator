/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef COMMON_H
#define COMMON_H

// Define KDEXT_64BIT to make all wdbgexts APIs recognize 64 bit addresses
// It is recommended for extensions to use 64 bit headers from wdbgexts so
// the extensions could support 64 bit targets.

#include <string>
#include <sstream>

#include <windows.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
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

#endif // COMMON_H
