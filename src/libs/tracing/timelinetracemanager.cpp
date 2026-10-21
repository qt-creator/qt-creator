// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinenotesmodel.h"
#include "timelinetracefile.h"
#include "timelinetracemanager.h"
#include "tracingtr.h"

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcassert.h>

#include <QFile>
#include <QFutureWatcher>
#include <QPointer>

#include <memory>
#include <optional>

namespace Timeline {

TraceEventTypeStorage::~TraceEventTypeStorage()
{
}

TraceEventStorage::~TraceEventStorage()
{
}

class TimelineTraceManager::TimelineTraceManagerPrivate
{
public:
    TimelineTraceManager *q = nullptr;

    std::unique_ptr<TraceEventTypeStorage> typeStorage;
    std::unique_ptr<TraceEventStorage> eventStorage;

    TimelineNotesModel *notesModel = nullptr;

    int numEvents = 0;
    quint64 availableFeatures = 0;
    quint64 visibleFeatures = 0;
    quint64 recordedFeatures = 0;
    bool aggregateTraces = false;

    QList<Initializer> initializers;
    QList<Finalizer> finalizers;
    QList<Preparer> preparers;
    QList<Clearer> clearers;

    qint64 traceStart = -1;
    qint64 traceEnd = -1;
    bool prepared = false;

    // A load() worker thread keeps calling loadEvent() on the timeline models
    // until it notices cancellation, so a new load must not clearAll() that
    // state underneath it. Cancel the running one and defer the new one.
    QPointer<TimelineTraceFile> activeLoadReader;
    struct QueuedLoad {
        QString filename;
        QFutureInterface<void> fi;
    };
    std::optional<QueuedLoad> queuedLoad;

    // Saves still running when the manager is destroyed; the workers
    // dereference the manager and its writer, so the destructor joins them.
    QList<QFuture<void>> saveFutures;

    // Replays running in another thread, by who started them: the models they fill belong to
    // that thread until it reports back. A restriction of all models has the manager as key.
    struct Replay
    {
        const void *key = nullptr;
        QFuture<void> future;
        std::shared_ptr<bool> superseded;
        std::function<void(bool)> complete; // Finalizes, once the thread is done.
    };
    QList<Replay> replays;
    bool replaying = false; // Between restrictionStarted() and restrictionFinished().

