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

#include "hostutils.h"

#include <utils/synchronousprocess.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QProcess>
#include <QString>
#include <QTextStream>

#ifdef Q_OS_WIN

// Enable Win API of XP SP1 and later
#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <utils/winutils.h>
#include <tlhelp32.h>
#include <psapi.h>

#endif // Q_OS_WIN

namespace Debugger {
namespace Internal {

#ifdef Q_OS_WIN

bool winResumeThread(unsigned long dwThreadId, QString *errorMessage)
{
    bool ok = false;
    HANDLE handle = NULL;
    do {
        if (!dwThreadId)
            break;

        handle = OpenThread(SYNCHRONIZE |THREAD_QUERY_INFORMATION |THREAD_SUSPEND_RESUME,
                            FALSE, dwThreadId);
        if (handle==NULL) {
            *errorMessage = QString::fromLatin1("Unable to open thread %1: %2").
                            arg(dwThreadId).arg(Utils::winErrorMessage(GetLastError()));
            break;
        }
        if (ResumeThread(handle) == DWORD(-1)) {
            *errorMessage = QString::fromLatin1("Unable to resume thread %1: %2").
                            arg(dwThreadId).arg(Utils::winErrorMessage(GetLastError()));
            break;
        }
        ok = true;
    } while (false);
    if (handle != NULL)
        CloseHandle(handle);
    return ok;
}

bool isWinProcessBeingDebugged(unsigned long pid)
{
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (processHandle == NULL)
        return false;
    BOOL debugged = FALSE;
    CheckRemoteDebuggerPresent(processHandle, &debugged);
    CloseHandle(processHandle);
    return debugged != FALSE;
}

// Simple exception formatting
void formatWindowsException(unsigned long code, quint64 address,
                            unsigned long flags, quint64 info1, quint64 info2,
                            QTextStream &str)
{
    str.setIntegerBase(16);
    str << "\nException at 0x"  << address
            <<  ", code: 0x" << code << ": ";
    switch (code) {
    case winExceptionCppException:
        str << "C++ exception";
        break;
    case winExceptionStartupCompleteTrap:
        str << "Startup complete";
        break;
    case winExceptionDllNotFound:
        str << "DLL not found";
        break;
    case winExceptionDllEntryPointNoFound:
        str << "DLL entry point not found";
        break;
    case winExceptionDllInitFailed:
        str << "DLL failed to initialize";
        break;
    case winExceptionMissingSystemFile:
        str << "System file is missing";
        break;
    case winExceptionRpcServerUnavailable:
        str << "RPC server unavailable";
        break;
    case winExceptionRpcServerInvalid:
        str << "Invalid RPC server";
        break;
    case winExceptionWX86Breakpoint:
        str << "Win32 x86 emulation subsystem breakpoint hit";
        break;
    case EXCEPTION_ACCESS_VIOLATION: {
            const bool writeOperation = info1;
            str << (writeOperation ? "write" : "read")
                << " access violation at: 0x" << info2;
    }
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        str << "arrary bounds exceeded";
        break;
    case EXCEPTION_BREAKPOINT:
        str << "breakpoint";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        str << "datatype misalignment";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        str << "floating point exception";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        str << "division by zero";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        str << " floating-point operation cannot be represented exactly as a decimal fraction";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        str << "invalid floating-point operation";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        str << "floating-point overflow";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        str << "floating-point operation stack over/underflow";
        break;
    case  EXCEPTION_FLT_UNDERFLOW:
        str << "floating-point UNDERFLOW";
        break;
    case  EXCEPTION_ILLEGAL_INSTRUCTION:
        str << "invalid instruction";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        str << "page in error";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        str << "integer division by zero";
        break;
    case EXCEPTION_INT_OVERFLOW:
        str << "integer overflow";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        str << "invalid disposition to exception dispatcher";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        str << "attempt to continue execution after noncontinuable exception";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        str << "privileged instruction";
        break;
    case EXCEPTION_SINGLE_STEP:
        str << "single step";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        str << "stack_overflow";
        break;
    }
    str << ", flags=0x" << flags;
    if (flags == EXCEPTION_NONCONTINUABLE)
        str << " (execution cannot be continued)";
    str.setIntegerBase(10);
}

bool isDebuggerWinException(unsigned long code)
{
    return code == EXCEPTION_BREAKPOINT || code == EXCEPTION_SINGLE_STEP;
}

bool isFatalWinException(long code)
{
    switch (code) {
    case EXCEPTION_BREAKPOINT:
    case EXCEPTION_SINGLE_STEP:
    case winExceptionStartupCompleteTrap: // Mysterious exception at start of application
    case winExceptionRpcServerUnavailable:
    case winExceptionRpcServerInvalid:
    case winExceptionDllNotFound:
    case winExceptionDllEntryPointNoFound:
    case winExceptionCppException:
        return false;
    default:
        break;
    }
    return true;
}

#else // Q_OS_WIN

bool winResumeThread(unsigned long, QString *) { return false; }
bool isWinProcessBeingDebugged(unsigned long) { return false; }
void formatWindowsException(unsigned long , quint64, unsigned long,
                            quint64, quint64, QTextStream &) { }
bool isFatalWinException(long) { return false; }
bool isDebuggerWinException(unsigned long) { return false; }

#endif // !Q_OS_WIN

} // namespace Internal
} // namespace Debugger
