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

#include "procinterrupt.h"
#include "debuggerconstants.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess> // makes kill visible on Windows.

using namespace Debugger::Internal;

static inline QString msgCannotInterrupt(qint64 pid, const QString &why)
{
    return QString::fromLatin1("Cannot interrupt process %1: %2").arg(pid).arg(why);
}

#if defined(Q_OS_WIN)

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* WinXP, needed for DebugBreakProcess() */

#include <utils/winutils.h>
#include <windows.h>

#if !defined(PROCESS_SUSPEND_RESUME) // Check flag for MinGW
#    define PROCESS_SUSPEND_RESUME (0x0800)
#endif // PROCESS_SUSPEND_RESUME

static BOOL isWow64Process(HANDLE hproc)
{
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

    BOOL ret = false;

    static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
    if (!fnIsWow64Process) {
        if (HMODULE hModule = GetModuleHandle(L"kernel32.dll"))
            fnIsWow64Process = reinterpret_cast<LPFN_ISWOW64PROCESS>(GetProcAddress(hModule, "IsWow64Process"));
    }

    if (!fnIsWow64Process) {
        qWarning("Cannot retrieve symbol 'IsWow64Process'.");
        return false;
    }

    if (!fnIsWow64Process(hproc, &ret)) {
        qWarning("IsWow64Process() failed for %p: %s",
                 hproc, qPrintable(Utils::winErrorMessage(GetLastError())));
        return false;
    }
    return ret;
}

// Open the process and break into it
bool Debugger::Internal::interruptProcess(qint64 pID, int engineType, QString *errorMessage, const bool engineExecutableIs64Bit)
{
    bool ok = false;
    HANDLE inferior = NULL;
    do {
        const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
                |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
                |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
        inferior = OpenProcess(rights, FALSE, DWORD(pID));
        if (inferior == NULL) {
            *errorMessage = QString::fromLatin1("Cannot open process %1: %2").
                    arg(pID).arg(Utils::winErrorMessage(GetLastError()));
            break;
        }

        enum DebugBreakApi {
            UseDebugBreakApi,
            UseWin64Interrupt,
            UseWin32Interrupt
        };
/*
    Windows 64 bit has a 32 bit subsystem (WOW64) which makes it possible to run a
    32 bit application inside a 64 bit environment.
    When GDB is used DebugBreakProcess must be called from the same system (32/64 bit) running
    the inferior. If CDB is used we could in theory break wow64 processes,
    but the break is actually a wow64 breakpoint. CDB is configured to ignore these
    breakpoints, because they also appear on module loading.
    Therefore we need helper executables (win(32/64)interrupt.exe) on Windows 64 bit calling
    DebugBreakProcess from the correct system.

    DebugBreak matrix for windows

    Api = UseDebugBreakApi
    Win64 = UseWin64Interrupt
    Win32 = UseWin32Interrupt
    N/A = This configuration is not possible

          | Windows 32bit   | Windows 64bit
          | QtCreator 32bit | QtCreator 32bit                   | QtCreator 64bit
          | Inferior 32bit  | Inferior 32bit  | Inferior 64bit  | Inferior 32bit  | Inferior 64bit |
----------|-----------------|-----------------|-----------------|-----------------|----------------|
CDB 32bit | Api             | Api             | NA              | Win32           | NA             |
    64bit | NA              | Win64           | Win64           | Api             | Api            |
----------|-----------------|-----------------|-----------------|-----------------|----------------|
GDB 32bit | Api             | Api             | NA              | Win32           | NA             |
    64bit | NA              | Api             | Win64           | Win32           | Api            |
----------|-----------------|-----------------|-----------------|-----------------|----------------|

*/

        DebugBreakApi breakApi = UseDebugBreakApi;
#ifdef Q_OS_WIN64
        if ((engineType == GdbEngineType && isWow64Process(inferior))
                || (engineType == CdbEngineType && !engineExecutableIs64Bit)) {
            breakApi = UseWin32Interrupt;
        }
#else
        if (isWow64Process(GetCurrentProcess())
                && ((engineType == CdbEngineType && engineExecutableIs64Bit)
                    || (engineType == GdbEngineType && !isWow64Process(inferior)))) {
            breakApi = UseWin64Interrupt;
        }
#endif
        if (breakApi == UseDebugBreakApi) {
            ok = DebugBreakProcess(inferior);
            if (!ok)
                *errorMessage = QLatin1String("DebugBreakProcess failed: ") + Utils::winErrorMessage(GetLastError());
        } else {
            const QString executable = breakApi == UseWin32Interrupt
                    ? QCoreApplication::applicationDirPath() + QLatin1String("/win32interrupt.exe")
                    : QCoreApplication::applicationDirPath() + QLatin1String("/win64interrupt.exe");
            if (!QFile::exists(executable)) {
                *errorMessage = QString::fromLatin1("%1 does not exist. If you have built QtCreator "
                                                    "on your own ,checkout "
                                                    "https://code.qt.io/cgit/qt-creator/binary-artifacts.git/.").
                        arg(QDir::toNativeSeparators(executable));
                break;
            }
            switch (QProcess::execute(executable, QStringList(QString::number(pID)))) {
            case -2:
                *errorMessage = QString::fromLatin1("Cannot start %1. Check src\\tools\\win64interrupt\\win64interrupt.c for more information.").
                                arg(QDir::toNativeSeparators(executable));
                break;
            case 0:
                ok = true;
                break;
            default:
                *errorMessage = QDir::toNativeSeparators(executable)
                                + QLatin1String(" could not break the process.");
                break;
            }
            break;
        }
    } while (false);
    if (inferior != NULL)
        CloseHandle(inferior);
    if (!ok)
        *errorMessage = msgCannotInterrupt(pID, *errorMessage);
    return ok;
}

#else // Q_OS_WIN

#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

bool Debugger::Internal::interruptProcess(qint64 pID, int /* engineType */,
                                          QString *errorMessage, const bool /*engineExecutableIs64Bit*/)
{
    if (pID <= 0) {
        *errorMessage = msgCannotInterrupt(pID, QString::fromLatin1("Invalid process id."));
        return false;
    }
    if (kill(pID, SIGINT)) {
        *errorMessage = msgCannotInterrupt(pID, QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    return true;
}

#endif // !Q_OS_WIN
