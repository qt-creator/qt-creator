/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

//
// WinANSI - An ANSI implementation for the modern Windows
//
// Copyright (C) 2008- Marius Storm-Olsen <mstormo@gmail.com>
//
// Based on work by Robert Kuster in article
//   http://software.rkuster.com/articles/winspy.htm
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// -------------------------------------------------------------------------------------------------

#include "sharedlibraryinjector.h"
#include <utils/winutils.h>

#include <QtCore/QDebug>

#ifdef __GNUC__   // MinGW does not have a complete windows.h

typedef DWORD (__stdcall *PTHREAD_START_ROUTINE) (LPVOID lpThreadParameter);

#endif

enum { debug = 0 };

static QString msgFuncFailed(const char *f, unsigned long error)
{
    return QString::fromLatin1("%1 failed: %2").arg(QLatin1String(f), Utils::winErrorMessage(error));
}

// Resolve a symbol from a library handle
template <class SymbolType>
inline bool resolveSymbol(const char *libraryName, HMODULE libraryHandle, const char *symbolName, SymbolType *s, QString *errorMessage)
{
    *s = 0;
    FARPROC WINAPI vs = ::GetProcAddress(libraryHandle, symbolName);
    if (vs == 0) {
        *errorMessage = QString::fromLatin1("Unable to resolve '%2' in '%1'.").arg(QString::fromAscii(symbolName), QString::fromAscii(libraryName));
        return false;
    }
    *s = reinterpret_cast<SymbolType>(vs);
    return true;
}

// Resolve a symbol from a library
template <class SymbolType>
inline bool resolveSymbol(const char *library, const char *symbolName, SymbolType *s, QString *errorMessage)
{
    *s = 0;
    const HMODULE hm = ::GetModuleHandleA(library);
    if (hm == NULL) {
        *errorMessage = QString::fromLatin1("Module '%1' does not exist.").arg(QString::fromAscii(library));
        return false;
    }
    return resolveSymbol(library, hm , symbolName, s, errorMessage);
}


