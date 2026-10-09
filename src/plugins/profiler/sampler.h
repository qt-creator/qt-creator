// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <utils/aspects.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/result.h>

#include <QtTaskTree/QTaskTree>

#include <QMutex>
#include <QPointer>
#include <QString>
#include <QUrl>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Profiler::Internal {

// Options for a single sampling session. The target is selected by `pid` when it
// is non-zero (e.g. a process launched to be profiled), otherwise by `processName`.
struct SamplerOptions
{
    qint64 pid = 0;       // Process id to attach to; 0 selects by processName instead.
    QString processName;  // Executable basename to attach to, e.g. "Qt Creator".
    int intervalUs = 200; // Target delay between samples; 0 = as fast as possible.
    // Polled from the sampling thread; while it returns true, nothing is sampled
    // and the target runs undisturbed. Unset means never paused.
    std::function<bool()> isPaused;
    // Called once, when sampling actually begins: at once, or at the first
    // resume of a recording that starts paused.
    std::function<void()> markStarted;
};

// Announces that what a frontend shows of a recording has changed. Its own
// QObject, because a capture working on a thread of its own reports through it
// and Qt's queued delivery is what makes that safe -- the session itself stays
// a plain struct that any thread may hold.
class PROFILER_EXPORT RecordingReporter : public QObject
{
    Q_OBJECT

signals:
    void changed();
};

