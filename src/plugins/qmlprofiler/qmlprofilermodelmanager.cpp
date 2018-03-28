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

#include "qmlprofilermodelmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertracefile.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofilerdetailsrewriter.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QRegExp>
#include <QStack>

#include <functional>

namespace QmlProfiler {
namespace Internal {

static const char *ProfileFeatureNames[] = {
    QT_TRANSLATE_NOOP("MainView", "JavaScript"),
    QT_TRANSLATE_NOOP("MainView", "Memory Usage"),
    QT_TRANSLATE_NOOP("MainView", "Pixmap Cache"),
    QT_TRANSLATE_NOOP("MainView", "Scene Graph"),
    QT_TRANSLATE_NOOP("MainView", "Animations"),
    QT_TRANSLATE_NOOP("MainView", "Painting"),
    QT_TRANSLATE_NOOP("MainView", "Compiling"),
    QT_TRANSLATE_NOOP("MainView", "Creating"),
    QT_TRANSLATE_NOOP("MainView", "Binding"),
    QT_TRANSLATE_NOOP("MainView", "Handling Signal"),
    QT_TRANSLATE_NOOP("MainView", "Input Events"),
    QT_TRANSLATE_NOOP("MainView", "Debug Messages")
};

Q_STATIC_ASSERT(sizeof(ProfileFeatureNames) == sizeof(char *) * MaximumProfileFeature);

} // namespace Internal

using namespace Internal;

class QmlProfilerModelManager::QmlProfilerModelManagerPrivate
{
public:
    QmlProfilerModelManagerPrivate() : file("qmlprofiler-data") {}

    QmlProfilerNotesModel *notesModel = nullptr;
    QmlProfilerTextMarkModel *textMarkModel = nullptr;

    QmlProfilerModelManager::State state = Empty;

    int numRegisteredModels = 0;
    int numFinishedFinalizers = 0;

    uint numLoadedEvents = 0;
    quint64 availableFeatures = 0;
    quint64 visibleFeatures = 0;
    quint64 recordedFeatures = 0;
    bool aggregateTraces = false;

    QHash<ProfileFeature, QVector<EventLoader> > eventLoaders;
    QVector<Finalizer> finalizers;

    QVector<QmlEventType> eventTypes;
    QmlProfilerDetailsRewriter *detailsRewriter = nullptr;

    Utils::TemporaryFile file;
    QDataStream eventStream;

    qint64 traceStart = -1;
    qint64 traceEnd = -1;
    qint64 restrictedTraceStart = -1;
    qint64 restrictedTraceEnd = -1;

