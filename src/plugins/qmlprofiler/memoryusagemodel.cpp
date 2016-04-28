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

namespace QmlProfiler {
namespace Internal {

MemoryUsageModel::MemoryUsageModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, MemoryAllocation, MaximumRangeType, ProfileMemory, parent)
{
    // Announce additional features. The base class already announces the main feature.
    announceFeatures(Constants::QML_JS_RANGE_FEATURES);
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

bool MemoryUsageModel::accepted(const QmlEventType &type) const
{
    return QmlProfilerTimelineModel::accepted(type) || type.rangeType != MaximumRangeType;
}

void MemoryUsageModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    if (type.message != MemoryAllocation) {
        if (type.rangeType != MaximumRangeType) {
            if (event.rangeStage() == RangeStart)
                m_rangeStack.push(RangeStackFrame(event.typeIndex(), event.timestamp()));
            else if (event.rangeStage() == RangeEnd)
                m_rangeStack.pop();
        }
        return;
    }

    if (type.detailType == SmallItem || type.detailType == LargeItem) {
        if (!m_rangeStack.empty() && m_currentUsageIndex > -1 &&
                type.detailType == selectionId(m_currentUsageIndex) &&
                m_data[m_currentUsageIndex].originTypeIndex == m_rangeStack.top().originTypeIndex &&
                m_rangeStack.top().startTime < startTime(m_currentUsageIndex)) {
            m_data[m_currentUsageIndex].update(event.number<qint64>(0));
            m_currentUsage = m_data[m_currentUsageIndex].size;
        } else {
            MemoryAllocationItem allocation(event.typeIndex(), m_currentUsage,
                    m_rangeStack.empty() ? -1 : m_rangeStack.top().originTypeIndex);
            allocation.update(event.number<qint64>(0));
            m_currentUsage = allocation.size;

            if (m_currentUsageIndex != -1) {
                insertEnd(m_currentUsageIndex,
                          event.timestamp() - startTime(m_currentUsageIndex) - 1);
            }
            m_currentUsageIndex = insertStart(event.timestamp(), SmallItem);
            m_data.insert(m_currentUsageIndex, allocation);
        }
    }

    if (type.detailType == HeapPage || type.detailType == LargeItem) {
        if (!m_rangeStack.empty() && m_currentJSHeapIndex > -1 &&
                type.detailType == selectionId(m_currentJSHeapIndex) &&
                m_data[m_currentJSHeapIndex].originTypeIndex ==
                m_rangeStack.top().originTypeIndex &&
                m_rangeStack.top().startTime < startTime(m_currentJSHeapIndex)) {
            m_data[m_currentJSHeapIndex].update(event.number<qint64>(0));
            m_currentSize = m_data[m_currentJSHeapIndex].size;
        } else {
            MemoryAllocationItem allocation(event.typeIndex(), m_currentSize,
                    m_rangeStack.empty() ? -1 : m_rangeStack.top().originTypeIndex);
            allocation.update(event.number<qint64>(0));
            m_currentSize = allocation.size;

            if (m_currentSize > m_maxSize)
                m_maxSize = m_currentSize;
            if (m_currentJSHeapIndex != -1)
                insertEnd(m_currentJSHeapIndex,
                          event.timestamp() - startTime(m_currentJSHeapIndex) - 1);
            m_currentJSHeapIndex = insertStart(event.timestamp(), type.detailType);
            m_data.insert(m_currentJSHeapIndex, allocation);
        }
    }
}

void MemoryUsageModel::finalize()
{
    if (m_currentJSHeapIndex != -1)
        insertEnd(m_currentJSHeapIndex, modelManager()->traceTime()->endTime() -
                  startTime(m_currentJSHeapIndex) - 1);
    if (m_currentUsageIndex != -1)
        insertEnd(m_currentUsageIndex, modelManager()->traceTime()->endTime() -
                  startTime(m_currentUsageIndex) - 1);


    computeNesting();
    setExpandedRowCount(3);
    setCollapsedRowCount(3);
}

void MemoryUsageModel::clear()
{
    m_data.clear();
    m_maxSize = 1;
    m_currentSize = 0;
    m_currentUsage = 0;
    m_currentUsageIndex = -1;
    m_currentJSHeapIndex = -1;
    m_rangeStack.clear();
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
