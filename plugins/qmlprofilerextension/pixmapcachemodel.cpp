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

#include "pixmapcachemodel.h"
#include "qmldebug/qmlprofilereventtypes.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/sortedtimelinemodel.h"
#include "qmlprofiler/singlecategorytimelinemodel_p.h"

#include <QDebug>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

class PixmapCacheModel::PixmapCacheModelPrivate :
        public SortedTimelineModel<PixmapCacheEvent,
                                   SingleCategoryTimelineModel::SingleCategoryTimelineModelPrivate>
{
public:
    void computeCacheSizes();
    void resizeUnfinishedLoads();
    void flattenLoads();
    void computeRowCounts();

    QVector < QString > pixmapUrls;
    QVector < QPair<int, int> > pixmapSizes;
    int expandedRowCount;
    int collapsedRowCount;
    void addVP(QVariantList &l, QString label, qint64 time) const;

    qint64 minCacheSize;
    qint64 maxCacheSize;
private:
    Q_DECLARE_PUBLIC(PixmapCacheModel)
};

PixmapCacheModel::PixmapCacheModel(QObject *parent)
    : SingleCategoryTimelineModel(new PixmapCacheModelPrivate(),
                                  QLatin1String("PixmapCacheTimeLineModel"),
                                  QLatin1String("Pixmap Cache"), QmlDebug::PixmapCacheEvent, parent)
{
    Q_D(PixmapCacheModel);
    d->collapsedRowCount = 1;
    d->expandedRowCount = 1;
}

int PixmapCacheModel::categoryDepth(int categoryIndex) const
{
    Q_D(const PixmapCacheModel);
    Q_UNUSED(categoryIndex);
    if (isEmpty())
        return 1;
    if (d->expanded)
        return d->expandedRowCount;
    return d->collapsedRowCount;
}

int PixmapCacheModel::getEventRow(int index) const
{
    Q_D(const PixmapCacheModel);
    if (d->expanded)
        return d->range(index).rowNumberExpanded;
    return d->range(index).rowNumberCollapsed;
}

int PixmapCacheModel::getEventId(int index) const
{
    Q_D(const PixmapCacheModel);
    return d->range(index).eventId;
}

QColor PixmapCacheModel::getColor(int index) const
{
    Q_D(const PixmapCacheModel);
    if (d->range(index).pixmapEventType == PixmapCacheCountChanged)
        return getColorByHue(PixmapCacheCountHue);

    return getEventColor(index);
}

float PixmapCacheModel::getHeight(int index) const
{
    Q_D(const PixmapCacheModel);
    if (d->range(index).pixmapEventType == PixmapCacheCountChanged) {
        float scale = d->maxCacheSize - d->minCacheSize;
        float fraction = 1.0f;
        if (scale > 1)
            fraction = (float)(d->range(index).cacheSize -
                                d->minCacheSize) / scale;

        return fraction * 0.85f + 0.15f;
    }

    return 1.0f;
}

QString getFilenameOnly(QString absUrl)
{
    int characterPos = absUrl.lastIndexOf(QLatin1Char('/'))+1;
    if (characterPos < absUrl.length())
        absUrl = absUrl.mid(characterPos);
    return absUrl;
}

const QVariantList PixmapCacheModel::getLabelsForCategory(int category) const
{
    Q_D(const PixmapCacheModel);
    Q_UNUSED(category);
    QVariantList result;

    if (d->expanded && !isEmpty()) {
        {
            // Cache Size
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(QLatin1String("Cache Size")));
            element.insert(QLatin1String("description"), QVariant(QLatin1String("Cache Size")));

            element.insert(QLatin1String("id"), QVariant(0));
            result << element;
        }

        for (int i=0; i < d->pixmapUrls.count(); i++) {
            // Loading
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(getFilenameOnly(d->pixmapUrls[i])));
            element.insert(QLatin1String("description"), QVariant(getFilenameOnly(d->pixmapUrls[i])));

            element.insert(QLatin1String("id"), QVariant(i+1));
            result << element;
        }
    }

    return result;
}

void PixmapCacheModel::PixmapCacheModelPrivate::addVP(QVariantList &l, QString label, qint64 time) const
{
    if (time > 0) {
        QVariantMap res;
        res.insert(label, QVariant(QmlProfilerBaseModel::formatTime(time)));
        l << res;
    }
}

