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

#ifndef MEMORYUSAGEMODEL_H
#define MEMORYUSAGEMODEL_H

#include "qmlprofiler/qmlprofilertimelinemodelproxy.h"
#include "qmlprofiler/singlecategorytimelinemodel.h"
#include "qmlprofiler/qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

class MemoryUsageModel : public QmlProfiler::SingleCategoryTimelineModel
{
    Q_OBJECT
public:

    struct MemoryAllocation {
        QmlDebug::MemoryType type;
        qint64 size;
        qint64 delta;
    };

    MemoryUsageModel(QObject *parent = 0);

    int rowCount() const;

    int getEventRow(int index) const;
    int getEventId(int index) const;
    QColor getColor(int index) const;
    float getHeight(int index) const;

    const QVariantList getLabels() const;
    const QVariantList getEventDetails(int index) const;

    void loadData();
    void clear();

private:
    class MemoryUsageModelPrivate;
    Q_DECLARE_PRIVATE(MemoryUsageModel)
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // MEMORYUSAGEMODEL_H