    void cancelQueuedLoad();
    void cancelRestriction() { cancelReplays(nullptr, true); }
    void cancelReplays(const void *key, bool mayFinish);
    void waitForReplays(const void *key);
    void finishReplaying();
    void dispatch(const TraceEvent &event, const TraceEventType &type);
    void reset();
    void updateTraceTime(qint64 time);
};

TimelineTraceManager::TimelineTraceManager(std::unique_ptr<TraceEventStorage> &&eventStorage,
                                           std::unique_ptr<TraceEventTypeStorage> &&typeStorage,
                                           QObject *parent) :
    QObject(parent), d(new TimelineTraceManagerPrivate)
{
    d->q = this;
    d->eventStorage = std::move(eventStorage);
    d->typeStorage = std::move(typeStorage);
}

TimelineTraceManager::~TimelineTraceManager()
{
    // A save still running dereferences this manager and its writer from a
    // worker thread; wait for it. Not cancelled: the write may be the very one
    // the user asked for when closing the trace, and cancelling would remove
    // the file.
    for (QFuture<void> &future : d->saveFutures)
        future.waitForFinished();

    d->cancelRestriction();
    d->cancelQueuedLoad();

    // The notes model is a child QObject and would otherwise be destroyed by ~QObject's
    // deleteChildren(), i.e. *after* this destructor frees d below. Its destruction emits
    // notesChanged, which fans out to views querying eventType()/the storages. Delete it
    // here, while d (and the storages it owns) is still alive.
    delete d->notesModel;
    delete d;
}

TimelineNotesModel *TimelineTraceManager::notesModel() const
{
    return d->notesModel;
}

bool TimelineTraceManager::isEmpty() const
{
    return d->eventStorage->size() == 0;
}

int TimelineTraceManager::numEvents() const
{
    return d->numEvents;
}

int TimelineTraceManager::numEventTypes() const
{
    return d->typeStorage->size();
}

void TimelineTraceManager::swapEventStorage(std::unique_ptr<TraceEventStorage> &other)
{
    d->eventStorage.swap(other);
}

const TraceEventStorage *TimelineTraceManager::eventStorage() const
{
    return d->eventStorage.get();
}

const TraceEventTypeStorage *TimelineTraceManager::typeStorage() const
{
    return d->typeStorage.get();
}

void TimelineTraceManager::appendEvent(TraceEvent &&event)
{
    d->dispatch(event, d->typeStorage->get(event.typeIndex()));
    d->eventStorage->append(std::move(event));
}

const TraceEventType &TimelineTraceManager::eventType(int typeId) const
{
    return d->typeStorage->get(typeId);
}

void TimelineTraceManager::setEventType(int typeId, TraceEventType &&type)
{
    d->recordedFeatures |= (1ull << type.feature());
    d->typeStorage->set(typeId, std::move(type));
}

int TimelineTraceManager::appendEventType(TraceEventType &&type)
{
    d->recordedFeatures |= (1ull << type.feature());
    return d->typeStorage->append(std::move(type));
}

void TimelineTraceManager::registerFeatures(quint64 features, Initializer initializer,
                                            Finalizer finalizer, Clearer clearer,
                                            Preparer preparer)
{
    if ((features & d->availableFeatures) != features) {
        d->availableFeatures |= features;
        emit availableFeaturesChanged(d->availableFeatures);
    }

    if ((features & d->visibleFeatures) != features) {
        d->visibleFeatures |= features;
        emit visibleFeaturesChanged(d->visibleFeatures);
    }

    if (initializer)
        d->initializers.append(initializer);

    if (finalizer)
        d->finalizers.append(finalizer);

    if (clearer)
        d->clearers.append(clearer);
    if (preparer)
        d->preparers.append(preparer);
}

quint64 TimelineTraceManager::availableFeatures() const
{
    return d->availableFeatures;
}

quint64 TimelineTraceManager::visibleFeatures() const
{
    return d->visibleFeatures;
}

void TimelineTraceManager::setVisibleFeatures(quint64 features)
{
    if (d->visibleFeatures != features) {
        d->visibleFeatures = features;
        emit visibleFeaturesChanged(d->visibleFeatures);
    }
}

quint64 TimelineTraceManager::recordedFeatures() const
{
    return d->recordedFeatures;
}

void TimelineTraceManager::setRecordedFeatures(quint64 features)
{
    if (d->recordedFeatures != features) {
        d->recordedFeatures = features;
        emit recordedFeaturesChanged(d->recordedFeatures);
    }
}

bool TimelineTraceManager::aggregateTraces() const
{
    return d->aggregateTraces;
}

void TimelineTraceManager::setAggregateTraces(bool aggregateTraces)
{
    d->aggregateTraces = aggregateTraces;
}

void TimelineTraceManager::initialize()
{
    for (const Initializer &initializer : std::as_const(d->initializers))
        initializer();
}

// Does what the preparers registered with registerFeatures() do: the processing of the events
// that finalize() then publishes. A trace file loading its events in another thread calls this
// there, while nothing else uses the models; otherwise finalize() calls it. With a future,
// progress continues on it.
void TimelineTraceManager::prepare(QFutureInterface<void> *future)
{
    d->eventStorage->finalize();

    // Each preparer gets an equal share of a tenth of the progress range loading had.
    const int progressStart = future ? future->progressMaximum() : 0;
    const int progressShare = progressStart / 10;
    if (future)
        future->setProgressRange(future->progressMinimum(), progressStart + progressShare);
    for (int i = 0; i < d->preparers.size(); ++i) {
        d->preparers.at(i)();
        if (future)
            future->setProgressValue(progressStart + progressShare * (i + 1) / d->preparers.size());
    }
    d->prepared = true;
}

void TimelineTraceManager::finalize()
{
    if (!d->prepared)
        prepare();
    d->prepared = false;

    // Load notes after the timeline models have been initialized ...
    // which happens on stateChanged(Done).

    for (const Finalizer &finalizer : std::as_const(d->finalizers))
        finalizer();
}

QFuture<void> TimelineTraceManager::save(const QString &filename)
{
    TimelineTraceFile *writer = createTraceFile();
    writer->setTraceTime(traceStart(), traceEnd(), traceDuration());
    writer->setTraceManager(this);
    writer->setNotes(d->notesModel);

    if (d->notesModel)
        d->notesModel->stash();

    QFutureInterface<void> fi;
    fi.reportStarted();
    writer->setFuture(fi);

    // stash() copies the notes into what is written, but only a write that made
    // it out whole may clear their modified state: until then closing the trace
    // must still count them as unsaved.
    const auto hadError = std::make_shared<bool>(false);
    connect(writer, &TimelineTraceFile::error, this, [this, hadError](const QString &message) {
        *hadError = true;
        emit error(message);
    });
    connect(writer, &QObject::destroyed, this, [this, hadError, future = fi.future()] {
        if (!*hadError && !future.isCanceled() && d->notesModel)
            d->notesModel->resetModified();
        emit saveFinished();
    });

    d->saveFutures.removeIf([](const QFuture<void> &f) { return f.isFinished(); });
    d->saveFutures.append(fi.future());

    Utils::asyncRun([filename, writer, fi] {
        QFile file(filename);

        if (file.open(QIODevice::WriteOnly))
            writer->save(&file);
        else
            writer->fail(Tr::tr("Could not open %1 for writing.").arg(filename));

        if (fi.isCanceled())
            file.remove();
        writer->deleteLater();
        QFutureInterface fiCopy = fi;
        fiCopy.reportFinished();
    });
    return fi.future();
}

QFuture<void> TimelineTraceManager::load(const QString &filename)
{
    QFutureInterface<void> fi;
    fi.reportStarted();

    if (d->activeLoadReader) {
        d->cancelQueuedLoad();
        d->queuedLoad = TimelineTraceManagerPrivate::QueuedLoad{filename, fi};

        // The superseded load is not the one whose completion the clients wait for.
        disconnect(d->activeLoadReader.data(), &QObject::destroyed,
                   this, &TimelineTraceManager::loadFinished);
        d->activeLoadReader->future().cancel();
        return fi.future();
    }

    startLoad(filename, fi);
    return fi.future();
}

/*!
    Cancels a load that is running or queued, and a range analysis that replays the events, and
    waits until the threads they run in no longer call into the manager and the models
    registered with it. Call it before deleting any of them while a trace may still be loading
    or analyzed.
*/
void TimelineTraceManager::cancelBackgroundWork()
{
    d->cancelRestriction();
    d->cancelQueuedLoad();
    if (TimelineTraceFile *reader = d->activeLoadReader) {
        QFutureInterface<void> future = reader->future();
        future.cancel();
        future.waitForFinished();
    }
}

void TimelineTraceManager::startLoad(const QString &filename, QFutureInterface<void> fi)
{
    d->cancelRestriction();
    clearAll();
    initialize();
    TimelineTraceFile *reader = createTraceFile();
    reader->setTraceManager(this);
    reader->setNotes(d->notesModel);
    d->activeLoadReader = reader;

    connect(reader, &QObject::destroyed, this, &TimelineTraceManager::loadFinished);
    connect(reader, &TimelineTraceFile::error, this, &TimelineTraceManager::error);

    reader->setFuture(fi);
    Utils::asyncRun([filename, reader, fi] {
        QFile file(filename);

        if (file.open(QIODevice::ReadOnly))
            reader->load(&file);
        else
            reader->fail(Tr::tr("Could not open %1 for reading.").arg(filename));

        reader->deleteLater();
        QFutureInterface fiCopy = fi;
        fiCopy.reportFinished();
    });

    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(reader);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, reader] {
        if (reader->isCanceled()) {
            // Only now the worker thread has stopped calling loadEvent().
            clearAll();
        } else {
            if (reader->traceStart() >= 0)
                decreaseTraceStart(reader->traceStart());
            if (reader->traceEnd() >= 0)
                increaseTraceEnd(reader->traceEnd());
            finalize();

            if (d->notesModel)
                d->notesModel->restore();
        }

        d->activeLoadReader = nullptr;

        if (d->queuedLoad) {
            TimelineTraceManagerPrivate::QueuedLoad queued = *d->queuedLoad;
            d->queuedLoad.reset();
            if (queued.fi.isCanceled())
                queued.fi.reportFinished();
            else
                startLoad(queued.filename, queued.fi);
        }
    });
    watcher->setFuture(fi.future());
}