const QVariantList PixmapCacheModel::getEventDetails(int index) const
{
    Q_D(const PixmapCacheModel);
    QVariantList result;
    const PixmapCacheModelPrivate::Range *ev = &d->range(index);

    {
        QVariantMap res;
        if (ev->pixmapEventType == PixmapCacheCountChanged)
            res.insert(QLatin1String("title"), QVariant(QLatin1String("Image Cached")));
        else if (ev->pixmapEventType == PixmapLoadingStarted)
            res.insert(QLatin1String("title"), QVariant(QLatin1String("Image Loaded")));
        result << res;
    }

    if (ev->pixmapEventType != PixmapCacheCountChanged) {
        d->addVP(result, tr("Duration"), ev->duration );
    }

    {
        QVariantMap res;
        res.insert(tr("File"), QVariant(getFilenameOnly(d->pixmapUrls[ev->urlIndex])));
        result << res;
    }

    {
        QVariantMap res;
        res.insert(tr("Width"), QVariant(QString::fromLatin1("%1 px").arg(d->pixmapSizes[ev->urlIndex].first)));
        result << res;
        res.clear();
        res.insert(tr("Height"), QVariant(QString::fromLatin1("%1 px").arg(d->pixmapSizes[ev->urlIndex].second)));
        result << res;
    }

    if (ev->pixmapEventType == PixmapLoadingStarted && ev->cacheSize == -1) {
        QVariantMap res;
        res.insert(tr("Result"), QVariant(QLatin1String("Load Error")));
        result << res;
    }

    return result;
}

