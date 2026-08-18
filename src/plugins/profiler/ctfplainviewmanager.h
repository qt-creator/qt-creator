// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <qwindowdefs.h>

#include <QObject>

#include <chrono>
#include <functional>

namespace QtTaskTree { class QTaskTree; }

namespace Timeline { class RangeDetailsWidget; }
namespace Utils { class FilePath; }

namespace Profiler {

namespace Internal { class CtfTraceManager; }

// The Chrome Trace Format / Common Trace Format timeline and statistics views
// for one trace, with no dependency on Qt Creator's action manager. Shared by
// the standalone viewer and CtfTraceBackend.
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
    Internal::CtfTraceManager *traceManager();

    // Called with a load's task tree before it starts, for a frontend that has
    // somewhere to report progress. Qt Creator attaches a Core::TaskProgress;
    // the standalone viewer has no ProgressManager to attach one to, and shows
    // its own indicator instead.
    void setTaskTreeSetup(const std::function<void(QtTaskTree::QTaskTree &)> &setup);
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
