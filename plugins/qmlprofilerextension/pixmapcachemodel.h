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

#ifndef PIXMAPCACHEMODEL_H
#define PIXMAPCACHEMODEL_H

#include "qmlprofiler/abstracttimelinemodel.h"
#include "qmlprofiler/qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>
#include <QSize>

namespace QmlProfilerExtension {
namespace Internal {

class PixmapCacheModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
public:
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

    enum PixmapEventType {
        PixmapSizeKnown,
        PixmapReferenceCountChanged,
        PixmapCacheCountChanged,
        PixmapLoadingStarted,
        PixmapLoadingFinished,
        PixmapLoadingError,

        MaximumPixmapEventType
    };

    struct PixmapCacheEvent {
        int typeId;
        PixmapEventType pixmapEventType;
        int urlIndex;
        int sizeIndex;
        int rowNumberCollapsed;
        qint64 cacheSize;
    };

    PixmapCacheModel(QObject *parent = 0);
    quint64 features() const;

    int rowMaxValue(int rowNumber) const;

    int row(int index) const;
    int typeId(int index) const;
    QColor color(int index) const;
    float relativeHeight(int index) const;

    QVariantList labels() const;

    QVariantMap details(int index) const;

protected:
    void loadData();
    void clear();

private:
    void computeMaxCacheSize();
    void resizeUnfinishedLoads();
    void flattenLoads();
    int updateCacheCount(int lastCacheSizeEvent, qint64 startTime, qint64 pixSize,
                         PixmapCacheEvent &newEvent, int typeId);

    QVector<PixmapCacheEvent> m_data;
    QVector<Pixmap> m_pixmaps;
    qint64 m_maxCacheSize;

    static const int s_pixmapCacheCountHue = 240;
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // PIXMAPCACHEMODEL_H
