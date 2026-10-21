// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerrangemodel.h"

#include <tracing/timelineformatdata.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QHash>
#include <QStack>
#include <QString>
#include <QUrl>

#include <limits>

using namespace QmlDebug;
namespace Profiler::Internal {

// Collapsed rows from this one on do not fit into a byte.
constexpr int DeepCollapsedRow = std::numeric_limits<quint8>::max();

QmlProfilerRangeModel::QmlProfilerRangeModel(QmlProfilerModelManager *manager, RangeType range,
                                             Timeline::TimelineModelAggregator *parent) :
    QmlProfilerTimelineModel(manager, UndefinedMessage, range, featureFromRangeType(range), parent)
{
    m_expandedRowTypes << -1;
}

void QmlProfilerRangeModel::clear()
{
    m_expandedRowTypes.clear();
    m_expandedRowTypes << -1;
    // Not clear(), which keeps the storage, see TimelineModel::clear().
    m_collapsedRows = QList<quint8>();
    m_deepCollapsedRows.clear();
    m_expandedRowsByType.clear();
    m_stack.clear();
    QmlProfilerTimelineModel::clear();
}

void QmlProfilerRangeModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    Q_UNUSED(type)
    // store starttime-based instance
    if (event.rangeStage() == RangeStart) {
        int index = insertStart(event.timestamp(), event.typeIndex());
        m_stack.append(index);
    } else if (event.rangeStage() == RangeEnd) {
        if (!m_stack.isEmpty()) {
            int index = m_stack.pop();
            insertEnd(index, event.timestamp() - startTime(index));
        } else {
            qWarning() << "Received inconsistent trace data from application.";
        }
    }
}

void QmlProfilerRangeModel::prepare()
{
    if (!m_stack.isEmpty()) {
        qWarning() << "End times for some events are missing.";
        const qint64 endTime = modelManager()->traceEnd();
        do {
            int index = m_stack.pop();
            insertEnd(index, endTime - startTime(index));
        } while (!m_stack.isEmpty());
    }

    // compute range nesting
    computeNesting();

    // compute nestingLevel - nonexpanded
    computeNestingContracted();

    // compute nestingLevel - expanded
    computeExpandedLevels();
}

void QmlProfilerRangeModel::computeNestingContracted()
{
    int i;
    int eventCount = count();

    int nestingLevels = Timeline::kCollapsedStartRow;
    int collapsedRowCount = nestingLevels + 1;
    QList<qint64> nestingEndTimes;
    nestingEndTimes.fill(0, nestingLevels + 1);
    m_collapsedRows.resize(eventCount);
    m_deepCollapsedRows.clear();

    for (i = 0; i < eventCount; i++) {
        qint64 st = startTime(i);

        // per type
        if (nestingEndTimes[nestingLevels] > st) {
            if (++nestingLevels == nestingEndTimes.size())
                nestingEndTimes << 0;
            if (nestingLevels == collapsedRowCount)
                ++collapsedRowCount;
        } else {
            while (nestingLevels > Timeline::kCollapsedStartRow &&
                   nestingEndTimes[nestingLevels-1] <= st)
                nestingLevels--;
        }
        nestingEndTimes[nestingLevels] = st + duration(i);

        if (nestingLevels < DeepCollapsedRow) {
            m_collapsedRows[i] = quint8(nestingLevels);
        } else {
            m_collapsedRows[i] = DeepCollapsedRow;
            m_deepCollapsedRows.insert(i, nestingLevels);
        }
    }
    setCollapsedRowCount(collapsedRowCount);
}

void QmlProfilerRangeModel::computeExpandedLevels()
{
    int eventCount = count();
    for (int i = 0; i < eventCount; i++) {
        const int eventTypeId = typeId(i);
        QTC_ASSERT(eventTypeId >= 0, continue);
        if (eventTypeId >= m_expandedRowsByType.size())
            m_expandedRowsByType.resize(eventTypeId + 1, -1);
        int &row = m_expandedRowsByType[eventTypeId];
        if (row < 0) {
            row = m_expandedRowTypes.size();
            m_expandedRowTypes << eventTypeId;
        }
    }
    setExpandedRowCount(m_expandedRowTypes.size());
}

int QmlProfilerRangeModel::expandedRow(int index) const
{
    const int eventTypeId = typeId(index);
    const int row = eventTypeId >= 0 && eventTypeId < m_expandedRowsByType.size()
                        ? m_expandedRowsByType[eventTypeId] : -1;
    return row < 0 ? 1 : row;
}

int QmlProfilerRangeModel::collapsedRow(int index) const
{
    if (index >= m_collapsedRows.size())
        return Timeline::kCollapsedStartRow;
    const int row = m_collapsedRows[index];
    return row < DeepCollapsedRow ? row : m_deepCollapsedRows.value(index);
}

QRgb QmlProfilerRangeModel::color(int index) const
{
    return colorBySelectionId(index);
}

Timeline::RowLabels QmlProfilerRangeModel::labels() const
{
    Timeline::RowLabels result;

    const QmlProfilerModelManager *manager = modelManager();
    for (int i = 1; i < expandedRowCount(); i++) { // Ignore the -1 for the first row
        const int typeId = m_expandedRowTypes[i];
        const QmlEventType &type = manager->eventType(typeId);
        result.append({type.data(), typeId});
    }

    return result;
}

Timeline::ItemDetails QmlProfilerRangeModel::details(int index) const
{
    Timeline::ItemDetails result;
    int id = selectionId(index);

    result.insert(QStringLiteral("displayName"),
                  Tr::tr(QmlProfilerModelManager::featureName(mainFeature())));
    result.insert(Tr::tr("Duration"), Timeline::formatTime(duration(index)));

    const QmlEventType &type = modelManager()->eventType(id);
    result.insert(Tr::tr("Details"), type.data());
    result.insert(Tr::tr("Location"), type.displayName());
    return result;
}

QString QmlProfilerRangeModel::itemLabel(int index) const
{
    return modelManager()->eventType(selectionId(index)).data();
}

Timeline::ItemLocation QmlProfilerRangeModel::location(int index) const
{
    return locationFromTypeId(index);
}

int QmlProfilerRangeModel::typeId(int index) const
{
    return selectionId(index);
}

} // namespace Profiler::Internal