// Shared state for one recording session. The GUI thread owns it and observes
// progress while a worker thread records; the sampler task captures it. Always
// handled through a shared_ptr, never copied.
struct RecordingSession : std::enable_shared_from_this<RecordingSession>
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
    // The environment to launch in; empty means the one this process inherited.
    Utils::Environment launchEnvironment;
    // Variables the target has to be launched with, set by prepareLaunch() for
    // a backend that is driven through the environment rather than the command
    // line. Kept apart from launchEnvironment because whoever launches may
    // bring an environment of its own -- a run configuration's -- that these
    // are merged into rather than replacing.
    Utils::EnvironmentItems launchEnvironmentChanges;
    // The executable a launch this backend does not own will start, set for a
    // recording of a run control: `launchCommand` stays empty there, and
    // `processName` cannot stand in for it -- that one selects a process to
    // attach to, and a backend attaching by name would take any process of
    // that name.
    Utils::FilePath launchExecutable;

    // QML debug channel for protocol-based backends; empty for native ones.
    QUrl serverUrl;

    // Set once a launched process is running; selects attach-by-pid instead.
    std::atomic<qint64> pid = 0;

    // Runtime control and output.
    std::atomic<int> progress = 0;   // 0..100 post-processing percent.
    std::optional<Utils::Result<Utils::FilePath>> result; // Set on the GUI thread when done.

    // Why the profiled target failed, if it did while the recording was still
    // running (see launchThenCapture()). A target that died before it recorded
    // anything is what explains an empty recording, and explains it better than
    // the backend can, so this is reported in place of the backend's account of
    // it -- but only of that: a trace that was recorded stands, whatever the
    // target went on to exit with (see Sampler::recordRecipe()).
    std::optional<QString> targetError;

    // The debug-information download a backend is waiting on, if any. Post-
    // processing can sit on one for minutes, and the plain progress bar cannot
    // move meanwhile, so the recording page says what is going on instead of
    // looking hung (see PerfSampler::captureRecipe()).
    struct DebugInfoDownload
    {
        int percent = -1; // 0..100; 0 when the size is not known yet, -1 when idle.
        QString url;      // The server being queried, empty when not reported.
    };

    void setDebugInfoDownload(int percent, const QString &url)
    {
        {
            QMutexLocker lock(&m_debugInfoDownloadMutex);
            if (m_debugInfoDownload.percent == percent && m_debugInfoDownload.url == url)
                return;
            m_debugInfoDownload = {percent, url};
        }
        notifyReports();
    }

    // Called for every message that is not a download report, so it has to be
    // cheap and quiet when there is nothing to clear.
    void clearDebugInfoDownload()
    {
        {
            QMutexLocker lock(&m_debugInfoDownloadMutex);
            if (m_debugInfoDownload.percent < 0)
                return;
            m_debugInfoDownload = {};
        }
        notifyReports();
    }

    DebugInfoDownload debugInfoDownload() const
    {
        for (const std::shared_ptr<RecordingSession> &child : m_children) {
            const DebugInfoDownload download = child->debugInfoDownload();
            if (download.percent >= 0)
                return download;
        }
        QMutexLocker lock(&m_debugInfoDownloadMutex);
        return m_debugInfoDownload;
    }

    // A composite backend captures through one sub-session per side (see
    // CombinedSampler). Registering them here is what lets one stop request
    // reach every capture, and lets the parent answer for the whole recording
    // when the GUI asks -- both without a timer copying state across.
    //
    // Called while the recipe is built, before any capture runs, and only from
    // the GUI thread, as every stop request is.
    void addSubSession(const std::shared_ptr<RecordingSession> &child)
    {
        // Weak, and held by the child rather than looked up: the parent already
        // owns the child, and a capture still running may outlive either.
        child->m_parent = weak_from_this();
        m_children.push_back(child);
        // A recording that starts paused has its sides begin paused too.
        if (m_paused)
            child->setPaused(true);
    }

    // Ends the recording: the captures wind down and write what they have.
    // Cascades, so stopping a composite stops both its sides, and one side
    // finishing can end the other.
    void requestStop()
    {
        m_stopRequested = true;
        // Taken by value and cleared first: stopping is a one-shot, and a
        // handler may start something that registers another.
        const std::vector<std::pair<QPointer<QObject>, std::function<void()>>> handlers
            = std::exchange(m_stopHandlers, {});
        // Pausing has no effect from now on (see setPaused()).
        m_pauseHandlers.clear();
        for (const auto &[context, handler] : handlers) {
            if (context)
                handler();
        }
        for (const std::shared_ptr<RecordingSession> &child : m_children)
            child->requestStop();
    }

    // Runs `handler` when the recording is asked to stop: a capture driven by
    // events hooks this to wind itself down, and one that samples in a loop of
    // its own hooks it to cancel the task's promise, which is what that loop
    // watches. As with a Qt connection, `context` bounds the handler's life.
    //
    // Registration and every stop request happen on the GUI thread.
    void onStopRequested(QObject *context, const std::function<void()> &handler)
    {
        if (m_stopRequested) {
            // Already asked. Queued rather than called here, because the caller
            // is still building the task this belongs to -- a timer would not
            // have fired until after that either.
            QMetaObject::invokeMethod(context, handler, Qt::QueuedConnection);
            return;
        }
        m_stopHandlers.emplace_back(context, handler);
    }

    // Suspends or continues capturing without ending the recording. Cascades
    // to sub-sessions like requestStop(). Has no effect once a stop has been
    // requested. GUI thread only. Set before the capture runs, it makes the
    // capture begin paused: a backend that supportsPause() reads isPaused()
    // when it sets up, and records nothing until the first resume.
    void setPaused(bool paused)
    {
        if (m_stopRequested || m_paused == paused)
            return;
        m_paused = paused;
        {
            QMutexLocker lock(&m_pauseIntervalsMutex);
            const qint64 now = steadyNowNs();
            if (paused)
                m_pauseIntervals.push_back({now, -1});
            else if (!m_pauseIntervals.empty())
                m_pauseIntervals.back().second = now;
        }
        for (const auto &[context, handler] : std::vector(m_pauseHandlers)) {
            if (context)
                handler(paused);
        }
        for (const std::shared_ptr<RecordingSession> &child : m_children)
            child->setPaused(paused);
        notifyReports();
    }

    bool isPaused() const { return m_paused; }

    bool isStopRequested() const { return m_stopRequested; }

    // The pauses so far as (start, end) on the steady_clock timeline, in
    // nanoseconds; the end is -1 for one still going on. Safe from any thread.
    // A capture that cannot stop at the moment of a pause, because what it
    // reads is delivered late, judges each sample by when it was taken with
    // this, rather than by isPaused() at the time it gets to it.
    std::vector<std::pair<qint64, qint64>> pausedIntervals() const
    {
        QMutexLocker lock(&m_pauseIntervalsMutex);
        return m_pauseIntervals;
    }

    // Whether the recording was paused at `steadyNs` on the steady_clock
    // timeline, whatever its state is now.
    bool wasPausedAt(qint64 steadyNs) const
    {
        QMutexLocker lock(&m_pauseIntervalsMutex);
        return std::any_of(m_pauseIntervals.cbegin(), m_pauseIntervals.cend(),
                           [steadyNs](const std::pair<qint64, qint64> &interval) {
                               return steadyNs >= interval.first
                                      && (interval.second < 0 || steadyNs < interval.second);
                           });
    }

    // How long capture has been live, not counting its pauses; zero while it
    // is not live yet. A composite is live from when its last side went live.
    std::chrono::nanoseconds recordedTime() const
    {
        qint64 liveSinceNs = startedMonotonicUs.load() * 1000;
        for (const std::shared_ptr<RecordingSession> &child : m_children)
            liveSinceNs = std::max(liveSinceNs, child->startedMonotonicUs.load() * 1000);
        if (!isStarted() || liveSinceNs < 0)
            return {};
        const qint64 now = steadyNowNs();
        qint64 recorded = now - liveSinceNs;
        for (const auto &[start, end] : pausedIntervals()) {
            const qint64 from = std::max(start, liveSinceNs);
            const qint64 to = end < 0 ? now : end;
            if (to > from)
                recorded -= to - from;
        }
        return std::chrono::nanoseconds(std::max<qint64>(0, recorded));
    }

    static qint64 steadyNowNs()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // Runs `handler` with the new state each time setPaused() changes it, for
    // as long as `context` lives. A capture that supports pausing hooks this.
    void onPausedChanged(QObject *context, const std::function<void(bool)> &handler)
    {
        // Unlike a stop, pausing is not a one-shot that clears them, so the
        // handlers whose context has gone are dropped here.
        std::erase_if(m_pauseHandlers, [](const auto &entry) { return !entry.first; });
        m_pauseHandlers.emplace_back(context, handler);
    }

    // Whether capture is live. A composite is only live once both its sides
    // are: until then a timed recording's clock must not start.
    bool isStarted() const
    {
        if (m_children.empty())
            return m_started.load(std::memory_order_relaxed);
        return std::all_of(m_children.cbegin(), m_children.cend(),
                           [](const std::shared_ptr<RecordingSession> &child) {
                               return child->isStarted();
                           });
    }

    // 0..100 for the progress bar. A composite sets this to say how its sides'
    // progress adds up to the whole, which only it knows: the captures are one
    // phase of it, whatever it does with their results is another.
    std::function<int()> progressProvider;

    int progressPercent() const
    {
        return progressProvider ? progressProvider() : progress.load(std::memory_order_relaxed);
    }

    void setProgress(int percent)
    {
        if (progress.exchange(percent, std::memory_order_relaxed) != percent)
            notifyReports();
    }

    // Emits changed() whenever what a frontend shows of this recording moves:
    // progress, the debug-info download, or capture going live. Connect to it
    // instead of reading those on a timer.
    RecordingReporter *reporter() const { return m_reporter.get(); }

    // Announces such a change. Safe from any thread -- the emission is queued
    // onto the reporter's own, which is the GUI thread -- so a capture may
    // report from wherever it happens to work.
    void notifyReports()
    {
        QMetaObject::invokeMethod(m_reporter.get(),
                                  [reporter = m_reporter] { emit reporter->changed(); },
                                  Qt::QueuedConnection);
        // A sub-session's report is the whole recording's, so it carries up to
        // whoever is watching the parent.
        if (const std::shared_ptr<RecordingSession> parent = m_parent.lock())
            parent->notifyReports();
    }

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
        if (!m_started.exchange(true)) {
            startedMonotonicUs.store(std::chrono::duration_cast<std::chrono::microseconds>(
                                         std::chrono::steady_clock::now().time_since_epoch())
                                         .count(),
                                     std::memory_order_relaxed);
            notifyReports();
        }
    }

