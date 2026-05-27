// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizertraceview.h"

#include "ctfvisualizertool.h"

namespace CtfVisualizer::Internal {

CtfVisualizerTraceView::CtfVisualizerTraceView(QWidget *parent, CtfVisualizerTool *tool)
    : Timeline::TimelineWidget(tool->modelAggregator(), tool->zoomControl(), parent)
{
    setObjectName(QLatin1String("CtfVisualizerTraceView"));
}

} // namespace CtfVisualizer::Internal
