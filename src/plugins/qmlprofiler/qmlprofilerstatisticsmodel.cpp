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

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QSet>
#include <QString>

#include <functional>

namespace QmlProfiler {

double QmlProfilerStatisticsModel::durationPercent(int typeId) const
{
    const QmlEventStats &global = m_data[-1];
    const QmlEventStats &stats = m_data[typeId];
    return double(stats.duration - stats.durationRecursive) / double(global.duration) * 100l;
}

double QmlProfilerStatisticsModel::durationSelfPercent(int typeId) const
{
    const QmlEventStats &global = m_data[-1];
    const QmlEventStats &stats = m_data[typeId];
    return double(stats.durationSelf) / double(global.duration) * 100l;
}

QmlProfilerStatisticsModel::QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager)
    : m_modelManager(modelManager)
{
    connect(modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerStatisticsModel::dataChanged);
    connect(modelManager->notesModel(), &Timeline::TimelineNotesModel::changed,
            this, &QmlProfilerStatisticsModel::notesChanged);
    modelManager->registerModelProxy();

    m_acceptedTypes << Compiling << Creating << Binding << HandlingSignal << Javascript;

    modelManager->announceFeatures(Constants::QML_JS_RANGE_FEATURES,
                                   [this](const QmlEvent &event, const QmlEventType &type) {
        loadEvent(event, type);
    }, [this]() {
        finalize();
    });
}

void QmlProfilerStatisticsModel::restrictToFeatures(quint64 features)
{
    bool didChange = false;
    for (int i = 0; i < MaximumRangeType; ++i) {
        RangeType type = static_cast<RangeType>(i);
        quint64 featureFlag = 1ULL << featureFromRangeType(type);
        if (Constants::QML_JS_RANGE_FEATURES & featureFlag) {
            bool accepted = features & featureFlag;
            if (accepted && !m_acceptedTypes.contains(type)) {
                m_acceptedTypes << type;
                didChange = true;
            } else if (!accepted && m_acceptedTypes.contains(type)) {
                m_acceptedTypes.removeOne(type);
                didChange = true;
            }
        }
    }
    if (!didChange || m_modelManager->state() != QmlProfilerModelManager::Done)
        return;

    clear();
    if (!m_modelManager->replayEvents(m_modelManager->traceTime()->startTime(),
                                       m_modelManager->traceTime()->endTime(),
                                       std::bind(&QmlProfilerStatisticsModel::loadEvent,
                                                 this, std::placeholders::_1,
                                                 std::placeholders::_2))) {
        emit m_modelManager->error(tr("Could not re-read events from temporary trace file."));
        clear();
    } else {
        finalize();
        notesChanged(-1); // Reload notes
    }
}

bool QmlProfilerStatisticsModel::isRestrictedToRange() const
{
    return m_modelManager->isRestrictedToRange();
}

const QHash<int, QmlProfilerStatisticsModel::QmlEventStats> &QmlProfilerStatisticsModel::getData() const
{
    return m_data;
}

const QVector<QmlEventType> &QmlProfilerStatisticsModel::getTypes() const
{
    return m_modelManager->eventTypes();
}

const QHash<int, QString> &QmlProfilerStatisticsModel::getNotes() const
{
    return m_notes;
}

void QmlProfilerStatisticsModel::clear()
{
    m_data.clear();
    m_notes.clear();
    m_callStack.clear();
    m_compileStack.clear();
    m_durations.clear();
    if (!m_calleesModel.isNull())
        m_calleesModel->clear();
    if (!m_callersModel.isNull())
        m_callersModel->clear();
}

void QmlProfilerStatisticsModel::setRelativesModel(QmlProfilerStatisticsRelativesModel *relative,
                                                   QmlProfilerStatisticsRelation relation)
{
    if (relation == QmlProfilerStatisticsParents)
        m_callersModel = relative;
    else
        m_calleesModel = relative;
}

QmlProfilerModelManager *QmlProfilerStatisticsModel::modelManager() const
{
    return m_modelManager;
}

void QmlProfilerStatisticsModel::dataChanged()
{
    if (m_modelManager->state() == QmlProfilerModelManager::ClearingData)
        clear();
}

void QmlProfilerStatisticsModel::notesChanged(int typeIndex)
{
    const QmlProfilerNotesModel *notesModel = m_modelManager->notesModel();
    if (typeIndex == -1) {
        m_notes.clear();
        for (int noteId = 0; noteId < notesModel->count(); ++noteId) {
            int noteType = notesModel->typeId(noteId);
            if (noteType != -1) {
                QString &note = m_notes[noteType];
                if (note.isEmpty()) {
                    note = notesModel->text(noteId);
                } else {
                    note.append(QStringLiteral("\n")).append(notesModel->text(noteId));
                }
            }
        }
    } else {
        m_notes.remove(typeIndex);
        const QVariantList changedNotes = notesModel->byTypeId(typeIndex);
        if (!changedNotes.isEmpty()) {
            QStringList newNotes;
            for (QVariantList::ConstIterator it = changedNotes.begin(); it !=  changedNotes.end();
                 ++it) {
                newNotes << notesModel->text(it->toInt());
            }
            m_notes[typeIndex] = newNotes.join(QStringLiteral("\n"));
        }
    }

    emit notesAvailable(typeIndex);
}

void QmlProfilerStatisticsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    if (!m_acceptedTypes.contains(type.rangeType()))
        return;

    bool isRecursive = false;
    QStack<QmlEvent> &stack = type.rangeType() == Compiling ? m_compileStack : m_callStack;
    switch (event.rangeStage()) {
    case RangeStart:
        stack.push(event);
        break;
    case RangeEnd: {
        // update stats
        QTC_ASSERT(!stack.isEmpty(), return);
        QTC_ASSERT(stack.top().typeIndex() == event.typeIndex(), return);
        QmlEventStats *stats = &m_data[event.typeIndex()];
        qint64 duration = event.timestamp() - stack.top().timestamp();
        stats->duration += duration;
        stats->durationSelf += duration;
        if (duration < stats->minTime)
            stats->minTime = duration;
        if (duration > stats->maxTime)
            stats->maxTime = duration;
        stats->calls++;
        // for median computing
        m_durations[event.typeIndex()].append(duration);
        stack.pop();

        // recursion detection: check whether event was already in stack
        for (int ii = 0; ii < stack.size(); ++ii) {
            if (stack.at(ii).typeIndex() == event.typeIndex()) {
                isRecursive = true;
                stats->durationRecursive += duration;
                break;
            }
        }

        if (!stack.isEmpty())
            m_data[stack.top().typeIndex()].durationSelf -= duration;
        else
            m_data[-1].duration += duration;
        break;
    }
    default:
        return;
    }

    if (!m_calleesModel.isNull())
        m_calleesModel->loadEvent(type.rangeType(), event, isRecursive);
    if (!m_callersModel.isNull())
        m_callersModel->loadEvent(type.rangeType(), event, isRecursive);
}


void QmlProfilerStatisticsModel::finalize()
{
    // post-process: calc mean time, median time, percentoftime
    for (QHash<int, QmlEventStats>::iterator it = m_data.begin(); it != m_data.end(); ++it) {
        QVector<qint64> eventDurations = m_durations[it.key()];
        if (!eventDurations.isEmpty()) {
            Utils::sort(eventDurations);
            it->medianTime = eventDurations.at(eventDurations.count()/2);
        }
    }

    // insert root event
    QmlEventStats &rootEvent = m_data[-1];
    rootEvent.minTime = rootEvent.maxTime = rootEvent.medianTime = rootEvent.duration;
    rootEvent.durationSelf = 0;
    rootEvent.calls = 1;

    emit dataAvailable();
}

int QmlProfilerStatisticsModel::count() const
{
    return m_data.count();
}

QmlProfilerStatisticsRelativesModel::QmlProfilerStatisticsRelativesModel(
        QmlProfilerModelManager *modelManager,
        QmlProfilerStatisticsModel *statisticsModel,
        QmlProfilerStatisticsRelation relation) :
    m_modelManager(modelManager), m_relation(relation)
{
    QTC_CHECK(modelManager);
    QTC_CHECK(statisticsModel);
    statisticsModel->setRelativesModel(this, relation);

    // Load the child models whenever the parent model is done to get the filtering for JS/QML
    // right.
    connect(statisticsModel, &QmlProfilerStatisticsModel::dataAvailable,
            this, &QmlProfilerStatisticsRelativesModel::dataAvailable);
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

const QVector<QmlEventType> &QmlProfilerStatisticsRelativesModel::getTypes() const
{
    return m_modelManager->eventTypes();
}

void QmlProfilerStatisticsRelativesModel::loadEvent(RangeType type, const QmlEvent &event,
                                                    bool isRecursive)
{
    QStack<Frame> &stack = (type == Compiling) ? m_compileStack : m_callStack;

    switch (event.rangeStage()) {
    case RangeStart:
        stack.push({event.timestamp(), event.typeIndex()});
        break;
    case RangeEnd: {
        int parentTypeIndex = stack.count() > 1 ? stack[stack.count() - 2].typeId : -1;
        int relativeTypeIndex = (m_relation == QmlProfilerStatisticsParents) ? parentTypeIndex :
                                                                               event.typeIndex();
        int selfTypeIndex = (m_relation == QmlProfilerStatisticsParents) ? event.typeIndex() :
                                                                           parentTypeIndex;

        QmlStatisticsRelativesMap &relativesMap = m_data[selfTypeIndex];
        QmlStatisticsRelativesMap::Iterator it = relativesMap.find(relativeTypeIndex);
        if (it != relativesMap.end()) {
            it->calls++;
            it->duration += event.timestamp() - stack.top().startTime;
            it->isRecursive = isRecursive || it->isRecursive;
        } else {
            QmlStatisticsRelativesData relative = {
                event.timestamp() - stack.top().startTime,
                1,
                isRecursive
            };
            relativesMap.insert(relativeTypeIndex, relative);
        }
        stack.pop();
        break;
    }
    default:
        break;
    }
}

QmlProfilerStatisticsRelation QmlProfilerStatisticsRelativesModel::relation() const
{
    return m_relation;
}

int QmlProfilerStatisticsRelativesModel::count() const
{
    return m_data.count();
}

void QmlProfilerStatisticsRelativesModel::clear()
{
    m_data.clear();
    m_callStack.clear();
    m_compileStack.clear();
}

} // namespace QmlProfiler