private:
    // The only state here that is not a lone integer, so it cannot ride on an
    // atomic like the rest: a backend may report from the thread it processes on
    // while the GUI polls, and the percentage and the URL have to agree.
    mutable QMutex m_debugInfoDownloadMutex;
    DebugInfoDownload m_debugInfoDownload;

    // Written by markStarted() and read through isStarted(), which is what a
    // composite has to answer for its sub-sessions rather than for itself.
    std::atomic_bool m_started = false;

    // Only so a handler registered after the fact still runs. Nothing polls a
    // stop any more: a capture with a loop of its own watches its task's
    // promise, everyone else is called (see requestStop()).
    bool m_stopRequested = false;
    // Atomic because a sampling thread polls it (see SamplerOptions::isPaused).
    std::atomic_bool m_paused = false;
    mutable QMutex m_pauseIntervalsMutex;
    std::vector<std::pair<qint64, qint64>> m_pauseIntervals;
    std::vector<std::pair<QPointer<QObject>, std::function<void(bool)>>> m_pauseHandlers;

    std::vector<std::shared_ptr<RecordingSession>> m_children;
    std::weak_ptr<RecordingSession> m_parent;
    std::vector<std::pair<QPointer<QObject>, std::function<void()>>> m_stopHandlers;

    // Deleted through deleteLater(), so that a worker thread dropping the last
    // reference to the session does not destroy a QObject off its own thread.
    std::shared_ptr<RecordingReporter> m_reporter{new RecordingReporter,
                                                  [](RecordingReporter *r) { r->deleteLater(); }};
};

