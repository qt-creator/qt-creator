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

#include <utils/qtcassert.h>

namespace QmlProfiler {
namespace Internal {

MemoryUsageModel::MemoryUsageModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, MemoryAllocation, MaximumRangeType, ProfileMemory, parent)
{
    // Announce additional features. The base class already announces the main feature.
    announceFeatures(Constants::QML_JS_RANGE_FEATURES ^ (1 << ProfileCompiling));
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

QRgb MemoryUsageModel::color(int index) const
{
    return colorBySelectionId(index);
}

float MemoryUsageModel::relativeHeight(int index) const
{
    return qMin(1.0f, (float)m_data[index].size / (float)m_maxSize);
}

QVariantMap MemoryUsageModel::location(int index) const
{
    return locationFromTypeId(index);
}

QVariantList MemoryUsageModel::labels() const
{
    QVariantList result;

    QVariantMap element;
    element.insert(QLatin1String("description"), tr("Memory Allocation"));
    element.insert(QLatin1String("id"), HeapPage);
    result << element;

    element.clear();
    element.insert(QLatin1String("description"), tr("Memory Usage"));
    element.insert(QLatin1String("id"), SmallItem);
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

    result.insert(tr("Total"), tr("%n bytes", 0, ev->size));
    if (ev->allocations > 0) {
        result.insert(tr("Allocated"), tr("%1 bytes").arg(ev->allocated));
        result.insert(tr("Allocations"), QString::number(ev->allocations));
    }
    if (ev->deallocations > 0) {
        result.insert(tr("Deallocated"), tr("%1 bytes").arg(-ev->deallocated));
        result.insert(tr("Deallocations"), QString::number(ev->deallocations));
    }
    QString memoryTypeName;
    switch (selectionId(index)) {
    case HeapPage:  memoryTypeName = tr("Heap Allocation"); break;
    case LargeItem: memoryTypeName = tr("Large Item Allocation"); break;
    case SmallItem: memoryTypeName = tr("Heap Usage"); break;
    default: Q_UNREACHABLE();
    }
    result.insert(tr("Type"), memoryTypeName);

    result.insert(tr("Location"),
                  modelManager()->qmlModel()->eventTypes().at(ev->typeId).displayName());
    return result;
}

bool MemoryUsageModel::accepted(const QmlEventType &type) const
{
    return QmlProfilerTimelineModel::accepted(type)
            || (type.rangeType() != MaximumRangeType && type.rangeType() != Compiling);
}

void MemoryUsageModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    if (type.message() != MemoryAllocation) {
        if (type.rangeType() != MaximumRangeType) {
            if (event.rangeStage() == RangeStart)
                m_rangeStack.push(RangeStackFrame(event.typeIndex(), event.timestamp()));
            else if (event.rangeStage() == RangeEnd)
                m_rangeStack.pop();

            m_continuation = ContinueNothing;
        }
        return;
    }

    auto canContinue = [&](EventContinuation continuation) {
        QTC_ASSERT(continuation != ContinueNothing, return false);
        if ((m_continuation & continuation) == 0)
            return false;

        int currentIndex = (continuation == ContinueAllocation ? m_currentJSHeapIndex :
                                                                 m_currentUsageIndex);

        if (m_rangeStack.isEmpty()) {
            qint64 amount = event.number<qint64>(0);
            // outside of ranges show monotonous allocation or deallocation
            return (amount >= 0 && m_data[currentIndex].allocated >= 0)
                    || (amount < 0 && m_data[currentIndex].deallocated > 0);
        } else {
            return m_data[currentIndex].typeId == m_rangeStack.top().originTypeIndex
                    && m_rangeStack.top().startTime < startTime(currentIndex);
        }
    };

    if (type.detailType() == SmallItem || type.detailType() == LargeItem) {
        if (canContinue(ContinueUsage)) {
            m_data[m_currentUsageIndex].update(event.number<qint64>(0));
            m_currentUsage = m_data[m_currentUsageIndex].size;
        } else {
            MemoryAllocationItem allocation(
                        m_rangeStack.empty() ? event.typeIndex() :
                                               m_rangeStack.top().originTypeIndex,
                        m_currentUsage);
            allocation.update(event.number<qint64>(0));
            m_currentUsage = allocation.size;

            if (m_currentUsageIndex != -1) {
                qint64 duration = event.timestamp() - startTime(m_currentUsageIndex);
                if (duration > 0) {
                    insertEnd(m_currentUsageIndex, duration - 1);
                    m_currentUsageIndex = insertStart(event.timestamp(), SmallItem);
                    m_data.insert(m_currentUsageIndex, allocation);
                } else {
                    // Ignore ranges of 0 duration. We only need to keep track of the sizes.
                    m_data[m_currentUsageIndex] = allocation;
                }
            } else {
                m_currentUsageIndex = insertStart(event.timestamp(), SmallItem);
                m_data.insert(m_currentUsageIndex, allocation);
            }
            m_continuation = m_continuation | ContinueUsage;
        }
    }

    if (type.detailType() == HeapPage || type.detailType() == LargeItem) {
        if (canContinue(ContinueAllocation)
                && type.detailType() == selectionId(m_currentJSHeapIndex)) {
            m_data[m_currentJSHeapIndex].update(event.number<qint64>(0));
            m_currentSize = m_data[m_currentJSHeapIndex].size;
        } else {
            MemoryAllocationItem allocation(
                        m_rangeStack.empty() ? event.typeIndex() :
                                               m_rangeStack.top().originTypeIndex,
                        m_currentSize);
            allocation.update(event.number<qint64>(0));
            m_currentSize = allocation.size;

            if (m_currentSize > m_maxSize)
                m_maxSize = m_currentSize;

            if (m_currentJSHeapIndex != -1) {
                qint64 duration = event.timestamp() - startTime(m_currentJSHeapIndex);
                if (duration > 0){
                    insertEnd(m_currentJSHeapIndex, duration - 1);
                    m_currentJSHeapIndex = insertStart(event.timestamp(), type.detailType());
                    m_data.insert(m_currentJSHeapIndex, allocation);
                } else {
                    // Ignore ranges of 0 duration. We only need to keep track of the sizes.
                    m_data[m_currentJSHeapIndex] = allocation;
                }
            } else {
                m_currentJSHeapIndex = insertStart(event.timestamp(), type.detailType());
                m_data.insert(m_currentJSHeapIndex, allocation);
            }

            m_continuation = m_continuation | ContinueAllocation;
        }
    }
}

void MemoryUsageModel::finalize()
{
    if (m_currentJSHeapIndex != -1)
        insertEnd(m_currentJSHeapIndex, modelManager()->traceTime()->endTime() -
                  startTime(m_currentJSHeapIndex));
    if (m_currentUsageIndex != -1)
        insertEnd(m_currentUsageIndex, modelManager()->traceTime()->endTime() -
                  startTime(m_currentUsageIndex));


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
    m_continuation = ContinueNothing;
    m_rangeStack.clear();
    QmlProfilerTimelineModel::clear();
}

bool MemoryUsageModel::handlesTypeId(int typeId) const
{
    Q_UNUSED(typeId);
    // We don't want the memory ranges allocated by some QML/JS function to be highlighted when
    // propagating a typeId selection to the timeline. The actual range should be highlighted.
    return false;
}

MemoryUsageModel::MemoryAllocationItem::MemoryAllocationItem(int typeId, qint64 baseAmount) :
    size(baseAmount), allocated(0), deallocated(0), allocations(0), deallocations(0),
    typeId(typeId)
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
