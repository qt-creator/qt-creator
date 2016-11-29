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
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <utils/winutils.h>
#include <tlhelp32.h>
#include <psapi.h>
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

QList<DeviceProcessItem> LocalProcessList::getLocalProcesses()
{
    QList<DeviceProcessItem> processes;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return processes;

    for (bool hasNext = Process32First(snapshot, &pe); hasNext; hasNext = Process32Next(snapshot, &pe)) {
        DeviceProcessItem p;
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

static QList<DeviceProcessItem> getLocalProcessesUsingProc(const QDir &procDir)
{
    QList<DeviceProcessItem> processes;
    const QString procDirPath = QLatin1String(procDirC);
    const QStringList procIds = procDir.entryList();
    foreach (const QString &procId, procIds) {
        if (!isUnixProcessId(procId))
            continue;
        DeviceProcessItem proc;
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
                if (data.size() < 2)
                    continue;
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
static QList<DeviceProcessItem> getLocalProcessesUsingPs()
{
    QList<DeviceProcessItem> processes;
    QProcess psProcess;
    QStringList args;
    args << QLatin1String("-e") << QLatin1String("-o") << QLatin1String("pid,comm,args");
    psProcess.start(QLatin1String("ps"), args);
    if (psProcess.waitForStarted()) {
        QByteArray output;
        if (Utils::SynchronousProcess::readDataFromProcess(psProcess, 30000, &output, 0, false)) {
            // Split "457 /Users/foo.app arg1 arg2"
            const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
            const int lineCount = lines.size();
            const QChar blank = QLatin1Char(' ');
            for (int l = 1; l < lineCount; l++) { // Skip header
                const QString line = lines.at(l).simplified();
                const int pidSep = line.indexOf(blank);
                const int cmdSep = pidSep != -1 ? line.indexOf(blank, pidSep + 1) : -1;
                if (cmdSep > 0) {
                    const int argsSep = line.indexOf(blank, cmdSep + 1);
                    DeviceProcessItem procData;
                    procData.pid = line.leftRef(pidSep).toInt();
                    procData.cmdLine = line.mid(cmdSep + 1);
                    if (argsSep == -1)
                        procData.exe = line.mid(cmdSep + 1);
                    else
                        procData.exe = line.mid(cmdSep + 1, argsSep - cmdSep -1);
                    processes.push_back(procData);
                }
            }
        }
    }
    return processes;
}

QList<DeviceProcessItem> LocalProcessList::getLocalProcesses()
{
    const QDir procDir = QDir(QLatin1String(procDirC));
    return procDir.exists() ? getLocalProcessesUsingProc(procDir) : getLocalProcessesUsingPs();
}

#endif // QT_OS_UNIX

void LocalProcessList::doKillProcess(const DeviceProcessItem &process)
{
    DeviceProcessSignalOperation::Ptr signalOperation = device()->signalOperation();
    connect(signalOperation.data(), &DeviceProcessSignalOperation::finished,
            this, &LocalProcessList::reportDelayedKillStatus);
    signalOperation->killProcess(process.pid);
}

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
    QTimer::singleShot(0, this, &LocalProcessList::handleUpdate);
}

void LocalProcessList::reportDelayedKillStatus(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        reportProcessKilled();
    else
        reportError(errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer
