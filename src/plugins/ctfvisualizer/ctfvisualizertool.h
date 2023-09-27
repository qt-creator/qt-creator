// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ctfvisualizerconstants.h"

#include <debugger/debuggermainwindow.h>

#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinezoomcontrol.h>

#include <QCoreApplication>
#include <QScopedPointer>

namespace Tasking { class TaskTree; }

namespace CtfVisualizer::Internal {

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

    void loadJson(const QString &fileName);

private:
    void createViews();

    void initialize();
    void finalize();

    void setAvailableThreads(const QList<CtfTimelineModel *> &threads);
    void toggleThreadRestriction(QAction *action);

    Utils::Perspective m_perspective{Constants::CtfVisualizerPerspectiveId,
                                     QCoreApplication::translate("QtC::CtfVisualizer",
                                                                 "Chrome Trace Format Visualizer")};

    std::unique_ptr<Tasking::TaskTree> m_loader;
    QScopedPointer<QAction> m_loadJson;

    CtfVisualizerTraceView *m_traceView = nullptr;
    const QScopedPointer<Timeline::TimelineModelAggregator> m_modelAggregator;
    const QScopedPointer<Timeline::TimelineZoomControl> m_zoomControl;

    const QScopedPointer<CtfStatisticsModel> m_statisticsModel;
    CtfStatisticsView *m_statisticsView = nullptr;

    const QScopedPointer<CtfTraceManager> m_traceManager;

    QToolButton *const m_restrictToThreadsButton;
    QMenu *const m_restrictToThreadsMenu;
};

} // namespace CtfVisualizer::Internal