qint64 TimelineTraceManager::traceStart() const
{
    return d->traceStart;
}

qint64 TimelineTraceManager::traceEnd() const
{
    return d->traceEnd;
}

qint64 TimelineTraceManager::traceDuration() const
{
    return traceEnd() - traceStart();
}

void TimelineTraceManager::decreaseTraceStart(qint64 start)
{
    QTC_ASSERT(start >= 0, return);
    if (d->traceStart > start || d->traceStart == -1) {
        d->traceStart = start;
        if (d->traceEnd == -1)
            d->traceEnd = d->traceStart;
        else
            QTC_ASSERT(d->traceEnd >= d->traceStart, d->traceEnd = d->traceStart);
    }
}

void TimelineTraceManager::increaseTraceEnd(qint64 end)
{
    QTC_ASSERT(end >= 0, return);
    if (d->traceEnd < end || d->traceEnd == -1) {
        d->traceEnd = end;
        if (d->traceStart == -1)
            d->traceStart = d->traceEnd;
        else
            QTC_ASSERT(d->traceEnd >= d->traceStart, d->traceStart = d->traceEnd);
    }
}

void TimelineTraceManager::TimelineTraceManagerPrivate::updateTraceTime(qint64 time)
{
    QTC_ASSERT(time >= 0, return);
    if (traceStart > time || traceStart == -1)
        traceStart = time;
    if (traceEnd < time || traceEnd == -1)
        traceEnd = time;
    QTC_ASSERT(traceEnd >= traceStart, traceStart = traceEnd);
}

