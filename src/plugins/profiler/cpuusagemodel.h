// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampletrace.h"

#include <tracing/timelinemodel.h>

namespace QmlProfiler::Internal {

// CPU usage of a sampled recording in the timeline: a "Total" graph row with
// the number of running threads per sampling tick, plus one row per thread
// marking when that thread was on-CPU. Graph rendering works through
// relativeHeight(), like the QML profiler's MemoryUsageModel.
//
// Item times are nanoseconds (the timeline's display unit); the sample data
// is microseconds.
class CpuUsageModel : public Timeline::TimelineModel
{
    Q_OBJECT

public:
    explicit CpuUsageModel(Timeline::TimelineModelAggregator *parent);

    // The model keeps the pointer; pass nullptr to clear.
    void setTraceData(const SampleTraceData *data);

    qint64 traceEndNs() const { return m_traceEndNs; }

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    QRgb color(int index) const override;
    float relativeHeight(int index) const override;
    qint64 rowMaxValue(int rowNumber) const override;
    Timeline::RowLabels labels() const override;
    Timeline::ItemDetails details(int index) const override;

    void clear() override;

private:
    void load();

    struct Item
    {
        int running = 0;    // running threads at this tick
        int threadRow = -1; // -1: total item; otherwise index into m_threads
        quint64 tid = 0;
    };

    const SampleTraceData *m_data = nullptr;
    QList<Item> m_items;      // parallel to TimelineModel item indices
    QList<quint64> m_threads; // per-thread row order (first appearance)
    int m_maxRunning = 1;     // peak concurrent on-CPU threads; the total row's full scale
    qint64 m_traceEndNs = 0;
};

} // namespace QmlProfiler::Internal
