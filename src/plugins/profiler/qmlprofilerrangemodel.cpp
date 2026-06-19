// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertr.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerrangemodel.h"

#include <tracing/timelineformattime.h>

#include <QCoreApplication>
#include <QHash>
#include <QStack>
#include <QString>
#include <QUrl>

using namespace QmlDebug;
namespace Profiler::Internal {

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
    m_data.clear();
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
        m_data.insert(index, Item());
    } else if (event.rangeStage() == RangeEnd) {
        if (!m_stack.isEmpty()) {
            int index = m_stack.pop();
            insertEnd(index, event.timestamp() - startTime(index));
        } else {
            qWarning() << "Received inconsistent trace data from application.";
        }
    }
}

void QmlProfilerRangeModel::finalize()
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

    QmlProfilerTimelineModel::finalize();
}

void QmlProfilerRangeModel::computeNestingContracted()
{
    int i;
    int eventCount = count();

    int nestingLevels = Constants::QML_MIN_LEVEL;
    int collapsedRowCount = nestingLevels + 1;
    QList<qint64> nestingEndTimes;
    nestingEndTimes.fill(0, nestingLevels + 1);

    for (i = 0; i < eventCount; i++) {
        qint64 st = startTime(i);

        // per type
        if (nestingEndTimes[nestingLevels] > st) {
            if (++nestingLevels == nestingEndTimes.size())
                nestingEndTimes << 0;
            if (nestingLevels == collapsedRowCount)
                ++collapsedRowCount;
        } else {
            while (nestingLevels > Constants::QML_MIN_LEVEL &&
                   nestingEndTimes[nestingLevels-1] <= st)
                nestingLevels--;
        }
        nestingEndTimes[nestingLevels] = st + duration(i);

        m_data[i].displayRowCollapsed = nestingLevels;
    }
    setCollapsedRowCount(collapsedRowCount);
}

void QmlProfilerRangeModel::computeExpandedLevels()
{
    QHash<int, int> eventRow;
    int eventCount = count();
    for (int i = 0; i < eventCount; i++) {
        int eventTypeId = typeId(i);
        if (!eventRow.contains(eventTypeId)) {
            eventRow[eventTypeId] = m_expandedRowTypes.size();
            m_expandedRowTypes << eventTypeId;
        }
        m_data[i].displayRowExpanded = eventRow[eventTypeId];
    }
    setExpandedRowCount(m_expandedRowTypes.size());
}

int QmlProfilerRangeModel::expandedRow(int index) const
{
    return m_data[index].displayRowExpanded;
}

int QmlProfilerRangeModel::collapsedRow(int index) const
{
    return m_data[index].displayRowCollapsed;
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

Timeline::ItemLocation QmlProfilerRangeModel::location(int index) const
{
    return locationFromTypeId(index);
}

int QmlProfilerRangeModel::typeId(int index) const
{
    return selectionId(index);
}

} // namespace Profiler::Internal
