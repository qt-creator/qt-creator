// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <tracing/timelinewidget.h>

namespace Timeline {
class TimelineModelAggregator;
class TimelineZoomControl;
}

namespace CtfVisualizer::Internal {

class CtfVisualizerTraceView : public Timeline::TimelineWidget
{
    Q_OBJECT

public:
    CtfVisualizerTraceView(QWidget *parent, Timeline::TimelineModelAggregator *aggregator,
                           Timeline::TimelineZoomControl *zoomControl);
    ~CtfVisualizerTraceView() override = default;
};

} // namespace CtfVisualizer::Internal
