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
#include "qmlprofilerdatamodel.h"
#include "qmlprofilertracefile.h"
#include "qmlprofilernotesmodel.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QMessageBox>

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

/////////////////////////////////////////////////////////////////////
QmlProfilerTraceTime::QmlProfilerTraceTime(QObject *parent) :
    QObject(parent), m_startTime(-1), m_endTime(-1),
    m_restrictedStartTime(-1), m_restrictedEndTime(-1)
{
}

qint64 QmlProfilerTraceTime::startTime() const
{
    return m_restrictedStartTime != -1 ? m_restrictedStartTime : m_startTime;
}

qint64 QmlProfilerTraceTime::endTime() const
{
    return m_restrictedEndTime != -1 ? m_restrictedEndTime : m_endTime;
}

qint64 QmlProfilerTraceTime::duration() const
{
    return endTime() - startTime();
}

bool QmlProfilerTraceTime::isRestrictedToRange() const
{
    return m_restrictedStartTime != -1 || m_restrictedEndTime != -1;
}

void QmlProfilerTraceTime::clear()
{
    restrictToRange(-1, -1);
    setTime(-1, -1);
}

void QmlProfilerTraceTime::setTime(qint64 startTime, qint64 endTime)
{
    QTC_ASSERT(startTime <= endTime, endTime = startTime);
    m_startTime = startTime;
    m_endTime = endTime;
}

void QmlProfilerTraceTime::decreaseStartTime(qint64 time)
{
    if (m_startTime > time || m_startTime == -1) {
        m_startTime = time;
        if (m_endTime == -1)
            m_endTime = m_startTime;
        else
            QTC_ASSERT(m_endTime >= m_startTime, m_endTime = m_startTime);
    }
}

void QmlProfilerTraceTime::increaseEndTime(qint64 time)
{
    if (m_endTime < time || m_endTime == -1) {
        m_endTime = time;
        if (m_startTime == -1)
            m_startTime = m_endTime;
        else
            QTC_ASSERT(m_endTime >= m_startTime, m_startTime = m_endTime);
    }
}

void QmlProfilerTraceTime::restrictToRange(qint64 startTime, qint64 endTime)
{
    QTC_ASSERT(endTime == -1 || startTime <= endTime, endTime = startTime);
    m_restrictedStartTime = startTime;
    m_restrictedEndTime = endTime;
}


} // namespace Internal

/////////////////////////////////////////////////////////////////////

class QmlProfilerModelManager::QmlProfilerModelManagerPrivate
{
public:
    QmlProfilerDataModel *model;
    QmlProfilerNotesModel *notesModel;

    QmlProfilerModelManager::State state;
    QmlProfilerTraceTime *traceTime;

    int numRegisteredModels;
    int numFinishedFinalizers;

    uint numLoadedEvents;
    quint64 availableFeatures;
    quint64 visibleFeatures;
    quint64 recordedFeatures;
    bool aggregateTraces;

    QHash<ProfileFeature, QVector<EventLoader> > eventLoaders;
    QVector<Finalizer> finalizers;

    void dispatch(const QmlEvent &event, const QmlEventType &type);
};


QmlProfilerModelManager::QmlProfilerModelManager(QObject *parent) :
    QObject(parent), d(new QmlProfilerModelManagerPrivate)
{
    d->numRegisteredModels = 0;
    d->numFinishedFinalizers = 0;
    d->numLoadedEvents = 0;
    d->availableFeatures = 0;
    d->visibleFeatures = 0;
    d->recordedFeatures = 0;
    d->aggregateTraces = false;
    d->model = new QmlProfilerDataModel(this);
    d->state = Empty;
    d->traceTime = new QmlProfilerTraceTime(this);
    d->notesModel = new QmlProfilerNotesModel(this);
    connect(d->model, &QmlProfilerDataModel::allTypesLoaded,
            this, &QmlProfilerModelManager::processingDone);
}

QmlProfilerModelManager::~QmlProfilerModelManager()
{
    delete d;
}

QmlProfilerTraceTime *QmlProfilerModelManager::traceTime() const
{
    return d->traceTime;
}

QmlProfilerDataModel *QmlProfilerModelManager::qmlModel() const
{
    return d->model;
}

QmlProfilerNotesModel *QmlProfilerModelManager::notesModel() const
{
    return d->notesModel;
}

bool QmlProfilerModelManager::isEmpty() const
{
    return d->model->isEmpty();
}

uint QmlProfilerModelManager::numLoadedEvents() const
{
    return d->numLoadedEvents;
}

uint QmlProfilerModelManager::numLoadedEventTypes() const
{
    return d->model->eventTypes().count();
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
    d->model->addEvents(events);
    const QVector<QmlEventType> &types = d->model->eventTypes();
    for (const QmlEvent &event : events)
        d->dispatch(event, types[event.typeIndex()]);
}

