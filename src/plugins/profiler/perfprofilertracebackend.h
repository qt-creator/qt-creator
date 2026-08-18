// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profilertracebackend.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Project;
}

namespace Timeline { class RangeDetailsWidget; }

namespace Profiler::Internal {

class PerfProfilerTraceManager;
class PerfTimelineModelManager;

// The Performance Analyzer's models and views for one trace, and the state a
// live recording needs.
class PerfProfilerTraceBackend : public ProfilerTraceBackend
{
    Q_OBJECT

public:
    explicit PerfProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                      QObject *parent = nullptr);
    ~PerfProfilerTraceBackend() override;

    QWidgetList views(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() override;

    void load(const Utils::FilePath &path) override;
    // A perf.data file, as recorded by perf itself, converted through perfparser.
    void loadPerfData(const Utils::FilePath &path, const Utils::FilePath &executableDir,
                      ProjectExplorer::Kit *kit);
    bool isSaveable() const override { return true; }
    // Writing runs in the background, so this reports that the write started,
    // not that it finished.
    Utils::Result<> save(const Utils::FilePath &path) override;
    void clear() override;
    std::chrono::milliseconds traceDuration() const override;

    PerfProfilerTraceManager *traceManager() const;
    PerfTimelineModelManager *modelManager() const;

    // Called when a run starts recording into this trace.
    void prepareRun(const ProjectExplorer::Project *project, const ProjectExplorer::Kit *kit);
    bool isRecording() const;
    bool isReaderRunning() const;
    void updateTime(qint64 duration, qint64 delay);

    void restrictToSelectedRange();
    void showFullRange();
    bool isEmpty() const;

    // The Stop action shown in the editor's toolbar; the tool binds it to the run.
    QAction *stopAction() const;

signals:
    // The trace manager and the models are about to go; whoever still holds one
    // has to let go now.
    void aboutToBeDestroyed();
    void recordingChanged(bool recording);
    void aggregatedChanged(bool aggregated);
    // The trace is being read or written; the tool disables its actions meanwhile.
    void busyChanged(bool busy);
    void saved();

private:
    void setupToolBar();
    void populateFileFinder(const ProjectExplorer::Project *project,
                            const ProjectExplorer::Kit *kit);
    void clearUi();
    void updateFilterMenu();
    void setToolActionsEnabled(bool on);
    void initialize();
    void finalize();
    void setRecording(bool recording);
    void setAggregated(bool aggregated);

    class PerfProfilerTraceBackendPrivate *d;
};

} // namespace Profiler::Internal