namespace Debugger {
namespace Internal {

SharedLibraryInjector::SharedLibraryInjector(unsigned long processId,
                                             unsigned long threadId) :
    m_processId(processId),
    m_threadId(threadId),
    m_hasEscalatedPrivileges(false)
{
}

SharedLibraryInjector::~SharedLibraryInjector()
{
}

void SharedLibraryInjector::setPid(unsigned long pid)
{
    m_processId = pid;
}

void SharedLibraryInjector::setThreadId(unsigned long tid)
{
    m_threadId  = tid;
}

void SharedLibraryInjector::setModulePath(const QString &modulePath)
{
    m_modulePath = modulePath;
}

bool SharedLibraryInjector::remoteInject(const QString &modulePath,
                                         bool waitForThread, QString *errorMessage)
{
    setModulePath(modulePath);
    return doRemoteInjection(m_processId, NULL, m_modulePath, waitForThread, errorMessage);
}

bool SharedLibraryInjector::stubInject(const QString &modulePath, unsigned long entryPoint, QString *errorMessage)
{
    setModulePath(modulePath);
    return doStubInjection(m_processId, m_modulePath, entryPoint, errorMessage);
}

bool SharedLibraryInjector::unload(HMODULE hFreeModule, QString *errorMessage)
{
    return doRemoteInjection(m_processId, hFreeModule, QString(), true, errorMessage);  // Always use remote thread to unload
}

bool SharedLibraryInjector::unload(const QString &modulePath, QString *errorMessage)
{
    const HMODULE hMod = modulePath.isEmpty() ?
                         findModuleHandle(m_modulePath, errorMessage) :
                         findModuleHandle(modulePath, errorMessage);
    if (!hMod)
        return false;

    return doRemoteInjection(m_processId, hMod, NULL, true, errorMessage); // Always use remote thread to unload
}

bool SharedLibraryInjector::hasLoaded(const QString &modulePath)
{
    QString errorMessage;
    return findModuleHandle(modulePath.isEmpty() ? m_modulePath : modulePath, &errorMessage) != NULL;
}

QString SharedLibraryInjector::findModule(const QString &moduleName)
{
    const TCHAR *moduleNameC = reinterpret_cast<const TCHAR*>(moduleName.utf16());
    if (GetFileAttributesW(moduleNameC) != INVALID_FILE_ATTRIBUTES)
        return moduleName;

    TCHAR testpathC[MAX_PATH];
    // Check application path first
    GetModuleFileNameW(NULL, testpathC, MAX_PATH);
    QString testPath = QString::fromUtf16(reinterpret_cast<unsigned short*>(testpathC));
    const int lastSlash = testPath.lastIndexOf(QLatin1Char('\\'));
    if (lastSlash != -1)
        testPath.truncate(lastSlash + 1);
    testPath += moduleName;
    if (GetFileAttributesW(reinterpret_cast<const TCHAR*>(testPath.utf16())) != INVALID_FILE_ATTRIBUTES)
        return testPath;
    // Path Search
    if (SearchPathW(NULL, reinterpret_cast<const TCHAR*>(moduleName.utf16()), NULL, sizeof(testpathC)/2, testpathC, NULL))
        return QString::fromUtf16(reinterpret_cast<unsigned short*>(testpathC));
    // Last chance, if the module has already been loaded in this process, then use that path
    const HMODULE loadedModule = GetModuleHandleW(reinterpret_cast<const TCHAR*>(moduleName.utf16()));
    if (loadedModule) {
        GetModuleFileNameW(loadedModule, testpathC, sizeof(testpathC));
        if (GetFileAttributes(testpathC) != INVALID_FILE_ATTRIBUTES)
            return QString::fromUtf16(reinterpret_cast<unsigned short*>(testpathC));
    }
    return QString();
}

unsigned long SharedLibraryInjector::getModuleEntryPoint(const QString &moduleName)
{
    // If file doesn't exist, just treat it like we cannot figure out the entry point
    if (moduleName.isEmpty() || GetFileAttributesW(reinterpret_cast<const TCHAR*>(moduleName.utf16())) == INVALID_FILE_ATTRIBUTES)
        return 0;

    // Read the first 1K of data from the file
    unsigned char peData[1024];
    unsigned long peDataSize = 0;
    const HANDLE hFile = CreateFileW(reinterpret_cast<const WCHAR*>(moduleName.utf16()), FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE
        || !ReadFile(hFile, peData, sizeof(peData), &peDataSize, NULL))
        return 0;
    CloseHandle(hFile);

    // Now we check to see if there is an optional header we can read
    IMAGE_DOS_HEADER *dosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(peData);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE   // DOS signature is incorrect, or
        || dosHeader->e_lfanew == 0)                // executable's PE data contains no offset to New EXE header
        return 0;

    IMAGE_NT_HEADERS *ntHeaders = (IMAGE_NT_HEADERS *)(peData + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE  // NT signature is incorrect, or
        || !ntHeaders->OptionalHeader.ImageBase     // ImageBase or EntryPoint addresses are incorrect
        || !ntHeaders->OptionalHeader.AddressOfEntryPoint)
        return 0;

    return ntHeaders->OptionalHeader.ImageBase
           + ntHeaders->OptionalHeader.AddressOfEntryPoint;
}

bool SharedLibraryInjector::escalatePrivileges(QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << m_hasEscalatedPrivileges;

    if (m_hasEscalatedPrivileges)
        return true;

    bool success = false;
    HANDLE hToken = 0;
    do {
        TOKEN_PRIVILEGES Debug_Privileges;
        if (!LookupPrivilegeValue (NULL, SE_DEBUG_NAME, &Debug_Privileges.Privileges[0].Luid)) {
            *errorMessage = msgFuncFailed("LookupPrivilegeValue", GetLastError());
            break;
        }

        Debug_Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; // set to enable privilege
        Debug_Privileges.PrivilegeCount = 1;                              // working with only 1

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
            *errorMessage = msgFuncFailed("OpenProcessToken", GetLastError());
            break;
        }
        if (!AdjustTokenPrivileges(hToken, FALSE, &Debug_Privileges, 0,  NULL, NULL)) {
            *errorMessage = msgFuncFailed("AdjustTokenPrivileges", GetLastError());
            break;

        }
        success = true;
    } while (false);