void QmlProfilerModelManager::addEvent(const QmlEvent &event)
{
    d->model->addEvent(event);
    d->dispatch(event, d->model->eventType(event.typeIndex()));
}

void QmlProfilerModelManager::addEventTypes(const QVector<QmlEventType> &types)
{
    d->model->addEventTypes(types);
}

void QmlProfilerModelManager::addEventType(const QmlEventType &type)
{
    d->model->addEventType(type);
}

void QmlProfilerModelManager::QmlProfilerModelManagerPrivate::dispatch(const QmlEvent &event,
                                                                       const QmlEventType &type)
{
    foreach (const EventLoader &loader, eventLoaders[type.feature()])
        loader(event, type);
    ++numLoadedEvents;
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

void QmlProfilerModelManager::acquiringDone()
{
    QTC_ASSERT(state() == AcquiringData, /**/);
    setState(ProcessingData);
    d->model->finalize();
}

void QmlProfilerModelManager::processingDone()
{
    QTC_ASSERT(state() == ProcessingData, /**/);
    // Load notes after the timeline models have been initialized ...
    // which happens on stateChanged(Done).

    foreach (const Finalizer &finalizer, d->finalizers) {
        finalizer();
        ++d->numFinishedFinalizers;
    }

    d->notesModel->loadData();
    setState(Done);

    emit loadFinished();
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

    d->notesModel->saveData();

    QmlProfilerFileWriter *writer = new QmlProfilerFileWriter(this);
    writer->setTraceTime(traceTime()->startTime(), traceTime()->endTime(),
                        traceTime()->duration());
    writer->setData(d->model);
    writer->setNotes(d->notesModel->notes());

    connect(writer, &QObject::destroyed, this, &QmlProfilerModelManager::saveFinished,
            Qt::QueuedConnection);

    QFuture<void> result = Utils::runAsync([file, writer] (QFutureInterface<void> &future) {
        writer->setFuture(&future);
        if (file->fileName().endsWith(QLatin1String(Constants::QtdFileExtension)))
            writer->saveQtd(file);
        else
            writer->saveQzt(file);
        writer->deleteLater();
        file->deleteLater();
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

    connect(reader, &QmlProfilerFileReader::error, this, [this, reader](const QString &message) {
        delete reader;
        emit error(message);
    }, Qt::QueuedConnection);

    connect(reader, &QmlProfilerFileReader::typesLoaded,
            this, &QmlProfilerModelManager::addEventTypes);

    connect(reader, &QmlProfilerFileReader::notesLoaded,
            d->notesModel, &QmlProfilerNotesModel::setNotes);

    connect(reader, &QmlProfilerFileReader::qmlEventsLoaded,
            this, &QmlProfilerModelManager::addEvents);

    connect(reader, &QmlProfilerFileReader::success, this, [this, reader]() {
        d->traceTime->setTime(reader->traceStart(), reader->traceEnd());
        setRecordedFeatures(reader->loadedFeatures());
        delete reader;
        acquiringDone();
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
            // we're not supposed to receive new data while processing older data
            QTC_ASSERT(d->state != ProcessingData, return);
        break;
        case ProcessingData:
            QTC_ASSERT(d->state == AcquiringData, return);
        break;
        case Done:
            QTC_ASSERT(d->state == ProcessingData || d->state == Empty, return);
        break;
        default:
            emit error(tr("Trying to set unknown state in events list."));
        break;
    }

    d->state = state;
    emit stateChanged();
}

QmlProfilerModelManager::State QmlProfilerModelManager::state() const
{
    return d->state;
}

void QmlProfilerModelManager::clear()
{
    setState(ClearingData);
    d->numLoadedEvents = 0;
    d->numFinishedFinalizers = 0;
    d->model->clear();
    d->traceTime->clear();
    d->notesModel->clear();
    setVisibleFeatures(0);
    setRecordedFeatures(0);

    setState(Empty);
}

void QmlProfilerModelManager::restrictToRange(qint64 startTime, qint64 endTime)
{
    d->notesModel->saveData();
    const QVector<QmlNote> notes = d->notesModel->notes();
    d->notesModel->clear();

    setState(ClearingData);
    setVisibleFeatures(0);

    startAcquiring();
    d->model->replayEvents(startTime, endTime,
                           std::bind(&QmlProfilerModelManagerPrivate::dispatch, d,
                                     std::placeholders::_1, std::placeholders::_2));
    d->notesModel->setNotes(notes);
    d->traceTime->restrictToRange(startTime, endTime);
    acquiringDone();
}

bool QmlProfilerModelManager::isRestrictedToRange() const
{
    return d->traceTime->isRestrictedToRange();
}

void QmlProfilerModelManager::startAcquiring()
{
    setState(AcquiringData);
}

} // namespace QmlProfiler
