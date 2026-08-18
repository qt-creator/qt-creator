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

#include <QCoreApplication>
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

    void beginRecording(const QString &target);
    void finishRecording();
    Sampler *backend() const;

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
    QtTaskTree::QSingleTaskTreeRunner runner;
    QTimer poll;
    int downloadPolls = 0;                        // Consecutive polls seeing a download.
    static constexpr int downloadPollsBeforeReporting = 6; // ~300 ms at a 50 ms poll.
    bool recording = false;
    bool processingShown = false;         // processingStarted() already emitted.
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

    poll.setInterval(50);
    connect(&poll, &QTimer::timeout, this, [this] {
        if (!session)
            return;
        // The recipe (or the user) may set stop on its own, e.g. because the
        // target exited; reflect the switch to post-processing exactly once.
        if (session->stop.load(std::memory_order_relaxed) && !processingShown) {
            processingShown = true;
            emit q->processingStarted();
        }
        // For startTimed(): once capture is actually live, start the span clock
        // exactly once, so launch and connect time is not counted against it.
        if (duration && !durationArmed && session->started.load(std::memory_order_relaxed)) {
            durationArmed = true;
            QTimer::singleShot(*duration, this, [this] {
                if (session)
                    session->stop.store(true);
            });
        }
        emit q->progressChanged(session->progress.load(std::memory_order_relaxed));

        // A debug-info download holds up post-processing without moving the
        // progress bar, for as long as the server takes to answer -- which may be
        // forever. Name it, so the wait is at least attributable (the Perf
        // backend's "Download missing debug information" setting turns it off).
        // A query answered from this machine passes through the same state in
        // microseconds, so only report one that has lasted long enough to be a
        // wait somebody is sitting through.
        const RecordingSession::DebugInfoDownload download = session->debugInfoDownload();
        if (download.percent < 0) {
            downloadPolls = 0;
            emit q->statusChanged({}, {});
        } else if (++downloadPolls >= downloadPollsBeforeReporting) {
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
            // The line names the servers; the whole URL, which carries a build id
            // long enough to crowd everything else out, goes in the tool tip.
            emit q->statusChanged(text, download.url);
        }
    });
}

Sampler *ProfilerRecorderPrivate::backend() const
{
    if (current < 0 || current >= int(offered.size()))
        return nullptr;
    return backends[offered[current]].get();
}

void ProfilerRecorderPrivate::beginRecording(const QString &target)
{
    recording = true;
    processingShown = false;
    downloadPolls = 0;
    emit q->started(target);
    poll.start();

    // The backend owns the complete recipe: it launches the target (when
    // session->launchCommand is set), captures it, and tears it down.
    runner.start(QtTaskTree::Group{backend()->recordRecipe(session)}, [] {},
                 [this](QtTaskTree::DoneWith) { finishRecording(); });
}

void ProfilerRecorderPrivate::finishRecording()
{
    recording = false;
    poll.stop();
    duration.reset(); // One-shot: do not auto-stop a later manual recording.
    durationArmed = false;

    const std::shared_ptr<RecordingSession> finished = session;
    session.reset();

    if (waitingForShutdown)
        return; // Shutting down: don't hand a trace to a frontend that is going away.

    if (!finished || !finished->result) {
        emit q->error(Tr::tr("Recording did not produce a trace."));
        return;
    }
    const Result<FilePath> &result = *finished->result;
    if (!result) {
        Sampler *sampler = backend();
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

void ProfilerRecorder::stop()
{
    if (d->session)
        d->session->stop.store(true);
}

void ProfilerRecorder::stopAndWait()
{
    if (!d->recording || !d->session)
        return;
    // The sampler loops until stopped; ask it to finish and pump events until
    // the task tree and its worker thread have wound down.
    d->waitingForShutdown = true;
    d->session->stop.store(true);
    while (d->recording)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
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
