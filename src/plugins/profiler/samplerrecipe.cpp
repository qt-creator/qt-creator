// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplerrecipe.h"

#include "profilertr.h"

#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>

#include <QPointer>

using namespace Profiler;
using namespace QtTaskTree;
using namespace Utils;

namespace QmlProfiler::Internal {

QtTaskTree::Group launchThenCapture(const std::shared_ptr<RecordingSession> &session,
                                    const QtTaskTree::ExecutableItem &capture)
{
    // Attaching to / connecting to an existing target: the backend uses the
    // session's pid or serverUrl directly, with no process to launch.
    if (!session->launchCommand)
        return Group{capture};

    // Launch the chosen command, then capture it once it is running. The process
    // and the capture run in parallel; the capture keeps the target alive until it
    // has finished. If the target exits on its own, session->stop is set so the
    // trace is still written.
    const CommandLine cmd = *session->launchCommand;
    const FilePath workingDir = session->launchWorkingDir;
    // Lets the "capture finished" handler terminate the launched process. The
    // pointer is valid exactly while the ProcessTask runs, i.e. whenever we might
    // still need to stop the process.
    const auto launched = std::make_shared<QPointer<Process>>();

    const auto onProcessSetup = [session, cmd, workingDir, launched](Process &process) {
        process.setCommand(cmd);
        if (!workingDir.isEmpty())
            process.setWorkingDirectory(workingDir);
        // Forward the target's stdout/stderr straight to our console. We do not
        // display its output, and reading it ourselves would make Process install
        // channel socket notifiers whose teardown on macOS can crash when the
        // process exits (QTBUG-style QCFSocketNotifier removal fault).
        process.setProcessChannelMode(QProcess::ForwardedChannels);
        *launched = &process;
        // The PID is known once the process is running; the same started() signal
        // also releases the capture in the When() clause below.
        QObject::connect(&process, &Process::started, &process,
                         [session, p = &process] { session->pid.store(p->processId()); });
    };
    const auto onProcessDone = [session](const Process &process, DoneWith result) {
        // A failure before capture produced anything (e.g. the executable was not
        // found) becomes the recording's error so the user sees the reason.
        if (result == DoneWith::Error && !session->result) {
            const QString error = process.errorString().isEmpty()
                                      ? Tr::tr("The process to profile could not be started.")
                                      : process.errorString();
            session->result.emplace(ResultError(error));
        }
        // If the target exited before the user stopped recording, end the capture
        // so the trace is written; if it is being torn down because the capture
        // already finished, this is a no-op for the backend.
        session->stop.store(true);
    };
    // Once the capture is done (Stop pressed or the target exited), terminate the
    // launched process so its parallel task ends and the recording can finish.
    // Without this the group would wait for the long-running process forever.
    const auto onCaptureDone = [launched] {
        if (Process *process = launched->data())
            process->stop();
    };

    return Group {
        When(ProcessTask(onProcessSetup, onProcessDone), &Process::started) >> Do {
            capture,
            onGroupDone(onCaptureDone),
        }
    };
}

} // namespace QmlProfiler::Internal
