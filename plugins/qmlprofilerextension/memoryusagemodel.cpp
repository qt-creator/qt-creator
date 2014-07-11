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
#include "qmlprofiler/sortedtimelinemodel.h"
#include "qmlprofiler/abstracttimelinemodel_p.h"

#include <QStack>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

class MemoryUsageModel::MemoryUsageModelPrivate :
        public SortedTimelineModel<MemoryAllocation,
                                   AbstractTimelineModel::AbstractTimelineModelPrivate>
{
public:
    static QString memoryTypeName(int type);

    qint64 maxSize;
private:
    Q_DECLARE_PUBLIC(MemoryUsageModel)
};

MemoryUsageModel::MemoryUsageModel(QObject *parent)
    : AbstractTimelineModel(new MemoryUsageModelPrivate(),
                                  QLatin1String("MemoryUsageTimelineModel"),
                                  QLatin1String("Memory Usage"), QmlDebug::MemoryAllocation,
                                  QmlDebug::MaximumRangeType, parent)
{
}

int MemoryUsageModel::rowCount() const
{
    return isEmpty() ? 1 : 3;
}

int MemoryUsageModel::rowMaxValue(int rowNumber) const
{
    Q_D(const MemoryUsageModel);
    Q_UNUSED(rowNumber);
    return d->maxSize;
}

int MemoryUsageModel::getEventRow(int index) const
{
    Q_D(const MemoryUsageModel);
    QmlDebug::MemoryType type = d->range(index).type;
    if (type == QmlDebug::HeapPage || type == QmlDebug::LargeItem)
        return 1;
    else
        return 2;
}

int MemoryUsageModel::getEventId(int index) const
{
    Q_D(const MemoryUsageModel);
    return d->range(index).type;
}

QColor MemoryUsageModel::getColor(int index) const
{
    return getEventColor(index);
}

float MemoryUsageModel::getHeight(int index) const
{
    Q_D(const MemoryUsageModel);
    return qMin(1.0f, (float)d->range(index).size / (float)d->maxSize);
}

const QVariantMap MemoryUsageModel::getEventLocation(int index) const
{
    static const QLatin1String file("file");
    static const QLatin1String line("line");
    static const QLatin1String column("column");

    Q_D(const MemoryUsageModel);
    QVariantMap result;

    int originType = d->range(index).originTypeIndex;
    if (originType > -1) {
        const QmlDebug::QmlEventLocation &location =
                d->modelManager->qmlModel()->getEventTypes().at(originType).location;

        result.insert(file, location.filename);
        result.insert(line, location.line);
        result.insert(column, location.column);
    }

    return result;
}

const QVariantList MemoryUsageModel::getLabels() const
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

const QVariantList MemoryUsageModel::getEventDetails(int index) const
{
    Q_D(const MemoryUsageModel);
    static QString title = QStringLiteral("title");

    QVariantList result;
    const MemoryUsageModelPrivate::Range *ev = &d->range(index);

    QVariantMap res;
    if (ev->delta > 0)
        res.insert(title, tr("Memory Allocated"));
    else
        res.insert(title, tr("Memory Freed"));
    result << res;
    res.clear();

    res.insert(tr("Total"), QVariant(QString::fromLatin1("%1 bytes").arg(ev->size)));
    result << res;
    res.clear();

    res.insert(tr("Allocation"), QVariant(QString::fromLatin1("%1 bytes").arg(ev->delta)));
    res.insert(tr("Type"), QVariant(MemoryUsageModelPrivate::memoryTypeName(ev->type)));
    if (ev->originTypeIndex != -1) {
        res.insert(tr("Location"),
                d->modelManager->qmlModel()->getEventTypes().at(ev->originTypeIndex).displayName);
    }
    result << res;
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
    MemoryAllocation dummy = {
        QmlDebug::MaximumMemoryType, -1, -1 , -1
    };

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex];
        while (!rangeStack.empty() && rangeStack.top().endTime < event.startTime)
            rangeStack.pop();
        if (!eventAccepted(type)) {
            if (type.rangeType != QmlDebug::MaximumRangeType) {
                rangeStack.push(RangeStackFrame(event.typeIndex, event.startTime,
                                                event.startTime + event.duration));
            }
            continue;
        }

        if (type.detailType == QmlDebug::SmallItem || type.detailType == QmlDebug::LargeItem) {
            currentUsage += event.numericData1;
            MemoryAllocation &last = currentUsageIndex > -1 ? d->data(currentUsageIndex) : dummy;
            if (!rangeStack.empty() && last.originTypeIndex == rangeStack.top().originTypeIndex) {
                last.size = currentUsage;
                last.delta += event.numericData1;
            } else {
                MemoryAllocation allocation = {
                    QmlDebug::SmallItem,
                    currentUsage,
                    event.numericData1,
                    rangeStack.empty() ? -1 : rangeStack.top().originTypeIndex
                };
                if (currentUsageIndex != -1) {
                    d->insertEnd(currentUsageIndex,
                                 event.startTime - d->range(currentUsageIndex).start - 1);
                }
                currentUsageIndex = d->insertStart(event.startTime, allocation);
            }
        }

        if (type.detailType == QmlDebug::HeapPage || type.detailType == QmlDebug::LargeItem) {
            currentSize += event.numericData1;
            MemoryAllocation &last = currentJSHeapIndex > -1 ? d->data(currentJSHeapIndex) : dummy;
            if (!rangeStack.empty() && last.originTypeIndex == rangeStack.top().originTypeIndex) {
                last.size = currentSize;
                last.delta += event.numericData1;
            } else {
                MemoryAllocation allocation = {
                    (QmlDebug::MemoryType)type.detailType,
                    currentSize,
                    event.numericData1,
                    rangeStack.empty() ? -1 : rangeStack.top().originTypeIndex
                };

                if (currentSize > d->maxSize)
                    d->maxSize = currentSize;
                if (currentJSHeapIndex != -1)
                    d->insertEnd(currentJSHeapIndex,
                                 event.startTime - d->range(currentJSHeapIndex).start - 1);
                currentJSHeapIndex = d->insertStart(event.startTime, allocation);
            }
        }

        d->modelManager->modelProxyCountUpdated(d->modelId, d->count(), simpleModel->getEvents().count());
    }

    if (currentJSHeapIndex != -1)
        d->insertEnd(currentJSHeapIndex, traceEndTime() -
                     d->range(currentJSHeapIndex).start - 1);
    if (currentUsageIndex != -1)
        d->insertEnd(currentUsageIndex, traceEndTime() -
                     d->range(currentUsageIndex).start - 1);


    d->computeNesting();
    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void MemoryUsageModel::clear()
{
    Q_D(MemoryUsageModel);
    d->SortedTimelineModel::clear();
    d->expanded = false;
    d->maxSize = 1;

    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
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


} // namespace Internal
} // namespace QmlProfilerExtension
