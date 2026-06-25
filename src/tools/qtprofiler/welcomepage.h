// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
class QtcButton;
class QtcComboBox;
}

namespace QtProfiler {

// Shown in the trace area while no trace is loaded yet. Lets the user pick a
// recording backend, configure it (each backend renders its own controls,
// including how to start: launch / attach / connect), then Start Recording.
class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);

    // Fills the backend selector. Does not emit backendChanged().
    void setBackends(const QStringList &names, int current);

    // Selects a backend by index, as if the user picked it (emits backendChanged()).
    void setCurrentBackend(int index);

    // Switches to a backend, showing its own configuration controls. Takes
    // ownership of `configWidget` (may be null).
    void setActiveBackend(QWidget *configWidget);

signals:
    void startRecordingRequested();
    void backendChanged(int index);

private:
    Utils::QtcComboBox *m_backendCombo = nullptr;
    QVBoxLayout *m_configLayout = nullptr;
    QWidget *m_configWidget = nullptr;
};

} // namespace QtProfiler
