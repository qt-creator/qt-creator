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
#include <QSize>

namespace QmlProfilerExtension {
namespace Internal {

using namespace QmlProfiler;

enum CacheState {
    Uncached,    // After loading started (or some other proof of existence) or after uncaching
    ToBeCached,  // After determining the pixmap is to be cached but before knowing its size
    Cached,      // After caching a pixmap or determining the size of a ToBeCached pixmap
    Uncacheable, // If loading failed without ToBeCached or after a corrupt pixmap has been uncached
    Corrupt      // If after ToBeCached we learn that loading failed
};

enum LoadState {
    Initial,
    Loading,
    Finished,
    Error
};

struct PixmapState {
    PixmapState(int width, int height, CacheState cache = Uncached) :
        size(width, height), started(-1), loadState(Initial), cacheState(cache) {}
    PixmapState(CacheState cache = Uncached) : started(-1), loadState(Initial), cacheState(cache) {}
    QSize size;
    int started;
    LoadState loadState;
    CacheState cacheState;
};

struct Pixmap {
    Pixmap() {}
    Pixmap(const QString &url) : url(url), sizes(1) {}
    QString url;
    QVector<PixmapState> sizes;
};

class PixmapCacheModel::PixmapCacheModelPrivate :
        public SortedTimelineModel<PixmapCacheEvent,
                                   SingleCategoryTimelineModel::SingleCategoryTimelineModelPrivate>
{
public:
    void computeMaxCacheSize();
    void resizeUnfinishedLoads();
    void flattenLoads();
    void computeRowCounts();
    int updateCacheCount(int lastCacheSizeEvent, qint64 startTime, qint64 pixSize,
                         PixmapCacheEvent &newEvent);

    QVector<Pixmap> pixmaps;
    int expandedRowCount;
    int collapsedRowCount;
    void addVP(QVariantList &l, QString label, qint64 time) const;

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
    d->maxCacheSize = 1;
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
    if (d->range(index).pixmapEventType == PixmapCacheCountChanged)
        return 0.15 + (float)d->range(index).cacheSize * 0.85 / (float)d->maxCacheSize;
    else
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

        for (int i=0; i < d->pixmaps.count(); i++) {
            // Loading
            QVariantMap element;
            element.insert(QLatin1String("displayName"),
                           QVariant(getFilenameOnly(d->pixmaps[i].url)));
            element.insert(QLatin1String("description"),
                           QVariant(getFilenameOnly(d->pixmaps[i].url)));

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
        res.insert(tr("File"), QVariant(getFilenameOnly(d->pixmaps[ev->urlIndex].url)));
        result << res;
    }

    {
        QVariantMap res;
        res.insert(tr("Width"), QVariant(QString::fromLatin1("%1 px")
                .arg(d->pixmaps[ev->urlIndex].sizes[ev->sizeIndex].size.width())));
        result << res;
        res.clear();
        res.insert(tr("Height"), QVariant(QString::fromLatin1("%1 px")
                .arg(d->pixmaps[ev->urlIndex].sizes[ev->sizeIndex].size.height())));
        result << res;
    }

    if (ev->pixmapEventType == PixmapLoadingStarted &&
            d->pixmaps[ev->urlIndex].sizes[ev->sizeIndex].loadState != Finished) {
        QVariantMap res;
        res.insert(tr("Result"), QVariant(QLatin1String("Load Error")));
        result << res;
    }

    return result;
}

/* Ultimately there is no way to know which cache entry a given event refers to as long as we only
 * receive the pixmap URL from the application. Multiple copies of different sizes may be cached
 * for each URL. However, we can apply some heuristics to make the result somewhat plausible by
 * using the following assumptions:
 *
 * - PixmapSizeKnown will happen at most once for every cache entry.
 * - PixmapSizeKnown cannot happen for entries with PixmapLoadingError and vice versa.
 * - PixmapCacheCountChanged can happen for entries with PixmapLoadingError but doesn't have to.
 * - Decreasing PixmapCacheCountChanged events can only happen for entries that have seen an
 *   increasing PixmapCacheCountChanged (but that may have happened before the trace).
 * - PixmapCacheCountChanged can happen before or after PixmapSizeKnown.
 * - For every PixmapLoadingFinished or PixmapLoadingError there is exactly one
 *   PixmapLoadingStarted event, but it may be before the trace.
 * - For every PixmapLoadingStarted there is exactly one PixmapLoadingFinished or
 *   PixmapLoadingError, but it may be after the trace.
 * - Decreasing PixmapCacheCountChanged events in the presence of corrupt cache entries are more
 *   likely to clear those entries than other, correctly loaded ones.
 * - Increasing PixmapCacheCountChanged events are more likely to refer to correctly loaded entries
 *   than to ones with PixmapLoadingError.
 * - PixmapLoadingFinished and PixmapLoadingError are more likely to refer to cache entries that
 *   have seen a PixmapLoadingStarted than to ones that haven't.
 *
 * For each URL we keep an ordered list of pixmaps possibly being loaded and assign new events to
 * the first entry that "fits". If multiple sizes of the same pixmap are being loaded concurrently
 * we generally assume that the PixmapLoadingFinished and PixmapLoadingError events occur in the
 * order we learn about the existence of these sizes, subject to the above constraints. This is not
 * necessarily the order the pixmaps are really loaded but it's the best we can do with the given
 * information. If they're loaded sequentially the representation is correct.
 */

void PixmapCacheModel::loadData()
{
    Q_D(PixmapCacheModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    int lastCacheSizeEvent = -1;
    int cumulatedCount = 0;

    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        if (!eventAccepted(event))
            continue;

        PixmapCacheEvent newEvent;
        newEvent.pixmapEventType = event.bindingType;
        qint64 startTime = event.startTime;

        newEvent.urlIndex = -1;
        for (QVector<Pixmap>::const_iterator it(d->pixmaps.cend()); it != d->pixmaps.cbegin();) {
            if ((--it)->url == event.location.filename) {
                newEvent.urlIndex = it - d->pixmaps.cbegin();
                break;
            }
        }

        newEvent.sizeIndex = -1;
        if (newEvent.urlIndex == -1) {
            newEvent.urlIndex = d->pixmaps.count();
            d->pixmaps << Pixmap(event.location.filename);
        }

        newEvent.eventId = newEvent.urlIndex + 1;
        newEvent.rowNumberExpanded = newEvent.urlIndex + 2;

        Pixmap &pixmap = d->pixmaps[newEvent.urlIndex];
        switch (newEvent.pixmapEventType) {
        case PixmapSizeKnown: {// pixmap size
            // Look for pixmaps for which we don't know the size, yet and which have actually been
            // loaded.
            for (QVector<PixmapState>::iterator i(pixmap.sizes.begin());
                    i != pixmap.sizes.end(); ++i) {
                if (i->size.isValid() || i->cacheState == Uncacheable || i->cacheState == Corrupt)
                    continue;

                // We can't have cached it before we knew the size
                Q_ASSERT(i->cacheState != Cached);

                i->size.setWidth(event.numericData1);
                i->size.setHeight(event.numericData2);
                newEvent.sizeIndex = i - pixmap.sizes.begin();
                break;
            }

            if (newEvent.sizeIndex == -1) {
                newEvent.sizeIndex = pixmap.sizes.length();
                pixmap.sizes << PixmapState(event.numericData1, event.numericData2);
            }

            PixmapState &state = pixmap.sizes[newEvent.sizeIndex];
            if (state.cacheState == ToBeCached) {
                lastCacheSizeEvent = d->updateCacheCount(lastCacheSizeEvent, startTime,
                                              state.size.width() * state.size.height(), newEvent);
                state.cacheState = Cached;
            }
            break;
        }
        case PixmapCacheCountChanged: {// Cache Size Changed Event
            startTime = event.startTime + 1; // delay 1 ns for proper sorting

            bool uncache = cumulatedCount > event.numericData3;
            cumulatedCount = event.numericData3;
            qint64 pixSize = 0;

            // First try to find a preferred pixmap, which either is Corrupt and will be uncached
            // or is uncached and will be cached.
            for (QVector<PixmapState>::iterator i(pixmap.sizes.begin());
                    i != pixmap.sizes.end(); ++i) {
                if (uncache && i->cacheState == Corrupt) {
                    newEvent.sizeIndex = i - pixmap.sizes.begin();
                    i->cacheState = Uncacheable;
                    break;
                } else if (!uncache && i->cacheState == Uncached) {
                    newEvent.sizeIndex = i - pixmap.sizes.begin();
                    if (i->size.isValid()) {
                        pixSize = i->size.width() * i->size.height();
                        i->cacheState = Cached;
                    } else {
                        i->cacheState = ToBeCached;
                    }
                    break;
                }
            }

            // If none found, check for cached or ToBeCached pixmaps that shall be uncached or
            // Error pixmaps that become corrupt cache entries. We also accept Initial to be
            // uncached as we may have missed the matching PixmapCacheCountChanged that cached it.
            if (newEvent.sizeIndex == -1) {
                for (QVector<PixmapState>::iterator i(pixmap.sizes.begin());
                        i != pixmap.sizes.end(); ++i) {
                    if (uncache && (i->cacheState == Cached || i->cacheState == ToBeCached ||
                                    i->cacheState == Uncached)) {
                        newEvent.sizeIndex = i - pixmap.sizes.begin();
                        if (i->size.isValid())
                            pixSize = -i->size.width() * i->size.height();
                        i->cacheState = Uncached;
                        break;
                    } else if (!uncache && i->cacheState == Uncacheable) {
                        newEvent.sizeIndex = i - pixmap.sizes.begin();
                        i->cacheState = Corrupt;
                        break;
                    }
                }
            }

            // If that does't work, create a new entry.
            if (newEvent.sizeIndex == -1) {
                newEvent.sizeIndex = pixmap.sizes.length();
                pixmap.sizes << PixmapState(uncache ? Uncached : ToBeCached);
            }

            lastCacheSizeEvent = d->updateCacheCount(lastCacheSizeEvent, startTime, pixSize,
                                                     newEvent);
            break;
        }
        case PixmapLoadingStarted: // Load
            // Look for a pixmap that hasn't been started, yet. There may have been a refcount
            // event, which we ignore.
            for (QVector<PixmapState>::const_iterator i(pixmap.sizes.cbegin());
                    i != pixmap.sizes.cend(); ++i) {
                if (i->loadState == Initial) {
                    newEvent.sizeIndex = i - pixmap.sizes.cbegin();
                    break;
                }
            }
            if (newEvent.sizeIndex == -1) {
                newEvent.sizeIndex = pixmap.sizes.length();
                pixmap.sizes << PixmapState();
            }
            pixmap.sizes[newEvent.sizeIndex].started = d->insertStart(startTime, newEvent);
            pixmap.sizes[newEvent.sizeIndex].loadState = Loading;
            break;
        case PixmapLoadingFinished:
        case PixmapLoadingError: {
            // First try to find one that has already started
            for (QVector<PixmapState>::const_iterator i(pixmap.sizes.cbegin());
                    i != pixmap.sizes.cend(); ++i) {
                if (i->loadState != Loading)
                    continue;
                // Pixmaps with known size cannot be errors and vice versa
                if (newEvent.pixmapEventType == PixmapLoadingError && i->size.isValid())
                    continue;

                newEvent.sizeIndex = i - pixmap.sizes.cbegin();
                break;
            }

            // If none was found use any other compatible one
            if (newEvent.sizeIndex == -1) {
                for (QVector<PixmapState>::const_iterator i(pixmap.sizes.cbegin());
                        i != pixmap.sizes.cend(); ++i) {
                    if (i->loadState != Initial)
                        continue;
                    // Pixmaps with known size cannot be errors and vice versa
                    if (newEvent.pixmapEventType == PixmapLoadingError && i->size.isValid())
                        continue;

                    newEvent.sizeIndex = i - pixmap.sizes.cbegin();
                    break;
                }
            }

            // If again none was found, create one.
            if (newEvent.sizeIndex == -1) {
                newEvent.sizeIndex = pixmap.sizes.length();
                pixmap.sizes << PixmapState();
            }

            PixmapState &state = pixmap.sizes[newEvent.sizeIndex];
            // If the pixmap loading wasn't started, start it at traceStartTime()
            if (state.loadState == Initial) {
                newEvent.pixmapEventType = PixmapLoadingStarted;
                state.started = d->insert(traceStartTime(), startTime - traceStartTime(), newEvent);
            }

            d->insertEnd(state.started, startTime - d->range(state.started).start);
            if (newEvent.pixmapEventType == PixmapLoadingError) {
                state.loadState = Error;
                switch (state.cacheState) {
                case Uncached:
                    state.cacheState = Uncacheable;
                    break;
                case ToBeCached:
                    state.cacheState = Corrupt;
                    break;
                default:
                    // Cached cannot happen as size would have to be known and Corrupt or
                    // Uncacheable cannot happen as we only accept one finish or error event per
                    // pixmap.
                    Q_ASSERT(false);
                }
            } else {
                state.loadState = Finished;
            }
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

    d->computeMaxCacheSize();
    d->flattenLoads();
    d->computeRowCounts();
    d->computeNesting();

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void PixmapCacheModel::clear()
{
    Q_D(PixmapCacheModel);
    d->SortedTimelineModel::clear();
    d->pixmaps.clear();
    d->collapsedRowCount = 1;
    d->expandedRowCount = 1;
    d->maxCacheSize = 1;
    d->expanded = false;

    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
}

void PixmapCacheModel::PixmapCacheModelPrivate::computeMaxCacheSize()
{
    maxCacheSize = 1;
    foreach (const PixmapCacheModel::PixmapCacheEvent &event, ranges) {
        if (event.pixmapEventType == PixmapCacheModel::PixmapCacheCountChanged) {
            if (event.cacheSize > maxCacheSize)
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

int PixmapCacheModel::PixmapCacheModelPrivate::updateCacheCount(int lastCacheSizeEvent,
        qint64 startTime, qint64 pixSize, PixmapCacheEvent &newEvent)
{
    newEvent.pixmapEventType = PixmapCacheCountChanged;
    newEvent.eventId = 0;
    newEvent.rowNumberExpanded = 1;
    newEvent.rowNumberCollapsed = 1;

    qint64 prevSize = 0;
    if (lastCacheSizeEvent != -1) {
        prevSize = range(lastCacheSizeEvent).cacheSize;
        insertEnd(lastCacheSizeEvent, startTime - range(lastCacheSizeEvent).start);
    }

    newEvent.cacheSize = prevSize + pixSize;
    return insertStart(startTime, newEvent);
}


} // namespace Internal
} // namespace QmlProfilerExtension