void TimelineTraceManager::setNotesModel(TimelineNotesModel *notesModel)
{
    d->notesModel = notesModel;
}

void TimelineTraceManager::clearEventStorage()
{
    d->cancelRestriction();
    d->reset();
    if (d->notesModel)
        d->notesModel->clear();
    setRecordedFeatures(0);
    d->eventStorage->clear();
}

void TimelineTraceManager::clearTypeStorage()
{
    d->typeStorage->clear();
    d->recordedFeatures = 0;
}

void TimelineTraceManager::clear()
{
    clearEventStorage();
}

void TimelineTraceManager::clearAll()
{
    clearEventStorage();
    clearTypeStorage();
}

void TimelineTraceManager::restrictByFilter(TraceEventFilter filter)
{
    if (d->notesModel)
        d->notesModel->stash();

    d->reset();

    QFutureInterface<void> future;
    replayEvents(filter(std::bind(&TimelineTraceManagerPrivate::dispatch, d,
                                  std::placeholders::_1, std::placeholders::_2)), [this] {
        initialize();
    }, [this] {
        if (d->notesModel)
            d->notesModel->restore();
        finalize();
    }, [this](const QString &message) {
        if (!message.isEmpty()) {
            emit error(Tr::tr("Could not re-read events from temporary trace file: %1\n"
                              "The trace data is lost.").arg(message));
        }
        clearAll();
    }, future);
}

// Like restrictByFilter(), but replays the events in another thread, so that the GUI stays
// responsive. The models are cleared and initialized right away, and finalized once all events
// are replayed. Until then, between restrictionStarted() and restrictionFinished(), nothing may
// use them.
void TimelineTraceManager::restrictByFilterInThread(TraceEventFilter filter)
{
    d->cancelReplays(nullptr, false);
    if (d->notesModel)
        d->notesModel->stash();
    d->reset();
    replayInThread(this, [this, filter] {
        return filter(std::bind(&TimelineTraceManagerPrivate::dispatch, d,
                                std::placeholders::_1, std::placeholders::_2));
    }, [this] { initialize(); }, [this] { prepare(); }, [this] {
        if (d->notesModel)
            d->notesModel->restore();
        finalize();
    }, [this](const QString &message) {
        if (!message.isEmpty()) {
            emit error(Tr::tr("Could not re-read events from temporary trace file: %1\n"
                              "The trace data is lost.").arg(message));
        }
        clearAll();
    });
}

