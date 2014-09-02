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
#include "qmlprofiler/abstracttimelinemodel_p.h"

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

class PixmapCacheModel::PixmapCacheModelPrivate : public AbstractTimelineModelPrivate
{
public:
    void computeMaxCacheSize();
    void resizeUnfinishedLoads();
    void flattenLoads();
    int updateCacheCount(int lastCacheSizeEvent, qint64 startTime, qint64 pixSize,
                         PixmapCacheEvent &newEvent);

    QVector<PixmapCacheEvent> data;
    QVector<Pixmap> pixmaps;
    int collapsedRowCount;

    qint64 maxCacheSize;
private:
    Q_DECLARE_PUBLIC(PixmapCacheModel)
};

PixmapCacheModel::PixmapCacheModel(QObject *parent)
    : AbstractTimelineModel(new PixmapCacheModelPrivate(), QLatin1String("Pixmap Cache"),
                            QmlDebug::PixmapCacheEvent, QmlDebug::MaximumRangeType, parent)
{
    Q_D(PixmapCacheModel);
    d->collapsedRowCount = 1;
    d->maxCacheSize = 1;
}

int PixmapCacheModel::rowCount() const
{
    Q_D(const PixmapCacheModel);
    if (isEmpty())
        return 1;
    if (d->expanded)
        return d->pixmaps.count() + 2;
    return d->collapsedRowCount;
}

int PixmapCacheModel::rowMaxValue(int rowNumber) const
{
    Q_D(const PixmapCacheModel);
    if (rowNumber == 1) {
        return d->maxCacheSize;
    } else {
        return AbstractTimelineModel::rowMaxValue(rowNumber);
    }
}

int PixmapCacheModel::row(int index) const
{
    Q_D(const PixmapCacheModel);
    if (d->expanded)
        return eventId(index) + 1;
    return d->data[index].rowNumberCollapsed;
}

int PixmapCacheModel::eventId(int index) const
{
    Q_D(const PixmapCacheModel);
    return d->data[index].pixmapEventType == PixmapCacheCountChanged ?
                0 : d->data[index].urlIndex + 1;
}

QColor PixmapCacheModel::color(int index) const
{
    Q_D(const PixmapCacheModel);
    if (d->data[index].pixmapEventType == PixmapCacheCountChanged)
        return colorByHue(PixmapCacheCountHue);

    return colorByEventId(index);
}

