/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "winutils.h"
#include "debuggerdialogs.h"

#include <QtCore/QDebug>
#include <QtCore/QString>

#ifdef Q_OS_WIN
#    ifdef __GNUC__  // Required for OpenThread under MinGW
#        define _WIN32_WINNT 0x0502
#    endif // __GNUC__
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

} // namespace Internal
} // namespace Debugger
