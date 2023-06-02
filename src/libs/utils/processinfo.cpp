// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processinfo.h"

#include "algorithm.h"
#include "process.h"

#include <QDir>
#include <QRegularExpression>

#if defined(Q_OS_UNIX)
#elif defined(Q_OS_WIN)
#include "winutils.h"
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <windows.h>
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

// Determine UNIX processes by reading "/proc". Default to ps if
// it does not exist

static QList<ProcessInfo> getLocalProcessesUsingProc(const FilePath &procDir)
{
    static const QString execs = "-exec test -f {}/exe \\; "
                                 "-exec test -f {}/cmdline \\; "
                                 "-exec echo -en 'p{}\\ne' \\; "
                                 "-exec readlink {}/exe \\; "
                                 "-exec echo -n c \\; "
                                 "-exec head -n 1 {}/cmdline \\; "
                                 "-exec echo \\; "
                                 "-exec echo __SKIP_ME__ \\;";

    CommandLine cmd{procDir.withNewPath("find"),
                    {procDir.nativePath(), "-maxdepth", "1", "-type", "d", "-name", "[0-9]*"}};

    cmd.addArgs(execs, CommandLine::Raw);

    Process procProcess;
    procProcess.setCommand(cmd);
    procProcess.runBlocking();

    QList<ProcessInfo> processes;

    const auto lines = procProcess.readAllStandardOutput().split('\n');
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->startsWith('p')) {
            ProcessInfo proc;
            bool ok;
            proc.processId = FilePath::fromUserInput(it->mid(1).trimmed()).fileName().toInt(&ok);
            QTC_ASSERT(ok, continue);
            ++it;

            QTC_ASSERT(it->startsWith('e'), continue);
            proc.executable = it->mid(1).trimmed();
            ++it;

            QTC_ASSERT(it->startsWith('c'), continue);
            proc.commandLine = it->mid(1).trimmed().replace('\0', ' ');
            if (!proc.commandLine.contains("__SKIP_ME__"))
                processes.append(proc);
        }
    }

    return processes;
}

// Determine UNIX processes by running ps
static QMap<qint64, QString> getLocalProcessDataUsingPs(const FilePath &deviceRoot,
                                                        const QString &column)
{
    Process process;
    process.setCommand({deviceRoot.withNewPath("ps"), {"-e", "-o", "pid," + column}});
    process.runBlocking();

    // Split "457 /Users/foo.app arg1 arg2"
    const QStringList lines = process.readAllStandardOutput().split(QLatin1Char('\n'));
    QMap<qint64, QString> result;
    for (int i = 1; i < lines.size(); ++i) { // Skip header
        const QString line = lines.at(i).trimmed();
        const int pidSep = line.indexOf(QChar::Space);
        const qint64 pid = line.left(pidSep).toLongLong();
        result.insert(pid, line.mid(pidSep + 1));
    }
    return result;
}

static QList<ProcessInfo> getLocalProcessesUsingPs(const FilePath &deviceRoot)
{
    QList<ProcessInfo> processes;

    // cmdLines are full command lines, usually with absolute path,
    // exeNames only the file part of the executable's path.
    const QMap<qint64, QString> exeNames = getLocalProcessDataUsingPs(deviceRoot, "comm");
    const QMap<qint64, QString> cmdLines = getLocalProcessDataUsingPs(deviceRoot, "args");

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

static QList<ProcessInfo> getProcessesUsingPidin(const FilePath &pidin)
{
    Process process;
    process.setCommand({pidin, {"-F", "%a %A {/%n}"}});
    process.runBlocking();

    QList<ProcessInfo> processes;
    QStringList lines = process.readAllStandardOutput().split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return processes;

    lines.pop_front(); // drop headers
    const QRegularExpression re("\\s*(\\d+)\\s+(.*){(.*)}");

    for (const QString &line : std::as_const(lines)) {
        const QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            const QStringList captures = match.capturedTexts();
            if (captures.size() == 4) {
                const int pid = captures[1].toInt();
                const QString args = captures[2];
                const QString exe = captures[3];
                ProcessInfo deviceProcess;
                deviceProcess.processId = pid;
                deviceProcess.executable = exe.trimmed();
                deviceProcess.commandLine = args.trimmed();
                processes.append(deviceProcess);
            }
        }
    }

    return Utils::sorted(std::move(processes));
}

static QList<ProcessInfo> processInfoListUnix(const FilePath &deviceRoot)
{
    const FilePath procDir = deviceRoot.withNewPath("/proc");
    const FilePath pidin = deviceRoot.withNewPath("pidin").searchInPath();

    if (pidin.isExecutableFile())
        return getProcessesUsingPidin(pidin);

    if (procDir.isReadableDir())
        return getLocalProcessesUsingProc(procDir);

    return getLocalProcessesUsingPs(deviceRoot);
}

#if defined(Q_OS_UNIX)

QList<ProcessInfo> ProcessInfo::processInfoList(const FilePath &deviceRoot)
{
    return processInfoListUnix(deviceRoot);
}

#elif defined(Q_OS_WIN)

QList<ProcessInfo> ProcessInfo::processInfoList(const FilePath &deviceRoot)
{
    if (deviceRoot.needsDevice())
        return processInfoListUnix(deviceRoot);

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
        const QString image = imageName(pe.th32ProcessID);
        p.executable = p.commandLine = image.isEmpty() ?
            QString::fromWCharArray(pe.szExeFile) : image;
        processes << p;
    }
    CloseHandle(snapshot);
    return processes;
}

#endif //Q_OS_WIN

} // namespace Utils
