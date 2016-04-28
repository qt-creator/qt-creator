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

#include "memoryusagemodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilereventtypes.h"

#include <QStack>

namespace QmlProfiler {
namespace Internal {

MemoryUsageModel::MemoryUsageModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, MemoryAllocation, MaximumRangeType, ProfileMemory, parent)
{
    m_maxSize = 1;
    announceFeatures((1ULL << mainFeature()) | Constants::QML_JS_RANGE_FEATURES);
}

int MemoryUsageModel::rowMaxValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return m_maxSize;
}

int MemoryUsageModel::expandedRow(int index) const
{
    int type = selectionId(index);
    return (type == HeapPage || type == LargeItem) ? 1 : 2;
}

int MemoryUsageModel::collapsedRow(int index) const
{
    return expandedRow(index);
}

int MemoryUsageModel::typeId(int index) const
{
    return m_data[index].typeId;
}

QColor MemoryUsageModel::color(int index) const
{
    return colorBySelectionId(index);
}

float MemoryUsageModel::relativeHeight(int index) const
{
    return qMin(1.0f, (float)m_data[index].size / (float)m_maxSize);
}

QVariantMap MemoryUsageModel::location(int index) const
{
    static const QLatin1String file("file");
    static const QLatin1String line("line");
    static const QLatin1String column("column");

    QVariantMap result;

    int originType = m_data[index].originTypeIndex;
    if (originType > -1) {
        const QmlEventLocation &location =
                modelManager()->qmlModel()->eventTypes().at(originType).location;

        result.insert(file, location.filename);
        result.insert(line, location.line);
        result.insert(column, location.column);
    }

    return result;
}

QVariantList MemoryUsageModel::labels() const
{
    QVariantList result;

    QVariantMap element;
    element.insert(QLatin1String("description"), QVariant(tr("Memory Allocation")));
    element.insert(QLatin1String("id"), QVariant(HeapPage));
    result << element;

    element.clear();
    element.insert(QLatin1String("description"), QVariant(tr("Memory Usage")));
    element.insert(QLatin1String("id"), QVariant(SmallItem));
    result << element;

    return result;
}

QVariantMap MemoryUsageModel::details(int index) const
{
    QVariantMap result;
    const MemoryAllocationItem *ev = &m_data[index];

    if (ev->allocated >= -ev->deallocated)
        result.insert(QLatin1String("displayName"), tr("Memory Allocated"));
    else
        result.insert(QLatin1String("displayName"), tr("Memory Freed"));

    result.insert(tr("Total"), QString::fromLatin1("%1 bytes").arg(ev->size));
    if (ev->allocations > 0) {
        result.insert(tr("Allocated"), QString::fromLatin1("%1 bytes").arg(ev->allocated));
        result.insert(tr("Allocations"), QString::number(ev->allocations));
    }
    if (ev->deallocations > 0) {
        result.insert(tr("Deallocated"), QString::fromLatin1("%1 bytes").arg(-ev->deallocated));
        result.insert(tr("Deallocations"), QString::number(ev->deallocations));
    }
    result.insert(tr("Type"), QVariant(memoryTypeName(selectionId(index))));

    if (ev->originTypeIndex != -1) {
        result.insert(tr("Location"),
                modelManager()->qmlModel()->eventTypes().at(ev->originTypeIndex).displayName);
    }
    return result;
}

struct RangeStackFrame {
    RangeStackFrame() : originTypeIndex(-1), startTime(-1), endTime(-1) {}
    RangeStackFrame(int originTypeIndex, qint64 startTime, qint64 endTime) :
        originTypeIndex(originTypeIndex), startTime(startTime), endTime(endTime) {}
    int originTypeIndex;
    qint64 startTime;
    qint64 endTime;
};

