// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QElapsedTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QProgressBar;
class QTimer;
QT_END_NAMESPACE

namespace Utils {
class QtcButton;
class QtcLabel;
}

namespace QmlTraceViewer {

// Shown while a recording is in progress: names the sampled process, counts up
// the elapsed time, and offers a Stop button. Recording runs until the user
// stops it.
class RecordingPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingPage(QWidget *parent = nullptr);

    // Resets and starts the elapsed-time counter.
    void start(const QString &processName);
    // Switches to the "processing the captured samples" state: the elapsed timer
    // stops, the Stop button is disabled and a progress bar appears, giving
    // immediate feedback while the worker still converts and writes the trace.
    void setProcessing();
    // Sets the post-processing progress (0..100).
    void setProgress(int percent);
    // Stops the elapsed-time counter.
    void stop();

signals:
    void stopRequested();

private:
    void updateElapsed();

    Utils::QtcLabel *m_titleLabel = nullptr;
    Utils::QtcLabel *m_timerLabel = nullptr;
    Utils::QtcButton *m_stopButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QTimer *m_tick = nullptr;
    QElapsedTimer m_elapsed;
};

} // namespace QmlTraceViewer
