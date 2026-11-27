// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE
class QAction;
class QUrl;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class RunControl;
} // namespace ProjectExplorer
namespace Utils { class FilePath; }

namespace Profiler::Internal {

class ProfilerTraceDocument;
class QmlProfilerTraceBackend;

// Overrides for one run's recording. Unset values keep what the run
// configuration and the trace's own feature selection say.
class QmlRecordingOptions
{
public:
    std::optional<quint64> features;
    // Zero turns periodic flushing off.
    std::optional<std::chrono::milliseconds> flushInterval;
};

// The QML profiler's menu entries and run-control glue. The trace itself lives
// in a ProfilerTraceDocument; this points a run at one and keeps the run
// actions in step.
class QmlProfilerTool : public QObject
{
    Q_OBJECT

public:
    QmlProfilerTool();
    ~QmlProfilerTool() override;

    static QmlProfilerTool *instance();

    void finalizeRunControl(ProjectExplorer::RunControl *runControl);
    void handleStop(QmlProfilerTraceBackend *backend);

    ProjectExplorer::RunControl *attachToWaitingApplication();
    ProjectExplorer::RunControl *attachToApplication(ProjectExplorer::Kit *kit,
                                                     const QUrl &serverUrl,
                                                     const QmlRecordingOptions &options = {});

    static QList<QAction *> profilerContextMenuActions();

    // The backend the running (or last) session records into, or null when no
    // trace has been opened for a run yet.
    QmlProfilerTraceBackend *liveBackend() const;
    ProfilerTraceDocument *liveDocument() const;

    static void logState(const QString &msg);
    static void showNonmodalWarning(const QString &warningMsg);

    static QString fileDialogTraceFilesFilter();
    void showSaveDialog();
    void showLoadDialog();
    void loadFile(const Utils::FilePath &filePath);

    void profileStartupProject(const QmlRecordingOptions &options = {});

signals:
    // A run has opened the trace it records into. Emitted before the debug
    // connection is made, so a listener can still configure the session.
    void liveBackendChanged(QmlProfilerTraceBackend *backend);

private:
    void updateRunActions();
    void setRunOptions(ProjectExplorer::RunControl *runControl,
                       const QmlRecordingOptions &options);

    class QmlProfilerToolPrivate;
    QmlProfilerToolPrivate *d;
};

void setupQmlProfilerTool();

} // namespace Profiler::Internal
