// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils { class FilePath; }
namespace ProjectExplorer { class RunControl; }

namespace Profiler::Internal {

class PerfProfilerToolPrivate;
class PerfProfilerTraceBackend;

// The Performance Analyzer's menu entries and run-control glue. The trace
// itself lives in a ProfilerTraceDocument; this points a run at one and keeps
// the run actions in step.
class PerfProfilerTool : public QObject
{
    Q_OBJECT

public:
    PerfProfilerTool();
    ~PerfProfilerTool();

    static PerfProfilerTool *instance();

    // The backend the running (or last) session records into, or null when no
    // trace has been opened for a run yet.
    PerfProfilerTraceBackend *liveBackend() const;

    // The entries a perf trace's context menus show; owned by the tool. The
    // backends fetch these themselves, so every path that creates one -- the
    // File > Open editor factory included -- gets the same menus.
    QList<QAction *> traceMenuActions() const;
    QAction *limitToRangeAction() const;
    QAction *showFullRangeAction() const;

    bool isRecording() const;

    void profileStartupProject();

    void onRunControlStarted();
    void onRunControlFinished();
    void onWorkerCreation(ProjectExplorer::RunControl *runControl);

    void loadTraceFile(const Utils::FilePath &filePath);
    void updateTime(qint64 duration, qint64 delay);

signals:
    // A run has opened the trace it records into, before it starts reading.
    void liveBackendChanged(PerfProfilerTraceBackend *backend);

private:
    // The trace the menu actions act on: the one being shown, else the live one.
    PerfProfilerTraceBackend *currentBackend() const;
    PerfProfilerTraceBackend *openLoadedTrace();

    void showLoadPerfDialog();
    void showLoadTraceDialog();
    void showSaveTraceDialog();
    void updateRunActions();
    void createTracePoints();

    PerfProfilerToolPrivate *d = nullptr;
};

void setupPerfProfilerTool();
void destroyPerfProfilerTool();

} // namespace Profiler::Internal
