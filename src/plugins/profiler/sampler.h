// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/aspects.h>
#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/result.h>

#include <QtTaskTree/QTaskTree>

#include <QString>
#include <QUrl>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>

namespace QmlProfiler::Internal {

// Shared state for one recording session. The GUI thread owns it and observes
// progress while a worker thread records; the sampler task captures it. Always
// handled through a shared_ptr, never copied.
struct RecordingSession
{
    // Inputs, set before recording starts.
    int intervalUs = 200;            // Backend-specific cadence hint.
    QString processName;             // Attach-by-name target, used when pid == 0.

    // QmlDebug::ProfileFeature bitmask to record for protocol-based backends; 0
    // means "record everything". Carried on the session so a composite backend
    // can drive its QML sub-capture (see CombinedSampler).
    quint64 requestedFeatures = 0;

    // Set when a target is to be launched (otherwise the backend attaches to or
    // connects to an already-running target). The backend composes the launch
    // itself (see launchThenCapture()), so it may rewrite the command, e.g. to
    // inject -qmljsdebugger arguments.
    std::optional<Utils::CommandLine> launchCommand;
    Utils::FilePath launchWorkingDir;

    // QML debug channel for protocol-based backends; empty for native ones.
    QUrl serverUrl;

    // Set once a launched process is running; selects attach-by-pid instead.
    std::atomic<qint64> pid = 0;

    // Runtime control and output.
    std::atomic_bool started = false; // Set once the backend is actually capturing.
    std::atomic_bool stop = false;   // The GUI sets this to end recording.
    std::atomic<int> progress = 0;   // 0..100 post-processing percent.
    std::optional<Utils::Result<Utils::FilePath>> result; // Set on the GUI thread when done.

    // Monotonic instant (steady_clock, microseconds) at which the backend went
    // live, or -1 if it never did. steady_clock is a single process-wide timeline,
    // so two sessions recorded together are directly comparable -- this is what
    // lets a combined recording correlate its two traces onto one axis.
    std::atomic<qint64> startedMonotonicUs = -1;

    // Marks the capture live, stamping the monotonic instant on the first call
    // (later calls only re-set the flag). Backends call this at the exact point
    // they begin capturing, in place of setting `started` directly.
    void markStarted()
    {
        if (!started.exchange(true)) {
            startedMonotonicUs.store(std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::steady_clock::now().time_since_epoch())
                                         .count(),
                                     std::memory_order_relaxed);
        }
    }
};

// Backend-specific recording settings. Besides holding the options, it renders its
// own configuration controls via AspectContainer::setLayouter(), keeping them next
// to the settings they use. All backends can launch an executable, so the launch
// command lives here; backend-specific alternatives (attach to a pid, connect to a
// debug server) are added by subclasses, which decide in createSession() which
// mode the current settings select.
class PROFILER_EXPORT SamplerSettings : public Utils::AspectContainer
{
    Q_OBJECT

public:
    SamplerSettings();

    // The executable to launch and profile, used unless a subclass selects a
    // different start mode (attach/connect).
    Utils::FilePathAspect executable{this};
    Utils::StringAspect arguments{this};
    Utils::FilePathAspect workingDirectory{this};

    // Builds a RecordingSession from the current settings, or an error explaining
    // what is missing/misconfigured (e.g. no executable, or attach with no process).
    virtual Utils::Result<std::shared_ptr<RecordingSession>> createSession() const = 0;

protected:
    // Populates session->launchCommand/launchWorkingDir from executable+arguments;
    // fails if no executable is set.
    Utils::Result<> fillLaunch(RecordingSession &session) const;
};

// A profiling backend: records a trace of a target process until the session is
// stopped, then writes it and reports the resulting trace directory.
//
// Implementations differ in how they capture (call-stack sampling today; a QML
// profiler or perf-based recorder could be added alongside). Each returns a
// QtTaskTree recipe so the window can compose it with process launching.
class PROFILER_EXPORT Sampler
{
public:
    virtual ~Sampler() = default;

    virtual QString displayName() const = 0;

    // Whether this backend can run in the current environment; when it cannot,
    // fills *error with a human-readable reason.
    virtual bool isAvailable(QString *error = nullptr) const = 0;

    // The complete recipe that records the target described by `session`: it
    // prepares and launches session->launchCommand (when set) and captures the
    // target until session->stop, storing its Result into session->result when
    // done. Concrete: it calls prepareLaunch() and then wraps captureRecipe() in
    // launchThenCapture(), so the caller never needs to know how a particular
    // backend starts its target. Backends customise the two hooks below.
    QtTaskTree::ExecutableItem recordRecipe(
        const std::shared_ptr<RecordingSession> &session) const;

    // Adjusts session->launchCommand before the target is launched -- e.g. the
    // QML backend injects -qmljsdebugger and allocates session->serverUrl here.
    // Called by recordRecipe() before the process starts; the default does
    // nothing. Public so a composite backend (see CombinedSampler) can delegate
    // to the backends it wraps.
    virtual void prepareLaunch(const std::shared_ptr<RecordingSession> &session) const;

    // The capture portion of the recipe, run after the target (if any) is
    // launched. Reads session->pid / session->serverUrl / session->stop and
    // stores the trace path into session->result. recordRecipe() wraps this in
    // launchThenCapture(); a composite backend calls it directly for each
    // backend it wraps.
    virtual QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const = 0;

    // Backend-specific recording settings, which also render the backend's own
    // configuration controls and attach/connect start buttons (see SamplerSettings).
    // Null when the backend has no options. Owned by the backend.
    virtual SamplerSettings *settings() const { return nullptr; }
};

} // namespace QmlProfiler::Internal
