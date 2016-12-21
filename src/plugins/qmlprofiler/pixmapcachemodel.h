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

#pragma once

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>
#include <QSize>

namespace QmlProfiler {
namespace Internal {

class PixmapCacheModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:
    enum CacheState {
        Uncached,    // After loading started (or some other proof of existence) or after uncaching
        ToBeCached,  // After determining the pixmap is to be cached but before knowing its size
        Cached,      // After caching a pixmap or determining the size of a ToBeCached pixmap
        Uncacheable, // If loading failed without ToBeCached or after a corrupt pixmap has been uncached
        Corrupt,     // If after ToBeCached we learn that loading failed

        MaximumCacheState
    };

    enum LoadState {
        Initial,
        Loading,
        Finished,
        Error,

        MaximumLoadState
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

    struct PixmapCacheItem {
        int typeId = -1;
        PixmapEventType pixmapEventType = MaximumPixmapEventType;
        int urlIndex = -1;
        int sizeIndex = -1;
        int rowNumberCollapsed = -1;
        qint64 cacheSize = 0;
    };

    PixmapCacheModel(QmlProfilerModelManager *manager, QObject *parent = 0);

    int rowMaxValue(int rowNumber) const override;

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    int typeId(int index) const override;
    QRgb color(int index) const override;
    float relativeHeight(int index) const override;

    QVariantList labels() const override;

    QVariantMap details(int index) const override;

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;

#ifdef WITH_TESTS
    LoadState loadState(int index) const;
    CacheState cacheState(int index) const;
    QString fileName(int index) const;
#endif

private:
    void computeMaxCacheSize();
    void resizeUnfinishedLoads();
    void flattenLoads();
    int updateCacheCount(int m_lastCacheSizeEvent, qint64 startTime, qint64 pixSize,
                         PixmapCacheItem &newEvent, int typeId);

    QVector<PixmapCacheItem> m_data;
    QVector<Pixmap> m_pixmaps;

    qint64 m_maxCacheSize = 1;
    int m_lastCacheSizeEvent = -1;
    int m_cumulatedCount = 0;

    static const int s_pixmapCacheCountHue = 240;
};

} // namespace Internal
} // namespace QmlProfiler
