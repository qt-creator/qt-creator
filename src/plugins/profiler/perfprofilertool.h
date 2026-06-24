// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
}

namespace ProjectExplorer {
class Kit;
class Project;
class RunControl;
}

namespace Profiler::Internal {

class PerfProfilerToolPrivate;

class PerfProfilerTool  : public QObject
{
    Q_OBJECT

public:
    PerfProfilerTool();
    ~PerfProfilerTool();

    static PerfProfilerTool *instance();

    bool isRecording() const;

    const QAction *stopAction() const;

    void onRunControlStarted();
    void onRunControlFinished();
    void onWorkerCreation(ProjectExplorer::RunControl *runControl);

    void loadTraceFile(const Utils::FilePath &filePath);
    void updateTime(qint64 duration, qint64 delay);

signals:
    void recordingChanged(bool recording);
    void aggregatedChanged(bool aggregated);

private:
    void setToolActionsEnabled(bool on);
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

    PerfProfilerToolPrivate *d = nullptr;
};

void setupPerfProfilerTool();
void destroyPerfProfilerTool();

} // namespace Profiler::Internal