    if (hToken)
        CloseHandle (hToken);

    m_hasEscalatedPrivileges = success;
    return success;
}

static void *writeDataToProcess(HANDLE process, void *data, unsigned size, QString *errorMessage)
{
    void *memory = VirtualAllocEx(process, NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (!memory) {
        *errorMessage = msgFuncFailed("VirtualAllocEx", GetLastError());
        return 0;
    }
    if (!WriteProcessMemory(process, memory, data, size, NULL)) {
        *errorMessage = msgFuncFailed("WriteProcessMemory", GetLastError());
        return 0;
    }
    return memory;
}

static void *writeUtf16StringToProcess(HANDLE process, const QString &what, QString *errorMessage)
{
    QByteArray whatData = QByteArray(reinterpret_cast<const char*>(what.utf16()), what.size() * 2);
    whatData += '\0';
    whatData += '\0';
    return writeDataToProcess(process, whatData.data(), whatData.size(), errorMessage);
}

bool SharedLibraryInjector::doStubInjection(unsigned long pid,
                                           const QString &modulePath,
                                           unsigned long entryPoint,
                                           QString *errorMessage)
{
    if (debug)
        qDebug() << pid << modulePath << entryPoint;
    if (modulePath.isEmpty())
        return false;

    if (!escalatePrivileges(errorMessage))
        return false;
//  MinGW lacks  OpenThread() and the advapi.lib as of 6.5.2009
#if (defined(WIN64) || defined(_WIN64) || defined(__WIN64__)) || defined(__GNUC__)
    *errorMessage = QLatin1String("Not implemented for this architecture.");
    return false;
#else
    byte stubCode[] = {
        0x68, 0, 0, 0, 0,   // push 0x00000000      - Placeholder for the entrypoint address
        0x9C,               // pushfd               - Save the flags and registers
        0x60,               // pushad
        0x68, 0, 0, 0, 0,   // push 0x00000000      - Placeholder for the string address
        0xB8, 0, 0, 0, 0,   // mov eax, 0x00000000  - Placeholder for the LoadLibrary address
        0xFF, 0xD0,         // call eax             - Call LoadLibrary with string address
        0x61,               // popad                - Restore flags and registry
        0x9D,               // popfd
        0xC3                // retn                 - Return (pops the entrypoint, and executes)
    };

    const HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
        return false;

    const HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, false, m_threadId);
    if (!hThread) {
        CloseHandle(hProcess);
        return false;
    }

    // Get address of LoadLibraryW in kernel32.dll
    unsigned long funcPtr;
    if (!resolveSymbol("kernel32.dll", "LoadLibraryW", &funcPtr, errorMessage))
        return false;

    // Allocate and write DLL path to target process' memory
    // 0-terminated Utf16 string.
    void *dllString = writeUtf16StringToProcess(hProcess, modulePath, errorMessage);
    if (!dllString)
        return false;

    // Allocate memory in target process, for the stubCode which will load the module, and execute
    // the process' entry-point
    const unsigned long stubLen = sizeof(stubCode);
    // Modify the stubCode to have the proper entry-point, DLL module string, and function pointer
    *(unsigned long*)(stubCode+1)  = entryPoint;
    *(unsigned long*)(stubCode+8)  = reinterpret_cast<unsigned long>(dllString);
    *(unsigned long*)(stubCode+13) = funcPtr;

    // If we cannot write the stubCode into the process, we simply bail out, and the process will
    // continues execution as normal
    void *stub = writeDataToProcess(hProcess, stubCode, stubLen, errorMessage);
    if (!stub)
        return false;
    // Set the process' main thread to start executing from the stubCode address which we've just
    // allocated in the process' memory.. If we cannot do that, bail out
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL;
    if(!GetThreadContext(hThread, &ctx))
        return false;
    ctx.Eip = reinterpret_cast<DWORD>(stub);
    ctx.ContextFlags = CONTEXT_CONTROL;
    if(!SetThreadContext(hThread, &ctx))
        return false;

    CloseHandle(hProcess);
    CloseHandle(hThread);
    return true;
#endif
}

// -------------------------------------------------------------------------------------------------

