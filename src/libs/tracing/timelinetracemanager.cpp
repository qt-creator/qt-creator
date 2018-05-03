/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "timelinetracemanager.h"
#include "timelinetracefile.h"

#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>
#include <utils/runextensions.h>

#include <QFile>
#include <QDataStream>

namespace Timeline {

class TimelineTraceManager::TimelineTraceManagerPrivate
{
public:
    TimelineNotesModel *notesModel = nullptr;

    int numEvents = 0;
    int numEventTypes = 0;
    quint64 availableFeatures = 0;
    quint64 visibleFeatures = 0;
    quint64 recordedFeatures = 0;
    bool aggregateTraces = false;

    QHash<quint8, QVector<TraceEventLoader>> eventLoaders;
    QVector<Initializer> initializers;
    QVector<Finalizer> finalizers;
    QVector<Clearer> clearers;

    qint64 traceStart = -1;
    qint64 traceEnd = -1;
    qint64 restrictedTraceStart = -1;
    qint64 restrictedTraceEnd = -1;

    void dispatch(const TraceEvent &event, const TraceEventType &type);
    void reset();
    void updateTraceTime(qint64 time);
    void restrictTraceTimeToRange(qint64 start, qint64 end);
};

TimelineTraceManager::TimelineTraceManager(QObject *parent) :
    QObject(parent), d(new TimelineTraceManagerPrivate)
{
}

TimelineTraceManager::~TimelineTraceManager()
{
    delete d;
}

TimelineNotesModel *TimelineTraceManager::notesModel() const
{
    return d->notesModel;
}

bool TimelineTraceManager::isEmpty() const
{
    return d->numEvents == 0;
}

int TimelineTraceManager::numEvents() const
{
    return d->numEvents;
}

int TimelineTraceManager::numEventTypes() const
{
    return d->numEventTypes;
}

void TimelineTraceManager::addEvent(const TraceEvent &event)
{
    d->dispatch(event, lookupType(event.typeIndex()));
}

void TimelineTraceManager::addEventType(const TraceEventType &type)
{
    const quint8 feature = type.feature();
    d->recordedFeatures |= (1ull << feature);
    ++(d->numEventTypes);
}

void TimelineTraceManager::registerFeatures(quint64 features, TraceEventLoader eventLoader,
                                            Initializer initializer, Finalizer finalizer,
                                            Clearer clearer)
{
    if ((features & d->availableFeatures) != features) {
        d->availableFeatures |= features;
        emit availableFeaturesChanged(d->availableFeatures);
    }

    if ((features & d->visibleFeatures) != features) {
        d->visibleFeatures |= features;
        emit visibleFeaturesChanged(d->visibleFeatures);
    }

    for (quint8 feature = 0; feature != sizeof(features) * 8; ++feature) {
        if (features & (1ULL << feature)) {
            if (eventLoader)
                d->eventLoaders[feature].append(eventLoader);
        }
    }

    if (initializer)
        d->initializers.append(initializer);

    if (finalizer)
        d->finalizers.append(finalizer);

    if (clearer)
        d->clearers.append(clearer);
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
    for (const Initializer &initializer : qAsConst(d->initializers))
        initializer();
}

void TimelineTraceManager::finalize()
{
    // Load notes after the timeline models have been initialized ...
    // which happens on stateChanged(Done).

    for (const Finalizer &finalizer : qAsConst(d->finalizers))
        finalizer();
}

QFuture<void> TimelineTraceManager::save(const QString &filename)
{
    QFile *file = new QFile(filename);
    if (!file->open(QIODevice::WriteOnly)) {
        delete file;
        return Utils::runAsync([this, filename](QFutureInterface<void> &future) {
            future.setProgressRange(0, 1);
            future.setProgressValue(1);
            emit error(tr("Could not open %1 for writing.").arg(filename));
            emit saveFinished();
        });
    }

    TimelineTraceFile *writer = createTraceFile();
    writer->setTraceTime(traceStart(), traceEnd(), traceDuration());
    writer->setTraceManager(this);
    writer->setNotes(d->notesModel);

    connect(writer, &QObject::destroyed, this, &TimelineTraceManager::saveFinished,
            Qt::QueuedConnection);

    connect(writer, &TimelineTraceFile::error, this, [this, file](const QString &message) {
        file->close();
        file->remove();
        delete file;
        emit error(message);
    }, Qt::QueuedConnection);

    connect(writer, &TimelineTraceFile::success, this, [file]() {
        file->close();
        delete file;
    }, Qt::QueuedConnection);

    connect(writer, &TimelineTraceFile::canceled, this, [file]() {
        file->close();
        file->remove();
        delete file;
    }, Qt::QueuedConnection);

    return Utils::runAsync([file, writer] (QFutureInterface<void> &future) {
        writer->setFuture(future);
        writer->save(file);
        writer->deleteLater();
    });
}

QFuture<void> TimelineTraceManager::load(const QString &filename)
{
    QFile *file = new QFile(filename, this);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        return Utils::runAsync([this, filename] (QFutureInterface<void> &future) {
            future.setProgressRange(0, 1);
            future.setProgressValue(1);
            emit error(tr("Could not open %1 for reading.").arg(filename));
            emit loadFinished();
        });
    }

