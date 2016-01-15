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

#include "qmlprofilerstatisticsmodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>
#include <QElapsedTimer>

#include <QDebug>

namespace QmlProfiler {

class QmlProfilerStatisticsModel::QmlProfilerStatisticsModelPrivate
{
public:
    QmlProfilerStatisticsModelPrivate(QmlProfilerStatisticsModel *qq) : q(qq) {}
    ~QmlProfilerStatisticsModelPrivate() {}

    QHash<int, QmlProfilerStatisticsModel::QmlEventStats> data;

    QmlProfilerModelManager *modelManager;
    QmlProfilerStatisticsModel *q;

    int modelId;

    QList<QmlDebug::RangeType> acceptedTypes;
    QSet<int> eventsInBindingLoop;
    QHash<int, QString> notes;
};

QmlProfilerStatisticsModel::QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager,
                                               QObject *parent) :
    QObject(parent), d(new QmlProfilerStatisticsModelPrivate(this))
{
    d->modelManager = modelManager;
    connect(modelManager->qmlModel(), &QmlProfilerDataModel::changed,
            this, &QmlProfilerStatisticsModel::dataChanged);
    connect(modelManager->notesModel(), &Timeline::TimelineNotesModel::changed,
            this, &QmlProfilerStatisticsModel::notesChanged);
    d->modelId = modelManager->registerModelProxy();

    // We're iterating twice in loadData.
    modelManager->setProxyCountWeight(d->modelId, 2);

    d->acceptedTypes << QmlDebug::Compiling << QmlDebug::Creating << QmlDebug::Binding << QmlDebug::HandlingSignal << QmlDebug::Javascript;

    modelManager->announceFeatures(d->modelId, QmlDebug::Constants::QML_JS_RANGE_FEATURES);
}

QmlProfilerStatisticsModel::~QmlProfilerStatisticsModel()
{
    delete d;
}

void QmlProfilerStatisticsModel::setEventTypeAccepted(QmlDebug::RangeType type, bool accepted)
{
    if (accepted && !d->acceptedTypes.contains(type))
        d->acceptedTypes << type;
    else if (!accepted && d->acceptedTypes.contains(type))
        d->acceptedTypes.removeOne(type);
}

bool QmlProfilerStatisticsModel::eventTypeAccepted(QmlDebug::RangeType type) const
{
    return d->acceptedTypes.contains(type);
}

const QHash<int, QmlProfilerStatisticsModel::QmlEventStats> &QmlProfilerStatisticsModel::getData() const
{
    return d->data;
}

const QVector<QmlProfilerDataModel::QmlEventTypeData> &QmlProfilerStatisticsModel::getTypes() const
{
    return d->modelManager->qmlModel()->getEventTypes();
}

const QHash<int, QString> &QmlProfilerStatisticsModel::getNotes() const
{
    return d->notes;
}

void QmlProfilerStatisticsModel::clear()
{
    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
    d->data.clear();
    d->eventsInBindingLoop.clear();
    d->notes.clear();
}

void QmlProfilerStatisticsModel::limitToRange(qint64 rangeStart, qint64 rangeEnd)
{
    if (!d->modelManager->isEmpty())
        loadData(rangeStart, rangeEnd);
}

void QmlProfilerStatisticsModel::dataChanged()
{
    if (d->modelManager->state() == QmlProfilerModelManager::ProcessingData)
        loadData();
    else if (d->modelManager->state() == QmlProfilerModelManager::ClearingData)
        clear();
}

void QmlProfilerStatisticsModel::notesChanged(int typeIndex)
{
    const QmlProfilerNotesModel *notesModel = d->modelManager->notesModel();
    if (typeIndex == -1) {
        d->notes.clear();
        for (int noteId = 0; noteId < notesModel->count(); ++noteId) {
            int noteType = notesModel->typeId(noteId);
            if (noteType != -1) {
                QString &note = d->notes[noteType];
                if (note.isEmpty()) {
                    note = notesModel->text(noteId);
                } else {
                    note.append(QStringLiteral("\n")).append(notesModel->text(noteId));
                }
            }
        }
    } else {
        d->notes.remove(typeIndex);
        const QVariantList changedNotes = notesModel->byTypeId(typeIndex);
        if (!changedNotes.isEmpty()) {
            QStringList newNotes;
            for (QVariantList::ConstIterator it = changedNotes.begin(); it !=  changedNotes.end();
                 ++it) {
                newNotes << notesModel->text(it->toInt());
            }
            d->notes[typeIndex] = newNotes.join(QStringLiteral("\n"));
        }
    }

    emit notesAvailable(typeIndex);
}

const QSet<int> &QmlProfilerStatisticsModel::eventsInBindingLoop() const
{
    return d->eventsInBindingLoop;
}

