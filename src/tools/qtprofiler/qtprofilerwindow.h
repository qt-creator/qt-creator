// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMainWindow>

#include <chrono>

namespace Utils { class FilePath; }

namespace QtProfiler {

class WindowPrivate;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

    void loadTraceFile(const Utils::FilePath &filePath);

    // Selects the recording backend whose display name contains `name`
    // (case-insensitive). Returns false if no backend matches.
    bool selectBackend(const QString &name);

    // Starts recording with the active backend and stops it after `duration`,
    // then loads the captured trace. Used by the --record-for command line option.
    void startTimedRecording(std::chrono::milliseconds duration);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    WindowPrivate *d;
};

} // namespace QtProfiler
