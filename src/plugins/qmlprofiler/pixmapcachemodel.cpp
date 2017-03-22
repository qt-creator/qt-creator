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

#include "pixmapcachemodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilereventtypes.h"

#include <timeline/timelineformattime.h>

namespace QmlProfiler {
namespace Internal {

PixmapCacheModel::PixmapCacheModel(QmlProfilerModelManager *manager, QObject *parent) :
    QmlProfilerTimelineModel(manager, PixmapCacheEvent, MaximumRangeType, ProfilePixmapCache,
                             parent)
{
}

int PixmapCacheModel::rowMaxValue(int rowNumber) const
{
    if (rowNumber == 1) {
        return m_maxCacheSize;
    } else {
        return QmlProfilerTimelineModel::rowMaxValue(rowNumber);
    }
}

int PixmapCacheModel::expandedRow(int index) const
{
    return selectionId(index) + 1;
}

int PixmapCacheModel::collapsedRow(int index) const
{
    return m_data[index].rowNumberCollapsed;
}

int PixmapCacheModel::typeId(int index) const
{
    return m_data[index].typeId;
}

QRgb PixmapCacheModel::color(int index) const
{
    if (m_data[index].pixmapEventType == PixmapCacheCountChanged)
        return colorByHue(s_pixmapCacheCountHue);

    return colorBySelectionId(index);
}

float PixmapCacheModel::relativeHeight(int index) const
{
    if (m_data[index].pixmapEventType == PixmapCacheCountChanged)
        return (float)m_data[index].cacheSize / (float)m_maxCacheSize;
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
    QVariantList result;

    // Cache Size
    QVariantMap element;
    element.insert(QLatin1String("description"), tr("Cache Size"));

    element.insert(QLatin1String("id"), 0);
    result << element;

    for (int i = 0; i < m_pixmaps.count(); ++i) {
        // Loading
        QVariantMap element;
        element.insert(QLatin1String("displayName"), m_pixmaps[i].url);
        element.insert(QLatin1String("description"), getFilenameOnly(m_pixmaps[i].url));

        element.insert(QLatin1String("id"), QVariant(i + 1));
        result << element;
    }

    return result;
}

QVariantMap PixmapCacheModel::details(int index) const
{
    QVariantMap result;
    const PixmapCacheItem *ev = &m_data[index];

    if (ev->pixmapEventType == PixmapCacheCountChanged) {
        result.insert(QLatin1String("displayName"), tr("Image Cached"));
        result.insert(tr("Cache Size"), QString::fromLatin1("%1 px").arg(ev->cacheSize));
    } else {
        result.insert(QLatin1String("displayName"), tr("Image Loaded"));
        if (m_pixmaps[ev->urlIndex].sizes[ev->sizeIndex].loadState != Finished)
            result.insert(tr("Result"), tr("Load Error"));
        result.insert(tr("Duration"), Timeline::formatTime(duration(index)));
    }

    result.insert(tr("File"), getFilenameOnly(m_pixmaps[ev->urlIndex].url));
    result.insert(tr("Width"), QString::fromLatin1("%1 px")
                  .arg(m_pixmaps[ev->urlIndex].sizes[ev->sizeIndex].size.width()));
    result.insert(tr("Height"), QString::fromLatin1("%1 px")
                  .arg(m_pixmaps[ev->urlIndex].sizes[ev->sizeIndex].size.height()));
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
void PixmapCacheModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    PixmapCacheItem newEvent;
    const PixmapEventType pixmapType = static_cast<PixmapEventType>(type.detailType());
    newEvent.pixmapEventType = pixmapType;
    qint64 pixmapStartTime = event.timestamp();

    newEvent.urlIndex = -1;
    for (auto i = m_pixmaps.cend(), begin = m_pixmaps.cbegin(); i != begin;) {
        if ((--i)->url == type.location().filename()) {
            newEvent.urlIndex = i - m_pixmaps.cbegin();
            break;
        }
    }

    newEvent.sizeIndex = -1;
    if (newEvent.urlIndex == -1) {
        newEvent.urlIndex = m_pixmaps.count();
        m_pixmaps << Pixmap(type.location().filename());
    }

    Pixmap &pixmap = m_pixmaps[newEvent.urlIndex];
    switch (pixmapType) {
    case PixmapSizeKnown: {// pixmap size
        // Look for pixmaps for which we don't know the size, yet and which have actually been
        // loaded.
        for (auto i = pixmap.sizes.begin(), end = pixmap.sizes.end(); i != end; ++i) {
            if (i->size.isValid() || i->cacheState == Uncacheable || i->cacheState == Corrupt)
                continue;

            // We can't have cached it before we knew the size
            Q_ASSERT(i->cacheState != Cached);

            i->size.setWidth(event.number<qint32>(0));
            i->size.setHeight(event.number<qint32>(1));
            newEvent.sizeIndex = i - pixmap.sizes.begin();
            break;
        }

        if (newEvent.sizeIndex == -1) {
            newEvent.sizeIndex = pixmap.sizes.length();
            pixmap.sizes << PixmapState(event.number<qint32>(0), event.number<qint32>(1));
        }

        PixmapState &state = pixmap.sizes[newEvent.sizeIndex];
        if (state.cacheState == ToBeCached) {
            m_lastCacheSizeEvent = updateCacheCount(
                        m_lastCacheSizeEvent, pixmapStartTime,
                        (qint64) state.size.width() * (qint64) state.size.height(),
                        newEvent, event.typeIndex());
            state.cacheState = Cached;
        }
        break;
    }
    case PixmapCacheCountChanged: {// Cache Size Changed Event
        bool uncache = m_cumulatedCount > event.number<qint32>(2);
        m_cumulatedCount = event.number<qint32>(2);
        qint64 pixSize = 0;

        // First try to find a preferred pixmap, which either is Corrupt and will be uncached
        // or is uncached and will be cached.
        for (auto i = pixmap.sizes.begin(), end = pixmap.sizes.end(); i != end; ++i) {
            if (uncache && i->cacheState == Corrupt) {
                newEvent.sizeIndex = i - pixmap.sizes.begin();
                i->cacheState = Uncacheable;
                break;
            } else if (!uncache && i->cacheState == Uncached) {
                newEvent.sizeIndex = i - pixmap.sizes.begin();
                if (i->size.isValid()) {
                    pixSize = (qint64) i->size.width() * i->size.height();
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
            for (auto i = pixmap.sizes.begin(), end = pixmap.sizes.end(); i != end; ++i) {
                if (uncache && (i->cacheState == Cached || i->cacheState == ToBeCached)) {
                    newEvent.sizeIndex = i - pixmap.sizes.begin();
                    if (i->size.isValid())
                        pixSize = (qint64) -i->size.width() * i->size.height();
                    i->cacheState = Uncached;
                    break;
                } else if (!uncache && i->cacheState == Uncacheable) {
                    // A pixmap can repeatedly be cached, become corrupt, and the be uncached again.
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
            // now the size is 0. Thus, there is no point in updating the size row.
        } else if (pixSize != 0) {
            m_lastCacheSizeEvent = updateCacheCount(m_lastCacheSizeEvent, pixmapStartTime, pixSize,
                                                    newEvent, event.typeIndex());
        }
        break;
    }
    case PixmapLoadingStarted: { // Load
        // Look for a pixmap that hasn't been started, yet. There may have been a refcount
        // event, which we ignore.
        for (auto i = pixmap.sizes.cbegin(), end = pixmap.sizes.cend(); i != end; ++i) {
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
        newEvent.typeId = event.typeIndex();
        state.started = insertStart(pixmapStartTime, newEvent.urlIndex + 1);
        m_data.insert(state.started, newEvent);
        break;
    }
    case PixmapLoadingFinished:
    case PixmapLoadingError: {
        // First try to find one that has already started
        for (auto i = pixmap.sizes.cbegin(), end = pixmap.sizes.cend(); i != end; ++i) {
            if (i->loadState != Loading)
                continue;
            // Pixmaps with known size cannot be errors and vice versa
            if (pixmapType == PixmapLoadingError && i->size.isValid())
                continue;

            newEvent.sizeIndex = i - pixmap.sizes.cbegin();
            break;
        }

        // If none was found use any other compatible one
        if (newEvent.sizeIndex == -1) {
            for (auto i = pixmap.sizes.cbegin(), end = pixmap.sizes.cend(); i != end; ++i) {
                if (i->loadState != Initial)
                    continue;
                // Pixmaps with known size cannot be errors and vice versa
                if (pixmapType == PixmapLoadingError && i->size.isValid())
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
            newEvent.typeId = event.typeIndex();
            qint64 traceStart = modelManager()->traceTime()->startTime();
            state.started = insert(traceStart, pixmapStartTime - traceStart,
                                   newEvent.urlIndex + 1);
            m_data.insert(state.started, newEvent);

            // All other indices are wrong now as we've prepended. Fix them ...
            if (m_lastCacheSizeEvent >= state.started)
                ++m_lastCacheSizeEvent;

            for (int pixmapIndex = 0; pixmapIndex < m_pixmaps.count(); ++pixmapIndex) {
                Pixmap &brokenPixmap = m_pixmaps[pixmapIndex];
                for (int sizeIndex = 0; sizeIndex < brokenPixmap.sizes.count(); ++sizeIndex) {
                    PixmapState &brokenSize = brokenPixmap.sizes[sizeIndex];
                    if ((pixmapIndex != newEvent.urlIndex || sizeIndex != newEvent.sizeIndex) &&
                            brokenSize.started >= state.started) {
                        ++brokenSize.started;
                    }
                }
            }
        } else {
            insertEnd(state.started, pixmapStartTime - startTime(state.started));
        }

        if (pixmapType == PixmapLoadingError) {
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
                Q_UNREACHABLE();
            }
        } else {
            state.loadState = Finished;
        }
        break;
    }
    default:
        break;
    }
}

void PixmapCacheModel::finalize()
{
    if (m_lastCacheSizeEvent != -1) {
        insertEnd(m_lastCacheSizeEvent, modelManager()->traceTime()->endTime() -
                  startTime(m_lastCacheSizeEvent));
    }

    resizeUnfinishedLoads();
    computeMaxCacheSize();
    flattenLoads();
    computeNesting();
}

void PixmapCacheModel::clear()
{
    m_pixmaps.clear();
    m_data.clear();
    m_maxCacheSize = 1;
    m_lastCacheSizeEvent = -1;
    m_cumulatedCount = 0;
    QmlProfilerTimelineModel::clear();
}

#ifdef WITH_TESTS
PixmapCacheModel::LoadState PixmapCacheModel::loadState(int index) const
{
    const PixmapCacheItem &item = m_data[index];
    if (item.urlIndex == -1 || item.sizeIndex == -1)
        return MaximumLoadState;

    return m_pixmaps[item.urlIndex].sizes[item.sizeIndex].loadState;
}

PixmapCacheModel::CacheState PixmapCacheModel::cacheState(int index) const
{
    const PixmapCacheItem &item = m_data[index];
    if (item.urlIndex == -1 || item.sizeIndex == -1)
        return MaximumCacheState;

    return m_pixmaps[item.urlIndex].sizes[item.sizeIndex].cacheState;
}

QString PixmapCacheModel::fileName(int index) const
{
    const PixmapCacheItem &item = m_data[index];
    return (item.urlIndex == -1) ? QString() : m_pixmaps[item.urlIndex].url;
}
#endif // WITH_TESTS

void PixmapCacheModel::computeMaxCacheSize()
{
    foreach (const PixmapCacheModel::PixmapCacheItem &event, m_data) {
        if (event.pixmapEventType == PixmapCacheModel::PixmapCacheCountChanged) {
            if (event.cacheSize > m_maxCacheSize)
                m_maxCacheSize = event.cacheSize;
        }
    }
}

void PixmapCacheModel::resizeUnfinishedLoads()
{
    // all the unfinished "load start" events continue till the end of the trace
    for (auto pixmap = m_pixmaps.begin(), pixmapsEnd = m_pixmaps.end();
         pixmap != pixmapsEnd; ++pixmap) {
        for (auto size = pixmap->sizes.begin(), sizesEnd = pixmap->sizes.end(); size != sizesEnd;
             ++size) {
            if (size->loadState == Loading) {
                insertEnd(size->started,
                          modelManager()->traceTime()->endTime() - startTime(size->started));
                size->loadState = Error;
            }
        }
    }
}

void PixmapCacheModel::flattenLoads()
{
    int collapsedRowCount = 0;

    // computes "compressed row"
    QVector <qint64> eventEndTimes;
    for (int i = 0; i < count(); i++) {
        PixmapCacheModel::PixmapCacheItem &event = m_data[i];
        if (event.pixmapEventType == PixmapCacheModel::PixmapLoadingStarted) {
            event.rowNumberCollapsed = 0;
            while (eventEndTimes.count() > event.rowNumberCollapsed &&
                   eventEndTimes[event.rowNumberCollapsed] > startTime(i))
                event.rowNumberCollapsed++;

            if (eventEndTimes.count() == event.rowNumberCollapsed)
                eventEndTimes << 0; // increase stack length, proper value added below
            eventEndTimes[event.rowNumberCollapsed] = endTime(i);

            // readjust to account for category empty row and bargraph
            event.rowNumberCollapsed += 2;
        }
        if (event.rowNumberCollapsed > collapsedRowCount)
            collapsedRowCount = event.rowNumberCollapsed;
    }

    // Starting from 0, count is maxIndex+1
    setCollapsedRowCount(collapsedRowCount + 1);
    setExpandedRowCount(m_pixmaps.count() + 2);
}

int PixmapCacheModel::updateCacheCount(int lastCacheSizeEvent,
        qint64 pixmapStartTime, qint64 pixSize, PixmapCacheItem &newEvent, int typeId)
{
    newEvent.pixmapEventType = PixmapCacheCountChanged;
    newEvent.rowNumberCollapsed = 1;
    newEvent.typeId = typeId;

    int index = lastCacheSizeEvent;
    if (lastCacheSizeEvent != -1) {
        newEvent.cacheSize = m_data[lastCacheSizeEvent].cacheSize + pixSize;
        qint64 duration = pixmapStartTime - startTime(lastCacheSizeEvent);
        if (duration > 0) {
            insertEnd(lastCacheSizeEvent, duration);
            index = insertStart(pixmapStartTime, 0);
            m_data.insert(index, newEvent);
        } else {
            // If the timestamps are the same, just replace it
            m_data[index] = newEvent;
        }
    } else {
        newEvent.cacheSize = pixSize;
        index = insertStart(pixmapStartTime, 0);
        m_data.insert(index, newEvent);
    }

    return index;
}


} // namespace Internal
} // namespace QmlProfiler
