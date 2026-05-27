// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertool.h"
#include "perfprofilertraceview.h"
#include "perftimelinemodelmanager.h"

namespace PerfProfiler::Internal {

PerfProfilerTraceView::PerfProfilerTraceView(QWidget *parent, PerfProfilerTool *tool)
    : Timeline::TimelineWidget(&modelManager(), tool->zoomControl(), parent)
{
    setObjectName(QLatin1String("PerfProfilerTraceView"));
}

} // namespace PerfProfiler::Internal
