// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampletrace.h"

#include <tracing/timelinemodel.h>

namespace Profiler::Internal {

// One thread of a sampled recording as a lane of its own, with one sub lane per
// call stack depth: a frame that stays on the stack over consecutive samples is
// one item from the first of them to the first sample that no longer has it.
class CallStackLaneModel : public Timeline::TimelineModel
{
    Q_OBJECT

public:
    CallStackLaneModel(Timeline::TimelineModelAggregator *parent, quint64 tid);

    // The model keeps the pointer; pass nullptr to clear.
    void setTraceData(const SampleTraceData *data);
    // As above, with the indices of the thread's samples in data->samples, in
    // time order, so that the model does not look at those of other threads.
    void setTraceData(const SampleTraceData *data, const QList<qsizetype> &samples);

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    QRgb color(int index) const override;
    Timeline::RowLabels labels() const override;
    Timeline::ItemDetails details(int index) const override;
    Timeline::OrderedItemDetails orderedDetails(int index) const override;
    QString itemLabel(int index) const override;
    void navigateToDetail(int itemIndex, int detailRow) override;

    void clear() override;

private:
    struct Item
    {
        int depth = 0;
        int label = -1; // Index into the trace's labels.
    };

    QString threadName() const;

    const SampleTraceData *m_data = nullptr;
    quint64 m_tid = 0;
    QList<Item> m_items; // Parallel to the timeline model's items.
    int m_depthCount = 0;
};

} // namespace Profiler::Internal
