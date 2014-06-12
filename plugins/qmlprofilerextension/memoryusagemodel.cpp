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

#include <QDebug>

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
    return qMin(1.0f, (float)d->range(index).size / (float)d->maxSize * 0.85f + 0.15f);
}

const QVariantList MemoryUsageModel::getLabels() const
{
    Q_D(const MemoryUsageModel);
    QVariantList result;

    if (d->expanded && !isEmpty()) {
        {
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(tr("Memory Allocation")));
            element.insert(QLatin1String("description"), QVariant(tr("Memory Allocation")));

            element.insert(QLatin1String("id"), QVariant(QmlDebug::HeapPage));
            result << element;
        }

        {
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(tr("Memory Usage")));
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
    QVariantList result;
    const MemoryUsageModelPrivate::Range *ev = &d->range(index);

    {
        QVariantMap res;
        if (ev->size > 0)
            res.insert(QLatin1String("title"), QVariant(QLatin1String("Memory Allocated")));
        else
            res.insert(QLatin1String("title"), QVariant(QLatin1String("Memory Freed")));

        result << res;
    }

    {
        QVariantMap res;
        res.insert(tr("Total"), QVariant(QString::fromLatin1("%1 bytes").arg(ev->size)));
        result << res;
    }

    {
        QVariantMap res;
        res.insert(tr("Allocation"), QVariant(QString::fromLatin1("%1 bytes").arg(ev->delta)));
        result << res;
    }


    {
        QVariantMap res;
        res.insert(tr("Type"), QVariant(MemoryUsageModelPrivate::memoryTypeName(ev->type)));
        result << res;
    }

    return result;
}

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
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        if (!eventAccepted(event))
            continue;

        if (event.detailType == QmlDebug::SmallItem || event.detailType == QmlDebug::LargeItem) {
            currentUsage += event.numericData1;
            MemoryAllocation allocation = {
                QmlDebug::SmallItem,
                currentUsage,
                event.numericData1
            };
            if (currentUsageIndex != -1) {
                d->insertEnd(currentUsageIndex,
                             event.startTime - d->range(currentUsageIndex).start - 1);
            }
            currentUsageIndex = d->insertStart(event.startTime, allocation);
        }

        if (event.detailType == QmlDebug::HeapPage || event.detailType == QmlDebug::LargeItem) {
            currentSize += event.numericData1;
            MemoryAllocation allocation = {
                (QmlDebug::MemoryType)event.detailType,
                currentSize,
                event.numericData1
            };

            if (currentSize > d->maxSize)
                d->maxSize = currentSize;
            if (currentJSHeapIndex != -1)
                d->insertEnd(currentJSHeapIndex,
                             event.startTime - d->range(currentJSHeapIndex).start - 1);
            currentJSHeapIndex = d->insertStart(event.startTime, allocation);
        }

        d->modelManager->modelProxyCountUpdated(d->modelId, d->count(), simpleModel->getEvents().count());
    }

    if (currentJSHeapIndex != -1)
        d->insertEnd(currentJSHeapIndex, simpleModel->lastTimeMark() -
                     d->range(currentJSHeapIndex).start - 1);
    if (currentUsageIndex != -1)
        d->insertEnd(currentUsageIndex, simpleModel->lastTimeMark() -
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
