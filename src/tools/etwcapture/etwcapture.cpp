// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Records the NT Kernel Logger for a process Qt Creator launched or picked.
// The session needs administrator rights, so Qt Creator does not run this
// directly but through etwcapture-launcher.exe, whose manifest asks for them.
// It communicates through the files named on the command line, because the
// elevated process has no channel back to its unelevated caller.

#include <profiler/winsampler.h>

#include <utils/filepath.h>

#include <QCoreApplication>
#include <QStringList>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <qt_windows.h>

using namespace Profiler::Internal;
using namespace Utils;

namespace {

struct Args
{
    qint64 pid = 0;
    int intervalUs = 200;
    FilePath outputDir;    // Where to leave the trace for Qt Creator to pick up.
    FilePath readyFile;    // Written once sampling is about to start.
    FilePath stopFile;     // Created by Qt Creator to end the recording.
    FilePath errorFile;    // Written with the reason when the recording fails.
    FilePath resultFile;   // Written with the trace directory when it succeeds.
};

Args parseArgs(const QStringList &args)
{
    Args result;
    for (int i = 0; i + 1 < args.size(); ++i) {
        const QString &arg = args.at(i);
        if (arg == "--pid")
            result.pid = args.at(++i).toLongLong();
        else if (arg == "--interval")
            result.intervalUs = args.at(++i).toInt();
        else if (arg == "--output")
            result.outputDir = FilePath::fromUserInput(args.at(++i));
        else if (arg == "--readyFile")
            result.readyFile = FilePath::fromUserInput(args.at(++i));
        else if (arg == "--stopFile")
            result.stopFile = FilePath::fromUserInput(args.at(++i));
        else if (arg == "--errorFile")
            result.errorFile = FilePath::fromUserInput(args.at(++i));
        else if (arg == "--resultFile")
            result.resultFile = FilePath::fromUserInput(args.at(++i));
    }
    return result;
}

std::atomic_bool g_stop{false};

BOOL WINAPI consoleHandler(DWORD controlType)
{
    if (controlType == CTRL_CLOSE_EVENT || controlType == CTRL_BREAK_EVENT
        || controlType == CTRL_C_EVENT) {
        g_stop.store(true, std::memory_order_release);
        return TRUE;
    }
    return FALSE;
}

// Moves the trace to where Qt Creator asked for it, so that it does not have to
// look for a directory this process named.
FilePath moveTraceToOutputDir(const FilePath &sourceDir, const FilePath &targetDir)
{
    if (targetDir.isEmpty() || !targetDir.createDir())
        return sourceDir;
    for (const QString &name : {QString("stream0"), QString("metadata")}) {
        if (!sourceDir.pathAppended(name).copyFile(targetDir.pathAppended(name)))
            return sourceDir;
    }
    sourceDir.removeRecursively();
    return targetDir;
}

// Qt Creator reads the reason from the error file, so a write that failed has
// to be said out loud here; there is nowhere left to report it to.
int fail(const Args &args, const QString &message)
{
    std::cerr << "ERROR: " << qPrintable(message) << std::endl;
    if (!args.errorFile.isEmpty()) {
        const Result<qint64> written = args.errorFile.writeFileContents(message.toUtf8());
        if (!written)
            std::cerr << "ERROR: " << qPrintable(written.error()) << std::endl;
    }
    return 1;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SetConsoleCtrlHandler(consoleHandler, TRUE);

    const Args args = parseArgs(QCoreApplication::arguments().mid(1));
    if (args.pid <= 0)
        return fail(args, "--pid <pid> is required.");
    if (args.stopFile.isEmpty())
        return fail(args, "--stopFile <path> is required.");

    SamplerOptions opts;
    opts.pid = args.pid;
    opts.intervalUs = args.intervalUs;

    // Tells Qt Creator that the consent prompt is behind us, so that it starts
    // the recording's clock here rather than when it asked for elevation. A
    // recording it never hears about would sit at "Waiting for capture..." for
    // as long as it ran, so this is worth refusing to record over.
    if (!args.readyFile.isEmpty()) {
        const Result<qint64> written = args.readyFile.writeFileContents({});
        if (!written)
            return fail(args, written.error());
    }

    // A capture that ends by itself, e.g. because it cannot open the target,
    // must not sit here until someone stops a recording that is not running.
    std::atomic_bool captureDone{false};
    Result<FilePath> captureResult = ResultError(QString());
    std::thread captureThread([&] {
        captureResult = recordSampleTrace(opts, [] {
            return g_stop.load(std::memory_order_acquire);
        });
        captureDone.store(true, std::memory_order_release);
    });

    while (!captureDone.load(std::memory_order_acquire)
           && !g_stop.load(std::memory_order_acquire) && !args.stopFile.exists()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    g_stop.store(true, std::memory_order_release);
    captureThread.join();

    if (!captureResult)
        return fail(args, captureResult.error());

    const FilePath traceDir = moveTraceToOutputDir(captureResult.value(), args.outputDir);
    if (!args.resultFile.isEmpty()) {
        const Result<qint64> written
            = args.resultFile.writeFileContents(traceDir.toFSPathString().toUtf8());
        if (!written)
            return fail(args, written.error());
    }
    return 0;
}
