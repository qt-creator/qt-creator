// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ctfvisualizerconstants.h"
#include "ctfvisualizertr.h"

#include <debugger/debuggermainwindow.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>

#include <QScopedPointer>

namespace CtfVisualizer {
namespace Internal {

class CtfTraceManager;
class CtfStatisticsModel;
class CtfStatisticsView;
class CtfTimelineModel;
class CtfVisualizerTraceView;


class CtfVisualizerTool  : public QObject
{
    Q_OBJECT

public:
    CtfVisualizerTool();
    ~CtfVisualizerTool();

    Timeline::TimelineModelAggregator *modelAggregator() const;
    CtfTraceManager *traceManager() const;
    Timeline::TimelineZoomControl *zoomControl() const;

    void loadJson();

private:
    void createViews();

    void initialize();
    void finalize();

    void setAvailableThreads(const QList<CtfTimelineModel *> &threads);
    void toggleThreadRestriction(QAction *action);

    Utils::Perspective m_perspective{Constants::CtfVisualizerPerspectiveId,
                                     ::CtfVisualizer::Tr::tr("Chrome Trace Format Visualizer")};

    bool m_isLoading;
    QScopedPointer<QAction> m_loadJson;

    CtfVisualizerTraceView *m_traceView;
    const QScopedPointer<Timeline::TimelineModelAggregator> m_modelAggregator;
    const QScopedPointer<Timeline::TimelineZoomControl> m_zoomControl;

    const QScopedPointer<CtfStatisticsModel> m_statisticsModel;
    CtfStatisticsView *m_statisticsView;

    const QScopedPointer<CtfTraceManager> m_traceManager;

    QToolButton *const m_restrictToThreadsButton;
    QMenu *const m_restrictToThreadsMenu;
};

} // namespace Internal
} // namespace CtfVisualizer
