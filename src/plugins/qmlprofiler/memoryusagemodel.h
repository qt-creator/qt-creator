// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"

#include <QStringList>
#include <QColor>
#include <QStack>

namespace QmlProfiler {
namespace Internal {

class MemoryUsageModel : public QmlProfilerTimelineModel
{
    Q_OBJECT
public:

    struct Item {
        qint64 size;
        qint64 allocated;
        qint64 deallocated;
        int allocations;
        int deallocations;
        int typeId;

        Item(int typeId = -1, qint64 baseAmount = 0);
        void update(qint64 amount);
    };

    MemoryUsageModel(QmlProfilerModelManager *manager, Timeline::TimelineModelAggregator *parent);

    qint64 rowMaxValue(int rowNumber) const override;

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    int typeId(int index) const override;
    QRgb color(int index) const override;
    float relativeHeight(int index) const override;

    QVariantMap location(int index) const override;

    QVariantList labels() const override;
    QVariantMap details(int index) const override;

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;
    bool handlesTypeId(int typeId) const override;

private:
    struct RangeStackFrame {
        RangeStackFrame() = default;
        RangeStackFrame(int originTypeIndex, qint64 startTime) :
            originTypeIndex(originTypeIndex), startTime(startTime) {}
        int originTypeIndex = -1;
        qint64 startTime = -1;
    };

    enum EventContinuation {
        ContinueNothing    = 0x0,
        ContinueAllocation = 0x1,
        ContinueUsage      = 0x2
    };

    QVector<Item> m_data;
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
