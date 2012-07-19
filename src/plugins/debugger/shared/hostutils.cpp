/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "hostutils.h"
#include "breakpoint.h"

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

// Resolve QueryFullProcessImageNameW out of kernel32.dll due
// to incomplete MinGW import libs and it not being present
// on Windows XP.
static BOOL queryFullProcessImageName(HANDLE h, DWORD flags, LPWSTR buffer, DWORD *size)
{
    // Resolve required symbols from the kernel32.dll
    typedef BOOL (WINAPI *QueryFullProcessImageNameWProtoType)
                 (HANDLE, DWORD, LPWSTR, PDWORD);
    static QueryFullProcessImageNameWProtoType queryFullProcessImageNameW = 0;
    if (!queryFullProcessImageNameW) {
        QLibrary kernel32Lib(QLatin1String("kernel32.dll"), 0);
        if (kernel32Lib.isLoaded() || kernel32Lib.load())
            queryFullProcessImageNameW = (QueryFullProcessImageNameWProtoType)kernel32Lib.resolve("QueryFullProcessImageNameW");
    }
    if (!queryFullProcessImageNameW)
        return FALSE;
    // Read out process
    return (*queryFullProcessImageNameW)(h, flags, buffer, size);
}

static QString imageName(DWORD processId)
{
    QString  rc;
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION , FALSE, processId);
    if (handle == INVALID_HANDLE_VALUE)
        return rc;
    WCHAR buffer[MAX_PATH];
    DWORD bufSize = MAX_PATH;
    if (queryFullProcessImageName(handle, 0, buffer, &bufSize))
        rc = QString::fromUtf16(reinterpret_cast<const ushort*>(buffer));
    CloseHandle(handle);
    return rc;
}

static QList<ProcData> winProcessList()
{
    QList<ProcData> rc;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return rc;

    for (bool hasNext = Process32First(snapshot, &pe); hasNext; hasNext = Process32Next(snapshot, &pe)) {
        ProcData procData;
        procData.ppid = QString::number(pe.th32ProcessID);
        procData.name = QString::fromUtf16(reinterpret_cast<ushort*>(pe.szExeFile));
        procData.image = imageName(pe.th32ProcessID);
        rc.push_back(procData);
    }
    CloseHandle(snapshot);
    return rc;
}

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

unsigned long winGetCurrentProcessId()
{
    return GetCurrentProcessId();
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
    if (flags == EXCEPTION_NONCONTINUABLE) {
        str << " (execution cannot be continued)";
    }
    str.setIntegerBase(10);
}

bool isDebuggerWinException(long code)
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

QList<ProcData> hostProcessList()
{
    return winProcessList();
}

#else // Q_OS_WIN

static bool isUnixProcessId(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i)
        if (!procname.at(i).isDigit())
            return false;
    return true;
}

// Determine UNIX processes by running ps
static QList<ProcData> unixProcessListPS()
{
#ifdef Q_OS_MAC
    static const char formatC[] = "pid state command";
#else
    static const char formatC[] = "pid,state,cmd";
#endif
    QList<ProcData> rc;
    QProcess psProcess;
    QStringList args;
    args << QLatin1String("-e") << QLatin1String("-o") << QLatin1String(formatC);
    psProcess.start(QLatin1String("ps"), args);
    if (!psProcess.waitForStarted())
        return rc;
    QByteArray output;
    if (!Utils::SynchronousProcess::readDataFromProcess(psProcess, 30000, &output, 0, false))
        return rc;
    // Split "457 S+   /Users/foo.app"
    const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
    const int lineCount = lines.size();
    const QChar blank = QLatin1Char(' ');
    for (int l = 1; l < lineCount; l++) { // Skip header
        const QString line = lines.at(l).simplified();
        const int pidSep = line.indexOf(blank);
        const int cmdSep = pidSep != -1 ? line.indexOf(blank, pidSep + 1) : -1;
        if (cmdSep > 0) {
            ProcData procData;
            procData.ppid = line.left(pidSep);
            procData.state = line.mid(pidSep + 1, cmdSep - pidSep - 1);
            procData.name = line.mid(cmdSep + 1);
            rc.push_back(procData);
        }
    }
    return rc;
}

// Determine UNIX processes by reading "/proc". Default to ps if
// it does not exist
static QList<ProcData> unixProcessList()
{
    const QDir procDir(QLatin1String("/proc/"));
    if (!procDir.exists())
        return unixProcessListPS();
    QList<ProcData> rc;
    const QStringList procIds = procDir.entryList();
    if (procIds.isEmpty())
        return rc;
    foreach (const QString &procId, procIds) {
        if (!isUnixProcessId(procId))
            continue;
        QString filename = QLatin1String("/proc/");
        filename += procId;
        filename += QLatin1String("/stat");
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            continue;           // process may have exited

        const QStringList data = QString::fromLocal8Bit(file.readAll()).split(QLatin1Char(' '));
        ProcData proc;
        proc.ppid = procId;
        proc.name = data.at(1);
        if (proc.name.startsWith(QLatin1Char('(')) && proc.name.endsWith(QLatin1Char(')'))) {
            proc.name.truncate(proc.name.size() - 1);
            proc.name.remove(0, 1);
        }
        proc.state = data.at(2);
        // PPID is element 3
        rc.push_back(proc);
    }
    return rc;
}

QList<ProcData> hostProcessList()
{
    return unixProcessList();
}

#endif // Q_OS_WIN

} // namespace Internal
} // namespace Debugger
