// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <tracing/timelinewidget.h>

namespace PerfProfiler::Internal {

class PerfProfilerTool;

class PerfProfilerTraceView : public Timeline::TimelineWidget
{
    Q_OBJECT

public:
    PerfProfilerTraceView(QWidget *parent, PerfProfilerTool *tool);
};

} // namespace PerfProfiler::Internal
