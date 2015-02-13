/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef MEMORYUSAGEMODEL_H
#define MEMORYUSAGEMODEL_H

#include "qmlprofiler/qmlprofilertimelinemodel.h"
#include "qmlprofiler/qmlprofilerdatamodel.h"

#include <QStringList>
#include <QColor>

namespace QmlProfilerExtension {
namespace Internal {

class MemoryUsageModel : public QmlProfiler::QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct MemoryAllocation {
        int typeId;
        qint64 size;
        qint64 allocated;
        qint64 deallocated;
        int allocations;
        int deallocations;
        int originTypeIndex;

        MemoryAllocation(int typeId = -1, qint64 baseAmount = 0, int originTypeIndex = -1);
        void update(qint64 amount);
    };

    MemoryUsageModel(QmlProfiler::QmlProfilerModelManager *manager, QObject *parent = 0);

    int rowMaxValue(int rowNumber) const;

    int expandedRow(int index) const;
    int collapsedRow(int index) const;
    int typeId(int index) const;
    QColor color(int index) const;
    float relativeHeight(int index) const;

    QVariantMap location(int index) const;

    QVariantList labels() const;
    QVariantMap details(int index) const;

protected:
    void loadData();
    void clear();

private:
    static QString memoryTypeName(int type);

    QVector<MemoryAllocation> m_data;
    qint64 m_maxSize;
};

} // namespace Internal
} // namespace QmlProfilerExtension

#endif // MEMORYUSAGEMODEL_H
