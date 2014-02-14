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

#ifndef SCENEGRAPHTIMELINEMODEL_H
#define SCENEGRAPHTIMELINEMODEL_H

#include "qmlprofiler/singlecategorytimelinemodel.h"
#include "qmlprofiler/qmlprofilermodelmanager.h"
#include "qmlprofiler/qmlprofilersimplemodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

#define timingFieldCount 16

class SceneGraphTimelineModel : public QmlProfiler::SingleCategoryTimelineModel
{
    Q_OBJECT
public:

    struct SceneGraphEvent {
        int sgEventType;
        qint64 timing[timingFieldCount];
    };

    SceneGraphTimelineModel(QObject *parent = 0);

    Q_INVOKABLE int categoryDepth(int categoryIndex) const;

    int getEventRow(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
    Q_INVOKABLE QColor getColor(int index) const;

    Q_INVOKABLE const QVariantList getLabelsForCategory(int category) const;

    Q_INVOKABLE const QVariantList getEventDetails(int index) const;

    void loadData();
    void clear();

private:
    class SceneGraphTimelineModelPrivate;
    Q_DECLARE_PRIVATE(SceneGraphTimelineModel)
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // SCENEGRAPHTIMELINEMODEL_H