float PixmapCacheModel::relativeHeight(int index) const
{
    Q_D(const PixmapCacheModel);
    if (d->data[index].pixmapEventType == PixmapCacheCountChanged)
        return (float)d->data[index].cacheSize / (float)d->maxCacheSize;
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

QVariantList PixmapCacheModel::labels() const
{
    Q_D(const PixmapCacheModel);
    QVariantList result;

    if (d->expanded && !isEmpty()) {
        {
            // Cache Size
            QVariantMap element;
            element.insert(QLatin1String("description"), QVariant(QLatin1String("Cache Size")));

            element.insert(QLatin1String("id"), QVariant(0));
            result << element;
        }

        for (int i=0; i < d->pixmaps.count(); i++) {
            // Loading
            QVariantMap element;
            element.insert(QLatin1String("description"),
                           QVariant(getFilenameOnly(d->pixmaps[i].url)));

            element.insert(QLatin1String("id"), QVariant(i+1));
            result << element;
        }
    }

    return result;
}

QVariantMap PixmapCacheModel::details(int index) const
{
    Q_D(const PixmapCacheModel);
    QVariantMap result;
    const PixmapCacheEvent *ev = &d->data[index];

    if (ev->pixmapEventType == PixmapCacheCountChanged) {
        result.insert(QLatin1String("displayName"), tr("Image Cached"));
    } else {
        if (ev->pixmapEventType == PixmapLoadingStarted) {
            result.insert(QLatin1String("displayName"), tr("Image Loaded"));
            if (d->pixmaps[ev->urlIndex].sizes[ev->sizeIndex].loadState != Finished)
                result.insert(tr("Result"), tr("Load Error"));
        }
        result.insert(tr("Duration"), QmlProfilerBaseModel::formatTime(range(index).duration));
    }

    result.insert(tr("Cache Size"), QString::fromLatin1("%1 px").arg(ev->cacheSize));
    result.insert(tr("File"), getFilenameOnly(d->pixmaps[ev->urlIndex].url));
    result.insert(tr("Width"), QString::fromLatin1("%1 px")
                  .arg(d->pixmaps[ev->urlIndex].sizes[ev->sizeIndex].size.width()));
    result.insert(tr("Height"), QString::fromLatin1("%1 px")
                  .arg(d->pixmaps[ev->urlIndex].sizes[ev->sizeIndex].size.height()));
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

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types = simpleModel->getEventTypes();
    foreach (const QmlProfilerDataModel::QmlEventData &event, simpleModel->getEvents()) {
        const QmlProfilerDataModel::QmlEventTypeData &type = types[event.typeIndex];
        if (!accepted(type))
            continue;

        PixmapCacheEvent newEvent;
        newEvent.pixmapEventType = type.detailType;
        qint64 startTime = event.startTime;

        newEvent.urlIndex = -1;
        for (QVector<Pixmap>::const_iterator it(d->pixmaps.cend()); it != d->pixmaps.cbegin();) {
            if ((--it)->url == type.location.filename) {
                newEvent.urlIndex = it - d->pixmaps.cbegin();
                break;
            }
        }

        newEvent.sizeIndex = -1;
        if (newEvent.urlIndex == -1) {
            newEvent.urlIndex = d->pixmaps.count();
            d->pixmaps << Pixmap(type.location.filename);
        }

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
        case PixmapLoadingStarted: { // Load
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

            PixmapState &state = pixmap.sizes[newEvent.sizeIndex];
            state.loadState = Loading;
            state.started = insertStart(startTime);
            d->data.insert(state.started, newEvent);
            break;
        }
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
                state.started = insert(traceStartTime(), startTime - traceStartTime());
                d->data.insert(state.started, newEvent);

                // All other indices are wrong now as we've prepended. Fix them ...
                if (lastCacheSizeEvent >= state.started)
                    ++lastCacheSizeEvent;

                for (int pixmapIndex = 0; pixmapIndex < d->pixmaps.count(); ++pixmapIndex) {
                    Pixmap &brokenPixmap = d->pixmaps[pixmapIndex];
                    for (int sizeIndex = 0; sizeIndex < brokenPixmap.sizes.count(); ++sizeIndex) {
                        PixmapState &brokenSize = brokenPixmap.sizes[sizeIndex];
                        if ((pixmapIndex != newEvent.urlIndex || sizeIndex != newEvent.sizeIndex) &&
                                brokenSize.started >= state.started) {
                            ++brokenSize.started;
                        }
                    }
                }
            }

            insertEnd(state.started, startTime - range(state.started).start);
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

        d->modelManager->modelProxyCountUpdated(d->modelId, count(),
                                                2 * simpleModel->getEvents().count());
    }

    if (lastCacheSizeEvent != -1)
        insertEnd(lastCacheSizeEvent, traceEndTime() - range(lastCacheSizeEvent).start);

    d->resizeUnfinishedLoads();

    d->computeMaxCacheSize();
    d->flattenLoads();
    computeNesting();

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void PixmapCacheModel::clear()
{
    Q_D(PixmapCacheModel);
    d->pixmaps.clear();
    d->collapsedRowCount = 1;
    d->maxCacheSize = 1;
    d->data.clear();
    AbstractTimelineModel::clear();
}

void PixmapCacheModel::PixmapCacheModelPrivate::computeMaxCacheSize()
{
    maxCacheSize = 1;
    foreach (const PixmapCacheModel::PixmapCacheEvent &event, data) {
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
    for (int i = 0; i < q->count(); i++) {
        if (data[i].pixmapEventType == PixmapCacheModel::PixmapLoadingStarted &&
                q->range(i).duration == 0) {
            q->insertEnd(i, q->traceEndTime() - q->range(i).start);
        }
    }
}

void PixmapCacheModel::PixmapCacheModelPrivate::flattenLoads()
{
    Q_Q(PixmapCacheModel);
    collapsedRowCount = 0;

    // computes "compressed row"
    QVector <qint64> eventEndTimes;
    for (int i = 0; i < q->count(); i++) {
        PixmapCacheModel::PixmapCacheEvent &event = data[i];
        const Range &start = q->range(i);
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
        if (event.rowNumberCollapsed > collapsedRowCount)
            collapsedRowCount = event.rowNumberCollapsed;
    }

    // Starting from 0, count is maxIndex+1
    collapsedRowCount++;
}

int PixmapCacheModel::PixmapCacheModelPrivate::updateCacheCount(int lastCacheSizeEvent,
        qint64 startTime, qint64 pixSize, PixmapCacheEvent &newEvent)
{
    Q_Q(PixmapCacheModel);
    newEvent.pixmapEventType = PixmapCacheCountChanged;
    newEvent.rowNumberCollapsed = 1;

    qint64 prevSize = 0;
    if (lastCacheSizeEvent != -1) {
        prevSize = data[lastCacheSizeEvent].cacheSize;
        q->insertEnd(lastCacheSizeEvent, startTime - q->range(lastCacheSizeEvent).start);
    }

    newEvent.cacheSize = prevSize + pixSize;
    int index = q->insertStart(startTime);
    data.insert(index, newEvent);
    return index;
}


} // namespace Internal
} // namespace QmlProfilerExtension
