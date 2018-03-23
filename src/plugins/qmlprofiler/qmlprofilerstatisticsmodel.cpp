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

QString QmlProfilerStatisticsModel::nameForType(RangeType typeNumber)
{
    switch (typeNumber) {
    case Painting: return QmlProfilerStatisticsModel::tr("Painting");
    case Compiling: return QmlProfilerStatisticsModel::tr("Compiling");
    case Creating: return QmlProfilerStatisticsModel::tr("Creating");
    case Binding: return QmlProfilerStatisticsModel::tr("Binding");
    case HandlingSignal: return QmlProfilerStatisticsModel::tr("Handling Signal");
    case Javascript: return QmlProfilerStatisticsModel::tr("JavaScript");
    default: return QString();
    }
}

double QmlProfilerStatisticsModel::durationPercent(int typeId) const
{
    return (typeId >= 0)
            ? double(m_data[typeId].totalNonRecursive()) / double(m_rootDuration) * 100
            : 100;
}

double QmlProfilerStatisticsModel::durationSelfPercent(int typeId) const
{
    return (typeId >= 0)
            ? (double(m_data[typeId].self) / double(m_rootDuration) * 100)
            : 0;
}

QmlProfilerStatisticsModel::QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager)
    : m_modelManager(modelManager)
{
    connect(modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerStatisticsModel::modelManagerStateChanged);
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

const QVector<QmlProfilerStatisticsModel::QmlEventStats> &
QmlProfilerStatisticsModel::getData() const
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

QStringList QmlProfilerStatisticsModel::details(int typeIndex) const
{
    QString data;
    QString displayName;
    if (typeIndex < 0) {
        data = tr("Main Program");
    } else {
        const QmlEventType &type = m_modelManager->eventTypes().at(typeIndex);
        displayName = nameForType(type.rangeType());

        const QChar ellipsisChar(0x2026);
        const int maxColumnWidth = 32;

        data = type.data();
        if (data.length() > maxColumnWidth)
            data = data.left(maxColumnWidth - 1) + ellipsisChar;
    }

    return QStringList({
        displayName,
        data,
        QString::number(durationPercent(typeIndex), 'f', 2) + QLatin1Char('%')
    });
}

QString QmlProfilerStatisticsModel::summary(const QVector<int> &typeIds) const
{
    const double cutoff = 0.1;
    const double round = 0.05;
    double maximum = 0;
    double sum = 0;

    for (int typeId : typeIds) {
        const double percentage = durationPercent(typeId);
        if (percentage > maximum)
            maximum = percentage;
        sum += percentage;
    }

    const QLatin1Char percent('%');

    if (sum < cutoff)
        return QLatin1Char('<') + QString::number(cutoff, 'f', 1) + percent;

    if (typeIds.length() == 1)
        return QLatin1Char('~') + QString::number(maximum, 'f', 1) + percent;

    // add/subtract 0.05 to avoid problematic rounding
    if (maximum < cutoff)
        return QChar(0x2264) + QString::number(sum + round, 'f', 1) + percent;

    return QChar(0x2265) + QString::number(qMax(maximum - round, cutoff), 'f', 1) + percent;
}

void QmlProfilerStatisticsModel::clear()
{
    m_rootDuration = 0;
    m_data.clear();
    m_notes.clear();
    m_callStack.clear();
    m_compileStack.clear();
    if (!m_calleesModel.isNull())
        m_calleesModel->clear();
    if (!m_callersModel.isNull())
        m_callersModel->clear();
}

void QmlProfilerStatisticsModel::setRelativesModel(QmlProfilerStatisticsRelativesModel *relative,
                                                   QmlProfilerStatisticsRelation relation)
{
    if (relation == QmlProfilerStatisticsCallers)
        m_callersModel = relative;
    else
        m_calleesModel = relative;
}

void QmlProfilerStatisticsModel::modelManagerStateChanged()
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

    const int typeIndex = event.typeIndex();
    bool isRecursive = false;
    QStack<QmlEvent> &stack = type.rangeType() == Compiling ? m_compileStack : m_callStack;
    switch (event.rangeStage()) {
    case RangeStart:
        stack.push(event);
        if (m_data.length() <= typeIndex)
            m_data.resize(m_modelManager->numLoadedEventTypes());
        break;
    case RangeEnd: {
        // update stats
        QTC_ASSERT(!stack.isEmpty(), return);
        QTC_ASSERT(stack.top().typeIndex() == typeIndex, return);
        QmlEventStats &stats = m_data[typeIndex];
        qint64 duration = event.timestamp() - stack.top().timestamp();
        stats.total += duration;
        stats.self += duration;
        stats.durations.push_back(duration);
        stack.pop();

        // recursion detection: check whether event was already in stack
        for (int ii = 0; ii < stack.size(); ++ii) {
            if (stack.at(ii).typeIndex() == typeIndex) {
                isRecursive = true;
                stats.recursive += duration;
                break;
            }
        }

        if (!stack.isEmpty())
            m_data[stack.top().typeIndex()].self -= duration;
        else
            m_rootDuration += duration;

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
    for (QmlEventStats &stats : m_data)
        stats.finalize();

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

const QVector<QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData> &
QmlProfilerStatisticsRelativesModel::getData(int typeId) const
{
    auto it = m_data.find(typeId);
    if (it != m_data.end()) {
        return it.value();
    } else {
        static const QVector<QmlStatisticsRelativesData> emptyVector;
        return emptyVector;
    }
}

const QVector<QmlEventType> &QmlProfilerStatisticsRelativesModel::getTypes() const
{
    return m_modelManager->eventTypes();
}

bool operator<(const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &a,
               const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &b)
{
    return a.typeIndex < b.typeIndex;
}

bool operator<(const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &a,
               int typeIndex)
{
    return a.typeIndex < typeIndex;
}

void QmlProfilerStatisticsRelativesModel::loadEvent(RangeType type, const QmlEvent &event,
                                                    bool isRecursive)
{
    QStack<Frame> &stack = (type == Compiling) ? m_compileStack : m_callStack;

    switch (event.rangeStage()) {
    case RangeStart:
        stack.push(Frame(event.timestamp(), event.typeIndex()));
        break;
    case RangeEnd: {
        int callerTypeIndex = stack.count() > 1 ? stack[stack.count() - 2].typeId : -1;
        int relativeTypeIndex = (m_relation == QmlProfilerStatisticsCallers) ? callerTypeIndex :
                                                                               event.typeIndex();
        int selfTypeIndex = (m_relation == QmlProfilerStatisticsCallers) ? event.typeIndex() :
                                                                           callerTypeIndex;

        QVector<QmlStatisticsRelativesData> &relatives = m_data[selfTypeIndex];
        auto it = std::lower_bound(relatives.begin(), relatives.end(), relativeTypeIndex);
        if (it != relatives.end() && it->typeIndex == relativeTypeIndex) {
            it->calls++;
            it->duration += event.timestamp() - stack.top().startTime;
            it->isRecursive = isRecursive || it->isRecursive;
        } else {
            relatives.insert(it, QmlStatisticsRelativesData(
                                 event.timestamp() - stack.top().startTime, 1, relativeTypeIndex,
                                 isRecursive));
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
