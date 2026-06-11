// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

#include <qwindowdefs.h>

#include <QObject>

#include <chrono>

namespace Utils { class FilePath; }

namespace Profiler {

// Headless counterpart to CtfVisualizerTool: hosts the Chrome Trace Format /
// Common Trace Format timeline and statistics views without QtCreator's
// perspective/action-manager infrastructure, for use in the standalone viewer.
class QMLPROFILER_EXPORT CtfPlainViewManager : public QObject
{
    Q_OBJECT

public:
    explicit CtfPlainViewManager(QObject *parent = nullptr);
    ~CtfPlainViewManager();

    QWidgetList views(QWidget *parent);
    QWidget *rangeDetailsWidget() const;
    void loadJson(const Utils::FilePath &file);
    void loadCtf2(const Utils::FilePath &dir);
    void clear();
    std::chrono::milliseconds traceDuration() const;

signals:
    void error(const QString &error);
    void loadFinished();

private:
    class CtfPlainViewManagerPrivate *d;
};

} // namespace Profiler