void QmlProfilerStatisticsModel::loadData(qint64 rangeStart, qint64 rangeEnd)
{
    clear();

    qint64 qmlTime = 0;
    qint64 lastEndTime = 0;
    QHash <int, QVector<qint64> > durations;

    const bool checkRanges = (rangeStart != -1) && (rangeEnd != -1);

    const QVector<QmlProfilerDataModel::QmlEventData> &eventList
            = d->modelManager->qmlModel()->getEvents();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &typesList
            = d->modelManager->qmlModel()->getEventTypes();

    // used by binding loop detection
    QStack<const QmlProfilerDataModel::QmlEventData*> callStack;
    callStack.push(0); // artificial root

    for (int i = 0; i < eventList.size(); ++i) {
        const QmlProfilerDataModel::QmlEventData *event = &eventList[i];
        const QmlProfilerDataModel::QmlEventTypeData *type = &typesList[event->typeIndex()];

        if (!d->acceptedTypes.contains(type->rangeType))
            continue;

        if (checkRanges) {
            if ((event->startTime() + event->duration() < rangeStart)
                    || (event->startTime() > rangeEnd))
                continue;
        }

        // update stats
        QmlEventStats *stats = &d->data[event->typeIndex()];

        stats->duration += event->duration();
        stats->durationSelf += event->duration();
        if (event->duration() < stats->minTime)
            stats->minTime = event->duration();
        if (event->duration() > stats->maxTime)
            stats->maxTime = event->duration();
        stats->calls++;

        // for median computing
        durations[event->typeIndex()].append(event->duration());

        // qml time computation
        if (event->startTime() > lastEndTime) { // assume parent event if starts before last end
            qmlTime += event->duration();
            lastEndTime = event->startTime() + event->duration();
        }

        //
        // binding loop detection
        //
        const QmlProfilerDataModel::QmlEventData *potentialParent = callStack.top();
        while (potentialParent && !(potentialParent->startTime() + potentialParent->duration() >
                    event->startTime())) {
            callStack.pop();
            potentialParent = callStack.top();
        }

        // check whether event is already in stack
        for (int ii = 1; ii < callStack.size(); ++ii) {
            if (callStack.at(ii)->typeIndex() == event->typeIndex()) {
                d->eventsInBindingLoop.insert(event->typeIndex());
                break;
            }
        }

        if (callStack.count() > 1)
            d->data[callStack.top()->typeIndex()].durationSelf -= event->duration();
        callStack.push(event);

        d->modelManager->modelProxyCountUpdated(d->modelId, i, eventList.count()*2);
    }

    // post-process: calc mean time, median time, percentoftime
    int i = d->data.size();
    int total = i * 2;

    for (QHash<int, QmlEventStats>::iterator it = d->data.begin(); it != d->data.end(); ++it) {
        QmlEventStats* stats = &it.value();
        if (stats->calls > 0)
            stats->timePerCall = stats->duration / (double)stats->calls;

        QVector<qint64> eventDurations = durations[it.key()];
        if (!eventDurations.isEmpty()) {
            Utils::sort(eventDurations);
            stats->medianTime = eventDurations.at(eventDurations.count()/2);
        }

        stats->percentOfTime = stats->duration * 100.0 / qmlTime;
        stats->percentSelf = stats->durationSelf * 100.0 / qmlTime;
        d->modelManager->modelProxyCountUpdated(d->modelId, i++, total);
    }

    // set binding loop flag
    foreach (int typeIndex, d->eventsInBindingLoop)
        d->data[typeIndex].isBindingLoop = true;

    // insert root event
    QmlEventStats rootEvent;
    rootEvent.duration = rootEvent.minTime = rootEvent.maxTime = rootEvent.timePerCall
                       = rootEvent.medianTime = qmlTime + 1;
    rootEvent.durationSelf = 1;
    rootEvent.calls = 1;
    rootEvent.percentOfTime = 100.0;
    rootEvent.percentSelf = 1.0 / rootEvent.duration;

    d->data.insert(-1, rootEvent);

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
    emit dataAvailable();
}

int QmlProfilerStatisticsModel::count() const
{
    return d->data.count();
}

QmlProfilerStatisticsRelativesModel::QmlProfilerStatisticsRelativesModel(
        QmlProfilerModelManager *modelManager, QmlProfilerStatisticsModel *statisticsModel,
        QObject *parent) : QObject(parent)
{
    QTC_CHECK(modelManager);
    m_modelManager = modelManager;

    QTC_CHECK(statisticsModel);
    m_statisticsModel = statisticsModel;

    // Load the child models whenever the parent model is done to get the filtering for JS/QML
    // right.
    connect(m_statisticsModel, &QmlProfilerStatisticsModel::dataAvailable,
            this, &QmlProfilerStatisticsRelativesModel::dataChanged);
}

const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesMap &
QmlProfilerStatisticsRelativesModel::getData(int typeId) const
{
    QHash <int, QmlStatisticsRelativesMap>::ConstIterator it = m_data.find(typeId);
    if (it != m_data.end()) {
        return it.value();
    } else {
        static const QmlStatisticsRelativesMap emptyMap;
        return emptyMap;
    }
}