// Why a recording that was stopped before its first resume has no trace.
PROFILER_EXPORT QString stoppedBeforeResumeMessage();
// Why one whose target went away before its first resume has none.
PROFILER_EXPORT QString exitedBeforeResumeMessage();

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

    // What the frontend last put into the three fields above (see
    // ProfilerRecorder::seedLaunchTarget). Persisted, so a value the user typed
    // over a seeded one still reads as theirs after a restart. Never shown.
    Utils::FilePathAspect seededExecutable{this};
    Utils::StringAspect seededArguments{this};
    Utils::FilePathAspect seededWorkingDirectory{this};

    // Builds a RecordingSession from the current settings, or an error explaining
    // what is missing/misconfigured (e.g. no executable, or attach with no process).
    virtual Utils::Result<std::shared_ptr<RecordingSession>> createSession() const = 0;

    // Builds a RecordingSession for a target something else launches -- Qt
    // Creator's run machinery, say (see profilersamplerruncontrol.cpp). It
    // carries the backend's own options and no target of its own, so it cannot
    // be missing one and never fails.
    std::shared_ptr<RecordingSession> createRunControlSession() const;

    // Set while a target is chosen elsewhere. The settings that pick one -- the
    // launch fields, and any attach or connect alternative -- then go read-only
    // rather than away, so the backend's remaining options stay usable.
    void setTargetChosenElsewhere(bool chosen);
    bool targetChosenElsewhere() const { return m_targetChosenElsewhere; }

    // Set while the backend's own options come from elsewhere too -- a live
    // profiler that records a target on a device with settings of its own.
    // They then go read-only as well.
    void setOptionsChosenElsewhere(bool chosen);
    bool optionsChosenElsewhere() const { return m_optionsChosenElsewhere; }

signals:
    // Emitted when setTargetChosenElsewhere() changes what may be picked, for
    // the controls that are not aspects (a "Select Process..." button, say) and
    // that updateTargetEnabled() therefore cannot reach.
    void targetSelectionChanged();
    // Likewise for setOptionsChosenElsewhere() and updateOptionsEnabled().
    void optionsSelectionChanged();

protected:
    // Populates session->launchCommand/launchWorkingDir from executable+arguments;
    // fails if no executable is set.
    Utils::Result<> fillLaunch(RecordingSession &session) const;

    // Copies the backend's own options -- a sampling cadence, the features to
    // record -- onto `session`, leaving its target alone. Both ways of building
    // a session go through this, so the two cannot drift apart.
    virtual void fillOptions(RecordingSession &session) const;

    // Applies targetChosenElsewhere() to the settings that pick a target. The
    // base handles the launch fields; a backend offering an attach or connect
    // alternative extends this.
    virtual void updateTargetEnabled();

    // Applies optionsChosenElsewhere() to the backend's own options. The base
    // has none.
    virtual void updateOptionsEnabled();

