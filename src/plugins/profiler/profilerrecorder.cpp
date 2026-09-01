// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilerrecorder.h"

#include "callstacksampler.h"
#include "combinedsampler.h"
#include "perfsampler.h"
#include "profilertr.h"
#include "qmlprofilersampler.h"
#include "sampler.h"

#include <utils/commandline.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QRegularExpression>
#include <QUrl>
#include <QTimer>
#include <QWidget>

#include <memory>
#include <optional>
#include <vector>

using namespace Utils;
using namespace Qt::StringLiterals;

using namespace std::chrono;

namespace Profiler::Internal {

// The host names behind a whitespace-separated URL list, for the recording page's
// status line. What perfparser reports is either one request URL, whose build-id
// path says nothing a user needs, or the whole DEBUGINFOD_URLS list from before a
// request was made -- in both cases the host is the part that identifies who is
// being waited on, and the only part short enough for one line.
static QString debugInfoServerNames(const QString &urls)
{
    static const QRegularExpression whitespace("\\s+"_L1);
    QStringList hosts;
    const QStringList entries = urls.split(whitespace, Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        const QString host = QUrl(entry).host();
        hosts << (host.isEmpty() ? entry : host);
    }
    hosts.removeDuplicates();
    return hosts.join(", "_L1);
}

class ProfilerRecorderPrivate : public QObject
{
public:
    explicit ProfilerRecorderPrivate(ProfilerRecorder *recorder);

    void startRecording(const QString &target);
    void beginRecording(const QString &target);
    void finishRecording();
    // Passes on what the session reports; connected to its reporter.
    void updateReports();
    Sampler *backend() const;
    Sampler *backendById(Utils::Id id) const;

    ProfilerRecorder *q = nullptr;

    std::vector<std::unique_ptr<Sampler>> backends;
    // Indices into `backends` that are usable here (see Sampler::isAvailable),
    // in display order. This is what the frontend offers.
    std::vector<int> offered;
    // The environment to launch in. It comes from the frontend's target rather
    // than the settings, so it is not persisted with them.
    Environment seededEnvironment;
    int current = 0; // Index into `offered`.

