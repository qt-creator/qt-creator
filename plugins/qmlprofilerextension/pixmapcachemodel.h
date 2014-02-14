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

#include "qmlprofiler/qmlprofilertimelinemodelproxy.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/qmlprofilersimplemodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

class PixmapCacheModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
public:

    struct PixmapCacheEvent {
        int eventId;
        int pixmapEventType;
        int urlIndex;
        qint64 cacheSize;
        int rowNumberExpanded;
        int rowNumberCollapsed;
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

    PixmapCacheModel(QObject *parent = 0);
    ~PixmapCacheModel();


//    void setModelManager(QmlProfiler::Internal::QmlProfilerModelManager *modelManager);

    QString name() const;
    int count() const;

    bool isEmpty() const;

    bool eventAccepted(const QmlProfiler::QmlProfilerSimpleModel::QmlEventData &event) const;

    Q_INVOKABLE qint64 lastTimeMark() const;

    Q_INVOKABLE bool expanded(int category) const;
    Q_INVOKABLE void setExpanded(int category, bool expanded);
    Q_INVOKABLE int categoryDepth(int categoryIndex) const;
    Q_INVOKABLE int categoryCount() const;
    Q_INVOKABLE const QString categoryLabel(int categoryIndex) const;

    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;

    int getEventType(int index) const;
    int getEventCategory(int index) const;
    int getEventRow(int index) const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getEndTime(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
    Q_INVOKABLE QColor getColor(int index) const;
    Q_INVOKABLE float getHeight(int index) const;

    Q_INVOKABLE const QVariantList getLabelsForCategory(int category) const;

    Q_INVOKABLE const QVariantList getEventDetails(int index) const;
    Q_INVOKABLE const QVariantMap getEventLocation(int index) const;

    Q_INVOKABLE int getEventIdForHash(const QString &eventHash) const;
    Q_INVOKABLE int getEventIdForLocation(const QString &filename, int line, int column) const;

    void loadData();
    void clear();

private:
    class PixmapCacheModelPrivate;
    PixmapCacheModelPrivate *d;

};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // PIXMAPCACHEMODEL_H