const QVector<QmlProfilerDataModel::QmlEventTypeData> &
QmlProfilerStatisticsRelativesModel::getTypes() const
{
    return m_modelManager->qmlModel()->getEventTypes();
}

int QmlProfilerStatisticsRelativesModel::count() const
{
    return m_data.count();
}

void QmlProfilerStatisticsRelativesModel::clear()
{
    m_data.clear();
}

void QmlProfilerStatisticsRelativesModel::dataChanged()
{
    loadData();

    emit dataAvailable();
}

QmlProfilerStatisticsParentsModel::QmlProfilerStatisticsParentsModel(
        QmlProfilerModelManager *modelManager, QmlProfilerStatisticsModel *statisticsModel,
        QObject *parent) :
    QmlProfilerStatisticsRelativesModel(modelManager, statisticsModel, parent)
{}

void QmlProfilerStatisticsParentsModel::loadData()
{
    clear();
    QmlProfilerDataModel *simpleModel = m_modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    // for level computation
    QHash<int, qint64> endtimesPerLevel;
    int level = QmlDebug::Constants::QML_MIN_LEVEL;
    endtimesPerLevel[0] = 0;

    const QSet<int> &eventsInBindingLoop = m_statisticsModel->eventsInBindingLoop();

    // compute parent-child relationship and call count
    QHash<int, int> lastParent;
    const QVector<QmlProfilerDataModel::QmlEventData> eventList = simpleModel->getEvents();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> typesList = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        // whitelist
        if (!m_statisticsModel->eventTypeAccepted(typesList[event.typeIndex()].rangeType))
            continue;

        // level computation
        if (endtimesPerLevel[level] > event.startTime()) {
            level++;
        } else {
            while (level > QmlDebug::Constants::QML_MIN_LEVEL &&
                   endtimesPerLevel[level-1] <= event.startTime())
                level--;
        }
        endtimesPerLevel[level] = event.startTime() + event.duration();

        int parentTypeIndex = -1;
        if (level > QmlDebug::Constants::QML_MIN_LEVEL && lastParent.contains(level-1))
            parentTypeIndex = lastParent[level-1];

        QmlStatisticsRelativesMap &relativesMap = m_data[event.typeIndex()];
        QmlStatisticsRelativesMap::Iterator it = relativesMap.find(parentTypeIndex);
        if (it != relativesMap.end()) {
            it.value().calls++;
            it.value().duration += event.duration();
        } else {
            QmlStatisticsRelativesData parent = {
                event.duration(),
                1,
                eventsInBindingLoop.contains(parentTypeIndex)
            };
            relativesMap.insert(parentTypeIndex, parent);
        }

        // now lastparent is the new type
        lastParent[level] = event.typeIndex();
    }
}

QmlProfilerStatisticsChildrenModel::QmlProfilerStatisticsChildrenModel(
        QmlProfilerModelManager *modelManager, QmlProfilerStatisticsModel *statisticsModel,
        QObject *parent) :
    QmlProfilerStatisticsRelativesModel(modelManager, statisticsModel, parent)
{}

void QmlProfilerStatisticsChildrenModel::loadData()
{
    clear();
    QmlProfilerDataModel *simpleModel = m_modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    // for level computation
    QHash<int, qint64> endtimesPerLevel;
    int level = QmlDebug::Constants::QML_MIN_LEVEL;
    endtimesPerLevel[0] = 0;

    const QSet<int> &eventsInBindingLoop = m_statisticsModel->eventsInBindingLoop();

    // compute parent-child relationship and call count
    QHash<int, int> lastParent;
    const QVector<QmlProfilerDataModel::QmlEventData> &eventList = simpleModel->getEvents();
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &typesList = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        // whitelist
        if (!m_statisticsModel->eventTypeAccepted(typesList[event.typeIndex()].rangeType))
            continue;

        // level computation
        if (endtimesPerLevel[level] > event.startTime()) {
            level++;
        } else {
            while (level > QmlDebug::Constants::QML_MIN_LEVEL &&
                   endtimesPerLevel[level-1] <= event.startTime())
                level--;
        }
        endtimesPerLevel[level] = event.startTime() + event.duration();

        int parentId = -1;

        if (level > QmlDebug::Constants::QML_MIN_LEVEL && lastParent.contains(level-1))
            parentId = lastParent[level-1];

        QmlStatisticsRelativesMap &relativesMap = m_data[parentId];
        QmlStatisticsRelativesMap::Iterator it = relativesMap.find(event.typeIndex());
        if (it != relativesMap.end()) {
            it.value().calls++;
            it.value().duration += event.duration();
        } else {
            QmlStatisticsRelativesData child = {
                event.duration(),
                1,
                eventsInBindingLoop.contains(parentId)
            };
            relativesMap.insert(event.typeIndex(), child);
        }

        // now lastparent is the new type
        lastParent[level] = event.typeIndex();
    }
}

}