bool SharedLibraryInjector::doRemoteInjection(unsigned long pid,
                                             HMODULE hFreeModule,
                                             const QString &modulePath,
                                             bool waitForThread,
                                             QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << '\n' << hFreeModule << pid << waitForThread<< modulePath;

    if (!hFreeModule && modulePath.isEmpty())
        return false;

    escalatePrivileges(errorMessage);

    const HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
        *errorMessage = msgFuncFailed("OpenProcess", GetLastError());
        qDebug() << "Failed to open process PID %d with all access\n" << pid;
        return false;
    }

    void *pszLibFileRemote = 0;
    HANDLE hThread = 0;
    // Call  "FreeLibrary(hFreeModule)" to unload
    if (hFreeModule) {
        PTHREAD_START_ROUTINE pfnThreadRtn;
        if (!resolveSymbol("Kernel32", "FreeLibrary", &pfnThreadRtn, errorMessage))
            return false;
        hThread = ::CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, hFreeModule, 0, NULL);
    } else {
        pszLibFileRemote = writeUtf16StringToProcess(hProcess, modulePath, errorMessage);
        if (!pszLibFileRemote)
            return false;
        // Loadlibrary routine
        PTHREAD_START_ROUTINE pfnThreadRtn;
        if (!resolveSymbol("Kernel32",  "LoadLibraryW", &pfnThreadRtn, errorMessage))
            return false;
        hThread = ::CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
    }
    if (hThread == NULL) {
        *errorMessage = msgFuncFailed("CreateRemoteThread", GetLastError());
        return false;
    }

    if (!SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST)) {
        *errorMessage = msgFuncFailed("SetThreadPriority", GetLastError());
        return false;
    }

    if (waitForThread) {
        if (::WaitForSingleObject(hThread, 20000) != WAIT_OBJECT_0) {
            *errorMessage = QString::fromLatin1("WaitForSingleObject timeout");
            ::CloseHandle(hThread);
            ::CloseHandle(hProcess);
            return false;
        }

        if (pszLibFileRemote)
           ::VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);
    }

    if (hThread)
        ::CloseHandle(hThread);

    if (hProcess)
        ::CloseHandle(hProcess);
    if (debug)
        qDebug() << "success" << Q_FUNC_INFO;
    return true;
}

typedef BOOL (WINAPI *PFNENUMPROCESSMODULES) (HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
typedef DWORD (WINAPI *PFNGETMODULEFILENAMEEXW) (HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);

// We will require this function to get a module handle of our original module
HMODULE SharedLibraryInjector::findModuleHandle(const QString &modulePath, QString *errorMessage)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << modulePath;

    if (!escalatePrivileges(errorMessage))
        return 0;

    HMODULE hMods[1024];
    DWORD cbNeeded;

    HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processId);
    if (hProcess == NULL)
        return 0;

    const char *psAPI_Lib = "PSAPI.DLL";
    HMODULE m_hModPSAPI = ::LoadLibraryA(psAPI_Lib);
    PFNENUMPROCESSMODULES m_pfnEnumProcessModules;
    PFNGETMODULEFILENAMEEXW m_pfnGetModuleFileNameExW;
    if (!resolveSymbol(psAPI_Lib, m_hModPSAPI, "EnumProcessModules", &m_pfnEnumProcessModules, errorMessage)
        || !resolveSymbol(psAPI_Lib, m_hModPSAPI, "GetModuleFileNameExW", &m_pfnGetModuleFileNameExW, errorMessage))
        return false;

    if(m_pfnEnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        const unsigned count = cbNeeded / sizeof(HMODULE);
        for (unsigned i = 0; i < count; i++) {
            TCHAR szModName[MAX_PATH];
            if (m_pfnGetModuleFileNameExW(hProcess, hMods[i], szModName, sizeof(szModName))) {
                if (QString::fromUtf16(reinterpret_cast<const unsigned short *>(szModName)) == modulePath) {
                    ::FreeLibrary(m_hModPSAPI);
                    ::CloseHandle(hProcess);
                    return hMods[i];
                }
            }
        }
    }

    ::CloseHandle(hProcess);
    return 0;
}

} // namespace Internal
} // namespace Debugger
