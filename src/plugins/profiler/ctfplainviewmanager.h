// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "profiler_global.h"

#include <qwindowdefs.h>

#include <QObject>
#include <QStringList>

#include <chrono>
#include <functional>

namespace QtTaskTree { class QTaskTree; }

namespace Timeline {
class RangeDetailsWidget;
class TimelineZoomControl;
} // namespace Timeline
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
    // What of the trace the timeline shows, which a load states and the reader
    // then zooms and scrolls within.
    Timeline::TimelineZoomControl *zoomControl();

    // Called with a load's task tree before it starts, for a frontend that has
    // somewhere to report progress. Qt Creator attaches a Core::TaskProgress;
    // the standalone viewer has no ProgressManager to attach one to, and shows
    // its own indicator instead.
    void setTaskTreeSetup(const std::function<void(QtTaskTree::QTaskTree &)> &setup);
    void loadJson(const Utils::FilePath &file);
    void loadCtf2(const Utils::FilePath &dir);

    // The tracepoint providers the loaded CTF trace declares, and the ones of
    // them the views show -- all of them for a trace as it was opened. What is
    // shown is stated as the list it is rather than as an empty one standing
    // for everything, so that a reader of it always sees the same answer as a
    // reader of the timeline.
    QStringList traceProviders() const;
    QStringList shownProviders() const;
    // Shows the events of `providers` and of no other, by reading the trace
    // again: what is left out is not kept anywhere, and it is the trace rather
    // than the views that is filtered. Showing none of them is no state to be
    // in, so an empty list is ignored.
    void setShownProviders(const QStringList &providers);

    void clear();
    std::chrono::milliseconds traceDuration() const;

signals:
    void error(const QString &error);
    void loadFinished();
    // The source location of the selected event, as the trace spelled it. Only
    // a trace whose producer recorded one has any.
    void gotoSourceLocation(const QString &file, int line, int column);

private:
    class CtfPlainViewManagerPrivate *d;
};

} // namespace Profiler
