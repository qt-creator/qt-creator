// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofilerconstants.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilerstatisticsview.h"
#include "perfprofilertraceview.h"
#include "perftimelinemodelmanager.h"

#include <debugger/debuggermainwindow.h>
#include <tracing/timelinezoomcontrol.h>
#include <utils/fileinprojectfinder.h>

#include <QLabel>
#include <QToolButton>

namespace ProjectExplorer {
class Kit;
class Project;
class RunControl;
}

namespace PerfProfiler {
namespace Internal {

class PerfProfilerRunner;

class PerfProfilerTool  : public QObject
{
    Q_OBJECT
public:
    PerfProfilerTool();
    ~PerfProfilerTool();

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
    void populateFileFinder(const ProjectExplorer::Project *project,
                            const ProjectExplorer::Kit *kit);
    void updateFilterMenu();
    void updateRunActions();
    void addLoadSaveActionsToMenu(QMenu *menu);
    void createTracePoints();

    void initialize();
    void finalize();

    Utils::Perspective m_perspective{Constants::PerfProfilerPerspectiveId,
                                     QCoreApplication::translate("QtC::PerfProfiler",
                                                                 "Performance Analyzer")};

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
    QList<QObject *> m_objectsToDelete;

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