private:
    bool m_targetChosenElsewhere = false;
    bool m_optionsChosenElsewhere = false;
};

// A repair for a system setting that stops a backend from recording, offered to
// the user as a button next to the failure. The command is run as root, so it
// names one specific change and nothing else.
struct SamplerFix
{
    QString buttonText;         // Button label, e.g. "Allow Sampling".
    QString detail;             // What applying it does, shown with the error.
    Utils::CommandLine command; // Run with root privileges by the UI.
};

// A profiling backend: records a trace of a target process until the session is
// stopped, then writes it and reports the resulting trace directory.
//
// Implementations differ in how they capture (call-stack sampling today; a QML
// profiler or perf-based recorder could be added alongside). Each returns a
// QtTaskTree recipe so the window can compose it with process launching.
// Identifies a backend to a frontend that has its own way of profiling the same
// thing, and does not change when the display name is translated.
namespace SamplerIds {
inline constexpr char CallStack[] = "Profiler.Sampler.CallStack";
inline constexpr char Perf[]      = "Profiler.Sampler.Perf";
inline constexpr char Qml[]       = "Profiler.Sampler.Qml";
inline constexpr char QtTrace[]   = "Profiler.Sampler.QtTrace";
inline constexpr char Combined[]  = "Profiler.Sampler.Combined";
} // namespace SamplerIds

class PROFILER_EXPORT Sampler
{
public:
    virtual ~Sampler() = default;

    virtual Utils::Id id() const = 0;
    virtual QString displayName() const = 0;

    // Whether this backend can run in the current environment; when it cannot,
    // fills *error with a human-readable reason.
    virtual bool isAvailable(QString *error = nullptr) const = 0;

    // Whether this backend is offered for selection. A backend that belongs on
    // this platform but cannot record yet -- a tool it drives is not installed,
    // say -- stays on offer, so that starting it reports what is missing instead
    // of the backend silently not being there. By default, what is available.
    virtual bool isOffered() const { return isAvailable(); }

    // The complete recipe that records the target described by `session`: it
    // prepares and launches session->launchCommand (when set) and captures the
    // target until a stop is requested, storing its Result into session->result
    // when done. Concrete: it calls prepareLaunch() and then wraps captureRecipe() in
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
    // launched. Reads session->pid / session->serverUrl and
    // stores the trace path into session->result. recordRecipe() wraps this in
    // launchThenCapture(); a composite backend calls it directly for each
    // backend it wraps.
    virtual QtTaskTree::ExecutableItem captureRecipe(
        const std::shared_ptr<RecordingSession> &session) const = 0;

    // Completes the recording once the capture is done and the target it was
    // launched for, if any, has ended. A backend whose trace the target writes
    // itself finishes here rather than in captureRecipe(): the process is
    // stopped only after the capture is done with it, and what it writes on the
    // way out -- the events Qt's CTF backend flushes as it exits -- belongs to
    // the recording (see QtTraceSampler). Called by whoever composed launch and
    // capture, and before session->result is read (see recordRecipe() and
    // profilersamplerruncontrol.cpp). The default does nothing.
    virtual void completeRecording(const std::shared_ptr<RecordingSession> &session) const;

    // Whether the target has to be started with a QML debug server for this
    // backend to capture it. prepareLaunch() arranges that for a launch the
    // backend composes itself; a frontend launching through Qt Creator's run
    // machinery has to arrange it on the run control instead, and asks this.
    virtual bool needsQmlChannel() const { return false; }

    // Whether a recording can be paused and resumed. The backend then begins
    // paused when its session is, and hooks RecordingSession::onPausedChanged().
    virtual bool supportsPause() const { return false; }

    // Backend-specific recording settings, which also render the backend's own
    // configuration controls and attach/connect start buttons (see SamplerSettings).
    // Null when the backend has no options. Owned by the backend.
    virtual SamplerSettings *settings() const { return nullptr; }

    // A system setting the user can change to make recording work, queried after
    // a recording failed. Re-reads the setting rather than interpreting the error,
    // so it only offers a change that is still needed. None by default.
    virtual std::optional<SamplerFix> availableFix() const { return std::nullopt; }
};

} // namespace Profiler::Internal
