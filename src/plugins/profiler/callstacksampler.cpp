// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callstacksampler.h"

#include "macsampler.h"
#ifdef Q_OS_WIN
#include "winsampler.h"
#endif
#include "processpickerdialog.h"

#include "profilertr.h"

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/layoutbuilder.h>
#include <utils/processinfo.h>
#include <utils/qtdesignwidgets.h>

#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPromise>

#include <QtTaskTree/QBarrier>

#include <memory>
#include <optional>

using namespace QtTaskTree;
using namespace Utils;

namespace Profiler::Internal {

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
// No sampling backend on this platform. isAvailable() already reports that, but
// captureRecipe() still has to compile.
static Result<FilePath> recordSampleTrace(const SamplerOptions &, const std::function<bool()> &,
                                          const std::function<void(int)> &)
{
    return ResultError(Tr::tr("Call-stack sampling is only implemented on macOS and Windows."));
}
#endif

CallStackSamplerSettings::CallStackSamplerSettings()
{
    setSettingsGroup("CallStackSampler");

    intervalUs.setSettingsKey("IntervalUs");
    intervalUs.setLabelText(Tr::tr("Sample interval (µs):"));
    intervalUs.setRange(0, 1000000); // 0 = as fast as possible.
    intervalUs.setDefaultValue(200);

    attach.setSettingsKey("Attach");
    attach.setLabel(Tr::tr("Attach to a running process"),
                    BoolAspect::LabelPlacement::AtCheckBox);
    updateTargetEnabled();
    connect(&attach, &BoolAspect::changed, this, [this] { updateTargetEnabled(); });

    setLayouter([this] {
        using namespace Layouting;
        auto pick = new QtcButton(Tr::tr("Select Process…"), QtcButton::SmallSecondary);
        auto picked = new QtcLabel(m_pickedName.isEmpty() ? Tr::tr("No process selected")
                                                          : m_pickedName,
                                   QtcLabel::Secondary);
        const auto updatePick = [this, pick] {
            pick->setEnabled(!targetChosenElsewhere() && attach());
        };
        updatePick();
        connect(&attach, &BoolAspect::changed, pick, updatePick);
        connect(this, &SamplerSettings::targetSelectionChanged, pick, updatePick);
        connect(pick, &QAbstractButton::clicked, this, [this, picked] {
            const std::optional<ProcessInfo> info = ProcessPickerDialog::pickProcess();
            if (!info)
                return;
            m_pickedPid = info->processId;
            m_pickedName = FilePath::fromUserInput(info->executable).fileName();
            picked->setText(m_pickedName);
        });
        return Column {
            executable,
            arguments,
            workingDirectory,
            Row { intervalUs, st },
            Row { attach, pick, picked, st },
        };
    });
}

void CallStackSamplerSettings::fillOptions(RecordingSession &session) const
{
    session.intervalUs = int(intervalUs());
}

// The launch settings are irrelevant while attaching, and both are while the
// target comes from a run configuration.
void CallStackSamplerSettings::updateTargetEnabled()
{
    const bool own = !targetChosenElsewhere();
    attach.setEnabled(own);
    const bool launching = own && !attach();
    executable.setEnabled(launching);
    arguments.setEnabled(launching);
    workingDirectory.setEnabled(launching);
}

Result<std::shared_ptr<RecordingSession>> CallStackSamplerSettings::createSession() const
{
    auto session = std::make_shared<RecordingSession>();
    fillOptions(*session);
    if (attach()) {
        if (m_pickedPid == 0)
            return ResultError(Tr::tr("Select a process to attach to."));
        session->pid = m_pickedPid;
        session->processName = m_pickedName;
        return session;
    }
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    return session;
}

CallStackSampler::CallStackSampler()
    : m_settings(std::make_unique<CallStackSamplerSettings>())
{}

CallStackSampler::~CallStackSampler() = default;

QString CallStackSampler::displayName() const
{
    return Tr::tr("Call-Stack Sampler");
}

bool CallStackSampler::isAvailable(QString *error) const
{
#if defined(Q_OS_MACOS)
    if (!canSampleOtherProcesses()) {
        if (error) {
            *error = Tr::tr("Sampling another process needs the "
                            "\"com.apple.security.cs.debugger\" entitlement, which this build of "
                            "%1 does not carry.").arg(QGuiApplication::applicationDisplayName());
        }
        return false;
    }
    return true;
#elif defined(Q_OS_WIN)
    Q_UNUSED(error)
    return true;
#else
    if (error)
        *error = Tr::tr("Call-stack sampling is only implemented on macOS and Windows.");
    return false;
#endif
}

SamplerSettings *CallStackSampler::settings() const
{
    return m_settings.get();
}

ExecutableItem CallStackSampler::captureRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    // The cadence comes from the session (set by createSession, or by a composite
    // backend that drives this capture), so the sampling worker can read it.
    const int intervalUs = session->intervalUs;

    // Where the worker leaves the trace it captured. Not the promise's own
    // result channel: stopping cancels the promise, and Qt drops a result added
    // to a cancelled future -- while the whole point of stopping is to keep
    // what was recorded until then. Nor the recipe's Storage, which is gone
    // once the tree is: a capture abandoned at shutdown may still be writing.
    auto captured = std::make_shared<std::optional<Result<FilePath>>>();

    const auto onSetup = [session, intervalUs, captured](QBarrier &barrier) {
        QBarrier *b = &barrier;

        // recordSampleTrace samples until its promise is cancelled, so it runs
        // on a worker thread; the target stays alive (the launched process is
        // only terminated once this task finishes) so it can still be
        // symbolized.
        QFuture<void> capture = Utils::asyncRun(
            [session, intervalUs, captured](QPromise<void> &promise) {
            SamplerOptions opts;
            opts.pid = session->pid.load();
            opts.processName = session->processName;
            opts.intervalUs = intervalUs;
            session->markStarted(); // capture is live; the duration clock can start
            // Progress is reported from this worker thread; the session queues
            // it onto the GUI thread, so the frontend hears it without watching.
            *captured = recordSampleTrace(opts,
                                          [&promise] { return promise.isCanceled(); },
                                          [session](int percent) { session->setProgress(percent); });
        });

        // Owned by the barrier, so it goes exactly when this task does.
        auto *watcher = new QFutureWatcher<void>(b);
        QObject::connect(watcher, &QFutureWatcherBase::finished, b, [b, session, captured] {
            // However the capture ended, what it wrote is what the recording
            // produced -- and ending it is not a failure, so the task finishes
            // with success and the rest of the recipe follows.
            if (captured->has_value())
                session->result = *captured;
            b->advance();
        });
        watcher->setFuture(capture);

        // Stopping cancels the future the sampling loop watches: it winds down,
        // writes what it has, and the handler above ends this task.
        session->onStopRequested(b, [watcher] { watcher->cancel(); });

        // A tree torn down mid-capture (see ProfilerRecorder::stopAndWait) does
        // not wait for the worker, so tell it to stop on the way out.
        QObject::connect(b, &QObject::destroyed, [capture]() mutable { capture.cancel(); });

        // And whatever is still running at shutdown is cancelled and waited for
        // here, as QThreadFunction's own synchronizer used to do.
        Utils::futureSynchronizer()->addFuture(capture);
    };
    return QBarrierTask(onSetup);
}

} // namespace Profiler::Internal