void PixmapCacheModel::loadData()
{
    Q_D(PixmapCacheModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    int lastCacheSizeEvent = -1;
    int cumulatedCount = 0;
    QVector < int > pixmapStartPoints;
    QVector < int > pixmapCachePoints;

    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        if (!eventAccepted(event))
            continue;

        PixmapCacheEvent newEvent;
        newEvent.pixmapEventType = event.bindingType;
        qint64 startTime = event.startTime;

        bool isNewEntry = false;
        newEvent.urlIndex = d->pixmapUrls.indexOf(event.location.filename);
        if (newEvent.urlIndex == -1) {
            isNewEntry = true;
            newEvent.urlIndex = d->pixmapUrls.count();
            d->pixmapUrls << event.location.filename;
            d->pixmapSizes << QPair<int, int>(0,0); // default value
            pixmapStartPoints << -1; // dummy value to be filled by load event
            pixmapCachePoints << -1; // dummy value to be filled by cache event
        }

        newEvent.eventId = newEvent.urlIndex + 1;
        newEvent.rowNumberExpanded = newEvent.urlIndex + 2;

        switch (newEvent.pixmapEventType) {
        case PixmapSizeKnown: // pixmap size
            d->pixmapSizes[newEvent.urlIndex] = QPair<int,int>((int)event.numericData1, (int)event.numericData2);
            if (pixmapCachePoints[newEvent.urlIndex] == -1)
                break;
            // else fall through and update cache size
            newEvent.pixmapEventType = PixmapCacheCountChanged;
        case PixmapCacheCountChanged: {// Cache Size Changed Event
            startTime = event.startTime + 1; // delay 1 ns for proper sorting
            newEvent.eventId = 0;
            newEvent.rowNumberExpanded = 1;
            newEvent.rowNumberCollapsed = 1;

            qint64 pixSize = d->pixmapSizes[newEvent.urlIndex].first * d->pixmapSizes[newEvent.urlIndex].second;
            qint64 prevSize = 0;
            if (lastCacheSizeEvent != -1) {
                prevSize = d->range(lastCacheSizeEvent).cacheSize;
                if (pixmapCachePoints[newEvent.urlIndex] == -1) {
                    // else it's a synthesized update and doesn't have a valid cache count
                    if (event.numericData3 < cumulatedCount)
                        pixSize = -pixSize;
                    cumulatedCount = event.numericData3;
                }
                d->insertEnd(lastCacheSizeEvent, startTime - d->range(lastCacheSizeEvent).start);
            }
            newEvent.cacheSize = prevSize + pixSize;
            lastCacheSizeEvent = d->insertStart(startTime, newEvent);
            pixmapCachePoints[newEvent.urlIndex] = lastCacheSizeEvent;
            break;
        }
        case PixmapLoadingStarted: // Load
            pixmapStartPoints[newEvent.urlIndex] = d->insertStart(startTime, newEvent);
            break;
        case PixmapLoadingFinished:
        case PixmapLoadingError: {
            int loadIndex = pixmapStartPoints[newEvent.urlIndex];
            if (!isNewEntry && loadIndex != -1) {
                d->insertEnd(loadIndex, startTime - d->range(loadIndex).start);
            } else {
                // if it's a new entry it means that we don't have a corresponding start
                newEvent.pixmapEventType = PixmapLoadingStarted;
                newEvent.rowNumberExpanded = newEvent.urlIndex + 2;
                loadIndex = d->insert(traceStartTime(), startTime - traceStartTime(), newEvent);
                pixmapStartPoints[newEvent.urlIndex] = loadIndex;
            }
            if (event.bindingType == PixmapLoadingFinished)
                d->data(loadIndex).cacheSize = 1;  // use count to mark success
            else
                d->data(loadIndex).cacheSize = -1; // ... or failure
            break;
        }
        default:
            break;
        }

        d->modelManager->modelProxyCountUpdated(d->modelId, d->count(), 2*simpleModel->getEvents().count());
    }

    if (lastCacheSizeEvent != -1) {
        d->insertEnd(lastCacheSizeEvent, traceEndTime() - d->range(lastCacheSizeEvent).start);
    }

    d->resizeUnfinishedLoads();

    d->computeCacheSizes();
    d->flattenLoads();
    d->computeRowCounts();
    d->computeNesting();

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void PixmapCacheModel::clear()
{
    Q_D(PixmapCacheModel);
    d->SortedTimelineModel::clear();
    d->pixmapUrls.clear();
    d->pixmapSizes.clear();
    d->collapsedRowCount = 1;
    d->expandedRowCount = 1;
    d->expanded = false;

    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
}

void PixmapCacheModel::PixmapCacheModelPrivate::computeCacheSizes()
{
    minCacheSize = -1;
    maxCacheSize = -1;
    foreach (const PixmapCacheModel::PixmapCacheEvent &event, ranges) {
        if (event.pixmapEventType == PixmapCacheModel::PixmapCacheCountChanged) {
            if (minCacheSize == -1 || event.cacheSize < minCacheSize)
                minCacheSize = event.cacheSize;
            if (maxCacheSize == -1 || event.cacheSize > maxCacheSize)
                maxCacheSize = event.cacheSize;
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::resizeUnfinishedLoads()
{
    Q_Q(PixmapCacheModel);
    // all the "load start" events with duration 0 continue till the end of the trace
    for (int i = 0; i < count(); i++) {
        if (range(i).pixmapEventType == PixmapCacheModel::PixmapLoadingStarted &&
                range(i).duration == 0) {
            insertEnd(i, q->traceEndTime() - range(i).start);
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::flattenLoads()
{
    // computes "compressed row"
    QVector <qint64> eventEndTimes;
    for (int i = 0; i < count(); i++) {
        PixmapCacheModel::PixmapCacheEvent &event = data(i);
        const Range &start = range(i);
        if (event.pixmapEventType == PixmapCacheModel::PixmapLoadingStarted) {
            event.rowNumberCollapsed = 0;
            while (eventEndTimes.count() > event.rowNumberCollapsed &&
                   eventEndTimes[event.rowNumberCollapsed] > start.start)
                event.rowNumberCollapsed++;

            if (eventEndTimes.count() == event.rowNumberCollapsed)
                eventEndTimes << 0; // increase stack length, proper value added below
            eventEndTimes[event.rowNumberCollapsed] = start.start + start.duration;

            // readjust to account for category empty row and bargraph
            event.rowNumberCollapsed += 2;
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::computeRowCounts()
{
    expandedRowCount = 0;
    collapsedRowCount = 0;
    foreach (const PixmapCacheModel::PixmapCacheEvent &event, ranges) {
        if (event.rowNumberExpanded > expandedRowCount)
            expandedRowCount = event.rowNumberExpanded;
        if (event.rowNumberCollapsed > collapsedRowCount)
            collapsedRowCount = event.rowNumberCollapsed;
    }

    // Starting from 0, count is maxIndex+1
    expandedRowCount++;
    collapsedRowCount++;
}



} // namespace Internal
} // namespace QmlProfilerExtension
