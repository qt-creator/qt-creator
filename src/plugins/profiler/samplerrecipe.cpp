// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplerrecipe.h"

#include "profilertr.h"

#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>

#include <QCoreApplication>
#include <QPointer>

using namespace QtTaskTree;
using namespace Utils;

namespace Profiler::Internal {

QtTaskTree::Group launchThenCapture(const std::shared_ptr<RecordingSession> &session,
                                    const QtTaskTree::ExecutableItem &capture)
{
    // Attaching to / connecting to an existing target: the backend uses the
    // session's pid or serverUrl directly, with no process to launch.
    if (!session->launchCommand)
        return Group{capture};

    // Launch the chosen command, then capture it once it is running. The process
    // and the capture run in parallel; the capture keeps the target alive until it
    // has finished. If the target exits on its own, a stop is requested so the
    // trace is still written.
    const CommandLine cmd = *session->launchCommand;
    const FilePath workingDir = session->launchWorkingDir;
    const Environment environment = session->launchEnvironment;
    const EnvironmentItems environmentChanges = session->launchEnvironmentChanges;
    // Lets the "capture finished" handler terminate the launched process. The
    // pointer is valid exactly while the ProcessTask runs, i.e. whenever we might
    // still need to stop the process.
    const auto launched = std::make_shared<QPointer<Process>>();
    // Set once the recording is over, which is when the target is stopped on
    // purpose -- so that its ending is not read as a failure then.
    const auto captureDone = std::make_shared<bool>(false);

    const auto onProcessSetup = [session, cmd, workingDir, environment, environmentChanges,
                                 launched](Process &process) {
        process.setCommand(cmd);
        if (!workingDir.isEmpty())
            process.setWorkingDirectory(workingDir);
        if (environment.hasChanges() || !environmentChanges.isEmpty()) {
            // The changes have to be merged into an environment rather than set
            // on their own, and with none given that is the one the target would
            // have been launched with anyway -- the device's, which for a local
            // target is this process's.
            Environment env = environment.hasChanges() ? environment
                                                       : cmd.executable().deviceEnvironment();
            env.modify(environmentChanges);
            process.setEnvironment(env);
        }
        // Forward the target's stdout/stderr straight to our console. We do not
        // display its output, and reading it ourselves would make Process install
        // channel socket notifiers whose teardown on macOS can crash when the
        // process exits (QTBUG-style QCFSocketNotifier removal fault).
        process.setProcessChannelMode(ProcessChannelMode::ForwardedChannels);
        *launched = &process;
        // The PID is known once the process is running; the same started() signal
        // also releases the capture in the When() clause below.
        QObject::connect(&process, &Process::started, &process,
                         [session, p = &process] { session->pid.store(p->processId()); });
    };
    const auto onProcessDone = [session, captureDone](const Process &process, DoneWith result) {
        // A target that failed is what the user needs to hear: one that was not
        // found, and equally one that came up and then exited with an error --
        // a library it could not load, an argument it would not take. Kept
        // aside rather than failing the recording here, because what was
        // captured until then, if anything, stands (see Sampler::recordRecipe()).
        //
        // Not once the recording is over: the target is then stopped on purpose
        // (see onCaptureDone) and ends in exactly the error a killed one does.
        // Nor when the recording has already failed for a reason of its own,
        // which came first and is the one to explain.
        if (result == DoneWith::Error && !session->result && !*captureDone) {
            const QString error = process.exitMessage();
            session->targetError = error.isEmpty()
                                       ? Tr::tr("The process to profile could not be started.")
                                       : error;
        }
        // If the target exited before the user stopped recording, end the capture
        // so the trace is written; if it is being torn down because the capture
        // already finished, this is a no-op for the backend.
        //
        // Queued, because a stop can advance the barrier a capture waits on, and
        // advancing it from inside this handler runs one task of the group from
        // another's handler -- which QtTaskTree warns about and says may crash.
        // Nothing waits on it: the group runs until the capture ends either way.
        QMetaObject::invokeMethod(qApp, [session] { session->requestStop(); },
                                  Qt::QueuedConnection);
    };
    // Once the capture is done (Stop pressed or the target exited), terminate the
    // launched process so its parallel task ends and the recording can finish.
    // Without this the group would wait for the long-running process forever.
    const auto onCaptureDone = [launched, captureDone] {
        *captureDone = true;
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

} // namespace Profiler::Internal
