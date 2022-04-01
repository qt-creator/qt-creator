/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "processinfo.h"

#include "qtcprocess.h"

#if defined(Q_OS_UNIX)
#include <QDir>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <utils/winutils.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif

namespace Utils {

bool ProcessInfo::operator<(const ProcessInfo &other) const
{
    if (processId != other.processId)
        return processId < other.processId;
    if (executable != other.executable)
        return executable < other.executable;
    return commandLine < other.commandLine;
}

#if defined(Q_OS_UNIX)

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

static QList<ProcessInfo> getLocalProcessesUsingProc()
{
    QList<ProcessInfo> processes;
    const QString procDirPath = QLatin1String(procDirC);
    const QDir procDir = QDir(QLatin1String(procDirC));
    const QStringList procIds = procDir.entryList();
    for (const QString &procId : procIds) {
        if (!isUnixProcessId(procId))
            continue;
        ProcessInfo proc;
        proc.processId = procId.toInt();
        const QString root = procDirPath + procId;

        const QFile exeFile(root + QLatin1String("/exe"));
        proc.executable = exeFile.symLinkTarget();

        QFile cmdLineFile(root + QLatin1String("/cmdline"));
        if (cmdLineFile.open(QIODevice::ReadOnly)) { // process may have exited
            const QList<QByteArray> tokens = cmdLineFile.readAll().split('\0');
            if (!tokens.isEmpty()) {
                if (proc.executable.isEmpty())
                    proc.executable = QString::fromLocal8Bit(tokens.front());
                for (const QByteArray &t : tokens) {
                    if (!proc.commandLine.isEmpty())
                        proc.commandLine.append(QLatin1Char(' '));
                    proc.commandLine.append(QString::fromLocal8Bit(t));
                }
            }
        }

        if (proc.executable.isEmpty()) {
            QFile statFile(root + QLatin1String("/stat"));
            if (statFile.open(QIODevice::ReadOnly)) {
                const QStringList data = QString::fromLocal8Bit(statFile.readAll()).split(QLatin1Char(' '));
                if (data.size() < 2)
                    continue;
                proc.executable = data.at(1);
                proc.commandLine = data.at(1); // PPID is element 3
                if (proc.executable.startsWith(QLatin1Char('(')) && proc.executable.endsWith(QLatin1Char(')'))) {
                    proc.executable.truncate(proc.executable.size() - 1);
                    proc.executable.remove(0, 1);
                }
            }
        }
        if (!proc.executable.isEmpty())
            processes.push_back(proc);
    }
    return processes;
}

// Determine UNIX processes by running ps
static QMap<qint64, QString> getLocalProcessDataUsingPs(const QString &column)
{
    QMap<qint64, QString> result;
    Utils::QtcProcess psProcess;
    psProcess.setCommand({"ps", {"-e", "-o", "pid," + column}});
    psProcess.start();
    if (psProcess.waitForStarted()) {
        QByteArray output;
        if (psProcess.readDataFromProcess(30, &output, nullptr, false)) {
            // Split "457 /Users/foo.app arg1 arg2"
            const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
            const int lineCount = lines.size();
            const QChar blank = QLatin1Char(' ');
            for (int l = 1; l < lineCount; l++) { // Skip header
                const QString line = lines.at(l).trimmed();
                const int pidSep = line.indexOf(blank);
                const qint64 pid = line.left(pidSep).toLongLong();
                result[pid] = line.mid(pidSep + 1);
            }
        }
    }
    return result;
}

static QList<ProcessInfo> getLocalProcessesUsingPs()
{
    QList<ProcessInfo> processes;

    // cmdLines are full command lines, usually with absolute path,
    // exeNames only the file part of the executable's path.
    const QMap<qint64, QString> exeNames = getLocalProcessDataUsingPs("comm");
    const QMap<qint64, QString> cmdLines = getLocalProcessDataUsingPs("args");

    for (auto it = exeNames.begin(), end = exeNames.end(); it != end; ++it) {
        const qint64 pid = it.key();
        if (pid <= 0)
            continue;
        const QString cmdLine = cmdLines.value(pid);
        if (cmdLines.isEmpty())
            continue;
        const QString exeName = it.value();
        if (exeName.isEmpty())
            continue;
        const int pos = cmdLine.indexOf(exeName);
        if (pos == -1)
            continue;
        processes.append({pid, cmdLine.left(pos + exeName.size()), cmdLine});
    }

    return processes;
}

QList<ProcessInfo> ProcessInfo::processInfoList()
{
    const QDir procDir = QDir(QLatin1String(procDirC));
    return procDir.exists() ? getLocalProcessesUsingProc() : getLocalProcessesUsingPs();
}

#elif defined(Q_OS_WIN)

QList<ProcessInfo> ProcessInfo::processInfoList()
{
    QList<ProcessInfo> processes;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return processes;

    for (bool hasNext = Process32First(snapshot, &pe); hasNext; hasNext = Process32Next(snapshot, &pe)) {
        ProcessInfo p;
        p.processId = pe.th32ProcessID;
        // Image has the absolute path, but can fail.
        const QString image = Utils::imageName(pe.th32ProcessID);
        p.executable = p.commandLine = image.isEmpty() ?
            QString::fromWCharArray(pe.szExeFile) : image;
        processes << p;
    }
    CloseHandle(snapshot);
    return processes;
}

#endif //Q_OS_WIN

} // namespace Utils
