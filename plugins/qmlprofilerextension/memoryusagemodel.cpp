/****************************************************************************
**
** Copyright (C) 2013 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "memoryusagemodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/abstracttimelinemodel_p.h"

#include <QStack>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

class MemoryUsageModel::MemoryUsageModelPrivate : public AbstractTimelineModelPrivate
{
public:
    static QString memoryTypeName(int type);

    QVector<MemoryAllocation> data;
    qint64 maxSize;
private:
    Q_DECLARE_PUBLIC(MemoryUsageModel)
};

MemoryUsageModel::MemoryUsageModel(QObject *parent)
    : AbstractTimelineModel(new MemoryUsageModelPrivate(),
                            tr(QmlProfilerModelManager::featureName(QmlDebug::ProfileMemory)),
                            QmlDebug::MemoryAllocation, QmlDebug::MaximumRangeType, parent)
{
}

quint64 MemoryUsageModel::features() const
{
    // Will listen to all range events, too, to determine context.
    return (1 << QmlDebug::ProfileMemory) | QmlDebug::Constants::QML_JS_RANGE_FEATURES;
}

int MemoryUsageModel::rowMaxValue(int rowNumber) const
{
    Q_D(const MemoryUsageModel);
    Q_UNUSED(rowNumber);
    return d->maxSize;
}

int MemoryUsageModel::row(int index) const
{
    Q_D(const MemoryUsageModel);
    QmlDebug::MemoryType type = d->data[index].type;
    if (type == QmlDebug::HeapPage || type == QmlDebug::LargeItem)
        return 1;
    else
        return 2;
}

int MemoryUsageModel::eventId(int index) const
{
    Q_D(const MemoryUsageModel);
    return d->data[index].type;
}

QColor MemoryUsageModel::color(int index) const
{
    return colorByEventId(index);
}

float MemoryUsageModel::relativeHeight(int index) const
{
    Q_D(const MemoryUsageModel);
    return qMin(1.0f, (float)d->data[index].size / (float)d->maxSize);
}

QVariantMap MemoryUsageModel::location(int index) const
{
    static const QLatin1String file("file");
    static const QLatin1String line("line");
    static const QLatin1String column("column");

    Q_D(const MemoryUsageModel);
    QVariantMap result;

    int originType = d->data[index].originTypeIndex;
    if (originType > -1) {
        const QmlDebug::QmlEventLocation &location =
                d->modelManager->qmlModel()->getEventTypes().at(originType).location;

        result.insert(file, location.filename);
        result.insert(line, location.line);
        result.insert(column, location.column);
    }

    return result;
}

QVariantList MemoryUsageModel::labels() const
{
    Q_D(const MemoryUsageModel);
    QVariantList result;

    if (d->expanded && !isEmpty()) {
        {
            QVariantMap element;
            element.insert(QLatin1String("description"), QVariant(tr("Memory Allocation")));

            element.insert(QLatin1String("id"), QVariant(QmlDebug::HeapPage));
            result << element;
        }

        {
            QVariantMap element;
            element.insert(QLatin1String("description"), QVariant(tr("Memory Usage")));

            element.insert(QLatin1String("id"), QVariant(QmlDebug::SmallItem));
            result << element;
        }
    }

    return result;
}

QVariantMap MemoryUsageModel::details(int index) const
{
    Q_D(const MemoryUsageModel);

    QVariantMap result;
    const MemoryAllocation *ev = &d->data[index];

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
    result.insert(tr("Type"), QVariant(MemoryUsageModelPrivate::memoryTypeName(ev->type)));

    if (ev->originTypeIndex != -1) {
        result.insert(tr("Location"),
                d->modelManager->qmlModel()->getEventTypes().at(ev->originTypeIndex).displayName);
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
    Q_D(MemoryUsageModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    qint64 currentSize = 0;
    qint64 currentUsage = 0;
    int currentUsageIndex = -1;
    int currentJSHeapIndex = -1;

    QStack<RangeStackFrame> rangeStack;
    MemoryAllocation dummy;

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex];
        while (!rangeStack.empty() && rangeStack.top().endTime < event.startTime)
            rangeStack.pop();
        if (!accepted(type)) {
            if (type.rangeType != QmlDebug::MaximumRangeType) {
                rangeStack.push(RangeStackFrame(event.typeIndex, event.startTime,
                                                event.startTime + event.duration));
            }
            continue;
        }

        if (type.detailType == QmlDebug::SmallItem || type.detailType == QmlDebug::LargeItem) {
            MemoryAllocation &last = currentUsageIndex > -1 ? d->data[currentUsageIndex] : dummy;
            if (!rangeStack.empty() && type.detailType == last.type &&
                    last.originTypeIndex == rangeStack.top().originTypeIndex &&
                    rangeStack.top().startTime < range(currentUsageIndex).start) {
                last.update(event.numericData1);
                currentUsage = last.size;
            } else {
                MemoryAllocation allocation(QmlDebug::SmallItem, currentUsage,
                        rangeStack.empty() ? -1 : rangeStack.top().originTypeIndex);
                allocation.update(event.numericData1);
                currentUsage = allocation.size;

                if (currentUsageIndex != -1) {
                    insertEnd(currentUsageIndex,
                              event.startTime - range(currentUsageIndex).start - 1);
                }
                currentUsageIndex = insertStart(event.startTime);
                d->data.insert(currentUsageIndex, allocation);
            }
        }

        if (type.detailType == QmlDebug::HeapPage || type.detailType == QmlDebug::LargeItem) {
            MemoryAllocation &last = currentJSHeapIndex > -1 ? d->data[currentJSHeapIndex] : dummy;
            if (!rangeStack.empty() && type.detailType == last.type &&
                    last.originTypeIndex == rangeStack.top().originTypeIndex &&
                    rangeStack.top().startTime < range(currentJSHeapIndex).start) {
                last.update(event.numericData1);
                currentSize = last.size;
            } else {
                MemoryAllocation allocation((QmlDebug::MemoryType)type.detailType, currentSize,
                        rangeStack.empty() ? -1 : rangeStack.top().originTypeIndex);
                allocation.update(event.numericData1);
                currentSize = allocation.size;

                if (currentSize > d->maxSize)
                    d->maxSize = currentSize;
                if (currentJSHeapIndex != -1)
                    insertEnd(currentJSHeapIndex,
                              event.startTime - range(currentJSHeapIndex).start - 1);
                currentJSHeapIndex = insertStart(event.startTime);
                d->data.insert(currentJSHeapIndex, allocation);
            }
        }

        d->modelManager->modelProxyCountUpdated(d->modelId, count(),
                                                simpleModel->getEvents().count());
    }

    if (currentJSHeapIndex != -1)
        insertEnd(currentJSHeapIndex, traceEndTime() - range(currentJSHeapIndex).start - 1);
    if (currentUsageIndex != -1)
        insertEnd(currentUsageIndex, traceEndTime() - range(currentUsageIndex).start - 1);


    computeNesting();
    d->expandedRowCount = d->collapsedRowCount = 3;
    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void MemoryUsageModel::clear()
{
    Q_D(MemoryUsageModel);
    d->data.clear();
    d->maxSize = 1;
    AbstractTimelineModel::clear();
}

QString MemoryUsageModel::MemoryUsageModelPrivate::memoryTypeName(int type)
{
    switch (type) {
    case QmlDebug::HeapPage: return tr("Heap Allocation");
    case QmlDebug::LargeItem: return tr("Large Item Allocation");
    case QmlDebug::SmallItem: return tr("Heap Usage");
    case QmlDebug::MaximumMemoryType: return tr("Total");
    default: return tr("Unknown");
    }
}

MemoryUsageModel::MemoryAllocation::MemoryAllocation(QmlDebug::MemoryType type, qint64 baseAmount,
                                                     int originTypeIndex) :
    type(type), size(baseAmount), allocated(0), deallocated(0), allocations(0), deallocations(0),
    originTypeIndex(originTypeIndex)
{
}

void MemoryUsageModel::MemoryAllocation::update(qint64 amount)
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
} // namespace QmlProfilerExtension
