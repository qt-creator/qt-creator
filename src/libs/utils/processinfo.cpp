// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processinfo.h"

#include "algorithm.h"
#include "qtcprocess.h"
#include "utilstr.h"

#include <QDir>
#include <QRegularExpression>

#if defined(Q_OS_UNIX)
#elif defined(Q_OS_WIN)
#include "winutils.h"
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
// windows.h needs to be included before psapi.h!
#include <windows.h>

#include <psapi.h>
#include <tlhelp32.h>
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

static Result<QList<ProcessInfo>> getLocalProcessesUsingProc(const FilePath &devicePath)
{
    const FilePath procDir = devicePath.withNewPath("/proc");
    if (!procDir.exists())
        return ResultError(Tr::tr("\"%1\" does not exist.").arg(procDir.toUserOutput()));

    const FilePath find = devicePath.withNewPath("find").searchInPath();
    if (!find.isExecutableFile())
        return ResultError(Tr::tr("\"find\" is not an existing executable"));

    static const QString execs = "-exec test -f {}/exe \\; "
                                 "-exec test -f {}/cmdline \\; "
                                 "-exec echo -en 'p{}\\ne' \\; "
                                 "-exec readlink {}/exe \\; "
                                 "-exec echo -n c \\; "
                                 "-exec head -n 1 {}/cmdline \\; "
                                 "-exec echo \\; "
                                 "-exec echo __SKIP_ME__ \\;";

    CommandLine cmd{find, {procDir.nativePath(), "-maxdepth", "1", "-type", "d", "-name", "[0-9]*"}};

    cmd.addArgs(execs, CommandLine::Raw);

    Process procProcess;
    procProcess.setCommand(cmd);
    procProcess.runBlocking();

    // We can only check the errorString here. The exit code maybe != 0 if one of the "test"s failed.
    if (!procProcess.errorString().isEmpty()) {
        return ResultError(Tr::tr("Failed to run %1: %2")
                                   .arg(cmd.executable().toUserOutput())
                                   .arg(procProcess.errorString()));
    }

    QList<ProcessInfo> processes;

    const auto lines = procProcess.readAllStandardOutput().split('\n');
    int currentLineIndex = 0;

    const auto debugInfo = [&processes, &lines, &currentLineIndex] {
        qDebug() << "Collected processes count:" << processes.count()
                 << "Lines count:" << lines.count() << "Current line:" << currentLineIndex;
        static const int s_printLinesCount = 10;
        qDebug() << "Last" << s_printLinesCount << "lines:";
        const int minIndex = qMax(currentLineIndex - s_printLinesCount, 0);
        const int maxIndex = qMin(currentLineIndex + 1, lines.size());
        for (int i = minIndex; i < maxIndex; ++i)
            qDebug() << i << lines.at(i);
    };

    for (auto it = lines.begin(); it != lines.end(); ++it, ++currentLineIndex) {
        if (it->startsWith('p')) {
            ProcessInfo proc;
            bool ok;
            proc.processId = FilePath::fromUserInput(it->mid(1).trimmed()).fileName().toInt(&ok);
            QTC_ASSERT(ok, debugInfo(); continue);
            ++it;
            ++currentLineIndex;

            QTC_ASSERT(it != lines.end() && it->startsWith('e'), debugInfo(); continue);
            proc.executable = it->mid(1).trimmed();
            ++it;
            ++currentLineIndex;

            QTC_ASSERT(it != lines.end() && it->startsWith('c'), debugInfo(); continue);
            proc.commandLine = it->mid(1).trimmed().replace('\0', ' ');
            if (!proc.commandLine.contains("__SKIP_ME__"))
                processes.append(proc);
        }
    }

    return processes;
}

