// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopprocesssignaloperation.h"

#include "../projectexplorertr.h"

#include <utils/winutils.h>
#include <utils/fileutils.h>
#include <utils/processinfo.h>

#include <QDir>
#include <QGuiApplication>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif // PROCESS_SUSPEND_RESUME
#else // Q_OS_WIN
#include <errno.h>
#include <signal.h>
#endif // else Q_OS_WIN

using namespace Utils;

namespace ProjectExplorer {

void DesktopProcessSignalOperation::killProcess(qint64 pid)
{
    emit finished(killProcessSilently(pid));
}

void DesktopProcessSignalOperation::killProcess(const QString &filePath)
{
    Result<> result = ResultOk;
    const QList<ProcessInfo> processInfoList = ProcessInfo::processInfoList().value_or(
        QList<ProcessInfo>());
    for (const ProcessInfo &processInfo : processInfoList) {
        if (processInfo.commandLine == filePath)
            result = killProcessSilently(processInfo.processId);
    }
    emit finished(result);
}

void DesktopProcessSignalOperation::interruptProcess(qint64 pid)
{
    emit finished(interruptProcessSilently(pid));
}

static Result<> cannotKillError(qint64 pid, const QString &why)
{
    return ResultError(Tr::tr("Cannot kill process with pid %1: %2").arg(pid).arg(why));
}

static Result<> appendCannotInterruptError(qint64 pid, const QString &why,
                                         const Result<> &previousResult = ResultOk)
{
    QString error = Tr::tr("Cannot interrupt process with pid %1: %2").arg(pid).arg(why);
    if (previousResult.has_value())
        error.append('\n' + previousResult.error());
    return ResultError(error);
}

Result<> DesktopProcessSignalOperation::killProcessSilently(qint64 pid)
{
#ifdef Q_OS_WIN
    const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
            |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
            |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
    if (const HANDLE handle = OpenProcess(rights, FALSE, DWORD(pid))) {
        const Result<> result = TerminateProcess(handle, UINT(-1))
                              ? ResultOk : cannotKillError(pid, winErrorMessage(GetLastError()));
        CloseHandle(handle);
        return result;
    } else {
        return cannotKillError(pid, Tr::tr("Cannot open process."));
    }
#else
    if (pid <= 0)
        return cannotKillError(pid, Tr::tr("Invalid process id."));
    else if (kill(pid, SIGKILL))
        return cannotKillError(pid, QString::fromLocal8Bit(strerror(errno)));
    return ResultOk;
#endif // Q_OS_WIN
}

Result<> DesktopProcessSignalOperation::interruptProcessSilently(qint64 pid)
{
    Result<> result = ResultOk;
#ifdef Q_OS_WIN
    enum SpecialInterrupt { NoSpecialInterrupt, Win32Interrupt, Win64Interrupt };

    bool is64BitSystem = is64BitWindowsSystem();
    SpecialInterrupt si = NoSpecialInterrupt;
    if (is64BitSystem)
        si = is64BitWindowsBinary(m_debuggerCommand) ? Win64Interrupt : Win32Interrupt;
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
    Win64 = UseWin64InterruptHelper
    Win32 = UseWin32InterruptHelper
    N/A = This configuration is not possible

          | Windows 32bit   | Windows 64bit
          | QtCreator 32bit | QtCreator 32bit                   | QtCreator 64bit
          | Inferior 32bit  | Inferior 32bit  | Inferior 64bit  | Inferior 32bit  | Inferior 64bit
----------|-----------------|-----------------|-----------------|-----------------|----------------
CDB 32bit | Api             | Api             | N/A             | Win32           | N/A
    64bit | N/A             | Win64           | Win64           | Api             | Api
----------|-----------------|-----------------|-----------------|-----------------|----------------
GDB 32bit | Api             | Api             | N/A             | Win32           | N/A
    64bit | N/A             | N/A             | Win64           | N/A             | Api
----------|-----------------|-----------------|-----------------|-----------------|----------------

    */
    HANDLE inferior = NULL;
    do {
        const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
                |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
                |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
        inferior = OpenProcess(rights, FALSE, pid);
        if (inferior == NULL) {
            return appendCannotInterruptError(pid, Tr::tr("Cannot open process: %1")
                                            + winErrorMessage(GetLastError()), result);
        }
        bool creatorIs64Bit = is64BitWindowsBinary(
            FilePath::fromUserInput(QCoreApplication::applicationFilePath()));
        if (!is64BitSystem
                || si == NoSpecialInterrupt
                || (si == Win64Interrupt && creatorIs64Bit)
                || (si == Win32Interrupt && !creatorIs64Bit)) {
            if (!DebugBreakProcess(inferior)) {
                result = appendCannotInterruptError(pid, Tr::tr("DebugBreakProcess failed:")
                    + QLatin1Char(' ') + winErrorMessage(GetLastError()), result);
            }
        } else if (si == Win32Interrupt || si == Win64Interrupt) {
            QString executable = QCoreApplication::applicationDirPath();
            executable += si == Win32Interrupt
                    ? QLatin1String("/win32interrupt.exe")
                    : QLatin1String("/win64interrupt.exe");
            if (!QFileInfo::exists(executable)) {
                result = appendCannotInterruptError(pid,
                                         Tr::tr("%1 does not exist. If you built %2 "
                                                "yourself, check out https://code.qt.io/cgit/"
                                                "qt-creator/binary-artifacts.git/.")
                                             .arg(QDir::toNativeSeparators(executable),
                                                  QGuiApplication::applicationDisplayName()), result);
            }
            switch (QProcess::execute(executable, QStringList(QString::number(pid)))) {
            case -2:
                return appendCannotInterruptError(pid, Tr::tr(
                            "Cannot start %1. Check src\\tools\\win64interrupt\\win64interrupt.c "
                            "for more information.").arg(QDir::toNativeSeparators(executable)), result);
            case 0:
                break;
            default:
                return appendCannotInterruptError(pid, QDir::toNativeSeparators(executable)
                            + QLatin1Char(' ') + Tr::tr("could not break the process."), result);
                break;
            }
        }
    } while (false);
    if (inferior != NULL)
        CloseHandle(inferior);
    return result;
#else
    if (pid <= 0)
        return appendCannotInterruptError(pid, Tr::tr("Invalid process id."));
    else if (kill(pid, SIGINT))
        return appendCannotInterruptError(pid, QString::fromLocal8Bit(strerror(errno)));
    return ResultOk;
#endif // Q_OS_WIN
}

} // namespace ProjectExplorer
