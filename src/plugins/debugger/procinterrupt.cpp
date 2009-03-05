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

#include "procinterrupt.h"

#if defined(Q_OS_WIN)

#include <windows.h>

using namespace Debugger::Internal;

typedef HANDLE (WINAPI *PtrCreateRemoteThread)(
    HANDLE hProcess,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    SIZE_T dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId);

PtrCreateRemoteThread resolveCreateRemoteThread()
{
    HINSTANCE hLib = LoadLibraryA("Kernel32");
    return (PtrCreateRemoteThread)GetProcAddress(hLib, "CreateRemoteThread");
}

bool Debugger::Internal::interruptProcess(int pID)
{
    DWORD pid = pID;
    if (!pid)
        return false;

    PtrCreateRemoteThread libFunc = resolveCreateRemoteThread();
    if (libFunc) {
        DWORD dwThreadId = 0;
        HANDLE hproc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        HANDLE hthread = libFunc(hproc, NULL, 0, (LPTHREAD_START_ROUTINE)DebugBreak, 0, 0, &dwThreadId);
        CloseHandle(hthread);
        if (dwThreadId)
            return true;
    }
    
    return false;
}

#endif // defined(Q_OS_WIN)



#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)

#include <sys/types.h>
#include <signal.h>

using namespace Debugger::Internal;

bool Debugger::Internal::interruptProcess(int pID)
{
    int procId = pID;
    if (procId != -1) {
        if (kill(procId, SIGINT) == 0)
            return true;
    }
    return false;
}

#endif // defined(Q_OS_LINUX) || defined(Q_OS_MAC)
