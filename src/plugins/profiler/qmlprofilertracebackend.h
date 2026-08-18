// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profilertracebackend.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer { class BuildConfiguration; }
namespace Timeline { class RangeDetailsWidget; }

namespace Profiler::Internal {

class QmlProfilerClientManager;
class QmlProfilerModelManager;
class QmlProfilerStateManager;

// The QML profiler's models and views for one trace, and the state a live
// recording needs. A trace loaded from a file stays Idle; one recorded into
// this document is driven by the run control through the state and client
// managers.
class QmlProfilerTraceBackend : public ProfilerTraceBackend
{
    Q_OBJECT

public:
    explicit QmlProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                     QObject *parent = nullptr);
    ~QmlProfilerTraceBackend() override;

    QWidgetList views(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() override;

    void load(const Utils::FilePath &path) override;
    bool isSaveable() const override { return true; }
    // Writing runs in the background, so this reports that the write started,
    // not that it finished; saved() marks the point the trace is on disk.
    Utils::Result<> save(const Utils::FilePath &path) override;
    bool isModified() const override;
    void clear() override;
    std::chrono::milliseconds traceDuration() const override;

    QmlProfilerModelManager *modelManager() const;
    QmlProfilerStateManager *stateManager() const;
    QmlProfilerClientManager *clientManager() const;

    // Called when a run starts recording into this trace.
    void prepareRun(const ProjectExplorer::BuildConfiguration *bc, bool aggregateTraces,
                    int flushInterval);
    // The run is over: stop the connection and settle the state machine.
    void handleStop();
    bool aggregatesTraces() const;

    // Discards the recorded data, as the toolbar's Clear button does.
    void clearData();

    // The Stop action for the editor's toolbar; the tool binds it to the run.
    QAction *stopAction() const;

signals:
    // A load or save is running; the editor disables its views meanwhile.
    void busyChanged(bool busy);
    void saved();
    // A new recording is about to discard notes the user has not saved.
    void saveBeforeRecordingRequested();

private:
    void setupToolBar();
    // Whether the trace may be discarded: nothing is unsaved, or the user
    // confirmed losing it.
    bool checkForUnsavedNotes();
    void createTextMarks();
    void clearEvents();
    void updateTimeDisplay();
    void showTimelineSearch();
    void recordingButtonChanged(bool recording);
    void profilerStateChanged();
    void serverRecordingChanged();
    void clientsDisconnected();

    class QmlProfilerTraceBackendPrivate *d;
};

} // namespace Profiler::Internal
