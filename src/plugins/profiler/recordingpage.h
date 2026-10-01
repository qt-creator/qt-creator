// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <QElapsedTimer>
#include <QWidget>

#include <chrono>

QT_BEGIN_NAMESPACE
class QProgressBar;
class QTimer;
QT_END_NAMESPACE

namespace Utils {
class QtcButton;
class QtcLabel;
}

namespace Profiler::Internal {

// Shown while a recording is in progress: names the sampled process, counts up
// the elapsed time, and offers a Stop button. Recording runs until the user
// stops it.
class PROFILER_EXPORT RecordingPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingPage(QWidget *parent = nullptr);

    // Shows the page for a recording that was asked for but is not capturing
    // yet: the elapsed time stays at zero until captureStarted().
    void showWaiting(const QString &processName);
    // Names the recording and starts counting, so that what the start took --
    // a launch, a debug connection, a consent prompt -- is not counted.
    void captureStarted();
    // As captureStarted(), but counting on from `recorded`, for a page that
    // comes back to a recording that has been capturing for that long.
    void captureRunning(std::chrono::milliseconds recorded);
    // Whether the recording can be paused; the Pause button is hidden otherwise.
    void setPauseSupported(bool supported);
    // Reflects that capture was suspended or continued: the clock stops and
    // the button offers the opposite action. Independent of captureStarted(),
    // since a recording may begin paused.
    void setPaused(bool paused);
    // Switches to the "processing the captured samples" state: the elapsed timer
    // stops, the Stop button is disabled and a progress bar appears, giving
    // immediate feedback while the worker still converts and writes the trace.
    void setProcessing();
    // Sets the post-processing progress (0..100).
    void setProgress(int percent);
    // Adds a line below the progress bar naming what post-processing is busy
    // with, for the steps that take long enough to look like a hang. Pass an
    // empty string to take it away again; the tool tip carries the detail that
    // is too long for the line itself, such as a full request URL.
    void setStatus(const QString &text, const QString &toolTip = {});
    // Stops the elapsed-time counter.
    void stop();

signals:
    void stopRequested();
    void pauseRequested();
    void resumeRequested();

private:
    void updateElapsed();
    void updateTitle();

    Utils::QtcLabel *m_titleLabel = nullptr;
    Utils::QtcLabel *m_timerLabel = nullptr;
    Utils::QtcLabel *m_statusLabel = nullptr;
    Utils::QtcButton *m_stopButton = nullptr;
    Utils::QtcButton *m_pauseButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QTimer *m_tick = nullptr;
    QElapsedTimer m_elapsed;
    qint64 m_recordedMs = 0; // Time recorded before the current stretch.
    bool m_capturing = false;
    bool m_paused = false;
    QString m_processName;
};

} // namespace Profiler::Internal
