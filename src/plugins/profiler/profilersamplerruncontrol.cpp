// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilersamplerruncontrol.h"

#include "profilermode.h"
#include "profilerrecorder.h"
#include "profilertr.h"
#include "sampler.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runcontrol.h>

#include <QtTaskTree/QBarrier>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>

#include <QPointer>

#include <memory>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

using namespace Qt::StringLiterals;

namespace Profiler::Internal {

Id samplerRunMode(Id backendId)
{
    return Id::fromString(QString(backendId.toString() + ".RunMode"_L1));
}

// Records the run control's target with `backendId`. The backend contributes
// what it captures and how; everything about the target -- what to start, with
// which arguments and environment, on which device, after which deployment --
// comes from the run configuration, exactly as for a plain run.
static Group samplerRecipe(RunControl *runControl, Id backendId)
{
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    ProfilerRecorder *recorder = profilerRecorder();
    QTC_ASSERT(recorder, return runControl->errorTask(Tr::tr("The profiler is not available.")));
    Sampler *sampler = recorder->backendById(backendId);
    QTC_ASSERT(sampler, return runControl->errorTask(Tr::tr("Unknown profiling backend.")));

    const bool qmlChannel = sampler->needsQmlChannel();
    if (qmlChannel)
        runControl->requestQmlChannel();

    const Result<std::shared_ptr<RecordingSession>> session
        = recorder->beginRunControlRecording(backendId, runControl->displayName());
    if (!session)
        return runControl->errorTask(session.error());

    // Lets the "capture finished" handler stop the target. The pointer is valid
    // exactly while the process task runs, i.e. whenever there is still
    // something to stop.
    const auto launched = std::make_shared<QPointer<Process>>();

    const auto modifier = [runControl, session = *session, qmlChannel, launched](Process &process) {
        *launched = &process;
        if (qmlChannel) {
            // The target has to come up as a QML debug server, blocking until the
            // capture has connected, or the first events are lost.
            const QUrl serverUrl = runControl->qmlChannel();
            QString code;
            if (serverUrl.scheme() == urlSocketScheme())
                code = "file:"_L1 + serverUrl.path();
            else if (serverUrl.scheme() == urlTcpScheme())
                code = "port:"_L1 + QString::number(serverUrl.port());
            else
                QTC_CHECK(false);
            CommandLine cmd = process.commandLine();
            cmd.prependArgs(ProcessArgs::quoteArg(qmlDebugCommandLineArguments(
                                QmlProfilerServices, code, /*block*/ true)), CommandLine::Raw);
            process.setCommand(cmd);
            session->serverUrl = serverUrl;
        }
        // The target may finish on its own; end the capture so that what it
        // recorded until then is still written.
        QObject::connect(&process, &Process::done, &process, [session] {
            session->stop.store(true);
        });
    };

    // Stopping the run -- from the recording page, or from the application
    // output -- ends the capture the way the target exiting does, rather than
    // killing the process from under a backend that still has a trace to
    // collect over its debug connection. The process is stopped below, once the
    // capture is done with it.
    QObject::connect(runControl, &RunControl::canceled, runControl, [session = *session] {
        session->stop.store(true);
    });

    // The group below unwinds the recording when it finishes, but it never runs
    // at all when a step the run control put in front of it -- the QML-channel
    // ports gatherer -- fails or is cancelled first, and the recorder would
    // refuse every later recording. The run control's terminal signal covers
    // those paths; ending is idempotent and scoped to this session.
    QObject::connect(runControl, &RunControl::stopped, runControl, [session = *session] {
        if (ProfilerRecorder *recorder = profilerRecorder())
            recorder->endRunControlRecording(session);
    });

    const auto onCaptureDone = [launched] {
        if (Process *process = launched->data())
            process->stop();
    };

    return Group {
        When (runControl->processTaskWithModifier(modifier, {.setupCanceler = false}),
              &Process::started) >> Do {
            // The capture reads the pid, so it is taken here rather than from
            // Process::started, whose handlers would race this branch for it.
            QSyncTask([runControl, session = *session, launched] {
                if (Process *process = launched->data())
                    session->pid.store(process->processId());
                runControl->reportStarted();
            }),
            sampler->captureRecipe(*session),
            onGroupDone(onCaptureDone),
        },
        onGroupDone([session = *session] {
            if (ProfilerRecorder *recorder = profilerRecorder())
                recorder->endRunControlRecording(session);
        }),
    };
}

// Factories

// One per backend: the run mode is what tells Qt Creator which backend a
// recording of the startup project uses, and having one each is what lets it
// answer whether that particular backend could record it.
class ProfilerSamplerRunWorkerFactory final : public RunWorkerFactory
{
public:
    explicit ProfilerSamplerRunWorkerFactory(Id backendId)
    {
        setId(Id::fromString(QString(backendId.toString() + ".RunWorkerFactory"_L1)));
        setRecipeProducer([backendId](RunControl *runControl) {
            return samplerRecipe(runControl, backendId);
        });
        addSupportedRunMode(samplerRunMode(backendId));
        addSupportForLocalRunConfigs();
        // The samplers capture a process on this machine -- by its pid, or over a
        // debug connection to it -- so a target on a device needs a worker of the
        // device's own, as the live profilers have.
        addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    }
};

void setupProfilerSamplerRunning()
{
    static ProfilerSamplerRunWorkerFactory theCallStack(SamplerIds::CallStack);
    static ProfilerSamplerRunWorkerFactory thePerf(SamplerIds::Perf);
    static ProfilerSamplerRunWorkerFactory theQml(SamplerIds::Qml);
    static ProfilerSamplerRunWorkerFactory theCombined(SamplerIds::Combined);
}

} // namespace Profiler::Internal
