/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "winutils.h"
#include "dbgwinutils.h"
#include "debuggerdialogs.h"
#include "breakpoint.h"

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QTextStream>

// Enable Win API of XP SP1 and later
#ifdef Q_OS_WIN
#    define _WIN32_WINNT 0x0502
#    include <windows.h>
#    include <utils/winutils.h>
#    if !defined(PROCESS_SUSPEND_RESUME) // Check flag for MinGW
#        define PROCESS_SUSPEND_RESUME (0x0800)
#    endif // PROCESS_SUSPEND_RESUME
#endif // Q_OS_WIN

#include <tlhelp32.h>
#include <psapi.h>
#include <QtCore/QLibrary>

namespace Debugger {
namespace Internal {

// Resolve QueryFullProcessImageNameW out of kernel32.dll due
// to incomplete MinGW import libs and it not being present
// on Windows XP.
static inline BOOL queryFullProcessImageName(HANDLE h,
                                                   DWORD flags,
                                                   LPWSTR buffer,
                                                   DWORD *size)
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

static inline QString imageName(DWORD processId)
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

QList<ProcData> winProcessList()
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

// Open the process and break into it
bool winDebugBreakProcess(unsigned long  pid, QString *errorMessage)
{
    bool ok = false;
    HANDLE inferior = NULL;
    do {
        const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
                |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
                |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME ;
        inferior = OpenProcess(rights, FALSE, pid);
        if (inferior == NULL) {
            *errorMessage = QString::fromLatin1("Cannot open process %1: %2").
                    arg(pid).arg(Utils::winErrorMessage(GetLastError()));
            break;
        }
        if (!DebugBreakProcess(inferior)) {
            *errorMessage = QString::fromLatin1("DebugBreakProcess failed: %1").arg(Utils::winErrorMessage(GetLastError()));
            break;
        }
        ok = true;
    } while (false);
    if (inferior != NULL)
        CloseHandle(inferior);
    return ok;
}

unsigned long winGetCurrentProcessId()
{
    return GetCurrentProcessId();
}

// Helper for normalizing file names:
// Map the device paths in  a file name to back to drive letters
// "/Device/HarddiskVolume1/file.cpp" -> "C:/file.cpp"

static bool mapDeviceToDriveLetter(QString *s)
{
    enum { bufSize = 512 };
    // Retrieve drive letters and get their device names.
    // Do not cache as it may change due to removable/network drives.
    TCHAR driveLetters[bufSize];
    if (!GetLogicalDriveStrings(bufSize-1, driveLetters))
        return false;

    TCHAR driveName[MAX_PATH];
    TCHAR szDrive[3] = TEXT(" :");
    for (const TCHAR *driveLetter = driveLetters; *driveLetter; driveLetter++) {
        szDrive[0] = *driveLetter; // Look up each device name
        if (QueryDosDevice(szDrive, driveName, MAX_PATH)) {
            const QString deviceName = QString::fromWCharArray(driveName);
            if (s->startsWith(deviceName)) {
                s->replace(0, deviceName.size(), QString::fromWCharArray(szDrive));
                return true;
            }
        }
    }
    return false;
}

// Determine normalized case of a Windows file name (camelcase.cpp -> CamelCase.cpp)
// Restriction: File needs to exists and be non-empty and will be to be opened/mapped.
// This is the MSDN-recommended way of doing that.

QString winNormalizeFileName(const QString &f)
{
    HANDLE hFile = CreateFile((const wchar_t*)f.utf16(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return f;
    // Get the file size. We need a non-empty file to map it.
    DWORD dwFileSizeHi = 0;
    DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);
    if (dwFileSizeLo == 0 && dwFileSizeHi == 0) {
        CloseHandle(hFile);
        return f;
    }
    // Create a file mapping object.
    HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
    if (!hFileMap)  {
        CloseHandle(hFile);
        return f;
    }

    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
    if (!pMem) {
        CloseHandle(hFileMap);
        CloseHandle(hFile);
        return f;
    }

    QString rc;
    WCHAR pszFilename[MAX_PATH];
    pszFilename[0] = 0;
    // Get a file name of the form "/Device/HarddiskVolume1/file.cpp"
    if (GetMappedFileName (GetCurrentProcess(), pMem, pszFilename, MAX_PATH)) {
        rc = QString::fromWCharArray(pszFilename);
        if (!mapDeviceToDriveLetter(&rc))
            rc.clear();
    }

    UnmapViewOfFile(pMem);
    CloseHandle(hFileMap);
    CloseHandle(hFile);
    return rc.isEmpty() ? f : rc;
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
    return code ==EXCEPTION_BREAKPOINT || code == EXCEPTION_SINGLE_STEP;
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

} // namespace Internal
} // namespace Debugger