void MemoryUsageModel::loadData()
{
    QmlProfilerDataModel *simpleModel = modelManager()->qmlModel();
    if (simpleModel->isEmpty())
        return;

    qint64 currentSize = 0;
    qint64 currentUsage = 0;
    int currentUsageIndex = -1;
    int currentJSHeapIndex = -1;

    QStack<RangeStackFrame> rangeStack;

    const QVector<QmlEventType> &types = simpleModel->eventTypes();
    foreach (const QmlEvent &event, simpleModel->events()) {
        const QmlEventType &type = types[event.typeIndex()];
        while (!rangeStack.empty() && rangeStack.top().endTime < event.timestamp())
            rangeStack.pop();
        if (!accepted(type)) {
            if (type.rangeType != MaximumRangeType) {
                rangeStack.push(RangeStackFrame(event.typeIndex(), event.timestamp(),
                                                event.timestamp() + event.duration()));
            }
            continue;
        }

        if (type.detailType == SmallItem || type.detailType == LargeItem) {
            if (!rangeStack.empty() && currentUsageIndex > -1 &&
                    type.detailType == selectionId(currentUsageIndex) &&
                    m_data[currentUsageIndex].originTypeIndex == rangeStack.top().originTypeIndex &&
                    rangeStack.top().startTime < startTime(currentUsageIndex)) {
                m_data[currentUsageIndex].update(event.numericData(0));
                currentUsage = m_data[currentUsageIndex].size;
            } else {
                MemoryAllocationItem allocation(event.typeIndex(), currentUsage,
                        rangeStack.empty() ? -1 : rangeStack.top().originTypeIndex);
                allocation.update(event.numericData(0));
                currentUsage = allocation.size;

                if (currentUsageIndex != -1) {
                    insertEnd(currentUsageIndex,
                              event.timestamp() - startTime(currentUsageIndex) - 1);
                }
                currentUsageIndex = insertStart(event.timestamp(), SmallItem);
                m_data.insert(currentUsageIndex, allocation);
            }
        }

        if (type.detailType == HeapPage || type.detailType == LargeItem) {
            if (!rangeStack.empty() && currentJSHeapIndex > -1 &&
                    type.detailType == selectionId(currentJSHeapIndex) &&
                    m_data[currentJSHeapIndex].originTypeIndex ==
                    rangeStack.top().originTypeIndex &&
                    rangeStack.top().startTime < startTime(currentJSHeapIndex)) {
                m_data[currentJSHeapIndex].update(event.numericData(0));
                currentSize = m_data[currentJSHeapIndex].size;
            } else {
                MemoryAllocationItem allocation(event.typeIndex(), currentSize,
                        rangeStack.empty() ? -1 : rangeStack.top().originTypeIndex);
                allocation.update(event.numericData(0));
                currentSize = allocation.size;

                if (currentSize > m_maxSize)
                    m_maxSize = currentSize;
                if (currentJSHeapIndex != -1)
                    insertEnd(currentJSHeapIndex,
                              event.timestamp() - startTime(currentJSHeapIndex) - 1);
                currentJSHeapIndex = insertStart(event.timestamp(), type.detailType);
                m_data.insert(currentJSHeapIndex, allocation);
            }
        }

        updateProgress(count(), simpleModel->events().count());
    }

    if (currentJSHeapIndex != -1)
        insertEnd(currentJSHeapIndex, modelManager()->traceTime()->endTime() -
                  startTime(currentJSHeapIndex) - 1);
    if (currentUsageIndex != -1)
        insertEnd(currentUsageIndex, modelManager()->traceTime()->endTime() -
                  startTime(currentUsageIndex) - 1);


    computeNesting();
    setExpandedRowCount(3);
    setCollapsedRowCount(3);
    updateProgress(1, 1);
}

void MemoryUsageModel::clear()
{
    m_data.clear();
    m_maxSize = 1;
    QmlProfilerTimelineModel::clear();
}

QString MemoryUsageModel::memoryTypeName(int type)
{
    switch (type) {
    case HeapPage: return tr("Heap Allocation");
    case LargeItem: return tr("Large Item Allocation");
    case SmallItem: return tr("Heap Usage");
    case MaximumMemoryType: return tr("Total");
    default: return tr("Unknown");
    }
}

MemoryUsageModel::MemoryAllocationItem::MemoryAllocationItem(int type, qint64 baseAmount,
                                                     int originTypeIndex) :
    typeId(type), size(baseAmount), allocated(0), deallocated(0), allocations(0), deallocations(0),
    originTypeIndex(originTypeIndex)
{
}

void MemoryUsageModel::MemoryAllocationItem::update(qint64 amount)
{
    size += amount;
    if (amount < 0) {
        deallocated += amount;
        ++deallocations;
    } else {
        allocated += amount;
        ++allocations;
    }
}


} // namespace Internal
} // namespace QmlProfiler