/*!
    Replays the events to the loader \a makeLoader makes in another thread. \a makeLoader and
    \a initializer run right away,
    \a preparer in the thread once all events are replayed, and then \a finalizer, or
    \a errorHandler if the events could not be read, in this one. The trace is busy from
    restrictionStarted() until restrictionFinished() is emitted.

    A replay started earlier with the same \a key is canceled, and so are all of them when the
    manager restricts, loads or clears its events. One with a different key that restricts all
    models is waited for, as it fills the same models.
*/
void TimelineTraceManager::replayInThread(const void *key,
                                          const std::function<TraceEventLoader()> &makeLoader,
                                          const Initializer &initializer,
                                          const Preparer &preparer, const Finalizer &finalizer,
                                          const ErrorHandler &errorHandler)
{
    d->cancelReplays(key, false);
    if (key != this)
        d->waitForReplays(this);
    const TraceEventLoader loader = makeLoader();
    if (initializer)
        initializer();

    if (!std::exchange(d->replaying, true))
        emit restrictionStarted();

    QFutureInterface<void> fi;
    fi.reportStarted();
    const auto superseded = std::make_shared<bool>(false);
    const auto replayError = std::make_shared<std::optional<QString>>();
    const auto complete = [this, superseded, replayError, finalizer, errorHandler](
                              bool mayFinish) {
        if (std::exchange(*superseded, true))
            return;
        d->replays.removeIf([&](const TimelineTraceManagerPrivate::Replay &replay) {
            return replay.superseded == superseded;
        });
        if (*replayError) {
            if (errorHandler)
                errorHandler(**replayError);
        } else if (finalizer) {
            finalizer();
        }
        if (mayFinish)
            d->finishReplaying();
    };
    d->replays.append({key, fi.future(), superseded, complete});

    auto watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [watcher, complete] {
        watcher->deleteLater();
        complete(true);
    });
    watcher->setFuture(fi.future());

    Utils::asyncRun([this, loader, preparer, replayError, fi]() mutable {
        replayEvents(loader, {}, {}, [replayError](const QString &message) {
            *replayError = message;
        }, fi);
        if (!*replayError && !fi.isCanceled() && preparer)
            preparer();
        fi.reportFinished();
    });
}

// Whatever cancels a replay replaces it, so the replay does not report back.
void TimelineTraceManager::TimelineTraceManagerPrivate::cancelReplays(const void *key,
                                                                      bool mayFinish)
{
    for (qsizetype i = replays.size() - 1; i >= 0; --i) {
        Replay &replay = replays[i];
        if (key && replay.key != key)
            continue;
        *replay.superseded = true;
        replay.future.cancel();
        replay.future.waitForFinished();
        replays.removeAt(i);
    }
    if (mayFinish)
        finishReplaying();
}

void TimelineTraceManager::TimelineTraceManagerPrivate::finishReplaying()
{
    if (replays.isEmpty() && std::exchange(replaying, false))
        emit q->restrictionFinished();
}

// Waits for the replays of key and completes them right away.
void TimelineTraceManager::TimelineTraceManagerPrivate::waitForReplays(const void *key)
{
    const QList<Replay> waited = Utils::filtered(replays, [key](const Replay &replay) {
        return replay.key == key;
    });
    for (Replay replay : waited) {
        replay.future.waitForFinished();
        replay.complete(false);
    }
}

void TimelineTraceManager::TimelineTraceManagerPrivate::dispatch(const TraceEvent &event,
                                                                 const TraceEventType &type)
{
    q->loadEvent(event, type);
    if (event.timestamp() >= 0)
        updateTraceTime(event.timestamp());
    ++numEvents;
}

void TimelineTraceManager::TimelineTraceManagerPrivate::cancelQueuedLoad()
{
    if (!queuedLoad)
        return;

    QueuedLoad queued = *queuedLoad;
    queuedLoad.reset();
    queued.fi.reportCanceled();
    queued.fi.reportFinished();
}

void TimelineTraceManager::TimelineTraceManagerPrivate::reset()
{
    prepared = false;
    traceStart = -1;
    traceEnd = -1;

    for (const Clearer &clearer : std::as_const(clearers))
        clearer();

    numEvents = 0;
}

} // namespace Timeline
