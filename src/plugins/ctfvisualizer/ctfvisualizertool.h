/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
** info@kdab.com, author Tim Henning <tim.henning@kdab.com>
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

#include "ctfvisualizerconstants.h"

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
                                     tr("Chrome Trace Format Visualizer")};

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
