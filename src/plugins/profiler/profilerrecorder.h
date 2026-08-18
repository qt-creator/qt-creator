// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"
#include "sampler.h"

#include <utils/environment.h>
#include <utils/filepath.h>

#include <QObject>
#include <QStringList>

#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Utils { class CommandLine; }

namespace Profiler::Internal {

class Sampler;

// Drives the recording backends: which of them are usable here, which one is
// selected, its configuration controls, and the recording itself. It owns no
// GUI, so the standalone viewer and Qt Creator's Profile mode can share one
// implementation; both drive a WelcomePage and a RecordingPage from it.
class PROFILER_EXPORT ProfilerRecorder : public QObject
{
    Q_OBJECT

public:
    explicit ProfilerRecorder(QObject *parent = nullptr);
    ~ProfilerRecorder() override;

    // The backends offered here, in display order. Backends that cannot run in
    // this environment are left out (see Sampler::isAvailable).
    QStringList backendNames() const;

    int currentBackend() const;
    void setCurrentBackend(int index);
    // Selects the backend whose display name contains `name`, case-insensitively.
    // Returns false if none matches.
    bool selectBackend(const QString &name);

    // The current backend's own configuration controls, or null when it has
    // none. The caller takes ownership.
    QWidget *createConfigWidget() const;

    // Points every backend at a target the frontend knows about: the command
    // line passed on the command line, the active run configuration, and so on.
    // Values the user has edited since the last seed are left alone.
    void seedLaunchTarget(const Utils::CommandLine &command,
                          const Utils::FilePath &workingDirectory,
                          const Utils::Environment &environment = {});

    bool isRecording() const;

    void start();
    // Starts, then stops again `duration` after the backend actually goes live,
    // so launch and connect time is not counted against the span.
    void startTimed(std::chrono::milliseconds duration);
    void stop();
    // Stops and pumps events until the recording has wound down. Needed on
    // shutdown: a still-running worker would block the global thread pool at
    // exit and leave the launched process behind.
    void stopAndWait();

    void writeSettings() const;

signals:
    void currentBackendChanged(int index);
    // A recording has begun; `target` names what is being recorded.
    void started(const QString &target);
    // Capture is over, but the backend is still symbolizing and writing.
    void processingStarted();
    void progressChanged(int percent);
    // What post-processing is waiting on, ready for a status line; empty once
    // the wait is over. `toolTip` carries the detail too long to show inline.
    void statusChanged(const QString &text, const QString &toolTip);
    // The recording produced the trace at `tracePath`.
    void finished(const Utils::FilePath &tracePath);
    // `fix` is a system setting the backend says would make recording work,
    // for the frontend to offer next to the message.
    void error(const QString &message, const std::optional<SamplerFix> &fix = {});

private:
    class ProfilerRecorderPrivate *d;
};

} // namespace Profiler::Internal
