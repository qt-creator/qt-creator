// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace QmlTraceViewer {

// Shown in the trace area while no trace is loaded yet. Offers the user the
// choice between opening an existing trace and recording a new one.
class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);

signals:
    void openTraceRequested();
    void recordTraceRequested();
};

} // namespace QmlTraceViewer
