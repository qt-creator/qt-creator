// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <qwindowdefs.h>

#include <QObject>

#include <chrono>

namespace Timeline { class RangeDetailsWidget; }
namespace Utils { class FilePath; }

namespace Profiler {

// Headless counterpart to CtfVisualizerTool: hosts the Chrome Trace Format /
// Common Trace Format timeline and statistics views without QtCreator's
// perspective/action-manager infrastructure, for use in the standalone viewer.
class PROFILER_EXPORT CtfPlainViewManager : public QObject
{
    Q_OBJECT

public:
    // `details` is the range details panel this manager's views fill. It is owned
    // by the caller, which lets every profiler backend share a single one.
    explicit CtfPlainViewManager(Timeline::RangeDetailsWidget *details,
                                 QObject *parent = nullptr);
    ~CtfPlainViewManager();

    QWidgetList views(QWidget *parent);
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
