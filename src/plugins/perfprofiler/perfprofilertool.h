/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfprofilerconstants.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilerstatisticsview.h"
#include "perfprofilertraceview.h"
#include "perftimelinemodelmanager.h"

#include <debugger/debuggermainwindow.h>
#include <projectexplorer/runconfiguration.h>
#include <tracing/timelinezoomcontrol.h>
#include <utils/fileinprojectfinder.h>

#include <QLabel>
#include <QToolButton>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerRunner;

class PerfProfilerTool  : public QObject
{
    Q_OBJECT
public:
    PerfProfilerTool(QObject *parent = nullptr);
    static PerfProfilerTool *instance();

    PerfProfilerTraceManager *traceManager() const;
    PerfTimelineModelManager *modelManager() const;
    Timeline::TimelineZoomControl *zoomControl() const;

    bool isRecording() const;
    void onReaderFinished();

    QAction *stopAction() const { return m_stopAction; }

    void onRunControlStarted();
    void onRunControlFinished();
    void onReaderStarted();
    void onWorkerCreation(ProjectExplorer::RunControl *runControl);

    void updateTime(qint64 duration, qint64 delay);
    void startLoading();
    void setToolActionsEnabled(bool on);

signals:
    void recordingChanged(bool recording);
    void aggregatedChanged(bool aggregated);
    void viewsCreated();

private:
    void createViews();

    void gotoSourceLocation(QString filePath, int lineNumber, int columnNumber);
    void showLoadPerfDialog();
    void showLoadTraceDialog();
    void showSaveTraceDialog();
    void setRecording(bool recording);
    void setAggregated(bool aggregated);
    void clearUi();
    void clearData();
    void clear();

    friend class PerfProfilerRunner;
    void populateFileFinder(const ProjectExplorer::RunConfiguration *rc = nullptr);
    void updateFilterMenu();
    void updateRunActions();
    void addLoadSaveActionsToMenu(QMenu *menu);
    void createTracePoints();

    void initialize();
    void finalize();

    Utils::Perspective m_perspective{Constants::PerfProfilerPerspectiveId,
                                     tr("Performance Analyzer")};

    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_loadPerfData = nullptr;
    QAction *m_loadTrace = nullptr;
    QAction *m_saveTrace = nullptr;
    QAction *m_limitToRange = nullptr;
    QAction *m_showFullRange = nullptr;
    QToolButton *m_clearButton = nullptr;
    QToolButton *m_recordButton = nullptr;
    QLabel *m_recordedLabel = nullptr;
    QLabel *m_delayLabel = nullptr;
    QToolButton *m_filterButton = nullptr;
    QMenu *m_filterMenu = nullptr;
    QToolButton *m_aggregateButton = nullptr;
    QToolButton *m_tracePointsButton = nullptr;

    PerfProfilerTraceView *m_traceView = nullptr;
    PerfProfilerStatisticsView *m_statisticsView = nullptr;
    PerfProfilerFlameGraphView *m_flameGraphView = nullptr;

    PerfProfilerTraceManager *m_traceManager = nullptr;
    PerfTimelineModelManager *m_modelManager = nullptr;
    Timeline::TimelineZoomControl *m_zoomControl = nullptr;
    Utils::FileInProjectFinder m_fileFinder;
    bool m_readerRunning = false;
    bool m_processRunning = false;
};

} // namespace Internal
} // namespace PerfProfiler
