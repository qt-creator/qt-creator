/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "procinterrupt.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <Tlhelp32.h>

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

DWORD findProcessId(DWORD parentId)
{
    HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(PROCESSENTRY32);

    DWORD procId = 0;

    BOOL moreProc = Process32First(hProcList, &procEntry);
    while (moreProc) {
        if (procEntry.th32ParentProcessID == parentId) {
            procId = procEntry.th32ProcessID;
            break;
        }
        moreProc = Process32Next(hProcList, &procEntry);
    }

    CloseHandle(hProcList);
    return procId;
}
#else

#include <QtCore/QLatin1String>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QFileInfoList>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>

#include <sys/types.h>
#include <signal.h>

#include <sys/sysctl.h>

#define OPProcessValueUnknown UINT_MAX

/* Mac OS X
int OPParentIDForProcessID(int pid)
    // Returns the parent process id for the given process id (pid)
{
    struct kinfo_proc info;
    size_t length = sizeof(struct kinfo_proc);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
    if (sysctl(mib, 4, &info, &length, NULL, 0) < 0)
        return OPProcessValueUnknown;
    if (length == 0)
        return OPProcessValueUnknown;
    return info.kp_eproc.e_ppid;
}
*/

int findParentProcess(int procId)
{
    QFile statFile(QLatin1String("/proc/") + QString::number(procId) + 
                   QLatin1String("/stat"));
    if (!statFile.open(QIODevice::ReadOnly))
        return -1;
    
    QByteArray line = statFile.readLine();
    line = line.mid(line.indexOf(')') + 4);
    //qDebug() << "1: " << line;
    line = line.left(line.indexOf(' '));
    //qDebug() << "2: " << line;
    
    return QString(line).toInt();
}

int findChildProcess(int parentId)
{
    QDir proc(QLatin1String("/proc"));
    QFileInfoList procList = proc.entryInfoList(QDir::Dirs);
    foreach (const QFileInfo &info, procList) {
        int procId = 0;
        bool ok = false;
        procId = info.baseName().toInt(&ok);
        if (!ok || !procId)
            continue;
        
        if (findParentProcess(procId) == parentId)
            return procId;
    }
    
    return -1;
}

#endif

bool Debugger::Internal::interruptProcess(int pID)
{
#ifdef Q_OS_WIN
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
#else
    int procId = pID;
    if (procId != -1) {
        if (kill(procId, SIGINT) == 0)
            return true;
    }

#endif
    
    return false;
}

bool Debugger::Internal::interruptChildProcess(Q_PID parentPID)
{
#ifdef WIN32
    DWORD pid = findProcessId(parentPID->dwProcessId);
    return interruptProcess(pid);
#else
    int procId = findChildProcess(parentPID);
    //qDebug() << "INTERRUPTING PROCESS" << procId;
    return interruptProcess(procId);
#endif
}
