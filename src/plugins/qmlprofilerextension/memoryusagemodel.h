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