    void dispatch(const QmlEvent &event, const QmlEventType &type);
    void rewriteType(int typeIndex);
    int resolveStackTop();
    void updateTraceTime(qint64 time);
    void restrictTraceTimeToRange(qint64 start, qint64 end);
};

QmlProfilerModelManager::QmlProfilerModelManager(QObject *parent) :
    QObject(parent), d(new QmlProfilerModelManagerPrivate)
{
    d->notesModel = new QmlProfilerNotesModel(this);
    d->textMarkModel = new QmlProfilerTextMarkModel(this);

    d->detailsRewriter = new QmlProfilerDetailsRewriter(this);
    connect(d->detailsRewriter, &QmlProfilerDetailsRewriter::rewriteDetailsString,
            this, &QmlProfilerModelManager::detailsChanged);
    connect(d->detailsRewriter, &QmlProfilerDetailsRewriter::eventDetailsChanged,
            this, &QmlProfilerModelManager::typeDetailsFinished);

    if (d->file.open())
        d->eventStream.setDevice(&d->file);
    else
        emit error(tr("Cannot open temporary trace file to store events."));
}

QmlProfilerModelManager::~QmlProfilerModelManager()
{
    delete d;
}

qint64 QmlProfilerModelManager::traceStart() const
{
    return d->restrictedTraceStart != -1 ? d->restrictedTraceStart : d->traceStart;
}

qint64 QmlProfilerModelManager::traceEnd() const
{
    return d->restrictedTraceEnd != -1 ? d->restrictedTraceEnd : d->traceEnd;
}

qint64 QmlProfilerModelManager::traceDuration() const
{
    return traceEnd() - traceStart();
}

void QmlProfilerModelManager::decreaseTraceStart(qint64 start)
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

void QmlProfilerModelManager::increaseTraceEnd(qint64 end)
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

QmlProfilerNotesModel *QmlProfilerModelManager::notesModel() const
{
    return d->notesModel;
}

QmlProfilerTextMarkModel *QmlProfilerModelManager::textMarkModel() const
{
    return d->textMarkModel;
}

bool QmlProfilerModelManager::isEmpty() const
{
    return d->file.pos() == 0;
}

uint QmlProfilerModelManager::numLoadedEvents() const
{
    return d->numLoadedEvents;
}

uint QmlProfilerModelManager::numLoadedEventTypes() const
{
    return d->eventTypes.count();
}

int QmlProfilerModelManager::registerModelProxy()
{
    return d->numRegisteredModels++;
}

int QmlProfilerModelManager::numFinishedFinalizers() const
{
    return d->numFinishedFinalizers;
}

int QmlProfilerModelManager::numRegisteredFinalizers() const
{
    return d->finalizers.count();
}

void QmlProfilerModelManager::addEvents(const QVector<QmlEvent> &events)
{
    for (const QmlEvent &event : events) {
        d->eventStream << event;
        d->updateTraceTime(event.timestamp());
        d->dispatch(event, d->eventTypes[event.typeIndex()]);
    }
}

void QmlProfilerModelManager::addEvent(const QmlEvent &event)
{
    d->eventStream << event;
    d->updateTraceTime(event.timestamp());
    QTC_ASSERT(event.typeIndex() < d->eventTypes.size(),
               d->eventTypes.resize(event.typeIndex() + 1));
    d->dispatch(event, d->eventTypes.at(event.typeIndex()));
}

void QmlProfilerModelManager::addEventTypes(const QVector<QmlEventType> &types)
{
    const int firstTypeId = d->eventTypes.length();;
    d->eventTypes.append(types);
    for (int typeId = firstTypeId, end = d->eventTypes.length(); typeId < end; ++typeId) {
        d->rewriteType(typeId);
        const QmlEventLocation &location = d->eventTypes[typeId].location();
        if (location.isValid()) {
            d->textMarkModel->addTextMarkId(typeId, QmlEventLocation(
                            findLocalFile(location.filename()), location.line(),
                            location.column()));
        }
    }
}

void QmlProfilerModelManager::addEventType(const QmlEventType &type)
{
    const int typeId = d->eventTypes.count();
    d->eventTypes.append(type);
    d->rewriteType(typeId);
    const QmlEventLocation &location = type.location();
    if (location.isValid()) {
        d->textMarkModel->addTextMarkId(
                    typeId, QmlEventLocation(findLocalFile(location.filename()),
                                             location.line(), location.column()));
    }
}

const QVector<QmlEventType> &QmlProfilerModelManager::eventTypes() const
{
    return d->eventTypes;
}

static bool isStateful(const QmlEventType &type)
{
    // Events of these types carry state that has to be taken into account when adding later events:
    // PixmapCacheEvent: Total size of the cache and size of pixmap currently being loaded
    // MemoryAllocation: Total size of the JS heap and the amount of it currently in use
    const Message message = type.message();
    return message == PixmapCacheEvent || message == MemoryAllocation;
}

bool QmlProfilerModelManager::replayEvents(qint64 rangeStart, qint64 rangeEnd,
                                           EventLoader loader) const
{
    QStack<QmlEvent> stack;
    QmlEvent event;
    QFile file(d->file.fileName());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream stream(&file);
    bool crossedRangeStart = false;
    while (!stream.atEnd()) {
        stream >> event;
        if (stream.status() == QDataStream::ReadPastEnd)
            break;

        const QmlEventType &type = d->eventTypes[event.typeIndex()];
        if (rangeStart != -1 && rangeEnd != -1) {
            // Double-check if rangeStart has been crossed. Some versions of Qt send dirty data.
            if (event.timestamp() < rangeStart && !crossedRangeStart) {
                if (type.rangeType() != MaximumRangeType) {
                    if (event.rangeStage() == RangeStart)
                        stack.push(event);
                    else if (event.rangeStage() == RangeEnd)
                        stack.pop();
                    continue;
                } else if (isStateful(type)) {
                    event.setTimestamp(rangeStart);
                } else {
                    continue;
                }
            } else {
                if (!crossedRangeStart) {
                    foreach (QmlEvent stashed, stack) {
                        stashed.setTimestamp(rangeStart);
                        loader(stashed, d->eventTypes[stashed.typeIndex()]);
                    }
                    stack.clear();
                    crossedRangeStart = true;
                }
                if (event.timestamp() > rangeEnd) {
                    if (type.rangeType() != MaximumRangeType) {
                        if (event.rangeStage() == RangeEnd) {
                            if (stack.isEmpty()) {
                                QmlEvent endEvent(event);
                                endEvent.setTimestamp(rangeEnd);
                                loader(endEvent, d->eventTypes[event.typeIndex()]);
                            } else {
                                stack.pop();
                            }
                        } else if (event.rangeStage() == RangeStart) {
                            stack.push(event);
                        }
                        continue;
                    } else if (isStateful(type)) {
                        event.setTimestamp(rangeEnd);
                    } else {
                        continue;
                    }
                }
            }
        }

        loader(event, type);
    }
    return true;
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::dispatch(const QmlEvent &event,
                                                                       const QmlEventType &type)
{
    for (const EventLoader &loader : eventLoaders.value(
             static_cast<ProfileFeature>(type.feature()))) {
        loader(event, type);
    }
    ++numLoadedEvents;
}

static QString getDisplayName(const QmlEventType &event)
{
    if (event.location().filename().isEmpty()) {
        return QmlProfilerModelManager::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(event.location().filename()).path();
        return filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(event.location().line());
    }
}

static QString getInitialDetails(const QmlEventType &event)
{
    QString details = event.data();
    // generate details string
    if (!details.isEmpty()) {
        details = details.replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        if (details.isEmpty()) {
            if (event.rangeType() == Javascript)
                details = QmlProfilerModelManager::tr("anonymous function");
        } else {
            QRegExp rewrite(QLatin1String("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)"));
            bool match = rewrite.exactMatch(details);
            if (match)
                details = rewrite.cap(1) + QLatin1String(": ") + rewrite.cap(3);
            if (details.startsWith(QLatin1String("file://")) ||
                    details.startsWith(QLatin1String("qrc:/")))
                details = details.mid(details.lastIndexOf(QLatin1Char('/')) + 1);
        }
    }

    return details;
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::rewriteType(int typeIndex)
{
    QmlEventType &type = eventTypes[typeIndex];
    type.setDisplayName(getDisplayName(type));
    type.setData(getInitialDetails(type));

    const QmlEventLocation &location = type.location();
    // There is no point in looking for invalid locations
    if (!location.isValid())
        return;

    // Only bindings and signal handlers need rewriting
    if (type.rangeType() == Binding || type.rangeType() == HandlingSignal)
        detailsRewriter->requestDetailsForLocation(typeIndex, location);
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::updateTraceTime(qint64 time)
{
    QTC_ASSERT(time >= 0, return);
    if (traceStart > time || traceStart == -1)
        traceStart = time;
    if (traceEnd < time || traceEnd == -1)
        traceEnd = time;
    QTC_ASSERT(traceEnd >= traceStart, traceStart = traceEnd);
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::restrictTraceTimeToRange(qint64 start, qint64 end)
{
    QTC_ASSERT(end == -1 || start <= end, end = start);
    restrictedTraceStart = start;
    restrictedTraceEnd = end;
}

void QmlProfilerModelManager::announceFeatures(quint64 features, EventLoader eventLoader,
                                               Finalizer finalizer)
{
    if ((features & d->availableFeatures) != features) {
        d->availableFeatures |= features;
        emit availableFeaturesChanged(d->availableFeatures);
    }
    if ((features & d->visibleFeatures) != features) {
        d->visibleFeatures |= features;
        emit visibleFeaturesChanged(d->visibleFeatures);
    }

    for (int feature = 0; feature != MaximumProfileFeature; ++feature) {
        if (features & (1ULL << feature))
            d->eventLoaders[static_cast<ProfileFeature>(feature)].append(eventLoader);
    }

    d->finalizers.append(finalizer);
}

quint64 QmlProfilerModelManager::availableFeatures() const
{
    return d->availableFeatures;
}

quint64 QmlProfilerModelManager::visibleFeatures() const
{
    return d->visibleFeatures;
}

void QmlProfilerModelManager::setVisibleFeatures(quint64 features)
{
    if (d->visibleFeatures != features) {
        d->visibleFeatures = features;
        emit visibleFeaturesChanged(d->visibleFeatures);
    }
}

quint64 QmlProfilerModelManager::recordedFeatures() const
{
    return d->recordedFeatures;
}

void QmlProfilerModelManager::setRecordedFeatures(quint64 features)
{
    if (d->recordedFeatures != features) {
        d->recordedFeatures = features;
        emit recordedFeaturesChanged(d->recordedFeatures);
    }
}

bool QmlProfilerModelManager::aggregateTraces() const
{
    return d->aggregateTraces;
}

void QmlProfilerModelManager::setAggregateTraces(bool aggregateTraces)
{
    d->aggregateTraces = aggregateTraces;
}


const char *QmlProfilerModelManager::featureName(ProfileFeature feature)
{
    return ProfileFeatureNames[feature];
}

void QmlProfilerModelManager::finalize()
{
    QTC_ASSERT(state() == AcquiringData, /**/);
    d->file.flush();
    d->detailsRewriter->reloadDocuments();

    // Load notes after the timeline models have been initialized ...
    // which happens on stateChanged(Done).

    foreach (const Finalizer &finalizer, d->finalizers) {
        finalizer();
        ++d->numFinishedFinalizers;
    }

    setState(Done);
}

void QmlProfilerModelManager::populateFileFinder(const ProjectExplorer::Target *target)
{
    d->detailsRewriter->populateFileFinder(target);
}

QString QmlProfilerModelManager::findLocalFile(const QString &remoteFile)
{
    return d->detailsRewriter->getLocalFile(remoteFile);
}

void QmlProfilerModelManager::save(const QString &filename)
{
    QFile *file = new QFile(filename);
    if (!file->open(QIODevice::WriteOnly)) {
        emit error(tr("Could not open %1 for writing.").arg(filename));
        delete file;
        emit saveFinished();
        return;
    }

    d->notesModel->stash();

    QmlProfilerFileWriter *writer = new QmlProfilerFileWriter(this);
    writer->setTraceTime(traceStart(), traceEnd(), traceDuration());
    writer->setData(this);
    writer->setNotes(d->notesModel->notes());

    connect(writer, &QObject::destroyed, this, &QmlProfilerModelManager::saveFinished,
            Qt::QueuedConnection);

    connect(writer, &QmlProfilerFileWriter::error, this, [this, file](const QString &message) {
        file->close();
        file->remove();
        delete file;
        emit error(message);
    }, Qt::QueuedConnection);

    connect(writer, &QmlProfilerFileWriter::success, this, [file]() {
        file->close();
        delete file;
    }, Qt::QueuedConnection);

    connect(writer, &QmlProfilerFileWriter::canceled, this, [file]() {
        file->close();
        file->remove();
        delete file;
    }, Qt::QueuedConnection);

    QFuture<void> result = Utils::runAsync([file, writer] (QFutureInterface<void> &future) {
        writer->setFuture(&future);
        if (file->fileName().endsWith(QLatin1String(Constants::QtdFileExtension)))
            writer->saveQtd(file);
        else
            writer->saveQzt(file);
        writer->deleteLater();
    });

    Core::ProgressManager::addTask(result, tr("Saving Trace Data"), Constants::TASK_SAVE,
                                   Core::ProgressManager::ShowInApplicationIcon);
}

void QmlProfilerModelManager::load(const QString &filename)
{
    bool isQtd = filename.endsWith(QLatin1String(Constants::QtdFileExtension));
    QFile *file = new QFile(filename, this);
    if (!file->open(isQtd ? (QIODevice::ReadOnly | QIODevice::Text) : QIODevice::ReadOnly)) {
        emit error(tr("Could not open %1 for reading.").arg(filename));
        delete file;
        emit loadFinished();
        return;
    }

    clear();
    setState(AcquiringData);
    QmlProfilerFileReader *reader = new QmlProfilerFileReader(this);

    connect(reader, &QObject::destroyed, this, &QmlProfilerModelManager::loadFinished,
            Qt::QueuedConnection);

    connect(reader, &QmlProfilerFileReader::typesLoaded,
            this, &QmlProfilerModelManager::addEventTypes);

    connect(reader, &QmlProfilerFileReader::notesLoaded,
            d->notesModel, &QmlProfilerNotesModel::setNotes);

    connect(reader, &QmlProfilerFileReader::qmlEventsLoaded,
            this, &QmlProfilerModelManager::addEvents);

    connect(reader, &QmlProfilerFileReader::success, this, [this, reader]() {
        if (reader->traceStart() >= 0)
            decreaseTraceStart(reader->traceStart());
        if (reader->traceEnd() >= 0)
            increaseTraceEnd(reader->traceEnd());
        setRecordedFeatures(reader->loadedFeatures());
        delete reader;
        finalize();
    }, Qt::QueuedConnection);

    connect(reader, &QmlProfilerFileReader::error, this, [this, reader](const QString &message) {
        clear();
        delete reader;
        emit error(message);
    }, Qt::QueuedConnection);

    connect(reader, &QmlProfilerFileReader::canceled, this, [this, reader]() {
        clear();
        delete reader;
    }, Qt::QueuedConnection);

    QFuture<void> result = Utils::runAsync([isQtd, file, reader] (QFutureInterface<void> &future) {
        reader->setFuture(&future);
        if (isQtd)
            reader->loadQtd(file);
        else
            reader->loadQzt(file);
        file->close();
        file->deleteLater();
    });

    Core::ProgressManager::addTask(result, tr("Loading Trace Data"), Constants::TASK_LOAD);
}

void QmlProfilerModelManager::setState(QmlProfilerModelManager::State state)
{
    // It's not an error, we are continuously calling "AcquiringData" for example
    if (d->state == state)
        return;

    switch (state) {
        case ClearingData:
            QTC_ASSERT(d->state == Done || d->state == Empty || d->state == AcquiringData, /**/);
        break;
        case Empty:
            // if it's not empty, complain but go on
            QTC_ASSERT(isEmpty(), /**/);
        break;
        case AcquiringData:
        break;
        case Done:
            QTC_ASSERT(d->state == AcquiringData || d->state == Empty, return);
        break;
        default:
            emit error(tr("Trying to set unknown state in events list."));
        break;
    }

    d->state = state;
    emit stateChanged();
}

void QmlProfilerModelManager::detailsChanged(int typeId, const QString &newString)
{
    QTC_ASSERT(typeId < d->eventTypes.count(), return);
    d->eventTypes[typeId].setData(newString);
    emit typeDetailsChanged(typeId);
}

QmlProfilerModelManager::State QmlProfilerModelManager::state() const
{
    return d->state;
}

void QmlProfilerModelManager::doClearEvents()
{
    d->numLoadedEvents = 0;
    d->numFinishedFinalizers = 0;
    d->file.remove();
    d->eventStream.unsetDevice();
    if (d->file.open())
        d->eventStream.setDevice(&d->file);
    else
        emit error(tr("Cannot open temporary trace file to store events."));
    d->restrictTraceTimeToRange(-1, -1);
    d->traceStart = -1;
    d->traceEnd = -1;
    d->notesModel->clear();
    setVisibleFeatures(0);
    setRecordedFeatures(0);
}

void QmlProfilerModelManager::clearEvents()
{
    setState(ClearingData);
    doClearEvents();
    setState(Empty);
}

void QmlProfilerModelManager::clear()
{
    setState(ClearingData);
    doClearEvents();
    d->eventTypes.clear();
    d->detailsRewriter->clear();
    setState(Empty);
}

void QmlProfilerModelManager::restrictToRange(qint64 startTime, qint64 endTime)
{
    d->notesModel->stash();
    const QVector<QmlNote> notes = d->notesModel->notes();
    d->notesModel->clear();

    setState(ClearingData);
    setVisibleFeatures(0);

    startAcquiring();
    if (!replayEvents(startTime, endTime,
                      std::bind(&QmlProfilerModelManagerPrivate::dispatch, d, std::placeholders::_1,
                                std::placeholders::_2))) {
        emit error(tr("Could not re-read events from temporary trace file. "
                      "The trace data is lost."));
        clear();
    } else {
        d->notesModel->setNotes(notes);
        d->restrictTraceTimeToRange(startTime, endTime);
        finalize();
    }
}

bool QmlProfilerModelManager::isRestrictedToRange() const
{
    return d->restrictedTraceStart != -1 || d->restrictedTraceEnd != -1;
}

void QmlProfilerModelManager::startAcquiring()
{
    setState(AcquiringData);
}

} // namespace QmlProfiler
