// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "etwlauncher_win.h"

#include "profilertr.h"

#include <utils/commandline.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QLibraryInfo>
#include <QScopeGuard>

#include <qt_windows.h>
#include <shellapi.h>

#include <string>

using namespace Utils;

namespace Profiler::Internal {
namespace {

// How long the elevated process may take to write its trace after a stop was
// requested. Only a capture that no longer reacts is killed; one that is still
// running is never cut short.
constexpr DWORD kStopGraceMs = 60'000;

FilePath launcherExecutable()
{
    const FilePath launcher = FilePath::fromString(QCoreApplication::applicationDirPath())
                                  .pathAppended("etwcapture-launcher.exe");
    return launcher.exists() ? launcher : FilePath();
}

QString takeContents(const FilePath &path)
{
    if (!path.exists())
        return {};
    const Result<QByteArray> contents = path.fileContents();
    path.removeFile();
    return contents ? QString::fromUtf8(*contents).trimmed() : QString();
}

} // namespace

Result<FilePath> recordSampleTraceElevated(const std::shared_ptr<RecordingSession> &session,
                                           int intervalUs,
                                           const std::function<bool()> &isCanceled)
{
    const FilePath launcher = launcherExecutable();
    if (launcher.isEmpty())
        return ResultError(Tr::tr("etwcapture-launcher.exe was not found."));

    const qint64 pid = session->pid.load();
    if (pid <= 0)
        return ResultError(Tr::tr("There is no process to sample."));

    // Concurrent recordings, and leftovers of ones that crashed, must not be
    // able to pass their files off as this recording's.
    const QString unique = QString::number(QDateTime::currentMSecsSinceEpoch()) + u'-'
                           + QString::number(QCoreApplication::applicationPid());
    const QDir tempDir = QDir::temp();
    const FilePath outputDir = FilePath::fromString(
        tempDir.filePath("qtprofiler-sample-" + unique));
    if (!outputDir.createDir())
        return ResultError(Tr::tr("Cannot create the trace directory %1.")
                               .arg(outputDir.toUserOutput()));

    const FilePath readyFile = FilePath::fromString(
        tempDir.filePath("qtprofiler-ready-" + unique));
    const FilePath stopFile = FilePath::fromString(
        tempDir.filePath("qtprofiler-stop-" + unique));
    const FilePath errorFile = FilePath::fromString(
        tempDir.filePath("qtprofiler-error-" + unique));
    const FilePath resultFile = FilePath::fromString(
        tempDir.filePath("qtprofiler-result-" + unique));
    bool wroteTrace = false;
    const QScopeGuard removeTemporaries([&] {
        readyFile.removeFile();
        stopFile.removeFile();
        errorFile.removeFile();
        resultFile.removeFile();
        if (!wroteTrace)
            outputDir.removeRecursively();
    });

    // The elevated process is created by the AppInfo service and so does not
    // inherit this environment. The launcher composes the PATH it needs, but
    // cannot know where Qt is when Qt Creator is not installed next to it.
    QString parameters;
    ProcessArgs::addArgs(&parameters,
                         {"--library-path", QLibraryInfo::path(QLibraryInfo::BinariesPath),
                          "--pid", QString::number(pid),
                          "--interval", QString::number(intervalUs),
                          "--output", outputDir.toFSPathString(),
                          "--readyFile", readyFile.toFSPathString(),
                          "--stopFile", stopFile.toFSPathString(),
                          "--errorFile", errorFile.toFSPathString(),
                          "--resultFile", resultFile.toFSPathString()},
                         OsTypeWindows);

    // Both strings have to outlive the call: ShellExecuteEx returns as soon as
    // the launch is initiated, and the child reads the command line after that.
    const std::wstring launcherPath = launcher.toFSPathString().toStdWString();
    const std::wstring launcherParameters = parameters.toStdWString();

    SHELLEXECUTEINFOW execInfo {};
    execInfo.cbSize = sizeof(execInfo);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    // No verb: the launcher's manifest asks for administrator rights, so
    // ShellExecuteEx shows the consent prompt for it. CreateProcess, and with
    // it QProcess, fails with ERROR_ELEVATION_REQUIRED instead of elevating.
    execInfo.lpFile = launcherPath.c_str();
    execInfo.lpParameters = launcherParameters.c_str();
    execInfo.nShow = SW_HIDE;
    if (!ShellExecuteExW(&execInfo)) {
        const DWORD error = GetLastError();
        if (error == ERROR_CANCELLED)
            return ResultError(Tr::tr("Sampling with ETW was not allowed to run as "
                                      "administrator."));
        return ResultError(Tr::tr("Cannot start etwcapture-launcher.exe: error %1.").arg(error));
    }

    const HANDLE launcherProcess = execInfo.hProcess;
    const QScopeGuard closeLauncherProcess([launcherProcess] { CloseHandle(launcherProcess); });

    bool stopRequested = false;
    DWORD stopRequestedAt = 0;
    while (true) {
        if (!session->isStarted() && readyFile.exists())
            session->markStarted();

        // The file's existence is the stop signal, so writing it once is
        // enough; a write that failed is retried on the next round.
        if (!stopRequested && isCanceled() && stopFile.writeFileContents({})) {
            stopRequested = true;
            stopRequestedAt = GetTickCount();
        }

        if (WaitForSingleObject(launcherProcess, 100) == WAIT_OBJECT_0)
            break;

        if (stopRequested && GetTickCount() - stopRequestedAt >= kStopGraceMs) {
            // The launcher holds etwcapture.exe in a job object that kills its
            // members when it is closed, so this takes the capture with it.
            TerminateProcess(launcherProcess, 1);
            return ResultError(Tr::tr("Sampling with ETW did not stop."));
        }
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(launcherProcess, &exitCode);

    const QString error = takeContents(errorFile);
    if (!error.isEmpty())
        return ResultError(error);
    if (exitCode != 0)
        return ResultError(Tr::tr("Sampling with ETW failed with exit code %1.").arg(exitCode));

    const QString traceDir = takeContents(resultFile);
    const FilePath trace = FilePath::fromString(traceDir);
    if (traceDir.isEmpty() || !trace.exists())
        return ResultError(Tr::tr("Sampling with ETW produced no trace."));
    wroteTrace = true;
    return trace;
}

} // namespace Profiler::Internal
