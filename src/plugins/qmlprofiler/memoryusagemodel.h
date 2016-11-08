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
#include <QStack>

namespace QmlProfiler {
namespace Internal {

class MemoryUsageModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct MemoryAllocationItem {
        qint64 size;
        qint64 allocated;
        qint64 deallocated;
        int allocations;
        int deallocations;
        int typeId;

        MemoryAllocationItem(int typeId = -1, qint64 baseAmount = 0);
        void update(qint64 amount);
    };

    MemoryUsageModel(QmlProfilerModelManager *manager, QObject *parent = 0);

    int rowMaxValue(int rowNumber) const override;

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    int typeId(int index) const override;
    QRgb color(int index) const override;
    float relativeHeight(int index) const override;

    QVariantMap location(int index) const override;

    QVariantList labels() const override;
    QVariantMap details(int index) const override;

    bool accepted(const QmlEventType &type) const override;
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;
    bool handlesTypeId(int typeId) const override;

private:
    struct RangeStackFrame {
        RangeStackFrame() : originTypeIndex(-1), startTime(-1) {}
        RangeStackFrame(int originTypeIndex, qint64 startTime) :
            originTypeIndex(originTypeIndex), startTime(startTime) {}
        int originTypeIndex;
        qint64 startTime;
    };

    enum EventContinuation {
        ContinueNothing    = 0x0,
        ContinueAllocation = 0x1,
        ContinueUsage      = 0x2
    };

    QVector<MemoryAllocationItem> m_data;
    QStack<RangeStackFrame> m_rangeStack;
    qint64 m_maxSize = 1;
    qint64 m_currentSize = 0;
    qint64 m_currentUsage = 0;
    int m_currentUsageIndex = -1;
    int m_currentJSHeapIndex = -1;

    int m_continuation = ContinueNothing;
};

} // namespace Internal
} // namespace QmlProfiler
