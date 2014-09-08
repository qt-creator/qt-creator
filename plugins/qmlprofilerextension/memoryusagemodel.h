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
#include "qmlprofiler/abstracttimelinemodel.h"
#include "qmlprofiler/qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

class MemoryUsageModel : public QmlProfiler::AbstractTimelineModel
{
    Q_OBJECT
public:

    struct MemoryAllocation {
        QmlDebug::MemoryType type;
        qint64 size;
        qint64 allocated;
        qint64 deallocated;
        int allocations;
        int deallocations;
        int originTypeIndex;

        MemoryAllocation(QmlDebug::MemoryType type = QmlDebug::MaximumMemoryType,
                         qint64 baseAmount = 0, int originTypeIndex = -1);
        void update(qint64 amount);
    };

    MemoryUsageModel(QObject *parent = 0);
    quint64 features() const;

    int rowCount() const;
    int rowMaxValue(int rowNumber) const;

    int row(int index) const;
    int eventId(int index) const;
    QColor color(int index) const;
    float relativeHeight(int index) const;

    QVariantMap location(int index) const;

    QVariantList labels() const;
    QVariantMap details(int index) const;

    void loadData();
    void clear();

private:
    class MemoryUsageModelPrivate;
    Q_DECLARE_PRIVATE(MemoryUsageModel)
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // MEMORYUSAGEMODEL_H
