// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertracebackend.h"

#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilerstatisticsview.h"
#include "perfprofilertr.h"
#include "perfprofilertracemanager.h"
#include "perftimelinemodelmanager.h"

#include <tracing/rangedetailswidget.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/fileinprojectfinder.h>

#include <QPointer>

using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class PerfProfilerTraceBackendPrivate
{
public:
    PerfProfilerTraceManager traceManager;
    PerfTimelineModelManager modelManager{&traceManager};
    Timeline::TimelineZoomControl zoomControl;
    PerfProfilerFlameGraphModel flameGraphModel{&traceManager};
    FileInProjectFinder fileFinder;

    QPointer<Timeline::RangeDetailsWidget> rangeDetails; // Not owned; shared with
                                                         // the document's other views.
    QPointer<Timeline::TimelineWidget> traceView;
    QPointer<PerfProfilerStatisticsView> statisticsView;
    QPointer<PerfProfilerFlameGraphView> flameGraphView;
};

PerfProfilerTraceBackend::PerfProfilerTraceBackend(Timeline::RangeDetailsWidget *details,
                                                   QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new PerfProfilerTraceBackendPrivate)
{
    d->rangeDetails = details;
    connect(&d->traceManager, &PerfProfilerTraceManager::error,
            this, &PerfProfilerTraceBackend::error);
    connect(&d->traceManager, &PerfProfilerTraceManager::loadFinished, this, [this] {
        emit loadFinished();
        emit traceChanged();
    });
    connect(&d->traceManager, &PerfProfilerTraceManager::saveFinished,
            this, &PerfProfilerTraceBackend::saved);
}

PerfProfilerTraceBackend::~PerfProfilerTraceBackend()
{
    delete d->traceView;
    delete d->statisticsView;
    delete d->flameGraphView;
    delete d;
}

QWidgetList PerfProfilerTraceBackend::views(QWidget *parent)
{
    d->traceView = new Timeline::TimelineWidget(&d->modelManager, &d->zoomControl,
                                                d->rangeDetails, parent);
    d->traceView->setObjectName("PerfProfilerTraceView");
    d->traceView->setWindowTitle(Tr::tr("Timeline"));

    d->flameGraphView = new PerfProfilerFlameGraphView(&d->flameGraphModel);
    d->flameGraphView->setParent(parent);
    d->flameGraphView->setWindowTitle(Tr::tr("Flame Graph"));

    d->statisticsView = new PerfProfilerStatisticsView(&d->traceManager);
    d->statisticsView->setParent(parent);
    d->statisticsView->setWindowTitle(Tr::tr("Statistics"));

    const auto goTo = [this](const QString &file, int line, int column) {
        if (line < 0 || file.isEmpty())
            return;
        FilePath path = FilePath::fromUserInput(file);
        if (!path.isAbsolutePath() || !path.isReadableFile()) {
            const FilePaths found = d->fileFinder.findFile(file);
            if (found.isEmpty())
                return;
            path = found.constFirst();
            if (!path.isReadableFile())
                return;
        }
        // Recorded locations count columns from 1, the editor from 0.
        emit gotoSourceLocation({path, line, column - 1});
    };
    connect(d->traceView, &Timeline::TimelineWidget::gotoSourceLocation, this, goTo);
    connect(d->statisticsView, &PerfProfilerStatisticsView::gotoSourceLocation, this, goTo);
    connect(d->flameGraphView, &PerfProfilerFlameGraphView::gotoSourceLocation, this, goTo);

    connect(d->statisticsView, &PerfProfilerStatisticsView::typeSelected,
            d->traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(d->flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            d->traceView, &Timeline::TimelineWidget::selectByTypeId);
    connect(d->traceView, &Timeline::TimelineWidget::typeSelected,
            d->statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(d->flameGraphView, &PerfProfilerFlameGraphView::typeSelected,
            d->statisticsView, &PerfProfilerStatisticsView::selectByTypeId);
    connect(d->traceView, &Timeline::TimelineWidget::typeSelected,
            d->flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);
    connect(d->statisticsView, &PerfProfilerStatisticsView::typeSelected,
            d->flameGraphView, &PerfProfilerFlameGraphView::selectByTypeId);

    // Route the flame graph's details into the shared range details view.
    connect(d->flameGraphView, &Timeline::FlameGraphWidget::detailsChanged, d->rangeDetails,
            [this](const QString &title, const QList<QPair<QString, QString>> &content) {
        d->rangeDetails->setData(d->flameGraphView, title, content);
    });
    connect(d->flameGraphView, &Timeline::FlameGraphWidget::detailsCleared, d->rangeDetails,
            [this] { d->rangeDetails->clear(d->flameGraphView); });

    return {d->traceView, d->flameGraphView, d->statisticsView};
}

void PerfProfilerTraceBackend::load(const FilePath &path)
{
    d->traceManager.loadFromTraceFile(path);
}

void PerfProfilerTraceBackend::loadPerfData(const FilePath &path, const FilePath &executableDir,
                                            ProjectExplorer::Kit *kit)
{
    d->traceManager.loadFromPerfData(path, executableDir.toUrlishString(), kit);
}

Result<> PerfProfilerTraceBackend::save(const FilePath &path)
{
    d->traceManager.saveToTraceFile(path);
    return ResultOk;
}

void PerfProfilerTraceBackend::clear()
{
    d->traceManager.clearAll();
    d->zoomControl.clear();
}

milliseconds PerfProfilerTraceBackend::traceDuration() const
{
    return duration_cast<milliseconds>(nanoseconds{d->traceManager.traceDuration()});
}

PerfProfilerTraceManager *PerfProfilerTraceBackend::traceManager() const
{
    return &d->traceManager;
}

PerfTimelineModelManager *PerfProfilerTraceBackend::modelManager() const
{
    return &d->modelManager;
}

FileInProjectFinder *PerfProfilerTraceBackend::fileFinder() const
{
    return &d->fileFinder;
}

} // namespace Profiler::Internal