// Determine UNIX processes by running ps
static Result<QMap<qint64, QString>> getLocalProcessDataUsingPs(
    const FilePath &ps, const QString &column)
{
    Process process;
    process.setCommand({ps, {"-e", "-o", "pid," + column}});
    process.runBlocking();
    if (!process.errorString().isEmpty()) {
        return ResultError(
            Tr::tr("Failed to run %1: %2").arg(ps.toUserOutput()).arg(process.errorString()));
    }

    if (process.exitCode() != 0) {
        return ResultError(Tr::tr("Failed to run %1: %2")
                                   .arg(ps.toUserOutput())
                                   .arg(process.readAllStandardError()));
    }

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

static Result<QList<ProcessInfo>> getLocalProcessesUsingPs(const FilePath &deviceRoot)
{
    QList<ProcessInfo> processes;

    const FilePath ps = deviceRoot.withNewPath("ps").searchInPath();
    if (!ps.isExecutableFile())
        return ResultError(Tr::tr("\"ps\" is not an existing executable."));

    // cmdLines are full command lines, usually with absolute path,
    // exeNames only the file part of the executable's path.
    using namespace Qt::Literals;
    const QString exeNameColumn = deviceRoot.osType() == OsTypeMac ? "comm"_L1 : "exe"_L1;
    const Result<QMap<qint64, QString>> exeNames = getLocalProcessDataUsingPs(ps, exeNameColumn);
    if (!exeNames)
        return ResultError(exeNames.error());

    const Result<QMap<qint64, QString>> cmdLines = getLocalProcessDataUsingPs(ps, "args");
    if (!cmdLines)
        return ResultError(cmdLines.error());

    for (auto it = exeNames->begin(), end = exeNames->end(); it != end; ++it) {
        const qint64 pid = it.key();
        if (pid <= 0)
            continue;
        const QString cmdLine = cmdLines->value(pid);
        if (cmdLines->isEmpty())
            continue;
        const QString exeName = it.value();
        if (exeName.isEmpty())
            continue;
        processes.append({pid, exeName, cmdLine});
    }

    return processes;
}

static Result<QList<ProcessInfo>> getProcessesUsingPidin(const FilePath &deviceRoot)
{
    const FilePath pidin = deviceRoot.withNewPath("pidin").searchInPath();
    if (!pidin.isExecutableFile())
        return ResultError(Tr::tr("\"pidin\" is not an existing executable."));

    Process process;
    process.setCommand({pidin, {"-F", "%a %A {/%n}"}});
    process.runBlocking();
    if (process.errorString().isEmpty()) {
        return ResultError(
            Tr::tr("Failed to run %1: %2").arg(pidin.toUserOutput()).arg(process.errorString()));
    }
    if (process.exitCode() != 0)
        return ResultError(process.readAllStandardError());

    QList<ProcessInfo> processes;
    QStringList lines = process.readAllStandardOutput().split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return processes;

    lines.pop_front(); // drop headers
    static const QRegularExpression re("\\s*(\\d+)\\s+(.*){(.*)}");

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

static Result<QList<ProcessInfo>> processInfoListUnix(const FilePath &deviceRoot)
{
    return getLocalProcessesUsingPs(deviceRoot)
        .transform_error(
            [](const QString &error) { return Tr::tr("Failed to run ps: %1").arg(error); })
        .or_else([&deviceRoot](const QString &error) {
            return getProcessesUsingPidin(deviceRoot)
                .transform_error([error](const QString &pidinError) {
                    return Tr::tr("Failed to run pidin: %1\n%2").arg(pidinError).arg(error);
                });
        })
        .or_else([&deviceRoot](const QString &error) {
            return getLocalProcessesUsingProc(deviceRoot)
                .transform_error([error](const QString &procError) {
                    return Tr::tr("Failed to check /proc: %1\n%2").arg(procError).arg(error);
                });
        });
}

Result<QList<ProcessInfo>> ProcessInfo::processInfoList(const FilePath &deviceRoot)
{
    if (deviceRoot.osType() != OsType::OsTypeWindows)
        return processInfoListUnix(deviceRoot);

    if (HostOsInfo::isWindowsHost() && deviceRoot.isLocal()) {
#if defined(Q_OS_WIN)
        QList<ProcessInfo> processes;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return ResultError(
                Tr::tr("Failed to create snapshot: %1").arg(winErrorMessage(GetLastError())));
        }

        for (bool hasNext = Process32First(snapshot, &pe); hasNext;
             hasNext = Process32Next(snapshot, &pe)) {
            ProcessInfo p;
            p.processId = pe.th32ProcessID;
            // Image has the absolute path, but can fail.
            const QString image = imageName(pe.th32ProcessID);
            p.executable = p.commandLine = image.isEmpty() ? QString::fromWCharArray(pe.szExeFile)
                                                           : image;
            processes << p;
        }
        CloseHandle(snapshot);
        return processes;
#endif
    }

    return {};
}

} // namespace Utils
