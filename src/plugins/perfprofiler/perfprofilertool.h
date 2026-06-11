// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilerconstants.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilerstatisticsview.h"

#include <coreplugin/perspective.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/timelinewidget.h>

#include <utils/action.h>
#include <utils/fileinprojectfinder.h>

#include <QCoreApplication>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

namespace ProjectExplorer {
class Kit;
class Project;
class RunControl;
}

namespace PerfProfiler::Internal {

class PerfProfilerTool  : public QObject
{
    Q_OBJECT

public:
    PerfProfilerTool();
    ~PerfProfilerTool();

    static PerfProfilerTool *instance();

    bool isRecording() const;
    void onReaderFinished();

    const QAction *stopAction() const { return &m_stopAction; }

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

private:
    void gotoSourceLocation(QString filePath, int lineNumber, int columnNumber);
    void showLoadPerfDialog();
    void showLoadTraceDialog();
    void showSaveTraceDialog();
    void setRecording(bool recording);
    void setAggregated(bool aggregated);
    void clearUi();
    void clearData();
    void clear();

    void populateFileFinder(const ProjectExplorer::Project *project,
                            const ProjectExplorer::Kit *kit);
    void updateFilterMenu();
    void updateRunActions();
    void addLoadSaveActionsToMenu(QMenu *menu);
    void createTracePoints();

    void initialize();
    void finalize();

    Core::Perspective m_perspective {
        Constants::PerfProfilerPerspectiveId,
        QCoreApplication::translate("QtC::PerfProfiler", "Performance Analyzer")
    };

    QAction m_startAction;
    QAction m_stopAction;
    Utils::Action m_loadPerfData;
    Utils::Action m_loadTrace;
    Utils::Action m_saveTrace;
    Utils::Action m_limitToRange;
    Utils::Action m_showFullRange;
    QToolButton m_clearButton;
    QToolButton m_recordButton;
    QLabel m_recordedLabel;
    QLabel m_delayLabel;
    QMenu m_filterMenu;
    QToolButton m_filterButton;
    QToolButton m_aggregateButton;
    QToolButton m_tracePointsButton;

    Timeline::TimelineZoomControl m_zoomControl;
    Timeline::TimelineWidget m_traceView;
    PerfProfilerStatisticsView m_statisticsView;
    PerfProfilerFlameGraphView m_flameGraphView{nullptr};
    Utils::FileInProjectFinder m_fileFinder;
    bool m_readerRunning = false;
    bool m_processRunning = false;
};

void setupPerfProfilerTool();
void destroyPerfProfilerTool();

} // namespace PerfProfiler::Internal