    clearAll();
    initialize();
    TimelineTraceFile *reader = createTraceFile();
    reader->setTraceManager(this);
    reader->setNotes(d->notesModel);

    connect(reader, &QObject::destroyed, this, &TimelineTraceManager::loadFinished,
            Qt::QueuedConnection);

    connect(reader, &TimelineTraceFile::success, this, [this, reader]() {
        if (reader->traceStart() >= 0)
            decreaseTraceStart(reader->traceStart());
        if (reader->traceEnd() >= 0)
            increaseTraceEnd(reader->traceEnd());
        finalize();
        delete reader;
    }, Qt::QueuedConnection);

    connect(reader, &TimelineTraceFile::error, this, [this, reader](const QString &message) {
        clearAll();
        delete reader;
        emit error(message);
    }, Qt::QueuedConnection);

    connect(reader, &TimelineTraceFile::canceled, this, [this, reader]() {
        clearAll();
        delete reader;
    }, Qt::QueuedConnection);

    return Utils::runAsync([file, reader] (QFutureInterface<void> &future) {
        reader->setFuture(future);
        reader->load(file);
        file->close();
        file->deleteLater();
    });
}

qint64 TimelineTraceManager::traceStart() const
{
    return d->restrictedTraceStart != -1 ? d->restrictedTraceStart : d->traceStart;
}

qint64 TimelineTraceManager::traceEnd() const
{
    return d->restrictedTraceEnd != -1 ? d->restrictedTraceEnd : d->traceEnd;
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

void TimelineTraceManager::TimelineTraceManagerPrivate::restrictTraceTimeToRange(qint64 start,
                                                                                 qint64 end)
{
    QTC_ASSERT(end == -1 || start <= end, end = start);
    restrictedTraceStart = start;
    restrictedTraceEnd = end;
}

void TimelineTraceManager::setNotesModel(TimelineNotesModel *notesModel)
{
    d->notesModel = notesModel;
}

void TimelineTraceManager::clearEventStorage()
{
    d->reset();
    if (d->notesModel)
        d->notesModel->clear();
    setVisibleFeatures(0);
    setRecordedFeatures(0);
}

void TimelineTraceManager::clearTypeStorage()
{
    d->numEventTypes = 0;
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

void TimelineTraceManager::restrictToRange(qint64 startTime, qint64 endTime)
{
    if (d->notesModel)
        d->notesModel->stash();

    d->reset();
    setVisibleFeatures(0);

    QFutureInterface<void> future;
    replayEvents(startTime, endTime,
                 std::bind(&TimelineTraceManagerPrivate::dispatch, d, std::placeholders::_1,
                           std::placeholders::_2),
                 [this, startTime, endTime]() {
        d->restrictTraceTimeToRange(startTime, endTime);
        initialize();
    }, [this]() {
        if (d->notesModel)
            d->notesModel->restore();
        finalize();
    }, [this](const QString &message) {
        emit error(tr("Could not re-read events from temporary trace file: %1\n"
                      "The trace data is lost.").arg(message));
        clearAll();
    }, future);
}

bool TimelineTraceManager::isRestrictedToRange() const
{
    return d->restrictedTraceStart != -1 || d->restrictedTraceEnd != -1;
}

void TimelineTraceManager::TimelineTraceManagerPrivate::dispatch(const TraceEvent &event,
                                                                 const TraceEventType &type)
{
    for (const TraceEventLoader &loader : eventLoaders[type.feature()])
        loader(event, type);

    if (event.timestamp() >= 0)
        updateTraceTime(event.timestamp());
    ++numEvents;
}

void TimelineTraceManager::TimelineTraceManagerPrivate::reset()
{
    restrictTraceTimeToRange(-1, -1);
    traceStart = -1;
    traceEnd = -1;

    for (const Clearer &clearer : qAsConst(clearers))
        clearer();

    numEvents = 0;
}

} // namespace Timeline