    std::shared_ptr<RecordingSession> session; // Non-null while recording.
    // The backend of the running recording, which outlives a change of the
    // selected one -- and which the run-control path does not select at all.
    Sampler *active = nullptr;
    QtTaskTree::QSingleTaskTreeRunner runner;
    bool ownsRecipe = false; // The recorder runs the capture itself.
    QTimer downloadDelay;    // Before a debug-info download is worth naming.
    bool downloadNameable = false;
    bool recording = false;
    bool waitingForShutdown = false;      // Set while stopAndWait() runs.
    std::optional<milliseconds> duration; // Set by startTimed(); auto-stop span.
    bool durationArmed = false;           // Stop timer armed once capture went live.
};

ProfilerRecorderPrivate::ProfilerRecorderPrivate(ProfilerRecorder *recorder)
    : QObject(recorder)
    , q(recorder)
{
    // The native call-stack samplers first (macOS mach-based, Linux perf-based),
    // then the QML-protocol profiler, then the composite that records a native
    // sampler and the QML profiler against one target at once (see
    // design-docs/native-mixed-profiler-design.md).
    backends.push_back(std::make_unique<CallStackSampler>());
    backends.push_back(std::make_unique<PerfSampler>());
    backends.push_back(std::make_unique<QmlProfilerSampler>());
    backends.push_back(std::make_unique<CombinedSampler>());

    for (const std::unique_ptr<Sampler> &backend : backends) {
        if (SamplerSettings *settings = backend->settings())
            settings->readSettings();
    }

    for (int i = 0; i < int(backends.size()); ++i) {
        if (backends[i]->isAvailable())
            offered.push_back(i);
    }
    // Should not happen -- QmlProfilerSampler is available everywhere -- but an
    // empty selector would leave the frontend with nothing to explain.
    if (offered.empty()) {
        for (int i = 0; i < int(backends.size()); ++i)
            offered.push_back(i);
    }

    // A query answered from this machine passes through the download state in
    // microseconds, so a download is only named once it has lasted long enough
    // to be a wait somebody is sitting through. One shot, armed when a download
    // appears: a delay, not a watch.
    downloadDelay.setSingleShot(true);
    downloadDelay.setInterval(300ms);
    connect(&downloadDelay, &QTimer::timeout, this, [this] {
        downloadNameable = true;
        updateReports();
    });
}

void ProfilerRecorderPrivate::updateReports()
{
    if (!session)
        return;
    // For startTimed(): once capture is actually live, start the span clock
    // exactly once, so launch and connect time is not counted against it.
    if (duration && !durationArmed && session->isStarted()) {
        durationArmed = true;
        QTimer::singleShot(*duration, this, [this] {
            if (session)
                session->requestStop();
        });
    }
    emit q->progressChanged(session->progressPercent());

    // A debug-info download holds up post-processing without moving the
    // progress bar, for as long as the server takes to answer -- which may be
    // forever. Name it, so the wait is at least attributable (the Perf
    // backend's "Download missing debug information" setting turns it off).
    const RecordingSession::DebugInfoDownload download = session->debugInfoDownload();
    if (download.percent < 0) {
        downloadDelay.stop();
        downloadNameable = false;
        emit q->statusChanged({}, {});
        return;
    }
    if (!downloadNameable) {
        if (!downloadDelay.isActive())
            downloadDelay.start();
        return;
    }
    const QString server = debugInfoServerNames(download.url);
    QString text;
    if (server.isEmpty()) {
        text = download.percent == 0
                   ? Tr::tr("Downloading debug information...")
                   : Tr::tr("Downloading debug information... %1%").arg(download.percent);
    } else {
        text = download.percent == 0
                   ? Tr::tr("Downloading debug information from %1...").arg(server)
                   : Tr::tr("Downloading debug information from %1... %2%")
                         .arg(server).arg(download.percent);
    }
    // The line names the servers; the whole URL, which carries a build id long
    // enough to crowd everything else out, goes in the tool tip.
    emit q->statusChanged(text, download.url);
}

Sampler *ProfilerRecorderPrivate::backend() const
{
    if (current < 0 || current >= int(offered.size()))
        return nullptr;
    return backends[offered[current]].get();
}

Sampler *ProfilerRecorderPrivate::backendById(Id id) const
{
    for (int index : offered) {
        if (backends[index]->id() == id)
            return backends[index].get();
    }
    return nullptr;
}

void ProfilerRecorderPrivate::startRecording(const QString &target)
{
    recording = true;
    downloadNameable = false;
    // The recipe (or the user) ends the capture; either way that is the switch
    // from recording to post-processing. Reported from the request itself, so
    // nothing has to watch the flag for it.
    session->onStopRequested(this, [this] { emit q->processingStarted(); });
    // Likewise for progress, the debug-info download and capture going live:
    // the session says when they move, rather than being asked.
    connect(session->reporter(), &RecordingReporter::changed,
            this, &ProfilerRecorderPrivate::updateReports);
    emit q->started(target);
}

void ProfilerRecorderPrivate::beginRecording(const QString &target)
{
    ownsRecipe = true;
    startRecording(target);

    // The backend owns the complete recipe: it launches the target (when
    // session->launchCommand is set), captures it, and tears it down.
    runner.start(QtTaskTree::Group{active->recordRecipe(session)}, [] {},
                 [this](QtTaskTree::DoneWith) { finishRecording(); });
}

void ProfilerRecorderPrivate::finishRecording()
{
    if (!recording)
        return;
    recording = false;
    ownsRecipe = false;
    downloadDelay.stop();
    duration.reset(); // One-shot: do not auto-stop a later manual recording.
    durationArmed = false;

    const std::shared_ptr<RecordingSession> finished = session;
    session.reset();
    Sampler *sampler = active;
    active = nullptr;

    if (waitingForShutdown)
        return; // Shutting down: don't hand a trace to a frontend that is going away.

    if (!finished || !finished->result) {
        emit q->error(Tr::tr("Recording did not produce a trace."));
        return;
    }
    const Result<FilePath> &result = *finished->result;
    if (!result) {
        emit q->error(result.error(), sampler ? sampler->availableFix() : std::nullopt);
        return;
    }
    emit q->finished(*result);
}

ProfilerRecorder::ProfilerRecorder(QObject *parent)
    : QObject(parent)
    , d(new ProfilerRecorderPrivate(this))
{}

ProfilerRecorder::~ProfilerRecorder()
{
    stopAndWait();
}

QStringList ProfilerRecorder::backendNames() const
{
    QStringList names;
    for (int index : d->offered)
        names << d->backends[index]->displayName();
    return names;
}

QList<Id> ProfilerRecorder::backendIds() const
{
    QList<Id> ids;
    for (int index : d->offered)
        ids << d->backends[index]->id();
    return ids;
}

void ProfilerRecorder::setTargetChosenElsewhere(bool chosen)
{
    for (const std::unique_ptr<Sampler> &backend : d->backends) {
        if (SamplerSettings *settings = backend->settings())
            settings->setTargetChosenElsewhere(chosen);
    }
}

Sampler *ProfilerRecorder::backendById(Id id) const
{
    return d->backendById(id);
}

int ProfilerRecorder::currentBackend() const
{
    return d->current;
}

void ProfilerRecorder::setCurrentBackend(int index)
{
    if (index < 0 || index >= int(d->offered.size()) || index == d->current)
        return;
    d->current = index;
    emit currentBackendChanged(index);
}

bool ProfilerRecorder::selectBackend(const QString &name)
{
    for (int i = 0; i < int(d->offered.size()); ++i) {
        if (d->backends[d->offered[i]]->displayName().contains(name, Qt::CaseInsensitive)) {
            setCurrentBackend(i);
            return true;
        }
    }
    return false;
}

QWidget *ProfilerRecorder::createConfigWidget() const
{
    Sampler *backend = d->backend();
    SamplerSettings *settings = backend ? backend->settings() : nullptr;
    if (!settings)
        return nullptr;
    auto widget = new QWidget;
    settings->layouter()().attachTo(widget);
    return widget;
}

void ProfilerRecorder::seedLaunchTarget(const CommandLine &command,
                                        const FilePath &workingDirectory,
                                        const Environment &environment)
{
    // A field the user has typed over reads as neither empty nor what we put
    // there last time, and is left alone.
    const auto seed = [](auto &field, auto &lastSeeded, const auto &value) {
        if (!field().isEmpty() && field() != lastSeeded())
            return;
        field.setValue(value);
        lastSeeded.setValue(value);
    };

    // Every backend launches through the same three settings, so seeding all of
    // them keeps the target when the user switches backend.
    for (const std::unique_ptr<Sampler> &backend : d->backends) {
        SamplerSettings *settings = backend->settings();
        if (!settings)
            continue;
        seed(settings->executable, settings->seededExecutable, command.executable());
        seed(settings->arguments, settings->seededArguments, command.arguments());
        seed(settings->workingDirectory, settings->seededWorkingDirectory, workingDirectory);
    }
    d->seededEnvironment = environment;
}

bool ProfilerRecorder::isRecording() const
{
    return d->recording;
}

void ProfilerRecorder::start()
{
    if (d->recording)
        return;

    Sampler *backend = d->backend();
    QTC_ASSERT(backend, return);

    QString reason;
    if (!backend->isAvailable(&reason)) {
        emit error(reason);
        return;
    }

    SamplerSettings *settings = backend->settings();
    if (!settings) {
        emit error(Tr::tr("This backend cannot be configured for recording."));
        return;
    }

    const Result<std::shared_ptr<RecordingSession>> created = settings->createSession();
    if (!created) {
        emit error(created.error());
        return;
    }
    d->session = *created;
    d->session->launchEnvironment = d->seededEnvironment;
    d->active = backend;

    // Name the recording for the frontend: the launched command, the attach
    // target, or the connect endpoint.
    QString target;
    if (d->session->launchCommand)
        target = d->session->launchCommand->executable().fileName();
    else if (!d->session->processName.isEmpty())
        target = d->session->processName;
    else if (!d->session->serverUrl.isEmpty())
        target = d->session->serverUrl.host() + u':' + QString::number(d->session->serverUrl.port());
    d->beginRecording(target);
}

void ProfilerRecorder::startTimed(milliseconds duration)
{
    start();
    if (!d->recording)
        return; // start() reported the error already.
    d->duration = duration;
    d->durationArmed = false;
}

Result<std::shared_ptr<RecordingSession>> ProfilerRecorder::beginRunControlRecording(
    Id backendId, const QString &target)
{
    if (d->recording)
        return ResultError(Tr::tr("A recording is already running."));

    Sampler *backend = d->backendById(backendId);
    QTC_ASSERT(backend, return ResultError(Tr::tr("Unknown profiling backend.")));

    QString reason;
    if (!backend->isAvailable(&reason))
        return ResultError(reason);

    SamplerSettings *settings = backend->settings();
    if (!settings)
        return ResultError(Tr::tr("This backend cannot be configured for recording."));

    d->active = backend;
    d->session = settings->createRunControlSession();
    d->startRecording(target);
    return d->session;
}

void ProfilerRecorder::endRunControlRecording(const std::shared_ptr<RecordingSession> &session)
{
    if (d->session != session)
        return;
    if (!d->ownsRecipe)
        d->finishRecording();
}

void ProfilerRecorder::stop()
{
    if (d->session)
        d->session->requestStop();
}

void ProfilerRecorder::stopAndWait()
{
    if (!d->recording || !d->session)
        return;
    // Whatever the trace still becomes is not for a frontend that is going
    // away, so leave the recording unreported.
    d->waitingForShutdown = true;

    // Ask the capture to end on its own terms first: the sampling worker polls
    // this flag, and perf's recipe stops "perf record" by it.
    d->session->requestStop();

    // A capture the run machinery drives winds down with its run control, which
    // is not ours to end. Both callers are shutting down, so nothing follows
    // that would want waitingForShutdown cleared.
    if (!d->ownsRecipe)
        return;

    // Ours is, so end it. Cancelling runs the recipe's done handler, leaving
    // the recording finished once this returns, and it is what bounds the wait:
    // a backend that would not come back on its own -- perfparser stalled on an
    // unresponsive debuginfod server, say -- has its tasks cancelled rather
    // than waited for.
    d->runner.cancel();
    d->waitingForShutdown = false;
}

void ProfilerRecorder::writeSettings() const
{
    for (const std::unique_ptr<Sampler> &backend : d->backends) {
        if (SamplerSettings *settings = backend->settings())
            settings->writeSettings();
    }
}

} // namespace Profiler::Internal
