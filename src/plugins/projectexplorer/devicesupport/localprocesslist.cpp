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

#include "localprocesslist.h"

#include <utils/synchronousprocess.h>

#include <QLibrary>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <QProcess>
#include <QDir>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
// Enable Win API of XP SP1 and later
#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <utils/winutils.h>
#include <tlhelp32.h>
#include <psapi.h>
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif
#endif

namespace ProjectExplorer {
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

LocalProcessList::LocalProcessList(const IDevice::ConstPtr &device, QObject *parent)
        : DeviceProcessList(device, parent)
        , m_myPid(GetCurrentProcessId())
{
}

QList<DeviceProcess> LocalProcessList::getLocalProcesses()
{
    QList<DeviceProcess> processes;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return processes;

    for (bool hasNext = Process32First(snapshot, &pe); hasNext; hasNext = Process32Next(snapshot, &pe)) {
        DeviceProcess p;
        p.pid = pe.th32ProcessID;
        // Image has the absolute path, but can fail.
        const QString image = imageName(pe.th32ProcessID);
        p.exe = p.cmdLine = image.isEmpty() ?
            QString::fromWCharArray(pe.szExeFile) :
            image;
        processes << p;
    }
    CloseHandle(snapshot);
    return processes;
}

void LocalProcessList::doKillProcess(const DeviceProcess &process)
{
    const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
            |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
            |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
    m_error.clear();
    if (const HANDLE handle = OpenProcess(rights, FALSE, process.pid)) {
        if (!TerminateProcess(handle, UINT(-1))) {
          m_error = tr("Cannot terminate process %1: %2").
                    arg(process.pid).arg(Utils::winErrorMessage(GetLastError()));
        }
        CloseHandle(handle);
    } else {
        m_error = tr("Cannot open process %1: %2").
                  arg(process.pid).arg(Utils::winErrorMessage(GetLastError()));
    }
    QTimer::singleShot(0, this, SLOT(reportDelayedKillStatus()));
}

#endif //Q_OS_WIN


#ifdef Q_OS_UNIX
LocalProcessList::LocalProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : DeviceProcessList(device, parent)
    , m_myPid(getpid())
{}

static bool isUnixProcessId(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i)
        if (!procname.at(i).isDigit())
            return false;
    return true;
}

// Determine UNIX processes by reading "/proc". Default to ps if
// it does not exist

static const char procDirC[] = "/proc/";

static QList<DeviceProcess> getLocalProcessesUsingProc(const QDir &procDir)
{
    QList<DeviceProcess> processes;
    const QString procDirPath = QLatin1String(procDirC);
    const QStringList procIds = procDir.entryList();
    foreach (const QString &procId, procIds) {
        if (!isUnixProcessId(procId))
            continue;
        DeviceProcess proc;
        proc.pid = procId.toInt();
        const QString root = procDirPath + procId;

        QFile exeFile(root + QLatin1String("/exe"));
        proc.exe = exeFile.symLinkTarget();

        QFile cmdLineFile(root + QLatin1String("/cmdline"));
        if (cmdLineFile.open(QIODevice::ReadOnly)) { // process may have exited
            QList<QByteArray> tokens = cmdLineFile.readAll().split('\0');
            if (!tokens.isEmpty()) {
                if (proc.exe.isEmpty())
                    proc.exe = QString::fromLocal8Bit(tokens.front());
                foreach (const QByteArray &t,  tokens) {
                    if (!proc.cmdLine.isEmpty())
                        proc.cmdLine.append(QLatin1Char(' '));
                    proc.cmdLine.append(QString::fromLocal8Bit(t));
                }
            }
        }

        if (proc.exe.isEmpty()) {
            QFile statFile(root + QLatin1String("/stat"));
            if (!statFile.open(QIODevice::ReadOnly)) {
                const QStringList data = QString::fromLocal8Bit(statFile.readAll()).split(QLatin1Char(' '));
                proc.exe = data.at(1);
                proc.cmdLine = data.at(1); // PPID is element 3
                if (proc.exe.startsWith(QLatin1Char('(')) && proc.exe.endsWith(QLatin1Char(')'))) {
                    proc.exe.truncate(proc.exe.size() - 1);
                    proc.exe.remove(0, 1);
                }
            }
        }
        if (!proc.exe.isEmpty())
            processes.push_back(proc);
    }
    return processes;
}

// Determine UNIX processes by running ps
static QList<DeviceProcess> getLocalProcessesUsingPs()
{
#ifdef Q_OS_MAC
    static const char formatC[] = "pid state command";
#else
    static const char formatC[] = "pid,state,cmd";
#endif
    QList<DeviceProcess> processes;
    QProcess psProcess;
    QStringList args;
    args << QLatin1String("-e") << QLatin1String("-o") << QLatin1String(formatC);
    psProcess.start(QLatin1String("ps"), args);
    if (psProcess.waitForStarted()) {
        QByteArray output;
        if (Utils::SynchronousProcess::readDataFromProcess(psProcess, 30000, &output, 0, false)) {
            // Split "457 S+   /Users/foo.app"
            const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
            const int lineCount = lines.size();
            const QChar blank = QLatin1Char(' ');
            for (int l = 1; l < lineCount; l++) { // Skip header
                const QString line = lines.at(l).simplified();
                const int pidSep = line.indexOf(blank);
                const int cmdSep = pidSep != -1 ? line.indexOf(blank, pidSep + 1) : -1;
                if (cmdSep > 0) {
                    DeviceProcess procData;
                    procData.pid = line.left(pidSep).toInt();
                    procData.exe = line.mid(cmdSep + 1);
                    procData.cmdLine = line.mid(cmdSep + 1);
                    processes.push_back(procData);
                }
            }
        }
    }
    return processes;
}

QList<DeviceProcess> LocalProcessList::getLocalProcesses()
{
    const QDir procDir = QDir(QLatin1String(procDirC));
    return procDir.exists() ? getLocalProcessesUsingProc(procDir) : getLocalProcessesUsingPs();
}

void LocalProcessList::doKillProcess(const DeviceProcess &process)
{
    if (kill(process.pid, SIGKILL) == -1)
        m_error = QString::fromLocal8Bit(strerror(errno));
    else
        m_error.clear();
    QTimer::singleShot(0, this, SLOT(reportDelayedKillStatus()));
}

#endif // QT_OS_UNIX

Qt::ItemFlags LocalProcessList::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = DeviceProcessList::flags(index);
    if (index.isValid() && at(index.row()).pid == m_myPid)
        flags &= ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    return flags;
}

void LocalProcessList::handleUpdate()
{
    reportProcessListUpdated(getLocalProcesses());
}

void LocalProcessList::doUpdate()
{
    QTimer::singleShot(0, this, SLOT(handleUpdate()));
}

void LocalProcessList::reportDelayedKillStatus()
{
    if (m_error.isEmpty())
        reportProcessKilled();
    else
        reportError(m_error);
}

} // namespace Internal
} // namespace ProjectExplorer
